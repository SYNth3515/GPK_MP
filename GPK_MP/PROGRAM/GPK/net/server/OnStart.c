bool bServerGameStarted = false;
// COOP: the host pressed "Start" in the lobby (the 10 second countdown is skipped); lobby is locked, game is starting
bool bServerStartRequested = false;
// COOP spawn test switches (edit, restart the game):
// iCoopSpawnMode: 0 = ring around island centre (old behaviour, worked on Jamaica at R=400), 1 = singleplayer sea/port points
int iCoopSpawnMode = 1;
// ring radius used when iCoopSpawnMode = 0 (also as fallback when island has no port points)
int iCoopRingRadius = 400;
// ring radius for LostShipsCity (the city is a cluster of wrecks, a small ring may land on a hull - raise it if so)
int iCoopLSCRingRadius = 400;
// COOP: ports farther from the island origin than this are not used for spawn (hangs seen at 4200 and 5950, none up to 3500)
int iCoopMaxPortDist = 2500;
// COOP test: open sea (no island) deathmatch spawn Z of team 0. Try 0.0, then 600.0, then -600.0
float fCoopOpenZ = 0.0;

// COOP s30: the spawn point of an island. The host puts his ship in a safe place with its bow to the open sea and writes
// "/spawna" in the chat: the place and the course of the ship are kept in SAVE\NetSaves\Server\SpawnLines.nsv and win over the
// points of NetServer_SpawnLineDefaults (the points that come with the mod). The sides stand on the line that goes from that
// place forward along the bow: the first side at the point, the next one "Start position" metres farther (near 250, middle 500,
// far 1000), and so on. The ships of one side stand abreast across the line, a little apart, with a small random shift.
// Respawn and late join use the same places. No point: the sea points of the ports, then the nearest port even if it is far,
// the ring around the island origin last.
object NSSpawnLinesOwn;		// the points of the host: NSSpawnLinesOwn.(island).x, z, ay (saved)
object NSLine;				// the line in use: x, z (the start), ux, uz (the course), ay
object NSLinePt;			// a place on the line: x, z, ay
float fNetLineShipStep = 55.0;		// metres between the ships of one side (across the line)
float fNetLineJitter = 10.0;		// the random shift, metres
float fNetLineMaxLen = 3000.0;		// the sides of a big deathmatch stand closer, so the line is not longer than this
float fNetLineMinGap = 150.0;		// ... but never closer than this

// the points that come with the mod: X, Z and the course (radians) of "/spawna" (or of "/getmypos": X, Z, the angle Y)
bool NetServer_SpawnLineDefaults(string sIsl)
{
	bool bOk = false;
	switch (sIsl)
	{
		// COOP s30: the points of the author (SpawnLines.nsv of 21.11.2026), every sea island
		case "Antigua": NetServer_SpawnLineSet(-1394.3900146, 553.2050171, 5.1124101); bOk = true; break;
		case "Barbados": NetServer_SpawnLineSet(-985.0910034, -1321.7700195, 4.0823302); bOk = true; break;
		case "Beliz": NetServer_SpawnLineSet(-1853.3699951, -1497.0300293, 1.6844701); bOk = true; break;
		case "Bermudes": NetServer_SpawnLineSet(-3222.1101074, -1640.7099609, 4.2602301); bOk = true; break;
		case "Guadeloupe": NetServer_SpawnLineSet(-2795.2900391, -1991.6899414, 4.0272799); bOk = true; break;
		case "LostShipsCity": NetServer_SpawnLineSet(-556.6010132, -172.7700043, 5.2971501); bOk = true; break;
		case "Dominica": NetServer_SpawnLineSet(-152.6739960, -1824.6199951, 4.0144200); bOk = true; break;
		case "Caiman": NetServer_SpawnLineSet(822.8720093, 547.0499878, 0.2824700); bOk = true; break;
		case "Caracas": NetServer_SpawnLineSet(549.3599854, 399.8999939, 0.4603800); bOk = true; break;
		case "Cartahena": NetServer_SpawnLineSet(135.2530060, 77.9940033, 5.2726998); bOk = true; break;
		case "Cuba2": NetServer_SpawnLineSet(-767.0739746, 711.8109741, 5.4666901); bOk = true; break;
		case "Cuba1": NetServer_SpawnLineSet(424.6780090, -1024.6500244, 4.0671101); bOk = true; break;
		case "Cumana": NetServer_SpawnLineSet(-1754.5600586, 1205.2800293, 5.6003098); bOk = true; break;
		case "Curacao": NetServer_SpawnLineSet(285.0419922, -2036.0200195, 3.4787300); bOk = true; break;
		case "Maracaibo": NetServer_SpawnLineSet(-281.0209961, -780.8549805, 0.6889440); bOk = true; break;
		case "Martinique": NetServer_SpawnLineSet(-789.0640259, -313.6560059, 4.3657498); bOk = true; break;
		case "Nevis": NetServer_SpawnLineSet(-1049.1099854, -403.1700134, 5.3839202); bOk = true; break;
		case "PortoBello": NetServer_SpawnLineSet(942.3909912, -41.5181007, 5.8570199); bOk = true; break;
		case "PuertoRico": NetServer_SpawnLineSet(1138.8800049, 1431.8399658, 0.1461350); bOk = true; break;
		case "SantaCatalina": NetServer_SpawnLineSet(-132.8800049, -548.4210205, 2.1945400); bOk = true; break;
		case "SentMartin": NetServer_SpawnLineSet(-1226.0200195, -808.1229858, 4.8086600); bOk = true; break;
		case "Tortuga": NetServer_SpawnLineSet(838.8610229, -1569.0799561, 3.3672400); bOk = true; break;
		case "Trinidad": NetServer_SpawnLineSet(-2835.6201172, -2497.4599609, 4.4565701); bOk = true; break;
		case "Hispaniola2": NetServer_SpawnLineSet(1694.8800049, -1299.4899902, 5.4265599); bOk = true; break;
		case "Hispaniola1": NetServer_SpawnLineSet(-865.0499878, -2958.9399414, 2.6029899); bOk = true; break;
		case "Jamaica": NetServer_SpawnLineSet(2687.6899414, -2036.8699951, 2.4919500); bOk = true; break;
		case "Terks": NetServer_SpawnLineSet(57.8086014, 729.9030151, 0.2993700); bOk = true; break;
		case "__none__": bOk = false; break;
	}
	return bOk;
}

void NetServer_LoadSpawnLines()
{
	DeleteAttribute(&NSSpawnLinesOwn, "");
	Net_LoadFile(true, &NSSpawnLinesOwn, "SpawnLines.nsv");
}

// the line from (fX, fZ) along the course fAy (the bow of a ship with Ang.y = fAy looks to sin(ay), cos(ay))
void NetServer_SpawnLineSet(float fX, float fZ, float fAy)
{
	NSLine.x = fX;
	NSLine.z = fZ;
	NSLine.ay = fAy;
	NSLine.ux = sin(fAy);
	NSLine.uz = cos(fAy);
}

// the line of the island -> NSLine (own = 1: the point of the host); false: the island has none
bool NetServer_GetSpawnLine(string sIsl)
{
	DeleteAttribute(&NSLine, "");
	if (sIsl == "") { return false; }
	if (CheckAttribute(&NSSpawnLinesOwn, sIsl + ".ay"))
	{
		NetServer_SpawnLineSet(stf(NSSpawnLinesOwn.(sIsl).x), stf(NSSpawnLinesOwn.(sIsl).z), stf(NSSpawnLinesOwn.(sIsl).ay));
		NSLine.own = 1;
		return true;
	}
	if (!NetServer_SpawnLineDefaults(sIsl)) { return false; }
	NSLine.own = 0;
	return true;
}

// the line of the running game (NetServer.Line) -> NSLine
bool NetServer_UseGameLine()
{
	if (!CheckAttribute(&NetServer, "Line.ay")) { return false; }
	NetServer_SpawnLineSet(stf(NetServer.Line.x), stf(NetServer.Line.z), stf(NetServer.Line.ay));
	return true;
}

// metres between the neighbour sides: "Start position" of the Create game screen, a big deathmatch closer
float NetServer_LineGap(int iSides)
{
	float fGap = 500.0;
	int iStartPos = 1;
	if (CheckAttribute(&NetServer, "StartPosition")) { iStartPos = sti(NetServer.StartPosition); }
	if (iStartPos <= 0) { fGap = 250.0; }
	if (iStartPos >= 2) { fGap = 1000.0; }
	if (iSides > 1)
	{
		float fMax = fNetLineMaxLen / makefloat(iSides - 1);
		if (fGap > fMax) { fGap = fMax; }
	}
	if (fGap < fNetLineMinGap) { fGap = fNetLineMinGap; }
	return fGap;
}

// the place fAlong metres forward on NSLine and fSide metres to its right -> NSLinePt.x, z
void NetServer_LinePoint(float fAlong, float fSide)
{
	float fUX = stf(NSLine.ux);
	float fUZ = stf(NSLine.uz);
	NSLinePt.x = stf(NSLine.x) + fUX * fAlong + fUZ * fSide;
	NSLinePt.z = stf(NSLine.z) + fUZ * fAlong - fUX * fSide;
}

// the place of a ship: the side iSide of iSides, the iRow-th ship of that side -> NSLinePt.x, z, ay
void NetServer_LineShipPlace(int iSide, int iSides, int iRow)
{
	float fAlong = makefloat(iSide) * NetServer_LineGap(iSides);
	// abreast across the line: 0, right, left, right * 2, left * 2 ...
	int iHalf = makeint((iRow + 1) / 2);
	float fSide = makefloat(iHalf) * fNetLineShipStep;
	if (iRow - makeint(iRow / 2) * 2 == 0) { fSide = -fSide; }
	fAlong = fAlong + (frnd() * 2.0 - 1.0) * fNetLineJitter;
	fSide = fSide + (frnd() * 2.0 - 1.0) * fNetLineJitter;
	NetServer_LinePoint(fAlong, fSide);

	// the sides look at each other: the first half along the course, the far half back
	float fAy = stf(NSLine.ay);
	if (iSides > 1)
	{
		if (iSide * 2 > iSides - 1) { fAy = fAy + PI; }
	}
	while (fAy < 0.0) { fAy = fAy + PIm2; }
	while (fAy >= PIm2) { fAy = fAy - PIm2; }
	NSLinePt.ay = fAy;
}

string Net_LineTeamKey(int iTeam)
{
	return "t" + iTeam;
}

string NetServer_LineNum(float f)
{
	return "" + makeint(f);
}

