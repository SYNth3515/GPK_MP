// COOP s37: THE REAL BOARDING. Create game: "Быстрый абордаж" off (NetServer.FastBoarding = 0). A player who boards the ship
// of another player (the boarding of the bots stays the fast one) does not get the result of the formula: the two ships stop
// side by side with the sails down and nothing hurts them, the two captains go to a boarding deck (the one of the single
// player by the decks of the two ships: GetShipLocationID) and fight there with their crews, the others go on with the sea
// battle. The crews: the rule of the single player (LAi_StartBoarding): the bigger crew gets as many fighters as the deck and
// the smaller of the two classes allow, the other one the same part of its crew; the captains are the players themselves.
// The fighters are land bots of the captains (net\Land.c: every captain's machine fights with its own crew, the other one sees
// its puppets) of the two teams NET_BOTREL_TEAM0 + 0 (the attacker) / + 1 (the defender).
// The duel ends with the death of a captain (or when he leaves): the winner goes back to the sea with the crew that lived
// (the living fighters + him x the sailors per fighter), the ship of the loser goes down (NETKILL_BY_BOARDING).
// The sea is not unloaded on the machines of the captains: its layer "net_realize" is frozen (like under a window,
// FreezeMainNetLayers), its cameras are off, the deck is loaded over it; the sea goes on behind it and comes back at the end.
// The server (also the machine of a captain) keeps the sea battle going for all.
//   NSC_LAND_BOARD_START (server -> captain): byte team (0 attacker, 1 defender), clientID the other captain, byte the team
//                        that stands at the "aloc" locators (the bigger ship), byte our fighters, byte the enemy fighters;
//                        an NSC_LAND_START of the deck follows in the same message
//   NSC_LAND_BOARD_END   (server -> captain): byte 1 = won

#define NET_BOARD_MAXCREW		44		// BRDLT_MAXCREW of the single player
#define NET_BOARD_END_MS		3500	// the death of the captain is seen before the duel ends

// ================================================================================================
// SERVER
// ================================================================================================
bool NetServer_BoardRealOn()
{
	if (NetServer_LandMode()) { return false; }
	if (sti(NetServer.NoBoarding)) { return false; }
	if (!CheckAttribute(&NetServer, "FastBoarding")) { return false; }
	return sti(NetServer.FastBoarding) == 0;
}

// the boarding duel of a slot (a captain or a fighter), 0 = none
int NetServer_BDOf(int w)
{
	if (w < 0 || w >= NET_MAXCLIENTS) { return 0; }
	if (!CheckAttribute(&NSClients[w], "BoardDuel")) { return 0; }
	return sti(NSClients[w].BoardDuel);
}

// 0 the attacker, 1 the defender, -1 not in a duel
int NetServer_BDTeamOf(int w)
{
	if (NetServer_BDOf(w) <= 0) { return -1; }
	if (!CheckAttribute(&NSClients[w], "BoardTeam")) { return -1; }
	return sti(NSClients[w].BoardTeam);
}

int NetServer_BDShipClass(int w)
{
	ref rC = NetServer_GetClient(w);
	if (!CheckAttribute(rC, "Ship.Type")) { return 5; }
	ref rShip = Net_GetShipByIndex(sti(rC.Ship.Type));
	if (!CheckAttribute(rShip, "Class")) { return 5; }
	return sti(rShip.Class);
}

// the DeckType of the ships of the single player by the class
string NetServer_BDDeckType(int iClass)
{
	if (iClass <= 2) { return "Big"; }
	if (iClass <= 4) { return "Medium"; }
	return "Low";
}

// GetShipLocationID of the single player
string NetServer_BDDeck(int wA, int wT)
{
	string sA = NetServer_BDDeckType(NetServer_BDShipClass(wA));
	string sT = NetServer_BDDeckType(NetServer_BDShipClass(wT));
	if (sA == "Big" || sT == "Big")
	{
		if (sA == "Low" || sT == "Low") { return "BOARDING_MEDIUM_DECK"; }
		return "BOARDING_BIG_DECK";
	}
	return "BOARDING_SMALL_DECK";
}

