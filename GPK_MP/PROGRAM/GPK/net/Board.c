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

// COOP s41: the difficulty of the bots ("Боты" of Create game / the admin menu) for the fighters of the deck: 0 harmless .. 3 hard
int NetServer_BDSkill()
{
	int iS = 2;
	if (CheckAttribute(&NetServer, "BotSkill")) { iS = sti(NetServer.BotSkill); }
	if (iS < 0) { iS = 0; }
	if (iS > 3) { iS = 3; }
	return iS;
}

// COOP s44: the fighters of the deck like the single player (LAi_boarding.c, GeneratorUtilite.c, LAi_equip.c): the level of a
// fighter is the level of the player captain +- 8 (CalculateAppropriateSkills) and the bonus of the crew that outnumbers the
// other (GetBoarding_player_hp_Bonus / GetBoarding_enemy_hp_Bonus), the blade and the pistol by his level (LAi_NPC_Equip),
// two musketeers for a side with the perk MusketsShoot (LAi_SetBoardingActors), the perks by the level (the client:
// NetLand_BoardBotPerks). The difficulty of the bots gives only the AI (NetLand_AiParam: the harmless ones stand) and the
// level and the cuirass of the captain of a bot; the rest follows the medium difficulty of the single player
#define NET_BOARD_RATE			5		// MOD_SKILL_ENEMY_RATE of the crews (the medium one)
#define NET_BOARD_MUSH			2		// the musketeers of a ship with the perk MusketsShoot (3 in a fort)
#define NET_BOARD_MUSH_FACE		1000	// LandBotFace 1001..1005: "Mushketer_1..5", the musketeers of the pirates (NetLand_BotModel)

// MOD_SKILL_ENEMY_RATE of the difficulty of the bots for the captain of a bot: harmless 1, easy 3, medium 5, hard 8
int NetServer_BDCapRate()
{
	int iS = NetServer_BDSkill();
	if (iS <= 0) { return 1; }
	if (iS == 1) { return 3; }
	if (iS == 2) { return 5; }
	return 8;
}

// the level of a player captain (the rank of the hero of the single player): his loadout of the deck, or the level of the
// choice of his lobby (Buy_ChoiceCode: "...|blade,gun,armor,level") before the first deck
int NetServer_BDCapLevel(int w)
{
	if (w < 0 || w >= NET_MAXPLAYERS) { return 1; }
	int iL = 1;
	if (CheckAttribute(&NSClients[w], "LandLo.Level")) { iL = sti(NSClients[w].LandLo.Level); }
	else
	{
		ref rC = NetServer_GetClient(w);
		if (CheckAttribute(rC, "LobbyChoice"))
		{
			string sCh = rC.LobbyChoice;
			int iLen = strlen(sCh);
			int iComma = -1;
			int iAt = findSubStr(sCh, ",", 0);
			while (iAt >= 0)
			{
				iComma = iAt;
				iAt = findSubStr(sCh, ",", iAt + 1);
			}
			if (iComma >= 0 && iComma < iLen - 1) { iL = sti(strcut(sCh, iComma + 1, iLen - 1)); }
		}
	}
	if (iL < 1) { iL = 1; }
	if (iL > 50) { iL = 50; }
	return iL;
}

// the bonus to the level of the crew that has more men than the other: 0.5 * rate (a player) or 2 * rate (a bot) for every
// time the other crew it has more
int NetServer_BDCrewBonus(float fMine, float fEnemy, bool bBotSide)
{
	if (fEnemy < 1.0) { fEnemy = 1.0; }
	if (fMine <= fEnemy) { return 0; }
	float fK = 0.5 * makefloat(NET_BOARD_RATE);
	if (bBotSide) { fK = 2.0 * makefloat(NET_BOARD_RATE); }
	return makeint(fK * (fMine - fEnemy) / fEnemy);
}

// the level of a fighter: the base +- 8 (four frandSmall(4) - 8) and the bonus, 1..50 (the levels of the loadout)
int NetServer_BDRank(int iBase, int iBonus)
{
	float fR = makefloat(iBase) + frandSmall(4.0) + frandSmall(4.0) + frandSmall(4.0) + frandSmall(4.0) - 8.0 + makefloat(iBonus);
	int iR = makeint(fR);
	if (iR < 1) { iR = 1; }
	if (iR > 50) { iR = 50; }
	return iR;
}

// an item that the game knows, "" otherwise
string NetServer_BDItem(string sId)
{
	if (sId == "") { return ""; }
	if (GetItemIndex(sId) < 0) { return ""; }
	return sId;
}

// the blade of the level (LAi_NPC_EquipBladeSelection, without the generated items: their ids are not on the other machines)
string NetServer_BDBladeByRank(int iRank)
{
	int iMin = iRank - 8;
	if (iMin < 0) { iMin = 0; }
	int iMax = iRank + 4;
	if (iMax > 25) { iMin = 12; iMax = 24; }
	if (iMax > 29) { iMin = 20; iMax = 31; }		// the single player: never true after the line above (no "excellent" blades)
	int iSel = rand(iMax - iMin) + iMin;
	string sB = "blade7";
	switch (iSel)
	{
		// "poor"
		case 0: sB = "blade1"; break;
		case 1: sB = "blade2"; break;
		case 2: sB = "blade3"; break;
		case 3: sB = "blade4"; break;
		case 4: sB = "blade5"; break;
		case 5: sB = "blade10"; break;
		case 6: sB = "blade35"; break;
		case 7: sB = "topor1"; break;
		case 8: sB = "topor3"; break;
		case 9: sB = "blade17"; break;
		// "ordinary"
		case 10: sB = "blade6"; break;
		case 11: sB = "blade7"; break;
		case 12: sB = "blade8"; break;
		case 13: sB = "blade9"; break;
		case 14: sB = "blade11"; break;
		case 15: sB = "blade12"; break;
		case 16: sB = "blade16"; break;
		case 17: sB = "blade18"; break;
		// "good"
		case 18: sB = "blade15"; break;
		case 19: sB = "blade19"; break;
		case 20: sB = "blade21"; break;
		case 21: sB = "blade22"; break;
		case 22: sB = "blade31"; break;
		case 23: sB = "blade34"; break;
		case 24: sB = "topor2"; break;
		// "excellent"
		case 25: sB = "blade13"; break;
		case 26: sB = "blade20"; break;
		case 27: sB = "blade23"; break;
		case 28: sB = "blade24"; break;
		case 29: sB = "blade25"; break;
		case 30: sB = "blade30"; break;
		case 31: sB = "blade33"; break;
	}
	string sOk = NetServer_BDItem(sB);
	if (sOk == "") { sOk = NetServer_BDItem("blade1"); }
	return sOk;
}

// the pistol of the level (LAi_NPC_Equip): the chance rate * level * 8 of 1000, the better pistols for the higher levels
string NetServer_BDGunByRank(int iRank)
{
	if (rand(1000) >= NET_BOARD_RATE * iRank * 8) { return ""; }
	int iRnd = rand(1);
	if (iRank > 10) { iRnd = rand(2); }
	if (iRank > 15) { iRnd = rand(2) + 1; }
	if (iRank > 20) { iRnd = rand(3) + 2; }
	string sG = "";
	int iChance = 0;
	switch (iRnd)
	{
		case 0: sG = "pistol1"; iChance = 70; break;
		case 1: sG = "pistol2"; iChance = 75; break;
		case 2: sG = "pistol3"; iChance = 80; break;
		case 3: sG = "pistol6"; iChance = 85; break;
		case 4: sG = "pistol4"; iChance = 90; break;
		case 5: sG = "pistol5"; iChance = 95; break;
	}
	if (rand(100) >= iChance) { return ""; }
	return NetServer_BDItem(sG);
}

// the cuirass of the captain of a bot (LAi_SetBoardingActors: (rate * 2 + rank) / 10, the 3rd and the 4th swapped)
string NetServer_BDCapArmor(int iRank)
{
	int iX = makeint(makefloat(NetServer_BDCapRate() * 2 + iRank) / 10.0);
	if (iX <= 0) { return ""; }
	if (iX >= 5) { iX = 5; }
	if (iX == 3) { iX = 4; }
	else
	{
		if (iX == 4) { iX = 3; }
	}
	return NetServer_BDItem("cirass" + iX);
}

// a free slot for a fighter of the deck (no sea bot, no other land bot), -1 = none
int NetServer_BDFreeSlot()
{
	for (int i=NET_BOT_FIRST; i<NET_MAXCLIENTS; i++)
	{
		if (sti(NSClients[i].Use)) { continue; }
		if (CheckAttribute(&NSClients[i], "Bot")) { if (sti(NSClients[i].Bot)) { continue; } }
		if (NetServer_SlotBurned(i)) { continue; }		// a sea ship was there: its entity lives on the machines
		if (NetServer_IsLandBot(i)) { continue; }
		if (NetServer_BDOf(i) > 0) { continue; }
		return i;
	}
	return -1;
}

