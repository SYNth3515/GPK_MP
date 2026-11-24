// ================================================================================================
// COOP: admin system (server side)
//
// The admin password is set on the "Create game" screen. EMPTY PASSWORD = NOBODY CAN BECOME AN ADMIN.
// A player becomes an admin by sending the password (lobby: "Admin" button, game: Esc menu -> Admin menu).
// The machine that runs the server logs in automatically with its own password (see client/Admin.c).
// An admin can also make another player an admin (for this game only: the flag lives in NSClients).
//
// Every admin command arrives in NetServer_OnAdminCommand and is checked against NSClients[].Admin.
// ================================================================================================

// Switches. Set to false if the action freezes / crashes the game on your build.
//
// Teleport and "reload ships": the engine has no "set ship position" message and the attributes Ship.Pos/Ang are only
// READ when the ship is created, so the ship entity of the SERVER is deleted and created again at the new place
// (the clients only show the ships, they get the new position with the normal ship info packets).
// If it is false, the buttons do nothing.
bool bNetAdminAllowRecreateShip = true;

// Weather / time of day: applied in the running game like Whr_UpdateWeather of the single player game: the sea and the weather
// entity are kept alive (only their data is replaced), sky / sun / rain / lightning / stars are created again on the clients.
// The sea of the SERVER keeps the waves of the old weather. If it is false, the buttons do nothing (wind and speed still work).
bool bNetAdminAllowWeather = true;

// distance in front of the bow of the other ship
float fNetAdminTeleportDist = 70.0;

int iNetAdminTimeIdx = 0;		// 0 morning, 1 day, 2 evening, 3 night (same as the lists on the Create game screen)
int iNetAdminWeatherIdx = 0;	// 0 calm, 1 windy, 2 heavy, 3 storm

// ------------------------------------------------------------------------------------------------
bool NetServer_IsAdmin(int wNetClientID)
{
	if (wNetClientID < 0 || wNetClientID >= NET_MAXCLIENTS) { return false; }
	if (!sti(NSClients[wNetClientID].Use)) { return false; }
	return sti(NSClients[wNetClientID].Admin) != 0;
}

bool NetServer_AdminTargetOk(int wTarget)
{
	if (wTarget < 0 || wTarget >= NET_MAXCLIENTS) { return false; }
	return sti(NSClients[wTarget].Use) != 0;
}

// target exists, the game is running and its ship is alive
bool NetServer_AdminShipOk(int wTarget)
{
	if (!bServerGameStarted) { return false; }
	if (!NetServer_AdminTargetOk(wTarget)) { return false; }
	if (!NetServer_HasShip(wTarget)) { return false; }		// COOP: a lobby player of the running game has no ship
	if (sti(NSClients[wTarget].Ship.Type) < 0) { return false; }
	if (sti(NSClients[wTarget].Dead)) { return false; }
	return true;
}

float NetServer_AdminNormAngle(float fAngle)
{
	float f = fAngle;
	while (f >= PIm2) { f = f - PIm2; }
	while (f < 0.0) { f = f + PIm2; }
	return f;
}

// ------------------------------------------------------------------------------------------------
// messages to the clients
// ------------------------------------------------------------------------------------------------
void NetServer_AdminSendStatus(int wToClientID)
{
	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_COMMAND);
	NMAddByte(iSMsg, NSC_COMMAND_ADMIN_STATUS);
	NMAddByte(iSMsg, NetServer_IsAdmin(wToClientID));
	NetServer_SendMessage(wToClientID, iSMsg, true);
	NMDelete(iSMsg);
}

// wToClientID may be DST_ALL
void NetServer_AdminSendFlags(int wToClientID, int wAboutClientID)
{
	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_COMMAND);
	NMAddByte(iSMsg, NSC_COMMAND_ADMIN_FLAGS);
	NMAddClientID(iSMsg, wAboutClientID);
	NMAddByte(iSMsg, sti(NSClients[wAboutClientID].Admin));
	NMAddByte(iSMsg, sti(NSClients[wAboutClientID].Muted));
	NMAddByte(iSMsg, NetServer_IsGod(wAboutClientID));		// COOP s27: immortality
	NetServer_SendMessage(wToClientID, iSMsg, true);
	NMDelete(iSMsg);
}