// the fighters of the bigger crew at most (LAi_StartBoarding: the locators of the deck and the smaller class of the two)
int NetServer_BDMaxCrew(string sDeck, int cA, int cT)
{
	int iMax = 11;
	int iLoc = FindLocation(sDeck);
	if (iLoc >= 0) { if (CheckAttribute(&Locations[iLoc], "boarding.locatorNum")) { iMax = sti(Locations[iLoc].boarding.locatorNum); } }
	int iC = cA;
	if (cT < iC) { iC = cT; }
	int iByClass = 13;
	switch (iC)
	{
		case 7: iByClass = 4; break;
		case 6: iByClass = 5; break;
		case 5: iByClass = 6; break;
		case 4: iByClass = 7; break;
		case 3: iByClass = 8; break;
		case 2: iByClass = 11; break;
		case 1: iByClass = 13; break;
	}
	if (iByClass < iMax) { iMax = iByClass; }
	if (iMax > NET_BOARD_MAXCREW) { iMax = NET_BOARD_MAXCREW; }
	return iMax;
}

// the crew of a captain: land bots of his (the free bot slots: no sea bot, no other land bot)
int NetServer_BDMakeCrew(int id, int wCap, int iTeam, int iN)
{
	int iLevel = 1;
	if (CheckAttribute(&NetServer, "BotSkill")) { iLevel = 1 + 3 * sti(NetServer.BotSkill); }
	int iMade = 0;
	for (int k=0; k<iN; k++)
	{
		int iFree = -1;
		for (int i=NET_BOT_FIRST; i<NET_MAXCLIENTS; i++)
		{
			if (sti(NSClients[i].Use)) { continue; }
			if (CheckAttribute(&NSClients[i], "Bot")) { if (sti(NSClients[i].Bot)) { continue; } }
			if (NetServer_SlotBurned(i)) { continue; }		// a sea ship was there: its entity lives on the machines
			if (NetServer_IsLandBot(i)) { continue; }
			if (NetServer_BDOf(i) > 0) { continue; }
			iFree = i;
			break;
		}
		if (iFree < 0) { Trace("COOP: boarding: no free slot for the crew"); break; }
		DeleteAttribute(&NSClients[iFree], "LandBotDeadSeen");
		DeleteAttribute(&NSClients[iFree], "LandBotDeathN");
		NSClients[iFree].LandBot = 1;
		NSClients[iFree].LandBotRemoved = 0;
		NSClients[iFree].LandBotOwner = wCap;
		NSClients[iFree].LandBotRel = NET_BOTREL_TEAM0 + iTeam;
		NSClients[iFree].LandBotFace = 0;		// no portrait: a pirate (NetLand_BotModel)
		NSClients[iFree].LandBotBlade = "";
		NSClients[iFree].LandBotLevel = iLevel;
		NSClients[iFree].LandBotGun = "";
		NSClients[iFree].LandBotNick = "Матрос";
		NSClients[iFree].BoardDuel = id;
		NSClients[iFree].BoardTeam = iTeam;
		NetServer_LandBotSendInfo(iFree);
		iMade++;
	}
	return iMade;
}

void NetServer_BDCaptainInit(int w, int id, int iTeam)
{
	NSClients[w].BoardDuel = id;
	NSClients[w].BoardTeam = iTeam;
	NSClients[w].LandDead = false;
	if (!CheckAttribute(&NSClients[w], "LandKills")) { NSClients[w].LandKills = 0; }
	if (!CheckAttribute(&NSClients[w], "LandDeaths")) { NSClients[w].LandDeaths = 0; }
	if (!CheckAttribute(&NSClients[w], "LandDmg")) { NSClients[w].LandDmg = 0.0; }
	ref rC = NetServer_GetClient(w);
	// the ship stops where it is: no sails, no rudder (NetServer_OnShip does not take the orders of the captain now)
	rC.Ship.SailState = 0.0;
	rC.Ship.TurnLeft = 0;
	rC.Ship.TurnRight = 0;
	rC.Ship.TurnState = 0.0;
	rC.BoardSent = -1;
}

