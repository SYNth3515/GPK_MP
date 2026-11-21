int iSecondsToStartGame = -1;

// COOP s30: the choice of the lobby of the player wID to wTo (server\ServerInfo.c sends all of them to a new lobby)
void NetServer_SendChoice(int wTo, int wID)
{
	if (!CheckAttribute(&NSClients[wID], "LobbyChoice")) { return; }
	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_CLIENT);
	NMAddByte(iSMsg, NSC_CLIENT_CHOICE);
	NMAddClientID(iSMsg, wID);
	NMAddString(iSMsg, NSClients[wID].LobbyChoice, 250);
	NetServer_SendMessage(wTo, iSMsg, true);
	NMDelete(iSMsg);
}

void NetServer_OnBuy(int wNetClientID, int iMsg)
{
	int iSMsg;

	ref rClient = NetServer_GetCLient(wNetClientID);

	int iSubCode = NMGetByte(iMsg);

	switch (iSubCode)
	{
		case NSC_BUY_CHOICE:		// COOP s30: the choice of the lobby: kept and sent to every lobby
			string sChoice = NMGetString(iMsg);
			rClient.LobbyChoice = sChoice;
			NetServer_SendChoice(DST_ALL, wNetClientID);
			return;
		break;
		case NSC_BUY_SHIPCHANGE:
			// COOP s30: the lobby of a player who reconnects sends the ship of his profile, but his ship is in the game already:
			// a new type here changed the ship of the slot on every machine while the old model stayed (crashes, freezes,
			// a crew of another ship). The ship of the running game never changes this way.
			if (bServerGameStarted)
			{
				bool bShipInGame = NetServer_HasShip(wNetClientID);
				if (NetServer_IsInGame(wNetClientID)) { bShipInGame = true; }
				if (bShipInGame)
				{
					Trace("COOP: ship change of " + wNetClientID + " ignored, the ship is in the game");
					return;
				}
			}
			rClient.Ship.Type = NMGetWord(iMsg);
			iSMsg = NMCreate();
			NMAddByte(iSMsg, NC_PLAYERINFO);
			NMAddByte(iSMsg, NSC_PLAYERINFO_SHIPCHANGE);
			NMAddClientID(iSMsg, wNetClientID);
			NMAddWord(iSMsg, sti(rClient.Ship.Type));
			NetServer_SendMessage(DST_ALL, iSMsg, true);
			NMDelete(iSMsg);
			return;
		break;
		case NSC_BUY_TEAMCHANGE:
			NetServer_OnBuy_TeamChangeRequest(wNetClientID, iMsg);
			return;
		break;
		case NSC_BUY_READY:
			NetServer_OnBuy_Ready(wNetClientID);
			return;
		break;
		case NSC_BUY_FULLINFO:
			NetServer_OnBuy_FullInfo(wNetClientID, iMsg);
			return;
		break;
	}
}	

void NetServer_OnBuy_TeamChangeRequest(int wNetClientID, int iMsg)
{
	ref rClient = NetServer_GetCLient(wNetClientID);
	int iNewTeam = NMGetByte(iMsg);

	// COOP: in the running game a player in the lobby (no ship yet) can still choose his team
	if (!bServerGameStarted && iSecondsToStartGame >= 0) { return; }
	if (bServerGameStarted)
	{
		if (NetServer_HasShip(wNetClientID)) { return; }
		if (NetServer_IsInGame(wNetClientID)) { return; }
	}
	if (iNewTeam < 0 || iNewTeam >= 4) { return; }
	if (NetServer_LandMode() && iNewTeam >= sti(NetServer.NumTeams)) { return; }		// COOP: land duel: only the teams of the game

	rClient.Team = iNewTeam;
	rClient.BuyReady = false;

	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_PLAYERINFO);
	NMAddByte(iSMsg, NSC_PLAYERINFO_TEAMCHANGE);
	NMAddClientID(iSMsg, wNetClientID);
	NMAddByte(iSMsg, sti(rClient.Team));
	NetServer_SendMessage(DST_ALL, iSMsg, true);
	NMDelete(iSMsg);

	iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_PLAYERINFO);
	NMAddByte(iSMsg, NSC_PLAYERINFO_BUYREADY);
	NMAddClientID(iSMsg, wNetClientID);
	NMAddByte(iSMsg, sti(rClient.BuyReady));
	NetServer_SendMessage(DST_ALL, iSMsg, true);
	NMDelete(iSMsg);
}

