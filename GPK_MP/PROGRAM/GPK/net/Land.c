// ================================================================================================
// COOP: land duel (prototype). Chosen on the "Create game" screen: the last row of the list of locations.
//
// net.dll knows nothing about people on land, so everything goes over our own messages (NC_LAND):
//   - every client loads the location like the single player game and plays its own hero (pchar);
//   - 8 times a second it sends where it stands, where it looks, the fight mode and its HP;
//   - every other player is a "puppet" here: an NPC (actor) that runs to the reported point and replays the attacks;
//   - a hit is decided on the machine of the attacker (his blade hits the puppet there): the damage goes to the victim,
//     the victim applies it to its own hero and tells everybody when it dies. The server only relays and counts.
// ================================================================================================

// the message codes are in Net_Defines.h (NC_LAND, NSC_LAND_*, NET_LANDACT_*)

#define NET_LAND_GROUP			"NetLandPuppets"
#define NET_LAND_HP				150.0
#define NET_LAND_BLADE			"blade5"
#define NET_LAND_PISTOL			"pistol1"
#define NET_LAND_TICK			10		// ms between the state packets of our hero (was 120)
#define NET_LAND_SMOOTH			8		// ms between the moves of the puppets on the screen
#define NET_LAND_SMOOTH_K		0.4		// the part of the way to the target that a puppet makes at a move
#define NET_LAND_IDLE_TICK		300		// a standing hero is repeated this often
#define NET_LAND_LEAD			0.08	// s: the puppet runs to the point where the hero will be, not to where he was
#define NET_LAND_RUN_SPEED		2.6		// m/s: faster = the puppet runs, slower = walks
#define NET_LAND_ACT_REPEAT		60		// ms: the attack packet is sent a second time (not guaranteed, numbered)
// how long the puppet plays an attack before it is told to stop (an action of CharacterPlayAction loops until it is cleared)
#define NET_LAND_T_FAST			1000
#define NET_LAND_T_FORCE			1400
#define NET_LAND_T_ROUND			1800
#define NET_LAND_T_BREAK			1500
#define NET_LAND_T_FEINT			1000
#define NET_LAND_T_SHOT			2000
#define NET_LAND_T_HOP			1000
#define NET_LAND_T_FLINCH			600
#define NET_LAND_T_PARRY			1200
#define NET_LAND_HOP_BUSY		480		// ms: the hero cannot start a new hop while one plays (recoil ~470, the side hops ~300-400)
#define NET_LAND_REACH			3.2		// m: a blade hit does not reach a hero that has moved farther away (a hop back)
#define NET_LAND_SNAP			2.0		// m: farther than this the puppet is put on the place at once

bool bNetLandSavedReload = false;
string sNetLandEntryGroup = "rld";		// where the hero came into the location: the puppets are put there as well
string sNetLandEntryLocator = "loc0";
int iNetLandProtectUntil = 0;			// nobody can hurt a player who has not chosen his equipment yet
bool bNetLandAnimOk = false;			// the patched Location.dll (message "GetAnim") is there: the animation of the hero is sent as it is
string sNetLandAnimSent0 = "";
string sNetLandAnimSent1 = "";
string sNetLandAnimSent2 = "";
int iNetLandAnimSentAt = 0;
int iNetLandPrelimAt = 0;
int iNetLandSwingBusyUntil = 0;			// the animation of the last swing is on: the game ignores the clicks
float fNetLandSpEMA = 0.0;
bool bNetLandActive = false;		// this client is in a land duel
bool bNetLandDead = false;			// our hero is dead (waits for the respawn)
bool bNetLandOpenLoadout = false;	// net\interfaces\NetMenu.c: open the loadout window at once
string sNetLandLoc = "";
int iNetLandSpawn = 0;
int iNetLandGen = 0;
int iNetLandRespawn = 0;
int iNetLandRespawnSec = 0;
int iNetLandNoPistol = 0;
float fNetLandPX = 0.0;				// the last sent position (speed of our hero)
float fNetLandPZ = 0.0;
int iNetLandPT = 0;
int iNetLandLastSend = 0;
float fNetLandLastAy = 0.0;
int iNetLandLastFlags = -1;
int iNetLandLastHP = -1;
bool bNetLandLastMoving = false;
int iNetLandActSeq = 0;				// numbers of our attack packets
bool bNetLandFwdHeld = false;			// W / S of our hero are down (the others play the walk by it, not by guessing from the speed)
bool bNetLandBackHeld = false;
int iNetLandFwdAt = 0;
int iNetLandBackAt = 0;
int iNetLandHopAt = 0;
bool bNetLandLeftHeld = false;
bool bNetLandRightHeld = false;
int iNetLandCleanStart = 0;			// a click that began a swing when the hero was free: the time to its hit is measured from it
int iNetLandLastStrike = 0;
int iNetLandDFast = 350;			// ms from the start of a swing to its hit (learned from the swings of this player)
int iNetLandDForce = 350;
int iNetLandDRound = 350;
int iNetLandDBreak = 350;
int iNetLandDFeint = 350;
int iNetLandVFast = 0;				// the variant of the animation that the tables of our hero hold now
int iNetLandVForce = 0;
int iNetLandVFeint = 0;
int iNetLandVFeintC = 0;
int iNetLandDeadVar = 1;
int iNetLandShotAt = 0;
int iNetLandSwingPending = 0;		// a swing was announced and the hit of it has not come yet
bool bNetLandRunHeld = false;			// Shift (ChrRun) of our hero is down
int iNetLandSpeedLog = 0;
int iNetLandParryUntil = 0;			// Shift+RMB (ChrParry): a parry stands until then
int iNetLandFixN = 0;
int iNetLandLastStart = 0;			// the last swing was announced at its start (ChrAttackAction)
int iNetLandPupGen = 0;
bool bNetLandBlockHeld = false;		// SPACE (ChrBlock) of our hero is down
int iNetLandBlockedAt = 0;			// the game itself said (a puppet hits our hero) that the hit was blocked / not
bool bNetLandBlockedLast = false;
int iNetLandActCounter = 0;			// ids of the actions of the puppets (for the stop timer)			// the "no boarding" switch of the Create game screen means "no pistols" on land
object oNetLandHero;				// the single player hero before the duel (given back at the end)
object oNetLandLocSave;				// attributes of the location changed for the duel

// ------------------------------------------------------------------------------------------------
// common
// ------------------------------------------------------------------------------------------------
bool NetLand_IsActive()
{
	return bNetLandActive;
}

// the same model for a player on every machine. The icon of the player (portrait face_N, N = FaceId of
// characters_face.c / FaceMaker) is turned back into the model that FaceMaker gives that FaceId. All the portraits
// of INTERFACES/PORTRAITS have a model here. A pirate (by the slot of the player) is only for an icon that has none
// (a NET/FACES icon, the empty icon, a number without a model).
string NetLand_FaceToModel(int iFace)
{
	switch (iFace)
	{
		case 0: return "beatrice"; break;
		case 1: return "devlin"; break;
		case 2: return "barmen_1"; break;
		case 3: return "barmen_2"; break;
		case 4: return "barmen_3"; break;
		case 5: return "barmen_4"; break;
		case 6: return "barmen_5"; break;
		case 7: return "barmen_6"; break;
		case 8: return "barmen_7"; break;
		case 9: return "barmen_8"; break;
		case 10: return "barmen_9"; break;
		case 11: return "barmen_10"; break;
		case 12: return "barmen_11"; break;
		case 13: return "barmen_12"; break;
		case 14: return "barmen_13"; break;
		case 15: return "barmen_14"; break;
		case 16: return "barmen_15"; break;
		case 17: return "barmen_16"; break;
		case 18: return "officer_1"; break;
		case 19: return "officer_2"; break;
		case 20: return "officer_3"; break;
		case 21: return "officer_4"; break;
		case 22: return "officer_5"; break;
		case 23: return "officer_6"; break;
		case 24: return "officer_7"; break;
		case 25: return "officer_8"; break;
		case 26: return "officer_9"; break;
		case 27: return "officer_10"; break;
		case 28: return "officer_11"; break;
		case 29: return "officer_12"; break;
		case 30: return "officer_13"; break;
		case 31: return "officer_14"; break;
		case 32: return "officer_15"; break;
		case 33: return "officer_16"; break;
		case 34: return "officer_17"; break;
		case 35: return "officer_18"; break;
		case 36: return "officer_19"; break;
		case 37: return "officer_20"; break;
		case 38: return "citiz_1"; break;
		case 39: return "citiz_2"; break;
		case 40: return "citiz_3"; break;
		case 41: return "citiz_4"; break;
		case 42: return "citiz_5"; break;
		case 43: return "citiz_6"; break;
		case 44: return "citiz_7"; break;
		case 45: return "citiz_8"; break;
		case 46: return "citiz_9"; break;
		case 47: return "citiz_10"; break;
		case 48: return "citiz_11"; break;
		case 49: return "citiz_12"; break;
		case 50: return "girl_1"; break;
		case 51: return "girl_2"; break;
		case 52: return "girl_3"; break;
		case 53: return "girl_4"; break;
		case 54: return "girl_5"; break;
		case 55: return "girl_6"; break;
		case 56: return "girl_7"; break;
		case 57: return "girl_8"; break;
		case 58: return "head_pir"; break;
		case 59: return "huber_eng"; break;
		case 60: return "huber_fra"; break;
		case 61: return "huber_spa"; break;
		case 62: return "huber_hol"; break;
		case 63: return "lady_1"; break;
		case 64: return "lady_2"; break;
		case 65: return "lady_3"; break;
		case 66: return "lady_4"; break;
		case 67: return "lady_5"; break;
		case 68: return "lady_6"; break;
		case 69: return "off_eng_1"; break;
		case 70: return "off_eng_2"; break;
		case 71: return "off_fra_1"; break;
		case 72: return "off_fra_2"; break;
		case 73: return "off_spa_1"; break;
		case 74: return "off_spa_2"; break;
		case 75: return "off_hol_1"; break;
		case 76: return "off_hol_2"; break;
		case 77: return "pirate_1"; break;
		case 78: return "pirate_2"; break;
		case 79: return "pirate_3"; break;
		case 80: return "pirate_4"; break;
		case 81: return "pirate_5"; break;
		case 82: return "pirate_6"; break;
		case 83: return "pirate_7"; break;
		case 84: return "pirate_8"; break;
		case 85: return "pirate_9"; break;
		case 86: return "pirate_10"; break;
		case 87: return "priest_1"; break;
		case 88: return "priest_2"; break;
		case 89: return "priest_3"; break;
		case 90: return "priest_4"; break;
		case 91: return "shipowner_1"; break;
		case 92: return "shipowner_2"; break;
		case 93: return "shipowner_3"; break;
		case 94: return "shipowner_4"; break;
		case 95: return "shipowner_5"; break;
		case 96: return "shipowner_6"; break;
		case 97: return "shipowner_7"; break;
		case 98: return "shipowner_8"; break;
		case 99: return "shipowner_9"; break;
		case 100: return "shipowner_10"; break;
		case 101: return "shipowner_11"; break;
		case 102: return "shipowner_12"; break;
		case 103: return "shipowner_13"; break;
		case 104: return "shipowner_14"; break;
		case 105: return "shipowner_15"; break;
		case 106: return "shipowner_16"; break;
		case 107: return "smuggler_boss"; break;
		case 108: return "sold_hol_1"; break;
		case 109: return "sold_hol_2"; break;
		case 110: return "sold_hol_3"; break;
		case 111: return "sold_hol_4"; break;
		case 112: return "sold_hol_5"; break;
		case 113: return "sold_hol_6"; break;
		case 114: return "sold_hol_7"; break;
		case 115: return "sold_hol_8"; break;
		case 116: return "sold_eng_1"; break;
		case 117: return "sold_eng_2"; break;
		case 118: return "sold_eng_3"; break;
		case 119: return "sold_eng_4"; break;
		case 120: return "sold_eng_5"; break;
		case 121: return "sold_eng_6"; break;
		case 122: return "sold_eng_7"; break;
		case 123: return "sold_eng_8"; break;
		case 124: return "sold_fra_1"; break;
		case 125: return "sold_fra_2"; break;
		case 126: return "sold_fra_3"; break;
		case 127: return "sold_fra_4"; break;
		case 128: return "sold_fra_5"; break;
		case 129: return "sold_fra_6"; break;
		case 130: return "sold_fra_7"; break;
		case 131: return "sold_fra_8"; break;
		case 133: return "sold_spa_2"; break;
		case 134: return "sold_spa_3"; break;
		case 135: return "sold_spa_4"; break;
		case 136: return "sold_spa_5"; break;
		case 137: return "sold_spa_6"; break;
		case 138: return "sold_spa_7"; break;
		case 139: return "sold_spa_8"; break;
		case 140: return "trader_1"; break;
		case 141: return "trader_2"; break;
		case 142: return "trader_3"; break;
		case 143: return "trader_4"; break;
		case 144: return "trader_5"; break;
		case 145: return "trader_6"; break;
		case 146: return "trader_7"; break;
		case 147: return "trader_8"; break;
		case 148: return "trader_9"; break;
		case 149: return "trader_10"; break;
		case 150: return "trader_11"; break;
		case 151: return "trader_12"; break;
		case 152: return "trader_13"; break;
		case 153: return "trader_14"; break;
		case 154: return "trader_15"; break;
		case 155: return "trader_16"; break;
		case 156: return "usurer_1"; break;
		case 157: return "usurer_2"; break;
		case 158: return "usurer_3"; break;
		case 159: return "usurer_4"; break;
		case 160: return "usurer_5"; break;
		case 161: return "usurer_6"; break;
		case 162: return "usurer_7"; break;
		case 163: return "usurer_8"; break;
		case 164: return "whore_1"; break;
		case 165: return "whore_2"; break;
		case 166: return "whore_3"; break;
		case 167: return "whore_4"; break;
		case 168: return "Researcher"; break;
		case 170: return "Daniel"; break;
		case 198: return "Corsair1_2"; break;
		case 199: return "AnnaDeLeiva"; break;
		case 200: return "Albermal"; break;
		case 201: return "Corsair1_3"; break;
		case 202: return "capitan"; break;
		case 203: return "pirat00"; break;
		case 204: return "MorrisWillams"; break;
		case 205: return "ThomasLinch"; break;
		case 206: return "Korsar1"; break;
		case 207: return "mummy"; break;
		case 208: return "liza"; break;
		case 209: return "bocman"; break;
		case 210: return "BigPirate"; break;
		case 211: return "chard"; break;
		case 212: return "chard2"; break;
		case 213: return "chard3"; break;
		case 214: return "Espinosa"; break;
		case 215: return "Hero1"; break;
		case 216: return "Isterling"; break;
		case 217: return "Morgan"; break;
		case 218: return "SpaOfficer1"; break;
		case 219: return "SpaOfficer2"; break;
		case 220: return "Youngman"; break;
		case 221: return "Blood5"; break;
		case 222: return "Mechanic"; break;
		case 223: return "capitan_2"; break;
		case 224: return "capitan_3"; break;
		case 225: return "MusketeerEnglish_1"; break;
		case 226: return "Dragoon_1"; break;
		case 227: return "Hobart"; break;
		case 228: return "Sharp"; break;
		case 229: return "SoldierEnglish_1"; break;
		case 230: return "Pitt"; break;
		case 231: return "skeletcap"; break;
		case 232: return "girl_9"; break;
		case 233: return "girl_10"; break;
		case 234: return "Chard3a"; break;
		case 235: return "Danielle"; break;
		case 242: return "Angellica"; break;
		case 243: return "YokoDias"; break;
		case 244: return "Jessika"; break;
		case 281: return "ozg_carlos"; break;
		case 282: return "ozg_horn"; break;
		case 283: return "ozg_piRat"; break;
		case 284: return "ozg_miko"; break;
		case 285: return "ozg_martin"; break;
		case 290: return "QuestCaptain"; break;
		case 291: return "OffOrion"; break;
		case 292: return "quest_mush_1"; break;
		case 293: return "quest_mush_2"; break;
	}
	return "";
}

// "face_222" / "face_222.tga" -> 222, -1 if it is not a portrait of the game
int NetLand_FaceNum(string sFace)
{
	if (findSubStr(sFace, "face_", 0) != 0) { return -1; }
	if (strlen(sFace) <= 5) { return -1; }
	string sNum = strcut(sFace, 5, strlen(sFace) - 1);
	int iDot = findSubStr(sNum, ".", 0);
	if (iDot == 0) { return -1; }
	if (iDot > 0) { sNum = strcut(sNum, 0, iDot - 1); }
	if (sNum == "") { return -1; }
	for (int i=0; i<strlen(sNum); i++)
	{
		string sCh = strcut(sNum, i, i);
		if (findSubStr("0123456789", sCh, 0) < 0) { return -1; }
	}
	return sti(sNum);
}

bool NetLand_ClientMusket(int wID)
{
	string sGun = "";
	if (wID == wClientID)
	{
		if (CheckAttribute(&oNetLandLo, "Gun")) { sGun = oNetLandLo.Gun; }
	}
	else
	{
		if (CheckAttribute(&NCClients[wID], "LandLo.Gun")) { sGun = NCClients[wID].LandLo.Gun; }
	}
	if (sGun == "mushket") { return true; }
	return false;
}