// a fighter of the deck: a land bot of wOwner (his machine runs it, the game's AI), -1 = no free slot
int NetServer_BDMakeFighter(int id, int wOwner, int iTeam, string sNick, int iFace, int iLevel, string sBlade, string sGun)
{
	int iFree = NetServer_BDFreeSlot();
	if (iFree < 0) { return -1; }
	DeleteAttribute(&NSClients[iFree], "LandBotDeadSeen");
	DeleteAttribute(&NSClients[iFree], "LandBotDeathN");
	DeleteAttribute(&NSClients[iFree], "BoardCapOf");
	DeleteAttribute(&NSClients[iFree], "BoardMush");		// COOP s44
	NSClients[iFree].LandBotArmor = "";
	NSClients[iFree].LandBot = 1;
	NSClients[iFree].LandBotRemoved = 0;
	NSClients[iFree].LandBotOwner = wOwner;
	NSClients[iFree].LandBotRel = NET_BOTREL_TEAM0 + iTeam;
	NSClients[iFree].LandBotFace = iFace;		// 0: no portrait, a pirate (NetLand_BotModel)
	NSClients[iFree].LandBotBlade = sBlade;
	NSClients[iFree].LandBotLevel = iLevel;
	NSClients[iFree].LandBotGun = sGun;
	NSClients[iFree].LandBotNick = sNick;
	NSClients[iFree].BoardDuel = id;
	NSClients[iFree].BoardTeam = iTeam;
	return iFree;
}

// the base level and the bonus of a team of the duel (NetServer_BDStart)
int NetServer_BDBaseOf(int id, int iTeam)
{
	string sD = "d" + id;
	string sK = "base" + iTeam;
	if (!CheckAttribute(&NetServer, "BD." + sD + "." + sK)) { return 1; }
	return sti(NetServer.BD.(sD).(sK));
}

int NetServer_BDBonusOf(int id, int iTeam)
{
	string sD = "d" + id;
	string sK = "bonus" + iTeam;
	if (!CheckAttribute(&NetServer, "BD." + sD + "." + sK)) { return 0; }
	return sti(NetServer.BD.(sD).(sK));
}

// the crew of a captain: land bots of wOwner (the captain himself; for a bot ship: the player who fights it)
// COOP s44: the level, the blade and the pistol of each one like the single player (NetServer_BDRank)
int NetServer_BDMakeCrew(int id, int wOwner, int iTeam, int iN)
{
	int iBase = NetServer_BDBaseOf(id, iTeam);
	int iBonus = NetServer_BDBonusOf(id, iTeam);
	int iMade = 0;
	for (int k=0; k<iN; k++)
	{
		int iLvl = NetServer_BDRank(iBase, iBonus);
		string sBl = NetServer_BDBladeByRank(iLvl);
		string sGn = NetServer_BDGunByRank(iLvl);
		int iFtr = NetServer_BDMakeFighter(id, wOwner, iTeam, "Матрос", 0, iLvl, sBl, sGn);
		if (iFtr < 0) { Trace("COOP: boarding: no free slot for the crew"); break; }
		NetServer_LandBotSendInfo(iFtr);
		iMade++;
	}
	return iMade;
}

// COOP s44: the musketeers of a side (LAi_SetBoardingActors): a ship with the perk MusketsShoot brings two of them, they
// stand at the "mush" locators of its side (NetBoard_TeleportMush) and shoot from there. They are not a part of the crew
// that comes back to the sea (NetServer_BDAlive), but a bot side is down only when they are dead too
int NetServer_BDMakeMush(int id, int wOwner, int iTeam, int wShip)
{
	ref rShip = NetServer_GetClient(wShip);
	if (!NetServer_HasPerk(rShip, "MusketsShoot")) { return 0; }
	string sMush = NetServer_BDItem("mushket");
	if (sMush == "") { return 0; }
	int iBase = NetServer_BDBaseOf(id, iTeam);
	int iBonus = NetServer_BDBonusOf(id, iTeam);
	int iMade = 0;
	for (int k=0; k<NET_BOARD_MUSH; k++)
	{
		int iLvl = NetServer_BDRank(iBase, iBonus);
		int iFace = NET_BOARD_MUSH_FACE + 1 + rand(4);
		int iFtr = NetServer_BDMakeFighter(id, wOwner, iTeam, "Мушкетёр", iFace, iLvl, NetServer_BDItem("blade1"), sMush);
		if (iFtr < 0) { Trace("COOP: boarding: no free slot for a musketeer"); break; }
		NSClients[iFtr].BoardMush = 1;
		NetServer_LandBotSendInfo(iFtr);
		iMade++;
	}
	return iMade;
}