// weather id -> (time of day, weather type), same table as on the Create game screen
string NetServer_AdminWeatherID(int iTime, int iType)
{
	if (iType == 4) { iType = 3; }	// COOP: the Tornado weather uses the id of the storm
	switch (iTime)
	{
		case 0:
			switch (iType)
			{
				case 0: return "6 Hours"; break;
				case 1: return "8 Hour"; break;
				case 2: return "10 Hours"; break;
				case 3: return "Storm01"; break;
			}
		break;
		case 1:
			switch (iType)
			{
				case 0: return "11 Hour"; break;
				case 1: return "14 Hour"; break;
				case 2: return "16 Nour"; break;
				case 3: return "Storm02"; break;
			}
		break;
		case 2:
			switch (iType)
			{
				case 0: return "19 Hour"; break;
				case 1: return "22 Hour"; break;
				case 2: return "23 Hour"; break;
				case 3: return "Storm03"; break;
			}
		break;
		case 3:
			switch (iType)
			{
				case 0: return "0 Hours"; break;
				case 1: return "1 hour"; break;
				case 2: return "3 Hours"; break;
				case 3: return "Storm04"; break;
			}
		break;
	}
	return "11 Hour";
}
// COOP s47: the waves follow the weather type (calm / breeze / swell), not the sky of the time of day: the waves of the
// table skies did not fit the types (day and night: calm = breeze, evening: the calm had the biggest waves). Each type takes
// the waves of one weather of the table; calm is also capped (MaxSeaHeight, the engine scales Amp1 + Amp2 down to it, as
// the single player game does near towns), so the sea is nearly flat. Storms and unknown ids keep their own waves.
// The server (NSSea, the ship physics) and the clients (NCSea) call it with the same id, no net message changes.
int NetWeather_TypeOfID(string sId)
{
	for (int iT=0; iT<4; iT++)
	{
		for (int iW=0; iW<3; iW++)
		{
			if (NetServer_AdminWeatherID(iT, iW) == sId) { return iW; }
		}
	}
	return -1;
}

string NetWeather_WavesID(int iType)
{
	switch (iType)
	{
		case 0: return "6 Hours"; break;	// Amp 5 + 1, capped to 1.5
		case 1: return "0 Hours"; break;	// Amp 10 + 1
		case 2: return "10 Hours"; break;	// Amp 25 + 1
	}
	return "";
}

void NetWeather_SetWaves(ref rSea, string sWeatherID)
{
	int iType = NetWeather_TypeOfID(sWeatherID);
	string sW = NetWeather_WavesID(iType);
	if (sW == "") { return; }
	int iW = -1;
	for (int i=0; i<iTotalNumWeathers; i++)
	{
		if (Weathers[i].id == sW) { iW = i; break; }
	}
	if (iW < 0) { Trace("COOP: waves weather " + sW + " not found"); return; }
	aref aSea2; makearef(aSea2, Weathers[iW].Sea2);

	rSea.Sea2.Amp1 = Whr_GetFloat(aSea2, "Amp1");
	rSea.Sea2.AnimSpeed1 = Whr_GetFloat(aSea2, "AnimSpeed1");
	rSea.Sea2.Scale1 = Whr_GetFloat(aSea2, "Scale1");
	rSea.Sea2.MoveSpeed1 = Whr_GetString(aSea2, "MoveSpeed1");
	rSea.Sea2.Amp2 = Whr_GetFloat(aSea2, "Amp2");
	rSea.Sea2.AnimSpeed2 = Whr_GetFloat(aSea2, "AnimSpeed2");
	rSea.Sea2.Scale2 = Whr_GetFloat(aSea2, "Scale2");
	rSea.Sea2.MoveSpeed2 = Whr_GetString(aSea2, "MoveSpeed2");
	rSea.Sea2.PosShift = Whr_GetFloat(aSea2, "PosShift");

	rSea.MaxSeaHeight = 50.0;
	if (iType == 0) { rSea.MaxSeaHeight = 1.5; }
	Trace("COOP: waves of " + sWeatherID + " type " + iType + " from " + sW + " MaxSeaHeight " + rSea.MaxSeaHeight);
}

void NetServer_AdminSyncWeatherIdx()
{
	for (int iT=0; iT<4; iT++)
	{
		for (int iW=0; iW<4; iW++)
		{
			if (NetServer_AdminWeatherID(iT, iW) == NetServer.WeatherID)
			{
				iNetAdminTimeIdx = iT;
				iNetAdminWeatherIdx = iW;
				if (iW == 3 && CheckAttribute(&NetServer, "Tornado") && sti(NetServer.Tornado) != 0) { iNetAdminWeatherIdx = 4; }
				return;
			}
		}
	}
}

// game speed, wind, weather: for the admin menu
void NetServer_AdminSendEnv(int wToClientID)
{
	if (!bServerGameStarted) { return; }

	NetServer_AdminSyncWeatherIdx();

	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_COMMAND);
	NMAddByte(iSMsg, NSC_COMMAND_ADMIN_ENV);
	NMAddFloat(iSMsg, stf(NetServer.GameSpeed));		// COOP: float: the speed goes below 1 (0.75 / 0.5)
	NMAddFloat(iSMsg, fServerWeatherAngle);
	NMAddFloat(iSMsg, fServerWeatherSpeed);
	NMAddString(iSMsg, NetServer.WeatherID, 64);
	NMAddByte(iSMsg, iNetAdminTimeIdx);
	NMAddByte(iSMsg, iNetAdminWeatherIdx);
	NetServer_SendMessage(wToClientID, iSMsg, true);
	NMDelete(iSMsg);
}