// net\Land.c (NetLand_ChatCmd): the chat commands "/spawna", "/spawnline", "/spawnclear" of the host
bool NetServer_SpawnLineCmd(string sMsg)
{
	bool bA = (findSubStr(sMsg, "/spawna", 0) == 0);
	bool bShow = (findSubStr(sMsg, "/spawnline", 0) == 0);
	bool bClear = (findSubStr(sMsg, "/spawnclear", 0) == 0);
	if (!bA && !bShow && !bClear) { return false; }

	if (!NetServer_IsStarted()) { Log_SetStringToLog("Точку спавна задаёт хост (тот, кто создал игру)"); return true; }
	if (!bServerGameStarted || NetServer_LandMode() || NetServer.IslandID == "")
	{
		Log_SetStringToLog("Точку спавна задают в морском бою у острова");
		return true;
	}
	string sIsl = NetServer.IslandID;

	if (bShow)
	{
		if (!NetServer_GetSpawnLine(sIsl)) { Log_SetStringToLog("У острова " + sIsl + " нет точки спавна"); return true; }
		string sFrom = "из мода";
		if (sti(NSLine.own)) { sFrom = "ваша"; }
		Log_SetStringToLog("Точка спавна " + sIsl + " (" + sFrom + "): X " + NetServer_LineNum(stf(NSLine.x)) + ", Z " + NetServer_LineNum(stf(NSLine.z)) + ", курс " + makeint(stf(NSLine.ay) * 180.0 / PI) + " гр");
		return true;
	}

	if (bClear)
	{
		DeleteAttribute(&NSSpawnLinesOwn, sIsl);
		Net_SaveFile(true, &NSSpawnLinesOwn, "SpawnLines.nsv");
		Log_SetStringToLog("Ваша точка спавна острова " + sIsl + " удалена");
		Trace("COOP: spawnline " + sIsl + " cleared");
		return true;
	}

	if (wClientID < 0 || wClientID >= NET_MAXCLIENTS || !NetServer_HasShip(wClientID))
	{
		Log_SetStringToLog("Нужен ваш корабль в море");
		return true;
	}
	ref rMe = NetServer_GetClient(wClientID);
	float fX = stf(rMe.Ship.Pos.x);
	float fZ = stf(rMe.Ship.Pos.z);
	float fAy = stf(rMe.Ship.Ang.y);
	while (fAy < 0.0) { fAy = fAy + PIm2; }
	while (fAy >= PIm2) { fAy = fAy - PIm2; }
	NSSpawnLinesOwn.(sIsl).x = fX;
	NSSpawnLinesOwn.(sIsl).z = fZ;
	NSSpawnLinesOwn.(sIsl).ay = fAy;
	Net_SaveFile(true, &NSSpawnLinesOwn, "SpawnLines.nsv");
	Log_SetStringToLog("Точка спавна острова " + sIsl + " сохранена: X " + NetServer_LineNum(fX) + ", Z " + NetServer_LineNum(fZ) + ", курс " + makeint(fAy * 180.0 / PI) + " гр");
	Log_SetStringToLog("Стороны встанут вперёд по носу через 250 / 500 / 1000 м. Работает со следующей игры");
	Trace("COOP: spawnline saved " + sIsl + " x=" + fX + " z=" + fZ + " ay=" + fAy);
	return true;
}

int iServerEnvSMsg;

void NetServer_EndGame(bool bReconnect)
{
	int i;
	//if (!bServerGameStarted) { return; }
	bServerGameStarted = false;
	bServerStartRequested = false;

	NetServer_ClearPacketsQueue();

	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_CONNECT);
	if (bReconnect)
		{ NMAddByte(iSMsg, NSC_CONNECT_RECONNECT); }
	else
		{ NMAddByte(iSMsg, NSC_CONNECT_DISCONNECT); }

	NetServer_SendMessage(DST_ALL, iSMsg, true);
	NMDelete(iSMsg);

	if (sti(NetServer.GameType) == NETGAME_DEATHMATCH)
	{
		if (sti(NetServer.wWinnerClientID) != DST_INVALID)
		{
			for (i=0; i<NET_MAXCLIENTS; i++)
			{
				if (!sti(NSClients[i].Use) && !NetServer_IsParked(i)) { continue; }

				NSClients[i].Stat.Won = sti(NSClients[i].ID) == sti(NetServer.wWinnerClientID);
				NSClients[i].Stat.Lost = sti(NSClients[i].ID) != sti(NetServer.wWinnerClientID);
			}
		}
	}
	else
	{
		if (sti(NetServer.iWinnerTeam) >= 0)
		{
			for (i=0; i<NET_MAXCLIENTS; i++)
			{
				if (!sti(NSClients[i].Use) && !NetServer_IsParked(i)) { continue; }

				NSClients[i].Stat.Won = sti(NSClients[i].Team) == sti(NetServer.iWinnerTeam);
				NSClients[i].Stat.Lost = sti(NSClients[i].Team) != sti(NetServer.iWinnerTeam);
			}
		}
	}

	// disconnect clients
	// COOP: players who left the game (parked ships) get their statistics too, and every ship entity of the server is deleted
	for (i=0; i<NET_MAXCLIENTS; i++)
	{
		bool bParked = NetServer_IsParked(i);
		if (!sti(NSClients[i].Use) && !bParked && !NetServer_HasShip(i)) { continue; }
		
		if (!NetServer_IsBot(i)) { NetServer_UpdateGlobalPlayerStatistics(&NSClients[i]); }		// COOP: bots have no statistics
		DeleteClass(&NSClients[i]);
		NSClients[i].Parked = false;
		NSClients[i].Spawned = false;
		NSClients[i].InGame = false;
	}

	NetServer_LandEnd();		// COOP: land duel (Land.c)
	NetServer_MinesClear();		// COOP s30: no mine of this game in the next one
	NetServer_DeleteWeatherEnvironment();
	NetServer_DeleteIslandEnvironment();
	NetServer_DeleteBallsEnvironment();
	NetServer_DeleteTouchEnvironment();
	NetServer_DeleteRiggingEnvironment();
	NetServer_DeleteShipEnvironment();
	NetServer_DeleteFortEnvironment();
	NetServer_DeleteTimeEnvironment();

	NMDelete(iServerEnvSMsg);

	DelEventHandler("frame", "NetServer_OnCheckGameOverFrame");
	DelEventHandler("NetServer_StartServerCountDown", "NetServer_StartServerCountDown");
}

// COOP: how many sides still fight: alive ships in deathmatch, teams with alive ships in team games (the fort counts as
// the defenders while it stands). A game is "won" only after at least two sides have met (a lonely captain does not win).
int NetServer_CountSides()
{
	int i;
	int iAlive = 0;
	int iTeamAlive[4];
	for (i=0; i<4; i++) { iTeamAlive[i] = 0; }

	for (i=0; i<NET_MAXCLIENTS; i++)
	{
		bool bSideSlot = sti(NSClients[i].Use);
		if (!bSideSlot && sti(NetServer.GameType) != NETGAME_DEATHMATCH) { bSideSlot = NetServer_IsTeamBot(i); }		// COOP s28
		if (!bSideSlot) { continue; }
		if (!NetServer_ShipIsAlive(i)) { continue; }
		iAlive++;
		int iTeam = sti(NSClients[i].Team);
		if (iTeam >= 0 && iTeam < 4) { iTeamAlive[iTeam] = 1; }
	}

	if (sti(NetServer.GameType) == NETGAME_DEATHMATCH) { return iAlive; }

	int iSides = 0;
	for (i=0; i<4; i++) { iSides = iSides + iTeamAlive[i]; }

	if (sti(NetServer.GameType) == NETGAME_FORT && iTeamAlive[0] == 0)
	{
		ref rFort = NetServer_GetCLient(NET_FORTCLIENT);
		if (CheckAttribute(rFort, "Ship.HP") && stf(rFort.Ship.HP) > 0.0) { iSides++; }
	}
	return iSides;
}

// COOP: the game over of the battle: an announcement only (client\OnStart.c: NetClient_OnGameOver), nobody is disconnected
void NetServer_AnnounceGameOver()
{
	NetServer.GameOver = true;
	NetServer.ContestStarted = false;
	Trace("COOP: game over announced, winner id=" + NetServer.wWinnerClientID + " team=" + NetServer.iWinnerTeam + " draw=" + NetServer.Draw);
	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_STARTGAME);
	NMAddByte(iSMsg, NSC_STARTGAME_GAMEOVER);
	NMAddByte(iSMsg, sti(NetServer.GameType));
	NMAddClientID(iSMsg, sti(NetServer.wWinnerClientID));
	NMAddByte(iSMsg, sti(NetServer.iWinnerTeam));
	NMAddByte(iSMsg, sti(NetServer.Draw));			// COOP: draw flag (client\OnStart.c: NetClient_OnGameOver)
	NetServer_SendMessage(DST_ALL, iSMsg, true);
	NMDelete(iSMsg);
}

// COOP: the fort of Capture the fort is the objective of the mode: its death ends the battle even with respawn,
// where nothing else ends it (the respawn return used to hide the fort from NetServer_CheckGameOver_Fort).
bool NetServer_FortVictory()
{
	if (sti(NetServer.GameType) != NETGAME_FORT) { return false; }
	if (sti(NetServer.GameOver)) { return false; }
	if (!NetServer_FortGameDead()) { return false; }

	NetServer.wWinnerClientID = DST_INVALID;
	NetServer.iWinnerTeam = 1;
	NetServer.Draw = 0;
	Trace("COOP: the fort of the game mode is destroyed, the attackers won");
	return true;
}

// COOP: the convoy ship stands in the point of the mode: the convoy team wins, also in a solo game and with respawn
bool NetServer_ConvoyVictory()
{
	if (sti(NetServer.GameType) != NETGAME_CONVOY) { return false; }
	if (sti(NetServer.GameOver)) { return false; }
	if (!NetServer_ConvoyReached()) { return false; }

	NetServer.wWinnerClientID = DST_INVALID;
	NetServer.iWinnerTeam = 0;
	NetServer.Draw = 0;
	Trace("COOP: the convoy ship reached the point, the convoy team won");
	return true;
}

void NetServer_OnCheckGameOverFrame()
{
	int i;

	// COOP: time limit of the match (also in respawn mode, where nothing else ends the battle)
	if (CheckAttribute(&NetServer, "TimeEnd") && sti(NetServer.TimeEnd) > 0)
	{
		if (sti(NetServer.TimeUp)) { return; }
		if (iServerTime >= sti(NetServer.TimeEnd))
		{
			NetServer.TimeUp = true;
			NetServer_TimeUpAnnounce();
			return;
		}
	}

	// COOP: with respawn the battle never ends, the server lives while somebody plays
	if (sti(NetServer.Respawn))
	{
		// COOP: the only exceptions are the objectives of the modes: a destroyed fort and a convoy ship in the point
		// end the battle. A sunk convoy ship does NOT save the defenders here (that stays a no-respawn rule).
		if (NetServer_FortVictory()) { NetServer_AnnounceGameOver(); }
		if (NetServer_ConvoyVictory()) { NetServer_AnnounceGameOver(); }
		return;
	}

	int iSides = NetServer_CountSides();

	// COOP: the game is NOT closed when somebody wins: the winner is announced and everybody keeps sailing until he leaves.
	// In deathmatch / team deathmatch new players can join and the battle starts again (a new winner is announced later).
	if (sti(NetServer.GameOver))
	{
		bool bRearm = false;
		if (sti(NetServer.GameType) == NETGAME_DEATHMATCH) { bRearm = true; }
		if (sti(NetServer.GameType) == NETGAME_NAVAL) { bRearm = true; }
		if (bRearm && iSides >= 2)
		{
			NetServer.GameOver = false;
			NetServer.ContestStarted = true;
			Trace("COOP: the battle goes on (new players), the game over check is armed again");
			NetServer.PrevAlive = 0;
		}
		return;
	}

	if (iSides >= 2) { NetServer.ContestStarted = true; }
	// COOP: the fort of the mode is down - the battle was surely on, even if the sides did not meet in this frame
	if (NetServer_FortGameDead()) { NetServer.ContestStarted = true; }
	// COOP: the convoy ship in the point wins the mode even in a solo game, where the sides never met
	if (NetServer_ConvoyReached()) { NetServer.ContestStarted = true; }
	if (!sti(NetServer.ContestStarted)) { return; }

	// check the game over - only if game started
	if (!sti(NetServer.GameOver))
	{
		NetServer.wWinnerClientID = DST_INVALID;		// for deathmatch
		NetServer.iWinnerTeam = -1;						// for all other modes
		NetServer.Draw = 0;										// COOP: 1 = nobody won (the last two ships sank together)

		bool bCurrentGameOver = false;
		switch (sti(NetServer.GameType))
		{
			case NETGAME_DEATHMATCH:	
				bCurrentGameOver = NetServer_CheckGameOver_DeathMatch();
			break;
			case NETGAME_NAVAL:			
				bCurrentGameOver = NetServer_CheckGameOver_Naval();
			break;
			case NETGAME_CONVOY:		
				bCurrentGameOver = NetServer_CheckGameOver_Convoy();
			break;
			case NETGAME_FORT:			
				bCurrentGameOver = NetServer_CheckGameOver_Fort();
			break;
		}

		if (!NET_DEBUG && bCurrentGameOver)
		{
			NetServer_AnnounceGameOver();
		}
	}
}

