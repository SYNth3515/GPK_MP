// ================================================================================================
// COOP: admin system (client side). The server decides everything, see server/Admin.c
// ================================================================================================

bool bNetClientIsAdmin = false;

// Reload ships: the server always recreates the ship entities. If this is 1, every client also deletes and creates its own
// copies of the ships (the real "delete and create again on all machines"). COOP s27: ON, the deletion goes the safe way of
// "Delete" of the admin menu now (client\OnClient.c NetClient_ShipEntityRecreate: the foam is made again after it).
int iNetAdminClientReload = 1;

// last environment sent by the server (admin menu)
float fNetCliAdminSpeed = 1.0;	// COOP: the game speed of the server (below 1: 0.75 / 0.5)
float fNetCliAdminWindAngle = 0.0;
float fNetCliAdminWindSpeed = 0.0;
string sNetCliAdminWeather = "";
int iNetCliAdminTime = 0;
int iNetCliAdminWeatherType = 0;

// COOP: "0.5" / "0.75" / "1" / "2" for the messages and the admin menu
// (a float in a string concatenation gives 7 digits in this engine, so the text is built from an int)
string NetClient_AdminSpeedText(float fSpeed)
{
	int i100 = makeint(fSpeed * 100.0 + 0.5);
	int iWhole = i100 / 100;
	int iFrac = i100 - iWhole * 100;
	if (iFrac == 0) { return iWhole + ""; }
	if (iFrac < 10) { return iWhole + ".0" + iFrac; }
	if (iFrac == (iFrac / 10) * 10) { return iWhole + "." + (iFrac / 10); }
	return iWhole + "." + iFrac;
}

void NetClient_AdminReset()
{
	bNetClientIsAdmin = false;
}

void NetClient_AdminLogin(string sPassword)
{
	Trace("COOP: admin login is sent");
	int iSMsg = NMCreate();
	NMAddClientID(iSMsg, wClientID);
	NMAddByte(iSMsg, NC_COMMAND);
	NMAddByte(iSMsg, NSC_COMMAND_ADMIN_LOGIN);
	NMAddString(iSMsg, sPassword, 24);
	NetClient_SendMessage(iSMsg, true);
	NMDelete(iSMsg);
}

// the machine that created the server knows its admin password: it is an admin without typing it
// (empty password = nobody is an admin, the host too)
void NetClient_AdminAutoLogin()
{
	if (!NetServer_IsStarted()) { return; }

	string sPassword = NetServer.AdminPassword;
	if (sPassword == "") { return; }

	NetClient_AdminLogin(sPassword);
}

void NetClient_AdminSend(int iAction, int wTarget, float fValue)
{
	int iSMsg = NMCreate();
	NMAddClientID(iSMsg, wClientID);
	NMAddByte(iSMsg, NC_COMMAND);
	NMAddByte(iSMsg, NSC_COMMAND_ADMIN_CMD);
	NMAddByte(iSMsg, iAction);
	NMAddClientID(iSMsg, wTarget);
	NMAddFloat(iSMsg, fValue);
	NetClient_SendMessage(iSMsg, true);
	NMDelete(iSMsg);
}

// COOP: the "Add" button of the "Бот+" window (server\Bots.c)
void NetClient_AdminBotAdd(int iShip, int iCannon, int iCrew, int iHull, int iSails, int iRel)
{
	int iSMsg = NMCreate();
	NMAddClientID(iSMsg, wClientID);
	NMAddByte(iSMsg, NC_COMMAND);
	NMAddByte(iSMsg, NSC_COMMAND_ADMIN_BOTADD);
	NMAddWord(iSMsg, iShip);
	NMAddByte(iSMsg, iCannon);
	NMAddByte(iSMsg, iCrew);
	NMAddByte(iSMsg, iHull);
	NMAddByte(iSMsg, iSails);
	NMAddByte(iSMsg, iRel);
	NetClient_SendMessage(iSMsg, true);
	NMDelete(iSMsg);
}