void NetServer_AdminSendEnvToAdmins()
{
	for (int i=0; i<NET_MAXCLIENTS; i++)
	{
		if (!NetServer_IsAdmin(i)) { continue; }
		NetServer_AdminSendEnv(i);
	}
}

// ------------------------------------------------------------------------------------------------
// login
// ------------------------------------------------------------------------------------------------
void NetServer_OnAdminLogin(int wNetClientID, int iMsg)
{
	string sPassword = NMGetString(iMsg);

	if (!NetServer_AdminTargetOk(wNetClientID)) { Trace("COOP: admin login from " + wNetClientID + ": no such player"); return; }

	ref rClient = NetServer_GetClient(wNetClientID);

	string sAdminPassword = NetServer.AdminPassword;

	// empty admin password on the server = nobody can be an admin
	if (sAdminPassword != "" && sPassword == sAdminPassword)
	{
		rClient.Admin = true;
		NetServer_AdminSendFlags(DST_ALL, wNetClientID);
		NetServer_AdminSendEnv(wNetClientID);
		Trace("COOP: admin login from " + wNetClientID + ": ok");
	}
	else { Trace("COOP: admin login from " + wNetClientID + ": a wrong password"); }

	NetServer_AdminSendStatus(wNetClientID);
}

// ------------------------------------------------------------------------------------------------
// actions
// ------------------------------------------------------------------------------------------------
void NetServer_AdminKick(int wAdmin, int wTarget)
{
	if (wTarget == wAdmin) { return; }
	if (!NetServer_AdminTargetOk(wTarget)) { return; }

	// the same message the server sends when the game is closed: the client leaves the lobby / game by itself
	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_CONNECT);
	NMAddByte(iSMsg, NSC_CONNECT_DISCONNECT);
	NetServer_SendMessage(wTarget, iSMsg, true);
	NMDelete(iSMsg);

	// and remove it right now (all clients get NSC_CLIENT_DROP, the lobby table is updated at once)
	NetServer_DropClient(wTarget);
	NSClients[wTarget].Parked = false;		// COOP: a kicked player can not take his ship back by reconnecting
}

void NetServer_AdminMute(int wTarget)
{
	if (!NetServer_AdminTargetOk(wTarget)) { return; }

	NSClients[wTarget].Muted = !sti(NSClients[wTarget].Muted);
	NetServer_AdminSendFlags(DST_ALL, wTarget);
}

// COOP s27: immortality of a player (the button of the admin menu)
int NetServer_IsGod(int i)
{
	if (i < 0 || i >= NET_MAXCLIENTS) { return 0; }
	if (NetServer_ShipSafe(i)) { return 1; }		// COOP s37: the ships of a boarding duel are not hurt (net\Board.c), s41: a prize and its captor (s47f: 15 s)
	if (!CheckAttribute(&NSClients[i], "God")) { return 0; }
	if (sti(NSClients[i].God)) { return 1; }
	return 0;
}

void NetServer_AdminGod(int wTarget)
{
	if (!NetServer_AdminTargetOk(wTarget)) { return; }
	if (NetServer_IsGod(wTarget)) { NSClients[wTarget].God = false; } else { NSClients[wTarget].God = true; }
	NetServer_AdminSendFlags(DST_ALL, wTarget);
	Trace("COOP: admin: immortality of " + wTarget + " = " + NetServer_IsGod(wTarget));
}

void NetServer_AdminMakeAdmin(int wTarget)
{
	if (!NetServer_AdminTargetOk(wTarget)) { return; }

	NSClients[wTarget].Admin = true;
	NetServer_AdminSendFlags(DST_ALL, wTarget);
	NetServer_AdminSendStatus(wTarget);
	NetServer_AdminSendEnv(wTarget);
}

void NetServer_AdminHeal(int wTarget)
{
	// COOP: a sunk ship comes back (the respawn) by "Heal"
	if (bServerGameStarted && NetServer_AdminTargetOk(wTarget))
	{
		if (NetServer_HasShip(wTarget))
		{
			if (sti(NSClients[wTarget].Dead)) { NetServer_RespawnShip(wTarget); return; }
		}
	}
	if (!NetServer_AdminShipOk(wTarget)) { return; }

	ref rT = NetServer_GetClient(wTarget);
	ref rBase = Net_GetShipByIndex(sti(rT.Ship.Type));

	rT.Ship.HP = stf(rT.Ship.MaxHP);
	rT.Ship.SP = 100.0;

	float fMaxCrew = stf(rBase.MaxCrew);
	if (stf(rT.Ship.Crew.Quantity) < fMaxCrew) { rT.Ship.Crew.Quantity = fMaxCrew; }
	NetServer_ShipRebuildHeal(wTarget);		// COOP s27: the fallen masts and the sails with holes come back whole (the ship is created again)
}

