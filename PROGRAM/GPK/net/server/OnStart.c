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
int iServerEnvSMsg;

void NetServer_EndGame(bool bReconnect)
{
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
				if (!sti(NSClients[i].Use)) { continue; }

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
				if (!sti(NSClients[i].Use)) { continue; }

				NSClients[i].Stat.Won = sti(NSClients[i].Team) == sti(NetServer.iWinnerTeam);
				NSClients[i].Stat.Lost = sti(NSClients[i].Team) != sti(NetServer.iWinnerTeam);
			}
		}
	}

	// disconnect clients
	for (int i=0; i<NET_MAXCLIENTS; i++)
	{
		if (!sti(NSClients[i].Use)) { continue; }
		
		NetServer_UpdateGlobalPlayerStatistics(&NSClients[i]);
		DeleteClass(&NSClients[i]);
	}

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

void NetServer_OnCheckGameOverFrame()
{
	int i;

	// check the game over - only if game started
	if (!sti(NetServer.GameOver))
	{
		NetServer.wWinnerClientID = DST_INVALID;		// for deathmatch
		NetServer.iWinnerTeam = -1;						// for all other modes

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
			// game is over
			NetServer.GameOver = true;
			int iSMsg = NMCreate();
			NMAddByte(iSMsg, NC_STARTGAME);
			NMAddByte(iSMsg, NSC_STARTGAME_GAMEOVER);
			NMAddByte(iSMsg, sti(NetServer.GameType));
			NMAddClientID(iSMsg, sti(NetServer.wWinnerClientID));
			NMAddByte(iSMsg, sti(NetServer.iWinnerTeam));
			NetServer_SendMessage(DST_ALL, iSMsg, true);
			NMDelete(iSMsg);
		}
	}
}

void NetServer_RestartGame()
{
	Net_DeleteServer();

	//Net_CreateServer();
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
						continue;
					}
				string sCoopKey = "l" + sti(NetServer.CoopN);
				NetServer.CoopLoc.(sCoopKey) = iCL;
				NetServer.CoopN = sti(NetServer.CoopN) + 1;
			}
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