// ------------------------------------------------------------------------------------------------
void NetClient_OnCommand(int iMsg)
{
	int iSubCode = NMGetByte(iMsg);

	switch (iSubCode)
	{
		case NSC_COMMAND_ADMIN_STATUS:	NetClient_OnAdminStatus(iMsg);	break;
		case NSC_COMMAND_ADMIN_FLAGS:	NetClient_OnAdminFlags(iMsg);	break;
		case NSC_COMMAND_ADMIN_ENV:		NetClient_OnAdminEnv(iMsg);		break;
		case NSC_COMMAND_SET_WEATHER:	NetClient_OnSetWeather(iMsg);	break;
		case NSC_COMMAND_SET_WIND:		NetClient_OnSetWind(iMsg);		break;
		case NSC_COMMAND_RELOAD_SHIPS:	NetClient_AdminReloadShips();	break;
		case NSC_COMMAND_NOCLIP_SHIP:	NetClient_OnNoclipShip(iMsg);	break;		// COOP s27
	}
}

// COOP s27: a ship in the sea noclip of an admin (server\Admin.c, NetServer_NoclipBroadcast). The patched net.dll does not move a
// ship with NcOn = 1 by itself here either (no waves, no island / ship collisions: the ship of the client was pushed out of the
// island and jumped back with every packet of the server), its height is kept every frame (the server sends only x / z / course).
bool bNetCliNoclipFrame = false;

void NetClient_OnNoclipShip(int iMsg)
{
	int wID = NMGetClientID(iMsg);
	int iOn = NMGetByte(iMsg);
	float fY = NMGetFloat(iMsg);
	float fVY = NMGetFloat(iMsg);
	if (wID < 0 || wID >= NET_TOTALCLIENTS) { return; }
	ref rC = NetClient_GetClient(wID);
	rC.NcOn = 0;
	rC.NcHold = 0;
	if (iOn == 1) { rC.NcOn = 1; }
	if (iOn == 2)
	{
		rC.NcHold = 1;		// after the noclip: the patched net.dll keeps the height of the ship
		if (IsEntity(rC) && CheckAttribute(rC, "Ship.Pos.x"))
		{
			float fAy = 0.0;
			if (CheckAttribute(rC, "Ship.Ang.y")) { fAy = stf(rC.Ship.Ang.y); }
			SendMessage(rC, "lffff", MSG_NET_SHIP_SETPOS, stf(rC.Ship.Pos.x), fY, stf(rC.Ship.Pos.z), fAy);
		}
	}
	rC.NcY = fY;
	rC.NcVY = fVY;
	if (iOn == 1 && !bNetCliNoclipFrame)
	{
		bNetCliNoclipFrame = true;
		SetEventHandler("frame", "NetClient_NoclipFrame", 0);
	}
}

void NetClient_NoclipFrame()
{
	bool bAny = false;
	float fDt = MakeFloat(GetDeltaTime()) * 0.001;
	if (fDt > 0.1) { fDt = 0.1; }
	if (bNetClientInGame)
	{
		for (int i=0; i<NET_TOTALCLIENTS; i++)
		{
			if (!CheckAttribute(&NCClients[i], "NcOn")) { continue; }
			if (sti(NCClients[i].NcOn) != 1) { continue; }
			ref rC = NetClient_GetClient(i);
			if (!IsEntity(rC) || !CheckAttribute(rC, "Ship.Pos.x"))
			{
				rC.NcOn = 0;
				continue;
			}
			bAny = true;
			float fY = stf(rC.NcY) + stf(rC.NcVY) * fDt;
			rC.NcY = fY;
			float fShAy = 0.0;
			if (CheckAttribute(rC, "Ship.Ang.y")) { fShAy = stf(rC.Ship.Ang.y); }
			SendMessage(rC, "lffff", MSG_NET_SHIP_SETPOS, stf(rC.Ship.Pos.x), fY, stf(rC.Ship.Pos.z), fShAy);
		}
	}
	if (!bAny)
	{
		bNetCliNoclipFrame = false;
		DelEventHandler("frame", "NetClient_NoclipFrame");
	}
}

void NetClient_OnAdminStatus(int iMsg)
{
	int iAdmin = NMGetByte(iMsg);

	bNetClientIsAdmin = (iAdmin != 0);
	Trace("COOP: admin status from the server: " + iAdmin);

	Event("NCEvent_AdminStatus", "l", iAdmin);
}