void NetServer_AdminHealAll()
{
	for (int i=0; i<NET_MAXCLIENTS; i++) { NetServer_AdminHeal(i); }
}

void NetServer_AdminDestroy(int wTarget)
{
	if (!NetServer_AdminShipOk(wTarget)) { return; }

	// no killer: the same as a lightning strike
	NetServer_SetClientDead(NetServer_GetClient(wTarget), NETKILL_BY_LIGHTNING, DST_INVALID);
}

// Puts the ship to the new place: the ship entity of the server is deleted and created again (see the note at the top).
// This is the same sequence as COOP_SrvRecreateShip in client/Time.c, which works on this build.
void NetServer_AdminPlaceShip(int i, float fX, float fZ, float fAy)
{
	if (!bNetAdminAllowRecreateShip) { return; }

	ref rClient = NetServer_GetClient(i);
	ref rShip = Net_GetShipByIndex(sti(rClient.Ship.Type));

	rClient.Ship.Pos.x = fX;
	rClient.Ship.Pos.y = 0.0;		// a new ship must never start with an old / NaN height
	rClient.Ship.Pos.z = fZ;
	rClient.Ship.Ang.y = NetServer_AdminNormAngle(fAy);
	rClient.Ship.Speed.x = 0.0;
	rClient.Ship.Speed.y = 0.0;
	rClient.Ship.Speed.z = 0.0;
	rClient.LastCollisionShip = -500;
	rClient.LastCollisionIsland = -500;
	rClient.CoopFix = 0;			// the NaN height watch (client/Time.c) may recreate this ship again if it needs to

	DeleteClass(&NSClients[i]);

	// the same as in NetServer_StartServerGame
	CreateEntity(&NSClients[i], "NetShip");
	SendMessage(&NSClients[i], "la", MSG_SHIP_CREATE, rShip);
	LayerAddObject("net_execute", &NSClients[i], 31);
	LayerAddObject("ship_cannon_trace", &NSClients[i], 31);

	// A ship created in the middle of the game can get a NaN height like at the start: run the watch of the first seconds again
	bool bWatchRunning = (iCoopSrvWatchTicks < 14);
	iCoopSrvWatchTicks = 0;
	if (!bWatchRunning) { PostEvent("COOP_SrvShipWatch", 700); }
}

// the point beside the ship rT (abeam, on the side where the ship rFrom is now), the same course; fNetAdminSideGap metres
// between the hulls -> fNetAdminSideX / Z / Ay
float fNetAdminSideX = 0.0;
float fNetAdminSideZ = 0.0;
float fNetAdminSideAy = 0.0;
float fNetAdminSideGap = 60.0;

void NetServer_AdminSidePoint(ref rFrom, ref rT)
{
	float fAy = stf(rT.Ship.Ang.y);
	float fRX = cos(fAy);			// the right side of the heading is (cos ay, -sin ay)
	float fRZ = 0.0 - sin(fAy);
	float fSide = 1.0;
	float fDot = (stf(rFrom.Ship.Pos.x) - stf(rT.Ship.Pos.x)) * fRX + (stf(rFrom.Ship.Pos.z) - stf(rT.Ship.Pos.z)) * fRZ;
	if (fDot < 0.0) { fSide = -1.0; }
	float fDist = NetServer_GetShipHalfWidth(rFrom) + NetServer_GetShipHalfWidth(rT) + fNetAdminSideGap;
	fNetAdminSideX = stf(rT.Ship.Pos.x) + fRX * fSide * fDist;
	fNetAdminSideZ = stf(rT.Ship.Pos.z) + fRZ * fSide * fDist;
	fNetAdminSideAy = fAy;
}

// The admin ship goes beside the target (abeam, the same course, on the side where the admin is now)
void NetServer_AdminTeleportToPlayer(int wAdmin, int wTarget)
{
	if (wAdmin == wTarget) { return; }
	if (!NetServer_AdminShipOk(wAdmin)) { return; }
	if (!NetServer_AdminShipOk(wTarget)) { return; }

	NetServer_AdminSidePoint(NetServer_GetClient(wAdmin), NetServer_GetClient(wTarget));
	NetServer_AdminPlaceShip(wAdmin, fNetAdminSideX, fNetAdminSideZ, fNetAdminSideAy);
}