bool NetLand_ModelIsWoman(string sModel)
{
	if (findSubStr(sModel, "girl_", 0) == 0) { return true; }
	if (findSubStr(sModel, "lady_", 0) == 0) { return true; }
	if (findSubStr(sModel, "whore_", 0) == 0) { return true; }
	if (findSubStr(sModel, "horse0", 0) == 0) { return true; }
	if (sModel == "beatrice" || sModel == "beatriceA" || sModel == "liza" || sModel == "AnnaDeLeiva") { return true; }
	if (sModel == "Angellica" || sModel == "YokoDias" || sModel == "Jessika" || sModel == "Danielle") { return true; }
	return false;
}

// sex of the character for the model: man / woman / skeleton
string NetLand_ModelSex(string sModel)
{
	if (NetLand_ModelIsWoman(sModel)) { return "woman"; }
	if (sModel == "mummy" || sModel == "skeletcap") { return "skeleton"; }
	return "man";
}

// animation: the musketeers fight with the blade and the pistol here, so they get the animation of a man
string NetLand_ModelAnim(string sModel)
{
	if (findSubStr(sModel, "mushketer_", 0) == 0) { return "mushketer"; }
	string sSex = NetLand_ModelSex(sModel);
	if (sSex == "woman") { return "woman"; }
	if (sSex == "skeleton") { return "skeleton"; }
	return "man";
}

float NetLand_ModelHeight(string sModel)
{
	if (NetLand_ModelSex(sModel) == "woman") { return 1.75; }
	return 1.8;
}

string NetLand_ModelFor(int wID)
{
	if (CheckAttribute(&NCClients[wID], "LandModel")) { return NCClients[wID].LandModel; }

	int iN = wID - (wID / 10) * 10 + 1;
	string sModel = "pirate_" + iN;

	string sFace = "";
	if (wID == wClientID && CheckAttribute(&NetClient, "FaceImage")) { sFace = NetClient.FaceImage; }
	if (sFace == "" && CheckAttribute(&NCClients[wID], "UserData.Face")) { sFace = NCClients[wID].UserData.Face; }

	int iFace = NetLand_FaceNum(sFace);
	if (iFace >= 0)
	{
		string sByFace = NetLand_FaceToModel(iFace);
		if (sByFace != "") { sModel = sByFace; }
	}
	// a musket: the musketeer of the game (the musket needs the animation "mushketer", see EquipCharacterByItem)
	if (NetLand_ClientMusket(wID))
	{
		int iMN = wID - (wID / 5) * 5 + 1;
		sModel = "mushketer_" + iMN;
	}
	Trace("COOP: land model of " + wID + ": face <" + sFace + "> -> " + sModel);
	NCClients[wID].LandModel = sModel;
	return sModel;
}


// the exact animation of the swing: iVar is the variant that the player's game plays (1..4)
string NetLand_ActName(int iAct, int iVar)
{
	if (iVar < 1) { iVar = 1; }
	switch (iAct)
	{
		case NET_LANDACT_FAST:	return "attack_fast_" + iVar; break;
		case NET_LANDACT_FORCE:	return "attack_force_" + iVar; break;
		case NET_LANDACT_ROUND:	return "attack_round_1"; break;
		case NET_LANDACT_BREAK:	return "attack_break_1"; break;
		case NET_LANDACT_FEINT:	return "attack_feint_" + iVar; break;
		case NET_LANDACT_FEINTC:	return "attack_feintc_" + iVar; break;
		case NET_LANDACT_SHOT:	return "Shot"; break;
		case NET_LANDACT_PARRY:	return "parry_" + iVar; break;
		case NET_LANDACT_RECOIL:	return "recoil"; break;
		case NET_LANDACT_STRAFEL:	return "straif_weapon_left"; break;
		case NET_LANDACT_STRAFER:	return "straif_weapon_right"; break;
	}
	return "";
}

// how many variants of the animation the game has for the swing
int NetLand_ActVariants(int iAct)
{
	switch (iAct)
	{
		case NET_LANDACT_FAST:	return 3; break;
		case NET_LANDACT_FORCE:	return 4; break;
		case NET_LANDACT_FEINT:	return 4; break;
		case NET_LANDACT_FEINTC:	return 4; break;
		case NET_LANDACT_PARRY:	return 4; break;
	}
	return 1;
}

// All the variants of the action of our hero are set to the one that is played now (characters.c: SetDefaultFight):
// the game cannot choose another one and the puppets of the others get the number of this variant.
void NetLand_SetVariant(ref rA, int iAct, int iVar)
{
	string sN;
	switch (iAct)
	{
		case NET_LANDACT_FAST:
			sN = "attack_fast_" + iVar;
			rA.actions.attack_fast.a1 = sN;
			rA.actions.attack_fast.a2 = sN;
			rA.actions.attack_fast.a3 = sN;
		break;
		case NET_LANDACT_FORCE:
			sN = "attack_force_" + iVar;
			rA.actions.attack_force.a1 = sN;
			rA.actions.attack_force.a2 = sN;
			rA.actions.attack_force.a3 = sN;
			rA.actions.attack_force.a4 = sN;
		break;
		case NET_LANDACT_FEINT:
			sN = "attack_feint_" + iVar;
			rA.actions.attack_feint.a1 = sN;
			rA.actions.attack_feint.a2 = sN;
			rA.actions.attack_feint.a3 = sN;
			rA.actions.attack_feint.a4 = sN;
		break;
		case NET_LANDACT_FEINTC:
			sN = "attack_feintc_" + iVar;
			rA.actions.attack_feintc.a1 = sN;
			rA.actions.attack_feintc.a2 = sN;
			rA.actions.attack_feintc.a3 = sN;
			rA.actions.attack_feintc.a4 = sN;
		break;
		case NET_LANDACT_PARRY:
			sN = "parry_" + iVar;
			rA.actions.parry.p1 = sN;
			rA.actions.parry.p2 = sN;
			rA.actions.parry.p3 = sN;
			rA.actions.parry.p4 = sN;
		break;
	}
	// the game reads the tables of the animations again only when the node "actions" itself is written
	// (EndChangeCharacterActions of characters.c does the same)
	rA.actions = "";
}

int NetLand_GetDelay(int iCode)
{
	switch (iCode)
	{
		case NET_LANDACT_FAST:	return iNetLandDFast; break;
		case NET_LANDACT_FORCE:	return iNetLandDForce; break;
		case NET_LANDACT_ROUND:	return iNetLandDRound; break;
		case NET_LANDACT_BREAK:	return iNetLandDBreak; break;
		case NET_LANDACT_FEINT:	return iNetLandDFeint; break;
		case NET_LANDACT_FEINTC:	return iNetLandDFeint; break;
	}
	return 0;
}

void NetLand_LearnDelay(int iCode, int iMs)
{
	switch (iCode)
	{
		case NET_LANDACT_FAST:	iNetLandDFast = (iNetLandDFast * 2 + iMs) / 3; break;
		case NET_LANDACT_FORCE:	iNetLandDForce = (iNetLandDForce * 2 + iMs) / 3; break;
		case NET_LANDACT_ROUND:	iNetLandDRound = (iNetLandDRound * 2 + iMs) / 3; break;
		case NET_LANDACT_BREAK:	iNetLandDBreak = (iNetLandDBreak * 2 + iMs) / 3; break;
		case NET_LANDACT_FEINT:	iNetLandDFeint = (iNetLandDFeint * 2 + iMs) / 3; break;
		case NET_LANDACT_FEINTC:	iNetLandDFeint = (iNetLandDFeint * 2 + iMs) / 3; break;
	}
}

int NetLand_LoadedVar(int iCode)
{
	switch (iCode)
	{
		case NET_LANDACT_FAST:	return iNetLandVFast; break;
		case NET_LANDACT_FORCE:	return iNetLandVForce; break;
		case NET_LANDACT_FEINT:	return iNetLandVFeint; break;
		case NET_LANDACT_FEINTC:	return iNetLandVFeintC; break;
	}
	return 1;
}

void NetLand_SetLoadedVar(int iCode, int iVar)
{
	switch (iCode)
	{
		case NET_LANDACT_FAST:	iNetLandVFast = iVar; break;
		case NET_LANDACT_FORCE:	iNetLandVForce = iVar; break;
		case NET_LANDACT_FEINT:	iNetLandVFeint = iVar; break;
		case NET_LANDACT_FEINTC:	iNetLandVFeintC = iVar; break;
	}
}

// the death of the hero: the game plays one of death_0..3 at random, so the variant is chosen by the player who dies
void NetLand_SetDeathVariant(ref rD, int iVar)
{
	int iDeathNum = iVar - 1;
	string sN = "death_" + iDeathNum;
	rD.actions.fightdead.d1 = sN;
	rD.actions.fightdead.d2 = sN;
	rD.actions.fightdead.d3 = sN;
	rD.actions.fightdead.d4 = sN;
	rD.actions = "";
}

int NetLand_ActCode(string sAttackType)
{
	if (sAttackType == "fast") { return NET_LANDACT_FAST; }
	if (sAttackType == "force") { return NET_LANDACT_FORCE; }
	if (sAttackType == "round") { return NET_LANDACT_ROUND; }
	if (sAttackType == "break") { return NET_LANDACT_BREAK; }
	if (sAttackType == "feint") { return NET_LANDACT_FEINT; }
	if (sAttackType == "feintc") { return NET_LANDACT_FEINTC; }
	if (bNetLandActive) { Trace("COOP: land action type <" + sAttackType + "> is not sent"); }
	return 0;
}

// ================================================================================================
// SERVER
// ================================================================================================
bool NetServer_LandMode()
{
	if (!CheckAttribute(&NetServer, "LandLoc")) { return false; }
	return NetServer.LandLoc != "";
}

void NetServer_LandSendStart(int wID, int iSpawn)
{
	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_LAND);
	NMAddByte(iSMsg, NSC_LAND_START);
	NMAddString(iSMsg, NetServer.LandLoc, 32);
	NMAddByte(iSMsg, iSpawn);
	NMAddByte(iSMsg, sti(NetServer.Respawn));
	NMAddByte(iSMsg, sti(NetServer.RespawnTime));
	NMAddByte(iSMsg, sti(NetServer.NoBoarding));		// on land: no pistols
	NetServer_SendMessage(wID, iSMsg, true);
	NMDelete(iSMsg);
}

void NetServer_LandInitSlot(int i, int iSpawn)
{
	NSClients[i].InGame = true;
	NSClients[i].Reconnect = false;
	NSClients[i].JoinRequested = false;
	NSClients[i].LandDead = false;
	NSClients[i].LandSpawn = iSpawn;
	if (!CheckAttribute(&NSClients[i], "LandKills")) { NSClients[i].LandKills = 0; }
	if (!CheckAttribute(&NSClients[i], "LandDeaths")) { NSClients[i].LandDeaths = 0; }
}

// NetServer_StartServerGame in the land mode: no sea, every player gets the location and a spawn point
void NetServer_LandStart()
{
	int iSpawn = 0;
	NetServer.LandAlivePeak = 0;
	SetEventHandler("NetServer_LandRespawn", "NetServer_LandRespawn", 0);
	for (int i=0; i<NET_MAXPLAYERS; i++)
	{
		if (!sti(NSClients[i].Use)) { continue; }
		NSClients[i].LandKills = 0;
		NSClients[i].LandDeaths = 0;
		DeleteAttribute(&NSClients[i], "LandLo");
		NetServer_LandInitSlot(i, iSpawn);
		NetServer_LandSendStart(i, iSpawn);
		iSpawn++;
	}
	NetServer.LandNextSpawn = iSpawn;
	Trace("COOP: land duel started in " + NetServer.LandLoc + ", players " + iSpawn);
}

// a player who comes into the running duel (the buy screen of the running game, reconnect)
void NetServer_LandJoin(int wID)
{
	int iSpawn = sti(NetServer.LandNextSpawn);
	NetServer.LandNextSpawn = iSpawn + 1;
	NetServer_LandInitSlot(wID, iSpawn);
	NetServer_LandSendStart(wID, iSpawn);
	NetServer_LandSendLoadoutsTo(wID);
	Trace("COOP: land duel join id=" + wID);
}

int NetServer_LandLoadoutMsg(int wID)
{
	aref lo;
	makearef(lo, NSClients[wID].LandLo);
	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_LAND);
	NMAddByte(iSMsg, NSC_LAND_LOADOUT_R);
	NMAddClientID(iSMsg, wID);
	NetLand_AddLoadoutData(iSMsg, lo);
	return iSMsg;
}

// the loadouts of the others to a player who has just come
void NetServer_LandSendLoadoutsTo(int wTo)
{
	for (int i=0; i<NET_MAXPLAYERS; i++)
	{
		if (i == wTo) { continue; }
		if (!sti(NSClients[i].Use) || !sti(NSClients[i].InGame)) { continue; }
		if (!CheckAttribute(&NSClients[i], "LandLo.Blade")) { continue; }
		int iSMsg = NetServer_LandLoadoutMsg(i);
		NetServer_SendMessage(wTo, iSMsg, true);
		NMDelete(iSMsg);
	}
}

void NetServer_LandEnd()
{
	DelEventHandler("NetServer_LandRespawn", "NetServer_LandRespawn");
}

// to every player of the duel except one
void NetServer_LandSendOthers(int wExcept, int iSMsg, bool bGaranted)
{
	for (int i=0; i<NET_MAXPLAYERS; i++)
	{
		if (i == wExcept) { continue; }
		if (!sti(NSClients[i].Use)) { continue; }
		if (!sti(NSClients[i].InGame)) { continue; }
		NetServer_SendMessage(i, iSMsg, bGaranted);
	}
}

void NetServer_OnLand(int wID, int iMsg)
{
	int iSMsg;
	if (!bServerGameStarted || !NetServer_LandMode()) { return; }
	if (wID < 0 || wID >= NET_MAXPLAYERS) { return; }
	if (!sti(NSClients[wID].Use)) { return; }

	int iSub = NMGetByte(iMsg);
	switch (iSub)
	{
		case NSC_LAND_STATE:
			float fX = NMGetFloat(iMsg);
			float fY = NMGetFloat(iMsg);
			float fZ = NMGetFloat(iMsg);
			float fAy = NMGetFloat(iMsg);
			int iFlags = NMGetByte(iMsg);
			int iHP = NMGetWord(iMsg);
			int iHPMax = NMGetWord(iMsg);
			float fVX = NMGetFloat(iMsg);
			float fVZ = NMGetFloat(iMsg);

			iSMsg = NMCreate();
			NMAddByte(iSMsg, NC_LAND);
			NMAddByte(iSMsg, NSC_LAND_STATE_R);
			NMAddClientID(iSMsg, wID);
			NMAddFloat(iSMsg, fX);
			NMAddFloat(iSMsg, fY);
			NMAddFloat(iSMsg, fZ);
			NMAddFloat(iSMsg, fAy);
			NMAddByte(iSMsg, iFlags);
			NMAddWord(iSMsg, iHP);
			NMAddWord(iSMsg, iHPMax);
			NMAddFloat(iSMsg, fVX);
			NMAddFloat(iSMsg, fVZ);
			NetServer_LandSendOthers(wID, iSMsg, false);
			NMDelete(iSMsg);
		break;

		case NSC_LAND_ACT:
			int iAct = NMGetByte(iMsg);
			int iActSeq = NMGetByte(iMsg);
			iSMsg = NMCreate();
			NMAddByte(iSMsg, NC_LAND);
			NMAddByte(iSMsg, NSC_LAND_ACT_R);
			NMAddClientID(iSMsg, wID);
			NMAddByte(iSMsg, iAct);
			NMAddByte(iSMsg, iActSeq);
			NetServer_LandSendOthers(wID, iSMsg, false);
			NMDelete(iSMsg);
		break;

		case NSC_LAND_HIT:
			int wVictim = NMGetClientID(iMsg);
			int iDamage = NMGetWord(iMsg);
			int iDamageB = NMGetWord(iMsg);
			int iHitDelay = NMGetWord(iMsg);
			int iKind = NMGetByte(iMsg);
			if (wVictim < 0 || wVictim >= NET_MAXPLAYERS || wVictim == wID) { return; }
			if (!sti(NSClients[wVictim].Use) || !sti(NSClients[wVictim].InGame)) { return; }
			if (sti(NSClients[wVictim].LandDead) || sti(NSClients[wID].LandDead)) { return; }
			iSMsg = NMCreate();
			NMAddByte(iSMsg, NC_LAND);
			NMAddByte(iSMsg, NSC_LAND_HIT_R);
			NMAddClientID(iSMsg, wID);
			NMAddWord(iSMsg, iDamage);
			NMAddWord(iSMsg, iDamageB);
			NMAddWord(iSMsg, iHitDelay);
			NMAddByte(iSMsg, iKind);
			NetServer_SendMessage(wVictim, iSMsg, true);
			NMDelete(iSMsg);
		break;

		case NSC_LAND_ANIM:
			string sAnimA0 = NMGetString(iMsg);
			string sAnimA1 = NMGetString(iMsg);
			string sAnimA2 = NMGetString(iMsg);
			iSMsg = NMCreate();
			NMAddByte(iSMsg, NC_LAND);
			NMAddByte(iSMsg, NSC_LAND_ANIM_R);
			NMAddClientID(iSMsg, wID);
			NMAddString(iSMsg, sAnimA0, 24);
			NMAddString(iSMsg, sAnimA1, 24);
			NMAddString(iSMsg, sAnimA2, 24);
			NetServer_LandSendOthers(wID, iSMsg, false);
			NMDelete(iSMsg);
		break;

		case NSC_LAND_HITRES:
			int wResAtt = NMGetClientID(iMsg);
			int iRes = NMGetByte(iMsg);
			if (wResAtt < 0 || wResAtt >= NET_MAXPLAYERS || wResAtt == wID) { return; }
			if (!sti(NSClients[wResAtt].Use) || !sti(NSClients[wResAtt].InGame)) { return; }
			iSMsg = NMCreate();
			NMAddByte(iSMsg, NC_LAND);
			NMAddByte(iSMsg, NSC_LAND_HITRES_R);
			NMAddClientID(iSMsg, wID);
			NMAddByte(iSMsg, iRes);
			NetServer_SendMessage(wResAtt, iSMsg, true);
			NMDelete(iSMsg);
		break;

		case NSC_LAND_DEAD:
			int wKiller = NMGetClientID(iMsg);
			int iDeadVar = NMGetByte(iMsg);
			NetServer_LandDead(wID, wKiller, iDeadVar);
		break;

		case NSC_LAND_LOADOUT:
			// kept for the players who come later, sent to the others now
			DeleteAttribute(&NSClients[wID], "LandLo");
			NSClients[wID].LandLo = "";
			aref arSrvLo;
			makearef(arSrvLo, NSClients[wID].LandLo);
			NetLand_ReadLoadoutData(iMsg, arSrvLo);
			iSMsg = NetServer_LandLoadoutMsg(wID);
			NetServer_LandSendOthers(wID, iSMsg, true);
			NMDelete(iSMsg);
		break;
	}
}