void NetServer_RestartGame()
{
	Net_DeleteServer();

	//Net_CreateServer();
}

// COOP s30: the exit of the host from the running game (and "Back to the lobby" of the admin menu) sends everybody back to
// the lobby (Buy screen): the server and the connections stay. false = the old way: the server is closed, all go to the
// main menu (interfaces\NetMenu.c, interfaces\EndGame.c).
bool bNetHostExitToLobby = true;

// a slot that leaves the game for good (a bot, a player who left during the game): as a slot of a new server
void NetServer_SlotClear(int i)
{
	DeleteAttribute(&NSClients[i], "");
	NSClients[i].index = i;
	NSClients[i].id = i;
	NSClients[i].Use = false;
	NSClients[i].BuyReady = false;
	NSClients[i].StartReady = false;
	NSClients[i].Server = true;
	NSClients[i].Team = 0;
	NSClients[i].LastPacketIndex = 0;
	NSClients[i].LastPingTime = iPingTime;
	NetServer_SlotStatClear(i);
	NSClients[i].Spawned = false;
	NSClients[i].InGame = false;
	NSClients[i].Parked = false;
}

void NetServer_SlotStatClear(int i)
{
	NSClients[i].Stat.Sunk = 0;
	NSClients[i].Stat.Wreck = 0;
	NSClients[i].Stat.Misses = 0;
	NSClients[i].Stat.Hits = 0;
	NSClients[i].Stat.Won = 0;
	NSClients[i].Stat.Lost = 0;
	NSClients[i].Stat.DamageInflicted = 0;
	NSClients[i].Stat.Deaths = 0;
}

// the connected player stays in his slot (nick, team, admin, mute, face, the name of the ship), all of the game goes
void NetServer_SlotToLobby(int i)
{
	string sShipName = "";
	if (CheckAttribute(&NSClients[i], "Ship.Name")) { sShipName = NSClients[i].Ship.Name; }
	int iSailColor = argb(255, 255, 255, 255);
	if (CheckAttribute(&NSClients[i], "Ship.Sails.Color")) { iSailColor = sti(NSClients[i].Ship.Sails.Color); }
	DeleteAttribute(&NSClients[i], "Ship");
	NSClients[i].Ship.Name = sShipName;
	NSClients[i].Ship.Type = -1;
	// as at the connect (server\OnConnect.c): the engine ship needs Ship.Pos and Ship.Ang (an assert of ship.cpp without them)
	NSClients[i].Ship.Pos.x = 0.0;
	NSClients[i].Ship.Pos.y = 0.0;
	NSClients[i].Ship.Pos.z = 0.0;
	NSClients[i].Ship.Ang.y = frnd() * PIm2;
	NSClients[i].Ship.Speed.x = 0.0;
	NSClients[i].Ship.Speed.y = 0.0;
	NSClients[i].Ship.Speed.z = 0.0;
	NSClients[i].Ship.Upgrades.Hull = 1;
	NSClients[i].Ship.Upgrades.Sails = 1;
	NSClients[i].Ship.Upgrades.Cannons = 1;
	NSClients[i].Ship.Sails.Color = iSailColor;
	DeleteAttribute(&NSClients[i], "Killer");
	DeleteAttribute(&NSClients[i], "WreckedNow");
	DeleteAttribute(&NSClients[i], "ShipGone");
	DeleteAttribute(&NSClients[i], "APerk");
	DeleteAttribute(&NSClients[i], "SpawnProtect");
	DeleteAttribute(&NSClients[i], "Skills");
	DeleteAttribute(&NSClients[i], "NcHold");
	DeleteAttribute(&NSClients[i], "AimTrack");
	DeleteAttribute(&NSClients[i], "LastCollisionShip");
	DeleteAttribute(&NSClients[i], "LastCollisionIsland");
	NSClients[i].Dead = false;
	NSClients[i].BuyReady = false;
	NSClients[i].StartReady = false;
	NSClients[i].Reconnect = false;
	NSClients[i].JoinRequested = false;
	NSClients[i].Spawned = false;
	NSClients[i].InGame = false;
	NSClients[i].Parked = false;
	NetServer_SlotStatClear(i);
}

void NetServer_ReturnToLobby()
{
	int i, iSMsg;
	if (!NetServer_IsStarted()) { return; }
	bool bWasRunning = bServerGameStarted;
	Trace("COOP: srv back to the lobby (game running " + bWasRunning + ")");

	bServerGameStarted = false;
	bServerStartRequested = false;
	iSecondsToStartGame = -1;
	DelEventHandler("NetServer_StartServerCountDown", "NetServer_StartServerCountDown");
	DelEventHandler("frame", "NetServer_OnCheckGameOverFrame");
	NetServer_ClearPacketsQueue();

	// the clients first: they delete their sea / land and open the lobby (client\OnConnect.c)
	iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_CONNECT);
	NMAddByte(iSMsg, NSC_CONNECT_TOLOBBY);
	NetServer_SendMessage(DST_ALL, iSMsg, true);
	NMDelete(iSMsg);

	for (i=0; i<NET_MAXCLIENTS; i++)
	{
		bool bUse = sti(NSClients[i].Use);
		bool bBot = NetServer_IsBot(i);
		bool bParked = NetServer_IsParked(i);
		bool bShip = NetServer_HasShip(i);
		if (!bUse && !bParked && !bShip && !bBot) { continue; }
		bool bPlayer = bUse;
		if (bParked) { bPlayer = true; }
		if (bWasRunning && !bBot && bPlayer) { NetServer_UpdateGlobalPlayerStatistics(&NSClients[i]); }
		DeleteClass(&NSClients[i]);
		if (bUse && !bBot) { NetServer_SlotToLobby(i); continue; }
		// a bot or a player who left: the slot is free again; the clients forget it
		NetServer_SlotClear(i);
		iSMsg = NMCreate();
		NMAddByte(iSMsg, NC_CLIENT);
		NMAddByte(iSMsg, NSC_CLIENT_DROP);
		NMAddClientID(iSMsg, i);
		NetServer_SendMessage(DST_ALL, iSMsg, true);
		NMDelete(iSMsg);
	}
	iNetBotCount = 0;
	iNetBotGen = iNetBotGen + 1;		// the tick of the bots of the old game stops
	NetServer_MinesClear();

	NetServer_LandEnd();
	NetServer_DeleteWeatherEnvironment();
	NetServer_DeleteIslandEnvironment();
	NetServer_DeleteBallsEnvironment();
	NetServer_DeleteTouchEnvironment();
	NetServer_DeleteRiggingEnvironment();
	NetServer_DeleteShipEnvironment();
	NetServer_DeleteFortEnvironment();
	NetServer_DeleteTimeEnvironment();

	// the lobbies see "not running" (the Start button of the host, the details)
	for (i=0; i<NET_MAXCLIENTS; i++)
	{
		if (!sti(NSClients[i].Use)) { continue; }
		NetServer_OnServerInfo_SendCommonServerInfo(i);
	}
}

// COOP s31: ONE player (the host: interfaces\NetMenu.c) goes from the running game to the lobby, the game goes on. His ship
// waits like the ship of a reconnect (sails down); "Join the battle" of the lobby gives it back (NetServer_OnCommandStartGame)
void NetServer_ClientToLobby(int wID)
{
	int i, iSMsg;
	if (wID < 0 || wID >= NET_MAXPLAYERS) { return; }
	if (!bServerGameStarted) { return; }
	if (!sti(NSClients[wID].Use)) { return; }
	ref rC = NetServer_GetClient(wID);
	rC.InGame = false;
	rC.JoinRequested = false;
	rC.BackToGame = true;
	if (NetServer_HasShip(wID))
	{
		rC.Reconnect = true;
		rC.Ship.SailState = 0.0;
		rC.Ship.TurnLeft = 0;
		rC.Ship.TurnRight = 0;
		rC.Ship.TurnState = 0.0;
	}
	Trace("COOP: client " + wID + " (" + rC.NickName + ") went to the lobby, the game goes on");

	// the others: a chat line, the puppet of the land duel goes away (client\OnClient.c)
	iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_CLIENT);
	NMAddByte(iSMsg, NSC_CLIENT_TOLOBBY);
	NMAddClientID(iSMsg, wID);
	for (i=0; i<NET_MAXCLIENTS; i++)
	{
		if (i == wID) { continue; }
		if (!sti(NSClients[i].Use)) { continue; }
		NetServer_SendMessage(i, iSMsg, true);
	}
	NMDelete(iSMsg);
}

// COOP s38: the host goes to the lobby from a sea battle (net\Board.c: NetClient_HostPark): his ship stops and waits, he stays
// in the game for the server (his machine keeps the sea and gets everything as before), a boarding duel of his is lost
void NetServer_ClientParkAtSea(int wID)
{
	if (wID < 0 || wID >= NET_MAXPLAYERS) { return; }
	if (!sti(NSClients[wID].Use)) { return; }
	NetServer_BDLeft(wID);
	NetServer_PrizeCaptorGone(wID);		// COOP s41: his prize goes down
	ref rC = NetServer_GetClient(wID);
	rC.HostParked = true;
	if (NetServer_HasShip(wID))
	{
		rC.Ship.SailState = 0.0;
		rC.Ship.TurnLeft = 0;
		rC.Ship.TurnRight = 0;
		rC.Ship.TurnState = 0.0;
	}
	Trace("COOP: client " + wID + " (" + rC.NickName + ") went to the lobby, his ship waits");
	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_CLIENT);
	NMAddByte(iSMsg, NSC_CLIENT_TOLOBBY);
	NMAddClientID(iSMsg, wID);
	for (int i=0; i<NET_MAXCLIENTS; i++)
	{
		if (i == wID) { continue; }
		if (!sti(NSClients[i].Use)) { continue; }
		NetServer_SendMessage(i, iSMsg, true);
	}
	NMDelete(iSMsg);
}

