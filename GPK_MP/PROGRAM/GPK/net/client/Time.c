int iServerDeltaTimeFraction = 0;
int iCoopSrvWatchTicks = 0;

// COOP: true if the ship height is NaN / INF (string form contains N or #)
bool COOP_SrvYBad(ref rC)
{
	string sY = "" + rC.Ship.Pos.y;
	if (findSubStr(&sY, "N", 0) >= 0) { return true; }
	if (findSubStr(&sY, "#", 0) >= 0) { return true; }
	return false;
}

void COOP_SrvRecreateShip(int i)
{
	ref rC = NetServer_GetClient(i);
	ref rShip = Net_GetShipByIndex(sti(rC.Ship.Type));
	rC.CoopFix = sti(rC.CoopFix) + 1;
	Trace("COOP: srv ship " + i + " height is NaN, recreating, attempt " + rC.CoopFix);
	DeleteClass(&NSClients[i]);
	NSClients[i].Ship.Pos.y = 0.0;
	NSClients[i].Ship.Speed.x = 0.0;
	NSClients[i].Ship.Speed.y = 0.0;
	NSClients[i].Ship.Speed.z = 0.0;
	CreateEntity(&NSClients[i], "NetShip");
	SendMessage(&NSClients[i], "la", MSG_SHIP_CREATE, rShip);
	LayerAddObject("net_execute", &NSClients[i], 31);
	LayerAddObject("ship_cannon_trace", &NSClients[i], 31);
}

void COOP_SrvShipWatch()
{
	int i;
	iCoopSrvWatchTicks = iCoopSrvWatchTicks + 1;
	for (i=0; i<NET_MAXCLIENTS; i++)
	{
		if (!NetServer_HasShip(i)) { continue; }		// COOP: lobby players of the running game have no ship, parked ships do
		ref rC = NetServer_GetClient(i);
		if (NetServer_NoclipUsed(rC)) { continue; }		// COOP s27: the noclip of the admin puts the ship anywhere (server\Admin.c)
		if (COOP_SrvYBad(rC) && sti(rC.CoopFix) < 6) { COOP_SrvRecreateShip(i); }
	}
	if (iCoopSrvWatchTicks < 14) { PostEvent("COOP_SrvShipWatch", 700); }
}

void NetServer_DeleteTimeEnvironment()
{
	DelEventHandler("COOP_SrvShipWatch", "COOP_SrvShipWatch");
	DelEventHandler("frame", "NetServer_TimeFrame");
}

void NetServer_CreateTimeEnvironment(int iServerEnvSMsg)
{
	iServerTime = 300;
	SetEventHandler("frame", "NetServer_TimeFrame", 0);

	// COOP: time limit of the match (minutes from the Create game screen, 0 = unlimited)
	NetServer.TimeEnd = 0;
	NetServer.TimeUp = false;
	if (CheckAttribute(&NetServer, "TimeLimit") && sti(NetServer.TimeLimit) > 0)
	{
		NetServer.TimeEnd = iServerTime + sti(NetServer.TimeLimit) * 60000;
	}

	// COOP: watch ship height for the first seconds (NetShip may start with NaN height and then never moves)
	iCoopSrvWatchTicks = 0;
	SetEventHandler("COOP_SrvShipWatch", "COOP_SrvShipWatch", 0);
	PostEvent("COOP_SrvShipWatch", 2500);

	iServerDeltaTimeFraction = 0;
	NMAddDword(iServerEnvSMsg, iServerTime);
	NMAddDword(iServerEnvSMsg, NetServer_TimeLeftMs());
}

// COOP: milliseconds until the end of the match for the clients: 0 = no limit, 1 = time is already up
int NetServer_TimeLeftMs()
{
	if (!CheckAttribute(&NetServer, "TimeEnd")) { return 0; }
	if (sti(NetServer.TimeEnd) <= 0) { return 0; }
	int iLeft = sti(NetServer.TimeEnd) - iServerTime;
	if (iLeft < 1) { iLeft = 1; }
	return iLeft;
}

// COOP: the time is up: the winner is the player (team) with the most damage inflicted, announced like a normal game over
void NetServer_TimeUpAnnounce()
{
	int i;
	int iTeamDamage[4];
	for (i=0; i<4; i++) { iTeamDamage[i] = 0; }

	int iBestID = DST_INVALID;
	int iBestDamage = -1;
	for (i=0; i<NET_MAXCLIENTS; i++)
	{
		if (!NetServer_TeamSideSlot(i)) { continue; }		// COOP s28: the damage of the team bots goes to their team
		int iDamage = 0;
		if (CheckAttribute(&NSClients[i], "Stat.DamageInflicted")) { iDamage = sti(NSClients[i].Stat.DamageInflicted); }
		if (sti(NSClients[i].Use) && iDamage > iBestDamage) { iBestDamage = iDamage; iBestID = i; }
		int iTeam = sti(NSClients[i].Team);
		if (iTeam >= 0 && iTeam < 4) { iTeamDamage[iTeam] = iTeamDamage[iTeam] + iDamage; }
	}

	int iBestTeam = 0;
	for (i=1; i<4; i++)
	{
		if (iTeamDamage[i] > iTeamDamage[iBestTeam]) { iBestTeam = i; }
	}

	// COOP: in Defend the convoy the time is up and the convoy ship is not in the point: the defenders kept it
	// from reaching, so the defending team wins even with less damage (a convoy ship inside the point is still a red win)
	if (sti(NetServer.GameType) == NETGAME_CONVOY)
	{
		iBestTeam = 1;
		iBestID = DST_INVALID;
		int iConvoyAtPoint = 0;
		if (NetServer_ConvoyReached()) { iBestTeam = 0; iConvoyAtPoint = 1; }
		Trace("COOP: time is up in Defend the convoy, the convoy is at the point=" + iConvoyAtPoint + ", winner team=" + iBestTeam);
	}

	NetServer.GameOver = true;
	NetServer.ContestStarted = false;
	NetServer.Draw = 0;
	NetServer.wWinnerClientID = iBestID;
	NetServer.iWinnerTeam = iBestTeam;
	if (sti(NetServer.GameType) == NETGAME_DEATHMATCH) { NetServer.iWinnerTeam = -1; }
	Trace("COOP: time is up, winner id=" + iBestID + " team=" + iBestTeam);

	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_STARTGAME);
	NMAddByte(iSMsg, NSC_STARTGAME_GAMEOVER);
	NMAddByte(iSMsg, sti(NetServer.GameType));
	NMAddClientID(iSMsg, iBestID);
	NMAddByte(iSMsg, sti(NetServer.iWinnerTeam));
	NMAddByte(iSMsg, 0);
	NetServer_SendMessage(DST_ALL, iSMsg, true);
	NMDelete(iSMsg);
}

void NetServer_TimeFrame()
{
	if (fHighPrecisionDeltaTime > 0.2) { Trace("COOP: dt spike srv t=" + iServerTime + " dt=" + fHighPrecisionDeltaTime); }
	int iFullDeltaTime = MakeInt(fHighPrecisionDeltaTime * 1000000.0);
	int iDeltaTime = iFullDeltaTime / 1000;
	iServerDeltaTimeFraction = iServerDeltaTimeFraction + (iFullDeltaTime - iDeltaTime * 1000);

	iServerTime = iServerTime + iDeltaTime;

	if (iServerDeltaTimeFraction > 1000)
	{
		int iMilliseconds = iServerDeltaTimeFraction / 1000;
		iServerTime = iServerTime + iMilliseconds;
		iServerDeltaTimeFraction = iServerDeltaTimeFraction - (iMilliseconds * 1000);
	}
}