void NetServer_LandDead(int wID, int wKiller, int iDeadVar)
{
	if (sti(NSClients[wID].LandDead)) { return; }
	NSClients[wID].LandDead = true;
	NSClients[wID].LandDeaths = sti(NSClients[wID].LandDeaths) + 1;

	int iKills = 0;
	if (wKiller >= 0 && wKiller < NET_MAXPLAYERS && wKiller != wID)
	{
		NSClients[wKiller].LandKills = sti(NSClients[wKiller].LandKills) + 1;
		iKills = sti(NSClients[wKiller].LandKills);
	}
	else
	{
		wKiller = DST_INVALID;
	}

	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_LAND);
	NMAddByte(iSMsg, NSC_LAND_DEAD_R);
	NMAddClientID(iSMsg, wID);
	NMAddClientID(iSMsg, wKiller);
	NMAddWord(iSMsg, iKills);
	NMAddByte(iSMsg, iDeadVar);
	NetServer_LandSendOthers(DST_INVALID, iSMsg, true);
	NMDelete(iSMsg);

	if (sti(NetServer.Respawn))
	{
		PostEvent("NetServer_LandRespawn", 1000 * sti(NetServer.RespawnTime), "ll", wID, sti(NetServer.Round));
		return;
	}
	NetServer_LandCheckOver();
}

// without respawn: the last one standing wins (like the deathmatch at sea); the game goes on until the players leave
void NetServer_LandCheckOver()
{
	if (sti(NetServer.GameOver)) { return; }
	int iAlive = 0;
	int iPlayers = 0;
	int wWinner = DST_INVALID;
	for (int i=0; i<NET_MAXPLAYERS; i++)
	{
		if (!sti(NSClients[i].Use)) { continue; }
		if (!sti(NSClients[i].InGame)) { continue; }
		iPlayers++;
		if (sti(NSClients[i].LandDead)) { continue; }
		iAlive++;
		wWinner = i;
	}
	if (iPlayers < 2 || iAlive > 1) { return; }

	NetServer.GameOver = true;
	int iDraw = 0;
	if (iAlive == 0) { iDraw = 1; wWinner = DST_INVALID; }

	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_STARTGAME);
	NMAddByte(iSMsg, NSC_STARTGAME_GAMEOVER);
	NMAddByte(iSMsg, NETGAME_DEATHMATCH);
	NMAddClientID(iSMsg, wWinner);
	NMAddByte(iSMsg, 0);
	NMAddByte(iSMsg, iDraw);
	NetServer_SendMessage(DST_ALL, iSMsg, true);
	NMDelete(iSMsg);
}

void NetServer_LandRespawn()
{
	int wID = GetEventData();
	int iRound = GetEventData();
	if (iRound != sti(NetServer.Round)) { return; }
	if (!bServerGameStarted || !NetServer_LandMode()) { return; }
	if (!sti(NSClients[wID].Use) || !sti(NSClients[wID].InGame)) { return; }

	NSClients[wID].LandDead = false;
	int iSpawn = sti(NetServer.LandNextSpawn);
	NetServer.LandNextSpawn = iSpawn + 1;

	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_LAND);
	NMAddByte(iSMsg, NSC_LAND_RESPAWN_R);
	NMAddClientID(iSMsg, wID);
	NMAddByte(iSMsg, iSpawn - (iSpawn / 250) * 250);
	NetServer_LandSendOthers(DST_INVALID, iSMsg, true);
	NMDelete(iSMsg);
}

// ================================================================================================
// CLIENT
// ================================================================================================
void NetClient_LandSend(int iSMsg, bool bGaranted)
{
	NetClient_SendMessage(iSMsg, bGaranted);
}

int NetClient_LandNewMsg(int iSub)
{
	int iSMsg = NMCreate();
	NMAddClientID(iSMsg, wClientID);
	NMAddByte(iSMsg, NC_LAND);
	NMAddByte(iSMsg, iSub);
	return iSMsg;
}

void NetClient_OnLand(int iMsg)
{
	int iSub = NMGetByte(iMsg);
	switch (iSub)
	{
		case NSC_LAND_START:		NetLand_OnStart(iMsg);		break;
		case NSC_LAND_STATE_R:		NetLand_OnState(iMsg);		break;
		case NSC_LAND_ACT_R:		NetLand_OnAct(iMsg);		break;
		case NSC_LAND_HIT_R:		NetLand_OnHit(iMsg);		break;
		case NSC_LAND_DEAD_R:		NetLand_OnDead(iMsg);		break;
		case NSC_LAND_HITRES_R:		NetLand_OnHitRes(iMsg);		break;
		case NSC_LAND_ANIM_R:		NetLand_OnAnim(iMsg);		break;
		case NSC_LAND_RESPAWN_R:	NetLand_OnRespawn(iMsg);	break;
		case NSC_LAND_LOADOUT_R:	NetLand_OnLoadout(iMsg);	break;
	}
}

// ---------- start / end ----------
void NetLand_OnStart(int iMsg)
{
	string sLoc = NMGetString(iMsg);
	int iSpawn = NMGetByte(iMsg);
	int iRespawn = NMGetByte(iMsg);
	int iRespawnSec = NMGetByte(iMsg);
	int iNoPistol = NMGetByte(iMsg);

	if (bNetLandActive) { return; }
	if (FindLocation(sLoc) < 0) { Trace("COOP: land duel: no location " + sLoc); return; }

	bNetLandActive = true;
	bNetLandDead = false;
	sNetLandLoc = sLoc;
	iNetLandSpawn = iSpawn;
	iNetLandRespawn = iRespawn;
	iNetLandRespawnSec = iRespawnSec;
	iNetLandNoPistol = iNoPistol;
	iNetLandGen++;
	for (int i=0; i<NET_MAXPLAYERS; i++) { DeleteAttribute(&NCClients[i], "LandModel"); DeleteAttribute(&NCClients[i], "LandLo"); }

	bNetClientInGame = true;
	NCServer.GameRunning = true;
	Event("NCEvent_StartGame");		// the buy screen closes

	SetEventHandler("NetLand_Load", "NetLand_Load", 0);
	SetEventHandler("NetLand_AfterLoad", "NetLand_AfterLoad", 0);
	SetEventHandler("NetLand_Tick", "NetLand_Tick", 0);
	SetEventHandler("NetLand_ActEnd", "NetLand_ActEnd", 0);
	SetEventHandler("NetLand_Smooth", "NetLand_Smooth", 0);
	SetEventHandler("NetLand_StaggerEnd", "NetLand_StaggerEnd", 0);
	SetEventHandler("NetLand_HitApply", "NetLand_HitApply", 0);
	SetEventHandler("NetLand_ActResend", "NetLand_ActResend", 0);
	SetEventHandler("Control Activation", "NetLand_CtrlOn", 0);
	SetEventHandler("Control Deactivation", "NetLand_CtrlOff", 0);
	bNetLandBlockHeld = false;
	bNetLandRunHeld = false;
	bNetLandFwdHeld = false;
	bNetLandBackHeld = false;
	bNetLandLeftHeld = false;
	bNetLandRightHeld = false;
	iNetLandSwingPending = 0;
	iNetLandBlockedAt = 0;
	iNetLandPT = 0;
	iNetLandLastFlags = -1;
	iNetLandLastHP = -1;
	bNetLandLastMoving = false;
	PostEvent("NetLand_Load", 300, "l", iNetLandGen);
	Trace("COOP: land duel start " + sLoc + " spawn " + iSpawn);
}

// our hero for the duel: a pirate with a blade and a pistol (the single player hero is given back at the end)
void NetLand_PrepareHero(ref mc)
{
	DeleteAttribute(&oNetLandHero, "");
	CopyAttributes(&oNetLandHero, mc);

	NetLand_LoInit();
	DeleteAttribute(&NCClients[wClientID], "LandModel");
	string sHeroModel = NetLand_ModelFor(wClientID);
	mc.model = sHeroModel;
	mc.model.animation = NetLand_ModelAnim(sHeroModel);
	mc.model.height = NetLand_ModelHeight(sHeroModel);
	mc.sex = NetLand_ModelSex(sHeroModel);
	if (CheckAttribute(&NetClient, "NickName")) { mc.name = NetClient.NickName; mc.lastname = ""; }

	DeleteAttribute(mc, "chr_ai.hp");
	aref arLo;
	makearef(arLo, oNetLandLo);
	NetLand_ApplyLo(mc, arLo, true, true);
	LAi_SetImmortal(mc, true);		// the death of the hero is ours (NetLand_LocalDie), never the game over of single player
}

void NetLand_Load()
{
	int iGen = GetEventData();
	if (iGen != iNetLandGen || !bNetLandActive) { return; }

	int iLoc = FindLocation(sNetLandLoc);
	if (iLoc < 0) { return; }
	ref rLoc = &Locations[iLoc];

	// a duel is not a boarding: the location makes its own weather and no sea (UnloadLocation deletes them again)
	// the first load changes the location for the duel and remembers it; a load of the hero after a respawn does not
	bool bBoardingLoc = CheckAttribute(rLoc, "boarding");
	if (!CheckAttribute(&oNetLandLocSave, "Index"))
	{
		DeleteAttribute(&oNetLandLocSave, "");
		oNetLandLocSave.Index = iLoc;
		if (bBoardingLoc) { oNetLandLocSave.boarding = rLoc.boarding; rLoc.boarding = "false"; }
		// a cabin: no sea behind the walls; a deck keeps its sea (without the coast foam of an island)
		if (CheckAttribute(rLoc, "CabinType") && CheckAttribute(rLoc, "environment.sea")) { oNetLandLocSave.sea = rLoc.environment.sea; rLoc.environment.sea = "false"; }
		if (!CheckAttribute(rLoc, "notCrateFoam")) { oNetLandLocSave.foam = true; rLoc.notCrateFoam = true; }
		if (CheckAttribute(rLoc, "environment.weather")) { oNetLandLocSave.weather = rLoc.environment.weather; }
		rLoc.environment.weather = "true";
		// churches, residences, shops...: a duel is a duel
		if (CheckAttribute(rLoc, "noFight")) { oNetLandLocSave.noFight = rLoc.noFight; rLoc.noFight = "0"; }
		bNetLandSavedReload = chrDisableReloadToLocation;
		oNetLandLocSave.reloadSaved = true;
	}
	chrDisableReloadToLocation = true;		// no door of a town leads out of the duel

	ref mc = GetMainCharacter();
	NetLand_PrepareHero(mc);

	string sGroup = "rld";
	string sLocator = "loc0";
	if (!bBoardingLoc)
	{
		sGroup = "reload";
		aref arReload;
		makearef(arReload, rLoc.reload);
		if (GetAttributesNum(arReload) > 0)
		{
			aref arFirst = GetAttributeN(arReload, 0);
			sLocator = arFirst.name;
		}
	}
	sNetLandEntryGroup = sGroup;
	sNetLandEntryLocator = sLocator;
	Trace("COOP: land duel loads " + sNetLandLoc + " at " + sGroup + "." + sLocator);
	LoadMainCharacterInFirstLocationGroup(sNetLandLoc, sGroup, sLocator);
	PostEvent("NetLand_AfterLoad", 700, "l", iNetLandGen);
}

// the n-th spawn point of the loaded location ("rld" of a cabin / deck, "reload" or "goto" otherwise)
bool NetLand_TeleportToSpawn(ref chr, int iSpawn)
{
	if (!IsEntity(loadedLocation)) { return false; }
	string sGroup = "rld";
	if (!CheckAttribute(loadedLocation, "locators.rld")) { sGroup = "goto"; }
	if (!CheckAttribute(loadedLocation, "locators." + sGroup)) { sGroup = "reload"; }
	if (!CheckAttribute(loadedLocation, "locators." + sGroup)) { return false; }

	aref arGroup;
	makearef(arGroup, loadedLocation.locators.(sGroup));
	int iNum = GetAttributesNum(arGroup);
	if (iNum <= 0) { return false; }
	aref arLoc = GetAttributeN(arGroup, iSpawn - (iSpawn / iNum) * iNum);
	return TeleportCharacterToPosAy(chr, stf(arLoc.x), stf(arLoc.y), stf(arLoc.z), 0.0);
}

void NetLand_AfterLoad()
{
	int iGen = GetEventData();
	if (iGen != iNetLandGen || !bNetLandActive) { return; }

	ref mc = GetMainCharacter();
	LAi_SetPlayerType(mc);
	LAi_LockFightMode(mc, false);		// a residence locks the fight mode when it is loaded
	LAi_group_SetRelation(NET_LAND_GROUP, LAI_GROUP_PLAYER, LAI_GROUP_ENEMY);
	NetLand_TeleportToSpawn(mc, iNetLandSpawn);

	Log_SetStringToLog(LanguageConvertString(iLangNetClient, "Net_LandStart"));
	PostEvent("NetLand_Tick", NET_LAND_TICK, "l", iNetLandGen);
	PostEvent("NetLand_Smooth", NET_LAND_SMOOTH, "l", iNetLandGen);
	NetLand_SendLoadout();
	iNetLandProtectUntil = Net_GetTickCount() + 90000;		// ends when the hero has applied his equipment or has begun to fight
	bNetLandOpenLoadout = true;		// the Esc menu opens the loadout window the first time
	SetEventHandler("NetLand_OpenMenu", "NetLand_OpenMenu", 0);
	PostEvent("NetLand_OpenMenu", 1500, "l", iNetLandGen);
}

// the loadout window at the start (once)
void NetLand_OpenMenu()
{
	int iGen = GetEventData();
	DelEventHandler("NetLand_OpenMenu", "NetLand_OpenMenu");
	if (iGen != iNetLandGen || !bNetLandActive) { return; }
	if (sti(InterfaceStates.Launched)) { return; }
	LaunchNetMenu();
}

bool NetLand_TakeOpenLoadout()
{
	if (!bNetLandActive || !bNetLandOpenLoadout) { return false; }
	bNetLandOpenLoadout = false;
	return true;
}

// client\OnStart.c: NetClient_OnEndGame (leave, disconnect, end of the game)
void NetLand_ClientStop()
{
	if (!bNetLandActive) { return; }
	bNetLandActive = false;
	bNetLandDead = false;
	iNetLandGen++;
	DelEventHandler("NetLand_Load", "NetLand_Load");
	DelEventHandler("NetLand_AfterLoad", "NetLand_AfterLoad");
	DelEventHandler("NetLand_Tick", "NetLand_Tick");
	DelEventHandler("NetLand_ActEnd", "NetLand_ActEnd");
	DelEventHandler("NetLand_Smooth", "NetLand_Smooth");
	DelEventHandler("NetLand_StaggerEnd", "NetLand_StaggerEnd");
	DelEventHandler("NetLand_HitApply", "NetLand_HitApply");
	DelEventHandler("NetLand_ActResend", "NetLand_ActResend");
	DelEventHandler("Control Activation", "NetLand_CtrlOn");
	DelEventHandler("Control Deactivation", "NetLand_CtrlOff");
	bNetLandBlockHeld = false;

	// the puppets leave the location
	for (int i=0; i<NET_MAXPLAYERS; i++)
	{
		if (!CheckAttribute(&NCClients[i], "LandChr")) { continue; }
		int iChr = sti(NCClients[i].LandChr);
		DeleteAttribute(&NCClients[i], "LandChr");
		if (iChr < 0) { continue; }
		ChangeCharacterAddressGroup(&Characters[iChr], "none", "", "");
	}

	if (IsEntity(loadedLocation)) { UnloadLocation(loadedLocation); }

	// the location and the single player hero as they were
	if (CheckAttribute(&oNetLandLocSave, "Index"))
	{
		ref rLoc = &Locations[sti(oNetLandLocSave.Index)];
		if (CheckAttribute(&oNetLandLocSave, "boarding")) { rLoc.boarding = oNetLandLocSave.boarding; }
		if (CheckAttribute(&oNetLandLocSave, "sea")) { rLoc.environment.sea = oNetLandLocSave.sea; }
		if (CheckAttribute(&oNetLandLocSave, "weather")) { rLoc.environment.weather = oNetLandLocSave.weather; }
		if (CheckAttribute(&oNetLandLocSave, "foam")) { DeleteAttribute(rLoc, "notCrateFoam"); }
		if (CheckAttribute(&oNetLandLocSave, "noFight")) { rLoc.noFight = oNetLandLocSave.noFight; }
		if (CheckAttribute(&oNetLandLocSave, "reloadSaved")) { chrDisableReloadToLocation = bNetLandSavedReload; }
	}
	DeleteAttribute(&oNetLandLocSave, "");

	if (CheckAttribute(&oNetLandHero, "index"))
	{
		ref mc = GetMainCharacter();
		DeleteAttribute(mc, "");
		CopyAttributes(mc, &oNetLandHero);
	}
	DeleteAttribute(&oNetLandHero, "");
	Trace("COOP: land duel stopped");
}