// The other direction: the target goes beside the admin (abeam, the same course, on the side where the target is now).
// COOP s30: it stood 70 m from the centre of the admin, bow to bow: the hulls touched or rammed at once (the host
// crashed right after "player to me" with the admin at 49 m/s)
void NetServer_AdminBringPlayer(int wAdmin, int wTarget)
{
	if (wAdmin == wTarget) { return; }
	if (!NetServer_AdminShipOk(wAdmin)) { return; }
	if (!NetServer_AdminShipOk(wTarget)) { return; }

	NetServer_AdminSidePoint(NetServer_GetClient(wTarget), NetServer_GetClient(wAdmin));
	NetServer_AdminPlaceShip(wTarget, fNetAdminSideX, fNetAdminSideZ, fNetAdminSideAy);
}

// debug: delete and create all ships again, at the same places
void NetServer_AdminReloadShips()
{
	if (!bServerGameStarted) { return; }
	if (!bNetAdminAllowRecreateShip) { return; }

	for (int i=0; i<NET_MAXCLIENTS; i++)
	{
		if (!NetServer_AdminShipOk(i)) { continue; }

		ref rC = NetServer_GetClient(i);
		NetServer_ShipRecreateLater(i, stf(rC.Ship.Pos.x), stf(rC.Ship.Pos.z), stf(rC.Ship.Ang.y));		// COOP s27: the new ship some frames later (server\OnShip.c)
	}

	// clients: see NetClient_AdminReloadShips
	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_COMMAND);
	NMAddByte(iSMsg, NSC_COMMAND_RELOAD_SHIPS);
	NetServer_SendMessage(DST_ALL, iSMsg, true);
	NMDelete(iSMsg);
}

// ------------------------------------------------------------------------------------------------
// environment
// ------------------------------------------------------------------------------------------------
void NetServer_AdminChangeWeather(int iTime, int iType)
{
	if (!bNetAdminAllowWeather) { return; }
	if (!bServerGameStarted) { return; }

	if (iTime > 3) { iTime = 0; }
	if (iTime < 0) { iTime = 3; }
	if (iType > 4) { iType = 0; }
	if (iType < 0) { iType = 4; }

	string sWeatherID = NetServer_AdminWeatherID(iTime, iType);

	int iNum = -1;
	for (int i=0; i<iTotalNumWeathers; i++)
	{
		if (sWeatherID == Weathers[i].id) { iNum = i; break; }
	}
	if (iNum < 0) { return; }

	iNetAdminTimeIdx = iTime;
	iNetAdminWeatherIdx = iType;
	NetServer.WeatherID = sWeatherID;
	NetServer.Tornado = 0;
	if (iType == 4) { NetServer.Tornado = 1; }
	iServerCurWeatherNum = iNum;

	// Like Whr_UpdateWeather of the single player game: the sea and the weather entity of the server are NOT deleted (the
	// ships and their physics use them, deleting them crashed the game), only the data of the new weather is applied.
	aref aCurWeather = NetServer_GetCurrentWeather();

	sServerLightingPath = Whr_GetString(aCurWeather, "Lighting");
	bServerWeatherIsNight = Whr_GetLong(aCurWeather, "Night");
	bServerWeatherIsStorm = Whr_GetLong(aCurWeather, "Storm");
	bServerWhrTornado = Whr_GetLong(aCurWeather, "Tornado");

	float fAngle = fServerWeatherAngle;		// COOP s45: the wind keeps its random course (the angle of the weather is always the same)
	float fSpeed = Whr_GetFloat(aCurWeather, "Wind.Speed");
	if (NetServer_WhrIsStorm()) { fSpeed = 7.0; }
	if (iType == 4 && NetServer_WhrIsStorm()) { fSpeed = fCoopTornadoWind; }

	// NetServer_WhrOnWindChange builds the wind of the server every frame from these values
	NSWeather.WindAngle = fAngle;
	NSWeather.WindSpeed = fSpeed;
	fServerWeatherDelta = 0.0;
	fServerWeatherAngle = fAngle;
	fServerWeatherSpeed = fSpeed;

	// tornado: switched on / off with the weather (sends the state to the clients)
	NetServer_TornadoSetMode(iType == 4 && NetServer_WhrIsStorm());

	// lightning of the new weather (these two only add / remove the event handler)
	NetServer_WhrDeleteLightningEnvironment();
	NetServer_WhrCreateLightningEnvironment();

	// clients: weather id and base wind, the same layout as in the start message
	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_COMMAND);
	NMAddByte(iSMsg, NSC_COMMAND_SET_WEATHER);
	NMAddString(iSMsg, sWeatherID, 64);
	NMAddFloat(iSMsg, fAngle);
	NMAddFloat(iSMsg, fSpeed);
	NetServer_SendMessage(DST_ALL, iSMsg, true);
	NMDelete(iSMsg);

	NetServer_AdminSendEnvToAdmins();
}