// COOP s41: the captain of a bot ship on the deck: a land bot of the player with the portrait of the bot (its skin), the name
// of the bot; his death ends the duel when his crew is dead too (COOP s43)
// COOP s44: his level is the level of the player + the difficulty of the bots (CalculateAppropriateSkills: (rate - 5) * 1.5),
// the blade and the pistol by it, the cuirass of the single player ((rate * 2 + level) / 10)
int NetServer_BDMakeBotCaptain(int id, int wOwner, int wSea, int iTeam)
{
	int iFace = 0;
	if (CheckAttribute(&NSClients[wSea], "BotFace")) { iFace = sti(NSClients[wSea].BotFace); }
	if (iFace <= NET_LAND_NPC_FACE && NetLand_FaceToModel(iFace) == "") { iFace = 0; }		// COOP s45: above = an NPC skin
	string sNick = "Капитан";
	if (CheckAttribute(&NSClients[wSea], "NickName")) { sNick = NSClients[wSea].NickName; }
	int iCapBonus = makeint(makefloat(NetServer_BDCapRate() - 5) * 1.5);
	int iLvl = NetServer_BDRank(NetServer_BDBaseOf(id, iTeam) + iCapBonus, 0);
	string sBl = NetServer_BDBladeByRank(iLvl);
	string sGn = NetServer_BDGunByRank(iLvl);
	int iFtr = NetServer_BDMakeFighter(id, wOwner, iTeam, sNick, iFace, iLvl, sBl, sGn);
	if (iFtr < 0) { return -1; }
	NSClients[iFtr].BoardCapOf = wSea;
	NSClients[iFtr].LandBotArmor = NetServer_BDCapArmor(iLvl);
	string sDC = "d" + id;
	NetServer.BD.(sDC).cap = iFtr;
	NetServer_LandBotSendInfo(iFtr);
	Trace("COOP: boarding duel " + id + ": the captain of bot " + wSea + " is land bot " + iFtr + " (face " + iFace + ", level " + iLvl + ", " + NSClients[iFtr].LandBotBlade + ", " + NSClients[iFtr].LandBotGun + ", " + NSClients[iFtr].LandBotArmor + ")");
	return iFtr;
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
	if (w < 0 || w >= NET_MAXPLAYERS) { return; }		// COOP s41: a bot ship has no machine
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
// COOP s41: or a player and a bot ship: the captain and the crew of the bot are land bots of the player. false = no duel (no
// free slot for the captain of the bot): the fast boarding goes on
bool NetServer_BDStart(int wA, int wT)
{
	ref rA = NetServer_GetClient(wA);
	ref rT = NetServer_GetClient(wT);
	int wBot = DST_INVALID;
	int wHum = DST_INVALID;
	if (NetServer_IsBot(wA)) { wBot = wA; wHum = wT; }
	if (NetServer_IsBot(wT)) { wBot = wT; wHum = wA; }
	if (wBot != DST_INVALID && NetServer_BDFreeSlot() < 0) { Trace("COOP: boarding: no free slot for the captain of bot " + wBot + ", the fast boarding"); return false; }
	int wOwnA = wA;
	int wOwnT = wT;
	if (wBot == wA) { wOwnA = wHum; }
	if (wBot == wT) { wOwnT = wHum; }
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
	// COOP s44: the levels of the crews like the single player: the base is the level of the player captain (against a bot
	// both sides take the level of the player: the rank of the hero), the crew that has more men gets a bonus
	int iBaseA = NetServer_BDCapLevel(wA);
	int iBaseT = NetServer_BDCapLevel(wT);
	if (wBot == wA) { iBaseA = iBaseT; }
	if (wBot == wT) { iBaseT = iBaseA; }
	NetServer.BD.(sD).base0 = iBaseA;
	NetServer.BD.(sD).base1 = iBaseT;
	bool bBotA = false;
	bool bBotT = false;
	if (wBot == wA) { bBotA = true; }
	if (wBot == wT) { bBotT = true; }
	NetServer.BD.(sD).bonus0 = NetServer_BDCrewBonus(fA, fT, bBotA);
	NetServer.BD.(sD).bonus1 = NetServer_BDCrewBonus(fT, fA, bBotT);
	if (!CheckAttribute(&NetServer, "LandTeamKills0")) { NetServer.LandTeamKills0 = 0; }
	if (!CheckAttribute(&NetServer, "LandTeamKills1")) { NetServer.LandTeamKills1 = 0; }

	NetServer_BDCaptainInit(wA, id, 0);
	NetServer_BDCaptainInit(wT, id, 1);
	SetEventHandler("NetServer_BDEndEvent", "NetServer_BDEndEvent", 0);

	// the captain is one of the fighters: the crew on the deck is one less
	iNetSrvBDScope = id;
	NetServer.BD.(sD).cap = -1;
	if (wBot != DST_INVALID)
	{
		int iBotTeam = 0;
		if (wBot == wT) { iBotTeam = 1; }
		NetServer_BDMakeBotCaptain(id, wHum, wBot, iBotTeam);		// first: the free slot was checked
	}
	int iCrewA = NetServer_BDMakeCrew(id, wOwnA, 0, nA - 1);
	int iCrewT = NetServer_BDMakeCrew(id, wOwnT, 1, nT - 1);
	int iMushA = NetServer_BDMakeMush(id, wOwnA, 0, wA);		// COOP s44
	int iMushT = NetServer_BDMakeMush(id, wOwnT, 1, wT);
	iCrewA = iCrewA + iMushA;
	iCrewT = iCrewT + iMushT;
	iNetSrvBDScope = 0;

	NetServer_BDSendStart(id, wA, 0, wT, iCrewA + 1, iCrewT + 1);
	NetServer_BDSendStart(id, wT, 1, wA, iCrewT + 1, iCrewA + 1);
	// the loadouts of the captains that the server knows already (the new ones come with the load of the deck)
	if (wBot == DST_INVALID)
	{
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
	}
	if (wA < NET_MAXPLAYERS) { NetServer_BotSendOpts(wA); }
	if (wT < NET_MAXPLAYERS) { NetServer_BotSendOpts(wT); }
	Trace("COOP: boarding duel " + id + ": " + wA + " (class " + cA + ", crew " + fA + ", " + nA + " fighters) boards " + wT + " (class " + cT + ", crew " + fT + ", " + nT + " fighters) on " + sDeck + ", the bigger ship " + iUp + ", bot " + wBot + ", levels " + NetServer.BD.(sD).base0 + "+" + NetServer.BD.(sD).bonus0 + " / " + NetServer.BD.(sD).base1 + "+" + NetServer.BD.(sD).bonus1 + ", musketeers " + iMushA + " / " + iMushT);
	return true;
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

// COOP s43: the side of a bot ship loses only when all its NPCs on the deck are dead (the crew and the captain): the death of
// the captain alone ended the duel while the fight went on (the player died after his "victory" and got the prize)
bool NetServer_BDBotSideDown(int id, int wBotShip)
{
	string sD = "d" + id;
	int iTeam = NetServer_BDTeamOf(wBotShip);
	if (iTeam < 0) { return true; }
	int wCap = -1;
	if (CheckAttribute(&NetServer, "BD." + sD + ".cap")) { wCap = sti(NetServer.BD.(sD).cap); }
	for (int i=NET_BOT_FIRST; i<NET_MAXCLIENTS; i++)
	{
		if (NetServer_BDOf(i) != id) { continue; }
		if (NetServer_BDTeamOf(i) != iTeam) { continue; }
		if (!CheckAttribute(&NSClients[i], "LandBot")) { continue; }		// the bot ship itself
		if (CheckAttribute(&NSClients[i], "LandBotRemoved")) { if (sti(NSClients[i].LandBotRemoved)) { continue; } }
		if (CheckAttribute(&NSClients[i], "LandBotDeadSeen")) { if (sti(NSClients[i].LandBotDeadSeen)) { continue; } }
		return false;		// one of them (maybe the captain) still fights
	}
	return true;
}

// COOP s43: a land bot of the deck is dead: the last NPC of a bot ship ends the duel (Land.c, the death of a bot)
void NetServer_BDFighterDead(int wLandBot)
{
	int id = NetServer_BDOf(wLandBot);
	if (id <= 0) { return; }
	string sD = "d" + id;
	if (!CheckAttribute(&NetServer, "BD." + sD + ".a")) { return; }
	int iTeam = NetServer_BDTeamOf(wLandBot);
	int wA = sti(NetServer.BD.(sD).a);
	int wT = sti(NetServer.BD.(sD).t);
	int wBotShip = DST_INVALID;
	if (NetServer_IsBot(wA) && NetServer_BDTeamOf(wA) == iTeam) { wBotShip = wA; }
	if (NetServer_IsBot(wT) && NetServer_BDTeamOf(wT) == iTeam) { wBotShip = wT; }
	if (wBotShip == DST_INVALID) { return; }
	if (!NetServer_BDBotSideDown(id, wBotShip)) { return; }
	NetServer_BDCaptainDead(wBotShip);
}

// NetServer_LandDead: a captain has fallen - the duel ends a little later (his death is seen)
void NetServer_BDCaptainDead(int wDead)
{
	int id = NetServer_BDOf(wDead);
	if (id <= 0) { return; }
	if (!NetServer_BDIsCaptain(wDead)) { return; }
	string sD = "d" + id;
	if (sti(NetServer.BD.(sD).ending)) { return; }
	// COOP s43: a bot ship: all its NPCs must be dead, not only the captain
	if (NetServer_IsBot(wDead) && !NetServer_BDBotSideDown(id, wDead))
	{
		Trace("COOP: boarding duel " + id + ": the captain of bot " + wDead + " has fallen, his crew fights on");
		return;
	}
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
		if (!CheckAttribute(&NSClients[i], "LandBot")) { continue; }		// COOP s41: the bot ship itself (a captain)
		if (CheckAttribute(&NSClients[i], "BoardCapOf")) { continue; }		// COOP s41: the captain of a bot is counted apart
		if (CheckAttribute(&NSClients[i], "BoardMush")) { continue; }		// COOP s44: the musketeers are not of the crew
		if (CheckAttribute(&NSClients[i], "LandBotDeadSeen")) { if (sti(NSClients[i].LandBotDeadSeen)) { continue; } }
		iN++;
	}
	return iN;
}

void NetServer_BDSendEnd(int w, int iWon)
{
	if (w < 0 || w >= NET_MAXPLAYERS) { return; }
	if (!sti(NSClients[w].Use)) { return; }
	NSClients[w].BoardSent = -2;		// COOP s45/s48i: the boarding state goes to him again, "nothing" too (his panel of the sea is new)
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
		DeleteAttribute(&NSClients[i], "BoardCapOf");
		DeleteAttribute(&NSClients[i], "BoardMush");		// COOP s44
		DeleteAttribute(&NSClients[i], "LandBotArmor");
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
	// the ship of the loser: a prize of the winner (COOP s41: his choice, NetServer_BoardWon) or it goes down with its crew
	rLose.Ship.Crew.Quantity = 0;
	if (!NetServer_IsClientDead(rLose)) { NetServer_BoardWon(wWin, wLose); }
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
		DeleteAttribute(&NSClients[i], "BoardCapOf");
		DeleteAttribute(&NSClients[i], "BoardMush");		// COOP s44
		DeleteAttribute(&NSClients[i], "LandBotArmor");
		NetServer_PrizeRestoreOrig(i);		// COOP s44: the own ship of a slot that sank as the old ship of a captor
		DeleteAttribute(&NSClients[i], "PrizeOf");		// COOP s41
		DeleteAttribute(&NSClients[i], "PrizeTake");
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
// COOP s38: the deck has the sea of the single player around it (the darkness and the net sea above the deck before): the
// stub "Sea" of the sharks goes (the location makes the real one), the weather of the net stops (it set its fog and light
// over the weather of the location)
bool bNetBoardSharks = false;
// COOP s43: the sinking effect (net_execute, not frozen) keeps the pointer of the Sea of the moment a ship sank: the stub Sea
// goes here and the deck makes and deletes its own one -> WaveXZ of a deleted sea (the crash at the boarding of a bot after a
// ship had sunk). It goes with the sea and is made again when the sea comes back.
bool bNetBoardSink = false;

void NetBoard_SinkBack()
{
	if (!bNetBoardSink) { return; }
	bNetBoardSink = false;
	if (IsEntity(&SinkEffect)) { return; }
	if (!IsEntity(&Sea))
	{
		CreateEntity(&Sea, "sea");
		iCoopSharksSeaStub = 1;
	}
	CreateEntity(&SinkEffect, "SINKEFFECT");
	LayerAddObject("net_execute", &SinkEffect, 65532);
	LayerAddObject("net_realize", &SinkEffect, 65532);
}

void NetBoard_SeaHide()
{
	NetClient_CamerasSetActive(false);
	LayerFreeze("net_realize", true);
	MoveParticlesToLayers("execute", "realize");
	NetBoard_NetWeatherRun(false);
	bNetBoardSharks = false;
	if (IsEntity(&Sharks)) { DeleteClass(&Sharks); bNetBoardSharks = true; }
	bNetBoardSink = false;
	if (IsEntity(&SinkEffect)) { DeleteClass(&SinkEffect); bNetBoardSink = true; }
	if (iCoopSharksSeaStub != 0)
	{
		if (IsEntity(&Sea)) { DeleteClass(&Sea); }
		iCoopSharksSeaStub = 0;
	}
	Trace("COOP: boarding: the sea is hidden");
}

// the sharks of the sea battle come back (the location took its Sea away with it)
void NetBoard_SharksBack()
{
	if (!bNetBoardSharks) { return; }
	bNetBoardSharks = false;
	if (IsEntity(&Sharks)) { return; }
	if (!IsEntity(&Sea))
	{
		CreateEntity(&Sea, "sea");
		iCoopSharksSeaStub = 1;
	}
	Sharks.execute = "net_execute";
	Sharks.realize = "net_realize";
	Sharks.executeModels = 75;
	Sharks.realizeModels = 75;
	Sharks.executeParticles = 78;
	Sharks.realizeParticles = 100001;
	CreateEntity(&Sharks, "Sharks");
}

void NetBoard_SeaShow()
{
	LayerFreeze("net_realize", false);
	MoveParticlesToLayers("net_execute", "net_realize");
	NetBoard_NetWeatherRun(true);
	NetBoard_SharksBack();
	NetBoard_SinkBack();
	// COOP s47e: Sailors.dll keeps the sea entity it found when it was made (a crew rebuild in the duel took the Sea of the
	// deck); that one is gone now: a sailor jumping overboard crashed the game (ShipMan::Jump). The crew is made again here
	NetClient_SailorsRebuild();
	NetClient_CamerasSetActive(true);
	ResetSoundScheme();
	SetMusic("music_sea_battle");
	// COOP s45: the deck (a location) made the quick action panel of the land (LI_LocationLoad: LandCommands.tga): the boarding
	// icon of the sea was a land icon then, not the grappling hooks of list_icons.tga. The panel of the sea again
	CreateILogAndActions(LOG_FOR_SEA);
	NetClient_BoardWasAvailable = (NetClient_BoardTarget >= 0);
	if (NetClient_BoardWasAvailable) { Log_SetActiveAction("Board"); }
	else { Log_SetActiveAction("Nothing"); }
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
	// COOP s48i: the server forgets the target it has sent (BoardSent): the old one showed the boarding icon after the duel
	NetClient_BoardTarget = -1;
	NetClient_BoardWasAvailable = false;
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
	NetClient_HostParkReset();		// COOP s38
	if (!bNetBoardDuel) { return; }
	bNetBoardDuel = false;
	LayerFreeze("net_realize", false);
	MoveParticlesToLayers("net_execute", "net_realize");
	NetBoard_NetWeatherRun(true);
	bNetBoardSharks = false;
	bNetBoardSink = false;		// COOP s43: the sea goes now, the next game makes the sinking effect again
}

// ================================================================================================
// COOP s38: THE HOST GOES TO THE LOBBY FROM A SEA BATTLE THAT GOES ON. The server ships of his machine live on: deleting his
// sea (NetClient_OnEndGame) crashed the game in the next frame (logs 2, 3). The sea is hidden like under the boarding deck and
// the lobby opens over it; "Join the battle" shows it again; the end of the game (all go to the lobby) deletes it as always.
// ================================================================================================
bool NetClient_HostCanPark()
{
	if (!NetServer_IsStarted()) { return false; }
	if (!bServerGameStarted) { return false; }
	if (NetServer_LandMode()) { return false; }
	if (!bNetClientInGame) { return false; }
	return true;
}

void NetClient_HostPark()
{
	Trace("COOP: the host goes to the lobby, his sea waits hidden");
	if (bNetBoardDuel)
	{
		// the duel on the deck is lost (the server ends it, NetServer_ClientParkAtSea); the deck goes now
		NetLand_ClientStop();
		bNetBoardDuel = false;
		NetBoard_NetWeatherRun(true);
		NetBoard_SharksBack();
		NetBoard_SinkBack();
	}
	NetServer_ClientParkAtSea(wClientID);
	bNetHostParked = true;
	NetClient_CamerasSetActive(false);
	LayerFreeze("net_realize", true);
	PauseAllSounds();
	ResetSoundScheme();
	SetMusic("music_MainMenu");
	if (sti(InterfaceStates.Launched))
	{
		interfaceResultCommand = RC_INTERFACE_DO_NOTHING;
		EndCancelInterface(true);
	}
	SetEventHandler("NetClient_LobbyOpen", "NetClient_LobbyOpen", 0);
	PostEvent("NetClient_LobbyOpen", 1);
}

// "Join the battle" of the lobby (interfaces\Buy.c: OnTmpStart)
void NetClient_HostUnpark()
{
	if (!bNetHostParked) { return; }
	bNetHostParked = false;
	NetServer_ClientUnparkAtSea(wClientID);
	LayerFreeze("net_realize", false);
	NetClient_CamerasSetActive(true);
	ResetSoundScheme();
	SetMusic("music_sea_battle");
	Trace("COOP: the host is back in the battle");
	Event("NCEvent_StartGame");		// the lobby closes
}

// the game is over (NetClient_OnEndGame deletes the sea now: the server has gone first)
void NetClient_HostParkReset()
{
	if (!bNetHostParked) { return; }
	bNetHostParked = false;
	LayerFreeze("net_realize", false);
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

// COOP s44: a musketeer of the deck stands at the "mush" locators of his side (LAi_SetBoardingActors: "locmush1", "alocmush2"),
// the n-th musketeer of the team at the n-th one; false = no such locator (he stands with the crew)
bool NetBoard_TeleportMush(ref chr, int iTeam, int wBot)
{
	if (wBot < NET_BOT_FIRST || wBot >= NET_MAXCLIENTS) { return false; }
	if (!CheckAttribute(&NCClients[wBot], "LandBotGun")) { return false; }
	if (!NetLand_IsMushItem(NCClients[wBot].LandBotGun)) { return false; }
	if (!IsEntity(loadedLocation)) { return false; }
	string sPre = "loc";
	if (iTeam == iNetBoardUpTeam) { sPre = "aloc"; }
	int iRel = NetLand_BotRelOf(wBot);
	int iN = 1;
	for (int i=NET_BOT_FIRST; i<wBot; i++)
	{
		if (!NetLand_IsLandBot(i)) { continue; }
		if (NetLand_BotRelOf(i) != iRel) { continue; }
		if (!CheckAttribute(&NCClients[i], "LandBotGun")) { continue; }
		if (NetLand_IsMushItem(NCClients[i].LandBotGun)) { iN++; }
	}
	string sL = sPre + "mush" + iN;
	if (!CheckAttribute(loadedLocation, "locators.rld." + sL)) { return false; }
	aref arL;
	makearef(arL, loadedLocation.locators.rld.(sL));
	return TeleportCharacterToPosAy(chr, stf(arL.x), stf(arL.y), stf(arL.z), 0.0);
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

// ================================================================================================
// COOP s41: THE PRIZE. A player who wins a boarding (the fast one or the duel on the deck) gets the ship of the loser as his
// prize, like the transfer screen of the single player: the goods (not more than the capacity), the crew (not more than the
// crew of the ship) and the cannons (only of the same type) go between the two ships, then
//   "Обменяться" - the player takes the prize (his slot gets its type, the column of the prize), his old ship goes down,
//   "Захватить"  - the prize goes on as an allied bot (the difficulty of the bots) with the column of the prize,
//   "Утопить"    - (also Esc and the end of the time) the prize goes down with what is left on it.
// While the player chooses, both ships stand and nothing hurts them (NetServer_ShipHeld). A bot that wins sinks the loser.
//   NSC_SHIP_PRIZE      (server -> winner): clientID prize, word type, byte cannon type, word cannons, word crew, goods...,
//                       word our cannons, word our crew, our goods..., word seconds
//   NSC_SHIP_PRIZE_DONE (winner -> server): byte action (0 sink, 1 exchange, 2 capture), clientID prize, word our crew,
//                       word our cannons, our goods... (the prize gets the rest)
//   NSC_SHIP_PRIZE_SWAP (server -> all):    clientID, word type, float max hp, float hp, byte upgrades hull, sails, cannon type,
//                       upgrades cannons, word crew, goods..., byte charge, float x, z, ay: the ship is a new one now
//   NSC_SHIP_PRIZE_END  (server -> winner): the choice is over (the window closes)
// ================================================================================================
#define NET_PRIZE_TIME_MS		90000
#define NET_PRIZE_SAFE_MS		15000	// COOP s47f: the captor is not hurt only this long while he chooses (his ship still stands)
#define NET_PRIZE_SINK			0
#define NET_PRIZE_EXCHANGE		1
#define NET_PRIZE_CAPTURE		2
#define NET_PRIZE_BOT_SIDE		70.0		// a captured ship of a player: the new bot stands so far beside it

int iNetPrizeGM[8];		// the goods of the two columns (our ship / the prize) of the choice being applied
int iNetPrizeGP[8];
int iNetPrizeCrewM = 0;
int iNetPrizeCanM = 0;

// the ship stands and is not hurt: the captain fights on the boarding deck, or it is a prize / its captor chooses
bool NetServer_ShipHeld(int w)
{
	if (w < 0 || w >= NET_MAXCLIENTS) { return false; }
	if (NetServer_BDOf(w) > 0) { return true; }
	if (CheckAttribute(&NSClients[w], "PrizeOf")) { return true; }
	if (CheckAttribute(&NSClients[w], "PrizeTake")) { return true; }
	return false;
}

// COOP s47f: the ship takes no damage (NetServer_IsGod): as NetServer_ShipHeld, but the captor only NET_PRIZE_SAFE_MS
bool NetServer_ShipSafe(int w)
{
	if (w < 0 || w >= NET_MAXCLIENTS) { return false; }
	if (NetServer_BDOf(w) > 0) { return true; }
	if (CheckAttribute(&NSClients[w], "PrizeOf")) { return true; }
	if (!CheckAttribute(&NSClients[w], "PrizeTake")) { return false; }
	if (!CheckAttribute(&NSClients[w], "PrizeSafeUntil")) { return false; }
	if (iServerTime < sti(NSClients[w].PrizeSafeUntil)) { return true; }
	return false;
}

// the boarding is won (server\OnShip.c, NetServer_BDEnd): the choice of the winner or the loser goes down
void NetServer_BoardWon(int wWin, int wLose)
{
	ref rLose = NetServer_GetClient(wLose);
	if (NetServer_IsClientDead(rLose)) { return; }
	if (NetServer_PrizeStart(wWin, wLose)) { return; }
	if (NetServer_IsBot(wWin) && NetServer_BotPrizeBetter(wWin, wLose)) { NetServer_BotPrizeSwap(wWin, wLose); return; }		// COOP s43
	NetServer_SetClientDead(rLose, NETKILL_BY_BOARDING, wWin);
}

// COOP s43: a bot that wins a boarding takes the ship of the loser when it is better: a bigger class (a smaller number), or
// the same class and 15 % more hull; the hull of the prize must have a quarter left
bool NetServer_BotPrizeBetter(int wW, int wL)
{
	ref rW = NetServer_GetClient(wW);
	ref rL = NetServer_GetClient(wL);
	if (NetServer_IsClientDead(rW)) { return false; }
	if (!CheckAttribute(rL, "Ship.Type") || !CheckAttribute(rW, "Ship.Type")) { return false; }
	int iTW = sti(rW.Ship.Type);
	int iTL = sti(rL.Ship.Type);
	if (iTW < 0 || iTL < 0 || iTW >= iNetShipsNum || iTL >= iNetShipsNum) { return false; }
	if (!CheckAttribute(rL, "Ship.HP") || !CheckAttribute(rL, "Ship.MaxHP") || !CheckAttribute(rW, "Ship.MaxHP")) { return false; }
	float fMaxL = stf(rL.Ship.MaxHP);
	if (fMaxL <= 0.0) { return false; }
	if (stf(rL.Ship.HP) < fMaxL * 0.25) { return false; }
	ref rSW = Net_GetShipByIndex(iTW);
	ref rSL = Net_GetShipByIndex(iTL);
	int iCW = sti(rSW.Class);
	int iCL = sti(rSL.Class);
	if (iCL < iCW) { return true; }
	if (iCL == iCW && fMaxL > stf(rW.Ship.MaxHP) * 1.15) { return true; }
	return false;
}

// COOP s43: the bot moves to the prize like the "Exchange" of a player: its crew (as many as the prize takes), the cannons of
// the prize, all the goods of both ships; its old ship goes down
void NetServer_BotPrizeSwap(int wW, int wL)
{
	ref rW = NetServer_GetClient(wW);
	ref rL = NetServer_GetClient(wL);
	for (int i=0; i<Net_GetGoodsNum(); i++)
	{
		string sG = NetGoods[i].Name;
		iNetPrizeGP[i] = NetServer_GoodTotal(rW, sG) + NetServer_GoodTotal(rL, sG);
		iNetPrizeGM[i] = 0;
	}
	int iCrew = sti(rW.Ship.Crew.Quantity) + sti(rL.Ship.Crew.Quantity);
	int iMaxL = Net_GetCrewCount(sti(rL.Ship.Type), 100);
	if (iCrew > iMaxL) { iCrew = iMaxL; }
	if (iCrew < 1) { iCrew = 1; }
	int iCan = NetServer_ShipCannonsNum(rL);
	Trace("COOP: prize: bot " + wW + " takes the better ship of " + wL + " (type " + rL.Ship.Type + " for " + rW.Ship.Type + "), crew " + iCrew + ", cannons " + iCan);
	NetServer_PrizeSwap(wW, wL, iCrew, iCan);
	NetServer_SetClientDead(rL, NETKILL_BY_BOARDING, wW);
}

void NetServer_PrizeStop(ref rC)
{
	rC.Ship.SailState = 0.0;
	rC.Ship.TurnLeft = 0;
	rC.Ship.TurnRight = 0;
	rC.Ship.TurnState = 0.0;
}

bool NetServer_PrizeStart(int wW, int wL)
{
	int i;
	if (wW < 0 || wW >= NET_MAXPLAYERS) { return false; }		// a bot has won
	if (wL < 0 || wL >= NET_MAXCLIENTS || wL == wW) { return false; }
	if (!sti(NSClients[wW].Use) || !sti(NSClients[wW].InGame)) { return false; }
	ref rW = NetServer_GetClient(wW);
	ref rL = NetServer_GetClient(wL);
	if (NetServer_IsClientDead(rW)) { return false; }
	if (CheckAttribute(rW, "PrizeTake")) { return false; }		// he chooses for another prize now
	if (!CheckAttribute(rL, "Ship.Type") || sti(rL.Ship.Type) < 0) { return false; }

	if (!CheckAttribute(&NetServer, "PrizeGen")) { NetServer.PrizeGen = 0; }
	int iGen = sti(NetServer.PrizeGen) + 1;
	NetServer.PrizeGen = iGen;
	rL.Ship.Crew.Quantity = 0;
	rL.PrizeOf = wW;
	rW.PrizeTake = wL;
	rW.PrizeGen = iGen;
	rW.PrizeSafeUntil = iServerTime + NET_PRIZE_SAFE_MS;		// COOP s47f
	NetServer_PrizeStop(rW);
	NetServer_PrizeStop(rL);

	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_SHIP);
	NMAddByte(iSMsg, NSC_SHIP_PRIZE);
	NMAddClientID(iSMsg, wL);
	NMAddWord(iSMsg, sti(rL.Ship.Type));
	NMAddByte(iSMsg, sti(rL.Ship.Cannons.Type));
	NMAddWord(iSMsg, NetServer_ShipCannonsNum(rL));
	NMAddWord(iSMsg, sti(rL.Ship.Crew.Quantity));
	string sG;
	for (i=0; i<Net_GetGoodsNum(); i++) { sG = NetGoods[i].Name; NMAddWord(iSMsg, NetServer_GoodTotal(rL, sG)); }
	NMAddWord(iSMsg, NetServer_ShipCannonsNum(rW));
	NMAddWord(iSMsg, sti(rW.Ship.Crew.Quantity));
	for (i=0; i<Net_GetGoodsNum(); i++) { sG = NetGoods[i].Name; NMAddWord(iSMsg, NetServer_GoodTotal(rW, sG)); }
	NMAddWord(iSMsg, NET_PRIZE_TIME_MS / 1000);
	NetServer_SendMessage(wW, iSMsg, true);
	NMDelete(iSMsg);

	SetEventHandler("NetServer_PrizeTimeout", "NetServer_PrizeTimeout", 0);
	PostEvent("NetServer_PrizeTimeout", NET_PRIZE_TIME_MS, "llll", wW, wL, iGen, sti(NetServer.Round));
	Trace("COOP: prize: " + wW + " has taken " + wL + " (type " + rL.Ship.Type + "), he chooses");
	return true;
}

// the columns as they are now (the time is over, the captor or the prize is gone)
void NetServer_PrizeKeepAsIs(int wW, int wL)
{
	ref rW = NetServer_GetClient(wW);
	for (int i=0; i<Net_GetGoodsNum(); i++) { string sG = NetGoods[i].Name; iNetPrizeGM[i] = NetServer_GoodTotal(rW, sG); }
	iNetPrizeCrewM = sti(rW.Ship.Crew.Quantity);
	iNetPrizeCanM = NetServer_ShipCannonsNum(rW);
}

void NetServer_PrizeTimeout()
{
	int wW = GetEventData();
	int wL = GetEventData();
	int iGen = GetEventData();
	int iRound = GetEventData();
	if (!bServerGameStarted) { return; }
	if (iRound != sti(NetServer.Round)) { return; }
	if (!CheckAttribute(&NSClients[wW], "PrizeTake")) { return; }
	if (sti(NSClients[wW].PrizeTake) != wL) { return; }
	if (sti(NSClients[wW].PrizeGen) != iGen) { return; }
	Trace("COOP: prize: the time of " + wW + " is over, " + wL + " goes down");
	NetServer_PrizeKeepAsIs(wW, wL);
	NetServer_PrizeApply(wW, wL, NET_PRIZE_SINK);
}

// a slot leaves the game (a player has gone, a bot is deleted): its prize goes down / it is not a prize any more
void NetServer_PrizeSlotGone(int w)
{
	if (w < 0 || w >= NET_MAXCLIENTS) { return; }
	NetServer_PrizeCaptorGone(w);
	if (CheckAttribute(&NSClients[w], "PrizeOf"))
	{
		int wC = sti(NSClients[w].PrizeOf);
		DeleteAttribute(&NSClients[w], "PrizeOf");
		if (wC >= 0 && wC < NET_MAXCLIENTS)
		{
			DeleteAttribute(&NSClients[wC], "PrizeTake");
			NetServer_PrizeSendEnd(wC);
		}
	}
}

// the captor has left the game (or went to the lobby): the prize goes down (the ship of a player that is the prize of somebody
// waits for the choice of its captor even when that player leaves)
void NetServer_PrizeCaptorGone(int w)
{
	if (w < 0 || w >= NET_MAXCLIENTS) { return; }
	if (!CheckAttribute(&NSClients[w], "PrizeTake")) { return; }
	int wL = sti(NSClients[w].PrizeTake);
	NetServer_PrizeKeepAsIs(w, wL);
	NetServer_PrizeApply(w, wL, NET_PRIZE_SINK);
}

void NetServer_PrizeSendEnd(int wW)
{
	if (wW < 0 || wW >= NET_MAXPLAYERS) { return; }
	if (!sti(NSClients[wW].Use)) { return; }
	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_SHIP);
	NMAddByte(iSMsg, NSC_SHIP_PRIZE_END);
	NetServer_SendMessage(wW, iSMsg, true);
	NMDelete(iSMsg);
}

// NSC_SHIP_PRIZE_DONE of the captor
void NetServer_PrizeDone(int wW, int iMsg)
{
	int iAct = NMGetByte(iMsg);
	int wL = NMGetClientID(iMsg);
	iNetPrizeCrewM = NMGetWord(iMsg);
	iNetPrizeCanM = NMGetWord(iMsg);
	for (int i=0; i<Net_GetGoodsNum(); i++) { iNetPrizeGM[i] = NMGetWord(iMsg); }
	if (wW < 0 || wW >= NET_MAXPLAYERS) { return; }
	if (!CheckAttribute(&NSClients[wW], "PrizeTake")) { return; }
	if (sti(NSClients[wW].PrizeTake) != wL) { return; }
	if (iAct < NET_PRIZE_SINK || iAct > NET_PRIZE_CAPTURE) { iAct = NET_PRIZE_SINK; }
	NetServer_PrizeApply(wW, wL, iAct);
}

// the goods / crew / cannons of a column on a ship (bPrize: the column of the prize); bBorts: the cannons are set again
void NetServer_PrizeSetLoad(ref rC, int iCrew, int iCannons, bool bPrize, bool bBorts)
{
	for (int i=0; i<Net_GetGoodsNum(); i++)
	{
		string sG = NetGoods[i].Name;
		if (bPrize) { rC.Ship.Goods.(sG) = iNetPrizeGP[i]; }
		else { rC.Ship.Goods.(sG) = iNetPrizeGM[i]; }
	}
	DeleteAttribute(rC, "Ship.GoodsHeld");
	rC.Ship.AmmoLast = NetServer_AmmoSum(rC);
	rC.Ship.Crew.Quantity = iCrew;
	if (bBorts || iCannons != NetServer_ShipCannonsNum(rC)) { NetServer_SetBortsByCount(rC, iCannons); }
	NetServer_CannonRecalculateParameters(rC);
}

void NetServer_PrizeApply(int wW, int wL, int iAct)
{
	int i;
	ref rW = NetServer_GetClient(wW);
	ref rL = NetServer_GetClient(wL);
	DeleteAttribute(rW, "PrizeTake");
	DeleteAttribute(rL, "PrizeOf");
	NetServer_PrizeSendEnd(wW);
	if (NetServer_IsClientDead(rL)) { return; }
	bool bWOk = true;
	if (NetServer_IsClientDead(rW)) { bWOk = false; }
	if (wW >= NET_MAXPLAYERS || !sti(NSClients[wW].Use)) { bWOk = false; }
	if (!bWOk)
	{
		NetServer_SetClientDead(rL, NETKILL_BY_BOARDING, wW);
		return;
	}

	int iTypeW = sti(rW.Ship.Type);
	int iTypeL = sti(rL.Ship.Type);
	// the goods: nothing comes from nowhere, the prize gets the rest
	for (i=0; i<Net_GetGoodsNum(); i++)
	{
		string sG = NetGoods[i].Name;
		int iTot = NetServer_GoodTotal(rW, sG) + NetServer_GoodTotal(rL, sG);
		if (iNetPrizeGM[i] < 0) { iNetPrizeGM[i] = 0; }
		if (iNetPrizeGM[i] > iTot) { iNetPrizeGM[i] = iTot; }
		iNetPrizeGP[i] = iTot - iNetPrizeGM[i];
	}
	// the crew: not more than the ship takes
	int iTotC = sti(rW.Ship.Crew.Quantity) + sti(rL.Ship.Crew.Quantity);
	int iMaxW = Net_GetCrewCount(iTypeW, 100);
	int iMaxL = Net_GetCrewCount(iTypeL, 100);
	int iCrewM = iNetPrizeCrewM;
	if (iCrewM < 0) { iCrewM = 0; }
	if (iCrewM > iTotC) { iCrewM = iTotC; }
	if (iCrewM > iMaxW) { iCrewM = iMaxW; }
	int iCrewP = iTotC - iCrewM;
	if (iCrewP > iMaxL) { iCrewP = iMaxL; }
	// the cannons go only between the ships with the same cannons
	int iCanW = NetServer_ShipCannonsNum(rW);
	int iCanL = NetServer_ShipCannonsNum(rL);
	int iCanM = iCanW;
	int iCanP = iCanL;
	if (sti(rW.Ship.Cannons.Type) == sti(rL.Ship.Cannons.Type))
	{
		iCanM = iNetPrizeCanM;
		if (iCanM < 0) { iCanM = 0; }
		if (iCanM > iCanW + iCanL) { iCanM = iCanW + iCanL; }
		if (iCanM > Net_GetShipCannonsNum(iTypeW)) { iCanM = Net_GetShipCannonsNum(iTypeW); }
		iCanP = iCanW + iCanL - iCanM;
		if (iCanP > Net_GetShipCannonsNum(iTypeL)) { iCanP = Net_GetShipCannonsNum(iTypeL); }
	}
	// a ship that sails on has a crew
	if (iAct == NET_PRIZE_SINK && iCrewM < 1)
	{
		iCrewM = iTotC;
		if (iCrewM > iMaxW) { iCrewM = iMaxW; }
		if (iCrewM < 1) { iCrewM = 1; }
	}
	if (iAct == NET_PRIZE_EXCHANGE && iCrewP < 1)
	{
		iCrewP = iTotC;
		if (iCrewP > iMaxL) { iCrewP = iMaxL; }
		if (iCrewP < 1) { iCrewP = 1; }
	}
	if (iAct == NET_PRIZE_CAPTURE)
	{
		if (iCrewP < 1 && iCrewM > 1) { iCrewM = iCrewM - 1; iCrewP = 1; }
		if (iCrewM < 1 && iCrewP > 1) { iCrewP = iCrewP - 1; iCrewM = 1; }
		if (iCrewP < 1) { iAct = NET_PRIZE_SINK; }
		if (iCrewM < 1) { iCrewM = 1; }
	}
	Trace("COOP: prize: " + wW + " chose " + iAct + " for " + wL + ": crew " + iCrewM + " / " + iCrewP + ", cannons " + iCanM + " / " + iCanP);

	if (iAct == NET_PRIZE_EXCHANGE)
	{
		NetServer_PrizeSwap(wW, wL, iCrewP, iCanP);
		NetServer_SetClientDead(rL, NETKILL_BY_BOARDING, wW);
	}
	else
	{
		NetServer_PrizeSetLoad(rW, iCrewM, iCanM, false, false);
		if (iAct == NET_PRIZE_CAPTURE) { NetServer_PrizeCapture(wW, wL, iCrewP, iCanP); }
		else { NetServer_SetClientDead(rL, NETKILL_BY_BOARDING, wW); }
	}
	NetServer_ShipSendLoadInfo(wW);
}

// COOP s44: the ship of a slot (the type, the hull, the sails, the upgrades, the cannons) to other attributes
object oNetPrizeOldShip;		// the old ship of the captor while NetServer_PrizeSwap gives it to the prize slot

void NetServer_PrizeCopyShip(aref arDst, aref arSrc)
{
	arDst.Ship.Type = sti(arSrc.Ship.Type);
	arDst.Ship.MaxHP = stf(arSrc.Ship.MaxHP);
	arDst.Ship.HP = stf(arSrc.Ship.HP);
	if (CheckAttribute(arSrc, "Ship.SP")) { arDst.Ship.SP = stf(arSrc.Ship.SP); }
	if (CheckAttribute(arSrc, "Ship.Upgrades.Hull")) { arDst.Ship.Upgrades.Hull = sti(arSrc.Ship.Upgrades.Hull); }
	if (CheckAttribute(arSrc, "Ship.Upgrades.Sails")) { arDst.Ship.Upgrades.Sails = sti(arSrc.Ship.Upgrades.Sails); }
	if (CheckAttribute(arSrc, "Ship.Upgrades.Cannons")) { arDst.Ship.Upgrades.Cannons = sti(arSrc.Ship.Upgrades.Cannons); }
	if (CheckAttribute(arSrc, "Ship.Cannons.Type")) { arDst.Ship.Cannons.Type = sti(arSrc.Ship.Cannons.Type); }
	if (CheckAttribute(arSrc, "Ship.SpeedRateModifier")) { arDst.Ship.SpeedRateModifier = stf(arSrc.Ship.SpeedRateModifier); }
	if (CheckAttribute(arSrc, "Ship.TurnRateModifier")) { arDst.Ship.TurnRateModifier = stf(arSrc.Ship.TurnRateModifier); }
	if (CheckAttribute(arSrc, "Ship.SailsDamageModifier")) { arDst.Ship.SailsDamageModifier = stf(arSrc.Ship.SailsDamageModifier); }
}

// COOP s44: NSC_SHIP_PRIZE_SWAP: the ship of a slot is another one now, every machine creates it again at this place
void NetServer_PrizeSendSwap(int w, float fX, float fZ, float fAy)
{
	ref rC = NetServer_GetClient(w);
	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_SHIP);
	NMAddByte(iSMsg, NSC_SHIP_PRIZE_SWAP);
	NMAddClientID(iSMsg, w);
	NMAddWord(iSMsg, sti(rC.Ship.Type));
	NMAddFloat(iSMsg, stf(rC.Ship.MaxHP));
	NMAddFloat(iSMsg, stf(rC.Ship.HP));
	NMAddByte(iSMsg, sti(rC.Ship.Upgrades.Hull));
	NMAddByte(iSMsg, sti(rC.Ship.Upgrades.Sails));
	NMAddByte(iSMsg, sti(rC.Ship.Cannons.Type));
	NMAddByte(iSMsg, sti(rC.Ship.Upgrades.Cannons));
	NMAddWord(iSMsg, sti(rC.Ship.Crew.Quantity));
	for (int i=0; i<Net_GetGoodsNum(); i++) { string sG = NetGoods[i].Name; NMAddWord(iSMsg, sti(rC.Ship.Goods.(sG))); }
	NMAddByte(iSMsg, sti(rC.Ship.Cannons.Charge.Type));
	NMAddFloat(iSMsg, fX);
	NMAddFloat(iSMsg, fZ);
	NMAddFloat(iSMsg, fAy);
	NetServer_SendToInGame(iSMsg);
	NMDelete(iSMsg);
}

// COOP s44: the prize slot gets the old ship of the captor at its own place: that ship goes down then (the callers kill the
// slot just after), not a second copy of the prize (a bot that took the xebec of a player: a xebec sank, not its caravel).
// Its own ship is kept: a player (and a bot that respawns) gets it back at the respawn or the next game (NetServer_PrizeRestoreOrig)
void NetServer_PrizeLoserTakesOld(int wL)
{
	aref arL;
	makearef(arL, NSClients[wL]);
	aref arOld;
	makearef(arOld, oNetPrizeOldShip);
	if (!CheckAttribute(arOld, "Ship.Type")) { return; }
	if (!CheckAttribute(arL, "PrizeOrig.Ship.Type"))
	{
		arL.PrizeOrig = "";
		aref arOrig;
		makearef(arOrig, arL.PrizeOrig);
		NetServer_PrizeCopyShip(arOrig, arL);
	}
	NetServer_PrizeCopyShip(arL, arOld);
	DeleteAttribute(arL, "Ship.Bort");
	// COOP s48j: "Ship.sails" holds Sails.Color too (a reconnect crashed the client without it)
	int iSailColL = argb(255, 255, 255, 255);
	if (CheckAttribute(arL, "Ship.Sails.Color")) { iSailColL = sti(arL.Ship.Sails.Color); }
	DeleteAttribute(arL, "Ship.sails");
	arL.Ship.Sails.Color = iSailColL;
	DeleteAttribute(arL, "Ship.masts");
	DeleteAttribute(arL, "Ship.MastsFallen");
	NetServer_PrizeSendSwap(wL, stf(arL.Ship.Pos.x), stf(arL.Ship.Pos.z), stf(arL.Ship.Ang.y));
	Trace("COOP: prize: the slot " + wL + " has the old ship of the captor now (type " + arL.Ship.Type + "), it goes down");
}

// COOP s44: the own ship of a slot that had to take the old ship of a captor; true = it is back
bool NetServer_PrizeRestoreOrig(int w)
{
	if (w < 0 || w >= NET_MAXCLIENTS) { return false; }
	aref arC;
	makearef(arC, NSClients[w]);
	if (!CheckAttribute(arC, "PrizeOrig.Ship.Type")) { return false; }
	aref arOrig;
	makearef(arOrig, arC.PrizeOrig);
	NetServer_PrizeCopyShip(arC, arOrig);
	DeleteAttribute(arC, "PrizeOrig");
	Trace("COOP: prize: the slot " + w + " has its own ship again (type " + arC.Ship.Type + ")");
	return true;
}

// "Обменяться": the slot of the captor gets the type and the column of the prize; the ship is created again where it stands
void NetServer_PrizeSwap(int wW, int wL, int iCrew, int iCannons)
{
	ref rW = NetServer_GetClient(wW);
	ref rL = NetServer_GetClient(wL);
	float fX = stf(rW.Ship.Pos.x);
	float fZ = stf(rW.Ship.Pos.z);
	float fAy = stf(rW.Ship.Ang.y);
	aref arOldW;
	makearef(arOldW, oNetPrizeOldShip);
	DeleteAttribute(arOldW, "");
	aref arW;
	makearef(arW, NSClients[wW]);
	NetServer_PrizeCopyShip(arOldW, arW);		// COOP s44: the old ship of the captor goes to the prize slot
	rW.Ship.Type = sti(rL.Ship.Type);
	rW.Ship.MaxHP = stf(rL.Ship.MaxHP);
	rW.Ship.HP = stf(rL.Ship.HP);
	if (CheckAttribute(rL, "Ship.SP")) { rW.Ship.SP = stf(rL.Ship.SP); }
	if (CheckAttribute(rL, "Ship.Upgrades.Hull")) { rW.Ship.Upgrades.Hull = sti(rL.Ship.Upgrades.Hull); }
	if (CheckAttribute(rL, "Ship.Upgrades.Sails")) { rW.Ship.Upgrades.Sails = sti(rL.Ship.Upgrades.Sails); }
	if (CheckAttribute(rL, "Ship.Upgrades.Cannons")) { rW.Ship.Upgrades.Cannons = sti(rL.Ship.Upgrades.Cannons); }
	rW.Ship.Cannons.Type = sti(rL.Ship.Cannons.Type);
	if (CheckAttribute(rL, "Ship.SpeedRateModifier")) { rW.Ship.SpeedRateModifier = stf(rL.Ship.SpeedRateModifier); }
	if (CheckAttribute(rL, "Ship.TurnRateModifier")) { rW.Ship.TurnRateModifier = stf(rL.Ship.TurnRateModifier); }
	if (CheckAttribute(rL, "Ship.SailsDamageModifier")) { rW.Ship.SailsDamageModifier = stf(rL.Ship.SailsDamageModifier); }
	DeleteAttribute(rW, "Ship.Bort");
	// COOP s48j: "Ship.sails" holds Sails.Color too (a reconnect crashed the client without it)
	int iSailColW = argb(255, 255, 255, 255);
	if (CheckAttribute(rW, "Ship.Sails.Color")) { iSailColW = sti(rW.Ship.Sails.Color); }
	DeleteAttribute(rW, "Ship.sails");
	rW.Ship.Sails.Color = iSailColW;
	DeleteAttribute(rW, "Ship.masts");
	DeleteAttribute(rW, "Ship.MastsFallen");
	DeleteAttribute(rW, "Ship.ShotAcc");
	rW.Ship.CannonsDumped = 0;
	NetServer_PrizeSetLoad(rW, iCrew, iCannons, true, true);
	NetServer_CalculateFireTime(rW);
	NetServer_CalculateReloadTime(rW);
	NetServer_SnapshotLoadout(rW);
	NetServer_ShipRecreateLater(wW, fX, fZ, fAy);
	NetServer_PrizeSendSwap(wW, fX, fZ, fAy);
	NetServer_PrizeLoserTakesOld(wL);		// COOP s44
	Trace("COOP: prize: " + wW + " takes the ship of " + wL + " (type " + rW.Ship.Type + ", crew " + iCrew + ", cannons " + iCannons + "), his old ship goes down");
}

// "Захватить": the prize sails on as an allied bot (of the team of the captor in a team game)
void NetServer_PrizeCapture(int wW, int wL, int iCrew, int iCannons)
{
	int iRel = NET_BOTREL_ALLY;
	int iTeams = NetServer_BotTeamsNum();
	if (iTeams > 0 && CheckAttribute(&NSClients[wW], "Team"))
	{
		int iTm = sti(NSClients[wW].Team);
		if (iTm >= 0 && iTm < iTeams) { iRel = NET_BOTREL_TEAM0 + iTm; }
	}
	ref rL = NetServer_GetClient(wL);
	if (NetServer_IsBot(wL))
	{
		NetServer_PrizeSetLoad(rL, iCrew, iCannons, true, false);
		rL.BotOwner = wW;
		NetServer_BotSetRel(wW, wL, iRel);
		NetServer_BotApplySkill(rL, NetServer_BDSkill());
		rL.BotWanderAy = stf(rL.Ship.Ang.y);
		rL.BotWanderUntil = iServerTime + 5000;
		Trace("COOP: prize: bot " + wL + " is captured by " + wW + ", relation " + iRel + ", crew " + iCrew);
		return;
	}
	// the ship of a player can not become a bot: a new allied bot of its type stands beside it, the ship of the player goes down
	int iB = NetServer_BotFreeSlot();
	if (iB == DST_INVALID)
	{
		Trace("COOP: prize: no free bot slot, " + wL + " goes down");
		NetServer_SetClientDead(rL, NETKILL_BY_BOARDING, wW);
		return;
	}
	int iHull = 100;
	if (stf(rL.Ship.MaxHP) > 0.0) { iHull = makeint(100.0 * stf(rL.Ship.HP) / stf(rL.Ship.MaxHP) + 0.5); }
	if (iHull < 5) { iHull = 5; }
	if (iHull > 100) { iHull = 100; }
	int iSails = 100;
	if (CheckAttribute(rL, "Ship.SP")) { iSails = makeint(stf(rL.Ship.SP)); }
	if (iSails < 5) { iSails = 5; }
	if (iSails > 100) { iSails = 100; }
	float fAy = stf(rL.Ship.Ang.y);
	float fX = stf(rL.Ship.Pos.x) + cos(fAy) * NET_PRIZE_BOT_SIDE;
	float fZ = stf(rL.Ship.Pos.z) - sin(fAy) * NET_PRIZE_BOT_SIDE;
	NetServer_BotSetup(iB, wW, sti(rL.Ship.Type), sti(rL.Ship.Cannons.Type), 100, iHull, iSails, iRel);
	ref rB = NetServer_GetClient(iB);
	NetServer_PrizeSetLoad(rB, iCrew, iCannons, true, true);
	NetServer_SetClientDead(rL, NETKILL_BY_BOARDING, wW);
	NetServer_BotSpawn(iB, fX, fZ, fAy);
	Trace("COOP: prize: the ship of player " + wL + " is captured by " + wW + ": bot " + iB + ", relation " + iRel + ", crew " + iCrew);
}

// ================================================================================================
// COOP s41: THE PRIZE, the client
// ================================================================================================
object NCPrize;
bool bNetPrizeWnd = false;		// the window is open (net\interfaces\Prize.c)
int iNetPrizeOpenGen = 0;

void NetPrize_OnStart(int iMsg)
{
	int i;
	DeleteAttribute(&NCPrize, "");
	NCPrize.id = NMGetClientID(iMsg);
	NCPrize.type = NMGetWord(iMsg);
	NCPrize.cantype = NMGetByte(iMsg);
	NCPrize.cannons = NMGetWord(iMsg);
	NCPrize.crew = NMGetWord(iMsg);
	string sG;
	for (i=0; i<Net_GetGoodsNum(); i++) { sG = NetGoods[i].Name; NCPrize.goods.(sG) = NMGetWord(iMsg); }
	NCPrize.my.cannons = NMGetWord(iMsg);
	NCPrize.my.crew = NMGetWord(iMsg);
	for (i=0; i<Net_GetGoodsNum(); i++) { sG = NetGoods[i].Name; NCPrize.my.goods.(sG) = NMGetWord(iMsg); }
	int iSec = NMGetWord(iMsg);
	NCPrize.until = iClientServerTime + iSec * 1000 - 1500;
	NCPrize.active = 1;
	iNetPrizeOpenGen++;
	Log_SetStringToLog("Корабль взят! Перегрузите добычу и решите его судьбу");
	SetEventHandler("NetPrize_OpenTry", "NetPrize_OpenTry", 0);
	PostEvent("NetPrize_OpenTry", 800, "l", iNetPrizeOpenGen);
	Trace("COOP: prize: we have taken " + NCPrize.id + " (type " + NCPrize.type + ", crew " + NCPrize.crew + ", cannons " + NCPrize.cannons + ")");
}

// the window opens when no other window is open (and the deck of the boarding has gone)
void NetPrize_OpenTry()
{
	int iGen = GetEventData();
	if (iGen != iNetPrizeOpenGen) { return; }
	if (!CheckAttribute(&NCPrize, "active") || !sti(NCPrize.active)) { return; }
	if (bNetPrizeWnd) { return; }
	if (!bNetClientInGame) { return; }
	if (bNetBoardDuel || sti(InterfaceStates.Launched))
	{
		PostEvent("NetPrize_OpenTry", 500, "l", iGen);
		return;
	}
	LaunchNetInterface_Prize();
}

void LaunchNetInterface_Prize()
{
	Interfaces[INTERFACE_NET_DUMP].SectionName = "GPK\net\interfaces\Prize.c";
	Interfaces[INTERFACE_NET_DUMP].IniFile = "RESOURCE\INI\INTERFACES\Net\Prize.ini";
	if (procInterfacePrepare(INTERFACE_NET_DUMP))
	{
		nPrevInterface = -1;
		CurrentInterface = INTERFACE_NET_DUMP;
		InitInterface(Interfaces[CurrentInterface].IniFile);
	}
}

// the window: the choice goes to the server (iCrew, iCannons and NCPrize.send.goods: our column)
void NetPrize_Send(int iAct, int iCrew, int iCannons)
{
	if (!CheckAttribute(&NCPrize, "active") || !sti(NCPrize.active)) { return; }
	NCPrize.active = 0;
	int iSMsg = NMCreate();
	NMAddClientID(iSMsg, wClientID);
	NMAddByte(iSMsg, NC_SHIP);
	NMAddByte(iSMsg, NSC_SHIP_PRIZE_DONE);
	NMAddByte(iSMsg, iAct);
	NMAddClientID(iSMsg, sti(NCPrize.id));
	NMAddWord(iSMsg, iCrew);
	NMAddWord(iSMsg, iCannons);
	for (int i=0; i<Net_GetGoodsNum(); i++) { string sG = NetGoods[i].Name; NMAddWord(iSMsg, sti(NCPrize.send.goods.(sG))); }
	NetClient_SendMessage(iSMsg, true);
	NMDelete(iSMsg);
	if (iAct == 1) { Log_SetStringToLog("Вы перешли на захваченный корабль, старый идёт ко дну"); }
	if (iAct == 2) { Log_SetStringToLog("Захваченный корабль идёт с вами под командой союзника"); }
	if (iAct == 0) { Log_SetStringToLog("Захваченный корабль пущен ко дну"); }
	Trace("COOP: prize: our choice " + iAct + ", crew " + iCrew + ", cannons " + iCannons);
}

// the server: the time is over / the prize is gone
void NetPrize_OnEnd(int iMsg)
{
	if (CheckAttribute(&NCPrize, "active") && sti(NCPrize.active))
	{
		NCPrize.active = 0;
		Log_SetStringToLog("Время решения вышло: захваченный корабль идёт ко дну");
	}
	iNetPrizeOpenGen++;
	if (bNetPrizeWnd) { Event("NetPrize_Close"); }
}

// a ship has a new type (our exchange or the one of another player)
void NetPrize_OnSwap(int iMsg)
{
	int wID = NMGetClientID(iMsg);
	int iType = NMGetWord(iMsg);
	float fMaxHP = NMGetFloat(iMsg);
	float fHP = NMGetFloat(iMsg);
	int iUpH = NMGetByte(iMsg);
	int iUpS = NMGetByte(iMsg);
	int iCanT = NMGetByte(iMsg);
	int iUpC = NMGetByte(iMsg);
	int iCrew = NMGetWord(iMsg);
	ref rC = NetClient_GetClient(wID);
	for (int i=0; i<Net_GetGoodsNum(); i++) { string sG = NetGoods[i].Name; rC.Ship.Goods.(sG) = NMGetWord(iMsg); }
	int iCharge = NMGetByte(iMsg);
	float fX = NMGetFloat(iMsg);
	float fZ = NMGetFloat(iMsg);
	float fAy = NMGetFloat(iMsg);
	if (wID < 0 || wID >= NET_MAXCLIENTS) { return; }
	if (iType < 0 || iType >= Net_GetShipsNum()) { return; }
	rC.Ship.Type = iType;
	rC.Ship.MaxHP = fMaxHP;
	rC.Ship.HP = fHP;
	rC.Ship.Upgrades.Hull = iUpH;
	rC.Ship.Upgrades.Sails = iUpS;
	rC.Ship.Cannons.Type = iCanT;
	rC.Ship.Upgrades.Cannons = iUpC;
	rC.Ship.Crew.Quantity = iCrew;
	rC.Ship.Cannons.Charge.Type = iCharge;
	ref rShip = Net_GetShipByIndex(iType);
	rC.Model = rShip.Name + "1";
	rC.Ship.NewPos.x = fX;
	rC.Ship.NewPos.z = fZ;
	if (NetClient_ShipEntityRecreate(wID, fX, fZ, fAy, "prize exchange"))
	{
		RefreshBattleInterface();
		if (wID == wClientID) { NetBI_MainShipAgain(); }
	}
	Trace("COOP: prize: ship " + wID + " is a " + rShip.Name + " now");
}