void NetServer_BDSendStart(int id, int w, int iTeam, int wEnemy, int iMy, int iEnemy)
{
	if (!sti(NSClients[w].Use)) { return; }
	string sD = "d" + id;
	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_LAND);
	NMAddByte(iSMsg, NSC_LAND_BOARD_START);
	NMAddByte(iSMsg, iTeam);
	NMAddClientID(iSMsg, wEnemy);
	NMAddByte(iSMsg, sti(NetServer.BD.(sD).up));
	NMAddByte(iSMsg, iMy);
	NMAddByte(iSMsg, iEnemy);
	// the deck (NetLand_OnStart): the spawn of the team, no respawn
	NMAddByte(iSMsg, NC_LAND);
	NMAddByte(iSMsg, NSC_LAND_START);
	NMAddString(iSMsg, NetServer.BD.(sD).deck, 32);
	NMAddByte(iSMsg, iTeam);
	NMAddByte(iSMsg, 0);
	NMAddByte(iSMsg, 0);
	int iLandSw = 0;
	if (CheckAttribute(&NetServer, "PistolReload") && sti(NetServer.PistolReload)) { iLandSw = 2; }
	NMAddByte(iSMsg, iLandSw);
	NMAddString(iSMsg, NetServer.WeatherID, 64);
	NMAddDword(iSMsg, 0);
	NetServer_SendMessage(w, iSMsg, true);
	NMDelete(iSMsg);
}

// NetServer_TryBoardPlayer: the two players board (the checks of the distance, of the brander and the musket volley are done)
void NetServer_BDStart(int wA, int wT)
{
	ref rA = NetServer_GetClient(wA);
	ref rT = NetServer_GetClient(wT);
	if (!CheckAttribute(&NetServer, "BDNext")) { NetServer.BDNext = 0; }
	int id = sti(NetServer.BDNext) + 1;
	if (id > 250) { id = 1; }
	NetServer.BDNext = id;
	string sD = "d" + id;
	DeleteAttribute(&NetServer, "BD." + sD);

	int cA = NetServer_BDShipClass(wA);
	int cT = NetServer_BDShipClass(wT);
	string sDeck = NetServer_BDDeck(wA, wT);
	// the side of the deck: the bigger ship stands at "aloc" (ChooseShipUpDeck of the single player: the bigger hold)
	int iUp = 0;
	if (cT < cA) { iUp = 1; }

	float fA = stf(rA.Ship.Crew.Quantity);
	float fT = stf(rT.Ship.Crew.Quantity);
	if (fA < 1.0) { fA = 1.0; }
	if (fT < 1.0) { fT = 1.0; }
	int iMax = NetServer_BDMaxCrew(sDeck, cA, cT);
	int nA = makeint(fA);
	int nT = makeint(fT);
	float fRel;
	if (fA >= fT)
	{
		if (nA > iMax) { fRel = fA / makefloat(iMax); nA = iMax; nT = makeint(fT / fRel + 0.5); }
	}
	else
	{
		if (nT > iMax) { fRel = fT / makefloat(iMax); nT = iMax; nA = makeint(fA / fRel + 0.5); }
	}
	if (nA < 1) { nA = 1; }
	if (nT < 1) { nT = 1; }

	NetServer.BD.(sD).a = wA;
	NetServer.BD.(sD).t = wT;
	NetServer.BD.(sD).deck = sDeck;
	NetServer.BD.(sD).up = iUp;
	NetServer.BD.(sD).baseA = fA;
	NetServer.BD.(sD).baseT = fT;
	NetServer.BD.(sD).perA = fA / makefloat(nA);
	NetServer.BD.(sD).perT = fT / makefloat(nT);
	NetServer.BD.(sD).ending = 0;
	NetServer.BD.(sD).round = sti(NetServer.Round);
	if (!CheckAttribute(&NetServer, "LandTeamKills0")) { NetServer.LandTeamKills0 = 0; }
	if (!CheckAttribute(&NetServer, "LandTeamKills1")) { NetServer.LandTeamKills1 = 0; }

	NetServer_BDCaptainInit(wA, id, 0);
	NetServer_BDCaptainInit(wT, id, 1);
	SetEventHandler("NetServer_BDEndEvent", "NetServer_BDEndEvent", 0);

	// the captain is one of the fighters: the crew on the deck is one less
	iNetSrvBDScope = id;
	int iCrewA = NetServer_BDMakeCrew(id, wA, 0, nA - 1);
	int iCrewT = NetServer_BDMakeCrew(id, wT, 1, nT - 1);
	iNetSrvBDScope = 0;

	NetServer_BDSendStart(id, wA, 0, wT, iCrewA + 1, iCrewT + 1);
	NetServer_BDSendStart(id, wT, 1, wA, iCrewT + 1, iCrewA + 1);
	// the loadouts of the captains that the server knows already (the new ones come with the load of the deck)
	if (CheckAttribute(&NSClients[wA], "LandLo.Blade") && sti(NSClients[wT].Use))
	{
		int iLoA = NetServer_LandLoadoutMsg(wA);
		NetServer_SendMessage(wT, iLoA, true);
		NMDelete(iLoA);
	}
	if (CheckAttribute(&NSClients[wT], "LandLo.Blade") && sti(NSClients[wA].Use))
	{
		int iLoT = NetServer_LandLoadoutMsg(wT);
		NetServer_SendMessage(wA, iLoT, true);
		NMDelete(iLoT);
	}
	NetServer_BotSendOpts(wA);
	NetServer_BotSendOpts(wT);
	Trace("COOP: boarding duel " + id + ": " + wA + " (class " + cA + ", crew " + fA + ", " + nA + " fighters) boards " + wT + " (class " + cT + ", crew " + fT + ", " + nT + " fighters) on " + sDeck + ", the bigger ship " + iUp);
}