void NetServer_OnBuy_Ready(int wNetClientID)
{
	int i;

	// countdown already started
	if (iSecondsToStartGame >= 0) { return; }
	if (bServerGameStarted) { return; }		// COOP: the running game is joined with the "Start" button (NetServer_OnCommandStartGame)

	// if client alone on server - dont start countdown
	if ((!NET_DEBUG) && NetServer_GetNumClients() == 1) { return; }

	ref rClient = NetServer_GetCLient(wNetClientID);
	rClient.BuyReady = true;
	
	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_PLAYERINFO);
	NMAddByte(iSMsg, NSC_PLAYERINFO_BUYREADY);
	NMAddClientID(iSMsg, wNetClientID);
	NMAddByte(iSMsg, sti(rClient.BuyReady));
	NetServer_SendMessage(DST_ALL, iSMsg, true);
	NMDelete(iSMsg);

	for (i=0; i<NET_MAXCLIENTS; i++)
	{
		if (!sti(NSClients[i].Use)) { continue; }
		if (!sti(NSClients[i].BuyReady)) { return; }
	}

	// if team game and only one team here - dont start countdown
	if ((!NET_DEBUG) && sti(NetServer.GameType) != NETGAME_DEATHMATCH && NetServer_GetNumTeams() <= 1) { return; }

	if (NET_DEBUG) 
		{ iSecondsToStartGame = 3; }
	else
		{ iSecondsToStartGame = 10; }

	iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_PLAYERINFO);
	NMAddByte(iSMsg, NSC_PLAYERINFO_STARTCOUNTDOWN);
	NMAddDword(iSMsg, iSecondsToStartGame);
	NetServer_SendMessage(DST_ALL, iSMsg, true);
	NMDelete(iSMsg);

	iSecondsToStartGame = iSecondsToStartGame * 1000;

	SetEventHandler("NetServer_StartServerCountDown", "NetServer_StartServerCountDown", 0);
	PostEvent("NetServer_StartServerCountDown", 250);
}

// COOP: the host pressed "Start" in the lobby. Skips the countdown and asks all clients for their buy info right away.
// Same rules as for the countdown: not alone on the server, at least 2 teams in team games.
// Note: like before, the server does not check WHO sent the command (the lobby shows the button only to the host).
void NetServer_OnCommandStartGame(int wNetClientID)
{
	// COOP: the game is running: this is a player from the lobby who wants to join it ("Start" button of the lobby).
	// He gets the buy info request; his answer (NetServer_OnBuy_FullInfo) puts his ship into the game.
	if (bServerGameStarted)
	{
		if (wNetClientID < 0 || wNetClientID >= NET_MAXCLIENTS) { return; }
		if (!sti(NSClients[wNetClientID].Use)) { return; }
		if (NetServer_IsInGame(wNetClientID)) { return; }
		// (a repeated press only repeats the request: the ship is created once, see NetServer_OnBuy_FullInfo)
		NSClients[wNetClientID].JoinRequested = true;

		// reconnect: the old ship is given back at once
		if (CheckAttribute(&NSClients[wNetClientID], "Reconnect") && sti(NSClients[wNetClientID].Reconnect) && NetServer_HasShip(wNetClientID))
		{
			NetServer_LateJoin(wNetClientID, true);
			return;
		}

		// COOP s31: the host went to the lobby (NetServer_ClientToLobby): he comes back even if "Join during the game" is off
		bool bBackToGame = false;
		if (CheckAttribute(&NSClients[wNetClientID], "BackToGame")) { bBackToGame = sti(NSClients[wNetClientID].BackToGame); }
		NSClients[wNetClientID].BackToGame = false;

		// COOP s30: "Join during the game" is off: a new player waits in the lobby for the next game
		if (!NetServer_LateJoinOn() && !bBackToGame)
		{
			NSClients[wNetClientID].JoinRequested = false;
			NetServer_SayTo(wNetClientID, "¬ход во врем€ игры выключен: дождитесь следующей игры");
			return;
		}

		// COOP: a land duel has no ships to buy: the player goes straight into the running duel
		if (NetServer_LandMode())
		{
			NetServer_LateJoin(wNetClientID, false);
			return;
		}

		int iJSMsg = NMCreate();
		NMAddByte(iJSMsg, NC_STARTGAME);
		NMAddByte(iJSMsg, NSC_STARTGAME_GETBUYINFO);
		NetServer_SendMessage(wNetClientID, iJSMsg, true);
		NMDelete(iJSMsg);
		return;
	}

	// already started, or the start was already requested (double click, countdown ended at the same time)
	if (bServerStartRequested) { return; }

	// COOP: the host can start alone (other players join the running game later), in team games with one team too

	bServerStartRequested = true;

	// stop the countdown if it is running (otherwise it would send the start request a second time when it ends)
	DelEventHandler("NetServer_StartServerCountDown", "NetServer_StartServerCountDown");

	// iSecondsToStartGame >= 0 also locks the lobby: no team change, no ready change, no new connections
	iSecondsToStartGame = 0;

	NetServer_StartGame();
}