void NetClient_OnAdminFlags(int iMsg)
{
	int wNetClientID = NMGetClientID(iMsg);
	int iAdmin = NMGetByte(iMsg);
	int iMuted = NMGetByte(iMsg);
	int iGod = NMGetByte(iMsg);		// COOP s27: immortality

	ref rClient = NetClient_GetClient(wNetClientID);
	rClient.Admin = iAdmin;
	rClient.Muted = iMuted;
	rClient.God = iGod;

	if (wNetClientID == wClientID) { bNetClientIsAdmin = (iAdmin != 0); }

	Event("NCEvent_AdminFlags");
}

void NetClient_OnAdminEnv(int iMsg)
{
	float fOldSpeed = fNetCliAdminSpeed;
	fNetCliAdminSpeed = NMGetFloat(iMsg);		// COOP: float: 0.5 / 0.75 / 1 / 2 ...
	fNetCliAdminWindAngle = NMGetFloat(iMsg);
	fNetCliAdminWindSpeed = NMGetFloat(iMsg);
	sNetCliAdminWeather = NMGetString(iMsg);
	iNetCliAdminTime = NMGetByte(iMsg);
	iNetCliAdminWeatherType = NMGetByte(iMsg);

	// COOP: a message for the player when the admin changes the game speed (the buttons of the admin menu and the +/- keys)
	if (fOldSpeed != fNetCliAdminSpeed) { Log_SetStringToLog("x" + NetClient_AdminSpeedText(fNetCliAdminSpeed)); }

	Event("NCEvent_AdminEnv");
}

// The admin changed the weather / time of day. Like Whr_UpdateWeather of the single player game: the sea (NCSea) and the
// weather entity (NCWeather) are NOT deleted, ships, foam and rigging use them; their data is replaced by
// NetClient_CreateWeatherEnvironment (it does not create an entity that is already alive). Sky, sun, rain, lightning and
// stars do not depend on ships and are created again.
void NetClient_OnSetWeather(int iMsg)
{
	// COOP: no weather here (lobby of a running game): read the data out of the message, the next message of the packet follows it
	if (!bWeatherLoaded)
	{
		string sSkipWeather = NMGetString(iMsg);
		float fSkipAngle = NMGetFloat(iMsg);
		float fSkipSpeed = NMGetFloat(iMsg);
		return;
	}

	NetClient_WhrDeleteRainEnvironment();
	NetClient_WhrDeleteSkyEnvironment();
	NetClient_WhrDeleteSunGlowEnvironment();
	NetClient_WhrDeleteLightningEnvironment();
	NetClient_WhrDeleteAstronomyEnvironment();

	DelEventHandler("NetClient_WhrOnCalcFogColor", "NetClient_WhrOnCalcFogColor");
	DelEventHandler("WhrCalcFogColor", "NetClient_WhrOnCalcFogColor");
	DelEventHandler("frame", "NetClient_WhrOnWindChange");
	DelEventHandler("frame", "NetClient_UpdateSky");

	NetClient_CreateWeatherEnvironment(iMsg);
}

void NetClient_AdminApplyWind(float fAngle, float fSpeed)
{
	if (COOP_WindBad(fAngle) || COOP_WindBad(fSpeed)) { return; }

	fClientWeatherAngle = fAngle;
	fClientWeatherSpeed = fSpeed;
	fClientWeatherDelta = 0.0;

	if (!bWeatherLoaded) { return; }

	// the same values as in NetClient_WhrOnWindChange
	NCWeather.WindAngle = fAngle;
	NCWeather.WindSpeed = fSpeed;
	Weather.Wind.Angle = fAngle;
	Weather.Wind.Speed = fSpeed;

	Particles.windpower = 0.05 * Clampf(5.0 * fSpeed / WIND_NORMAL_POWER);
	Particles.winddirection.x = sin(fAngle);
	Particles.winddirection.z = cos(fAngle);
}

void NetClient_OnSetWind(int iMsg)
{
	float fAngle = NMGetFloat(iMsg);
	float fSpeed = NMGetFloat(iMsg);

	NetClient_AdminApplyWind(fAngle, fSpeed);
}