void NetServer_AdminSetWind(float fAngle, float fSpeed)
{
	if (!bServerGameStarted) { return; }

	float fNewAngle = NetServer_AdminNormAngle(fAngle);
	float fNewSpeed = fSpeed;
	if (fNewSpeed < 0.0) { fNewSpeed = 0.0; }
	if (fNewSpeed > 30.0) { fNewSpeed = 30.0; }

	// NetServer_WhrOnWindChange builds the wind of the server every frame from these two values
	fServerWeatherAngle = fNewAngle;
	fServerWeatherSpeed = fNewSpeed;

	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_COMMAND);
	NMAddByte(iSMsg, NSC_COMMAND_SET_WIND);
	NMAddFloat(iSMsg, fNewAngle);
	NMAddFloat(iSMsg, fNewSpeed);
	NetServer_SendMessage(DST_ALL, iSMsg, true);
	NMDelete(iSMsg);

	NetServer_AdminSendEnvToAdmins();
}

// COOP: the game speed of the server (the +/- buttons of the admin menu and the +/- keys of the game)
// 1x / 2x / ... / 8x, and below 1x the step is 0.25: 0.75x and 0.5x (the ships sail slower: server/OnShip.c)
void NetServer_AdminChangeSpeed(float fDelta)
{
	float fSpeed = stf(NetServer.GameSpeed);

	if (fDelta > 0.0)
	{
		if (fSpeed < 1.0) { fSpeed = fSpeed + 0.25; }		// 0.5 -> 0.75 -> 1
		else { fSpeed = fSpeed + 1.0; }
	}
	else
	{
		if (fSpeed <= 1.0) { fSpeed = fSpeed - 0.25; }	// 1 -> 0.75 -> 0.5
		else { fSpeed = fSpeed - 1.0; }
	}

	if (fSpeed < 0.5) { fSpeed = 0.5; }
	if (fSpeed > 8.0) { fSpeed = 8.0; }

	NetServer.GameSpeed = fSpeed;
	NetServer_BallsSpeedChanged();		// COOP: the balls fly faster with the ships (server\OnBall.c)

	NetServer_AdminSendEnvToAdmins();
}

// ------------------------------------------------------------------------------------------------
// COOP s27: the sea noclip of the admin. The client sends its keys when they change (client\Admin.c, NetNoclip_*), the server
// moves the ship every frame by a step (W / S along the course, A / D to the sides, Q / E turn) with the message of the patched
// net.dll MSG_NET_SHIP_SETPOS: the ship is put there at once with no speed (no recreation). Space / Shift: up / down (the sky, under
// the water). The patched net.dll does not move a ship with NcOn = 1 by itself (no sails, waves, islands, collisions).
// ------------------------------------------------------------------------------------------------
#define NET_NOCLIP_SEA_TURN		0.02		// rad per frame (Q / E)
bool bNetSrvNoclipFrame = false;

int NetServer_NoclipBit(int iMask, int iBit)
{
	return (iMask / iBit) - (iMask / (iBit * 2)) * 2;
}

// all the clients: the ship of wID is in the noclip (its height and the speed up / down) or not (client\Admin.c)
void NetServer_NoclipBroadcast(int wID)
{
	ref rC = NetServer_GetClient(wID);
	int iOn = NetServer_NcAttr(rC, "NcOn");
	float fVY = 0.0;
	if (iOn != 1 && NetServer_NcAttr(rC, "NcHold") == 1) { iOn = 2; }		// after the noclip: the height stays
	if (iOn == 1)
	{
		int iMask = sti(rC.NcMask);
		float fV = stf(rC.NcStep) * 60.0;
		if (NetServer_NoclipBit(iMask, 64) == 1) { fVY = fVY + fV; }
		if (NetServer_NoclipBit(iMask, 128) == 1) { fVY = fVY - fV; }
	}
	float fY = 0.0;
	if (CheckAttribute(rC, "NcY")) { fY = stf(rC.NcY); }
	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_COMMAND);
	NMAddByte(iSMsg, NSC_COMMAND_NOCLIP_SHIP);
	NMAddClientID(iSMsg, wID);
	NMAddByte(iSMsg, iOn);
	NMAddFloat(iSMsg, fY);
	NMAddFloat(iSMsg, fVY);
	NetServer_SendMessage(DST_ALL, iSMsg, true);
	NMDelete(iSMsg);
}

int NetServer_NcAttr(ref rC, string sName)
{
	if (!CheckAttribute(rC, sName)) { return 0; }
	return sti(rC.(sName));
}

// the player turned the noclip on in this round: the NaN height watch does not recreate his ship (client\Time.c)
bool NetServer_NoclipUsed(ref rC)
{
	if (!CheckAttribute(rC, "NcUsedRound")) { return false; }
	return sti(rC.NcUsedRound) == sti(NetServer.Round);
}