// ---------- our hero ----------
// Location.dll (patched): message "GetAnim" returns the name of the action that player iPlayer (0..7) of the animation
// of the character plays now; 0 = nothing plays / the dll is not patched
string NetLand_GetAnim(ref chr, int iPlayer)
{
	string sAct = "";
	int iOk = SendMessage(chr, "lsle", MSG_CHARACTER_EX_MSG, "GetAnim", iPlayer, &sAct);
	if (iOk == 0) { return ""; }
	bNetLandAnimOk = true;
	return sAct;
}

// our hero: what do the players 0, 1, 2 of his animation play? A change (and every 250 ms) goes to the others.
void NetLand_AnimSample(ref mc, int iNow)
{
	string sA0 = NetLand_GetAnim(mc, 0);
	string sA1 = NetLand_GetAnim(mc, 1);
	string sA2 = NetLand_GetAnim(mc, 2);
	if (!bNetLandAnimOk) { return; }
	bool bChanged = false;
	if (sA0 != sNetLandAnimSent0 || sA1 != sNetLandAnimSent1 || sA2 != sNetLandAnimSent2) { bChanged = true; }
	int iSince = iNow - iNetLandAnimSentAt;
	if (!bChanged && iSince < 250) { return; }
	if (bChanged) { Trace("COOP: land anim " + sA0 + " | " + sA1 + " | " + sA2); }
	sNetLandAnimSent0 = sA0;
	sNetLandAnimSent1 = sA1;
	sNetLandAnimSent2 = sA2;
	iNetLandAnimSentAt = iNow;
	int iAMsg = NetClient_LandNewMsg(NSC_LAND_ANIM);
	NMAddString(iAMsg, sA0, 24);
	NMAddString(iAMsg, sA1, 24);
	NMAddString(iAMsg, sA2, 24);
	NetClient_LandSend(iAMsg, false);
	NMDelete(iAMsg);
}

// the puppet plays what the hero of the player plays
void NetLand_OnAnim(int iMsg)
{
	int wID = NMGetClientID(iMsg);
	string sA0 = NMGetString(iMsg);
	string sA1 = NMGetString(iMsg);
	string sA2 = NMGetString(iMsg);
	if (!bNetLandActive) { return; }
	int iChr = NetLand_GetPuppet(wID);
	if (iChr < 0) { return; }
	ref ch = &Characters[iChr];
	if (!IsEntity(ch)) { return; }
	if (CheckAttribute(ch, "NetLandDead")) { if (sti(ch.NetLandDead)) { return; } }
	if (!CheckAttribute(ch, "NetLandAnimSync")) { return; }
	if (sti(ch.NetLandAnimSync) != 1) { return; }
	// the script action (player 2) is what the hero was told to play, else the body (player 0)
	string sPick = sA2;
	if (sPick == "") { sPick = sA0; }
	string sCur = "";
	if (CheckAttribute(ch, "NetLandAnimCur")) { sCur = ch.NetLandAnimCur; }
	if (sPick == sCur) { return; }
	ch.NetLandAnimCur = sPick;
	CharacterPlayAction(ch, sPick);
}

bool NetLand_PuppetAnimSync(ref ch)
{
	if (!CheckAttribute(ch, "NetLandAnimSync")) { return false; }
	if (sti(ch.NetLandAnimSync) == 1) { return true; }
	return false;
}

// the block of our hero (SPACE in the fight mode): the others see it, and the hits are resolved by it
void NetLand_CtrlOn()
{
	string sCtl = GetEventData();
	if (sCtl == "ChrBlock") { bNetLandBlockHeld = true; }
	if (sCtl == "ChrRun") { bNetLandRunHeld = true; }
	if (!bNetLandActive || bNetLandDead) { return; }
	if (sCtl == "ChrForward") { bNetLandFwdHeld = true; iNetLandFwdAt = Net_GetTickCount(); }
	if (sCtl == "ChrBackward") { bNetLandBackHeld = true; iNetLandBackAt = Net_GetTickCount(); NetLand_Hop(NET_LANDACT_RECOIL); }
	if (sCtl == "ChrStrafeLeft") { bNetLandLeftHeld = true; NetLand_Hop(NET_LANDACT_STRAFEL); }
	if (sCtl == "ChrStrafeRight") { bNetLandRightHeld = true; NetLand_Hop(NET_LANDACT_STRAFER); }
	if (sCtl == "ChrFire") { NetLand_ShotStart(); }
	if (findSubStr(sCtl, "Chr", 0) == 0)
	{
		Trace("COOP: land control " + sCtl);
	}
	// Shift + RMB: the hop back / parry of the hero. The others must see it, it is not a swing of the sword.
	if (sCtl == "ChrParry")
	{
		ref mcP = GetMainCharacter();
		if (IsEntity(mcP))
		{
			if (SendMessage(mcP, "ls", MSG_CHARACTER_EX_MSG, "IsFightMode") != 0)
			{
				iNetLandParryUntil = Net_GetTickCount() + NET_LAND_T_PARRY;
				int iPV = 1 + rand(3);
				NetLand_SetVariant(mcP, NET_LANDACT_PARRY, iPV);
				NetLand_OnLocalAction(mcP, NET_LANDACT_PARRY + iPV * 16);
			}
		}
	}
}

void NetLand_CtrlOff()
{
	string sCtl = GetEventData();
	if (sCtl == "ChrBlock") { bNetLandBlockHeld = false; }
	if (sCtl == "ChrRun") { bNetLandRunHeld = false; }
	if (sCtl == "ChrForward") { bNetLandFwdHeld = false; }
	if (sCtl == "ChrBackward") { bNetLandBackHeld = false; }
	if (sCtl == "ChrStrafeLeft") { bNetLandLeftHeld = false; }
	if (sCtl == "ChrStrafeRight") { bNetLandRightHeld = false; }
}

bool NetLand_HeroBlocking()
{
	if (bNetLandDead) { return false; }
	int iParryLeft = iNetLandParryUntil - Net_GetTickCount();
	if (iNetLandParryUntil != 0 && iParryLeft > 0) { return true; }
	if (!bNetLandBlockHeld) { return false; }
	ref mc = GetMainCharacter();
	if (!IsEntity(mc)) { return false; }
	if (SendMessage(mc, "ls", MSG_CHARACTER_EX_MSG, "IsFightMode") == 0) { return false; }
	float fEn = Lai_CharacterGetEnergy(mc);
	if (fEn < 1.0) { return false; }
	return true;
}

void NetLand_Tick()
{
	int iGen = GetEventData();
	if (iGen != iNetLandGen || !bNetLandActive) { return; }

	ref mc = GetMainCharacter();
	float fX = 0.0;
	float fY = 0.0;
	float fZ = 0.0;
	float fAy = 0.0;
	if (IsEntity(mc) && GetCharacterPos(mc, &fX, &fY, &fZ))
	{
		GetCharacterAy(mc, &fAy);
		int iNow = Net_GetTickCount();
		int iFlags = 0;
		if (SendMessage(mc, "ls", MSG_CHARACTER_EX_MSG, "IsFightMode") != 0) { iFlags = iFlags + 1; }
		if (bNetLandDead) { iFlags = iFlags + 2; }
		if (NetLand_HeroBlocking()) { iFlags = iFlags + 4; }
		float fHPNow = LAi_GetCharacterHP(mc);
		float fHPMaxNow = LAi_GetCharacterMaxHP(mc);
		int iHP = makeint(fHPNow);
		int iHPMax = makeint(fHPMaxNow);

		// our own speed: the others extrapolate with it (the puppet runs ahead of the packet, not behind it)
		float fVX = 0.0;
		float fVZ = 0.0;
		if (iNetLandPT != 0)
		{
			int iDt = iNow - iNetLandPT;
			if (iDt > 5 && iDt < 500)
			{
				fVX = (fX - fNetLandPX) * 1000.0 / makefloat(iDt);
				fVZ = (fZ - fNetLandPZ) * 1000.0 / makefloat(iDt);
			}
		}
		fNetLandPX = fX;
		fNetLandPZ = fZ;
		iNetLandPT = iNow;
		float fSp = sqrt(fVX * fVX + fVZ * fVZ);
		if (fSp > 12.0 || bNetLandDead) { fVX = 0.0; fVZ = 0.0; fSp = 0.0; }	// a teleport (spawn), not a run
		bool bMoving = false;
		if (fSp > 0.3) { bMoving = true; }
		fNetLandSpEMA = fNetLandSpEMA * 0.6 + fSp * 0.4;
		if (fNetLandSpEMA > NET_LAND_RUN_SPEED) { iFlags = iFlags + 8; }
		NetLand_AnimSample(mc, iNow);
		if (bNetLandAnimOk) { iFlags = iFlags + 64; }
		// W / S: the keys that are down now (the walk is played by them, also against a wall)
		int iFwdAgo = iNow - iNetLandFwdAt;
		int iBackAgo = iNow - iNetLandBackAt;
		if (bNetLandFwdHeld) { iFlags = iFlags + 16; }
		if (bNetLandBackHeld) { iFlags = iFlags + 32; }

		int iSpLog = iNow - iNetLandSpeedLog;
		if (bMoving && iSpLog > 1500)
		{
			iNetLandSpeedLog = iNow;
			Trace("COOP: land hero speed " + fSp + " run " + bNetLandRunHeld + " flags " + iFlags);
		}

		// a standing hero is sent rarely, a moving one every tick
		bool bSend = true;
		int iSince = iNow - iNetLandLastSend;
		bool bSame = false;
		if (iFlags == iNetLandLastFlags && iHP == iNetLandLastHP) { bSame = true; }
		if (!bMoving && !bNetLandLastMoving && bSame && iSince < NET_LAND_IDLE_TICK)
		{
			float fDA = fAy - fNetLandLastAy;
			if (fDA > -0.03 && fDA < 0.03) { bSend = false; }
		}

		if (bSend)
		{
			iNetLandLastSend = iNow;
			fNetLandLastAy = fAy;
			iNetLandLastFlags = iFlags;
			iNetLandLastHP = iHP;
			bNetLandLastMoving = bMoving;

			int iSMsg = NetClient_LandNewMsg(NSC_LAND_STATE);
			NMAddFloat(iSMsg, fX);
			NMAddFloat(iSMsg, fY);
			NMAddFloat(iSMsg, fZ);
			NMAddFloat(iSMsg, fAy);
			NMAddByte(iSMsg, iFlags);
			NMAddWord(iSMsg, iHP);
			NMAddWord(iSMsg, iHPMax);
			NMAddFloat(iSMsg, fVX);
			NMAddFloat(iSMsg, fVZ);
			NetClient_LandSend(iSMsg, false);
			NMDelete(iSMsg);
		}
	}

	PostEvent("NetLand_Tick", NET_LAND_TICK, "l", iGen);
}

// Loc_ai\LAi_fightparams.c (ChrAttackAction): our hero begins a swing. The animation goes to the others NOW,
// the damage follows with the hit itself (ChrFgtActApply / blade hit), so the victim sees the swing and can
// block or hop away before the hit comes.
int NetLand_SwingBusy(int iCode)
{
	switch (iCode)
	{
		case NET_LANDACT_FAST:	return 950; break;
		case NET_LANDACT_FORCE:	return 1250; break;
		case NET_LANDACT_ROUND:	return 1500; break;
		case NET_LANDACT_BREAK:	return 1250; break;
		case NET_LANDACT_FEINT:	return 850; break;
		case NET_LANDACT_FEINTC:	return 850; break;
	}
	return 1000;
}

void NetLand_AttackStart(aref attack, string sType)
{
	if (!bNetLandActive || bNetLandDead) { return; }
	if (sti(attack.index) != GetMainCharacterIndex()) { return; }
	int iCode = NetLand_ActCode(sType);
	if (iCode <= 0) { return; }
	int iNow = Net_GetTickCount();
	// the animation of the last swing is still on: the game ignores the click, nothing is sent
	int iBusy = iNetLandSwingBusyUntil - iNow;
	if (iNetLandSwingBusyUntil != 0 && iBusy > 0) { return; }
	iNetLandSwingBusyUntil = iNow + NetLand_SwingBusy(iCode);
	iNetLandProtectUntil = 0;
	iNetLandSwingPending = iNow;
	// a swing that begins when the hero was free: the time from the click to its hit is measured on it
	int iFree = iNow - iNetLandLastStrike;
	iNetLandCleanStart = 0;
	if (iNetLandLastStrike == 0 || iFree > 1500) { iNetLandCleanStart = iNow; }
	int iN = NetLand_ActVariants(iCode);
	int iVar = 1;
	if (iN > 1) { iVar = 1 + rand(iN - 1); }
	ref mc = GetMainCharacter();
	NetLand_SetVariant(mc, iCode, iVar);
	NetLand_SetLoadedVar(iCode, iVar);
	Trace("COOP: land swing " + sType + " variant " + iVar);
	// the swing goes to the others at once, also if it hits nobody; its hit follows later
	NetLand_OnLocalAction(attack, iCode + iVar * 16);
}

// ChrFgtActApply: only when the start of the swing was not announced (a game that does not ask ChrAttackAction)
void NetLand_OnLocalActionLate(aref chr, int iAct)
{
	if (iAct <= 0) { return; }		// not a swing (hit_parry ...)
	int iNow = Net_GetTickCount();
	if (iNetLandCleanStart != 0)
	{
		int iD = iNow - iNetLandCleanStart;
		if (iD > 100 && iD < 1500) { NetLand_LearnDelay(iAct, iD); }
	}
	iNetLandCleanStart = 0;
	int iPendAgo = iNow - iNetLandSwingPending;
	bool bAnnounced = false;
	if (iNetLandSwingPending != 0 && iPendAgo >= 0 && iPendAgo < 1800) { bAnnounced = true; }
	iNetLandSwingPending = 0;
	iNetLandLastStrike = iNow;
	if (bAnnounced) { return; }		// the others have this swing already
	// a swing that was not announced by a click (a chain of blows): it goes now
	int iVar = NetLand_LoadedVar(iAct);
	if (iVar < 1) { iVar = 1; }
	NetLand_OnLocalAction(chr, iAct + iVar * 16);
}

// the shot is announced when the player presses the fire key (the pistol is drawn and aimed first), the fire event
// of the game (LAi_events.c) comes later and sends only if the start was not announced
void NetLand_ShotLate(aref chr)
{
	int iAgo = Net_GetTickCount() - iNetLandShotAt;
	if (iNetLandShotAt != 0 && iAgo >= 0 && iAgo < 1500) { return; }
	NetLand_OnLocalAction(chr, NET_LANDACT_SHOT);
}

void NetLand_ShotStart()
{
	if (!bNetLandActive || bNetLandDead || iNetLandNoPistol != 0) { return; }
	ref mc = GetMainCharacter();
	if (!IsEntity(mc)) { return; }
	if (SendMessage(mc, "ls", MSG_CHARACTER_EX_MSG, "IsFightMode") == 0) { return; }
	if (!CheckAttribute(mc, "equip.gun")) { return; }
	if (CheckAttribute(mc, "chr_ai.charge"))
	{
		float fCharge = stf(mc.chr_ai.charge);
		if (fCharge < 1.0) { return; }
	}
	int iAgo = Net_GetTickCount() - iNetLandShotAt;
	if (iNetLandShotAt != 0 && iAgo >= 0 && iAgo < 1200) { return; }
	iNetLandShotAt = Net_GetTickCount();
	iNetLandProtectUntil = 0;
	NetLand_OnLocalAction(mc, NET_LANDACT_SHOT);
}

// a hop of our hero in the fight (S, A, D): the others see the hop, not a walk
void NetLand_Hop(int iCode)
{
	ref mc = GetMainCharacter();
	if (!IsEntity(mc)) { return; }
	if (SendMessage(mc, "ls", MSG_CHARACTER_EX_MSG, "IsFightMode") == 0) { return; }
	int iAgo = Net_GetTickCount() - iNetLandHopAt;
	if (iNetLandHopAt != 0 && iAgo >= 0 && iAgo < NET_LAND_HOP_BUSY) { return; }
	// W + A / D is a run to the side, not a hop
	if (bNetLandFwdHeld) { return; }
	iNetLandHopAt = Net_GetTickCount();
	NetLand_OnLocalAction(mc, iCode);
}