// debug: delete and create the ships of this client again (only if iNetAdminClientReload is 1)
void NetClient_AdminReloadShips()
{
	if (iNetAdminClientReload == 0) { return; }

	for (int i=0; i<NET_MAXCLIENTS; i++)
	{
		if (!COOP_CliSlotLive(i)) { continue; }
		if (!sti(NCClients[i].Use)) { continue; }
		if (sti(NCClients[i].Ship.Type) < 0) { continue; }
		if (sti(NCClients[i].Dead)) { continue; }

		ref rC = NetClient_GetClient(i);
		float fRX = stf(rC.Ship.Pos.x);
		float fRZ = stf(rC.Ship.Pos.z);
		if (CheckAttribute(rC, "Ship.NewPos.x"))
		{
			fRX = stf(rC.Ship.NewPos.x);
			fRZ = stf(rC.Ship.NewPos.z);
		}
		NetClient_ShipEntityRecreate(i, fRX, fRZ, stf(rC.Ship.Ang.y), "admin reload");
	}
	RefreshBattleInterface();
}

// ================================================================================================
// COOP s27: free camera of the admin (the button "Freecam" of the admin menu). Only on this machine: the camera flies, the ship / hero
// stands. At sea and on land. W / S forward and back (where the camera looks), A / D to the sides, Space up, Shift down, the
// mouse turns the camera, CapsLock / Ctrl change the speed.
// The camera is FREE_CAMERA of SEA_CAMERAS.dll (the free camera of the engine), patched (s27): it reads the controls NcFwd ...
// NcDown made here, and takes its place from the attributes x, y, z, ax, ay when the attribute gen changes. It is switched on
// by its own SEA_CAMERAS manager (AI_CAMERAS_SET_CAMERA = 55001). The cameras of the game sleep meanwhile (at sea the On of the net
// cameras, on land locCameraSleep), the keys of the ship / hero are locked.
// ================================================================================================
bool bNetFreecam = false;
bool bNetFreecamLand = false;
bool bNetFreecamDeckWasOn = false;
int iNetFreecamGen = 0;
object objNetNoclip;			// the controls made once
object objNetNcCams;			// SEA_CAMERAS (the manager that switches the camera on)
object objNetNcCam;				// FREE_CAMERA
object objNetNcChr;				// an empty "character" for AI_CAMERAS_SET_CAMERA

void NetNoclip_MapKey(string sCtl, string sKey)
{
	if (!CheckAttribute(&objNetNoclip, "ctl." + sCtl))
	{
		int iCode = CreateControl(sCtl);
		objNetNoclip.ctl.(sCtl) = iCode;
		MapControl(iCode, CI_GetKeyCode(sKey));
		SetControlFlags(iCode, 0);
	}
}

void NetFreecam_Lock(bool bLand, bool bLock)
{
	if (bLand)
	{
		LockControl("ChrForward", bLock);
		LockControl("ChrForward1", bLock);
		LockControl("ChrForward2", bLock);
		LockControl("ChrBackward", bLock);
		LockControl("ChrBackward1", bLock);
		LockControl("ChrBackward2", bLock);
		LockControl("ChrStrafeLeft", bLock);
		LockControl("ChrStrafeRight", bLock);
		LockControl("ChrJump", bLock);
		LockControl("ChrRun", bLock);
		LockControl("ChrFightMode", bLock);
		LockControl("ChrFire", bLock);
		LockControl("ChrTurnH", bLock);
		LockControl("ChrTurnH1", bLock);
		LockControl("ChrCamTurnH", bLock);
		LockControl("ChrCamTurnV", bLock);
		LockControl("ChrAction", bLock);
		return;
	}
	LockControl("Ship_TurnLeft", bLock);
	LockControl("Ship_TurnRight", bLock);
	LockControl("Ship_SailUp", bLock);
	LockControl("Ship_SailDown", bLock);
	LockControl("Ship_Fire", bLock);
	LockControl("Sea_CameraSwitch", bLock);
	LockControl("BIFastCommand", bLock);
	LockControl("TelescopeIn", bLock);
	LockControl("TelescopeOut", bLock);
	LockControl("ShipCamera_Turn_V", bLock);
	LockControl("ShipCamera_Turn_H", bLock);
	LockControl("ShipCamera_Forward", bLock);
	LockControl("ShipCamera_Backward", bLock);
	LockControl("DeckCamera_Turn_V", bLock);
	LockControl("DeckCamera_Turn_H", bLock);
	LockControl("DeckCamera_Forward", bLock);
	LockControl("DeckCamera_Backward", bLock);
}