void NetServer_ClientUnparkAtSea(int wID)
{
	if (wID < 0 || wID >= NET_MAXPLAYERS) { return; }
	DeleteAttribute(&NSClients[wID], "HostParked");
	Trace("COOP: client " + wID + " is back from the lobby");
}

void NetServer_StartGame()
{
	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_STARTGAME);
	NMAddByte(iSMsg, NSC_STARTGAME_GETBUYINFO);
	NetServer_SendMessage(DST_ALL, iSMsg, true);
	NMDelete(iSMsg);
}

void NetServer_StartServerGame()
{
	int		iSMsg;
	int		i;
	int		iTeams[4];	// 
	object	oTeams[4];	// team positions

	for (i=0; i<4; i++) { iTeams[i] = 0; }

	if (bServerGameStarted) { return; }
	//DelEventHandler("NetServer_StartServerGame", "NetServer_StartServerGame");

	bServerGameStarted = true;

	// COOP: a new game: round number (stale respawn events of an older game are ignored), win check, perks of the ships
	NetServer.Round = sti(NetServer.Round) + 1;
	NetServer.PrevAlive = 0;
	NetServer.Draw = 0;
	NetServer.GameOver = false;
	NetServer.ContestStarted = false;
	NetServer_BDResetAll();		// COOP s37: no boarding duel of the old game
	DeleteAttribute(&NetServer, "TeamPos");
	NetServer_ApplyShipPerksToTypes();

	// COOP s30: the bots of the lobby come in after the players (server\Bots.c)
	SetEventHandler("NetServer_LBotsSpawnEvent", "NetServer_LBotsSpawnEvent", 0);
	int iLBotWait = 2500;
	if (NetServer_LandMode()) { iLBotWait = 5000; }
	if (NetServer_LBotsNum() > 0) { PostEvent("NetServer_LBotsSpawnEvent", iLBotWait, "l", sti(NetServer.Round)); }

	// COOP: the game speed of this game: the choice of the Create game screen (the admin changes it during the game)
	NetServer.GameSpeed = fServerStartGameSpeed;
	Trace("COOP: game speed at start = " + fServerStartGameSpeed);

	// COOP: land duel (Land.c): a location instead of the sea
	if (NetServer_LandMode()) { NetServer_LandStart(); return; }

		int iServerEnvSMsg = NMCreate();
		NMAddByte(iServerEnvSMsg, NC_STARTGAME);
		NMAddByte(iServerEnvSMsg, NSC_STARTGAME_ENVIRONMENT);

	// create all environments
		Trace("COOP: srv NetServer_CreateWeatherEnvironment");
		NetServer_CreateWeatherEnvironment(iServerEnvSMsg);
		Trace("COOP: srv NetServer_CreateIslandEnvironment");
		NetServer_CreateIslandEnvironment(iServerEnvSMsg);
		Trace("COOP: srv NetServer_CreateBallsEnvironment");
		NetServer_CreateBallsEnvironment(iServerEnvSMsg);
		Trace("COOP: srv NetServer_CreateTouchEnvironment");
		NetServer_CreateTouchEnvironment(iServerEnvSMsg);
		Trace("COOP: srv NetServer_CreateRiggingEnvironment");
		NetServer_CreateRiggingEnvironment(iServerEnvSMsg);
		Trace("COOP: srv NetServer_CreateShipEnvironment");
		NetServer_CreateShipEnvironment(iServerEnvSMsg);
		Trace("COOP: srv NetServer_CreateFortEnvironment");
		NetServer_CreateFortEnvironment(iServerEnvSMsg);
		Trace("COOP: srv NetServer_CreateTimeEnvironment");
		NetServer_CreateTimeEnvironment(iServerEnvSMsg);

	// send message to all client's for start game and load environment
		NetServer_SendMessage(DST_ALL, iServerEnvSMsg, true);
		NMDelete(iServerEnvSMsg);

	NetServer.CoopN = 0;	// COOP: reset, filled below only for island games
	// setup group positions
	if (NetServer.IslandID == "")
	{
		switch (sti(NetServer.GameType))
		{
			case NETGAME_DEATHMATCH:
				oTeams[0].Pos.x = 0.0;
				oTeams[0].Pos.z = fCoopOpenZ;
			break;
			case NETGAME_NAVAL:
				oTeams[0].Pos.x = -500.0;	oTeams[1].Pos.x = 500.0;	oTeams[2].Pos.x = 500.0;	oTeams[3].Pos.x = -500.0;
				oTeams[0].Pos.z = -500.0;	oTeams[1].Pos.z = -500.0;	oTeams[2].Pos.z = 500.0;	oTeams[3].Pos.z = 500.0;
			break;
			case NETGAME_CONVOY:
				oTeams[0].Pos.x = 1300.0;	oTeams[1].Pos.x = 400.0;
				oTeams[0].Pos.z = 1300.0;	oTeams[1].Pos.z = 400.0;
			break;
		}
	}
	else
	{
		int iRand;
		string s1, s2, s3, s4;
		ref rIsland = Net_GetIsland(NetServer.IslandID);

		string sStartPosition = "l" + (2 + sti(NetServer.StartPosition));

		// COOP: GPK island locators have no net_* groups (they exist only in Corsairs 3 files and the island models differ).
		// Fallback: start on a ring in open water around the island. Tune the radius if a spawn lands on shore.
		// COOP: universal sea spawn. Singleplayer reload locators carry real sea start points reload.lK.ships.l0..lN (x, z, ay):
		// the same points the player gets when he leaves a port, so they are always navigable water. The ring below is the last resort.
		NetServer.CoopN = 0;
		DeleteAttribute(&NetServer, "CoopLoc");
		aref arCoopReload; makearef(arCoopReload, rIsland.reload);
		int iCoopNumReload = GetAttributesNum(arCoopReload);
		int iCoopFarBest = -1;			// COOP s30: the nearest of the skipped ports
		float fCoopFarBestD2 = 0.0;
		for (int iCL=0; iCL<iCoopNumReload; iCL++)
		{
			aref arCoopLoc = GetAttributeN(arCoopReload, iCL);
			if (CheckAttribute(arCoopLoc, "ships.l0.x") && CheckAttribute(arCoopLoc, "ships.l0.z"))
			{
					float fCoopPX = stf(arCoopLoc.ships.l0.x);
					float fCoopPZ = stf(arCoopLoc.ships.l0.z);
					float fCoopMaxD = makefloat(iCoopMaxPortDist);
					if (fCoopPX * fCoopPX + fCoopPZ * fCoopPZ > fCoopMaxD * fCoopMaxD)
					{
						Trace("COOP: port " + iCL + " skipped, too far from the island origin x=" + fCoopPX + " z=" + fCoopPZ);
						float fCoopFarD2 = fCoopPX * fCoopPX + fCoopPZ * fCoopPZ;
						if (iCoopFarBest < 0 || fCoopFarD2 < fCoopFarBestD2) { iCoopFarBest = iCL; fCoopFarBestD2 = fCoopFarD2; }
						continue;
					}
				string sCoopKey = "l" + sti(NetServer.CoopN);
				NetServer.CoopLoc.(sCoopKey) = iCL;
				NetServer.CoopN = sti(NetServer.CoopN) + 1;
			}
		}
		if (sti(NetServer.CoopN) == 0 && iCoopFarBest >= 0)
		{
			// the ring around the island origin is inside many islands (Trinidad): the nearest far port is better
			NetServer.CoopLoc.l0 = iCoopFarBest;
			NetServer.CoopN = 1;
			Trace("COOP: no near port, the nearest far port " + iCoopFarBest + " is used");
		}
		NetServer.CoopStart = 0;
		if (sti(NetServer.CoopN) > 0) { NetServer.CoopStart = rand(sti(NetServer.CoopN) - 1); }
		Trace("COOP: sea spawn locators found = " + NetServer.CoopN + " start=" + NetServer.CoopStart + " island=" + NetServer.IslandID);
		Trace("COOP: spawn mode=" + iCoopSpawnMode + " ringR=" + iCoopRingRadius);

		// COOP: ring radius. 4200 put ships on land / out of bounds on Jamaica (client hang). (0,0) is open water on Barbados and Jamaica,
		// so start with a small ring around it and raise fCoopR step by step.
		float fCoopR = makefloat(iCoopRingRadius);
		if (NetServer.IslandID == "LostShipsCity") { fCoopR = makefloat(iCoopLSCRingRadius); }
		float fCoopD = fCoopR * 0.7071;
		float fCoopO = fCoopR * 1.2;
		if (!CheckAttribute(rIsland, "net_deathmatch.l1.x"))
		{
			Trace("COOP: ring spawn fallback (deathmatch) for island " + NetServer.IslandID);
			rIsland.net_deathmatch.l1.x = fCoopR; rIsland.net_deathmatch.l1.z = 0.0;
			rIsland.net_deathmatch.l2.x = fCoopD; rIsland.net_deathmatch.l2.z = fCoopD;
			rIsland.net_deathmatch.l3.x = 0.0; rIsland.net_deathmatch.l3.z = fCoopR;
			rIsland.net_deathmatch.l4.x = -fCoopD; rIsland.net_deathmatch.l4.z = fCoopD;
			rIsland.net_deathmatch.l5.x = -fCoopR; rIsland.net_deathmatch.l5.z = 0.0;
			rIsland.net_deathmatch.l6.x = -fCoopD; rIsland.net_deathmatch.l6.z = -fCoopD;
			rIsland.net_deathmatch.l7.x = 0.0; rIsland.net_deathmatch.l7.z = -fCoopR;
			rIsland.net_deathmatch.l8.x = fCoopD; rIsland.net_deathmatch.l8.z = -fCoopD;
		}
		if (!CheckAttribute(rIsland, "net_team.l1.x"))
		{
			Trace("COOP: ring spawn fallback (team) for island " + NetServer.IslandID);
			rIsland.net_team.l1.x = fCoopR; rIsland.net_team.l1.z = 0.0;
			rIsland.net_team.l2.x = -fCoopR; rIsland.net_team.l2.z = 0.0;
			rIsland.net_team.l3.x = 0.0; rIsland.net_team.l3.z = fCoopR;
			rIsland.net_team.l4.x = 0.0; rIsland.net_team.l4.z = -fCoopR;
			rIsland.net_team.l5.x = fCoopD; rIsland.net_team.l5.z = fCoopD;
			rIsland.net_team.l6.x = -fCoopD; rIsland.net_team.l6.z = -fCoopD;
			rIsland.net_team.l7.x = -fCoopD; rIsland.net_team.l7.z = fCoopD;
			rIsland.net_team.l8.x = fCoopD; rIsland.net_team.l8.z = -fCoopD;
			rIsland.net_team.l9.x = fCoopO; rIsland.net_team.l9.z = 0.0;
			rIsland.net_team.l10.x = -fCoopO; rIsland.net_team.l10.z = 0.0;
			rIsland.net_team.l11.x = 0.0; rIsland.net_team.l11.z = fCoopO;
			rIsland.net_team.l12.x = 0.0; rIsland.net_team.l12.z = -fCoopO;
		}
		if (!CheckAttribute(rIsland, "net_convoy.l1.x"))
		{
			Trace("COOP: ring spawn fallback (convoy) for island " + NetServer.IslandID);
			rIsland.net_convoy.l1.x = fCoopR; rIsland.net_convoy.l1.z = 0.0;
			rIsland.net_convoy.l2.x = -fCoopR; rIsland.net_convoy.l2.z = 0.0;
			rIsland.net_convoy.l3.x = 0.0; rIsland.net_convoy.l3.z = fCoopR;
			rIsland.net_convoy.l4.x = 0.0; rIsland.net_convoy.l4.z = fCoopR;
			rIsland.net_convoy.l5.x = 0.0; rIsland.net_convoy.l5.z = -fCoopR;
			rIsland.net_convoy.l6.x = -fCoopR; rIsland.net_convoy.l6.z = 0.0;
			rIsland.net_convoy.l7.x = -fCoopR; rIsland.net_convoy.l7.z = 0.0;
			rIsland.net_convoy.l8.x = fCoopR; rIsland.net_convoy.l8.z = 0.0;
			rIsland.net_convoy.l9.x = 0.0; rIsland.net_convoy.l9.z = -fCoopR;
		}
		if (!CheckAttribute(rIsland, "net_fort.l1.x"))
		{
			Trace("COOP: ring spawn fallback (fort) for island " + NetServer.IslandID);
			rIsland.net_fort.l1.x = -fCoopR; rIsland.net_fort.l1.z = 0.0;
			rIsland.net_fort.l2.x = fCoopR; rIsland.net_fort.l2.z = 0.0;
			rIsland.net_fort.l3.x = 0.0; rIsland.net_fort.l3.z = fCoopR;
			rIsland.net_fort.l4.x = 0.0; rIsland.net_fort.l4.z = -fCoopR;
		}

		aref arConvoy, arTeam, arDeathmatch;
		makearef(arConvoy, rIsland.net_convoy);
		makearef(arTeam, rIsland.net_team);
		makearef(arDeathmatch, rIsland.net_deathmatch);

		int iNumConvoySpawns = GetAttributesNum(arConvoy) / 3;
		int iNumTeamSpawns = GetAttributesNum(arTeam) / 4;
		int iNumDeathmatchSpawns = GetAttributesNum(arDeathmatch);

		switch (sti(NetServer.GameType))
		{
			case NETGAME_DEATHMATCH:
				sStartPosition = "l" + (1 + rand(iNumDeathmatchSpawns - 1));
				oTeams[0].Pos.x = stf(arDeathmatch.(sStartPosition).x);
				oTeams[0].Pos.z = stf(arDeathmatch.(sStartPosition).z);
			break;
			case NETGAME_NAVAL:
				iRand = 4 * rand(iNumTeamSpawns - 1);
				s1 = "l" + (iRand + 1);
				s2 = "l" + (iRand + 2);
				s3 = "l" + (iRand + 3);
				s4 = "l" + (iRand + 4);

				oTeams[0].Pos.x = stf(arTeam.(s1).x);	oTeams[0].Pos.z = stf(arTeam.(s1).z);
				oTeams[1].Pos.x = stf(arTeam.(s2).x);	oTeams[1].Pos.z = stf(arTeam.(s2).z);
				oTeams[2].Pos.x = stf(arTeam.(s3).x);	oTeams[2].Pos.z = stf(arTeam.(s3).z);
				oTeams[3].Pos.x = stf(arTeam.(s4).x);	oTeams[3].Pos.z = stf(arTeam.(s4).z);
			break;
			case NETGAME_FORT:
				oTeams[0].Pos.x = stf(rIsland.net_fort.l1.x);
				oTeams[0].Pos.z = stf(rIsland.net_fort.l1.z);

				oTeams[1].Pos.x = stf(rIsland.net_fort.(sStartPosition).x);
				oTeams[1].Pos.z = stf(rIsland.net_fort.(sStartPosition).z);
			break;
			case NETGAME_CONVOY:
				iRand = 3 * rand(iNumConvoySpawns - 1);
				s1 = "l" + (iRand + 1);
				s2 = "l" + (iRand + 2);
				s3 = "l" + (iRand + 3);

				NetServer.ConvoyPoint.x = stf(arConvoy.(s2).x);
				NetServer.ConvoyPoint.z = stf(arConvoy.(s2).z);

				oTeams[0].Pos.x = stf(arConvoy.(s1).x);
				oTeams[0].Pos.z = stf(arConvoy.(s1).z);

				oTeams[1].Pos.x = stf(arConvoy.(s3).x);
				oTeams[1].Pos.z = stf(arConvoy.(s3).z);
			break;
		}
	}

	Trace("COOP: spawn team0 x=" + oTeams[0].Pos.x + " z=" + oTeams[0].Pos.z + " island=" + NetServer.IslandID);

	// COOP s30: the spawn point of the island (NetServer_GetSpawnLine): the sides stand on the line from it, in random order.
	DeleteAttribute(&NetServer, "Line");
	bool bLine = false;
	if (NetServer.IslandID != "") { bLine = NetServer_GetSpawnLine(NetServer.IslandID); }
	int iLineSideOf[4];
	int iLineSides = 0;
	int iLineDM = 0;
	for (i=0; i<4; i++) { iLineSideOf[i] = -1; }
	if (bLine)
	{
		NetServer.Line.x = stf(NSLine.x);
		NetServer.Line.z = stf(NSLine.z);
		NetServer.Line.ay = stf(NSLine.ay);
		for (i=0; i<NET_MAXCLIENTS; i++)
		{
			if (!sti(NSClients[i].Use)) { continue; }
			int iLT = sti(NSClients[i].Team);
			if (sti(NetServer.GameType) == NETGAME_DEATHMATCH) { iLineSides = iLineSides + 1; continue; }
			if (iLT >= 0 && iLT < 4) { iLineSideOf[iLT] = 1; }
		}
		if (sti(NetServer.GameType) != NETGAME_DEATHMATCH)
		{
			int iLineTeams = 0;
			for (i=0; i<4; i++) { if (iLineSideOf[i] > 0) { iLineTeams = iLineTeams + 1; } }
			bool bLineBack = (rand(1) == 1);		// which team stands at the point
			for (i=0; i<4; i++)
			{
				if (iLineSideOf[i] < 0) { continue; }
				iLineSideOf[i] = iLineSides;
				if (bLineBack) { iLineSideOf[i] = iLineTeams - 1 - iLineSides; }
				string sLineKey = Net_LineTeamKey(i);
				NetServer.Line.Side.(sLineKey) = iLineSideOf[i];
				iLineSides = iLineSides + 1;
			}
		}
		if (iLineSides < 1) { iLineSides = 1; }
		NetServer.Line.Sides = iLineSides;
		Trace("COOP: spawn line of " + NetServer.IslandID + ": x=" + NSLine.x + " z=" + NSLine.z + " ay=" + NSLine.ay + " sides " + iLineSides + " gap " + NetServer_LineGap(iLineSides) + " start position " + NetServer.StartPosition);

		// the convoy goes past the last side (the ring point of the old code may be inside the island)
		if (sti(NetServer.GameType) == NETGAME_CONVOY)
		{
			NetServer_LinePoint(makefloat(iLineSides) * NetServer_LineGap(iLineSides), 0.0);
			NetServer.ConvoyPoint.x = stf(NSLinePt.x);
			NetServer.ConvoyPoint.z = stf(NSLinePt.z);
		}
	}

	// COOP: team start points are remembered for players who join later and for the respawn
	for (i=0; i<4; i++)
	{
		if (!CheckAttribute(&oTeams[i], "Pos.x") || !CheckAttribute(&oTeams[i], "Pos.z")) { continue; }
		string sTeamPos = "t" + i;
		NetServer.TeamPos.(sTeamPos).x = stf(oTeams[i].Pos.x);
		NetServer.TeamPos.(sTeamPos).z = stf(oTeams[i].Pos.z);
	}

	// load players
	
	// send message to all client's for load clients

	// client
	// 1. Ship Type
	//		a. Ship Name
	//		b. Ship Upgrade
	// 2. Cannons Type
	// 3. Crew quantity
	// 4. Skills
	// 5. Perks

	for (i=0; i<NET_MAXCLIENTS; i++)
	{
		if (!sti(NSClients[i].Use)) { continue; }

		NSClients[i].Server = true;

		int iTeam = sti(NSClients[i].Team);

		if ((iTeam == 0) && (sti(NetServer.GameType) == NETGAME_CONVOY) && (iTeams[0] == 0))
		{
			NetServer.ConvoyShipID = NSClients[i].ID;
		}

		iSMsg = NMCreate();
		NMAddByte(iSMsg, NC_CLIENT);
		NMAddByte(iSMsg, NSC_CLIENT_LOAD);
		NMAddClientID(iSMsg, i);									// clientID
		NMAddString(iSMsg, NSClients[i].NickName, 24);				// Nick Name
		NMAddWord(iSMsg, sti(NSClients[i].Ship.Type));				// Ship Type
		NMAddFloat(iSMsg, stf(NSClients[i].Ship.HP));				// Ship HP
		NMAddByte(iSMsg, sti(NSClients[i].Ship.Upgrades.Hull));		// Ship Upgrade: Hull
		NMAddByte(iSMsg, sti(NSClients[i].Ship.Upgrades.Sails));	// Ship Upgrade: Masts
		NMAddByte(iSMsg, sti(NSClients[i].Ship.Cannons.Type));		// Cannons Type
		NMAddByte(iSMsg, sti(NSClients[i].Ship.Upgrades.Cannons));	// Cannons Upgrade
		NMAddWord(iSMsg, sti(NSClients[i].Ship.Crew.Quantity));		// Crew quantity

		// sails parameters

		// skills
		NMAddByte(iSMsg, sti(NSClients[i].Skills.Accuracy));	// Skills: Accuracy
		NMAddByte(iSMsg, sti(NSClients[i].Skills.Cannons));		// Skills: Cannons
		NMAddByte(iSMsg, sti(NSClients[i].Skills.Sailing));		// Skills: Sailing
		NMAddByte(iSMsg, sti(NSClients[i].Skills.Defence));		// Skills: Defence
		NMAddByte(iSMsg, sti(NSClients[i].Skills.Repair));		// Skills: Repair
		NMAddByte(iSMsg, NetServer_GetSkillGrapplingByte(&NSClients[i]));	// Skills: Grappling (COOP: spyglass)
	
		//NMAddBytes(iSMsg, 0);									// Perks: 

		float fRadius = 60.0;
		if (sti(NetServer.GameType) == NETGAME_DEATHMATCH) { fRadius = 350 * (1.0 + stf(NetServer.StartPosition)); }

		if (sti(NetServer.GameType) == NETGAME_DEATHMATCH)
			{ NSClients[i].FriendlyTeam = -1; }
		else
			{ NSClients[i].FriendlyTeam = iTeam; }

		// select ship position
		float x = stf(oTeams[iTeam].Pos.x);
		float z = stf(oTeams[iTeam].Pos.z);
		float dx = makefloat(iTeams[iTeam] - (makeint(iTeams[iTeam] / 4) * 4)) * fRadius;
		float dz = makefloat(iTeams[iTeam] / 4) * fRadius;

		// COOP: universal sea spawn - each team gets its own port, each ship its own sea point of that port
		if (iCoopSpawnMode == 1 && sti(NetServer.CoopN) > 0 && NetServer.IslandID != "")
		{
			ref rCoopIsl = Net_GetIsland(NetServer.IslandID);
			aref arCoopRel; makearef(arCoopRel, rCoopIsl.reload);
			int iCoopN = sti(NetServer.CoopN);
			int iCoopIdx = sti(NetServer.CoopStart) + iTeam;
			iCoopIdx = iCoopIdx - makeint(iCoopIdx / iCoopN) * iCoopN;
			string sCoopK = "l" + iCoopIdx;
			aref arCoopL = GetAttributeN(arCoopRel, sti(NetServer.CoopLoc.(sCoopK)));
			aref arCoopSh; makearef(arCoopSh, arCoopL.ships);
			int iCoopShips = GetAttributesNum(arCoopSh);
			int iCoopSlot = iTeams[iTeam];
			if (iCoopShips > 0) { iCoopSlot = iCoopSlot - makeint(iCoopSlot / iCoopShips) * iCoopShips; }
			string sCoopP = "l" + iCoopSlot;
			if (!CheckAttribute(arCoopSh, sCoopP + ".x") || !CheckAttribute(arCoopSh, sCoopP + ".z")) { sCoopP = "l0"; }
			x = stf(arCoopSh.(sCoopP).x);
			z = stf(arCoopSh.(sCoopP).z);
			dx = 0.0;
			dz = 0.0;
			Trace("COOP: ship " + i + " team " + iTeam + " port " + sCoopK + " point " + sCoopP + " x=" + x + " z=" + z);
		}

		NSClients[i].Ship.Pos.x = x + dx;
		NSClients[i].Ship.Pos.z = z + dz;

		// COOP s30: the spawn line of the island (NetServer_LineShipPlace)
		if (bLine)
		{
			int iLSide = 0;
			int iLRow = iTeams[iTeam];
			if (sti(NetServer.GameType) == NETGAME_DEATHMATCH) { iLSide = iLineDM; iLineDM = iLineDM + 1; iLRow = 0; }
			else
			{
				if (iTeam >= 0 && iTeam < 4) { iLSide = iLineSideOf[iTeam]; }
				if (iLSide < 0) { iLSide = 0; }
			}
			NetServer_LineShipPlace(iLSide, iLineSides, iLRow);
			NSClients[i].Ship.Pos.x = stf(NSLinePt.x);
			NSClients[i].Ship.Pos.z = stf(NSLinePt.z);
			NSClients[i].Ship.Ang.y = stf(NSLinePt.ay);
			Trace("COOP: line spawn ship " + i + " side " + iLSide + " row " + iLRow + " x=" + NSLinePt.x + " z=" + NSLinePt.z + " ay=" + NSLinePt.ay);
		}
		// a new entity never starts with the height / speed of an older game
		NSClients[i].Ship.Pos.y = 0.0;
		if (!CheckAttribute(&NSClients[i], "Ship.Ang.y")) { NSClients[i].Ship.Ang.y = frnd() * PIm2; }
		NSClients[i].Ship.Speed.x = 0.0;
		NSClients[i].Ship.Speed.y = 0.0;
		NSClients[i].Ship.Speed.z = 0.0;

		NSClients[i].LastCollisionShip = -500;
		NSClients[i].LastCollisionIsland = -500;

		NSClients[i].Ship.TurnLeft = 0.0;
		NSClients[i].Ship.TurnRight = 0.0;
		NSClients[i].Ship.TurnState = 0.0;
		NSClients[i].Ship.SailState = 0.0;

		// load ship for server
		ref rShip = Net_GetShipByIndex(sti(NSClients[i].Ship.Type));
		NSClients[i].Model = rShip.Name + "1";

		// when ship create, he trying to check - can he be placed there, or not, if not he must found new point to spawn
		CreateEntity(&NSClients[i], "NetShip");
		SendMessage(&NSClients[i], "la", MSG_SHIP_CREATE, rShip);

		LayerAddObject("net_execute", &NSClients[i], 31);
		LayerAddObject("ship_cannon_trace", &NSClients[i], 31);

		PostEvent("NetServer_ShipUpdateParameters", 1, "l", i);

		// COOP: the ship of this slot exists in this game; remember the start point and the bought loadout (respawn, reconnect)
		NSClients[i].Spawned = true;
		NSClients[i].InGame = true;
		NSClients[i].Parked = false;
		NSClients[i].Reconnect = false;
		NSClients[i].Dead = false;
		NSClients[i].Life = 0;
		NSClients[i].SpawnPos.x = stf(NSClients[i].Ship.Pos.x);
		NSClients[i].SpawnPos.z = stf(NSClients[i].Ship.Pos.z);
		NSClients[i].SpawnPos.ay = stf(NSClients[i].Ship.Ang.y);
		NetServer_SnapshotLoadout(&NSClients[i]);

		iTeams[iTeam] = iTeams[iTeam] + 1;
		
		// 
		NMAddFloat(iSMsg, stf(NSClients[i].Ship.Pos.x));		// pos.x
		NMAddFloat(iSMsg, stf(NSClients[i].Ship.Pos.y));		// pos.y
		NMAddFloat(iSMsg, stf(NSClients[i].Ship.Pos.z));		// pos.z
		NMAddFloat(iSMsg, stf(NSClients[i].Ship.Ang.y));		// ang.y

		// add goods
		for (int j=0; j<iNetGoodsNum; j++)
		{
			ref rGood = Net_GetGoodByIndex(j);
			string sGoodName = rGood.Name;
			NMAddWord(iSMsg, sti(NSClients[i].Ship.Goods.(sGoodName)));
		}
		NMAddByte(iSMsg, sti(NSClients[i].Ship.Cannons.Charge.Type));	// start charge
		int iSeaPerksS = 0;
		if (CheckAttribute(&NSClients[i], "SeaPerks")) { iSeaPerksS = sti(NSClients[i].SeaPerks); }
		NMAddDword(iSMsg, iSeaPerksS);		// COOP s28: the sea perks bought in the lobby (client\OnClient.c reads it)

		// send message
		NetServer_SendMessage(DST_ALL, iSMsg, true);
		NMDelete(iSMsg);
	}

	// send some game info
	if (sti(NetServer.GameType) == NETGAME_CONVOY)
	{
		iSMsg = NMCreate();
		NMAddByte(iSMsg, NC_GAMEINFO);
		NMAddByte(iSMsg, sti(NetServer.GameType));
		NMAddClientID(iSMsg, sti(NetServer.ConvoyShipID));
		NMAddFloat(iSMsg, stf(NetServer.ConvoyPoint.x));
		NMAddFloat(iSMsg, stf(NetServer.ConvoyPoint.z));
		NetServer_SendMessage(DST_ALL, iSMsg, true);
		NMDelete(iSMsg);
	}

	SetEventHandler("frame", "NetServer_OnCheckGameOverFrame", 1);
}