void NetServer_AdminNoclip(int wAdmin, float fValue)
{
	if (wAdmin < 0 || wAdmin >= NET_MAXCLIENTS) { return; }
	ref rC = NetServer_GetClient(wAdmin);
	if (fValue < 0.0)
	{
		rC.NcOn = 0;
		rC.NcHold = 1;		// the patched net.dll keeps the height where the noclip left the ship (till the death / the next round)
		NetServer_NoclipBroadcast(wAdmin);
		Trace("COOP: sea noclip off for " + wAdmin);
		return;
	}
	int iV = makeint(fValue + 0.5);
	int iSteps = iV / 256;
	if (!CheckAttribute(rC, "NcOn")) { rC.NcOn = 0; }
	if (sti(rC.NcOn) != 1) { rC.NcY = stf(rC.Ship.Pos.y); }
	rC.NcOn = 1;
	rC.NcHold = 0;
	rC.NcMask = iV - iSteps * 256;
	rC.NcStep = makefloat(iSteps) / 100.0;
	rC.NcUsedRound = sti(NetServer.Round);
	NetServer_NoclipBroadcast(wAdmin);
	Trace("COOP: sea noclip keys " + sti(rC.NcMask) + " step " + rC.NcStep + " for " + wAdmin);
	if (!bNetSrvNoclipFrame)
	{
		bNetSrvNoclipFrame = true;
		SetEventHandler("frame", "NetServer_NoclipFrame", 0);
		Trace("COOP: sea noclip on for " + wAdmin);
	}
}

void NetServer_NoclipFrame()
{
	bool bAny = false;
	float fK = MakeFloat(GetDeltaTime()) * 0.06;		// the steps are per frame of 60 fps
	if (fK > 6.0) { fK = 6.0; }
	if (bServerGameStarted)
	{
		for (int i=0; i<NET_MAXCLIENTS; i++)
		{
			ref rC = NetServer_GetClient(i);
			int iNcOn = NetServer_NcAttr(rC, "NcOn");
			if (iNcOn != 1 && NetServer_NcAttr(rC, "NcHold") != 1) { continue; }
			bool bNcEnd = false;
			if (sti(rC.Use) == 0) { bNcEnd = true; }
			if (sti(rC.Dead) != 0) { bNcEnd = true; }
			if (!IsEntity(rC)) { bNcEnd = true; }
			if (!NetServer_NoclipUsed(rC)) { bNcEnd = true; }
			if (bNcEnd)
			{
				rC.NcOn = 0;
				rC.NcHold = 0;		// the death / the next round: the waves again
				NetServer_NoclipBroadcast(i);
				continue;
			}
			bAny = true;
			if (iNcOn != 1) { continue; }
			int iMask = sti(rC.NcMask);
			float fStep = stf(rC.NcStep) * fK;
			float fX = stf(rC.Ship.Pos.x);
			float fY = stf(rC.NcY);
			float fZ = stf(rC.Ship.Pos.z);
			float fAy = stf(rC.Ship.Ang.y);
			if (NetServer_NoclipBit(iMask, 16) == 1) { fAy = fAy - NET_NOCLIP_SEA_TURN * fK; }
			if (NetServer_NoclipBit(iMask, 32) == 1) { fAy = fAy + NET_NOCLIP_SEA_TURN * fK; }
			float fSin = sin(fAy);
			float fCs = cos(fAy);
			if (NetServer_NoclipBit(iMask, 1) == 1) { fX = fX + fSin * fStep; fZ = fZ + fCs * fStep; }
			if (NetServer_NoclipBit(iMask, 2) == 1) { fX = fX - fSin * fStep; fZ = fZ - fCs * fStep; }
			if (NetServer_NoclipBit(iMask, 8) == 1) { fX = fX + fCs * fStep; fZ = fZ - fSin * fStep; }
			if (NetServer_NoclipBit(iMask, 4) == 1) { fX = fX - fCs * fStep; fZ = fZ + fSin * fStep; }
			if (NetServer_NoclipBit(iMask, 64) == 1) { fY = fY + fStep; }
			if (NetServer_NoclipBit(iMask, 128) == 1) { fY = fY - fStep; }
			fAy = NetServer_AdminNormAngle(fAy);
			rC.NcY = fY;
			rC.Ship.Pos.x = fX;
			rC.Ship.Pos.y = fY;
			rC.Ship.Pos.z = fZ;
			rC.Ship.Ang.y = fAy;
			rC.Ship.Speed.x = 0.0;
			rC.Ship.Speed.z = 0.0;
			SendMessage(rC, "lffff", MSG_NET_SHIP_SETPOS, fX, fY, fZ, fAy);		// every frame: the ship stands still between the keys
		}
	}
	if (!bAny)
	{
		bNetSrvNoclipFrame = false;
		DelEventHandler("frame", "NetServer_NoclipFrame");
	}
}