void NetFreecam_Toggle()
{
	if (bNetFreecam) { NetFreecam_Off(); return; }

	float x = 0.0;
	float y = 0.0;
	float z = 0.0;
	float ay = 0.0;
	bool bLand = NetLand_IsActive();
	if (bLand)
	{
		ref mc = GetMainCharacter();
		if (!IsEntity(mc)) { return; }
		GetCharacterPos(mc, &x, &y, &z);
		GetCharacterAy(mc, &ay);
		y = y + 2.5;
	}
	else
	{
		if (!bNetClientInGame) { Log_SetStringToLog("Freecam: сначала войдите в игру"); return; }
		if (!IsEntity(&NCShipCamera)) { return; }
		ref rMe = NetClient_GetClient(wClientID);
		if (!CheckAttribute(rMe, "Ship.Pos.x")) { return; }
		x = stf(rMe.Ship.Pos.x);
		z = stf(rMe.Ship.Pos.z);
		y = 20.0;
		if (CheckAttribute(rMe, "Ship.Ang.y")) { ay = stf(rMe.Ship.Ang.y); }
		// a little behind the ship, looking at it
		x = x - sin(ay) * 40.0;
		z = z - cos(ay) * 40.0;
	}

	NetNoclip_MapKey("NcFwd", "KEY_W");
	NetNoclip_MapKey("NcBack", "KEY_S");
	NetNoclip_MapKey("NcLeft", "KEY_A");
	NetNoclip_MapKey("NcRight", "KEY_D");
	NetNoclip_MapKey("NcUp", "VK_SPACE");
	NetNoclip_MapKey("NcDown", "VK_SHIFT");

	DeleteClass(&objNetNcCams);
	DeleteClass(&objNetNcCam);
	objNetNcCam.Perspective = 1.285;
	iNetFreecamGen++;
	objNetNcCam.x = x;
	objNetNcCam.y = y;
	objNetNcCam.z = z;
	objNetNcCam.ax = 0.3;		// a little down
	objNetNcCam.ay = ay;
	objNetNcCam.gen = iNetFreecamGen;
	CreateEntity(&objNetNcCams, "SEA_CAMERAS");
	CreateEntity(&objNetNcCam, "FREE_CAMERA");
	if (!IsEntity(&objNetNcCam) || !IsEntity(&objNetNcCams))
	{
		Trace("COOP: freecam: FREE_CAMERA / SEA_CAMERAS can not be created");
		DeleteClass(&objNetNcCam);
		DeleteClass(&objNetNcCams);
		return;
	}
	if (bLand)
	{
		LayerAddObject("execute", &objNetNcCam, 1000000);
		locCameraSleep(true);
	}
	else
	{
		LayerAddObject("net_execute", &objNetNcCam, 1000000);
		bNetFreecamDeckWasOn = false;
		if (CheckAttribute(&NCDeckCamera, "On")) { bNetFreecamDeckWasOn = sti(NCDeckCamera.On); }
		if (bNetFreecamDeckWasOn) { Telescope_Off(); }
		NCDeckCamera.On = false;
		NCShipCamera.On = false;
	}
	SendMessage(&objNetNcCams, "li", 55000, &objNetNcCam);
	SendMessage(&objNetNcCams, "lia", 55001, &objNetNcCam, &objNetNcChr);

	bNetFreecam = true;
	bNetFreecamLand = bLand;
	NetFreecam_Lock(bLand, true);
	SetEventHandler("frame", "NetFreecam_Frame", 0);
	Log_SetStringToLog("Freecam включен: W/S/A/D, Пробел вверх, Shift вниз, мышь - обзор");
	Trace("COOP: freecam on (land " + bLand + ") at " + x + "," + y + "," + z);
}

void NetFreecam_Off()
{
	if (!bNetFreecam) { return; }
	bNetFreecam = false;
	DelEventHandler("frame", "NetFreecam_Frame");
	NetFreecam_Lock(bNetFreecamLand, false);
	DeleteClass(&objNetNcCams);
	DeleteClass(&objNetNcCam);
	if (bNetFreecamLand)
	{
		locCameraSleep(false);
		locCameraFollow();
	}
	else
	{
		if (bNetFreecamDeckWasOn) { NetClient_SetDeckCameraOn(); }
		else { NetClient_SetShipCameraOn(); }
	}
	Log_SetStringToLog("Freecam выключен");
	Trace("COOP: freecam off");
}