void NetServer_DownloadClientFile(int wClientID, string sFileName)
{
	if (!sti(NetServer.AllowDownload)) { return; }
	//if (NFExist(sFileName)) { return; }

	int iFileID1 = NFRCreateFile(wClientID, sFileName, -1, -1);

	Trace("NetServer try download client file: " + sFileName);

	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_FILE);
	NMAddByte(iSMsg, NSC_FILE_GET_FILE);
	NMAddDword(iSMsg, 0);
	NMAddDword(iSMsg, iFileID1);
	NMAddString(iSMsg, sFileName, 128);
	NetServer_SendMessage(wClientID, iSMsg, true);
	NMDelete(iSMsg);
}

// ================================================================================================
// COOP: helpers of the running game: late join, reconnect, respawn
// ================================================================================================
object NSSpawnCand;		// spawn point candidates (NetServer_PickSpawn)
object NSSpawn;			// the chosen point: x, z, ay

bool NetServer_IsParked(int i)
{
	if (i < 0 || i >= NET_MAXCLIENTS) { return false; }
	if (!CheckAttribute(&NSClients[i], "Parked")) { return false; }
	return sti(NSClients[i].Parked) != 0;
}

// the slot has a ship entity in this game (alive, wrecked or sinking)
bool NetServer_HasShip(int i)
{
	if (i < 0 || i >= NET_MAXCLIENTS) { return false; }
	if (!CheckAttribute(&NSClients[i], "Spawned")) { return false; }
	if (!sti(NSClients[i].Spawned)) { return false; }
	if (sti(NSClients[i].Ship.Type) < 0) { return false; }
	return true;
}