// Loc_ai\LAi_events.c (fire): our hero shoots
void NetLand_OnLocalAction(aref chr, int iAct)
{
	if (!bNetLandActive || bNetLandDead) { return; }
	if (iAct <= 0) { return; }
	if (sti(chr.index) != GetMainCharacterIndex()) { return; }
	iNetLandActSeq = iNetLandActSeq + 1;
	if (iNetLandActSeq > 255) { iNetLandActSeq = 1; }
	NetLand_SendAct(iAct, iNetLandActSeq);
	// not guaranteed (a guaranteed packet waits for the resend and arrives late): the same numbered packet twice
	PostEvent("NetLand_ActResend", NET_LAND_ACT_REPEAT, "lll", iNetLandGen, iAct, iNetLandActSeq);
}

void NetLand_SendAct(int iAct, int iSeq)
{
	int iSMsg = NetClient_LandNewMsg(NSC_LAND_ACT);
	NMAddByte(iSMsg, iAct);
	NMAddByte(iSMsg, iSeq);
	NetClient_LandSend(iSMsg, false);
	NMDelete(iSMsg);
}

void NetLand_ActResend()
{
	int iGen = GetEventData();
	int iAct = GetEventData();
	int iSeq = GetEventData();
	if (iGen != iNetLandGen || !bNetLandActive || bNetLandDead) { return; }
	NetLand_SendAct(iAct, iSeq);
}

// client id of a puppet, or DST_INVALID
int NetLand_PuppetID(aref chr)
{
	if (!bNetLandActive) { return DST_INVALID; }
	if (!CheckAttribute(chr, "NetLandID")) { return DST_INVALID; }
	return sti(chr.NetLandID);
}

bool NetLand_IsOurHero(aref chr)
{
	if (!CheckAttribute(chr, "index")) { return false; }
	return sti(chr.index) == GetMainCharacterIndex();
}

// fDamage - the damage of a clean hit, fDamageB - if the victim blocks it. The victim knows if he blocks.
void NetLand_SendHit(int wVictim, float fDamage, float fDamageB, int iKind, int iDelay)
{
	int iDmg = makeint(fDamage + 0.5);
	int iDmgB = makeint(fDamageB + 0.5);
	if (iDmg <= 0 && iDmgB <= 0) { return; }
	if (iDmgB > iDmg) { iDmgB = iDmg; }
	int iSMsg = NetClient_LandNewMsg(NSC_LAND_HIT);
	NMAddClientID(iSMsg, wVictim);
	NMAddWord(iSMsg, iDmg);
	NMAddWord(iSMsg, iDmgB);
	NMAddWord(iSMsg, iDelay);
	NMAddByte(iSMsg, iKind);
	NetClient_LandSend(iSMsg, true);
	NMDelete(iSMsg);
}

// Loc_ai\LAi_fightparams.c: LAi_ApplyCharacterAttackDamage. true = the hit is ours to handle (no single player damage)
bool NetLand_BladeHit(aref attack, aref enemy, string attackType, bool isBlocked)
{
	if (!bNetLandActive) { return false; }
	// a puppet hits: its owner has decided it on his machine already. But the game has just worked out
	// if OUR hero blocks this swing (his real block, angle and timing): the damage will use it
	if (NetLand_PuppetID(attack) != DST_INVALID)
	{
		if (NetLand_IsOurHero(enemy))
		{
			iNetLandBlockedAt = Net_GetTickCount();
			bNetLandBlockedLast = isBlocked;
		}
		return true;
	}

	int wVictim = NetLand_PuppetID(enemy);
	if (wVictim == DST_INVALID) { return false; }
	if (!NetLand_IsOurHero(attack) || bNetLandDead) { return true; }
	if (CheckAttribute(enemy, "NetLandDead") && sti(enemy.NetLandDead)) { return true; }

	// the puppet never blocks on our machine: both damages go, the victim picks
	float fClean = NetLand_BladeDamage(attack, enemy, attackType, false);
	float fBlock = NetLand_BladeDamage(attack, enemy, attackType, true);
	NetLand_SendHit(wVictim, fClean, fBlock, 0, NetLand_GetDelay(NetLand_ActCode(attackType)));
	return true;
}

// Loc_ai\LAi_fightparams.c: LAi_ApplyCharacterFireDamage
bool NetLand_FireHit(aref attack, aref enemy, float kDist)
{
	if (!bNetLandActive) { return false; }
	if (NetLand_PuppetID(attack) != DST_INVALID) { return true; }

	int wVictim = NetLand_PuppetID(enemy);
	if (wVictim == DST_INVALID) { return false; }
	if (!NetLand_IsOurHero(attack) || bNetLandDead) { return true; }
	if (CheckAttribute(enemy, "NetLandDead") && sti(enemy.NetLandDead)) { return true; }

	float fP = LAi_GunCalcProbability(attack, kDist);
	if (rand(10000) > fP * 10000) { return true; }
	float fShot = LAi_GunCalcDamage(attack, enemy);
	NetLand_SendHit(wVictim, fShot, fShot, 1, 0);
	return true;
}

// the server: somebody hit us
void NetLand_OnHit(int iMsg)
{
	int wAttacker = NMGetClientID(iMsg);
	int iDmg = NMGetWord(iMsg);
	int iDmgB = NMGetWord(iMsg);
	int iHitDelay = NMGetWord(iMsg);
	int iKind = NMGetByte(iMsg);

	if (!bNetLandActive || bNetLandDead) { return; }
	if (iKind == 0 && iDmgB < iDmg && NetLand_HeroBlocking())
	{
		// the attacker sees the block now, not when the hit lands
		int iParryL = iNetLandParryUntil - Net_GetTickCount();
		if (iNetLandParryUntil != 0 && iParryL > 0) { NetLand_SendHitRes(wAttacker, NET_LANDRES_PARRY); }
		else { NetLand_SendHitRes(wAttacker, NET_LANDRES_BLOCK); }
		iNetLandPrelimAt = Net_GetTickCount();
	}
	if (iHitDelay > 0 && iKind == 0)
	{
		// the swing is on the screen already: the hit lands when its blow does (block / hop away until then)
		if (iHitDelay > 900) { iHitDelay = 900; }
		PostEvent("NetLand_HitApply", iHitDelay, "lllll", iNetLandGen, wAttacker, iDmg, iDmgB, iKind);
		return;
	}
	NetLand_ApplyHit(wAttacker, iDmg, iDmgB, iKind);
}

void NetLand_HitApply()
{
	int iGen = GetEventData();
	int wAtt = GetEventData();
	int iD = GetEventData();
	int iDB = GetEventData();
	int iK = GetEventData();
	if (iGen != iNetLandGen || !bNetLandActive || bNetLandDead) { return; }
	NetLand_ApplyHit(wAtt, iD, iDB, iK);
}

void NetLand_ApplyHit(int wAttacker, int iDmg, int iDmgB, int iKind)
{
	if (!bNetLandActive || bNetLandDead) { return; }
	ref mc = GetMainCharacter();
	if (!IsEntity(mc)) { return; }

	// a new player, or one that has just come back, cannot be hurt
	int iProtLeft = iNetLandProtectUntil - Net_GetTickCount();
	if (iNetLandProtectUntil != 0 && iProtLeft > 0)
	{
		NetLand_SendHitRes(wAttacker, NET_LANDRES_MISS);
		return;
	}

	// a blade: it does not reach a hero that has hopped away (his game knows where he is)
	if (iKind == 0)
	{
		int iAttChr = NetLand_GetPuppet(wAttacker);
		if (iAttChr >= 0)
		{
			ref rAtt = &Characters[iAttChr];
			if (IsEntity(rAtt))
			{
				float fAX = 0.0;
				float fAY = 0.0;
				float fAZ = 0.0;
				float fMX2 = 0.0;
				float fMY2 = 0.0;
				float fMZ2 = 0.0;
				if (GetCharacterPos(rAtt, &fAX, &fAY, &fAZ) && GetCharacterPos(mc, &fMX2, &fMY2, &fMZ2))
				{
					float fD2 = sqrt((fAX - fMX2) * (fAX - fMX2) + (fAZ - fMZ2) * (fAZ - fMZ2));
					if (fD2 > NET_LAND_REACH) { Trace("COOP: land hit missed, distance " + fD2); NetLand_SendHitRes(wAttacker, NET_LANDRES_MISS); return; }
				}
			}
		}
	}

	// a blade: the victim decides if it was blocked (the attacker cannot see it, the puppet does not block there)
	bool bBlocked = false;
	if (iKind == 0 && iDmgB < iDmg)
	{
		int iAgo = Net_GetTickCount() - iNetLandBlockedAt;
		if (iNetLandBlockedAt != 0 && iAgo >= 0 && iAgo < 700) { bBlocked = bNetLandBlockedLast; }
		else { bBlocked = NetLand_HeroBlocking(); }
	}
	iNetLandBlockedAt = 0;
	if (bBlocked)
	{
		int iParryLeft = iNetLandParryUntil - Net_GetTickCount();
		int iPrelimAgo = Net_GetTickCount() - iNetLandPrelimAt;
		bool bTold = false;
		if (iNetLandPrelimAt != 0 && iPrelimAgo >= 0 && iPrelimAgo < 1500) { bTold = true; }
		if (!bTold)
		{
			if (iNetLandParryUntil != 0 && iParryLeft > 0) { NetLand_SendHitRes(wAttacker, NET_LANDRES_PARRY); }
			else { NetLand_SendHitRes(wAttacker, NET_LANDRES_BLOCK); }
		}
		// a block: only the damage that goes through, no hit reaction (the game has played the block itself)
		if (iDmgB > 0)
		{
			float fHPOld = LAi_GetCharacterHP(mc);
			float fHPMaxB = LAi_GetCharacterMaxHP(mc);
			float fHPB = fHPOld - makefloat(iDmgB);
			if (fHPB < 0.0) { fHPB = 0.0; }
			LAi_SetHP(mc, fHPB, fHPMaxB);
		}
	}
	else
	{
		if (iDmg <= 0) { return; }
		NetLand_SendHitRes(wAttacker, NET_LANDRES_HIT);
		LAi_ApplyCharacterDamage(mc, iDmg);		// blood and the hit sound like in single player, the hero is immortal there
	}
	if (LAi_GetCharacterHP(mc) < 1.0) { NetLand_LocalDie(wAttacker); }
}

void NetLand_LocalDie(int wKiller)
{
	if (bNetLandDead) { return; }
	bNetLandDead = true;
	ref mc = GetMainCharacter();
	mc.chr_ai.hp = 0.0;
	LAi_SetActorType(mc);
	// one of the four deaths of the game, the same one is shown to the others. The body stays lying when the
	// animation has ended (LAi_events.c does not let the stay template make the hero stand up)
	iNetLandDeadVar = 1 + rand(3);
	int iDeathNum = iNetLandDeadVar - 1;
	CharacterPlayAction(mc, "death_" + iDeathNum);

	int iSMsg = NetClient_LandNewMsg(NSC_LAND_DEAD);
	NMAddClientID(iSMsg, wKiller);
	NMAddByte(iSMsg, iNetLandDeadVar);
	NetClient_LandSend(iSMsg, true);
	NMDelete(iSMsg);

	if (iNetLandRespawn) { NetClient_LogMessageNum("Net_RespawnIn", iNetLandRespawnSec, 0); }
	else { Log_SetStringToLog(LanguageConvertString(iLangNetClient, "Net_LandYouDied")); }
}

// LAi_events.c: the dead hero lies where he fell
bool NetLand_HeroDeadHold(aref chr)
{
	if (!bNetLandActive || !bNetLandDead) { return false; }
	if (sti(chr.index) == GetMainCharacterIndex()) { return true; }
	return false;
}

// ---------- the other players ----------
// puppet of a player (created at his first state packet)
int NetLand_GetPuppet(int wID)
{
	if (wID < 0 || wID >= NET_MAXPLAYERS || wID == wClientID) { return -1; }
	if (CheckAttribute(&NCClients[wID], "LandChr")) { return sti(NCClients[wID].LandChr); }
	if (!IsEntity(loadedLocation)) { return -1; }

	string sPModel = NetLand_ModelFor(wID);
	iNetLandPupGen++;
	int iChr = NPC_GenerateCharacter("NetLandP_" + wID + "_" + iNetLandPupGen, sPModel, NetLand_ModelSex(sPModel), NetLand_ModelAnim(sPModel), 10, PIRATE, 0, false);
	if (iChr < 0) { return -1; }
	ref ch = &Characters[iChr];
	ch.name = NCClients[wID].NickName;
	ch.lastname = "";
	ch.NetLandID = wID;
	ch.NetLandDead = false;
	DeleteAttribute(ch, "chr_ai.hp");
	NetLand_ApplyPuppetLo(ch, wID);
	LAi_SetImmortal(ch, true);
	LAi_CharacterDisableDialog(ch);
	LAi_SetActorType(ch);
	// every type of the game sets it (LAi_warrior.c...), the actor does not: a puppet fought with bare hands
	SendMessage(ch, "lsl", MSG_CHARACTER_EX_MSG, "SetFightWOWeapon", false);
	LAi_group_MoveCharacter(ch, NET_LAND_GROUP);
	ChangeCharacterAddressGroup(ch, sNetLandLoc, sNetLandEntryGroup, sNetLandEntryLocator);
	ch.NetLandCreated = Net_GetTickCount();
	NCClients[wID].LandChr = iChr;
	Trace("COOP: land puppet of " + wID + " is character " + iChr);
	return iChr;
}

// the sword out / in. The same messages as LAi_tmpl_fight (Loc_ai/templates/LAi_fight.c): unlock, change, lock.
// ("SetFightMode" of the first version is not the message the game uses for it: the puppet never took the sword out.)
void NetLand_PuppetFight(ref ch, int iOn)
{
	int iEng = 0;
	if (SendMessage(ch, "ls", MSG_CHARACTER_EX_MSG, "IsFightMode") != 0) { iEng = 1; }
	if (iEng == iOn) { return; }
	// the game refuses a change while the character is busy (the sword goes into the sheath, a shot): try again
	int iNow = Net_GetTickCount();
	if (CheckAttribute(ch, "NetLandFightTry"))
	{
		int iAgo = iNow - sti(ch.NetLandFightTry);
		if (iAgo >= 0 && iAgo < 200) { return; }
	}
	ch.NetLandFightTry = iNow;
	SendMessage(ch, "lsl", MSG_CHARACTER_EX_MSG, "LockFightMode", 0);
	if (iOn)
	{
		bool bWO = false;
		if (ch.model.animation == "mushketer") { bWO = true; }
		SendMessage(ch, "lsl", MSG_CHARACTER_EX_MSG, "SetFightWOWeapon", bWO);
		NetLand_PuppetWeapons(ch);
		SendMessage(ch, "lsl", MSG_CHARACTER_EX_MSG, "ChangeFightMode", 1);
		SendMessage(ch, "lsl", MSG_CHARACTER_EX_MSG, "LockFightMode", 1);
	}
	else
	{
		SendMessage(ch, "lsl", MSG_CHARACTER_EX_MSG, "ChangeFightMode", 0);
	}
	int iAfter = 0;
	if (SendMessage(ch, "ls", MSG_CHARACTER_EX_MSG, "IsFightMode") != 0) { iAfter = 1; }
	Trace("COOP: land puppet " + ch.id + " fight " + iOn + " (game says " + iAfter + ")");
}

// the blade and the pistol of the puppet are sent to the character again (the same messages as SetNewModelToChar)
void NetLand_PuppetWeapons(ref ch)
{
	if (!IsEntity(ch)) { return; }
	if (CheckAttribute(ch, "equip.gun"))
	{
		SendMessage(ch, "ls", MSG_CHARACTER_SETGUN, ch.equip.gun);
	}
	if (CheckAttribute(ch, "equip.blade"))
	{
		ref rBl = ItemsFromID(ch.equip.blade);
		SendMessage(ch, "llsfll", MSG_CHARACTER_SETBLADE, 0, rBl.model, 0.1, argb(64, 64, 64, 64), argb(0, 32, 32, 32));
		Trace("COOP: land puppet " + ch.id + " blade " + ch.equip.blade + " model " + rBl.model);
	}
	else { Trace("COOP: land puppet " + ch.id + " has NO equip.blade"); }
}

bool NetLand_PuppetActing(ref ch)
{
	if (!CheckAttribute(ch, "NetLandActUntil")) { return false; }
	int iLeft = Net_GetTickCount() - sti(ch.NetLandActUntil);
	if (iLeft < 0) { return true; }
	return false;
}

void NetLand_PuppetStay(ref ch)
{
	if (CheckAttribute(ch, "NetLandTask"))
	{
		if (sti(ch.NetLandTask) == 0) { return; }
	}
	SetCharacterTask_Stay(ch);
	ch.NetLandTask = 0;
}