// ------------------------------------------------------------------------------------------------
// the single entry point of all admin commands
// ------------------------------------------------------------------------------------------------
void NetServer_OnAdminCommand(int wNetClientID, int iMsg)
{
	int iAction = NMGetByte(iMsg);
	int wTarget = NMGetClientID(iMsg);
	float fValue = NMGetFloat(iMsg);

	// not an admin: ignore (a modified client can send anything)
	if (!NetServer_IsAdmin(wNetClientID)) { Trace("COOP: admin command " + iAction + " from " + wNetClientID + ": not an admin"); return; }
	if (iAction != NET_ADMIN_SYNC) { Trace("COOP: admin command " + iAction + " from " + wNetClientID + " target " + wTarget + " value " + fValue); }

	// COOP: a land duel has players and no ships (Land.c)
	if (iAction != NET_ADMIN_SYNC && NetServer_LandAdmin(wNetClientID, iAction, wTarget, fValue)) { return; }

	switch (iAction)
	{
		case NET_ADMIN_SYNC:
			for (int i=0; i<NET_MAXCLIENTS; i++)
			{
				if (!sti(NSClients[i].Use)) { continue; }
				NetServer_AdminSendFlags(wNetClientID, i);
			}
			NetServer_AdminSendEnv(wNetClientID);
			NetServer_BotSendOpts(wNetClientID);		// COOP: the switch of the bot respawn
		break;
		case NET_ADMIN_KICK:			NetServer_AdminKick(wNetClientID, wTarget);					break;
		case NET_ADMIN_MUTE:			NetServer_AdminMute(wTarget);								break;
		case NET_ADMIN_GOD:				NetServer_AdminGod(wTarget);								break;
		case NET_ADMIN_NOCLIP:			NetServer_AdminNoclip(wNetClientID, fValue);				break;		// COOP s27
		case NET_ADMIN_TP_TO:			NetServer_AdminTeleportToPlayer(wNetClientID, wTarget);		break;
		case NET_ADMIN_TP_BRING:		NetServer_AdminBringPlayer(wNetClientID, wTarget);			break;
		case NET_ADMIN_MAKE_ADMIN:		NetServer_AdminMakeAdmin(wTarget);							break;
		case NET_ADMIN_RELOAD_SHIPS:	NetServer_AdminReloadShips();								break;
		case NET_ADMIN_HEAL:			NetServer_AdminHeal(wTarget);								break;
		case NET_ADMIN_ALL_HEAL:		NetServer_AdminHealAll();									break;
		case NET_ADMIN_DESTROY:			NetServer_AdminDestroy(wTarget);							break;
		case NET_ADMIN_WEATHER:			NetServer_AdminChangeWeather(iNetAdminTimeIdx, iNetAdminWeatherIdx + makeint(fValue));	break;
		case NET_ADMIN_TIME:			NetServer_AdminChangeWeather(iNetAdminTimeIdx + makeint(fValue), iNetAdminWeatherIdx);	break;
		case NET_ADMIN_SPEED:			NetServer_AdminChangeSpeed(fValue);				break;
		case NET_ADMIN_WIND_ANGLE:		NetServer_AdminSetWind(fServerWeatherAngle + fValue, fServerWeatherSpeed);	break;
		case NET_ADMIN_WIND_SPEED:		NetServer_AdminSetWind(fServerWeatherAngle, fServerWeatherSpeed + fValue);	break;
		// COOP: bots (server\Bots.c)
		case NET_ADMIN_BOT_REL:			NetServer_AdminBotAction(wNetClientID, iAction, wTarget, fValue);	break;
		case NET_ADMIN_BOT_BRING:		NetServer_AdminBotAction(wNetClientID, iAction, wTarget, fValue);	break;
		case NET_ADMIN_BOT_GOTO:		NetServer_AdminBotAction(wNetClientID, iAction, wTarget, fValue);	break;
		case NET_ADMIN_BOT_HEAL:		NetServer_AdminBotAction(wNetClientID, iAction, wTarget, fValue);	break;
		case NET_ADMIN_BOT_DESTROY:		NetServer_AdminBotAction(wNetClientID, iAction, wTarget, fValue);	break;
		case NET_ADMIN_BOT_DELETE:		NetServer_AdminBotAction(wNetClientID, iAction, wTarget, fValue);	break;
		case NET_ADMIN_BOT_RESPAWN:		NetServer_BotSetRespawn(makeint(fValue + 0.5));						break;
		case NET_ADMIN_BOT_SKILL:		NetServer_BotSetSkill(makeint(fValue + 0.5));						break;
		case NET_ADMIN_LATEJOIN:		NetServer_SetLateJoin(makeint(fValue + 0.5));						break;		// COOP s30
		case NET_ADMIN_TOLOBBY:			NetServer_ReturnToLobby();											break;		// COOP s30 (server\OnStart.c)
		case NET_ADMIN_LBOT_DEL:		NetServer_LBotDel(makeint(fValue + 0.5));							break;		// COOP s30 (server\Bots.c)
	}
}