// the ship is afloat and fights (not wrecked / sinking)
bool NetServer_ShipIsAlive(int i)
{
	if (!NetServer_HasShip(i)) { return false; }
	if (sti(NSClients[i].Dead)) { return false; }
	if (!CheckAttribute(&NSClients[i], "Ship.HP")) { return false; }
	if (stf(NSClients[i].Ship.HP) <= 0.0) { return false; }
	return true;
}

// the client got the environment of the running game (a client in the lobby must not get ship loads: no sea there)
bool NetServer_IsInGame(int i)
{
	if (i < 0 || i >= NET_MAXCLIENTS) { return false; }
	if (!sti(NSClients[i].Use)) { return false; }
	if (!CheckAttribute(&NSClients[i], "InGame")) { return false; }
	return sti(NSClients[i].InGame) != 0;
}

// guaranteed message to every client that plays (not to the lobby)
void NetServer_SendToInGame(int iSMsg)
{
	for (int i=0; i<NET_MAXCLIENTS; i++)
	{
		if (!NetServer_IsInGame(i)) { continue; }
		NetServer_SendMessage(i, iSMsg, true);
	}
}

// the bought loadout: given back at the respawn
void NetServer_SnapshotLoadout(ref rClient)
{
	rClient.Ship.Start.Crew = sti(rClient.Ship.Crew.Quantity);
	rClient.Ship.Start.Charge = sti(rClient.Ship.Cannons.Charge.Type);
	rClient.Ship.Start.CannonsDumped = sti(rClient.Ship.CannonsDumped);		// COOP: cannons bought in the lobby
	for (int b=0; b<NET_BORT_NUM; b++)
	{
		string sSB = Net_BortLetter(b);
		rClient.Ship.Start.Bort.(sSB) = NetServer_BortHave(rClient, b);
	}
	for (int j=0; j<Net_GetGoodsNum(); j++)
	{
		ref rGood = Net_GetGoodByIndex(j);
		string sGood = rGood.Name;
		rClient.Ship.Start.Goods.(sGood) = sti(rClient.Ship.Goods.(sGood));
	}
}