void NetLand_OnState(int iMsg)
{
	int wID = NMGetClientID(iMsg);
	float fX = NMGetFloat(iMsg);
	float fY = NMGetFloat(iMsg);
	float fZ = NMGetFloat(iMsg);
	float fAy = NMGetFloat(iMsg);
	int iFlags = NMGetByte(iMsg);
	int iHP = NMGetWord(iMsg);
	int iHPMax = NMGetWord(iMsg);
	float fVX = NMGetFloat(iMsg);
	float fVZ = NMGetFloat(iMsg);

	if (!bNetLandActive) { return; }
	int iChr = NetLand_GetPuppet(wID);
	if (iChr < 0) { return; }
	ref ch = &Characters[iChr];
	if (!IsEntity(ch))
	{
		if (CheckAttribute(ch, "NetLandCreated"))
		{
			int iAgeC = Net_GetTickCount() - sti(ch.NetLandCreated);
			if (iAgeC > 2500)
			{
				Trace("COOP: land puppet of " + wID + " is not in the location, a new one is made");
				DeleteAttribute(&NCClients[wID], "LandChr");
				ChangeCharacterAddressGroup(ch, "none", "", "");
			}
		}
		return;
	}

	// the blade and the pistol are given to the puppet while it was not in the location yet: the messages
	// of the equipment were lost (an empty hand that waves). Send them again now when it is there.
	if (!CheckAttribute(ch, "NetLandEquipped"))
	{
		SetNewModelToChar(ch);
		SendMessage(ch, "lsl", MSG_CHARACTER_EX_MSG, "SetFightWOWeapon", false);
		ch.NetLandEquipped = 1;
	}

	if (iHPMax > 0)
	{
		float fOldHP = LAi_GetCharacterHP(ch);
		float fOldMax = LAi_GetCharacterMaxHP(ch);
		int iOldHP = makeint(fOldHP);
		int iOldMax = makeint(fOldMax);
		if (iOldHP != iHP || iOldMax != iHPMax) { LAi_SetHP(ch, makefloat(iHP), makefloat(iHPMax)); }
	}
	if (sti(ch.NetLandDead)) { return; }			// lies dead until the respawn

	// the sword in the hand like the player (not in the middle of a swing)
	int iFight = 0;
	if (iFlags - (iFlags / 2) * 2 == 1) { iFight = 1; }
	int iBlock = 0;
	int iBitA = iFlags / 4;
	int iBitB = iFlags / 8;
	iBitB = iBitB * 2;
	int iBitC = iBitA - iBitB;
	if (iBitC == 1) { iBlock = 1; }
	bool bActing = NetLand_PuppetActing(ch);
	bool bHolster = false;
	if (CheckAttribute(ch, "NetLandHolster"))
	{
		int iHolsterLeft = sti(ch.NetLandHolster) - Net_GetTickCount();
		if (iHolsterLeft > 0) { bHolster = true; }
	}
	if (!bActing && !bHolster) { NetLand_PuppetFight(ch, iFight); }

	// the packet is only remembered: the puppet is moved smoothly and its walk is played by NetLand_Smooth
	int iRunA = iFlags / 8;
	int iRunB = iFlags / 16;
	iRunB = iRunB * 2;
	int iRun = iRunA - iRunB;
	ch.NetLandLX = fX;
	ch.NetLandLY = fY;
	ch.NetLandLZ = fZ;
	ch.NetLandLAy = fAy;
	ch.NetLandLVX = fVX;
	ch.NetLandLVZ = fVZ;
	ch.NetLandLT = Net_GetTickCount();
	ch.NetLandLFight = iFight;
	int iSyncA = iFlags / 64;
	int iSyncB = iFlags / 128;
	iSyncB = iSyncB * 2;
	int iSyncC = iSyncA - iSyncB;
	if (iSyncC == 1 && !CheckAttribute(ch, "NetLandAnimSync")) { Trace("COOP: land puppet of " + wID + " plays the animation of its hero"); }
	ch.NetLandAnimSync = iSyncC;
	if (iSyncC != 1) { DeleteAttribute(ch, "NetLandAnimCur"); }
	ch.NetLandLRun = iRun;
	int iKeyA = iFlags / 16;
	int iKeyB = iFlags / 32;
	int iKeyC = iFlags / 64;
	iKeyC = iKeyC * 2;
	int iKeyBack = iKeyB - iKeyC;
	iKeyB = iKeyB * 2;
	int iKeyFwd = iKeyA - iKeyB;
	ch.NetLandLFwd = iKeyFwd;
	ch.NetLandLBack = iKeyBack;
	NetLand_PuppetBlock(ch, iBlock, iFight);
}

// the block pose of the puppet: as long as the player holds the block
void NetLand_PuppetBlock(ref ch, int iBlock, int iFight)
{
	if (NetLand_PuppetAnimSync(ch)) { return; }
	if (NetLand_PuppetActing(ch)) { return; }
	int iHas = 0;
	if (CheckAttribute(ch, "NetLandBlock")) { iHas = sti(ch.NetLandBlock); }
	if (iBlock == 1 && iFight == 1)
	{
		if (iHas == 0)
		{
			CharacterPlayAction(ch, "block");
			ch.NetLandBlock = 1;
			ch.NetLandLoco = "";
		}
		return;
	}
	if (iHas == 1)
	{
		CharacterPlayAction(ch, "");
		ch.NetLandBlock = 0;
		ch.NetLandLoco = "";
	}
}

// the animation of the walk, exactly the one that the player has: forward / back / to the side, walk / run, in
// the fight and out of it (the names of characters.c: fight walk, fight back run, straif_weapon_left ...)
string NetLand_LocoAction(int iFwd, int iBack, int iFight, int iRun, float fSp)
{
	bool bRun = false;
	if (iRun == 1 || fSp > NET_LAND_RUN_SPEED) { bRun = true; }
	if (iFwd == 1)
	{
		if (iFight == 1)
		{
			if (bRun) { return "fight run"; }
			return "fight walk";
		}
		if (bRun) { return "run"; }
		return "walk";
	}
	// out of the fight S is a walk backwards, in the fight it is a hop (NetLand_Hop) and needs no loop
	if (iBack == 1 && iFight == 0)
	{
		if (bRun) { return "back run"; }
		return "back walk";
	}
	return "";
}

// one move of a puppet (every NET_LAND_SMOOTH ms): the place is the place of the last packet plus the way that the
// player has made since then, the walk animation is played in place
void NetLand_PuppetMove(ref ch, int iNow)
{
	int iAgeMs = iNow - sti(ch.NetLandLT);
	float fVX = stf(ch.NetLandLVX);
	float fVZ = stf(ch.NetLandLVZ);
	float fT = makefloat(iAgeMs) / 1000.0;
	if (fT > 0.35) { fT = 0.35; }
	fT = fT + NET_LAND_LEAD;
	if (iAgeMs > 450) { fVX = 0.0; fVZ = 0.0; fT = 0.0; }		// the packets have stopped: stand
	float fTX = stf(ch.NetLandLX) + fVX * fT;
	float fTY = stf(ch.NetLandLY);
	float fTZ = stf(ch.NetLandLZ);
	fTZ = fTZ + fVZ * fT;
	float fAy = stf(ch.NetLandLAy);

	float fCX = 0.0;
	float fCY = 0.0;
	float fCZ = 0.0;
	if (!GetCharacterPos(ch, &fCX, &fCY, &fCZ)) { return; }
	float fCAy = 0.0;
	GetCharacterAy(ch, &fCAy);
	float fDX = fTX - fCX;
	float fDZ = fTZ - fCZ;
	float fDist = sqrt(fDX * fDX + fDZ * fDZ);
	float fDA = fAy - fCAy;
	while (fDA > PI) { fDA = fDA - PIm2; }
	while (fDA < -PI) { fDA = fDA + PIm2; }

	bool bActing = NetLand_PuppetActing(ch);
	int iKind = 0;
	if (bActing && CheckAttribute(ch, "NetLandActKind")) { iKind = sti(ch.NetLandActKind); }
	int iBlk = 0;
	if (CheckAttribute(ch, "NetLandBlock")) { iBlk = sti(ch.NetLandBlock); }

	// a swing and a block stand on the place (small differences are left), a hop (parry) goes with the player
	bool bFollow = true;
	bool bHop = false;
	if (iKind == NET_LANDACT_PARRY || iKind == NET_LANDACT_RECOIL || iKind == NET_LANDACT_STRAFEL || iKind == NET_LANDACT_STRAFER) { bHop = true; }
	if (bActing && !bHop && fDist < 0.6) { bFollow = false; }
	if (iBlk == 1 && fDist < 0.4) { bFollow = false; }
	if (bFollow)
	{
		float fNX = fTX;
		float fNY = fTY;
		float fNZ = fTZ;
		float fNAy = fAy;
		if (fDist <= NET_LAND_SNAP)
		{
			fNX = fCX + fDX * NET_LAND_SMOOTH_K;
			fNY = fCY + (fTY - fCY) * NET_LAND_SMOOTH_K;
			fNZ = fCZ + fDZ * NET_LAND_SMOOTH_K;
			fNAy = fCAy + fDA * NET_LAND_SMOOTH_K;
		}
		if (fDist > 0.01 || fDA > 0.01 || fDA < -0.01) { TeleportCharacterToPosAy(ch, fNX, fNY, fNZ, fNAy); }
	}

	// the animation of the walk (not under a swing / a block); with the patched dll it comes from the hero
	if (NetLand_PuppetAnimSync(ch)) { return; }
	if (bActing || iBlk == 1) { return; }
	int iFight = 0;
	if (CheckAttribute(ch, "NetLandLFight")) { iFight = sti(ch.NetLandLFight); }
	int iRun = 0;
	if (CheckAttribute(ch, "NetLandLRun")) { iRun = sti(ch.NetLandLRun); }
	float fSp = sqrt(fVX * fVX + fVZ * fVZ);
	int iFwd = 0;
	if (CheckAttribute(ch, "NetLandLFwd")) { iFwd = sti(ch.NetLandLFwd); }
	int iBack = 0;
	if (CheckAttribute(ch, "NetLandLBack")) { iBack = sti(ch.NetLandLBack); }
	if (iAgeMs > 450) { iFwd = 0; iBack = 0; }
	string sLoco = NetLand_LocoAction(iFwd, iBack, iFight, iRun, fSp);
	string sCur = "";
	if (CheckAttribute(ch, "NetLandLoco")) { sCur = ch.NetLandLoco; }
	if (sLoco != sCur)
	{
		CharacterPlayAction(ch, sLoco);
		ch.NetLandLoco = sLoco;
	}
}

void NetLand_Smooth()
{
	int iGen = GetEventData();
	if (iGen != iNetLandGen || !bNetLandActive) { return; }
	int iNow = Net_GetTickCount();
	for (int i=0; i<NET_MAXPLAYERS; i++)
	{
		if (i == wClientID) { continue; }
		if (!CheckAttribute(&NCClients[i], "LandChr")) { continue; }
		int iChr = sti(NCClients[i].LandChr);
		if (iChr < 0) { continue; }
		ref ch = &Characters[iChr];
		if (!IsEntity(ch)) { continue; }
		if (!CheckAttribute(ch, "NetLandLT")) { continue; }
		if (CheckAttribute(ch, "NetLandDead")) { if (sti(ch.NetLandDead)) { continue; } }
		NetLand_PuppetMove(ch, iNow);
	}
	PostEvent("NetLand_Smooth", NET_LAND_SMOOTH, "l", iGen);
}

// the puppet of the victim takes a blocked hit: the game's own block_hit animation (it has the sound of the blades)
void NetLand_PuppetFlinch(ref ch, string sAction, int iDur)
{
	if (NetLand_PuppetAnimSync(ch)) { return; }
	if (!IsEntity(ch)) { return; }
	iNetLandActCounter++;
	ch.NetLandActId = iNetLandActCounter;
	ch.NetLandActKind = 0;
	ch.NetLandActUntil = Net_GetTickCount() + iDur;
	ch.NetLandBlock = 0;
	ch.NetLandLoco = "";
	CharacterPlayAction(ch, sAction);
	PostEvent("NetLand_ActEnd", iDur, "lll", iNetLandGen, sti(ch.index), iNetLandActCounter);
}

// our hero is thrown back by a parry of the victim
void NetLand_StaggerEnd()
{
	int iGen = GetEventData();
	if (iGen != iNetLandGen || !bNetLandActive || bNetLandDead) { return; }
	ref mc = GetMainCharacter();
	if (IsEntity(mc)) { CharacterPlayAction(mc, ""); }
}

void NetLand_OnHitRes(int iMsg)
{
	int wVictim = NMGetClientID(iMsg);
	int iRes = NMGetByte(iMsg);
	if (!bNetLandActive || bNetLandDead) { return; }
	if (iRes == NET_LANDRES_BLOCK)
	{
		int iVChr = NetLand_GetPuppet(wVictim);
		if (iVChr >= 0) { NetLand_PuppetFlinch(&Characters[iVChr], "block_hit", NET_LAND_T_FLINCH); }
	}
	if (iRes == NET_LANDRES_PARRY)
	{
		ref mc = GetMainCharacter();
		if (IsEntity(mc))
		{
			CharacterPlayAction(mc, "hit_parry");
			PostEvent("NetLand_StaggerEnd", 700, "l", iNetLandGen);
		}
	}
}

void NetLand_SendHitRes(int wAttacker, int iRes)
{
	int iSMsg = NetClient_LandNewMsg(NSC_LAND_HITRES);
	NMAddClientID(iSMsg, wAttacker);
	NMAddByte(iSMsg, iRes);
	NetClient_LandSend(iSMsg, true);
	NMDelete(iSMsg);
}

int NetLand_ActTime(int iAct)
{
	switch (iAct)
	{
		case NET_LANDACT_FAST:	return NET_LAND_T_FAST; break;
		case NET_LANDACT_FORCE:	return NET_LAND_T_FORCE; break;
		case NET_LANDACT_ROUND:	return NET_LAND_T_ROUND; break;
		case NET_LANDACT_BREAK:	return NET_LAND_T_BREAK; break;
		case NET_LANDACT_FEINT:	return NET_LAND_T_FEINT; break;
		case NET_LANDACT_FEINTC:	return NET_LAND_T_FEINT; break;
		case NET_LANDACT_SHOT:	return NET_LAND_T_SHOT; break;
		case NET_LANDACT_PARRY:	return NET_LAND_T_PARRY; break;
		case NET_LANDACT_RECOIL:	return NET_LAND_T_HOP; break;
		case NET_LANDACT_STRAFEL:	return NET_LAND_T_HOP; break;
		case NET_LANDACT_STRAFER:	return NET_LAND_T_HOP; break;
	}
	return 700;
}

void NetLand_OnAct(int iMsg)
{
	int wID = NMGetClientID(iMsg);
	int iAct = NMGetByte(iMsg);
	int iSeq = NMGetByte(iMsg);
	int iVar = iAct / 16;
	iAct = iAct - iVar * 16;
	if (!bNetLandActive) { return; }
	int iChr = NetLand_GetPuppet(wID);
	if (iChr < 0) { return; }
	ref ch = &Characters[iChr];
	if (!IsEntity(ch) || sti(ch.NetLandDead)) { return; }
	if (NetLand_PuppetAnimSync(ch)) { return; }		// the actions come as they are played

	// the second copy of a packet that has come already
	int iSeq1 = -1;
	int iSeq2 = -1;
	if (CheckAttribute(ch, "NetLandSeq1")) { iSeq1 = sti(ch.NetLandSeq1); }
	if (CheckAttribute(ch, "NetLandSeq2")) { iSeq2 = sti(ch.NetLandSeq2); }
	if (iSeq == iSeq1 || iSeq == iSeq2) { return; }
	if (CheckAttribute(ch, "NetLandSeq1")) { ch.NetLandSeq2 = ch.NetLandSeq1; }
	ch.NetLandSeq1 = iSeq;

	string sAction = NetLand_ActName(iAct, iVar);
	if (sAction == "") { return; }
	NetLand_PuppetFight(ch, 1);
	NetLand_PuppetStay(ch);
	ch.NetLandBlock = 0;
	ch.NetLandLoco = "";

	// the action of CharacterPlayAction does not end by itself: it is cleared by the timer (NetLand_ActEnd)
	int iDur = NetLand_ActTime(iAct);
	iNetLandActCounter++;
	ch.NetLandActId = iNetLandActCounter;
	ch.NetLandActKind = iAct;
	ch.NetLandActName = sAction;
	ch.NetLandActStart = Net_GetTickCount();
	ch.NetLandActUntil = Net_GetTickCount() + iDur;
	CharacterPlayAction(ch, sAction);
	PostEvent("NetLand_ActEnd", iDur, "lll", iNetLandGen, iChr, iNetLandActCounter);
}

// the action of the puppet is over: back to the stand (the state packets set the fight / the walk / the block again)
void NetLand_FinishAction(ref ch)
{
	int iKind = 0;
	if (CheckAttribute(ch, "NetLandActKind")) { iKind = sti(ch.NetLandActKind); }
	if (CheckAttribute(ch, "NetLandActStart") && CheckAttribute(ch, "NetLandActName"))
	{
		int iLast = Net_GetTickCount() - sti(ch.NetLandActStart);
		Trace("COOP: land action " + ch.NetLandActName + " lasted " + iLast + " ms");
	}
	DeleteAttribute(ch, "NetLandActId");
	DeleteAttribute(ch, "NetLandActUntil");
	DeleteAttribute(ch, "NetLandActStart");
	if (!IsEntity(ch)) { return; }
	if (CheckAttribute(ch, "NetLandDead")) { if (sti(ch.NetLandDead)) { return; } }
	CharacterPlayAction(ch, "");
	ch.NetLandLoco = "";
	if (iKind == NET_LANDACT_SHOT)
	{
		// the pistol stays in the hand of a puppet whose shot has ended: the weapons are set again (the sword stays out)
		SetNewModelToChar(ch);
		bool bWOS = false;
		if (ch.model.animation == "mushketer") { bWOS = true; }
		SendMessage(ch, "lsl", MSG_CHARACTER_EX_MSG, "SetFightWOWeapon", bWOS);
		NetLand_PuppetWeapons(ch);
	}
}