// the mode is gone (the duel ended, the sea was left): the noclip goes off by itself
void NetFreecam_Frame()
{
	if (!bNetFreecam) { return; }
	if (bNetFreecamLand)
	{
		if (!NetLand_IsActive()) { NetFreecam_Off(); }
		return;
	}
	if (!bNetClientInGame) { NetFreecam_Off(); return; }
	// the net cameras stay off (a death / respawn may switch one on)
	if (CheckAttribute(&NCShipCamera, "On"))
	{
		if (sti(NCShipCamera.On)) { NCShipCamera.On = false; }
	}
	if (CheckAttribute(&NCDeckCamera, "On"))
	{
		if (sti(NCDeckCamera.On)) { NCDeckCamera.On = false; }
	}
}

// ================================================================================================
// COOP s27: noclip of the admin (the button "Ноуклип" of the admin menu), land duel: the hero is moved by a teleport every frame
// by a step: W / S forward and back (where the hero looks), A / D to the sides, Q / E turn, Space up, Shift down. CapsLock makes
// the step bigger, Ctrl smaller (every press). The walking keys of the hero are locked meanwhile. The keys are own controls
// (NcFwd ...) read with Control Activation / Deactivation (they fire for them: the log shows "ctl NcLeft").
// ================================================================================================
#define NET_NOCLIP_STEP			0.15	// m per frame at the start
#define NET_NOCLIP_STEP_MIN		0.01
#define NET_NOCLIP_STEP_MAX		5.0
#define NET_NOCLIP_STEP_K		1.5		// CapsLock / Ctrl change the step this many times
#define NET_NOCLIP_TURN			0.04	// rad per frame (Q / E)

bool bNetNoclip = false;
bool bNcFwd = false;
bool bNcBack = false;
bool bNcLeft = false;
bool bNcRight = false;
bool bNcUp = false;
bool bNcDown = false;
bool bNcTurnL = false;
bool bNcTurnR = false;
float fNcStep = NET_NOCLIP_STEP;
float fNcX = 0.0;
float fNcY = 0.0;
float fNcZ = 0.0;
float fNcAy = 0.0;
// COOP s27: the sea noclip (the server moves the ship: server\Admin.c, NetServer_AdminNoclip)
#define NET_NOCLIP_SEA_STEP		1.0		// m per frame at the start
#define NET_NOCLIP_SEA_STEP_MIN	0.1
#define NET_NOCLIP_SEA_STEP_MAX	30.0
bool bNetNoclipSea = false;
float fNcSeaStep = 0.0;
int iNcSeaSent = -1;

void NetNoclip_LockShip(bool bLock)
{
	LockControl("Ship_TurnLeft", bLock);
	LockControl("Ship_TurnRight", bLock);
	LockControl("Ship_SailUp", bLock);
	LockControl("Ship_SailDown", bLock);
	LockControl("Ship_Fire", bLock);		// Space: up
}

void NetNoclip_LockHero(bool bLock)
{
	LockControl("ChrForward", bLock);
	LockControl("ChrForward1", bLock);
	LockControl("ChrForward2", bLock);
	LockControl("ChrBackward", bLock);
	LockControl("ChrBackward1", bLock);
	LockControl("ChrBackward2", bLock);
	LockControl("ChrStrafeLeft", bLock);
	LockControl("ChrStrafeRight", bLock);
	LockControl("ChrJump", bLock);
	LockControl("ChrRun", bLock);
	LockControl("ChrFightMode", bLock);
	LockControl("ChrFire", bLock);
}