// the other captain of the duel
int NetServer_BDOther(int id, int w)
{
	string sD = "d" + id;
	if (!CheckAttribute(&NetServer, "BD." + sD + ".a")) { return DST_INVALID; }
	if (sti(NetServer.BD.(sD).a) == w) { return sti(NetServer.BD.(sD).t); }
	return sti(NetServer.BD.(sD).a);
}

bool NetServer_BDIsCaptain(int w)
{
	int id = NetServer_BDOf(w);
	if (id <= 0) { return false; }
	string sD = "d" + id;
	if (!CheckAttribute(&NetServer, "BD." + sD + ".a")) { return false; }
	if (sti(NetServer.BD.(sD).a) == w) { return true; }
	if (sti(NetServer.BD.(sD).t) == w) { return true; }
	return false;
}

// NetServer_LandDead: a captain has fallen - the duel ends a little later (his death is seen)
void NetServer_BDCaptainDead(int wDead)
{
	int id = NetServer_BDOf(wDead);
	if (id <= 0) { return; }
	if (!NetServer_BDIsCaptain(wDead)) { return; }
	string sD = "d" + id;
	if (sti(NetServer.BD.(sD).ending)) { return; }
	NetServer.BD.(sD).ending = 1;
	int wWin = NetServer_BDOther(id, wDead);
	Trace("COOP: boarding duel " + id + ": the captain " + wDead + " has fallen, " + wWin + " wins");
	PostEvent("NetServer_BDEndEvent", NET_BOARD_END_MS, "llll", id, wWin, wDead, sti(NetServer.Round));
}

void NetServer_BDEndEvent()
{
	int id = GetEventData();
	int wWin = GetEventData();
	int wLose = GetEventData();
	int iRound = GetEventData();
	if (iRound != sti(NetServer.Round)) { return; }
	NetServer_BDEnd(id, wWin, wLose);
}

// the living fighters of a team of the duel
int NetServer_BDAlive(int id, int iTeam)
{
	int iN = 0;
	for (int i=NET_BOT_FIRST; i<NET_MAXCLIENTS; i++)
	{
		if (NetServer_BDOf(i) != id) { continue; }
		if (NetServer_BDTeamOf(i) != iTeam) { continue; }
		if (CheckAttribute(&NSClients[i], "LandBotDeadSeen")) { if (sti(NSClients[i].LandBotDeadSeen)) { continue; } }
		iN++;
	}
	return iN;
}

void NetServer_BDSendEnd(int w, int iWon)
{
	if (w < 0 || w >= NET_MAXPLAYERS) { return; }
	if (!sti(NSClients[w].Use)) { return; }
	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_LAND);
	NMAddByte(iSMsg, NSC_LAND_BOARD_END);
	NMAddByte(iSMsg, iWon);
	NetServer_SendMessage(w, iSMsg, true);
	NMDelete(iSMsg);
}