// the game tells that the action asked by the script (CharacterPlayAction) has played to its end
void NetLand_PuppetEndAction(aref chr)
{
	if (!bNetLandActive) { return; }
	ref ch = &Characters[sti(chr.index)];
	if (!CheckAttribute(ch, "NetLandActId")) { return; }
	NetLand_FinishAction(ch);
}

// the safety timer: only if the game did not say that the action had ended
void NetLand_ActEnd()
{
	int iGen = GetEventData();
	int iChr = GetEventData();
	int iId = GetEventData();
	if (iGen != iNetLandGen || !bNetLandActive) { return; }
	if (iChr < 0) { return; }
	ref ch = &Characters[iChr];
	if (!CheckAttribute(ch, "NetLandActId")) { return; }
	if (sti(ch.NetLandActId) != iId) { return; }		// a newer action has its own timer
	NetLand_FinishAction(ch);
}

void NetLand_OnDead(int iMsg)
{
	int wDead = NMGetClientID(iMsg);
	int wKiller = NMGetClientID(iMsg);
	int iKills = NMGetWord(iMsg);
	int iDeadVarR = NMGetByte(iMsg);
	if (!bNetLandActive) { return; }
	if (wDead < 0 || wDead >= NET_MAXPLAYERS) { return; }

	// the kill line
	string sDead = NCClients[wDead].NickName;
	string sLine = sDead + " " + LanguageConvertString(iLangNetClient, "Net_LandDied");
	if (wKiller >= 0 && wKiller < NET_MAXPLAYERS && wKiller != wDead)
	{
		string sKiller = NCClients[wKiller].NickName;
		sLine = sKiller + " " + LanguageConvertString(iLangNetClient, "Net_LandKilled") + " " + sDead + " (" + iKills + ")";
	}
	NetClient_ChatAddSystemLine(sLine);

	if (wDead == wClientID) { return; }		// we know it already (NetLand_LocalDie)
	int iChr = NetLand_GetPuppet(wDead);
	if (iChr < 0) { return; }
	ref ch = &Characters[iChr];
	ch.NetLandDead = true;
	DeleteAttribute(ch, "NetLandActId");
	DeleteAttribute(ch, "NetLandActUntil");
	ch.NetLandBlock = 0;
	ch.NetLandTask = 0;
	ch.NetLandKilled = 1;
	if (IsEntity(ch))
	{
		// the death task of the game: the body falls once and stays lying (the played action "death_1" repeated itself)
		if (iDeadVarR < 1) { iDeadVarR = 1; }
		NetLand_SetDeathVariant(ch, iDeadVarR);
		SetCharacterTask_Dead(ch);
	}
}

// the hero that has died by the death task is brought back by loading the location once more (a new body)
void NetLand_ReloadHero(int iSpawn)
{
	iNetLandSpawn = iSpawn;
	bNetLandDead = false;
	iNetLandGen++;				// the timers of the old load stop
	for (int i=0; i<NET_MAXPLAYERS; i++) { DeleteAttribute(&NCClients[i], "LandChr"); }	// the puppets go with the old location
	iNetLandPT = 0;
	iNetLandLastFlags = -1;
	PostEvent("NetLand_Load", 100, "l", iNetLandGen);
}

void NetLand_OnRespawn(int iMsg)
{
	int wID = NMGetClientID(iMsg);
	int iSpawn = NMGetByte(iMsg);
	if (!bNetLandActive) { return; }
	if (wID < 0 || wID >= NET_MAXPLAYERS) { return; }

	if (wID == wClientID)
	{
		ref mc = GetMainCharacter();
		LAi_SetHP(mc, LAi_GetCharacterMaxHP(mc), LAi_GetCharacterMaxHP(mc));
		CharacterPlayAction(mc, "");
		LAi_SetPlayerType(mc);
		NetLand_TeleportToSpawn(mc, iSpawn);
		iNetLandProtectUntil = Net_GetTickCount() + NET_RESPAWN_PROTECT;
		bNetLandDead = false;
		Log_SetStringToLog(LanguageConvertString(iLangNetClient, "Net_LandRespawned"));
		return;
	}

	int iChr = NetLand_GetPuppet(wID);
	if (iChr < 0) { return; }
	ref ch = &Characters[iChr];
	if (CheckAttribute(ch, "NetLandKilled"))
	{
		// a body that the game has killed does not get up: it is removed, the next state packet makes a new puppet
		DeleteAttribute(&NCClients[wID], "LandChr");
		ChangeCharacterAddressGroup(ch, "none", "", "");
		return;
	}
	ch.NetLandDead = false;
	ch.NetLandTask = 0;
	ch.NetLandBlock = 0;
	DeleteAttribute(ch, "NetLandActUntil");
	LAi_SetHP(ch, LAi_GetCharacterMaxHP(ch), LAi_GetCharacterMaxHP(ch));
	if (IsEntity(ch))
	{
		CharacterPlayAction(ch, "");
		LAi_SetActorType(ch);
		NetLand_TeleportToSpawn(ch, iSpawn);
	}
}

// client\OnClient.c: NetClient_Drop - the player left: his puppet goes away
void NetLand_PlayerLeft(int wID)
{
	if (!bNetLandActive) { return; }
	if (!CheckAttribute(&NCClients[wID], "LandChr")) { return; }
	int iChr = sti(NCClients[wID].LandChr);
	DeleteAttribute(&NCClients[wID], "LandChr");
	if (iChr >= 0) { ChangeCharacterAddressGroup(&Characters[iChr], "none", "", ""); }
}

// ================================================================================================
// COOP: land duel loadout ("Снаряжение" in the Esc menu): blade, pistol, charges, potions, level, PIRATES,
// personal skills and perks, paid from the money of the server (credit / 20). The other players get it too: their
// puppets hold the same weapons and have the same defence (the damage is counted on the machine of the attacker).
// ================================================================================================
object oNetLandLo;					// our loadout
object oNetLandLoDefault;			// a player who has not sent his loadout yet

#define NET_LAND_PERKS			13
#define NET_LAND_PRICE_MULT		20		// the budget is the whole credit of the server; the prices (were for credit / 20) go up by the same factor
#define NET_LAND_LO_ROWS		17		// rows of the loadout window (NetMenu.ini: TEXT_LO_0..16)

string NetLand_PerkName(int i)
{
	switch (i)
	{
		case 0: return "BasicDefense"; break;
		case 1: return "AdvancedDefense"; break;
		case 2: return "CriticalHit"; break;
		case 3: return "HPPlus"; break;
		case 4: return "HardHitter"; break;
		case 5: return "Tireless"; break;
		case 6: return "EnergyPlus"; break;
		case 7: return "Sliding"; break;
		case 8: return "BladeDancer"; break;
		case 9: return "SwordplayProfessional"; break;
		case 10: return "Gunman"; break;
		case 11: return "GunProfessional"; break;
		case 12: return "Rush"; break;
	}
	return "";
}

string NetLand_PerkTitle(int i)
{
	switch (i)
	{
		case 0: return "Основы защиты"; break;
		case 1: return "Продвинутая защита"; break;
		case 2: return "Критический удар"; break;
		case 3: return "Здоровье+"; break;
		case 4: return "Тяжелая рука"; break;
		case 5: return "Неутомимый"; break;
		case 6: return "Энергия+"; break;
		case 7: return "Скольжение"; break;
		case 8: return "Танец клинка"; break;
		case 9: return "Мастер клинка"; break;
		case 10: return "Стрелок"; break;
		case 11: return "Меткий стрелок"; break;
		case 12: return "Натиск (F)"; break;
	}
	return "";
}

bool NetLand_PerkIsActive(int i) { return (i == 12); }

string NetLand_SpecialName(int i)
{
	switch (i)
	{
		case 0: return SPECIAL_S; break;
		case 1: return SPECIAL_P; break;
		case 2: return SPECIAL_E; break;
		case 3: return SPECIAL_C; break;
		case 4: return SPECIAL_I; break;
		case 5: return SPECIAL_A; break;
		case 6: return SPECIAL_L; break;
	}
	return SPECIAL_S;
}

string NetLand_SpecialTitle(int i)
{
	switch (i)
	{
		case 0: return "Сила (P)"; break;
		case 1: return "Восприятие (I)"; break;
		case 2: return "Выносливость (R)"; break;
		case 3: return "Лидерство (A)"; break;
		case 4: return "Обучаемость (T)"; break;
		case 5: return "Ловкость (E)"; break;
		case 6: return "Удача (S)"; break;
	}
	return "";
}

string NetLand_SkillName(int i)
{
	switch (i)
	{
		case 0: return SKILL_F_LIGHT; break;
		case 1: return SKILL_FENCING; break;
		case 2: return SKILL_F_HEAVY; break;
		case 3: return SKILL_PISTOL; break;
		case 4: return SKILL_FORTUNE; break;
	}
	return SKILL_FENCING;
}

string NetLand_SkillTitle(int i)
{
	switch (i)
	{
		case 0: return "Легкое оружие"; break;
		case 1: return "Среднее оружие"; break;
		case 2: return "Тяжелое оружие"; break;
		case 3: return "Пистолеты"; break;
		case 4: return "Везение"; break;
	}
	return "";
}

// the perks every captain has by the switches of the server ("all passive" / "all active" perks)
bool NetLand_PerkForced(int i)
{
	string sSwitch = "PassivePerks";
	if (NetLand_PerkIsActive(i)) { sSwitch = "ActivePerks"; }
	if (!CheckAttribute(&NCServer, sSwitch)) { return false; }
	if (sti(NCServer.(sSwitch)) != 0) { return true; }
	return false;
}

bool NetLand_PerksChosen()
{
	if (CheckAttribute(&NCServer, "ActivePerks") && sti(NCServer.ActivePerks)) { return false; }
	if (CheckAttribute(&NCServer, "PassivePerks") && sti(NCServer.PassivePerks)) { return false; }
	return true;
}

void NetLand_LoDefaults(aref lo)
{
	int i;
	DeleteAttribute(lo, "");
	lo.Blade = NET_LAND_BLADE;
	lo.Gun = NET_LAND_PISTOL;
	if (iNetLandNoPistol) { lo.Gun = ""; }
	lo.Ammo = 3;
	lo.Potions = 3;
	lo.Level = 1;
	for (i=0; i<7; i++) { string sS = "s" + i; lo.SP.(sS) = 6; }
	for (i=0; i<5; i++) { string sK = "k" + i; lo.Sk.(sK) = 10; }
	for (i=0; i<NET_LAND_PERKS; i++) { string sP = "p" + i; lo.Perk.(sP) = 0; }
	lo.Inited = true;
}

void NetLand_LoInit()
{
	aref lo;
	makearef(lo, oNetLandLo);
	if (!CheckAttribute(lo, "Inited")) { NetLand_LoDefaults(lo); }
	if (iNetLandNoPistol) { lo.Gun = ""; }
	aref ld;
	makearef(ld, oNetLandLoDefault);
	NetLand_LoDefaults(ld);
}

// ---------- prices ----------
int NetLand_ItemPrice(string sId)
{
	if (sId == "") { return 0; }
	int iIdx = GetItemIndex(sId);
	if (iIdx < 0) { return 0; }
	if (!CheckAttribute(&Items[iIdx], "price")) { return 0; }
	return sti(Items[iIdx].price) * NET_LAND_PRICE_MULT;
}

int NetLand_LevelCost(int iLevel)
{
	int iCost = 0;
	int iStep = 500;
	for (int i=2; i<=iLevel; i++)
	{
		iCost = iCost + iStep;
		iStep = iStep * 2;
	}
	return iCost * NET_LAND_PRICE_MULT;
}

int NetLand_LoBudget()
{
	if (!CheckAttribute(&NCServer, "Credit")) { return 15000 * NET_LAND_PRICE_MULT; }
	return sti(NCServer.Credit);
}

int NetLand_LoCost(aref lo)
{
	int iCost = NetLand_ItemPrice(lo.Blade) + NetLand_ItemPrice(lo.Gun);
	if (lo.Gun != "") { iCost = iCost + sti(lo.Ammo) * 10 * (NetLand_ItemPrice("bullet") + NetLand_ItemPrice("GunPowder")); }
	iCost = iCost + sti(lo.Potions) * NetLand_ItemPrice("potion1");
	iCost = iCost + NetLand_LevelCost(sti(lo.Level));
	return iCost;
}

// ---------- points ----------
int NetLand_LoSpecialSum(aref lo)
{
	int iSum = 0;
	for (int i=0; i<7; i++) { string sS = "s" + i; iSum = iSum + sti(lo.SP.(sS)); }
	return iSum;
}

int NetLand_LoSkillFree(aref lo)
{
	int iUsed = 0;
	for (int i=0; i<5; i++) { string sK = "k" + i; iUsed = iUsed + sti(lo.Sk.(sK)) - 10; }
	return (sti(lo.Level) - 1) * 10 - iUsed;
}

int NetLand_LoPerkFree(aref lo)
{
	int iUsed = 0;
	for (int i=0; i<NET_LAND_PERKS; i++) { string sP = "p" + i; iUsed = iUsed + sti(lo.Perk.(sP)); }
	return (sti(lo.Level) - 1) - iUsed;
}

bool NetLand_LoHasPerk(aref lo, int i)
{
	if (NetLand_PerkForced(i)) { return true; }
	string sP = "p" + i;
	if (!CheckAttribute(lo, "Perk." + sP)) { return false; }
	return sti(lo.Perk.(sP)) != 0;
}

// the conditions of single player (perks_init.c): AdvancedDefense needs BasicDefense etc.
bool NetLand_LoPerkCondOk(aref lo, int i)
{
	string sName = NetLand_PerkName(i);
	if (!CheckAttribute(&ChrPerksList, "list." + sName + ".condition")) { return true; }
	aref arCond;
	makearef(arCond, ChrPerksList.list.(sName).condition);
	int iNum = GetAttributesNum(arCond);
	for (int c=0; c<iNum; c++)
	{
		aref arC = GetAttributeN(arCond, c);
		string sNeed = GetAttributeName(arC);
		bool bHave = false;
		for (int j=0; j<NET_LAND_PERKS; j++)
		{
			if (NetLand_PerkName(j) == sNeed && NetLand_LoHasPerk(lo, j)) { bHave = true; }
		}
		if (!bHave) { return false; }
	}
	return true;
}

// ---------- the window (net\interfaces\NetMenu.c) ----------
string NetLand_ItemTitle(string sId)
{
	if (sId == "") { return "нет"; }
	int iIdx = GetItemIndex(sId);
	if (iIdx < 0) { return sId; }
	return GetConvertStr(Items[iIdx].name, "ItemsDescribe.txt");
}

// next blade / pistol / musket of the single player items (the first ITEMS_QUANTITY: no generated copies)
string NetLand_NextItem(string sCur, string sGroup, int iDir, bool bAllowNone)
{
	int iCur = -1;
	if (sCur != "") { iCur = GetItemIndex(sCur); }
	int iIdx = iCur;
	for (int k=0; k<ITEMS_QUANTITY + 1; k++)
	{
		iIdx = iIdx + iDir;
		if (iIdx >= ITEMS_QUANTITY) { iIdx = -1; }
		if (iIdx < -1) { iIdx = ITEMS_QUANTITY - 1; }
		if (iIdx == -1)
		{
			if (bAllowNone) { return ""; }
			continue;
		}
		if (!CheckAttribute(&Items[iIdx], "groupID")) { continue; }
		if (Items[iIdx].groupID != sGroup) { continue; }
		string sId = Items[iIdx].id;
		if (sId == "unarmed" || sId == "rabble") { continue; }
		if (sId == "mushket2x2") { continue; }		// the quest musket stays a quest one
		return sId;
	}
	return sCur;
}

string NetLand_LoRowText(int iRow)
{
	aref lo;
	makearef(lo, oNetLandLo);
	switch (iRow)
	{
		case 0: return "Клинок: " + NetLand_ItemTitle(lo.Blade) + " (" + NetLand_ItemPrice(lo.Blade) + ")"; break;
		case 1:
			if (iNetLandNoPistol) { return "Пистолет: запрещены на сервере"; }
			return "Пистолет: " + NetLand_ItemTitle(lo.Gun) + " (" + NetLand_ItemPrice(lo.Gun) + ")";
		break;
		case 2: return "Пули и порох: " + (sti(lo.Ammo) * 10); break;
		case 3: return "Лечебные зелья: " + lo.Potions; break;
		case 4: return "Уровень: " + lo.Level + " (+1 = " + (NetLand_LevelCost(sti(lo.Level) + 1) - NetLand_LevelCost(sti(lo.Level))) + ")"; break;
	}
	if (iRow >= 5 && iRow < 10)
	{
		string sK = "k" + (iRow - 5);
		return NetLand_SkillTitle(iRow - 5) + ": " + lo.Sk.(sK);
	}
	if (iRow >= 10 && iRow < 17)
	{
		string sS = "s" + (iRow - 10);
		return NetLand_SpecialTitle(iRow - 10) + ": " + lo.SP.(sS);
	}
	return "";
}