void NetServer_StartServerCountDown()
{
	iSecondsToStartGame = iSecondsToStartGame - 250;

	// check players/teams numbers (if only 1 player or 1 team, or zero players - stop countdown and return to wait state)
	bool bStopCountdown = false;
	if (!NET_DEBUG)
	{
		if (NetServer_GetNumClients() <= 1) { bStopCountdown = true; }
		if (sti(NetServer.GameType) != NETGAME_DEATHMATCH && NetServer_GetNumTeams() <= 1) { bStopCountdown = true; }
	}
	if (bStopCountdown)
	{
		iSecondsToStartGame = -1;

		int iSMsg = NMCreate();
		NMAddByte(iSMsg, NC_PLAYERINFO);
		NMAddByte(iSMsg, NSC_PLAYERINFO_STOPCOUNTDOWN);
		NetServer_SendMessage(DST_ALL, iSMsg, true);
		NMDelete(iSMsg);

		return;
	}

	if (iSecondsToStartGame <= 0)
	{
		NetServer_StartGame();
		return;
	}

	PostEvent("NetServer_StartServerCountDown", 250);
}

void NetServer_OnBuy_FullInfo(int wNetClientID, int iMsg)
{
	int i;
	int iCredit = 0;

	ref rClient = NetServer_GetCLient(wNetClientID);

	// COOP: a player who already has a ship in the running game can not buy again (a repeated / late packet)
	if (bServerGameStarted)
	{
		if (NetServer_IsInGame(wNetClientID)) { return; }
		if (NetServer_HasShip(wNetClientID)) { return; }
	}

	// receive and calculates all 
	int iShipType = NMGetWord(iMsg);
	int iShipUpgradeHull = NMGetByte(iMsg);
	int iShipUpgradeRigging = NMGetByte(iMsg);
	if (iShipUpgradeRigging < 1 || iShipUpgradeRigging > 4) { iShipUpgradeRigging = 1; }

	int iCannonsType = NMGetByte(iMsg);
	int iCannonsUpgrade = NMGetByte(iMsg);

	int iSkills[8], iPerks[6], iGoods[8];

	// add skills money
	for (i=0; i<Net_GetSkillsNum(); i++) 
	{ 
		iSkills[i] = NMGetByte(iMsg); 
		iCredit = iCredit + Net_GetSkillCost(i, iSkills[i]);
	}

	// add perks money
	for (i=0; i<6; i++) 
	{ 
		iPerks[i] = NMGetByte(iMsg); 
		if (iPerks[i]) { iCredit = iCredit + Net_GetPerkCost(i); }
	}

	// add goods money
	for (i=0; i<Net_GetGoodsNum(); i++) 
	{ 
		iGoods[i] = NMGetWord(iMsg); 
		iCredit = iCredit + Net_GetGoodCost(i, iGoods[i]);
		
		ref rGood = Net_GetGoodByIndex(i);
		string sGood = rGood.Name;
		rClient.Ship.Goods.(sGood) = iGoods[i];
	}

	int iCrewPercent = NMGetByte(iMsg);
	if (iCrewPercent < 10) { iCrewPercent = 10; }
	if (iCrewPercent > 100) { iCrewPercent = 100; }
	iCredit = iCredit + Net_GetCrewCost(iShipType, iCrewPercent);	// hiring the crew is paid

	// start charge chosen in the buy menu (0 balls, 1 grapes, 2 knippels, 3 bombs); if there is none of it in the hold, the first available one
	int iStartCharge = NMGetByte(iMsg);
	if (iStartCharge < 0 || iStartCharge > 3) { iStartCharge = 0; }
	if (iGoods[iStartCharge] <= 0)
	{
		for (i=0; i<4; i++) { if (iGoods[i] > 0) { iStartCharge = i; break; } }
	}

	// COOP: percent of the cannons bought in the lobby (sent as percent + 1; 0 = an old client = all cannons)
	int iCannonsPercent = NMGetByte(iMsg) - 1;
	if (iCannonsPercent < 0 || iCannonsPercent > 100) { iCannonsPercent = 100; }

	// COOP s28: the sea perks bought in the lobby, a bit mask (common\Perks.c: NET_SPERK_*)
	int iSeaPerks = NMGetDword(iMsg);
	if (iSeaPerks < 0 || iSeaPerks >= Net_SeaPerkPow2(NET_SPERK_NUM)) { iSeaPerks = 0; }
	rClient.SeaPerks = iSeaPerks;
	for (i=0; i<NET_SPERK_NUM; i++)
	{
		if (Net_SeaPerkBit(iSeaPerks, i)) { iCredit = iCredit + Net_GetSeaPerkCost(i); }
	}
	Trace("COOP: buy of " + wNetClientID + ": sea perks " + iSeaPerks);

	if (iShipType < 0 || iShipType >= Net_GetShipsNum()) { iShipType = 0; }		// COOP: never a broken ship type
	ref rShip = Net_GetShipByIndex(iShipType);

	float fShipMaxHP = stf(rShip.HP) * (1.0 + stf(NetShipHullUpgrades[iShipUpgradeHull - 1].hp) / 100.0);

	rClient.Ship.Type = iShipType;
	rClient.Ship.HP = fShipMaxHP;
	rClient.Ship.MaxHP = fShipMaxHP;
	rClient.Ship.SP = 100.0;
	rClient.Ship.Upgrades.Hull = iShipUpgradeHull;
	rClient.Ship.Upgrades.Sails = iShipUpgradeRigging;
	rClient.Ship.Cannons.Borts = "";
	rClient.Ship.Cannons.Type = iCannonsType;
	rClient.Ship.Upgrades.Cannons = iCannonsUpgrade;
	rClient.Ship.Cannons.Charge.Type = iStartCharge;
	rClient.Ship.Crew.Quantity = Net_GetCrewCount(iShipType, iCrewPercent);
	DeleteAttribute(rClient, "Ship.ShotAcc");
	NetServer_SetBortsByCount(rClient, Net_GetCannonsCountByPercent(iShipType, iCannonsPercent));		// COOP: cannons on the borts
	rClient.Ship.MinesDropped = 0;
	rClient.Ship.MineCooldown = 0;
	rClient.Ship.BranderActive = 0;
	rClient.Ship.BranderArmUntil = 0;
	DeleteAttribute(rClient, "Ship.GoodsHeld");
	DeleteAttribute(rClient, "Ship.AmmoLast");		// powder tracking starts from the new goods, not from the previous ship
	rClient.Skills.Accuracy = iSkills[0];
	rClient.Skills.Cannons = iSkills[1];
	rClient.Skills.Sailing = iSkills[2];
	rClient.Skills.Defence = iSkills[3];
	rClient.Skills.Repair = iSkills[4];
	rClient.Skills.Grappling = iSkills[5];
	rClient.Skills.Repair.Active = 0;
	rClient.Skills.Repair.Cooldown = iServerTime - 60 * 1000;		// FIX-ME

	rClient.Ship.SpeedRateModifier = 1.0 + (stf(NetShipHullUpgrades[iShipUpgradeHull - 1].speedrate) + stf(NetShipSailUpgrade[iShipUpgradeRigging - 1].speedrate)) / 100.0;
	rClient.Ship.TurnRateModifier = 1.0 + (stf(NetShipHullUpgrades[iShipUpgradeHull - 1].turnrate) + stf(NetShipSailUpgrade[iShipUpgradeRigging - 1].turnrate)) / 100.0;
	rClient.Ship.SailsDamageModifier = 1.0 + stf(NetShipSailUpgrade[iShipUpgradeRigging - 1].dmgtosails) / 100.0;

	NetServer_CannonRecalculateParameters(rClient);
	NetServer_CalculateFireTime(rClient);
	NetServer_CalculateReloadTime(rClient);

	rClient.StartReady = true;

	// COOP: the game is already running: this player joins it now with the ship he has just bought
	if (bServerGameStarted)
	{
		if (!NetServer_IsInGame(wNetClientID)) { NetServer_LateJoin(wNetClientID, false); }
		return;
	}

	// check for all clients ready
	bool bAllClientsReady = true;
	for (i=0; i<NET_MAXCLIENTS; i++) 
	{ 
		if (sti(NSClients[i].Use) && !sti(NSClients[i].StartReady))
		{
			bAllClientsReady = false;
			break;
		}
	}

	// send message to all 
	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_PLAYERINFO);
	NMAddByte(iSMsg, NSC_PLAYERINFO_STARTREADY);
	NMAddClientID(iSMsg, wNetClientID);
	NMAddByte(iSMsg, sti(rClient.StartReady));
	NetServer_SendMessage(DST_ALL, iSMsg, true);
	NMDelete(iSMsg);

	if (bAllClientsReady)
	{
		NetServer_StartServerGame();
		return;
	}
}