// WindCatcher (passive perk): acceleration +10% for every ship type. The base values are kept, so a second game does not add it twice.
void NetServer_ApplyShipPerksToTypes()
{
	float fMul = 1.0;
	if (sti(NetServer.PassivePerks)) { fMul = 1.1; }

	for (int i=0; i<Net_GetShipsNum(); i++)
	{
		ref rShip = Net_GetShipByIndex(i);
		if (!CheckAttribute(rShip, "InertiaAccelerationX")) { continue; }
		if (!CheckAttribute(rShip, "NetBaseInertia.x"))
		{
			rShip.NetBaseInertia.x = stf(rShip.InertiaAccelerationX);
			rShip.NetBaseInertia.y = stf(rShip.InertiaAccelerationY);
			rShip.NetBaseInertia.z = stf(rShip.InertiaAccelerationZ);
		}
		rShip.InertiaAccelerationX = stf(rShip.NetBaseInertia.x) * fMul;
		rShip.InertiaAccelerationY = stf(rShip.NetBaseInertia.y) * fMul;
		rShip.InertiaAccelerationZ = stf(rShip.NetBaseInertia.z) * fMul;
	}
}

// The data of NSC_STARTGAME_ENVIRONMENT for a player who comes into the running game: the same layout as
// NetServer_StartServerGame writes (weather, balls, fort, time), with the CURRENT values (weather / wind of the admin, time).
void NetServer_AddEnvironmentData(int iSMsg)
{
	// weather (server\weather\Weather.c)
	NMAddString(iSMsg, NetServer.WeatherID, 64);
	NMAddFloat(iSMsg, fServerWeatherAngle);
	NMAddFloat(iSMsg, fServerWeatherSpeed);

	// island: nothing; balls
	NMAddFloat(iSMsg, stf(NSBalls.SpeedMultiply));

	// touch, rigging, ship: nothing; fort (server\Fort.c)
	if (CheckAttribute(&NetServer, "FortLocator") && NetServer.FortLocator != "")
	{
		NMAddByte(iSMsg, true);
		NMAddString(iSMsg, NetServer.FortLocator, 16);
		NMAddString(iSMsg, NetServer.FortIsland, 24);		// COOP: the island whose fort model is used (the fort of Bot+ may use another island)
		NMAddFloat(iSMsg, stf(NetServer.FortDX));            // COOP: the shift of the fort from its locator
		NMAddFloat(iSMsg, stf(NetServer.FortDZ));
	}
	else
	{
		NMAddByte(iSMsg, false);
	}

	// time (+ milliseconds left until the end of the match, 0 = no limit)
	NMAddDword(iSMsg, iServerTime);
	NMAddDword(iSMsg, NetServer_TimeLeftMs());
}

// COOP: Grappling skill of a client for NSC_CLIENT_LOAD (the client shows it in the spyglass)
int NetServer_GetSkillGrapplingByte(ref rC)
{
	if (!CheckAttribute(rC, "Skills.Grappling")) { return 1; }
	return sti(rC.Skills.Grappling);
}

// NSC_CLIENT_LOAD of an existing ship with its current state (same layout as in NetServer_StartServerGame).
// The client sets MaxHP from the HP of this message, so MaxHP is sent; the current HP comes with the next ship info.
int NetServer_CreateClientLoadMessage(int i)
{
	ref rC = NetServer_GetClient(i);

	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_CLIENT);
	NMAddByte(iSMsg, NSC_CLIENT_LOAD);
	NMAddClientID(iSMsg, i);
	NMAddString(iSMsg, rC.NickName, 24);
	NMAddWord(iSMsg, sti(rC.Ship.Type));
	NMAddFloat(iSMsg, stf(rC.Ship.MaxHP));
	NMAddByte(iSMsg, sti(rC.Ship.Upgrades.Hull));
	NMAddByte(iSMsg, sti(rC.Ship.Upgrades.Sails));
	NMAddByte(iSMsg, sti(rC.Ship.Cannons.Type));
	NMAddByte(iSMsg, sti(rC.Ship.Upgrades.Cannons));
	NMAddWord(iSMsg, sti(rC.Ship.Crew.Quantity));

	NMAddByte(iSMsg, sti(rC.Skills.Accuracy));
	NMAddByte(iSMsg, sti(rC.Skills.Cannons));
	NMAddByte(iSMsg, sti(rC.Skills.Sailing));
	NMAddByte(iSMsg, sti(rC.Skills.Defence));
	NMAddByte(iSMsg, sti(rC.Skills.Repair));
	NMAddByte(iSMsg, NetServer_GetSkillGrapplingByte(rC));	// COOP: Grappling for the spyglass

	NMAddFloat(iSMsg, stf(rC.Ship.Pos.x));
	NMAddFloat(iSMsg, 0.0);
	NMAddFloat(iSMsg, stf(rC.Ship.Pos.z));
	NMAddFloat(iSMsg, stf(rC.Ship.Ang.y));

	for (int j=0; j<iNetGoodsNum; j++)
	{
		ref rGood = Net_GetGoodByIndex(j);
		string sGoodName = rGood.Name;
		NMAddWord(iSMsg, sti(rC.Ship.Goods.(sGoodName)));
	}
	NMAddByte(iSMsg, sti(rC.Ship.Cannons.Charge.Type));
	int iSeaPerksL = 0;
	if (CheckAttribute(rC, "SeaPerks")) { iSeaPerksL = sti(rC.SeaPerks); }
	NMAddDword(iSMsg, iSeaPerksL);		// COOP s28: the sea perks bought in the lobby (client\OnClient.c reads it)
	return iSMsg;
}

void NetServer_SendDropTo(int wToClientID, int wAboutClientID)
{
	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_CLIENT);
	NMAddByte(iSMsg, NSC_CLIENT_DROP);
	NMAddClientID(iSMsg, wAboutClientID);
	NetServer_SendMessage(wToClientID, iSMsg, true);
	NMDelete(iSMsg);
}

// creates the ship entity of the SERVER (same sequence as NetServer_StartServerGame / NetServer_AdminPlaceShip)
void NetServer_CreateShipEntity(int i)
{
	ref rShip = Net_GetShipByIndex(sti(NSClients[i].Ship.Type));
	NSClients[i].Model = rShip.Name + "1";
	CreateEntity(&NSClients[i], "NetShip");
	SendMessage(&NSClients[i], "la", MSG_SHIP_CREATE, rShip);
	LayerAddObject("net_execute", &NSClients[i], 31);
	LayerAddObject("ship_cannon_trace", &NSClients[i], 31);
}

// the NaN height watch of the first seconds (client\Time.c) for a ship created in the middle of the game
void NetServer_RestartShipWatch()
{
	bool bWatchRunning = false;
	if (iCoopSrvWatchTicks < 14) { bWatchRunning = true; }
	iCoopSrvWatchTicks = 0;
	if (!bWatchRunning) { PostEvent("COOP_SrvShipWatch", 700); }
}

// Puts an existing server ship to a new place: the engine reads Ship.Pos / Ang only when the ship is created,
// so the entity of the SERVER is deleted and created again (the clients get the place with the ship info packets).
void NetServer_PlaceShipEntity(int i, float fX, float fZ, float fAy)
{
	ref rClient = NetServer_GetClient(i);

	float fAng = fAy;
	while (fAng >= PIm2) { fAng = fAng - PIm2; }
	while (fAng < 0.0) { fAng = fAng + PIm2; }

	rClient.Ship.Pos.x = fX;
	rClient.Ship.Pos.y = 0.0;		// a new ship must never start with an old / NaN height
	rClient.Ship.Pos.z = fZ;
	rClient.Ship.Ang.y = fAng;
	rClient.Ship.Speed.x = 0.0;
	rClient.Ship.Speed.y = 0.0;
	rClient.Ship.Speed.z = 0.0;
	rClient.LastCollisionShip = -500;
	rClient.LastCollisionIsland = -500;
	rClient.CoopFix = 0;

	DeleteClass(&NSClients[i]);
	NetServer_CreateShipEntity(i);
	NetServer_RestartShipWatch();
}

void NetServer_SpawnCandAdd(float fX, float fZ, float fAy)
{
	int n = sti(NSSpawnCand.Num);
	string sC = "l" + n;
	NSSpawnCand.(sC).x = fX;
	NSSpawnCand.(sC).z = fZ;
	NSSpawnCand.(sC).ay = fAy;
	NSSpawnCand.Num = n + 1;
}