void NetNoclip_Toggle()
{
	if (bNetNoclip) { NetNoclip_Off(); return; }
	bNetNoclipSea = false;
	if (!NetLand_IsActive())
	{
		if (!bNetClientInGame) { Log_SetStringToLog("Ноуклип: сначала войдите в игру"); return; }
		ref rMe = NetClient_GetClient(wClientID);
		if (sti(rMe.Dead)) { Log_SetStringToLog("Ноуклип: ваш корабль потоплен"); return; }
		bNetNoclipSea = true;
	}
	else
	{
	ref mc = GetMainCharacter();
	if (!IsEntity(mc)) { return; }
	float x = 0.0;
	float y = 0.0;
	float z = 0.0;
	float ay = 0.0;
	GetCharacterPos(mc, &x, &y, &z);
	GetCharacterAy(mc, &ay);
	fNcX = x;
	fNcY = y;
	fNcZ = z;
	fNcAy = ay;
	}
	if (fNcStep < NET_NOCLIP_STEP_MIN) { fNcStep = NET_NOCLIP_STEP; }		// the global start value does not take a define here
	if (fNcSeaStep < NET_NOCLIP_SEA_STEP_MIN) { fNcSeaStep = NET_NOCLIP_SEA_STEP; }
	iNcSeaSent = -1;

	NetNoclip_MapKey("NcFwd", "KEY_W");
	NetNoclip_MapKey("NcBack", "KEY_S");
	NetNoclip_MapKey("NcLeft", "KEY_A");
	NetNoclip_MapKey("NcRight", "KEY_D");
	NetNoclip_MapKey("NcUp", "VK_SPACE");
	NetNoclip_MapKey("NcDown", "VK_SHIFT");
	NetNoclip_MapKey("NcTurnL", "KEY_Q");
	NetNoclip_MapKey("NcTurnR", "KEY_E");
	NetNoclip_MapKey("NcFaster", "VK_CAPSLOCK");
	NetNoclip_MapKey("NcSlower", "VK_CONTROL");

	bNcFwd = false;
	bNcBack = false;
	bNcLeft = false;
	bNcRight = false;
	bNcUp = false;
	bNcDown = false;
	bNcTurnL = false;
	bNcTurnR = false;
	bNetNoclip = true;
	if (bNetNoclipSea) { NetNoclip_LockShip(true); }
	else { NetNoclip_LockHero(true); }
	SetEventHandler("Control Activation", "NetNoclip_CtrlOn", 0);
	SetEventHandler("Control Deactivation", "NetNoclip_CtrlOff", 0);
	SetEventHandler("frame", "NetNoclip_Frame", 0);
	if (bNetNoclipSea)
	{
		NetNoclip_SeaSend();
		Log_SetStringToLog("Ноуклип корабля включен: W/S вперёд-назад, A/D вбок, Q/E поворот, Пробел вверх, Shift вниз, CapsLock/Ctrl шаг");
		Trace("COOP: sea noclip on, step " + fNcSeaStep);
		return;
	}
	Log_SetStringToLog("Ноуклип включен: W/S/A/D, Q/E поворот, Пробел вверх, Shift вниз, CapsLock/Ctrl шаг");
	Trace("COOP: noclip on at " + fNcX + "," + fNcY + "," + fNcZ + " step " + fNcStep);
}

void NetNoclip_Off()
{
	if (!bNetNoclip) { return; }
	bNetNoclip = false;
	DelEventHandler("Control Activation", "NetNoclip_CtrlOn");
	DelEventHandler("Control Deactivation", "NetNoclip_CtrlOff");
	DelEventHandler("frame", "NetNoclip_Frame");
	if (bNetNoclipSea)
	{
		NetNoclip_LockShip(false);
		if (bNetClientInGame) { NetClient_AdminSend(NET_ADMIN_NOCLIP, wClientID, -1.0); }
		bNetNoclipSea = false;
	}
	else { NetNoclip_LockHero(false); }
	Log_SetStringToLog("Ноуклип выключен");
	Trace("COOP: noclip off");
}