string NetLand_LoInfoText()
{
	aref lo;
	makearef(lo, oNetLandLo);
	int iLeft = NetLand_LoBudget() - NetLand_LoCost(lo);
	string sText = "Деньги: " + iLeft + " из " + NetLand_LoBudget();
	sText = sText + ".  Очки PIRATES: " + (42 - NetLand_LoSpecialSum(lo));
	sText = sText + ".  Очки навыков: " + NetLand_LoSkillFree(lo);
	if (NetLand_PerksChosen()) { sText = sText + ".  Перки: " + NetLand_LoPerkFree(lo); }
	return sText;
}

string NetLand_LoHPText()
{
	object oTmp;
	oTmp.index = -1;
	aref lo;
	makearef(lo, oNetLandLo);
	NetLand_ApplyStats(&oTmp, lo);
	int iEnergy = 30 + sti(lo.SP.s5) * 10;
	return "Здоровье: " + makeint(NetLand_MaxHP(&oTmp, lo)) + ", энергия: " + iEnergy;
}

// -1 / +1 on a row; nothing happens if the money or the points do not allow it
void NetLand_LoStep(int iRow, int iDir)
{
	aref lo;
	makearef(lo, oNetLandLo);
	object oOld;
	CopyAttributes(&oOld, lo);

	switch (iRow)
	{
		case 0: lo.Blade = NetLand_NextItem(lo.Blade, BLADE_ITEM_TYPE, iDir, false); break;
		case 1:
			if (iNetLandNoPistol) { return; }
			lo.Gun = NetLand_NextItem(lo.Gun, GUN_ITEM_TYPE, iDir, true);
		break;
		case 2: lo.Ammo = sti(lo.Ammo) + iDir; if (sti(lo.Ammo) < 0) { lo.Ammo = 0; } if (sti(lo.Ammo) > 10) { lo.Ammo = 10; } break;
		case 3: lo.Potions = sti(lo.Potions) + iDir; if (sti(lo.Potions) < 0) { lo.Potions = 0; } if (sti(lo.Potions) > 10) { lo.Potions = 10; } break;
		case 4:
			lo.Level = sti(lo.Level) + iDir;
			if (sti(lo.Level) < 1) { lo.Level = 1; }
			if (sti(lo.Level) > 30) { lo.Level = 30; }
		break;
	}
	if (iRow >= 5 && iRow < 10)
	{
		string sK = "k" + (iRow - 5);
		lo.Sk.(sK) = sti(lo.Sk.(sK)) + iDir * 5;
		if (sti(lo.Sk.(sK)) < 10) { lo.Sk.(sK) = 10; }
		if (sti(lo.Sk.(sK)) > 100) { lo.Sk.(sK) = 100; }
	}
	if (iRow >= 10 && iRow < 17)
	{
		string sS = "s" + (iRow - 10);
		lo.SP.(sS) = sti(lo.SP.(sS)) + iDir;
		if (sti(lo.SP.(sS)) < 1) { lo.SP.(sS) = 1; }
		if (sti(lo.SP.(sS)) > SPECIAL_MAX) { lo.SP.(sS) = SPECIAL_MAX; }
	}

	// a level less: the skills and perks above it go away
	int iGuard = 0;
	int k = 4;
	while (NetLand_LoSkillFree(lo) < 0 && iGuard < 200)
	{
		iGuard++;
		string sKk = "k" + k;
		if (sti(lo.Sk.(sKk)) > 10) { lo.Sk.(sKk) = sti(lo.Sk.(sKk)) - 5; }
		else { k = k - 1; if (k < 0) { k = 4; } }
	}
	int p = NET_LAND_PERKS - 1;
	iGuard = 0;
	while (NetLand_LoPerkFree(lo) < 0 && iGuard < 100)
	{
		iGuard++;
		string sPp = "p" + p;
		if (sti(lo.Perk.(sPp))) { lo.Perk.(sPp) = 0; }
		p = p - 1;
		if (p < 0) { p = NET_LAND_PERKS - 1; }
	}

	bool bBad = false;
	if (NetLand_LoCost(lo) > NetLand_LoBudget()) { bBad = true; }
	if (NetLand_LoSpecialSum(lo) > 42) { bBad = true; }
	if (NetLand_LoSkillFree(lo) < 0) { bBad = true; }
	if (bBad)
	{
		DeleteAttribute(lo, "");
		CopyAttributes(lo, &oOld);
	}
}

string NetLand_LoPerkRowText(int i)
{
	aref lo;
	makearef(lo, oNetLandLo);
	string sMark = "   ";
	if (NetLand_LoHasPerk(lo, i)) { sMark = "[+] "; }
	return sMark + NetLand_PerkTitle(i);
}

void NetLand_LoPerkToggle(int i)
{
	if (i < 0 || i >= NET_LAND_PERKS) { return; }
	if (!NetLand_PerksChosen()) { return; }
	aref lo;
	makearef(lo, oNetLandLo);
	string sP = "p" + i;
	if (sti(lo.Perk.(sP)))
	{
		lo.Perk.(sP) = 0;
		// the perks that need this one go too
		for (int j=0; j<NET_LAND_PERKS; j++)
		{
			string sPj = "p" + j;
			if (sti(lo.Perk.(sPj)) && !NetLand_LoPerkCondOk(lo, j)) { lo.Perk.(sPj) = 0; }
		}
		return;
	}
	if (NetLand_LoPerkFree(lo) <= 0) { return; }
	if (!NetLand_LoPerkCondOk(lo, i)) { return; }
	lo.Perk.(sP) = 1;
}

// ---------- applying ----------
void NetLand_ApplyStats(ref chr, aref lo)
{
	int i;
	chr.rank = sti(lo.Level);
	SetSPECIAL(chr, sti(lo.SP.s0), sti(lo.SP.s1), sti(lo.SP.s2), sti(lo.SP.s3), sti(lo.SP.s4), sti(lo.SP.s5), sti(lo.SP.s6));
	SetSelfSkill(chr, sti(lo.Sk.k0), sti(lo.Sk.k1), sti(lo.Sk.k2), sti(lo.Sk.k3), sti(lo.Sk.k4));
	DeleteAttribute(chr, "perks.list");
	for (i=0; i<NET_LAND_PERKS; i++)
	{
		if (!NetLand_LoHasPerk(lo, i)) { continue; }
		string sName = NetLand_PerkName(i);
		chr.perks.list.(sName) = true;
	}
}

// HP like in single player: base by Strength * Endurance, + every level, HPPlus
float NetLand_MaxHP(ref chr, aref lo)
{
	float fHP = makefloat(GetCharacterBaseHPValue(chr) + (sti(lo.Level) - 1) * GetCharacterAddHPValue(chr));
	if (fHP < 10.0) { fHP = 10.0; }
	return fHP;
}

// bFull: the hero is healed to the new maximum (the start, a respawn, a change of the loadout of a hero that was unhurt)
void NetLand_ApplyLo(ref chr, aref lo, bool bHero, bool bFull)
{
	NetLand_ApplyStats(chr, lo);

	DeleteAttribute(chr, "equip");
	DeleteAttribute(chr, "items");
	bool bMusket = false;
	if (CheckAttribute(lo, "Gun") && !iNetLandNoPistol) { if (lo.Gun == "mushket") { bMusket = true; } }
	if (bMusket)
	{
		// the musketeer has no sword in the hand (single player: SetMainCharacterToMushketer)
		chr.model.animation = "mushketer";
		GiveItem2Character(chr, "unarmed");
		EquipCharacterByItem(chr, "unarmed");
	}
	else
	{
		GiveItem2Character(chr, lo.Blade);
		EquipCharacterByItem(chr, lo.Blade);
	}
	string sGun = lo.Gun;
	if (iNetLandNoPistol) { sGun = ""; }
	if (sGun != "")
	{
		GiveItem2Character(chr, sGun);
		EquipCharacterByItem(chr, sGun);
		if (bHero)
		{
			TakeNItems(chr, "bullet", sti(lo.Ammo) * 10);
			TakeNItems(chr, "GunPowder", sti(lo.Ammo) * 10);
		}
		else
		{
			TakeNItems(chr, "bullet", 50);
			TakeNItems(chr, "GunPowder", 50);
		}
		// EquipCharacterByItem leaves the gun of the hero unloaded: the gun is loaded now
		int iQ = 1;
		int iGunIdx = GetItemIndex(sGun);
		if (iGunIdx >= 0)
		{
			if (CheckAttribute(&Items[iGunIdx], "chargeQ")) { iQ = sti(Items[iGunIdx].chargeQ); }
		}
		LAi_GunSetChargeQuant(chr, iQ);
	}
	if (bHero) { TakeNItems(chr, "potion1", sti(lo.Potions)); }

	float fMax = NetLand_MaxHP(chr, lo);
	float fCur = fMax;
	if (!bFull)
	{
		if (CheckAttribute(chr, "chr_ai.hp") && IsEntity(chr) && stf(chr.chr_ai.hp) < fMax) { fCur = stf(chr.chr_ai.hp); }
	}
	LAi_SetHP(chr, fCur, fMax);
	SetEnergyToCharacter(chr);
}

void NetLand_LoApply()
{
	iNetLandProtectUntil = 0;
	aref lo;
	makearef(lo, oNetLandLo);
	if (NetLand_LoCost(lo) > NetLand_LoBudget()) { return; }

	if (bNetLandActive && !bNetLandDead)
	{
		ref mc = GetMainCharacter();
		float fHPNow = LAi_GetCharacterHP(mc);
		float fHPMaxNow = LAi_GetCharacterMaxHP(mc);
		bool bUnhurt = false;
		if (fHPNow >= fHPMaxNow - 1.0) { bUnhurt = true; }
		// a musket changes the model of the hero (a musketeer), a sword brings the old one back
		DeleteAttribute(&NCClients[wClientID], "LandModel");
		string sNewModel = NetLand_ModelFor(wClientID);
		if (mc.model != sNewModel)
		{
			mc.model = sNewModel;
			mc.model.animation = NetLand_ModelAnim(sNewModel);
			mc.model.height = NetLand_ModelHeight(sNewModel);
			mc.sex = NetLand_ModelSex(sNewModel);
			Characters_RefreshModel(mc);
		}
		NetLand_ApplyLo(mc, lo, true, bUnhurt);
	}
	NetLand_SendLoadout();
}

void NetLand_SendLoadout()
{
	if (!bNetLandActive) { return; }
	aref lo;
	makearef(lo, oNetLandLo);
	int iSMsg = NetClient_LandNewMsg(NSC_LAND_LOADOUT);
	NetLand_AddLoadoutData(iSMsg, lo);
	NetClient_LandSend(iSMsg, true);
	NMDelete(iSMsg);
}

void NetLand_AddLoadoutData(int iSMsg, aref lo)
{
	int i;
	NMAddString(iSMsg, lo.Blade, 24);
	NMAddString(iSMsg, lo.Gun, 24);
	NMAddByte(iSMsg, sti(lo.Level));
	for (i=0; i<7; i++) { string sS = "s" + i; NMAddByte(iSMsg, sti(lo.SP.(sS))); }
	for (i=0; i<5; i++) { string sK = "k" + i; NMAddByte(iSMsg, sti(lo.Sk.(sK))); }
	for (i=0; i<NET_LAND_PERKS; i++) { string sP = "p" + i; NMAddByte(iSMsg, sti(lo.Perk.(sP))); }
}

void NetLand_ReadLoadoutData(int iMsg, aref lo)
{
	int i;
	NetLand_LoDefaults(lo);
	lo.Blade = NMGetString(iMsg);
	lo.Gun = NMGetString(iMsg);
	lo.Level = NMGetByte(iMsg);
	for (i=0; i<7; i++) { string sS = "s" + i; lo.SP.(sS) = NMGetByte(iMsg); }
	for (i=0; i<5; i++) { string sK = "k" + i; lo.Sk.(sK) = NMGetByte(iMsg); }
	for (i=0; i<NET_LAND_PERKS; i++) { string sP = "p" + i; lo.Perk.(sP) = NMGetByte(iMsg); }
	if (GetItemIndex(lo.Blade) < 0) { lo.Blade = NET_LAND_BLADE; }
	if (lo.Gun != "" && GetItemIndex(lo.Gun) < 0) { lo.Gun = NET_LAND_PISTOL; }
}

// the server: the loadout of another player
void NetLand_OnLoadout(int iMsg)
{
	int wID = NMGetClientID(iMsg);
	object oLo;
	aref lo;
	makearef(lo, oLo);
	NetLand_ReadLoadoutData(iMsg, lo);
	if (wID < 0 || wID >= NET_MAXPLAYERS || wID == wClientID) { return; }

	DeleteAttribute(&NCClients[wID], "LandLo");
	NCClients[wID].LandLo = "";
	aref arDst;
	makearef(arDst, NCClients[wID].LandLo);
	CopyAttributes(arDst, lo);

	if (!bNetLandActive || !CheckAttribute(&NCClients[wID], "LandChr")) { return; }
	int iChr = sti(NCClients[wID].LandChr);
	if (iChr < 0) { return; }
	// a musket (or a sword again) is another model: the old puppet goes, the next state packet makes a new one
	DeleteAttribute(&NCClients[wID], "LandModel");
	string sPupModel = NetLand_ModelFor(wID);
	if (Characters[iChr].model != sPupModel)
	{
		DeleteAttribute(&NCClients[wID], "LandChr");
		ChangeCharacterAddressGroup(&Characters[iChr], "none", "", "");
		return;
	}
	NetLand_ApplyLo(&Characters[iChr], arDst, false, true);
}

// the loadout of a player for his puppet (the default one until he sends his own)
void NetLand_ApplyPuppetLo(ref ch, int wID)
{
	aref lo;
	if (CheckAttribute(&NCClients[wID], "LandLo.Blade")) { makearef(lo, NCClients[wID].LandLo); }
	else { makearef(lo, oNetLandLoDefault); }
	NetLand_ApplyLo(ch, lo, false, true);
}

// damage of a blade like LAi_ApplyCharacterAttackDamage: critical hits, Rush, defence perks of the victim
float NetLand_BladeDamage(aref attack, aref enemy, string attackType, bool isBlocked)
{
	float fDmg = LAi_CalcDamageForBlade(attack, enemy, attackType, isBlocked);
	float fCritical = 0.0;
	if (!isBlocked)
	{
		if (IsCharacterPerkOn(attack, "SwordplayProfessional")) { if (rand(100) <= 15) { fCritical = 1.0; } }
		else { if (IsCharacterPerkOn(attack, "CriticalHit")) { if (rand(100) <= 5) { fCritical = 1.0; } } }
	}
	if (fCritical > 0.0 && NetLand_IsOurHero(attack)) { Log_SetStringToLog(XI_ConvertString("Critical Hit")); }
	if (IsCharacterPerkOn(attack, "Rush")) { fDmg = fDmg * 3.0; }
	if (IsCharacterPerkOn(enemy, "Rush")) { fDmg = fDmg * 0.5; }
	float fDef = 1.0;
	if (IsCharacterPerkOn(enemy, "BasicDefense")) { fDef = 0.9; }
	if (IsCharacterPerkOn(enemy, "AdvancedDefense")) { fDef = 0.8; }
	if (IsCharacterPerkOn(enemy, "SwordplayProfessional")) { fDef = 0.7; }
	return fDmg * fDef * (1.0 + fCritical);
}


// ---------- the list of the servers: the map of a land duel is its location with the loading screen of it ----------
// server/OnNetMessage.c, server/ServerInfo.c: the "island" that a server shows
string NetServer_AdvertisedIsland()
{
	if (NetServer_LandMode())
	{
		string sAdLoc = "";
		if (CheckAttribute(&NetServer, "LandLoc")) { sAdLoc = NetServer.LandLoc; }
		if (sAdLoc != "") { return "LAND:" + sAdLoc; }
	}
	return NetServer.IslandID;
}

bool NetLand_IsAdvert(string sIsland)
{
	if (findSubStr(sIsland, "LAND:", 0) == 0) { return true; }
	return false;
}

string NetLand_AdvertLoc(string sIsland)
{
	if (strlen(sIsland) <= 5) { return ""; }
	return strcut(sIsland, 5, strlen(sIsland) - 1);
}

string NetLand_LoadingImage(string sLoc)
{
	int iLocIdx = FindLocation(sLoc);
	if (iLocIdx >= 0)
	{
		if (CheckAttribute(&Locations[iLocIdx], "image")) { return Locations[iLocIdx].image; }
	}
	return "loading\\Town.tga";
}

string NetLand_LocName(string sLoc)
{
	int iLocIdx = FindLocation(sLoc);
	string sName = sLoc;
	if (iLocIdx >= 0)
	{
		if (CheckAttribute(&Locations[iLocIdx], "id.label")) { sName = GetConvertStr(Locations[iLocIdx].id.label, "LocLables.txt"); }
	}
	int iUs = findSubStr(sLoc, "_", 0);
	if (iUs > 0) { sName = strcut(sLoc, 0, iUs - 1) + ": " + sName; }
	return sName;
}