void NetServer_BDEnd(int id, int wWin, int wLose)
{
	string sD = "d" + id;
	if (!CheckAttribute(&NetServer, "BD." + sD + ".a")) { return; }
	int wA = sti(NetServer.BD.(sD).a);
	int iWinTeam = 0;
	if (wWin != wA) { iWinTeam = 1; }
	float fBaseW = stf(NetServer.BD.(sD).baseA);
	float fPerW = stf(NetServer.BD.(sD).perA);
	float fBaseL = stf(NetServer.BD.(sD).baseT);
	if (iWinTeam == 1)
	{
		fBaseW = stf(NetServer.BD.(sD).baseT);
		fPerW = stf(NetServer.BD.(sD).perT);
		fBaseL = stf(NetServer.BD.(sD).baseA);
	}
	int iAlive = NetServer_BDAlive(id, iWinTeam);
	// the sailors of the winner: the living fighters and the captain, each one for his part of the crew
	int iNewCrew = makeint(makefloat(iAlive + 1) * fPerW + 0.5);
	if (iNewCrew > makeint(fBaseW)) { iNewCrew = makeint(fBaseW); }
	if (iNewCrew < 1) { iNewCrew = 1; }
	ref rWin = NetServer_GetClient(wWin);
	ref rLose = NetServer_GetClient(wLose);
	rWin.Ship.Crew.Quantity = iNewCrew;
	int iWinLost = makeint(fBaseW) - iNewCrew;
	int iLoseLost = makeint(fBaseL);
	Trace("COOP: boarding duel " + id + " is over: " + wWin + " won (" + iAlive + " fighters live, crew " + iNewCrew + "), " + wLose + " lost");

	// the crews leave the slots of the bots (the machines of the captains drop their bots with the deck)
	for (int i=NET_BOT_FIRST; i<NET_MAXCLIENTS; i++)
	{
		if (NetServer_BDOf(i) != id) { continue; }
		DeleteAttribute(&NSClients[i], "LandBot");
		DeleteAttribute(&NSClients[i], "LandBotRemoved");
		DeleteAttribute(&NSClients[i], "LandBotDeadSeen");
		DeleteAttribute(&NSClients[i], "LandBotDeathN");
		DeleteAttribute(&NSClients[i], "BoardDuel");
		DeleteAttribute(&NSClients[i], "BoardTeam");
	}
	// the captains back to the sea, the ships obey them again
	NetServer_BDSendEnd(wWin, 1);
	NetServer_BDSendEnd(wLose, 0);
	DeleteAttribute(&NSClients[wWin], "BoardDuel");
	DeleteAttribute(&NSClients[wWin], "BoardTeam");
	DeleteAttribute(&NSClients[wLose], "BoardDuel");
	DeleteAttribute(&NSClients[wLose], "BoardTeam");
	NSClients[wWin].LandDead = false;
	NSClients[wLose].LandDead = false;
	DeleteAttribute(&NetServer, "BD." + sD);

	NetServer_SendBoardResult(wWin, iWinLost, iLoseLost);
	NetServer_SendBoardResult(wLose, iLoseLost, iWinLost);
	// the ship of the loser goes down with its whole crew (like the fast boarding)
	rLose.Ship.Crew.Quantity = 0;
	if (!NetServer_IsClientDead(rLose)) { NetServer_SetClientDead(rLose, NETKILL_BY_BOARDING, wWin); }
}

// a captain has left the game: the other one wins
void NetServer_BDLeft(int w)
{
	if (!NetServer_BDIsCaptain(w)) { return; }
	int id = NetServer_BDOf(w);
	int wOther = NetServer_BDOther(id, w);
	Trace("COOP: boarding duel " + id + ": the captain " + w + " has left");
	NetServer_BDEnd(id, wOther, w);
}

// a new game: no duel
void NetServer_BDResetAll()
{
	for (int i=0; i<NET_MAXCLIENTS; i++)
	{
		if (NetServer_BDOf(i) > 0 && i >= NET_BOT_FIRST)
		{
			DeleteAttribute(&NSClients[i], "LandBot");
			DeleteAttribute(&NSClients[i], "LandBotRemoved");
			DeleteAttribute(&NSClients[i], "LandBotDeadSeen");
		}
		DeleteAttribute(&NSClients[i], "BoardDuel");
		DeleteAttribute(&NSClients[i], "BoardTeam");
	}
	DeleteAttribute(&NetServer, "BD");
	iNetSrvBDScope = 0;
}

// ================================================================================================
// CLIENT
// ================================================================================================
// the boarding button of the lobby (the loadout of the deck)
bool NetBoard_RealOnClient()
{
	if (!CheckAttribute(&NCServer, "FastBoarding")) { return false; }
	if (sti(NCServer.FastBoarding)) { return false; }
	if (CheckAttribute(&NCServer, "NoBoarding")) { if (sti(NCServer.NoBoarding)) { return false; } }
	return true;
}

// the sea stays, it is not drawn; its cameras and its particles give place to the deck
void NetBoard_SeaHide()
{
	NetClient_CamerasSetActive(false);
	LayerFreeze("net_realize", true);
	MoveParticlesToLayers("execute", "realize");
	Trace("COOP: boarding: the sea is hidden");
}