void NetNoclip_SetKey(string sCtl, bool bOn)
{
	switch (sCtl)
	{
		case "NcFwd": bNcFwd = bOn; break;
		case "NcBack": bNcBack = bOn; break;
		case "NcLeft": bNcLeft = bOn; break;
		case "NcRight": bNcRight = bOn; break;
		case "NcUp": bNcUp = bOn; break;
		case "NcDown": bNcDown = bOn; break;
		case "NcTurnL": bNcTurnL = bOn; break;
		case "NcTurnR": bNcTurnR = bOn; break;
		case "NcFaster":
			if (bOn && bNetNoclipSea)
			{
				fNcSeaStep = fNcSeaStep * NET_NOCLIP_STEP_K;
				if (fNcSeaStep > NET_NOCLIP_SEA_STEP_MAX) { fNcSeaStep = NET_NOCLIP_SEA_STEP_MAX; }
				Log_SetStringToLog("Ноуклип: шаг " + makeint(fNcSeaStep * 100.0) + " см за кадр");
			}
			if (bOn && !bNetNoclipSea)
			{
				fNcStep = fNcStep * NET_NOCLIP_STEP_K;
				if (fNcStep > NET_NOCLIP_STEP_MAX) { fNcStep = NET_NOCLIP_STEP_MAX; }
				Log_SetStringToLog("Ноуклип: шаг " + makeint(fNcStep * 100.0) + " см за такт");
			}
		break;
		case "NcSlower":
			if (bOn && bNetNoclipSea)
			{
				fNcSeaStep = fNcSeaStep / NET_NOCLIP_STEP_K;
				if (fNcSeaStep < NET_NOCLIP_SEA_STEP_MIN) { fNcSeaStep = NET_NOCLIP_SEA_STEP_MIN; }
				Log_SetStringToLog("Ноуклип: шаг " + makeint(fNcSeaStep * 100.0) + " см за кадр");
			}
			if (bOn && !bNetNoclipSea)
			{
				fNcStep = fNcStep / NET_NOCLIP_STEP_K;
				if (fNcStep < NET_NOCLIP_STEP_MIN) { fNcStep = NET_NOCLIP_STEP_MIN; }
				Log_SetStringToLog("Ноуклип: шаг " + makeint(fNcStep * 100.0) + " см за такт");
			}
		break;
	}
}

void NetNoclip_CtrlOn()
{
	string sCtl = GetEventData();
	if (!bNetNoclip) { return; }
	NetNoclip_SetKey(sCtl, true);
}

void NetNoclip_CtrlOff()
{
	string sCtl = GetEventData();
	if (!bNetNoclip) { return; }
	NetNoclip_SetKey(sCtl, false);
}

// the sea: the keys and the step go to the server when they change (the server moves the ship every frame)
void NetNoclip_SeaSend()
{
	int iMask = 0;
	if (bNcFwd) { iMask = iMask + 1; }
	if (bNcBack) { iMask = iMask + 2; }
	if (bNcLeft) { iMask = iMask + 4; }
	if (bNcRight) { iMask = iMask + 8; }
	if (bNcTurnL) { iMask = iMask + 16; }
	if (bNcTurnR) { iMask = iMask + 32; }
	if (bNcUp) { iMask = iMask + 64; }
	if (bNcDown) { iMask = iMask + 128; }
	int iVal = iMask + 256 * makeint(fNcSeaStep * 100.0 + 0.5);
	if (iVal == iNcSeaSent) { return; }
	iNcSeaSent = iVal;
	Trace("COOP: sea noclip keys " + iMask + " step " + fNcSeaStep);
	NetClient_AdminSend(NET_ADMIN_NOCLIP, wClientID, makefloat(iVal));
}

void NetNoclip_Frame()
{
	if (!bNetNoclip) { return; }
	if (bNetNoclipSea)
	{
		if (!bNetClientInGame || NetLand_IsActive()) { NetNoclip_Off(); return; }
		ref rMe = NetClient_GetClient(wClientID);
		if (sti(rMe.Dead)) { NetNoclip_Off(); return; }
		NetNoclip_SeaSend();
		return;
	}
	if (!NetLand_IsActive()) { NetNoclip_Off(); return; }
	ref mc = GetMainCharacter();
	if (!IsEntity(mc)) { return; }

	if (bNcTurnL) { fNcAy = fNcAy - NET_NOCLIP_TURN; }
	if (bNcTurnR) { fNcAy = fNcAy + NET_NOCLIP_TURN; }

	float fSin = sin(fNcAy);
	float fCs = cos(fNcAy);
	if (bNcFwd) { fNcX = fNcX + fSin * fNcStep; fNcZ = fNcZ + fCs * fNcStep; }
	if (bNcBack) { fNcX = fNcX - fSin * fNcStep; fNcZ = fNcZ - fCs * fNcStep; }
	if (bNcRight) { fNcX = fNcX + fCs * fNcStep; fNcZ = fNcZ - fSin * fNcStep; }
	if (bNcLeft) { fNcX = fNcX - fCs * fNcStep; fNcZ = fNcZ + fSin * fNcStep; }
	if (bNcUp) { fNcY = fNcY + fNcStep; }
	if (bNcDown) { fNcY = fNcY - fNcStep; }
	// the hero stands where the noclip put him (the engine may pull him down to the ground between the frames)
	TeleportCharacterToPosAy(mc, fNcX, fNcY, fNcZ, fNcAy);
}