// COOP: spawn point for a late player / a respawn. Candidates: the start points of this game (own team only in team games),
// the sea points of the island ports (the same points the start uses) and the team start point. The point that is the
// farthest from the enemies is taken; a point with a ship closer than 45 m is used only if there is nothing else.
void NetServer_PickSpawn(int wID)
{
	int i, j, n;
	string sC;
	ref rC = NetServer_GetClient(wID);
	int iTeam = sti(rC.Team);
	bool bTeamGame = true;
	if (sti(NetServer.GameType) == NETGAME_DEATHMATCH) { bTeamGame = false; }

	DeleteAttribute(&NSSpawnCand, "");
	NSSpawnCand.Num = 0;

	// COOP s30: the spawn line of the game: the places of the own side (every side place in a deathmatch / for a new team)
	bool bLineGame = false;
	if (NetServer.IslandID != "") { bLineGame = NetServer_UseGameLine(); }
	if (bLineGame)
	{
		int iLSides = 1;
		if (CheckAttribute(&NetServer, "Line.Sides")) { iLSides = sti(NetServer.Line.Sides); }
		string sLTeam = Net_LineTeamKey(iTeam);
		if (bTeamGame && CheckAttribute(&NetServer, "Line.Side." + sLTeam))
		{
			int iLS = sti(NetServer.Line.Side.(sLTeam));
			for (j=0; j<6; j++)
			{
				NetServer_LineShipPlace(iLS, iLSides, j);
				NetServer_SpawnCandAdd(stf(NSLinePt.x), stf(NSLinePt.z), stf(NSLinePt.ay));
			}
		}
		else
		{
			int iLDM = iLSides;
			if (iLDM < 2) { iLDM = 2; }
			for (j=0; j<iLDM; j++)
			{
				NetServer_LineShipPlace(j, iLDM, 0);
				NetServer_SpawnCandAdd(stf(NSLinePt.x), stf(NSLinePt.z), stf(NSLinePt.ay));
			}
		}
	}

	// own start point and the start points of the others
	for (i=0; i<NET_MAXCLIENTS; i++)
	{
		if (!NetServer_HasShip(i) && i != wID) { continue; }
		if (!CheckAttribute(&NSClients[i], "SpawnPos.x")) { continue; }
		if (bTeamGame && sti(NSClients[i].Team) != iTeam) { continue; }
		NetServer_SpawnCandAdd(stf(NSClients[i].SpawnPos.x), stf(NSClients[i].SpawnPos.z), stf(NSClients[i].SpawnPos.ay));
	}

	// sea points of the island ports (server\OnStart.c: NetServer.CoopLoc), the team port only in team games (not with a spawn line)
	if (!bLineGame && NetServer.IslandID != "" && CheckAttribute(&NetServer, "CoopN") && sti(NetServer.CoopN) > 0)
	{
		ref rIsl = Net_GetIsland(NetServer.IslandID);
		aref arRel; makearef(arRel, rIsl.reload);
		int iCoopN = sti(NetServer.CoopN);
		int iTeamPort = sti(NetServer.CoopStart) + iTeam;
		iTeamPort = iTeamPort - makeint(iTeamPort / iCoopN) * iCoopN;
		for (i=0; i<iCoopN; i++)
		{
			if (bTeamGame && i != iTeamPort) { continue; }
			string sK = "l" + i;
			if (!CheckAttribute(&NetServer, "CoopLoc." + sK)) { continue; }
			aref arL = GetAttributeN(arRel, sti(NetServer.CoopLoc.(sK)));
			aref arSh; makearef(arSh, arL.ships);
			n = GetAttributesNum(arSh);
			for (j=0; j<n; j++)
			{
				aref arP = GetAttributeN(arSh, j);
				if (!CheckAttribute(arP, "x") || !CheckAttribute(arP, "z")) { continue; }
				float fPAy = frnd() * PIm2;
				if (CheckAttribute(arP, "ay")) { fPAy = stf(arP.ay); }
				NetServer_SpawnCandAdd(stf(arP.x), stf(arP.z), fPAy);
			}
		}
	}

	// the start point of the team (open sea)
	string sTeamPos = "t" + iTeam;
	if (!bTeamGame) { sTeamPos = "t0"; }
	if (!bLineGame && CheckAttribute(&NetServer, "TeamPos." + sTeamPos + ".x"))
	{
		float fTX = stf(NetServer.TeamPos.(sTeamPos).x);
		float fTZ = stf(NetServer.TeamPos.(sTeamPos).z);
		NetServer_SpawnCandAdd(fTX, fTZ, frnd() * PIm2);
		if (NetServer.IslandID == "")
		{
			// open sea: a few more points around the team point
			NetServer_SpawnCandAdd(fTX + 150.0, fTZ, frnd() * PIm2);
			NetServer_SpawnCandAdd(fTX - 150.0, fTZ, frnd() * PIm2);
			NetServer_SpawnCandAdd(fTX, fTZ + 150.0, frnd() * PIm2);
			NetServer_SpawnCandAdd(fTX, fTZ - 150.0, frnd() * PIm2);
		}
	}

	n = sti(NSSpawnCand.Num);
	if (n == 0)
	{
		// nothing known: a point on the ring of the start code around the island origin / the open sea centre
		float fA = frnd() * PIm2;
		NetServer_SpawnCandAdd(makefloat(iCoopRingRadius) * sin(fA), makefloat(iCoopRingRadius) * cos(fA), fA + PI);
		n = 1;
	}

	int iBest = 0;
	float fBestScore = -1000000.0;
	for (i=0; i<n; i++)
	{
		sC = "l" + i;
		float fX = stf(NSSpawnCand.(sC).x);
		float fZ = stf(NSSpawnCand.(sC).z);
		float fMinEnemy = 100000.0;
		float fMinAny = 100000.0;
		for (j=0; j<NET_MAXCLIENTS; j++)
		{
			if (j == wID) { continue; }
			if (!NetServer_ShipIsAlive(j)) { continue; }
			float fDX = stf(NSClients[j].Ship.Pos.x) - fX;
			float fDZ = stf(NSClients[j].Ship.Pos.z) - fZ;
			float fD = sqrt(fDX * fDX + fDZ * fDZ);
			if (fD < fMinAny) { fMinAny = fD; }
			if (bTeamGame && sti(NSClients[j].Team) == iTeam) { continue; }
			if (fD < fMinEnemy) { fMinEnemy = fD; }
		}
		float fScore = fMinEnemy + frnd() * 20.0;
		if (fMinAny < 45.0) { fScore = fScore - 50000.0; }
		if (fScore > fBestScore) { fBestScore = fScore; iBest = i; }
	}

	sC = "l" + iBest;
	NSSpawn.x = stf(NSSpawnCand.(sC).x);
	NSSpawn.z = stf(NSSpawnCand.(sC).z);
	NSSpawn.ay = stf(NSSpawnCand.(sC).ay);
	Trace("COOP: spawn point for " + wID + ": x=" + NSSpawn.x + " z=" + NSSpawn.z + " (" + n + " candidates)");
}

// COOP: a player comes into the RUNNING game.
//   bRestore = false: a new player (or a player whose ship was sunk) with the ship he has just bought in the lobby
//   bRestore = true:  a reconnect, he gets his old ship (it waited parked)
// He gets: the environment, his ship, all other ships (ships of players who left: load + drop), the mines, the convoy point.
void NetServer_LateJoin(int wID, bool bRestore)
{
	int i, iSMsg;
	ref rC = NetServer_GetClient(wID);

	if (!bServerGameStarted) { return; }
	if (!sti(rC.Use)) { return; }
	if (NetServer_IsInGame(wID)) { return; }
	if (NetServer_LandMode()) { NetServer_LandJoin(wID); return; }		// COOP: land duel (Land.c)
	if (bRestore && !NetServer_HasShip(wID)) { bRestore = false; }

	int iRestoreLog = 0;
	if (bRestore) { iRestoreLog = 1; }
	Trace("COOP: late join id=" + wID + " (" + rC.NickName + ") restore=" + iRestoreLog);

	// 1. environment of the game
	iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_STARTGAME);
	NMAddByte(iSMsg, NSC_STARTGAME_ENVIRONMENT);
	NetServer_AddEnvironmentData(iSMsg);
	NetServer_SendMessage(wID, iSMsg, true);
	NMDelete(iSMsg);

	rC.InGame = true;
	rC.Reconnect = false;
	rC.JoinRequested = false;

	// 2. own ship
	if (bRestore)
	{
		// the ship exists on every client already: only the player himself gets its load
		iSMsg = NetServer_CreateClientLoadMessage(wID);
		NetServer_SendMessage(wID, iSMsg, true);
		NMDelete(iSMsg);
	}
	else
	{
		NetServer_SpawnLateShip(wID);

		iSMsg = NetServer_CreateClientLoadMessage(wID);
		NetServer_SendToInGame(iSMsg);
		NMDelete(iSMsg);
	}

	// 3. the other ships (sinking ships are not sent: they go down anyway)
	for (i=0; i<NET_MAXCLIENTS; i++)
	{
		if (i == wID) { continue; }
		if (!NetServer_HasShip(i)) { continue; }
		if (sti(NSClients[i].Dead) && !sti(NetServer.Respawn)) { continue; }
		bool bBotSlot = NetServer_IsBot(i);
		// COOP s30: the wreck of a player who left for good (kicked: his slot is not parked) never comes back, the respawn
		// waits for him; the new player got it as a whole ship with no hull
		if (!bBotSlot && sti(NSClients[i].Dead) && !sti(NSClients[i].Use) && !NetServer_IsParked(i)) { continue; }
		bool bBotGone = false;		// COOP: a sunk or deleted bot is not sent
		if (bBotSlot)
		{
			if (sti(NSClients[i].Dead)) { bBotGone = true; }
			if (NetServer_BotRemoved(i)) { bBotGone = true; }
		}
		if (bBotGone) { continue; }

		// COOP: a bot: its names and relation before the ship (server\Bots.c)
		if (bBotSlot)
		{
			iSMsg = NetServer_BotInfoMessage(i, false);
			NetServer_SendMessage(wID, iSMsg, true);
			NMDelete(iSMsg);
		}

		iSMsg = NetServer_CreateClientLoadMessage(i);
		NetServer_SendMessage(wID, iSMsg, true);
		NMDelete(iSMsg);

		// the player of this ship has left the game: the ship stays, the player is not in the list
		if (!sti(NSClients[i].Use) && !bBotSlot) { NetServer_SendDropTo(wID, i); }
	}

	// 4. mines in the sea
	for (i=0; i<NET_MINE_MAX; i++)
	{
		if (!sti(NSMines[i].Use)) { continue; }
		iSMsg = NMCreate();
		NMAddByte(iSMsg, NC_SHIP);
		NMAddByte(iSMsg, NSC_SHIP_MINE_ADD);
		NMAddByte(iSMsg, i);
		NMAddFloat(iSMsg, stf(NSMines[i].x));
		NMAddFloat(iSMsg, stf(NSMines[i].z));
		NMAddClientID(iSMsg, sti(NSMines[i].Owner));
		NetServer_SendMessage(wID, iSMsg, true);
		NMDelete(iSMsg);
	}

	// 5. convoy point
	if (sti(NetServer.GameType) == NETGAME_CONVOY)
	{
		iSMsg = NMCreate();
		NMAddByte(iSMsg, NC_GAMEINFO);
		NMAddByte(iSMsg, sti(NetServer.GameType));
		NMAddClientID(iSMsg, sti(NetServer.ConvoyShipID));
		NMAddFloat(iSMsg, stf(NetServer.ConvoyPoint.x));
		NMAddFloat(iSMsg, stf(NetServer.ConvoyPoint.z));
		NetServer_SendMessage(wID, iSMsg, true);
		NMDelete(iSMsg);
	}

	// 6. state of the active perks of his ship, admin flags of the others
	NetServer_SendAllPerkStates(wID);
	for (i=0; i<NET_MAXCLIENTS; i++)
	{
		if (!sti(NSClients[i].Use)) { continue; }
		NetServer_AdminSendFlags(wID, i);
	}

	// 7. a wrecked ship of a reconnecting player (respawn mode) comes back at once
	if (bRestore && sti(rC.Dead) && sti(NetServer.Respawn))
	{
		NetServer_RespawnShip(wID);
	}
}

// the server ship of a player who comes into the running game with a new ship
void NetServer_SpawnLateShip(int wID)
{
	ref rC = NetServer_GetClient(wID);

	rC.Server = true;
	if (sti(NetServer.GameType) == NETGAME_DEATHMATCH) { rC.FriendlyTeam = -1; }
	else { rC.FriendlyTeam = sti(rC.Team); }

	rC.Dead = false;
	rC.Life = 0;
	DeleteAttribute(rC, "Killer");
	DeleteAttribute(rC, "Ship.Sink");
	DeleteAttribute(rC, "Ship.sails");
	DeleteAttribute(rC, "Ship.masts");
	rC.Ship.TurnLeft = 0.0;
	rC.Ship.TurnRight = 0.0;
	rC.Ship.TurnState = 0.0;
	rC.Ship.SailState = 0.0;
	rC.BoardSent = -1;

	NetServer_PickSpawn(wID);
	rC.Ship.Pos.x = stf(NSSpawn.x);
	rC.Ship.Pos.y = 0.0;
	rC.Ship.Pos.z = stf(NSSpawn.z);
	rC.Ship.Ang.y = stf(NSSpawn.ay);
	rC.Ship.Speed.x = 0.0;
	rC.Ship.Speed.y = 0.0;
	rC.Ship.Speed.z = 0.0;
	rC.LastCollisionShip = -500;
	rC.LastCollisionIsland = -500;
	rC.CoopFix = 0;

	// the slot may have had a ship in an older game: a fresh entity
	if (IsEntity(&NSClients[wID])) { DeleteClass(&NSClients[wID]); }
	NetServer_CreateShipEntity(wID);

	rC.Spawned = true;
	rC.SpawnPos.x = stf(rC.Ship.Pos.x);
	rC.SpawnPos.z = stf(rC.Ship.Pos.z);
	rC.SpawnPos.ay = stf(rC.Ship.Ang.y);
	NetServer_SnapshotLoadout(rC);

	PostEvent("NetServer_ShipUpdateParameters", 1, "l", wID);
	NetServer_RestartShipWatch();

	Trace("COOP: late ship id=" + wID + " type=" + rC.Ship.Type + " x=" + rC.Ship.Pos.x + " z=" + rC.Ship.Pos.z);
}