void NetBoard_SeaShow()
{
	LayerFreeze("net_realize", false);
	MoveParticlesToLayers("net_execute", "net_realize");
	NetClient_CamerasSetActive(true);
	ResetSoundScheme();
	SetMusic("music_sea_battle");
	Trace("COOP: boarding: the sea is back");
}

void NetBoard_OnStart(int iMsg)
{
	int iTeam = NMGetByte(iMsg);
	int wEnemy = NMGetClientID(iMsg);
	int iUp = NMGetByte(iMsg);
	int iMy = NMGetByte(iMsg);
	int iEnemy = NMGetByte(iMsg);
	if (bNetLandActive || bNetBoardDuel) { Trace("COOP: boarding: a start in a duel, no"); return; }
	bNetBoardDuel = true;
	iNetBoardTeam = iTeam;
	wNetBoardEnemy = wEnemy;
	iNetBoardUpTeam = iUp;
	NetBoard_SeaHide();
	string sEnemy = "";
	if (wEnemy >= 0 && wEnemy < NET_MAXCLIENTS) { if (CheckAttribute(&NCClients[wEnemy], "NickName")) { sEnemy = NCClients[wEnemy].NickName; } }
	Log_SetStringToLog("Абордаж! Капитан " + sEnemy + ". Наших бойцов: " + iMy + ", у врага: " + iEnemy);
	PlayStereoSound(NET_SOUND_ABORDAGE);
	Trace("COOP: boarding: the duel starts, team " + iTeam + ", the enemy " + wEnemy + ", the bigger ship " + iUp + ", fighters " + iMy + " / " + iEnemy);
}

void NetBoard_OnEnd(int iMsg)
{
	int iWon = NMGetByte(iMsg);
	if (!bNetBoardDuel) { return; }
	NetLand_ClientStop();
	bNetBoardDuel = false;
	NetBoard_SeaShow();
	if (iWon) { Log_SetStringToLog("Абордаж выигран!"); }
	else { Log_SetStringToLog("Абордаж проигран: корабль идёт ко дну"); }
	Trace("COOP: boarding: the duel is over, won " + iWon);
}

// client\OnStart.c: NetClient_OnEndGame (the land duel has been stopped already)
void NetBoard_ClientReset()
{
	if (!bNetBoardDuel) { return; }
	bNetBoardDuel = false;
	LayerFreeze("net_realize", false);
	MoveParticlesToLayers("net_execute", "net_realize");
}

// NetLand_TeleportToSpawn on the deck: the team at its side ("loc" / "aloc" of the single player), the n-th one of the team
bool NetBoard_TeleportToSide(ref chr, int iTeam, int iSpawn)
{
	if (!IsEntity(loadedLocation)) { return false; }
	if (!CheckAttribute(loadedLocation, "locators.rld")) { return false; }
	string sPre = "loc";
	if (iTeam == iNetBoardUpTeam) { sPre = "aloc"; }
	int iPreLen = strlen(sPre);
	aref arGroup;
	makearef(arGroup, loadedLocation.locators.rld);
	int iNum = GetAttributesNum(arGroup);
	int iCount = 0;
	int i;
	for (i=0; i<iNum; i++)
	{
		aref arC = GetAttributeN(arGroup, i);
		if (NetBoard_SideLocator(GetAttributeName(arC), sPre, iPreLen)) { iCount++; }
	}
	if (iCount <= 0) { return false; }
	int iWant = iSpawn - (iSpawn / iCount) * iCount;
	int iSeen = 0;
	for (i=0; i<iNum; i++)
	{
		aref arL = GetAttributeN(arGroup, i);
		if (!NetBoard_SideLocator(GetAttributeName(arL), sPre, iPreLen)) { continue; }
		if (iSeen == iWant) { return TeleportCharacterToPosAy(chr, stf(arL.x), stf(arL.y), stf(arL.z), 0.0); }
		iSeen++;
	}
	return false;
}

// "loc3" for "loc", "aloc3" for "aloc" (not "locmush1", not "aloc..." for "loc")
bool NetBoard_SideLocator(string sName, string sPre, int iPreLen)
{
	if (strlen(sName) <= iPreLen) { return false; }
	if (strcut(sName, 0, iPreLen - 1) != sPre) { return false; }
	string sC = strcut(sName, iPreLen, iPreLen);
	if (findSubStr("0123456789", sC, 0) < 0) { return false; }
	return true;
}
