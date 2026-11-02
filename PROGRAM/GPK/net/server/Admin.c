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
	NetServer_SendMessage(wToClientID, iSMsg, true);
	NMDelete(iSMsg);
}

// weather id -> (time of day, weather type), same table as on the Create game screen
string NetServer_AdminWeatherID(int iTime, int iType)
{
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
	NMAddByte(iSMsg, makeint(stf(NetServer.GameSpeed)));
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

	if (!NetServer_AdminTargetOk(wNetClientID)) { return; }

	ref rClient = NetServer_GetClient(wNetClientID);

	string sAdminPassword = NetServer.AdminPassword;

	// empty admin password on the server = nobody can be an admin
	if (sAdminPassword != "" && sPassword == sAdminPassword)
	{
		rClient.Admin = true;
		NetServer_AdminSendFlags(DST_ALL, wNetClientID);
		NetServer_AdminSendEnv(wNetClientID);
	}

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
}

void NetServer_AdminMute(int wTarget)
{
	if (!NetServer_AdminTargetOk(wTarget)) { return; }

	NSClients[wTarget].Muted = !sti(NSClients[wTarget].Muted);
	NetServer_AdminSendFlags(DST_ALL, wTarget);
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
	if (!NetServer_AdminShipOk(wTarget)) { return; }

	ref rT = NetServer_GetClient(wTarget);
	ref rBase = Net_GetShipByIndex(sti(rT.Ship.Type));

	rT.Ship.HP = stf(rT.Ship.MaxHP);
	rT.Ship.SP = 100.0;

	float fMaxCrew = stf(rBase.MaxCrew);
	if (stf(rT.Ship.Crew.Quantity) < fMaxCrew) { rT.Ship.Crew.Quantity = fMaxCrew; }
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

// The admin ship goes to fNetAdminTeleportDist in front of the bow of the target; the bow of the admin looks at the target
void NetServer_AdminTeleportToPlayer(int wAdmin, int wTarget)
{
	if (wAdmin == wTarget) { return; }
	if (!NetServer_AdminShipOk(wAdmin)) { return; }
	if (!NetServer_AdminShipOk(wTarget)) { return; }

	ref rT = NetServer_GetClient(wTarget);
	float fAy = stf(rT.Ship.Ang.y);
	float fX = stf(rT.Ship.Pos.x) + sin(fAy) * fNetAdminTeleportDist;
	float fZ = stf(rT.Ship.Pos.z) + cos(fAy) * fNetAdminTeleportDist;

	NetServer_AdminPlaceShip(wAdmin, fX, fZ, fAy + PI);
}

// The same technique for the other direction: the target goes in front of the bow of the admin, bow to the admin
void NetServer_AdminBringPlayer(int wAdmin, int wTarget)
{
	if (wAdmin == wTarget) { return; }
	if (!NetServer_AdminShipOk(wAdmin)) { return; }
	if (!NetServer_AdminShipOk(wTarget)) { return; }

	ref rA = NetServer_GetClient(wAdmin);
	float fAy = stf(rA.Ship.Ang.y);
	float fX = stf(rA.Ship.Pos.x) + sin(fAy) * fNetAdminTeleportDist;
	float fZ = stf(rA.Ship.Pos.z) + cos(fAy) * fNetAdminTeleportDist;

	NetServer_AdminPlaceShip(wTarget, fX, fZ, fAy + PI);
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
		NetServer_AdminPlaceShip(i, stf(rC.Ship.Pos.x), stf(rC.Ship.Pos.z), stf(rC.Ship.Ang.y));
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
	if (iType > 3) { iType = 0; }
	if (iType < 0) { iType = 3; }

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
	iServerCurWeatherNum = iNum;

	// Like Whr_UpdateWeather of the single player game: the sea and the weather entity of the server are NOT deleted (the
	// ships and their physics use them, deleting them crashed the game), only the data of the new weather is applied.
	aref aCurWeather = NetServer_GetCurrentWeather();

	sServerLightingPath = Whr_GetString(aCurWeather, "Lighting");
	bServerWeatherIsNight = Whr_GetLong(aCurWeather, "Night");
	bServerWeatherIsStorm = Whr_GetLong(aCurWeather, "Storm");
	bServerWhrTornado = Whr_GetLong(aCurWeather, "Tornado");

	float fAngle = Whr_GetFloat(aCurWeather, "Wind.Angle");
	float fSpeed = Whr_GetFloat(aCurWeather, "Wind.Speed");
	if (NetServer_WhrIsStorm()) { fSpeed = 7.0; }

	// NetServer_WhrOnWindChange builds the wind of the server every frame from these values
	NSWeather.WindAngle = fAngle;
	NSWeather.WindSpeed = fSpeed;
	fServerWeatherDelta = 0.0;
	fServerWeatherAngle = fAngle;
	fServerWeatherSpeed = fSpeed;

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

void NetServer_AdminChangeSpeed(int iDelta)
{
	int iSpeed = makeint(stf(NetServer.GameSpeed)) + iDelta;
	if (iSpeed < 1) { iSpeed = 1; }
	if (iSpeed > 8) { iSpeed = 8; }

	NetServer.GameSpeed = iSpeed;

	NetServer_AdminSendEnvToAdmins();
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
	if (!NetServer_IsAdmin(wNetClientID)) { return; }

	switch (iAction)
	{
		case NET_ADMIN_SYNC:
			for (int i=0; i<NET_MAXCLIENTS; i++)
			{
				if (!sti(NSClients[i].Use)) { continue; }
				NetServer_AdminSendFlags(wNetClientID, i);
			}
			NetServer_AdminSendEnv(wNetClientID);
		break;
		case NET_ADMIN_KICK:			NetServer_AdminKick(wNetClientID, wTarget);					break;
		case NET_ADMIN_MUTE:			NetServer_AdminMute(wTarget);								break;
		case NET_ADMIN_TP_TO:			NetServer_AdminTeleportToPlayer(wNetClientID, wTarget);		break;
		case NET_ADMIN_TP_BRING:		NetServer_AdminBringPlayer(wNetClientID, wTarget);			break;
		case NET_ADMIN_MAKE_ADMIN:		NetServer_AdminMakeAdmin(wTarget);							break;
		case NET_ADMIN_RELOAD_SHIPS:	NetServer_AdminReloadShips();								break;
		case NET_ADMIN_HEAL:			NetServer_AdminHeal(wTarget);								break;
		case NET_ADMIN_ALL_HEAL:		NetServer_AdminHealAll();									break;
		case NET_ADMIN_DESTROY:			NetServer_AdminDestroy(wTarget);							break;
		case NET_ADMIN_WEATHER:			NetServer_AdminChangeWeather(iNetAdminTimeIdx, iNetAdminWeatherIdx + makeint(fValue));	break;
		case NET_ADMIN_TIME:			NetServer_AdminChangeWeather(iNetAdminTimeIdx + makeint(fValue), iNetAdminWeatherIdx);	break;
		case NET_ADMIN_SPEED:			NetServer_AdminChangeSpeed(makeint(fValue));				break;
		case NET_ADMIN_WIND_ANGLE:		NetServer_AdminSetWind(fServerWeatherAngle + fValue, fServerWeatherSpeed);	break;
		case NET_ADMIN_WIND_SPEED:		NetServer_AdminSetWind(fServerWeatherAngle, fServerWeatherSpeed + fValue);	break;
	}
}
