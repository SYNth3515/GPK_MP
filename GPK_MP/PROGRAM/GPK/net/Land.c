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
#define NET_LAND_GROUP_ALLY		"NetLandPuppetsA"	// COOP: team duel: the puppets of the players of our team (friends of our hero)
#define NET_LAND_HP				150.0
#define NET_LAND_BLADE			"blade5"
#define NET_LAND_PISTOL			"pistol1"
#define NET_LAND_TICK			10		// ms between the state packets of our hero (was 120)
#define NET_LAND_SMOOTH			8		// ms between the moves of the puppets on the screen
#define NET_LAND_BOT_CORPSE_MS	60000	// a dead bot (the bots do not come back) is removed with its body after this
#define NET_LAND_MUSH_HAND		2		// "GunBelt" for the musket of a skin without a musketeer twin: 2 the right hand (patched dlls), 0 the left one
// the turn of that musket in the right hand, degrees around the axes of the locator of the sword (-180..180). The chat command
// "/mush X Y Z" changes it at once on this machine (to find the angles); the numbers go here then
#define NET_LAND_MUSH_RX		25		// found in the game with /mush 25 12 3
#define NET_LAND_MUSH_RY		12
#define NET_LAND_MUSH_RZ		3
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
#define NET_LAND_BLOCK_GRACE		250		// ms: SPACE let go this long ago still blocks a blow whose packet was late
#define NET_LAND_T_PARRY			1200
#define NET_LAND_HOP_BUSY		480		// ms: the hero cannot start a new hop while one plays (recoil ~470, the side hops ~300-400)
#define NET_LAND_REACH			3.2		// m: a blade hit does not reach a hero that has moved farther away (a hop back)
#define NET_LAND_SNAP			2.0		// m: farther than this the puppet is put on the place at once
#define NET_LAND_T_HIT			520		// ms: a hit reaction that the script plays on our hero itself (CharacterPlayAction never ends by itself)
#define NET_LAND_T_BLOCKHIT		420		// ms: the same for a blocked blow
#define NET_LAND_BLOCK_HOLD		0		// 1: a puppet whose player holds the block is asked for the block state again (the animation of the block restarted: "it jitters")
#define NET_LAND_NATIVE_REACT	1		// 1: a hit / a blow in the block / a throw back by a dodge are played on our hero by the game's own fight state (8, 14, 10):
										// the game plays it with its sound and the player cannot break it with a block or a swing (as in single player)
#define NET_LAND_T_STAGGER		700		// ms: the attacker thrown back by a dodge (hit_parry) if the game does not take the state
#define NET_LAND_FACE_RUN		1		// 1: on W / S with A / D our hero sends the angle of his model (turned along the way), not of the camera
#define NET_LAND_SKIP_SPEED		2500	// %: the game's hit reaction on a puppet that blocked / dodged is played this fast (it cannot be cut)
#define NET_LAND_BLOCK_POSE		0		// 1: while its player blocks, a puppet stands in the game's own state 14 frozen at its first frame:
										// to the game it blocks, so a blow into it is blocked by the game itself (no hit reaction, no cry, no lost energy)
#define NET_LAND_PRE_BLOCK		1		// 1: a puppet that blocks gets the game's state 14 a moment before the blow of our hero reaches it:
										// the game blocks the blow itself (no hit reaction, no cry); else the block is resolved by the script
#define NET_LAND_PRE_LEAD		90		// ms before the expected touch of the blade
#define NET_LAND_ACK_WAIT		700		// ms: our blow has put a puppet into its hit reaction; until its player tells that the hit has
										// reached him (NSC_LAND_ANIM), what he does is from before it (it is broken there by our blow)
#define NET_LAND_MIN_DIST		0.75	// m: a puppet is never put nearer to our hero (the lunge of a swing pushed our hero away)
#define NET_LAND_PRED_HOLD		350		// ms: a reaction of a puppet that our machine shows at once is not taken back by the old animation still on the way

bool bNetLandSavedReload = false;
// COOP: team duel (2 teams): the score of the teams (kills of one team over the other), the end of the match (ms tick, 0 = no limit)
int iNetLandTeamKills0 = 0;
int iNetLandTeamKills1 = 0;
int iNetLandMatchEnd = 0;
int iNetLandTimerGen = 0;
int iNetLandAnchorA = -1;			// team spawns: the two spawn locators that are the farthest from each other (found once per location)
int iNetLandAnchorB = -1;
string sNetLandAnchorLoc = "";
string sNetLandEntryGroup = "rld";		// where the hero came into the location: the puppets are put there as well
string sNetLandEntryLocator = "loc0";
int iNetLandProtectUntil = 0;			// nobody can hurt a player who has not chosen his equipment yet
int iNetLandFReqCounter = 0;
bool bNetLandFStateOk = false;			// the patched dll has GetFightState / SetFightState: the fight state machines are mirrored
int iNetLandFSentState = -1;
int iNetLandFSentSub = -1;
bool bNetLandFrozen = false;
bool bNetLandDeadTask = false;			// our hero was killed by the death task of the game (a body that cannot get up: the respawn loads the location again)
bool bNetLandRespawnLoad = false;		// the load that is going on is a respawn, not the start of the duel
int iNetLandReactId = 0;				// ids of the reactions that the script plays on our hero (for the stop timer)
int iNetLandBlkLog = 0;
int iNetLandStunUntil = 0;				// a reaction of our hero played by the script (the game did not take its state): the fight controls are locked until then
int iNetLandStunLvl = 0;				// 1 the swings are locked, 2 the swings and the block
int iNetLandLockLvl = 0;				// what is locked now (NetLand_LockFight)
int iNetLandReactChk = 0;				// id of the last native reaction (its check)
int iNetLandReactLog = 0;
int iNetLandPupLog = 0;
bool bNetLandEndActOk = false;			// the game tells when a played animation of a puppet ends (the clear timer is only a safety then)
int iNetLandHitD[12];					// ms from the click to the first touch of the blade, learned for every kind of swing (NetLand_BladeHit)
int iNetLandSwingAt = 0;				// the click of the last swing of our hero
int iNetLandSwingHit = 0;				// its first touch has been measured
int iNetLandFaceLock[4];				// the side of the turn on W+A, W+D, S+A, S+D is known (measured once on a free run)
bool bNetLandBreakOk = false;			// Location.dll knows "BreakFight": the game's own break of an action (the swing broken by a block)
bool bNetLandHoldOk = false;			// Location.dll knows "SetBlockHold": a puppet holds the block by the game itself, like the player
bool bNetLandDuelOk = false;			// Location.dll knows "NetDuel": our hero (Blaze) plays every hit reaction and every blow in the
										// block, like the puppets (single player skips half of the hit reactions of the player and a
										// third of his blows in the block at random: the puppet on the other screen showed them)
bool bNetLandNoFightOk = false;			// Location.dll knows "NoFightHit": the game's own hit of a character without the sword out
int iNetLandAckFrom = -1;				// the last hit that has come to our hero: its attacker and its sequence byte
int iNetLandAckSeq = 0;
int iNetLandAckAt = 0;
bool bNetLandAckPub = true;				// it is in our animation packets already
int iNetLandAckSentFrom = -1;
int iNetLandAckSentSeq = 0;
bool bNetLandLocReady = false;			// the location of the duel is loaded: the packets of the others are used (they are read always)
object oNetLandAnimDur;				// learned lengths of the one-shot animations (ms), seen on the puppets
bool bNetLandAnimOk = false;			// the patched Location.dll (message "GetAnim") is there: the animation of the hero is sent as it is
string sNetLandAnimSent0 = "";
string sNetLandAnimSent1 = "";
string sNetLandAnimSent2 = "";
int iNetLandAnimSentAt = 0;
int iNetLandPrelimAt = 0;
int iNetLandSwingBusyUntil = 0;			// the animation of the last swing is on: the game ignores the clicks
float fNetLandSpEMA = 0.0;
bool bNetLandActive = false;		// this client is in a land duel
// COOP s37: the real boarding (net\Board.c): the globals are here (Land.c is compiled first)
bool bNetBoardDuel = false;		// client: our captain fights on a boarding deck (the sea waits behind it)
int iNetBoardTeam = 0;			// client: 0 the attacker, 1 the defender
int wNetBoardEnemy = -1;		// client: the other captain
int iNetBoardUpTeam = 0;		// client: the team of the bigger ship (it stands at the "aloc" locators)
int iNetSrvBDScope = 0;			// server: the boarding duel of the land message that is being handled (0: none / the land mode)
bool bNetLandDead = false;			// our hero is dead (waits for the respawn)
bool bNetLandOpenLoadout = false;	// net\interfaces\NetMenu.c: open the loadout window at once
string sNetLandLoc = "";
int iNetLandSpawn = 0;
int iNetLandGen = 0;
int iNetLandRespawn = 0;
bool bNetLandUiLoSync = false;
bool bNetBuyProfile = false;		// the lobby screen (Buy.c) edits the ship of a profile, without a server
string sNetBuyProfNick = "";
bool bNetLandUiLo = false;		// the window is open in the current interface
bool bNetLandUiLoReg = false;
string sNetLandProfNick = "";		// the profile edited in the profile screen (PlayerSettings.c)
int iNetLandBudgetOverride = 0;		// the money of the profile screen (1 000 000)
#define NET_LAND_POISON_PRICE	5000	// COOP s29: 20000 -> 5000.// the poison on the blade (the loadout): a blow that hits poisons with the chance below
#define NET_LAND_POISON_CHANCE	15		// % (single player: the skeletons, 15%)
int iNetLandStartAt = 0;
string sNetLandWeather = "";		// the weather of the server: its hour and kind for the location of the duel
float fNetLandSpeed = 1.0;				// the speed of the game in the duel (the admin menu, the Create game screen): kept and set again
int iNetLandSpeedAt = 0;				// the last time it was set again (something of the game sets 1x back)
bool bNetLandPoisonDeath = false;		// our hero dies by the poison: the kill line says it
int iNetLandPoisonUntil = 0;		// our hero is poisoned until then (his hero is immortal: the poison of the game does nothing)
int iNetLandPoisonBy = DST_INVALID;
int iNetLandPoisonAt = 0;
float fNetLandPoisonAcc = 0.0;
int iNetLandRespawnSec = 0;
int iNetLandNoPistol = 0;
float fNetLandPX = 0.0;				// the last sent position (speed of our hero)
float fNetLandPZ = 0.0;
int iNetLandPT = 0;
float fNetLandFaceOff[4];				// the turn of the model of our hero on W+A, W+D, S+A, S+D (rad), fixed (NetLand_OnStart)
float fNetLandVX = 0.0;				// the last measured speed of our hero
float fNetLandVZ = 0.0;
int iNetLandLastSend = 0;
float fNetLandLastAy = 0.0;
int iNetLandLastFlags = -1;
int iNetLandLastHP = -1;
int iNetLandLastEn = -1;
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
int iNetLandMushRX = NET_LAND_MUSH_RX;	// the turn of the musket in the right hand (the chat command "/mush X Y Z")
int iNetLandMushRY = NET_LAND_MUSH_RY;
int iNetLandMushRZ = NET_LAND_MUSH_RZ;
int iNetLandSwimLog = 0;
// the swim of the puppets, changed by the chat command "/swim A K L" (to find what is smooth):
// A 1 = the puppet plays the animation of its player by name, 0 = the game animates the swimmer itself;
// K the part of the way to its player per frame (percent); L 1 = it runs ahead of the packets by the speed, 0 = no
int iNetLandSwimPlay = 0;
int iNetLandSwimK = 100;			// found in the game: /swim 0 100 0
int iNetLandSwimLead = 0;
int iNetLandSwimDiag = 0;
int iNetLandSwimLogM = 0;			// the lines of the close look at the swimmers in the log
int iNetLandSentN = 0;				// the state packets of our hero (a line into the log once in 3 s)
int iNetLandSentMovN = 0;
int iNetLandSentAt = 0;
int iNetLandHitSeq = 0;			// sequence byte of the blows we send (the packets go twice)
int iNetLandDeadAt = 0;			// tick of our death (the weapon going away must not be taken for the end of the fall)
int iNetLandDeadReplay = 0;
int iNetLandShotAt = 0;
int iNetLandSwingPending = 0;		// a swing was announced and the hit of it has not come yet
bool bNetLandRunHeld = false;			// Shift (ChrRun) of our hero is down
int iNetLandSpeedLog = 0;
int iNetLandParryUntil = 0;			// Shift+RMB (ChrParry): a parry stands until then
int iNetLandFixN = 0;
int iNetLandLastStart = 0;			// the last swing was announced at its start (ChrAttackAction)
int iNetLandPupGen = 0;
int iNetLandBlockRelAt = 0;			// tick when SPACE was let go (a blow that left before that still meets the block)
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
// of INTERFACES/PORTRAITS have a model here. COOP: only models that exist in RESOURCE\MODELS\Characters: the ladies are
// Lady01..Lady06 on the disk (lady_N of FaceMaker is not there), 164..167 are horse01..04 (the same portraits in the single
// player; whore_1..4 have a skeleton of 44 bones and no fight animation), beatrice / lady_5 / Daniel / Corsair1_3 / pirat00
// have no model at all. A pirate (by the slot of the player) is only for an icon that has none
// (a NET/FACES icon, the empty icon, a number without a model).
string NetLand_FaceToModel(int iFace)
{
	switch (iFace)
	{
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
		case 63: return "Lady01"; break;
		case 64: return "lady02"; break;
		case 65: return "Lady03"; break;
		case 66: return "lady04"; break;
		case 68: return "Lady06"; break;
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
		case 164: return "horse01"; break;
		case 165: return "horse02"; break;
		case 166: return "horse03"; break;
		case 167: return "horse04"; break;
		case 168: return "Researcher"; break;
		case 198: return "Corsair1_2"; break;
		case 199: return "AnnaDeLeiva"; break;
		case 200: return "Albermal"; break;
		case 202: return "capitan"; break;
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

// the skin of a character: the model without the musket ending (a puppet / the hero keep it in NetLandSkin)
string NetLand_SkinOf(ref chr)
{
	if (CheckAttribute(chr, "NetLandSkin")) { return chr.NetLandSkin; }
	return NetLand_BaseModel(chr.model);
}

// the player of a character: a puppet knows it, our hero is us
int NetLand_ChrPlayer(ref chr)
{
	if (CheckAttribute(chr, "NetLandID")) { return sti(chr.NetLandID); }
	return wClientID;
}

// only these models have a "<model>_mush" twin on the disk (the rule of the single player: IsPCharHaveMushketerModel).
// Any other "<model>_mush" is a file that does not exist: the model is not loaded and the player is gone
bool NetLand_HasMushTwin(string sModel)
{
	if (findSubStr(sModel, "Blad", 0) >= 0) { return true; }
	if (findSubStr(sModel, "Pat", 0) >= 0) { return true; }
	if (findSubStr(sModel, "Resc", 0) >= 0) { return true; }
	if (findSubStr(sModel, "Port", 0) >= 0) { return true; }
	return false;
}

// the model of a player with a musket: his own twin if there is one, else his own skin (the animation of the musketeer, the
// musket of the gun is put into the hand: NetLand_MushHand)
string NetLand_MushModel(string sSkin, int wID)
{
	if (NetLand_HasMushTwin(sSkin)) { return sSkin + "_mush"; }
	if (sSkin == "MusketeerEnglish_1") { return "MusketeerEnglish_2"; }		// COOP s35: the same mesh with the musket locators
	return sSkin;
}

// COOP s35: the model of a musketeer skin with a blade and a pistol. Its own .gm has every hand locator on the right wrist (the
// musket): the sword hung on the belt, the pistol sat on the musket. "<skin>_blade.gm" = the same mesh with the locators of a
// man (_backup_s35\tools\make_blade_twins.py); MusketeerEnglish_1 is that twin of MusketeerEnglish_2
string NetLand_BladeModel(string sSkin)
{
	if (sSkin == "MusketeerEnglish_2") { return "MusketeerEnglish_1"; }
	if (findSubStr(sSkin, "Mushketer_", 0) == 0 && strlen(sSkin) == 11) { return sSkin + "_blade"; }
	if (findSubStr(sSkin, "quest_mush_", 0) == 0 && strlen(sSkin) == 12) { return sSkin + "_blade"; }
	// the musketeers of the nations eng/fra/hol/spa_mush_1..3: the same locators of the musket
	if (findSubStr(sSkin, "_mush_", 0) == 3 && strlen(sSkin) == 10) { return sSkin + "_blade"; }
	return sSkin;
}

// a skin that is a musketeer model itself (the musket sits in its hands by its own locators)
bool NetLand_IsMushSkin(string sSkin)
{
	if (findSubStr(sSkin, "mush", 0) >= 0) { return true; }
	if (findSubStr(sSkin, "Mush", 0) >= 0) { return true; }
	if (findSubStr(sSkin, "Musket", 0) >= 0) { return true; }
	return false;
}

// a skin without a musketeer twin: the musket (the gun of the character) is on its belt locator, the animation of the musketeer
// holds nothing. The gun goes to the RIGHT hand: "GunBelt" 2 (the patched Location.dll sends the message 0x5213 to blade.dll, that
// draws the gun on the locator of the sword, "Saber_hand"); "GunBelt" 0 put it into the left hand like a pistol. Asked again now
// and then: an event of an animation ("To belt" / "To hand") puts it back. The single player never sends 2: nothing changes there
void NetLand_MushHand(ref chr)
{
	if (!IsEntity(chr)) { return; }
	if (!CheckAttribute(chr, "NetLandMushTie")) { return; }
	if (sti(chr.NetLandMushTie) != 1) { return; }
	if (!CheckAttribute(chr, "model.animation")) { return; }
	if (chr.model.animation != "mushketer") { return; }
	if (CheckAttribute(chr, "NetLandDead")) { if (sti(chr.NetLandDead)) { NetLand_MushUntie(chr); return; } }
	if (CheckAttribute(chr, "NetLandBotDead")) { if (sti(chr.NetLandBotDead)) { NetLand_MushUntie(chr); return; } }
	if (sti(chr.index) == GetMainCharacterIndex()) { if (bNetLandDead) { return; } }		// a dead hero: nothing more to his body
	int iBelt = NET_LAND_MUSH_HAND;
	if (iBelt == 2) { iBelt = NetLand_MushCodeKey(NetLand_MushKey(chr)); }		// COOP s32: the angles of its skeleton
	SendMessage(chr, "lsl", MSG_CHARACTER_EX_MSG, "GunBelt", iBelt);
}

int NetLand_MushAngle(int iA)
{
	while (iA > 180) { iA = iA - 360; }
	while (iA < -180) { iA = iA + 360; }
	return iA;
}

int NetLand_MushCode()
{
	int iX = NetLand_MushAngle(iNetLandMushRX) + 256;
	int iY = NetLand_MushAngle(iNetLandMushRY) + 256;
	int iZ = NetLand_MushAngle(iNetLandMushRZ) + 256;
	return 1073741824 + iX * 262144 + iY * 512 + iZ;
}

// COOP s32: the turn of the musket for each skeleton. The label "Saber_hand" of Jessika, Yoko, Angellica, whore_1..4 and
// DSolovey is turned otherwise than the one of a man, and their bind pose is other: one angle for all put the musket across
// the hip. The defaults: the musket stands in the world as on a man with NET_LAND_MUSH_R* (_backup_s32\tools\mush_angles.py).
// The key is the animation file of the musket (model.animfile, "mush_<x>"), "man" = mushketer.an (the globals above).
// "/mush X Y Z" sets the angles of the skeleton of our hero on this machine (the screen says which)
// COOP s33: Yoko / Angellica 210 210 90 (found in the game by the user); Jessika, whore, man_kvl2 had the same computed angles
object oNetLandMushAng;
int iNetLandMushAX = 0;
int iNetLandMushAY = 0;
int iNetLandMushAZ = 0;

string NetLand_MushKey(ref chr)
{
	if (CheckAttribute(chr, "model.animfile"))
	{
		if (chr.model.animfile != "") { return chr.model.animfile; }
	}
	return "man";
}

// the angles of a key into iNetLandMushAX / AY / AZ
void NetLand_MushAngGet(string sKey)
{
	iNetLandMushAX = iNetLandMushRX;
	iNetLandMushAY = iNetLandMushRY;
	iNetLandMushAZ = iNetLandMushRZ;
	if (sKey == "man" || sKey == "" || sKey == "mushketer") { return; }
	if (CheckAttribute(&oNetLandMushAng, sKey + ".x"))
	{
		iNetLandMushAX = sti(oNetLandMushAng.(sKey).x);
		iNetLandMushAY = sti(oNetLandMushAng.(sKey).y);
		iNetLandMushAZ = sti(oNetLandMushAng.(sKey).z);
		return;
	}
	switch (sKey)
	{
		case "mush_woman":		iNetLandMushAX = 27; iNetLandMushAY = 10; iNetLandMushAZ = -1; break;
		case "mush_Danielle":	iNetLandMushAX = 27; iNetLandMushAY = 10; iNetLandMushAZ = -1; break;
		case "mush_towngirl2":	iNetLandMushAX = 14; iNetLandMushAY = -9; iNetLandMushAZ = -7; break;
		case "mush_man2":		iNetLandMushAX = 21; iNetLandMushAY = 13; iNetLandMushAZ = 5; break;
		case "mush_Jessika":	iNetLandMushAX = 205; iNetLandMushAY = 215; iNetLandMushAZ = 90; break;	// COOP s35: the user
		case "mush_YokoDias":	iNetLandMushAX = 210; iNetLandMushAY = 210; iNetLandMushAZ = 90; break;
		case "mush_Angellica":	iNetLandMushAX = 210; iNetLandMushAY = 210; iNetLandMushAZ = 90; break;
		case "mush_man_kvl2":	iNetLandMushAX = 210; iNetLandMushAY = 210; iNetLandMushAZ = 90; break;
		case "mush_whore":		iNetLandMushAX = 210; iNetLandMushAY = 210; iNetLandMushAZ = 90; break;
	}
}

int NetLand_MushCodeKey(string sKey)
{
	NetLand_MushAngGet(sKey);
	int iX = NetLand_MushAngle(iNetLandMushAX) + 256;
	int iY = NetLand_MushAngle(iNetLandMushAY) + 256;
	int iZ = NetLand_MushAngle(iNetLandMushAZ) + 256;
	return 1073741824 + iX * 262144 + iY * 512 + iZ;
}

// a whole number of a chat command: the part from iFrom to the next space (a minus is allowed); iNetLandCmdPos is after it
int iNetLandCmdPos = 0;
int NetLand_CmdNum(string sCmd, int iFrom, int iDef)
{
	int iLen = strlen(sCmd);
	int iP = iFrom;
	while (iP < iLen)
	{
		if (strcut(sCmd, iP, iP) != " ") { break; }
		iP++;
	}
	if (iP >= iLen) { iNetLandCmdPos = iLen; return iDef; }
	int iSign = 1;
	if (strcut(sCmd, iP, iP) == "-") { iSign = -1; iP++; }
	int iVal = 0;
	bool bAny = false;
	while (iP < iLen)
	{
		string sD = strcut(sCmd, iP, iP);
		int iDig = findSubStr("0123456789", sD, 0);
		if (iDig < 0) { break; }
		iVal = iVal * 10 + iDig;
		bAny = true;
		iP++;
	}
	iNetLandCmdPos = iP;
	if (!bAny) { return iDef; }
	return iVal * iSign;
}

// net\interfaces\ChatString.c: "/mush X Y Z" turns the musket in the right hand on this machine (it is not sent to the chat)
bool NetLand_ChatCmd(string sMsg)
{
	if (findSubStr(sMsg, "/getmypos", 0) == 0) { NetLand_GetMyPos(); return true; }		// COOP s27
	if (findSubStr(sMsg, "/spawn", 0) == 0) { return NetServer_SpawnLineCmd(sMsg); }
	if (findSubStr(sMsg, "/sea", 0) == 0)		// COOP s30: the detail of the waves (client\weather\Sea.c)
	{
		iNetLandCmdPos = 4;
		NetClient_SeaDetailCmd(NetLand_CmdNum(sMsg, iNetLandCmdPos, iNetSeaDetail));
		return true;
	}		// COOP s30: the spawn line (server\OnStart.c)
	if (findSubStr(sMsg, "/swim", 0) == 0)
	{
		iNetLandCmdPos = 5;
		iNetLandSwimPlay = NetLand_CmdNum(sMsg, iNetLandCmdPos, iNetLandSwimPlay);
		iNetLandSwimK = NetLand_CmdNum(sMsg, iNetLandCmdPos, iNetLandSwimK);
		iNetLandSwimLead = NetLand_CmdNum(sMsg, iNetLandCmdPos, iNetLandSwimLead);
		if (iNetLandSwimPlay != 0) { iNetLandSwimPlay = 1; }
		if (iNetLandSwimLead != 0) { iNetLandSwimLead = 1; }
		if (iNetLandSwimK < 5) { iNetLandSwimK = 5; }
		if (iNetLandSwimK > 100) { iNetLandSwimK = 100; }
		iNetLandSwimDiag = 0;		// a new close look at the next swim
		string sSwInfo = "Плавание: анимация " + iNetLandSwimPlay + ", K " + iNetLandSwimK + ", опережение " + iNetLandSwimLead;
		Log_SetStringToLog(sSwInfo);
		Trace("COOP: land swim settings: play " + iNetLandSwimPlay + ", k " + iNetLandSwimK + ", lead " + iNetLandSwimLead);
		return true;
	}
	if (findSubStr(sMsg, "/skin", 0) == 0) { NetLand_SkinCmd(sMsg); return true; }		// COOP s30
	if (findSubStr(sMsg, "/mush", 0) != 0) { return false; }
	iNetLandCmdPos = 5;
	// COOP s32: the angles of the skeleton of our hero
	ref mcMush = GetMainCharacter();
	string sMKey = NetLand_MushKey(mcMush);
	NetLand_MushAngGet(sMKey);
	int iMX = NetLand_MushAngle(NetLand_CmdNum(sMsg, iNetLandCmdPos, iNetLandMushAX));
	int iMY = NetLand_MushAngle(NetLand_CmdNum(sMsg, iNetLandCmdPos, iNetLandMushAY));
	int iMZ = NetLand_MushAngle(NetLand_CmdNum(sMsg, iNetLandCmdPos, iNetLandMushAZ));
	if (sMKey == "man")
	{
		iNetLandMushRX = iMX;
		iNetLandMushRY = iMY;
		iNetLandMushRZ = iMZ;
	}
	else
	{
		oNetLandMushAng.(sMKey).x = iMX;
		oNetLandMushAng.(sMKey).y = iMY;
		oNetLandMushAng.(sMKey).z = iMZ;
	}
	string sInfo = "Мушкет (" + sMKey + "): X " + iMX + ", Y " + iMY + ", Z " + iMZ;
	Log_SetStringToLog(sInfo);
	Trace("COOP: land the musket in the right hand is turned: " + sMKey + " " + iMX + " " + iMY + " " + iMZ);
	NetLand_MushHand(mcMush);
	return true;
}

// COOP s27: "/getmypos": the place and the angles of our hero (land) or of our ship (sea) in the log on the screen and in compile.log
void NetLand_GetMyPos()
{
	float x = 0.0;
	float y = 0.0;
	float z = 0.0;
	float ax = 0.0;
	float ay = 0.0;
	float az = 0.0;
	string sWhat = "";
	if (NetLand_IsActive())
	{
		ref mc = GetMainCharacter();
		if (!IsEntity(mc)) { return; }
		GetCharacterPos(mc, &x, &y, &z);
		GetCharacterAy(mc, &ay);
		sWhat = "Герой";
	}
	else
	{
		if (!bNetClientInGame) { Log_SetStringToLog("/getmypos: вы не в игре"); return; }
		ref rMe = NetClient_GetClient(wClientID);
		if (!CheckAttribute(rMe, "Ship.Pos.x")) { return; }
		x = stf(rMe.Ship.Pos.x);
		y = stf(rMe.Ship.Pos.y);
		z = stf(rMe.Ship.Pos.z);
		if (CheckAttribute(rMe, "Ship.Ang.x")) { ax = stf(rMe.Ship.Ang.x); }
		if (CheckAttribute(rMe, "Ship.Ang.y")) { ay = stf(rMe.Ship.Ang.y); }
		if (CheckAttribute(rMe, "Ship.Ang.z")) { az = stf(rMe.Ship.Ang.z); }
		sWhat = "Корабль";
	}
	// whole centimetres / thousandths of a radian: a float in a string gives 7 digits here
	string sPos = sWhat + ": X " + NetLand_Num2(x) + ", Y " + NetLand_Num2(y) + ", Z " + NetLand_Num2(z);
	string sAng = "Углы: X " + NetLand_Num3(ax) + ", Y " + NetLand_Num3(ay) + " (" + makeint(ay * 180.0 / PI) + " гр), Z " + NetLand_Num3(az);
	Log_SetStringToLog(sPos);
	Log_SetStringToLog(sAng);
	Trace("COOP: getmypos " + sWhat + " x=" + x + " y=" + y + " z=" + z + " ax=" + ax + " ay=" + ay + " az=" + az);
}

// a number with 2 / 3 digits after the point
string NetLand_NumK(float fIn, int iK)
{
	string sSign = "";
	float f = fIn;
	if (f < 0.0)
	{
		sSign = "-";
		f = 0.0 - f;
	}
	int iAll = makeint(f * iK + 0.5);
	int iWhole = iAll / iK;
	int iFrac = iAll - iWhole * iK;
	string sFrac = "" + iFrac;
	if (iK >= 100 && iFrac < 10) { sFrac = "0" + sFrac; }
	if (iK >= 1000 && iFrac < 100) { sFrac = "0" + sFrac; }
	return sSign + iWhole + "." + sFrac;
}
string NetLand_Num2(float f) { return NetLand_NumK(f, 100); }
string NetLand_Num3(float f) { return NetLand_NumK(f, 1000); }

// COOP s34: the guns held as a musket: the musket and the double-barrelled musket (the musket of the officer musketeer)
bool NetLand_IsMushItem(string sGun)
{
	if (sGun == "mushket") { return true; }
	if (sGun == "mushket2x2") { return true; }
	return false;
}

// the tied musket goes (the gun is a pistol now, the player is dead)
void NetLand_MushUntie(ref chr)
{
	if (!CheckAttribute(chr, "NetLandMushTied")) { return; }
	DeleteAttribute(chr, "NetLandMushTied");
	if (!IsEntity(chr)) { return; }
	int iMI = GetItemIndex("mushket");
	if (iMI >= 0) { SendMessage(chr, "lsl", MSG_CHARACTER_EX_MSG, "UntieItem", iMI); }
	iMI = GetItemIndex("mushket2x2");
	if (iMI >= 0) { SendMessage(chr, "lsl", MSG_CHARACTER_EX_MSG, "UntieItem", iMI); }
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
	if (NetLand_IsMushItem(sGun)) { return true; }		// COOP s34: the double-barrelled one too
	return false;		// COOP s35: a musketeer skin takes the musket only when it is chosen (NetLand_BladeModel)
}

bool NetLand_ModelIsWoman(string sModel)
{
	if (findSubStr(sModel, "girl_", 0) == 0) { return true; }
	if (findSubStr(sModel, "lady_", 0) == 0) { return true; }
	if (findSubStr(sModel, "Lady0", 0) == 0) { return true; }
	if (findSubStr(sModel, "lady0", 0) == 0) { return true; }
	if (findSubStr(sModel, "Girl_", 0) == 0) { return true; }
	if (sModel == "BaynesDaughter" || sModel == "BaynesWife" || sModel == "CitizenWom_1") { return true; }
	if (findSubStr(sModel, "whore_", 0) == 0) { return true; }
	if (findSubStr(sModel, "horse0", 0) == 0) { return true; }
	if (sModel == "beatrice" || sModel == "beatriceA" || sModel == "liza" || sModel == "AnnaDeLeiva") { return true; }
	if (sModel == "Angellica" || sModel == "YokoDias" || sModel == "Jessika" || sModel == "Danielle") { return true; }
	return false;
}

// the model without the "_mush" ending that the single player gives to a hero with a musket
string NetLand_BaseModel(string sModel)
{
	int iLen = strlen(sModel);
	if (iLen > 5)
	{
		if (strcut(sModel, iLen - 5, iLen - 1) == "_mush") { return strcut(sModel, 0, iLen - 6); }
	}
	return sModel;
}

// sex of the character for the model: man / woman / skeleton
string NetLand_ModelSex(string sModel)
{
	if (NetLand_ModelIsWoman(sModel)) { return "woman"; }
	if (sModel == "mummy" || sModel == "skeletcap") { return "skeleton"; }
	if (sModel == "skel1" || sModel == "skel2" || sModel == "skel3" || sModel == "skel4") { return "skeleton"; }
	return "man";
}

// COOP: models whose skeleton is not the one of man.an (123 bones) / woman.an (128 bones): the bones of the .gm were counted.
// The animation of man / woman makes them crooked. "" = the usual animation of the sex
string NetLand_ModelSkelAnim(string sModel)
{
	if (sModel == "Jessika" || sModel == "jessika") { return "Jessika"; }		// Jessika.an, 49 bones, its own fight set
	if (sModel == "AnnaDeLeiva") { return "towngirl2"; }		// woman2.an, 84 bones (the single player gives her towngirl2)
	if (sModel == "DSolovey") { return "man_kvl2"; }			// 40 bones
	// man2.an, 79 bones
	if (sModel == "Researcher" || sModel == "Skelt" || sModel == "priest") { return "man2"; }
	if (sModel == "priest_sp1" || sModel == "priest_sp2" || sModel == "priest_sp3") { return "man2"; }
	if (findSubStr(sModel, "prison_", 0) == 0) { return "man2"; }
	return "";
}

// animation: the musketeers fight with the blade and the pistol here, so they get the animation of a man
string NetLand_ModelAnim(string sModel)
{
	// COOP s35: findSubStr does not look at the case: Mushketer_1..5 got the animation of the musket with a blade
	if (findSubStr(sModel, "whore_", 0) == 0) { return "whore"; }		// COOP s30: towngirl3.an (44 bones) with the moves of man.an
	// the three special ladies of the single player have an animation set named after the model
	if (sModel == "YokoDias" || sModel == "Angellica" || sModel == "Danielle") { return sModel; }
	string sSkelAnim = NetLand_ModelSkelAnim(sModel);
	if (sSkelAnim != "") { return sSkelAnim; }
	string sSex = NetLand_ModelSex(sModel);
	if (sSex == "woman") { return "woman"; }
	if (sSex == "skeleton") { return "skeleton"; }
	return "man";
}

// COOP s30: the musket of a skin whose skeleton is not the one of the man: RESOURCE\ANIMATION\mush_<anim>.ani (mushketer.an
// carried over to that skeleton: _backup_s30\tools\gen_anims_s30.py). The scripts keep model.animation = "mushketer" (all the
// rules of the musketeer of the game look at that name), the engine gets model.animfile (characters\characters_face.c:
// Character_AnimFile). "" = the skin takes mushketer.an itself (man.an, skeleton.an)
bool bNetLandMushAnims = true;

string NetLand_MushAnimFile(string sSkin)
{
	if (!bNetLandMushAnims) { return ""; }
	string sA = NetLand_ModelAnim(sSkin);
	if (sA == "woman" || sA == "towngirl2" || sA == "Jessika" || sA == "YokoDias") { return "mush_" + sA; }
	if (sA == "Angellica" || sA == "Danielle" || sA == "man_kvl2" || sA == "man2") { return "mush_" + sA; }
	if (sA == "whore") { return "mush_whore"; }
	return "";
}

// COOP s30: the file of the animation of a character for the engine (the musket of such a skin), "" = none
void NetLand_SetAnimFile(ref chr, string sFile)
{
	if (sFile == "") { DeleteAttribute(chr, "model.animfile"); return; }
	chr.model.animfile = sFile;
}

// COOP s30: "/skin <model>" puts the skin on our hero on this machine only (a test of the animations: "/skin whore_1",
// "/skin YokoDias m" = with the animation of the musket, "/skin" = back to the skin of the profile)
void NetLand_SkinCmd(string sMsg)
{
	ref chr = GetMainCharacter();
	string sArg = "";
	if (strlen(sMsg) > 6) { sArg = strcut(sMsg, 6, strlen(sMsg) - 1); }
	bool bMushT = false;
	int iArgLen = strlen(sArg);
	if (iArgLen > 2)
	{
		if (strcut(sArg, iArgLen - 2, iArgLen - 1) == " m") { bMushT = true; sArg = strcut(sArg, 0, iArgLen - 3); }
	}
	if (sArg == "") { sArg = NetLand_ModelFor(wClientID); }
	chr.model = sArg;
	chr.model.animation = NetLand_ModelAnim(sArg);
	NetLand_SetAnimFile(chr, "");
	if (bMushT)
	{
		chr.model.animation = "mushketer";
		NetLand_SetAnimFile(chr, NetLand_MushAnimFile(sArg));
	}
	chr.model.height = NetLand_ModelHeight(sArg);
	if (IsEntity(chr)) { SetNewModelToChar(chr); }
	string sSkInfo = "Скин: " + sArg + ", анимация " + Character_AnimFile(chr);
	Log_SetStringToLog(sSkInfo);
	Trace("COOP: /skin " + sArg + " anim " + chr.model.animation + " file " + Character_AnimFile(chr));
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

	string sByFace = NetLand_FaceModel(sFace);		// a portrait face_N or an NPC skin npc_N
	if (sByFace != "") { sModel = sByFace; }
	Trace("COOP: land model of " + wID + ": face <" + sFace + "> -> " + sModel);
	NCClients[wID].LandModel = sModel;
	return sModel;
}

// ------------------------------------------------------------------------------------------------
// COOP: the NPC skins without a portrait of their own (RESOURCE\MODELS\Characters that no face_N gives; not the "_mush" twins,
// the props Genres*, the crab, whore_1..4). The profile shows them after the portraits with the icon NET\Faces\face0 and keeps
// "npc_N", N = the number in this list (the same on every machine: the face of the net has 12 letters at most, a model
// name does not fit). New models are added at the END only, or the old profiles get other skins
// ------------------------------------------------------------------------------------------------
// COOP s35: Mushketer_1..5 (npc_60..64): the pirates of the single player with the musket. Not in the list of the faces any
// more; the old profiles keep them (the numbers stay)
bool NetLand_NpcHidden(int iN)
{
	if (iN >= 60 && iN <= 64) { return true; }
	return false;
}

int NetLand_NpcCount()
{
	return 131;		// COOP s30: 128..131 whore_1..4 (RESOURCE\ANIMATION\whore.ani, the fight set of the man on their skeleton)
}

string NetLand_NpcModel(int iN)
{
	switch (iN)
	{
		case 1: return "Aztec"; break;
		case 2: return "AztecCitizen1"; break;
		case 3: return "AztecCitizen2"; break;
		case 4: return "AztecWarrior1"; break;
		case 5: return "AztecWarrior2"; break;
		case 6: return "AztecWarrior3"; break;
		case 7: return "AztecWarrior4"; break;
		case 8: return "AztecWarrior5"; break;
		case 9: return "azzy"; break;
		case 10: return "BaynesDaughter"; break;
		case 11: return "BaynesWife"; break;
		case 12: return "Blad_0"; break;
		case 13: return "Blad_1"; break;
		case 14: return "Blad_2"; break;
		case 15: return "Blad_3"; break;
		case 16: return "Blad_4"; break;
		case 17: return "Blad_5"; break;
		case 18: return "Blad_6"; break;
		case 19: return "Blad_7"; break;
		case 20: return "Blad_8"; break;
		case 21: return "Blad_9"; break;
		case 22: return "Blood5_Cirass"; break;
		case 23: return "Blood5_Cirass1"; break;
		case 24: return "Blood5_Cirass2"; break;
		case 25: return "Blood5_Cirass3"; break;
		case 26: return "blood_bomj"; break;
		case 27: return "CitizenWom_1"; break;
		case 28: return "Cyclop"; break;
		case 29: return "dOgeron"; break;
		case 30: return "DSolovey"; break;
		case 31: return "eng_mush_1"; break;
		case 32: return "eng_mush_2"; break;
		case 33: return "eng_mush_3"; break;
		case 34: return "Espinosa_Cirass"; break;
		case 35: return "Espinosa_Cirass1"; break;
		case 36: return "Espinosa_Cirass2"; break;
		case 37: return "Espinosa_Cirass3"; break;
		case 38: return "fra_mush_1"; break;
		case 39: return "fra_mush_2"; break;
		case 40: return "fra_mush_3"; break;
		case 41: return "Gaskon"; break;
		case 42: return "Girl_11"; break;
		case 43: return "Helper"; break;
		case 44: return "Hero1_Cirass"; break;
		case 45: return "Hero1_Cirass1"; break;
		case 46: return "Hero1_Cirass2"; break;
		case 47: return "Hero1_Cirass3"; break;
		case 48: return "hol_mush_1"; break;
		case 49: return "hol_mush_2"; break;
		case 50: return "hol_mush_3"; break;
		case 51: return "horse05"; break;
		case 52: return "horse06"; break;
		case 53: return "horse07"; break;
		case 54: return "horse08"; break;
		case 55: return "indsair2"; break;
		case 56: return "indsar1"; break;
		case 57: return "Mansfield"; break;
		case 58: return "mictlantecuhtli"; break;
		case 59: return "Modyford"; break;
		case 60: return "Mushketer_1"; break;
		case 61: return "Mushketer_2"; break;
		case 62: return "Mushketer_3"; break;
		case 63: return "Mushketer_4"; break;
		case 64: return "Mushketer_5"; break;
		case 65: return "MusketeerEnglish_2"; break;
		case 66: return "Ostin"; break;
		case 67: return "ozg_green"; break;
		case 68: return "ozg_matias"; break;
		case 69: return "panhandler_1"; break;
		case 70: return "panhandler_2"; break;
		case 71: return "panhandler_3"; break;
		case 72: return "panhandler_4"; break;
		case 73: return "panhandler_5"; break;
		case 74: return "panhandler_6"; break;
		case 75: return "Pat_0"; break;
		case 76: return "Pat_1"; break;
		case 77: return "Pat_2"; break;
		case 78: return "Pat_3"; break;
		case 79: return "Pat_4"; break;
		case 80: return "Pat_5"; break;
		case 81: return "Pat_6"; break;
		case 82: return "Pat_7"; break;
		case 83: return "Pat_8"; break;
		case 84: return "Pat_9"; break;
		case 85: return "Port_0"; break;
		case 86: return "Port_1"; break;
		case 87: return "Port_2"; break;
		case 88: return "Port_3"; break;
		case 89: return "Port_4"; break;
		case 90: return "Port_5"; break;
		case 91: return "Port_6"; break;
		case 92: return "Port_7"; break;
		case 93: return "Port_8"; break;
		case 94: return "priest"; break;
		case 95: return "priest_sp1"; break;
		case 96: return "priest_sp2"; break;
		case 97: return "priest_sp3"; break;
		case 98: return "prison_1"; break;
		case 99: return "prison_2"; break;
		case 100: return "prison_3"; break;
		case 101: return "prison_4"; break;
		case 102: return "prison_5"; break;
		case 103: return "Protocusto"; break;
		case 104: return "Q_Barmen"; break;
		case 105: return "QuestMan1"; break;
		case 106: return "Resc_0"; break;
		case 107: return "Resc_1"; break;
		case 108: return "Resc_2"; break;
		case 109: return "Resc_3"; break;
		case 110: return "Resc_4"; break;
		case 111: return "Resc_5"; break;
		case 112: return "Resc_6"; break;
		case 113: return "Resc_7"; break;
		case 114: return "Resc_8"; break;
		case 115: return "Resc_9"; break;
		case 116: return "skel1"; break;
		case 117: return "skel2"; break;
		case 118: return "skel3"; break;
		case 119: return "skel4"; break;
		case 120: return "Skelt"; break;
		case 121: return "sold_new"; break;
		case 122: return "sold_spa_1"; break;
		case 123: return "spa_mush_1"; break;
		case 124: return "spa_mush_2"; break;
		case 125: return "spa_mush_3"; break;
		case 126: return "SpaOfficer3"; break;
		case 127: return "stuvesant"; break;
		case 128: return "whore_1"; break;
		case 129: return "whore_2"; break;
		case 130: return "whore_3"; break;
		case 131: return "whore_4"; break;
	}
	return "";
}

// "npc_12" -> 12, -1 if it is not an NPC skin of the profile
int NetLand_NpcNum(string sFace)
{
	if (findSubStr(sFace, "npc_", 0) != 0) { return -1; }
	if (strlen(sFace) <= 4) { return -1; }
	string sNumN = strcut(sFace, 4, strlen(sFace) - 1);
	for (int i=0; i<strlen(sNumN); i++)
	{
		string sChN = strcut(sNumN, i, i);
		if (findSubStr("0123456789", sChN, 0) < 0) { return -1; }
	}
	return sti(sNumN);
}

// the icon of an NPC skin, "" if it is not one
// COOP s30: the portraits of the NPC skins made by /portraits (net\MenuScene.c, _backup_s30\tools\make_portraits.py):
// interfaces\portraits\256\npc_N.tga.tx; a skin without one has the empty face of NET\Faces
object NetNpcPortraits;
bool bNetNpcPortraits;
void NetLand_NpcPortraitsScan()
{
	object oNpP;
	int iNpN = NFFindFiles(&oNpP, "resource\textures\interfaces\portraits\256\", "npc_*.tga.tx", false);
	DeleteAttribute(&NetNpcPortraits, "");
	for (int i=0; i<iNpN; i++)
	{
		string sFl = "f" + i;
		string sNpName = oNpP.(sFl).Name;
		NetNpcPortraits.(sNpName) = oNpP.(sFl).FilePath;
	}
	bNetNpcPortraits = true;
	Trace("COOP: NPC portraits: " + iNpN);
}
bool NetLand_NpcHasPortrait(string sFace)
{
	if (!bNetNpcPortraits) { NetLand_NpcPortraitsScan(); }
	return CheckAttribute(&NetNpcPortraits, sFace);
}
string NetLand_NpcFaceTex(string sFace)
{
	if (NetLand_NpcNum(sFace) < 1) { return ""; }
	if (NetLand_NpcHasPortrait(sFace)) { return "interfaces\portraits\256\" + sFace + ".tga"; }
	return sUserFacesPath + "_emptyface_.tga";		// COOP s30: no portrait of this NPC skin
}

// the model of a face of the profile: a portrait face_N or an NPC skin npc_N, "" = none (a pirate by the slot)
string NetLand_FaceModel(string sFace)
{
	int iFaceM = NetLand_FaceNum(sFace);
	if (iFaceM >= 0) { return NetLand_FaceToModel(iFaceM); }
	int iNpcM = NetLand_NpcNum(sFace);
	if (iNpcM >= 1) { return NetLand_NpcModel(iNpcM); }
	return "";
}

// COOP s30: the skin of the profile for the hero of the main menu (interface\mainmenu.c). The net client is not created yet
// at the start of the game, so the last profile is read from Profiles.nsv
// the hero of the main menu (interface\mainmenu.c): the model of the script and the entity of its blade and gun
object NetMMHero;
object NetMMBlade;
// COOP s30: the profile screen (net\MenuScene.c) shows the hero of its face and nick, not of NetClient
bool bNetMMHeroOvr;
string sNetMMFaceOvr;
string sNetMMNickOvr;

// the model of an item (a blade, a gun) for the entity "blade", "" = none
string NetMM_ItemModel(string sId)
{
	if (sId == "") { return ""; }
	int iIdx = GetItemIndex(sId);
	if (iIdx < 0) { return ""; }
	if (!CheckAttribute(&Items[iIdx], "model")) { return ""; }
	return Items[iIdx].model;
}

// the code of the musket in the right hand for the hero of the menu: the start values of the globals do not reach the game
// (NetLand_StartLand sets them too), so the angles of NET_LAND_MUSH_R* are set here
int NetMM_MushCode(string sAni)
{
	iNetLandMushRX = NET_LAND_MUSH_RX;
	iNetLandMushRY = NET_LAND_MUSH_RY;
	iNetLandMushRZ = NET_LAND_MUSH_RZ;
	if (findSubStr(sAni, "mush_", 0) == 0) { return NetLand_MushCodeKey(sAni); }		// COOP s32: the angles of its skeleton
	return NetLand_MushCode();
}

// oH.model (the skin in the armor of the loadout), oH.anim, oH.blade, oH.gun (models, "" = none): the profile of the net game
// (the loadout of the duel, NCProfiles.NetLo.<nick>). A musket like in the duel (NetLand_ApplyLo): the musketeer twin of the
// skin or the skin itself with the musket in the right hand (oH.tie = 1: the message 0x5213 of the patched blade.dll), the
// animation of the musketeer (or mush_<anim> of a skin that is not a man), no blade.
// oH.ship: the ship of the lobby of the profile (NCProfiles.NetShip.<nick>.Ship, the index of NetShips), -1 = none
void NetMM_HeroInfo(ref oH)
{
	object oPr;
	string sFace = "";
	string sNick = "";
	if (bNetMMHeroOvr)
	{
		// the profile screen: its face and nick, the profiles as they are now (the loadout may be not saved in the file yet)
		CopyAttributes(&oPr, &NCProfiles);
		sFace = sNetMMFaceOvr;
		sNick = sNetMMNickOvr;
	}
	else
	{
		Net_LoadFile(false, &oPr, "Profiles.nsv");
		// COOP s32: the chosen profile first (a preview leaves the edited face in NetClient)
		if (CheckAttribute(&oPr, "LastProfile.FaceImage")) { sFace = oPr.LastProfile.FaceImage; }
		if (CheckAttribute(&oPr, "LastProfile.NickName")) { sNick = oPr.LastProfile.NickName; }
		if (sFace == "" && CheckAttribute(&NetClient, "FaceImage")) { sFace = NetClient.FaceImage; }
		if (sNick == "" && CheckAttribute(&NetClient, "NickName")) { sNick = NetClient.NickName; }
	}
	if (sNick == "default") { sNick = ""; }		// Net_ClientFirstInit: the client is not made yet at the start of the game
	string sModel = NetLand_FaceModel(sFace);
	if (sModel == "")
	{
		if (CheckAttribute(&oPr, "LastProfile.FaceImage")) { sModel = NetLand_FaceModel(oPr.LastProfile.FaceImage); }
	}
	if (sModel == "") { sModel = "Pirate_1"; }
	if (sNick == "")
	{
		if (CheckAttribute(&oPr, "LastProfile.NickName")) { sNick = oPr.LastProfile.NickName; }
	}
	if (sNick == "") { sNick = "default"; }
	oH.model = sModel;
	oH.anim = NetLand_ModelAnim(sModel);
	oH.blade = "";
	oH.gun = "";
	oH.tie = 0;
	oH.ship = -1;
	if (CheckAttribute(&oPr, "NetShip." + sNick + ".Ship")) { oH.ship = sti(oPr.NetShip.(sNick).Ship); }
	// COOP s30: the look of the ship of the profile (the sails upgrade of the shop, the color, the gerald, the flag)
	oH.hascolor = false;
	oH.scolor = 0;
	oH.gerald = "";
	oH.flag = "";
	oH.sails = 1;
	if (CheckAttribute(&oPr, "NetShip." + sNick + ".Rig")) { oH.sails = sti(oPr.NetShip.(sNick).Rig) + 1; }
	if (CheckAttribute(&oPr, "Profiles." + sNick))
	{
		aref aPf;
		makearef(aPf, oPr.Profiles.(sNick));
		if (CheckAttribute(aPf, "ColorSail"))
		{
			int iCsl = sti(aPf.ColorSail);
			if (iCsl >= 0 && iCsl < SAILS_COLOR_QUANTITY)
			{
				oH.hascolor = true;
				oH.scolor = sti(SailsColors[iCsl].color);
			}
		}
		if (CheckAttribute(aPf, "ImageSail")) { oH.gerald = aPf.ImageSail; }
		if (CheckAttribute(aPf, "ImageFlag")) { oH.flag = aPf.ImageFlag; }
	}
	if (!CheckAttribute(&oPr, "NetLo." + sNick + ".Inited")) { return; }
	aref lo;
	makearef(lo, oPr.NetLo.(sNick));
	string sDress = NetLand_ArmorModel(sModel, NetLand_LoArmor(lo));
	oH.model = sDress;
	bool bMush = false;
	string sMushGun = "mushket";
	if (CheckAttribute(lo, "Gun")) { if (NetLand_IsMushItem(lo.Gun)) { bMush = true; sMushGun = lo.Gun; } }
	if (bMush)
	{
		string sMushModel = NetLand_MushModel(sDress, -1);
		oH.model = sMushModel;
		oH.anim = "mushketer";
		string sMushFile = NetLand_MushAnimFile(sModel);
		if (sMushFile != "") { oH.anim = sMushFile; }
		if (sMushModel == sDress)
		{
			if (!NetLand_IsMushSkin(sModel)) { oH.tie = 1; }
		}
		oH.gun = NetMM_ItemModel(sMushGun);
		return;
	}
	oH.model = NetLand_BladeModel(sDress);		// COOP s35
	if (CheckAttribute(lo, "Blade")) { oH.blade = NetMM_ItemModel(lo.Blade); }
	if (CheckAttribute(lo, "Gun"))
	{
		if (!NetLand_IsMushItem(lo.Gun)) { oH.gun = NetMM_ItemModel(lo.Gun); }
	}
}

// ================================================================================================
// COOP: PREVIEW. A game of one player on this machine: a local server (LAN, a password of its own), the client joins it
// (interfaces\FindGame.c), the lobby starts it at once (interfaces\Buy.c). Esc -> leave: the main menu, the server is closed
//   1 (the profile, "Предпросмотр" under the faces): a land duel in the big cabin (Cabin_Huge); an immortal doll with the
//     skin and the loadout of the player stands in front of him and does nothing
//   2 (the shop of the ship of the profile): a deathmatch alone in the open sea with the chosen ship
// ================================================================================================
int iNetPreview = 0;			// 0 no preview, 1 the skin, 2 the ship
string sNetPreviewPass = "";

// COOP s32: the preview needs the one server of the game and a free port: no preview while our server runs (the host went
// to the menu, the battle goes on) or the client is connected (the preview replaced the running server, could not connect
// and crashed on cancel)
bool NetPreview_Busy()
{
	if (NetServer_IsStarted()) { return true; }
	if (bNetClientConnected) { return true; }
	return false;
}

void NetPreview_Begin(int iMode)
{
	if (NetServer_IsStarted()) { Net_DeleteServer(); }
	object oPv;
	sNetPreviewPass = "pv" + rand(99999);
	oPv.ServerName = "Предпросмотр";
	oPv.ServerPort = NET_DEFAULT_SERVER_PORT;
	oPv.ServerPassword = sNetPreviewPass;
	oPv.AdminPassword = "";
	oPv.Dedicated = 0;
	oPv.P2P = 0;
	oPv.LanServer = 1;
	oPv.InetServer = 0;
	oPv.UpdateMasterServer = false;
	oPv.MaxFps = 100;
	oPv.GameType = NETGAME_DEATHMATCH;
	oPv.NumTeams = 1;
	oPv.MaxPlayers = 2;
	oPv.GameSpeed = 1.0;
	oPv.TimeLimit = 0;
	oPv.StartPosition = 0;
	oPv.Credit = 1000000;
	oPv.MaxShipClass = 1;			// every ship
	oPv.BotSkill = 0;
	oPv.IslandID = "";				// the open sea
	oPv.LandLoc = "";
	if (iMode == 1) { oPv.LandLoc = "Cabin_Huge"; }
	oPv.WeatherID = "11 Hour";
	oPv.Tornado = 0;
	oPv.AllowUpload = true;
	oPv.AllowDownload = true;
	oPv.Respawn = 1;
	oPv.RespawnTime = 10;
	oPv.NoBoarding = 0;
	oPv.PassivePerks = 0;
	oPv.ActivePerks = 0;
	Net_CreateServer(&oPv);
	iNetPreview = iMode;
	Trace("COOP: preview " + iMode + ": the local server is made");
}

// interfaces\FindGame.c, a moment after it is open: the client joins the server of the preview
void NetPreview_Connect()
{
	if (iNetPreview <= 0) { return; }
	if (!NetServer_IsStarted()) { NetPreview_Abort("no server"); return; }
	NetClient.ServerPassword = sNetPreviewPass;
	Trace("COOP: preview " + iNetPreview + ": the client joins 127.0.0.1:" + NET_DEFAULT_SERVER_PORT);
	NetClient_TryConnect("127.0.0.1", NET_DEFAULT_SERVER_PORT);
}

void NetPreview_Abort(string sWhy)
{
	if (iNetPreview <= 0) { return; }
	Trace("COOP: preview stops: " + sWhy);
	iNetPreview = 0;
	if (NetServer_IsStarted()) { Net_DeleteServer(); }
	NetClient_LoadLastProfile();		// COOP s32: the face of the chosen profile again (the preview put the edited one)
}

// the doll of the skin preview: the skin and the loadout of our hero, immortal, it stands 2 m in front of him and looks at him
void NetLand_PreviewDoll()
{
	int iGen = GetEventData();
	if (iGen != iNetLandGen || !bNetLandActive || !bNetLandLocReady) { return; }
	if (iNetPreview != 1) { return; }
	if (!IsEntity(loadedLocation)) { return; }
	ref mc = GetMainCharacter();
	string sDollModel = NetLand_ModelFor(wClientID);
	int iChr = NPC_GenerateCharacter("NetLandPreview", sDollModel, NetLand_ModelSex(sDollModel), NetLand_ModelAnim(sDollModel), 10, PIRATE, 0, true);
	if (iChr < 0) { return; }
	ref ch = &Characters[iChr];
	ch.name = "Предпросмотр";
	if (CheckAttribute(&NetClient, "NickName")) { ch.name = NetClient.NickName; }
	ch.lastname = "";
	ch.NetLandSkin = sDollModel;
	ch.RebirthPhantom = true;
	aref lo;
	makearef(lo, oNetLandLo);
	if (CheckAttribute(lo, "Blade")) { NetLand_ApplyLo(ch, lo, false, true); }
	LAi_SetImmortal(ch, true);
	LAi_CharacterDisableDialog(ch);
	LAi_SetStayType(ch);
	LAi_group_MoveCharacter(ch, "NetLandPreview");
	LAi_group_SetRelation("NetLandPreview", LAI_GROUP_PLAYER, LAI_GROUP_NEITRAL);
	ChangeCharacterAddressGroup(ch, sNetLandLoc, sNetLandEntryGroup, sNetLandEntryLocator);
	// the same character as in the last preview (the same id): the entity keeps the old model without this
	SetNewModelToChar(ch);
	float fPx, fPy, fPz, fPay;
	GetCharacterPos(mc, &fPx, &fPy, &fPz);
	GetCharacterAy(mc, &fPay);
	TeleportCharacterToPosAy(ch, fPx + sin(fPay) * 2.0, fPy, fPz + cos(fPay) * 2.0, fPay + PI);
	Log_SetStringToLog("Предпросмотр: перед вами ваш скин. Esc - выход в меню");
	Trace("COOP: preview: the doll " + sDollModel + " is character " + iChr);
}

// The icon of the hero in the interfaces (the land interface draws battle_interface\portraits\face_N.tga of the hero
// and of the officers, N = FaceId) is taken from FaceId. Our hero is the single player hero, so he keeps the FaceId of
// Blaze unless it is set here: a portrait face_N of the net game is exactly the chosen icon, a NET/FACES avatar has no
// portrait of the game and gets the face of the model (a pirate by the slot), the same model the hero has on the screen.
void NetLand_SetHeroFace(ref mc)
{
	string sFace = "";
	if (CheckAttribute(&NetClient, "FaceImage")) { sFace = NetClient.FaceImage; }
	if (sFace == "" && CheckAttribute(&NCClients[wClientID], "UserData.Face")) { sFace = NCClients[wClientID].UserData.Face; }

	int iFace = NetLand_FaceNum(sFace);
	if (iFace >= 0)
	{
		mc.FaceId = iFace;
		Trace("COOP: land hero face <" + sFace + "> -> " + iFace);
		return;
	}
	// FaceMaker sets FaceId by the model (a model without its own case keeps the old FaceId)
	FaceMaker(mc);
	Trace("COOP: land hero face <" + sFace + "> -> model " + mc.model + ", face " + mc.FaceId);
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
// 1..4: death_0..3 (the sword out), 5..6: death_citizen_1..2 (no sword). Both tables of the game get it: the body falls the same
// way on every screen, also if the puppet has the sword out and its player had not (or the other way round)
void NetLand_SetDeathVariant(ref rD, int iVar)
{
	int iDeathNum = iVar - 1;
	string sN = "death_" + iDeathNum;
	if (iVar >= 5) { sN = "death_citizen_" + (iVar - 4); }
	rD.actions.fightdead.d1 = sN;
	rD.actions.fightdead.d2 = sN;
	rD.actions.fightdead.d3 = sN;
	rD.actions.fightdead.d4 = sN;
	rD.actions.dead.d1 = sN;
	rD.actions.dead.d2 = sN;
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

// ================================================================================================
// COOP: TEAM DUEL. "Create game" -> land duel -> teams = 2: two teams (0 and 1, the lobby chooses), allies do not hurt each other.
//   no respawn:                    the team that has no living player loses (the other one wins)
//   respawn + a time limit:        when the time is up the team with more kills over the other team wins (a draw if equal)
//   respawn without a time limit:  the usual endless duel with respawn, only with allies
// teams = 1 (the default): the usual duel of everybody against everybody, nothing of this works.
// Bots (they have no team) stay as they are: they are not counted in the score of the teams.
// ================================================================================================
bool NetServer_LandTeamGame()
{
	if (!NetServer_LandMode()) { return iNetSrvBDScope > 0; }		// COOP s37: a boarding duel is a duel of two teams
	if (!CheckAttribute(&NetServer, "NumTeams")) { return false; }
	return sti(NetServer.NumTeams) == 2;
}

// team 0 / 1 of a player (the server), -1 = no team game, a bot, nobody
int NetServer_LandTeamOf(int w)
{
	if (w < 0 || w >= NET_MAXPLAYERS) { return -1; }
	if (!NetServer_LandMode()) { return NetServer_BDTeamOf(w); }		// COOP s37: the boarding duel
	if (!NetServer_LandTeamGame()) { return -1; }
	int iTeamS = sti(NSClients[w].Team);
	if (iTeamS < 0) { iTeamS = 0; }
	if (iTeamS > 1) { iTeamS = 1; }
	return iTeamS;
}

// the same on a client: is this game a team duel
bool NetLand_TeamGame()
{
	if (bNetBoardDuel) { return true; }		// COOP s37: the boarding: the attacker and the defender
	if (!CheckAttribute(&NCServer, "NumTeams")) { return false; }
	if (sti(NCServer.NumTeams) != 2) { return false; }		// (a client that has not got the info yet has 4)
	if (bNetLandActive) { return true; }
	if (!CheckAttribute(&NCServer, "IslandID")) { return false; }
	return NetLand_IsAdvert(NCServer.IslandID);
}

// team 0 / 1 of a player (the client)
int NetLand_TeamOf(int w)
{
	if (w < 0 || w >= NET_MAXPLAYERS) { return 0; }
	if (bNetBoardDuel)
	{
		if (w == wClientID) { return iNetBoardTeam; }
		if (w == wNetBoardEnemy) { return 1 - iNetBoardTeam; }
		return 0;
	}
	if (!CheckAttribute(&NCClients[w], "Team")) { return 0; }
	int iTeamC = sti(NCClients[w].Team);
	if (iTeamC < 0) { iTeamC = 0; }
	if (iTeamC > 1) { iTeamC = 1; }
	return iTeamC;
}

// a player of OUR team (not we ourselves): his puppet is a friend, nobody hits him
bool NetLand_IsAlly(int w)
{
	if (w < 0 || w >= NET_MAXPLAYERS || w == wClientID) { return false; }
	if (!NetLand_TeamGame()) { return false; }
	return NetLand_TeamOf(w) == NetLand_TeamOf(wClientID);
}

string NetLand_PuppetGroup(int w)
{
	if (NetLand_IsAlly(w)) { return NET_LAND_GROUP_ALLY; }
	return NET_LAND_GROUP;
}

// the milliseconds that are left to the end of the match: 0 = no limit, at least 1 once there is a limit
int NetServer_LandTimeLeftMs()
{
	if (!CheckAttribute(&NetServer, "LandEndAt")) { return 0; }
	if (sti(NetServer.LandEndAt) == 0) { return 0; }
	int iLeftL = sti(NetServer.LandEndAt) - Net_GetTickCount();
	if (iLeftL < 1) { iLeftL = 1; }
	return iLeftL;
}

// the score of the teams to everybody (wTo = DST_INVALID) or to one player
void NetServer_LandSendTeamScore(int wTo)
{
	if (!NetServer_LandTeamGame()) { return; }
	int iSMsgTs = NMCreate();
	NMAddByte(iSMsgTs, NC_LAND);
	NMAddByte(iSMsgTs, NSC_LAND_TEAMSCORE_R);
	NMAddWord(iSMsgTs, sti(NetServer.LandTeamKills0));
	NMAddWord(iSMsgTs, sti(NetServer.LandTeamKills1));
	if (wTo == DST_INVALID) { NetServer_LandSendOthers(DST_INVALID, iSMsgTs, true); }
	else { NetServer_SendMessage(wTo, iSMsgTs, true); }
	NMDelete(iSMsgTs);
}

// the game over of a team duel (the client shows the winner like a team of a sea battle: the name of the colour)
void NetServer_LandAnnounceTeamOver(int iWinTeam, int iDraw)
{
	Trace("COOP: land team duel over, winner team " + iWinTeam + ", draw " + iDraw + ", kills " + sti(NetServer.LandTeamKills0) + ":" + sti(NetServer.LandTeamKills1));
	int iWT = iWinTeam;
	if (iWT < 0) { iWT = 0; }
	int iSMsgO = NMCreate();
	NMAddByte(iSMsgO, NC_STARTGAME);
	NMAddByte(iSMsgO, NSC_STARTGAME_GAMEOVER);
	NMAddByte(iSMsgO, NETGAME_NAVAL);
	NMAddClientID(iSMsgO, DST_INVALID);
	NMAddByte(iSMsgO, iWT);
	NMAddByte(iSMsgO, iDraw);
	NetServer_SendMessage(DST_ALL, iSMsgO, true);
	NMDelete(iSMsgO);
}

// no respawn: a team without a living player has lost. Teams with nobody in the game do not count (nobody to win against)
void NetServer_LandCheckOverTeams()
{
	if (sti(NetServer.GameOver)) { return; }
	int iMembers0 = 0;
	int iMembers1 = 0;
	int iAlive0 = 0;
	int iAlive1 = 0;
	for (int iTm=0; iTm<NET_MAXPLAYERS; iTm++)
	{
		if (!sti(NSClients[iTm].Use)) { continue; }
		if (!sti(NSClients[iTm].InGame)) { continue; }
		int iTmT = NetServer_LandTeamOf(iTm);
		bool bTmAlive = true;
		if (sti(NSClients[iTm].LandDead)) { bTmAlive = false; }
		if (iTmT == 0) { iMembers0++; if (bTmAlive) { iAlive0++; } }
		else { iMembers1++; if (bTmAlive) { iAlive1++; } }
	}
	// COOP s28: the bots of the teams are members too (alive while their owner's machine says so)
	for (int iTb=NET_BOT_FIRST; iTb<NET_MAXCLIENTS; iTb++)
	{
		int iTbT = NetServer_LandSideOf(iTb);
		if (iTbT < 0) { continue; }
		bool bTbAlive = true;
		if (CheckAttribute(&NSClients[iTb], "LandBotDeadSeen")) { if (sti(NSClients[iTb].LandBotDeadSeen) != 0) { bTbAlive = false; } }
		if (iTbT == 0) { iMembers0++; if (bTbAlive) { iAlive0++; } }
		else { iMembers1++; if (bTbAlive) { iAlive1++; } }
	}
	if (iMembers0 == 0 || iMembers1 == 0) { return; }
	if (iAlive0 > 0 && iAlive1 > 0) { return; }

	NetServer.GameOver = true;
	if (iAlive0 > 0) { NetServer_LandAnnounceTeamOver(0, 0); return; }
	if (iAlive1 > 0) { NetServer_LandAnnounceTeamOver(1, 0); return; }
	NetServer_LandAnnounceTeamOver(0, 1);
}

// respawn + a time limit: the time is up, the team with more kills over the other team wins
void NetServer_LandTimeUp()
{
	NetServer.LandTimeUp = true;
	if (sti(NetServer.GameOver)) { return; }
	NetServer.GameOver = true;
	int iK0 = sti(NetServer.LandTeamKills0);
	int iK1 = sti(NetServer.LandTeamKills1);
	if (iK0 > iK1) { NetServer_LandAnnounceTeamOver(0, 0); return; }
	if (iK1 > iK0) { NetServer_LandAnnounceTeamOver(1, 0); return; }
	NetServer_LandAnnounceTeamOver(0, 1);
}

// twice a second in a team duel: the time limit, and (no respawn) a team that has lost its last living player by a leave
void NetServer_LandTimeCheck()
{
	int iRoundT = GetEventData();
	if (iRoundT != sti(NetServer.Round)) { return; }
	if (!bServerGameStarted || !NetServer_LandTeamGame()) { return; }
	if (sti(NetServer.LandEndAt) != 0 && !sti(NetServer.LandTimeUp))
	{
		if (sti(NetServer.LandEndAt) - Net_GetTickCount() <= 0) { NetServer_LandTimeUp(); }
	}
	if (!sti(NetServer.Respawn) && !sti(NetServer.GameOver)) { NetServer_LandCheckOverTeams(); }
	PostEvent("NetServer_LandTimeCheck", 500, "l", iRoundT);
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
	// on land: 1 = no pistols, + 2 = the pistols load in the fight (COOP s31: "Дозарядка пистолей" of the Create game screen)
	int iLandSw = sti(NetServer.NoBoarding);
	if (CheckAttribute(&NetServer, "PistolReload") && sti(NetServer.PistolReload)) { iLandSw = iLandSw + 2; }
	NMAddByte(iSMsg, iLandSw);
	NMAddString(iSMsg, NetServer.WeatherID, 64);		// the time of day and the weather of the server (the same for all)
	NMAddDword(iSMsg, NetServer_LandTimeLeftMs());		// COOP: team duel: ms to the end of the match (0 = no limit)
	NetServer_SendMessage(wID, iSMsg, true);
	NMDelete(iSMsg);
	float fStartSp = 1.0;
	if (CheckAttribute(&NetServer, "GameSpeed")) { fStartSp = stf(NetServer.GameSpeed); }
	if (fStartSp < 0.5) { fStartSp = 1.0; }
	if (fStartSp > 3.0) { fStartSp = 3.0; }
	NetServer_LandAdminTo(wID, NET_LANDADM_SPEED, DST_INVALID, makeint(fStartSp * 4.0 + 0.5));
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
	NetServer_LandBotsReset();
	SetEventHandler("NetServer_LandRespawn", "NetServer_LandRespawn", 0);
	SetEventHandler("NetServer_LandBotCorpse", "NetServer_LandBotCorpse", 0);
	// COOP: team duel: the score, the time limit (only with respawn: without it the game ends by the death of a team)
	NetServer.LandTeamKills0 = 0;
	NetServer.LandTeamKills1 = 0;
	NetServer.LandTimeUp = false;
	NetServer.LandEndAt = 0;
	DelEventHandler("NetServer_LandTimeCheck", "NetServer_LandTimeCheck");
	if (NetServer_LandTeamGame())
	{
		if (sti(NetServer.Respawn) && sti(NetServer.TimeLimit) > 0) { NetServer.LandEndAt = Net_GetTickCount() + sti(NetServer.TimeLimit) * 60000; }
		SetEventHandler("NetServer_LandTimeCheck", "NetServer_LandTimeCheck", 0);
		PostEvent("NetServer_LandTimeCheck", 1000, "l", sti(NetServer.Round));
	}
	for (int i=0; i<NET_MAXPLAYERS; i++)
	{
		if (!sti(NSClients[i].Use)) { continue; }
		NSClients[i].LandKills = 0;
		NSClients[i].LandDeaths = 0;
		NSClients[i].LandDmg = 0.0;
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
	NetServer_LandBotsTo(wID);
	for (int iSt=0; iSt<NET_MAXPLAYERS; iSt++)
	{
		if (!sti(NSClients[iSt].Use)) { continue; }
		NetServer_LandSendStat(wID, iSt);
	}
	NetServer_BotSendOpts(wID);		// the respawn and the difficulty of the bots
	NetServer_LandSendTeamScore(wID);		// COOP: team duel
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
	DelEventHandler("NetServer_LandTimeCheck", "NetServer_LandTimeCheck");		// COOP: team duel
}

// to every player of the duel except one
void NetServer_LandSendOthers(int wExcept, int iSMsg, bool bGaranted)
{
	// COOP s37: at sea only the captains of the boarding duel of the message (iNetSrvBDScope) get it
	bool bSeaDuel = !NetServer_LandMode();
	if (bSeaDuel && iNetSrvBDScope <= 0) { return; }
	for (int i=0; i<NET_MAXPLAYERS; i++)
	{
		if (i == wExcept) { continue; }
		if (!sti(NSClients[i].Use)) { continue; }
		if (!sti(NSClients[i].InGame)) { continue; }
		if (bSeaDuel) { if (NetServer_BDOf(i) != iNetSrvBDScope) { continue; } }
		NetServer_SendMessage(i, iSMsg, bGaranted);
	}
}

void NetServer_OnLand(int wID, int iMsg)
{
	int iSMsg;
	if (!bServerGameStarted) { return; }
	if (wID < 0 || wID >= NET_MAXPLAYERS) { return; }
	if (!sti(NSClients[wID].Use)) { return; }
	// COOP s37: at sea the land messages are the ones of the captains of a boarding duel (net\Board.c)
	iNetSrvBDScope = 0;
	if (!NetServer_LandMode())
	{
		iNetSrvBDScope = NetServer_BDOf(wID);
		if (iNetSrvBDScope <= 0) { return; }
	}

	int iSub = NMGetByte(iMsg);
	if (iSub == NSC_LAND_BOTSTATE || iSub == NSC_LAND_BOTHIT || iSub == NSC_LAND_BOTHITP) { NetServer_OnLandBot(wID, iSub, iMsg); return; }
	if (iSub == NSC_LAND_DMG) { NetServer_OnLandDmg(wID, iMsg); return; }
	if (iSub == NSC_LAND_BOTEVT) { NetServer_OnLandBotEvt(wID, iMsg); return; }		// COOP s28
	if (iSub == NSC_LAND_NPCLIST || iSub == NSC_LAND_NPCSTATE) { NetServer_OnLandNpc(wID, iSub, iMsg); return; }		// COOP s35
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
			int iEn = NMGetWord(iMsg);
			int iEnMax = NMGetWord(iMsg);

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
			NMAddWord(iSMsg, iEn);
			NMAddWord(iSMsg, iEnMax);
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
			int iHitSeq = NMGetByte(iMsg);
			if (wVictim < 0 || wVictim >= NET_MAXPLAYERS || wVictim == wID) { return; }
			if (!sti(NSClients[wVictim].Use) || !sti(NSClients[wVictim].InGame)) { return; }
			if (sti(NSClients[wVictim].LandDead) || sti(NSClients[wID].LandDead)) { return; }
			// COOP: team duel: allies do not hurt each other
			if (NetServer_LandTeamGame()) { if (NetServer_LandTeamOf(wID) == NetServer_LandTeamOf(wVictim)) { return; } }
			iSMsg = NMCreate();
			NMAddByte(iSMsg, NC_LAND);
			NMAddByte(iSMsg, NSC_LAND_HIT_R);
			NMAddClientID(iSMsg, wID);
			NMAddWord(iSMsg, iDamage);
			NMAddWord(iSMsg, iDamageB);
			NMAddWord(iSMsg, iHitDelay);
			NMAddByte(iSMsg, iKind);
			NMAddByte(iSMsg, iHitSeq);
			// not guaranteed (a lost guaranteed packet is sent again late, the hit or the dodge shows half a second
			// late): two copies instead, the victim drops the second by the sequence byte
			NetServer_SendMessage(wVictim, iSMsg, false);
			NetServer_SendMessage(wVictim, iSMsg, false);
			NMDelete(iSMsg);
		break;

		case NSC_LAND_ANIM:
			string sAnimA0 = NMGetString(iMsg);
			string sAnimA1 = NMGetString(iMsg);
			string sAnimA2 = NMGetString(iMsg);
			int iAnimFS = NMGetByte(iMsg);
			int iAnimFB = NMGetByte(iMsg);
			int iAnimAckF = NMGetByte(iMsg);
			int iAnimAckS = NMGetByte(iMsg);
			iSMsg = NMCreate();
			NMAddByte(iSMsg, NC_LAND);
			NMAddByte(iSMsg, NSC_LAND_ANIM_R);
			NMAddClientID(iSMsg, wID);
			NMAddString(iSMsg, sAnimA0, 24);
			NMAddString(iSMsg, sAnimA1, 24);
			NMAddString(iSMsg, sAnimA2, 24);
			NMAddByte(iSMsg, iAnimFS);
			NMAddByte(iSMsg, iAnimFB);
			NMAddByte(iSMsg, iAnimAckF);
			NMAddByte(iSMsg, iAnimAckS);
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
			NetServer_SendMessage(wResAtt, iSMsg, false);
			NetServer_SendMessage(wResAtt, iSMsg, false);
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
	// the number of this death: its respawn timer does nothing when the player is alive again (the "heal" of the admin)
	int iDeathN = 1;
	if (CheckAttribute(&NSClients[wID], "LandDeathN")) { iDeathN = sti(NSClients[wID].LandDeathN) + 1; }
	NSClients[wID].LandDeathN = iDeathN;

	int iKills = 0;
	bool bTeamScore = false;
	if (wKiller >= 0 && wKiller < NET_MAXPLAYERS && wKiller != wID)
	{
		// COOP: team duel: the death by an ally (poison, ...) is not a kill; a kill adds to the score of the team of the killer
		bool bCountKill = true;
		if (NetServer_LandTeamGame())
		{
			if (NetServer_LandTeamOf(wKiller) == NetServer_LandTeamOf(wID)) { bCountKill = false; }
		}
		if (bCountKill)
		{
			NSClients[wKiller].LandKills = sti(NSClients[wKiller].LandKills) + 1;
			if (NetServer_LandTeamGame())
			{
				if (NetServer_LandTeamOf(wKiller) == 0) { NetServer.LandTeamKills0 = sti(NetServer.LandTeamKills0) + 1; }
				else { NetServer.LandTeamKills1 = sti(NetServer.LandTeamKills1) + 1; }
				bTeamScore = true;
			}
		}
		iKills = sti(NSClients[wKiller].LandKills);
	}
	else
	{
		if (!NetServer_IsLandBot(wKiller)) { wKiller = DST_INVALID; }		// a bot killed him: its name in the kill line
		// COOP s28: the team duel: a bot of the other team killed him: a kill of that team
		int iBotSideK = NetServer_LandSideOf(wKiller);
		int iVictimSideK = NetServer_LandSideOf(wID);
		if (wKiller != DST_INVALID && iBotSideK >= 0 && iVictimSideK >= 0 && iBotSideK != iVictimSideK)
		{
			if (iBotSideK == 0) { NetServer.LandTeamKills0 = sti(NetServer.LandTeamKills0) + 1; }
			else { NetServer.LandTeamKills1 = sti(NetServer.LandTeamKills1) + 1; }
			bTeamScore = true;
		}
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
	NetServer_LandSendStat(DST_INVALID, wID);
	if (wKiller >= 0 && wKiller < NET_MAXPLAYERS) { NetServer_LandSendStat(DST_INVALID, wKiller); }
	if (bTeamScore) { NetServer_LandSendTeamScore(DST_INVALID); }		// COOP: team duel
	if (!NetServer_LandMode()) { NetServer_BDCaptainDead(wID); return; }		// COOP s37: the boarding duel ends

	if (sti(NetServer.Respawn))
	{
		PostEvent("NetServer_LandRespawn", 1000 * sti(NetServer.RespawnTime), "lll", wID, sti(NetServer.Round), iDeathN);
		return;
	}
	NetServer_LandCheckOver();
}

// without respawn: the last one standing wins (like the deathmatch at sea); the game goes on until the players leave
void NetServer_LandCheckOver()
{
	if (sti(NetServer.GameOver)) { return; }
	if (NetServer_LandTeamGame()) { NetServer_LandCheckOverTeams(); return; }		// COOP: team duel: a team without a living player loses
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
	int iDeathR = GetEventData();
	if (iRound != sti(NetServer.Round)) { return; }
	if (!bServerGameStarted || !NetServer_LandMode()) { return; }
	if (!sti(NSClients[wID].Use) || !sti(NSClients[wID].InGame)) { return; }
	// alive already (the admin brought him back) or another death since: this timer is old
	if (!sti(NSClients[wID].LandDead)) { Trace("COOP: land the respawn timer of " + wID + " is old: he is alive"); return; }
	if (CheckAttribute(&NSClients[wID], "LandDeathN")) { if (iDeathR != sti(NSClients[wID].LandDeathN)) { return; } }

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
		case NSC_LAND_ADMIN_R:		NetLand_OnAdmin(iMsg);		break;
		case NSC_LAND_BOTSTATE_R:	NetLand_OnBotState(iMsg);	break;
		case NSC_LAND_BOTHIT_R:		NetLand_OnBotHit(iMsg);		break;
		case NSC_LAND_BOTINFO_R:	NetLand_OnBotInfo(iMsg);	break;
		case NSC_LAND_DMG_R:		NetLand_OnDmg(iMsg);		break;
		case NSC_LAND_STAT_R:		NetLand_OnStat(iMsg);		break;
		case NSC_LAND_TEAMSCORE_R:	NetLand_OnTeamScore(iMsg);	break;
		case NSC_LAND_NPCLIST_R:	NetLand_OnNpcList(iMsg);	break;		// COOP s35
		case NSC_LAND_NPCSTATE_R:	NetLand_OnNpcState(iMsg);	break;
		case NSC_LAND_BOARD_START:	NetBoard_OnStart(iMsg);		break;		// COOP s37: the real boarding (net\Board.c)
		case NSC_LAND_BOARD_END:	NetBoard_OnEnd(iMsg);		break;
	}
}

// ---------- start / end ----------
void NetLand_OnStart(int iMsg)
{
	string sLoc = NMGetString(iMsg);
	NetLand_NpcReset();		// COOP s35
	int iSpawn = NMGetByte(iMsg);
	int iRespawn = NMGetByte(iMsg);
	int iRespawnSec = NMGetByte(iMsg);
	int iNoPistol = NMGetByte(iMsg);
	int iPistolReload = iNoPistol / 2;		// COOP s31: + 2 = the pistols load in the fight
	iNoPistol = iNoPistol - iPistolReload * 2;
	sNetLandWeather = NMGetString(iMsg);
	int iLandTimeLeft = NMGetDword(iMsg);		// COOP: team duel: ms to the end of the match (0 = no limit)

	if (bNetLandActive) { return; }
	// COOP s27: a location of TDM (LandTdm.c) is put into its slot first
	if (NetTdm_IsKey(sLoc))
	{
		string sTdmKey = sLoc;
		sLoc = NetTdm_Prepare(sTdmKey);
		if (sLoc == "") { Trace("COOP: land duel: no TDM location " + sTdmKey); return; }
	}
	if (FindLocation(sLoc) < 0) { Trace("COOP: land duel: no location " + sLoc); return; }
	// COOP s27: "Clone_location" is the template of the single player (a quest copies another location into it); its own farm
	// field lies in locations\decks\FarmField, the path "locations\" of the template found no model (a dark screen)
	if (sLoc == "Clone_location")
	{
		int iCloneIdx = FindLocation(sLoc);
		Locations[iCloneIdx].filespath.models = "locations\decks\FarmField";
	}

	bNetLandActive = true;
	bNetLandLocReady = false;
	bNetLandDead = false;
	sNetLandLoc = sLoc;
	// the turn of the musket in the right hand: set here (the start values of the globals did not reach the game)
	iNetLandMushRX = NET_LAND_MUSH_RX;
	iNetLandMushRY = NET_LAND_MUSH_RY;
	iNetLandMushRZ = NET_LAND_MUSH_RZ;
	Trace("COOP: land the musket in the right hand: " + iNetLandMushRX + " " + iNetLandMushRY + " " + iNetLandMushRZ);
	NetLand_SwimClearAll();
	iNetLandSpawn = iSpawn;
	iNetLandRespawn = iRespawn;
	iNetLandRespawnSec = iRespawnSec;
	iNetLandNoPistol = iNoPistol;
	// COOP s31: the option of the single player (Loc_ai\LAi_character.c: a gun loads in the fight mode too) is the switch of
	// the server for the duel; the single player value comes back at the end (NetLand_ClientStop)
	if (!CheckAttribute(&InterfaceStates, "NetLandRplOld")) { InterfaceStates.NetLandRplOld = bRechargePistolOnLine; }
	bRechargePistolOnLine = (iPistolReload != 0);
	Trace("COOP: land the pistols load in the fight " + iPistolReload);
	iNetLandGen++;
	// COOP: team duel: the score, the end of the match, the spawn bases of the teams
	iNetLandTeamKills0 = 0;
	iNetLandTeamKills1 = 0;
	iNetLandMatchEnd = 0;
	if (iLandTimeLeft > 0) { iNetLandMatchEnd = Net_GetTickCount() + iLandTimeLeft; }
	iNetLandAnchorA = -1;
	iNetLandAnchorB = -1;
	sNetLandAnchorLoc = "";
	iNetLandTimerGen++;
	for (int i=0; i<NET_MAXPLAYERS; i++) { DeleteAttribute(&NCClients[i], "LandModel"); DeleteAttribute(&NCClients[i], "LandLo"); }

	bNetClientInGame = true;
	NCServer.GameRunning = true;
	Event("NCEvent_StartGame");		// the buy screen closes

	SetEventHandler("NetLand_Load", "NetLand_Load", 0);
	SetEventHandler("NetLand_AfterLoad", "NetLand_AfterLoad", 0);
	SetEventHandler("NetLand_Tick", "NetLand_Tick", 0);
	SetEventHandler("NetLand_ActEnd", "NetLand_ActEnd", 0);
	SetEventHandler("NetLand_Smooth", "NetLand_Smooth", 0);
	SetEventHandler("NetLand_FStateCheck", "NetLand_FStateCheck", 0);
	SetEventHandler("NetLand_StaggerEnd", "NetLand_StaggerEnd", 0);
	SetEventHandler("NetLand_HitApply", "NetLand_HitApply", 0);
	SetEventHandler("NetLand_ReEquipGun", "NetLand_ReEquipGun", 0);
	SetEventHandler("NetLand_PreviewDoll", "NetLand_PreviewDoll", 0);		// COOP: the skin preview
	SetEventHandler("NetLand_BotPlace", "NetLand_BotPlace", 0);
	SetEventHandler("NetLand_ActResend", "NetLand_ActResend", 0);
	SetEventHandler("NetLand_ReactEnd", "NetLand_ReactEnd", 0);
	SetEventHandler("NetLand_HitReact", "NetLand_HitReact", 0);
	SetEventHandler("NetLand_Rearm", "NetLand_Rearm", 0);
	SetEventHandler("NetLand_AnimClear", "NetLand_AnimClear", 0);
	SetEventHandler("NetLand_PredEnd", "NetLand_PredEnd", 0);
	SetEventHandler("NetLand_AttRes", "NetLand_AttRes", 0);
	SetEventHandler("NetLand_ReactCheck", "NetLand_ReactCheck", 0);
	SetEventHandler("NetLand_PupReactCheck", "NetLand_PupReactCheck", 0);
	SetEventHandler("NetLand_SkipPoll", "NetLand_SkipPoll", 0);
	SetEventHandler("NetLand_PreBlock", "NetLand_PreBlock", 0);
	SetEventHandler("NetLand_ArmThaw", "NetLand_ArmThaw", 0);
	SetEventHandler("NetLand_PupHitVar", "NetLand_PupHitVar", 0);
	SetEventHandler("NetLand_TimerTick", "NetLand_TimerTick", 0);
	if (NetLand_TeamGame()) { PostEvent("NetLand_TimerTick", 1500, "l", iNetLandTimerGen); }
	iNetLandStunUntil = 0;
	NetLand_LockFight(0);
	fNetLandFaceOff[0] = -0.785;
	fNetLandFaceOff[1] = 0.785;
	fNetLandFaceOff[2] = 0.785;
	fNetLandFaceOff[3] = -0.785;
	DeleteAttribute(&oNetLandAnimDur, "");
	bNetLandDeadTask = false;
	bNetLandRespawnLoad = false;
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
	// the hero of the single player is kept once: a second call (a new load of the location) would save the duel hero
	if (!CheckAttribute(&oNetLandHero, "index"))
	{
		DeleteAttribute(&oNetLandHero, "");
		CopyAttributes(&oNetLandHero, mc);
	}

	NetLand_LoInit();
	DeleteAttribute(&NCClients[wClientID], "LandModel");
	string sHeroModel = NetLand_ModelFor(wClientID);
	mc.model = sHeroModel;
	mc.NetLandSkin = sHeroModel;
	mc.model.animation = NetLand_ModelAnim(sHeroModel);
	mc.model.height = NetLand_ModelHeight(sHeroModel);
	mc.sex = NetLand_ModelSex(sHeroModel);
	NetLand_SetHeroFace(mc);		// the portrait of the hero in the interfaces, like the model above (not the one of Blaze)
	if (CheckAttribute(&NetClient, "NickName")) { mc.name = NetClient.NickName; mc.lastname = ""; }

	DeleteAttribute(mc, "chr_ai.hp");
	aref arLo;
	makearef(arLo, oNetLandLo);
	NetLand_ApplyLo(mc, arLo, true, true);
	LAi_SetImmortal(mc, true);		// the death of the hero is ours (NetLand_LocalDie), never the game over of single player
}

// the weather of the server (Create game, the admin menu) for the location: its hour and the weather itself, so that every
// player has the same time of day (the location makes its weather by the clock of the game, each machine had its own)
void NetLand_ApplyWeather()
{
	if (sNetLandWeather == "") { return; }
	for (int i=0; i<MAX_WEATHERS; i++)
	{
		if (!CheckAttribute(&Weathers[i], "id")) { continue; }
		if (Weathers[i].id != sNetLandWeather) { continue; }
		int iHour = 12;
		if (CheckAttribute(&Weathers[i], "Hour.Min")) { iHour = sti(Weathers[i].Hour.Min); }
		SetCurrentTime(iHour, 0);
		worldMap.date.hour = iHour;
		iNextWeatherNum = i;
		Trace("COOP: land weather " + sNetLandWeather + " (" + i + "), hour " + iHour);
		return;
	}
	Trace("COOP: land weather " + sNetLandWeather + " is unknown");
}

void NetLand_Load()
{
	NetLand_ApplyWeather();
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
		// COOP s37: the boarding deck neither (the sea of the net game waits behind it, its Sea object is the one of the sharks)
		if ((CheckAttribute(rLoc, "CabinType") || bNetBoardDuel) && CheckAttribute(rLoc, "environment.sea")) { oNetLandLocSave.sea = rLoc.environment.sea; rLoc.environment.sea = "false"; }
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
	// the door of the location if it has one (the hold, the gun deck, the crew cabin of the ship are "boarding" locations with doors),
	// "rld" otherwise (the decks, the cabins)
	if (CheckAttribute(rLoc, "reload"))
	{
		aref arReload;
		makearef(arReload, rLoc.reload);
		if (GetAttributesNum(arReload) > 0)
		{
			aref arFirst = GetAttributeN(arReload, 0);
			if (CheckAttribute(arFirst, "name"))
			{
				sGroup = "reload";
				sLocator = arFirst.name;
			}
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
	// COOP s37: the boarding deck: our hero, the puppet of the other captain and the crews at the sides of their ships
	if (bNetBoardDuel)
	{
		int iBdTeam = -1;
		if (CheckAttribute(chr, "index")) { if (sti(chr.index) == GetMainCharacterIndex()) { iBdTeam = iNetBoardTeam; } }
		if (iBdTeam < 0 && CheckAttribute(chr, "NetLandID")) { iBdTeam = NetLand_TeamOf(sti(chr.NetLandID)); }
		if (iBdTeam < 0 && CheckAttribute(chr, "NetLandBotID"))
		{
			int iBdRel = NetLand_BotRelOf(sti(chr.NetLandBotID));
			if (iBdRel >= NET_BOTREL_TEAM0) { iBdTeam = iBdRel - NET_BOTREL_TEAM0; }
			iSpawn = sti(chr.NetLandBotID) + 1;		// the fighters one by one at the locators of their side
		}
		if (iBdTeam >= 0) { if (NetBoard_TeleportToSide(chr, iBdTeam, iSpawn)) { return true; } }
	}
	string sGroup = "rld";
	if (!CheckAttribute(loadedLocation, "locators.rld")) { sGroup = "goto"; }
	if (!CheckAttribute(loadedLocation, "locators." + sGroup)) { sGroup = "reload"; }
	if (!CheckAttribute(loadedLocation, "locators." + sGroup)) { return false; }

	aref arGroup;
	makearef(arGroup, loadedLocation.locators.(sGroup));
	int iNum = GetAttributesNum(arGroup);
	if (iNum <= 0) { return false; }
	int iSpIdx = iSpawn - (iSpawn / iNum) * iNum;
	// COOP: team duel: the team of the character (our hero or the puppet of a player), -1 = none (the usual duel, a bot)
	int iTeamSp = -1;
	if (iNum >= 2 && NetLand_TeamGame())
	{
		if (CheckAttribute(chr, "index")) { if (sti(chr.index) == GetMainCharacterIndex()) { iTeamSp = NetLand_TeamOf(wClientID); } }
		if (iTeamSp < 0 && CheckAttribute(chr, "NetLandID")) { iTeamSp = NetLand_TeamOf(sti(chr.NetLandID)); }
	}
	if (iTeamSp >= 0) { iSpIdx = NetLand_TeamSpawnIdx(arGroup, iNum, iTeamSp, iSpawn); }
	aref arLoc = GetAttributeN(arGroup, iSpIdx);
	return TeleportCharacterToPosAy(chr, stf(arLoc.x), stf(arLoc.y), stf(arLoc.z), 0.0);
}

// COOP: team duel: the two spawn locators that are the farthest from each other are the bases of the two teams, a team
// spawns on the locators that are nearer to its base (the n-th spawn of the team takes the n-th locator of its side)
int NetLand_TeamSpawnIdx(aref arGroup, int iNum, int iTeam, int iSpawn)
{
	if (iNetLandAnchorA < 0 || sNetLandAnchorLoc != sNetLandLoc)
	{
		sNetLandAnchorLoc = sNetLandLoc;
		iNetLandAnchorA = 0;
		iNetLandAnchorB = 1;
		float fFar = -1.0;
		for (int iLa=0; iLa<iNum; iLa++)
		{
			aref arLa = GetAttributeN(arGroup, iLa);
			for (int iLb=iLa+1; iLb<iNum; iLb++)
			{
				aref arLb = GetAttributeN(arGroup, iLb);
				float fDx = stf(arLa.x) - stf(arLb.x);
				float fDz = stf(arLa.z) - stf(arLb.z);
				float fDist = fDx * fDx + fDz * fDz;
				if (fDist > fFar) { fFar = fDist; iNetLandAnchorA = iLa; iNetLandAnchorB = iLb; }
			}
		}
	}
	aref arBaseA = GetAttributeN(arGroup, iNetLandAnchorA);
	aref arBaseB = GetAttributeN(arGroup, iNetLandAnchorB);
	float fAX = stf(arBaseA.x);
	float fAZ = stf(arBaseA.z);
	float fBX = stf(arBaseB.x);
	float fBZ = stf(arBaseB.z);

	// the locators of this team: the ones that are not farther from its base than from the other one
	int iCount = 0;
	int iLc;
	for (iLc=0; iLc<iNum; iLc++)
	{
		aref arLc = GetAttributeN(arGroup, iLc);
		float fToA = (stf(arLc.x) - fAX) * (stf(arLc.x) - fAX) + (stf(arLc.z) - fAZ) * (stf(arLc.z) - fAZ);
		float fToB = (stf(arLc.x) - fBX) * (stf(arLc.x) - fBX) + (stf(arLc.z) - fBZ) * (stf(arLc.z) - fBZ);
		int iLcTeam = 1;
		if (fToA <= fToB) { iLcTeam = 0; }
		if (iLcTeam == iTeam) { iCount++; }
	}
	if (iCount <= 0) { return iSpawn - (iSpawn / iNum) * iNum; }

	int iWant = iSpawn - (iSpawn / iCount) * iCount;
	int iSeen = 0;
	for (iLc=0; iLc<iNum; iLc++)
	{
		aref arLd = GetAttributeN(arGroup, iLc);
		float fToA2 = (stf(arLd.x) - fAX) * (stf(arLd.x) - fAX) + (stf(arLd.z) - fAZ) * (stf(arLd.z) - fAZ);
		float fToB2 = (stf(arLd.x) - fBX) * (stf(arLd.x) - fBX) + (stf(arLd.z) - fBZ) * (stf(arLd.z) - fBZ);
		int iLdTeam = 1;
		if (fToA2 <= fToB2) { iLdTeam = 0; }
		if (iLdTeam != iTeam) { continue; }
		if (iSeen == iWant) { return iLc; }
		iSeen++;
	}
	return iSpawn - (iSpawn / iNum) * iNum;
}

void NetLand_AfterLoad()
{
	int iGen = GetEventData();
	if (iGen != iNetLandGen || !bNetLandActive) { return; }

	ref mc = GetMainCharacter();
	bNetLandLocReady = true;
	NetLand_FixEntry();
	if (!CheckAttribute(mc, "BaseNation")) { mc.BaseNation = PIRATE; }
	// "SetBlockHold" 2 empties the table of the characters that hold the block (the old location has gone); an old dll does not
	// know the message and answers 0: then the block of a puppet is resolved by the script as before
	bNetLandHoldOk = false;
	if (SendMessage(mc, "lsl", MSG_CHARACTER_EX_MSG, "SetBlockHold", 2) == 1) { bNetLandHoldOk = true; }
	Trace("COOP: land the dll holds the blocks of the puppets: " + bNetLandHoldOk);
	bNetLandBreakOk = false;
	if (SendMessage(mc, "ls", MSG_CHARACTER_EX_MSG, "BreakFight") == 1) { bNetLandBreakOk = true; }
	Trace("COOP: land the dll breaks the actions: " + bNetLandBreakOk);
	bNetLandDuelOk = false;
	bNetLandNoFightOk = false;
	int iDuelV = SendMessage(mc, "lsl", MSG_CHARACTER_EX_MSG, "NetDuel", 1);
	if (iDuelV >= 1) { bNetLandDuelOk = true; }
	if (iDuelV >= 2) { bNetLandNoFightOk = true; }
	Trace("COOP: land the dll plays every hit reaction of the hero: " + bNetLandDuelOk + ", the hits without the sword: " + bNetLandNoFightOk);
	iNetLandAckFrom = -1;
	iNetLandAckSentFrom = -1;
	bNetLandAckPub = true;
	LAi_SetPlayerType(mc);
	LAi_LockFightMode(mc, false);		// a residence locks the fight mode when it is loaded
	LAi_group_SetRelation(NET_LAND_GROUP, LAI_GROUP_PLAYER, LAI_GROUP_ENEMY);
	LAi_group_SetRelation(NET_LAND_GROUP_ALLY, LAI_GROUP_PLAYER, LAI_GROUP_FRIEND);		// COOP: team duel
	NetLand_TeleportToSpawn(mc, iNetLandSpawn);
	NetLand_BotsAfterLoad();
	NetLand_NpcAfterLoad();		// COOP s35: the NPC of the host
	NetLand_UnderwaterClean(mc);

	if (!bNetLandRespawnLoad) { Log_SetStringToLog(LanguageConvertString(iLangNetClient, "Net_LandStart")); }
	else { Log_SetStringToLog(LanguageConvertString(iLangNetClient, "Net_LandRespawned")); }
	PostEvent("NetLand_Tick", NET_LAND_TICK, "l", iNetLandGen);
	PostEvent("NetLand_Smooth", NET_LAND_SMOOTH, "l", iNetLandGen);
	NetLand_SendLoadout();
	if (iNetPreview == 1) { PostEvent("NetLand_PreviewDoll", 1200, "l", iNetLandGen); }		// COOP: the skin preview
	// a musket: the model is made with the location, the musket is put on again when it exists
	if (CheckAttribute(&oNetLandLo, "Gun") && !iNetLandNoPistol)
	{
		if (NetLand_IsMushItem(oNetLandLo.Gun))
		{
			PostEvent("NetLand_ReEquipGun", 500, "ls", sti(mc.index), oNetLandLo.Gun);
			PostEvent("NetLand_ReEquipGun", 1800, "ls", sti(mc.index), oNetLandLo.Gun);
		}
	}
	if (bNetLandRespawnLoad)
	{
		// a respawn: a short protection, no loadout window again
		bNetLandRespawnLoad = false;
		iNetLandProtectUntil = Net_GetTickCount() + NET_RESPAWN_PROTECT;
		return;
	}
	iNetLandProtectUntil = Net_GetTickCount() + NET_RESPAWN_PROTECT;		// the loadout was chosen in the lobby: no long protection
	iNetLandStartAt = Net_GetTickCount();
	// the loadout was chosen in the lobby: no window, a reminder
	Log_SetStringToLog("F2 - снаряжение, F1 - статистика");
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
	// COOP s31: bRechargePistolOnLine of the single player was kept here by NetLand_OnStart
	if (CheckAttribute(&InterfaceStates, "NetLandRplOld"))
	{
		bRechargePistolOnLine = sti(InterfaceStates.NetLandRplOld);
		DeleteAttribute(&InterfaceStates, "NetLandRplOld");
	}
	if (!bNetLandActive) { return; }
	NetLand_SwimClearAll();
	bNetLandActive = false;
	bNetLandDead = false;
	iNetLandGen++;
	bNetLandLocReady = false;
	ref mcH = GetMainCharacter();
	// the underwater timer, the poison: nothing of the duel stays
	if (IsEntity(mcH)) { DeleteAttribute(mcH, "showTimer"); }
	ClearAllLogStrings();		// (an empty eternal string crashes the game)
	NetLand_PoisonOff();
	sNetLandWeather = "";
	if (bNetLandHoldOk && IsEntity(mcH)) { SendMessage(mcH, "lsl", MSG_CHARACTER_EX_MSG, "SetBlockHold", 2); }
	// the single player rules of the hit reactions of the player again
	if (bNetLandDuelOk && IsEntity(mcH)) { SendMessage(mcH, "lsl", MSG_CHARACTER_EX_MSG, "NetDuel", 0); }
	bNetLandDuelOk = false;
	bNetLandNoFightOk = false;
	DelEventHandler("NetLand_Load", "NetLand_Load");
	DelEventHandler("NetLand_AfterLoad", "NetLand_AfterLoad");
	DelEventHandler("NetLand_Tick", "NetLand_Tick");
	DelEventHandler("NetLand_ActEnd", "NetLand_ActEnd");
	DelEventHandler("NetLand_Smooth", "NetLand_Smooth");
	DelEventHandler("NetLand_FStateCheck", "NetLand_FStateCheck");
	DelEventHandler("NetLand_StaggerEnd", "NetLand_StaggerEnd");
	DelEventHandler("NetLand_HitApply", "NetLand_HitApply");
	DelEventHandler("NetLand_ReEquipGun", "NetLand_ReEquipGun");
	DelEventHandler("NetLand_BotPlace", "NetLand_BotPlace");
	NetLand_BotClear();
	NetLand_BotsForget(DST_INVALID);
	DelEventHandler("NetLand_ActResend", "NetLand_ActResend");
	DelEventHandler("NetLand_ReactEnd", "NetLand_ReactEnd");
	DelEventHandler("NetLand_HitReact", "NetLand_HitReact");
	DelEventHandler("NetLand_Rearm", "NetLand_Rearm");
	DelEventHandler("NetLand_AnimClear", "NetLand_AnimClear");
	DelEventHandler("NetLand_PredEnd", "NetLand_PredEnd");
	DelEventHandler("NetLand_AttRes", "NetLand_AttRes");
	DelEventHandler("NetLand_ReactCheck", "NetLand_ReactCheck");
	DelEventHandler("NetLand_PupReactCheck", "NetLand_PupReactCheck");
	DelEventHandler("NetLand_SkipPoll", "NetLand_SkipPoll");
	DelEventHandler("NetLand_PreBlock", "NetLand_PreBlock");
	DelEventHandler("NetLand_ArmThaw", "NetLand_ArmThaw");
	DelEventHandler("NetLand_PupHitVar", "NetLand_PupHitVar");
	DelEventHandler("NetLand_TimerTick", "NetLand_TimerTick");
	iNetLandTimerGen++;
	iNetLandMatchEnd = 0;
	iNetLandStunUntil = 0;
	NetLand_LockFight(0);
	bNetLandDeadTask = false;
	bNetLandRespawnLoad = false;
	DelEventHandler("Control Activation", "NetLand_CtrlOn");
	DelEventHandler("Control Deactivation", "NetLand_CtrlOff");
	bNetLandBlockHeld = false;

	// the puppets leave the location
	for (int i=0; i<NET_MAXPLAYERS; i++)
	{
		int iChr = NetLand_PupIdx(i);
		DeleteAttribute(&NCClients[i], "LandChr");
		if (iChr < 0) { continue; }
		NetLand_SwimSet(&Characters[iChr], 0);
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
	if (fNetLandSpeed < 0.99 || fNetLandSpeed > 1.01) { SetTimeScale(1.0); }
	fNetLandSpeed = 1.0;
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

// the fight state machine of a character (Location.dll patched): 1..6 a blow, 7 a shot, 0xd block, 0x10 parry, 0x11 recoil,
// 0x12 / 0x13 hops to the sides; "GetFightState" gives 0x10000 | state | variant << 8 (0 - the dll is not patched)
int NetLand_GetFState(ref chr)
{
	int iPk = SendMessage(chr, "ls", MSG_CHARACTER_EX_MSG, "GetFightState");
	if (iPk != 0) { bNetLandFStateOk = true; }
	return iPk;
}

void NetLand_SetFState(ref chr, int iState, int iSub)
{
	SendMessage(chr, "lsll", MSG_CHARACTER_EX_MSG, "SetFightState", iState, iSub);
}

// the speed of a player of the animation in percent (0 stops it: a body that lies stays as it lies)
void NetLand_AnimSpeed(ref chr, int iPlayer, int iPercent)
{
	SendMessage(chr, "lsll", MSG_CHARACTER_EX_MSG, "AnimSpeed", iPlayer, iPercent);
}

// our hero: what do the players 0, 1, 2 of his animation play? A change (and every 250 ms) goes to the others.
void NetLand_AnimSample(ref mc, int iNow)
{
	string sA0 = NetLand_GetAnim(mc, 0);
	string sA1 = NetLand_GetAnim(mc, 1);
	string sA2 = NetLand_GetAnim(mc, 2);
	int iPk = NetLand_GetFState(mc);
	int iFS = 0;
	int iFB = 0;
	if (iPk != 0)
	{
		iFS = iPk - (iPk / 256) * 256;
		int iPkHigh = iPk / 256;
		iFB = iPkHigh - (iPkHigh / 256) * 256;
		// only the blows and the parry have variants: the byte of the others changes by itself (a packet every 10 ms in a block)
		bool bVarFS = false;
		if (iFS >= 1 && iFS <= 6) { bVarFS = true; }
		if (iFS == 16) { bVarFS = true; }
		if (!bVarFS) { iFB = 0; }
		// the puppets play the animations by their names, the byte is not used: in a swing it changed every frame and an
		// animation packet went every 10 ms (the spam of blows came late)
		iFB = 0;
	}
	if (!bNetLandAnimOk && !bNetLandFStateOk) { return; }
	bool bChanged = false;
	// the hit that has come to our hero goes back to its attacker when the reaction (or the block, the dodge) shows already: an
	// animation sent before that is old for him, his blow breaks it here
	if (!bNetLandAckPub)
	{
		int iAckAgo = iNow - iNetLandAckAt;
		bool bAckNow = false;
		if (iFS >= 8 && iFS <= 16) { bAckNow = true; }
		if (iAckAgo < 0 || iAckAgo >= 120) { bAckNow = true; }
		if (bAckNow)
		{
			bNetLandAckPub = true;
			iNetLandAckSentFrom = iNetLandAckFrom;
			iNetLandAckSentSeq = iNetLandAckSeq;
			bChanged = true;
		}
	}
	if (sA0 != sNetLandAnimSent0 || sA1 != sNetLandAnimSent1 || sA2 != sNetLandAnimSent2) { bChanged = true; }
	if (iFS != iNetLandFSentState || iFB != iNetLandFSentSub) { bChanged = true; }
	int iSince = iNow - iNetLandAnimSentAt;
	if (!bChanged && iSince < 250) { return; }
	if (bChanged) { Trace("COOP: land anim " + sA0 + " | " + sA1 + " | " + sA2 + " state " + iFS + "/" + iFB); }
	iNetLandFSentState = iFS;
	iNetLandFSentSub = iFB;
	sNetLandAnimSent0 = sA0;
	sNetLandAnimSent1 = sA1;
	sNetLandAnimSent2 = sA2;
	iNetLandAnimSentAt = iNow;
	int iAMsg = NetClient_LandNewMsg(NSC_LAND_ANIM);
	NMAddString(iAMsg, sA0, 24);
	NMAddString(iAMsg, sA1, 24);
	NMAddString(iAMsg, sA2, 24);
	NMAddByte(iAMsg, iFS);
	NMAddByte(iAMsg, iFB);
	NMAddByte(iAMsg, iNetLandAckSentFrom + 1);
	NMAddByte(iAMsg, iNetLandAckSentSeq);
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
	int iHSt = NMGetByte(iMsg);
	int iHSb = NMGetByte(iMsg);
	int iAckF = NMGetByte(iMsg) - 1;
	int iAckS = NMGetByte(iMsg);
	if (!bNetLandActive || !bNetLandLocReady) { return; }
	int iChr = NetLand_GetPuppet(wID);
	if (iChr < 0) { return; }
	ref ch = &Characters[iChr];
	if (!IsEntity(ch)) { return; }
	// its player tells that our last blow has reached him
	if (iAckF == wClientID && CheckAttribute(ch, "NetLandPHitSeq"))
	{
		if (iAckS == sti(ch.NetLandPHitSeq)) { ch.NetLandPHitAck = 1; }
	}
	if (CheckAttribute(ch, "NetLandDead")) { if (sti(ch.NetLandDead)) { return; } }
	// the fight state of the hero (13 block, 14 a blow in the block, 16 a dodge...): our blows on this puppet are resolved by it
	int iOldHSt = -1;
	if (CheckAttribute(ch, "NetLandHSt")) { iOldHSt = sti(ch.NetLandHSt); }
	ch.NetLandHSt = iHSt;
	ch.NetLandHStAt = Net_GetTickCount();
	// his hero has got a blow into the block from somebody else (a bot, a third player): the puppet that holds the block by the
	// game shows it too - the name "block_hit" gives way to the held block and was never seen (our own blows into it are shown
	// by the game itself: the puppet is in 14 already then)
	if (iHSt == 14 && iOldHSt != 14 && bNetLandHoldOk && CheckAttribute(ch, "NetLandHold"))
	{
		if (sti(ch.NetLandHold) == 1)
		{
			int iPk14 = NetLand_GetFState(ch);
			int iCur14 = iPk14 - (iPk14 / 256) * 256;
			if (iCur14 == 13) { NetLand_SetFState(ch, 14, 0); }
		}
	}
	if (!CheckAttribute(ch, "NetLandAnimSync")) { return; }
	if (sti(ch.NetLandAnimSync) != 1) { return; }
	// the fight state machine of the puppet is asked for the state of the hero's (a block, a parry, a hop, a blow...). Until
	// the game has taken such a request the animation is played by its name as well
	if (iHSt != 0) { ch.NetLandFSeen = 1; }
	bool bSkipNames = false;
	if (NetLand_NativePuppet(ch))
	{
		int iOldSt = -1;
		int iOldSb = -1;
		if (CheckAttribute(ch, "NetLandFSt")) { iOldSt = sti(ch.NetLandFSt); }
		if (CheckAttribute(ch, "NetLandFSb")) { iOldSb = sti(ch.NetLandFSb); }
		if (iHSt != iOldSt || iHSb != iOldSb)
		{
			ch.NetLandFSt = iHSt;
			ch.NetLandFSb = iHSb;
			// the variant index of the hero's game means nothing to the puppet (its tables differ): the tables of the puppet get
			// the animation that the hero plays (the last digit of its name), the request goes with the index 0 (always valid)
			string sNmV = sA2;
			if (sNmV == "") { sNmV = sA0; }
			int iNmLen = strlen(sNmV);
			bool bVarState = false;
			if (iHSt >= 1 && iHSt <= 6) { bVarState = true; }
			if (iHSt == 16) { bVarState = true; }
			if (iNmLen > 2 && bVarState && NetLand_FStateVerifiable(iHSt))
			{
				int iVarN = sti(strcut(sNmV, iNmLen - 1, iNmLen - 1));
				if (iVarN >= 1 && iVarN <= 4)
				{
					int iCodeV = iHSt;
					if (iHSt == 6) { iCodeV = NET_LANDACT_FEINTC; }
					if (iHSt == 16) { iCodeV = NET_LANDACT_PARRY; }
					NetLand_SetVariant(ch, iCodeV, iVarN);
				}
			}
			bool bOldNat = NetLand_FStateVerifiable(iOldSt);
			// the parser of the scripts does not take "||" together with "&&" in one expression: two checks instead
			bool bNeedState = false;
			int iSetSt = iHSt;
			if (NetLand_FStateVerifiable(iHSt)) { bNeedState = true; }
			// the puppet was in a blow and its hero is in something else now (a block, a hop, a hit, nothing): the blow is
			// ended on the puppet, else it stands in the state 1 and neither blocks nor is hit like the player
			if (bOldNat && !NetLand_FStateVerifiable(iHSt)) { bNeedState = true; iSetSt = 0; }
			if (bNeedState)
			{
				NetLand_SetFState(ch, iSetSt, 0);
				iNetLandFReqCounter++;
				ch.NetLandFReq = iNetLandFReqCounter;
				if (NetLand_FStateVerifiable(iSetSt)) { PostEvent("NetLand_FStateCheck", 120, "llll", iNetLandGen, iChr, iSetSt, iNetLandFReqCounter); }
			}
		}
		if (NetLand_FStateVerifiable(iHSt) && NetLand_NativeVerified(ch)) { bSkipNames = true; }
	}
	if (bSkipNames) { return; }
	// the script action (player 2) is what the hero was told to play, else the body (player 0)
	string sPick = sA2;
	if (sPick == "") { sPick = sA0; }
	string sCur = "";
	if (CheckAttribute(ch, "NetLandAnimCur")) { sCur = ch.NetLandAnimCur; }
	ch.NetLandAnimLast = sPick;
	if (sPick == sCur) { return; }
	if (CheckAttribute(ch, "NetLandBlkPose")) { if (sti(ch.NetLandBlkPose) == 1) { return; } }
	NetLand_PuppetHoldBlock(ch, Net_GetTickCount());
	if (CheckAttribute(ch, "NetLandHold")) { if (sti(ch.NetLandHold) == 1) { return; } }
	// a reaction that our machine has shown at once (a blow in the block of this puppet): the old animation that is still on the
	// way does not take it back; the newest one is played when the hold is over (NetLand_PredEnd)
	if (CheckAttribute(ch, "NetLandPredUntil"))
	{
		int iPredLeft = sti(ch.NetLandPredUntil) - Net_GetTickCount();
		if (iPredLeft > 0) { return; }
	}
	// our blow has put the puppet into its hit reaction and has not reached its player yet: his swing / hop / block of this packet
	// began before it and is broken by it on his screen (a swing begun so late cannot reach us before). It was played here, then the
	// reaction came again with his packet: the hits of the puppet and of the player did not match
	// (a reaction name of that moment also: the hit into a feint was played once more after the counter blow)
	if (NetLand_PupHitUnacked(ch)) { return; }
	// the puppet still shows the reaction of our blow: an older reaction name of its player (it is reported for a moment while his
	// game begins the new one) is not played over it
	if (findSubStr(sPick, "hit_", 0) == 0 && CheckAttribute(ch, "NetLandPHitName") && CheckAttribute(ch, "NetLandPHitAt"))
	{
		int iPHAgo = Net_GetTickCount() - sti(ch.NetLandPHitAt);
		string sPHName = ch.NetLandPHitName;
		if (iPHAgo >= 0 && iPHAgo < 900 && sPick != sPHName && bNetLandFStateOk)
		{
			int iPkPH = NetLand_GetFState(ch);
			int iCurPH = iPkPH - (iPkPH / 256) * 256;
			if (iCurPH >= 8 && iCurPH <= 12) { return; }
		}
	}
	// our blow has put the puppet into the game's hit reaction; its player may be swinging again already (on his screen our hit
	// came later or broke the swing only then). The reaction of the game hid the new swing: the damage came, the swing was not
	// seen. An action of its player (a swing, a hop, a dodge, a shot) breaks the reaction; the stand / idle names wait for its end
	if (bNetLandFStateOk)
	{
		int iPkO = NetLand_GetFState(ch);
		int iCurO = iPkO - (iPkO / 256) * 256;
		if (iCurO >= 8 && iCurO <= 12)
		{
			if (NetLand_IsActionName(sPick))
			{
				if (bNetLandBreakOk)
				{
					SendMessage(ch, "ls", MSG_CHARACTER_EX_MSG, "BreakFight");
					if (iNetLandPupLog < 60) { iNetLandPupLog++; Trace("COOP: land puppet " + ch.id + ": its player does " + sPick + ", the hit reaction " + iCurO + " is broken"); }
				}
			}
			else
			{
				// the same reaction of its player: it plays already
				if (sPick == NetLand_GetAnim(ch, 0)) { ch.NetLandAnimCur = sPick; return; }
				if (findSubStr(sPick, "hit_", 0) != 0) { return; }
			}
		}
	}
	NetLand_PuppetPlayName(ch, iChr, sPick);
}

// our blow has put this puppet into its hit reaction and its player has not told yet that it has reached him
bool NetLand_PupHitUnacked(ref ch)
{
	if (!CheckAttribute(ch, "NetLandPHitSeq")) { return false; }
	if (!CheckAttribute(ch, "NetLandPHitAt")) { return false; }
	if (CheckAttribute(ch, "NetLandPHitAck")) { if (sti(ch.NetLandPHitAck) == 1) { return false; } }
	int iAgoU = Net_GetTickCount() - sti(ch.NetLandPHitAt);
	if (iAgoU < 0 || iAgoU >= NET_LAND_ACK_WAIT) { return false; }
	return true;
}

// an action that the player begins himself (the puppet must show it at once)
bool NetLand_IsActionName(string sA)
{
	if (findSubStr(sA, "attack_", 0) == 0) { return true; }
	if (findSubStr(sA, "straif_", 0) == 0) { return true; }
	if (findSubStr(sA, "Recoil", 0) == 0) { return true; }
	if (findSubStr(sA, "recoil", 0) == 0) { return true; }
	if (findSubStr(sA, "parry_", 0) == 0) { return true; }
	if (findSubStr(sA, "Shot", 0) == 0) { return true; }
	if (findSubStr(sA, "block", 0) == 0) { return true; }
	return false;
}

// the puppet plays an animation of its hero by the name
void NetLand_PuppetPlayName(ref ch, int iChr, string sPick)
{
	string sCur = "";
	if (CheckAttribute(ch, "NetLandAnimCur")) { sCur = ch.NetLandAnimCur; }
	int iNowA = Net_GetTickCount();
	// a swimmer (NetSwim 1: the game knows it swims) gets the animation of the swim from the game itself: the swim of its
	// player played by name over the game's own one may be what jerked (/swim 1 plays it by name again)
	if (iNetLandSwimPlay == 0 && findSubStr(sPick, "wim", 0) >= 0)
	{
		DeleteAttribute(ch, "NetLandClearId");
		ch.NetLandAnimAt = iNowA;
		ch.NetLandAnimCur = sPick;
		CharacterPlayAction(ch, "");
		return;
	}
	// the clear timer of the one-shot animation before this one must not stop this one (a hop, then W: the run was cleared
	// and the puppet slid standing)
	DeleteAttribute(ch, "NetLandClearId");
	if (sCur != "" && NetLand_OneShot(sCur) && CheckAttribute(ch, "NetLandAnimAt")) { NetLand_LearnAnimDur(sCur, iNowA - sti(ch.NetLandAnimAt)); }
	// the shot is over: the state machine of the puppet is reset, the game puts the pistol away and the blade back
	if (findSubStr(sCur, "hot", 0) >= 0 && findSubStr(sPick, "hot", 0) < 0) { NetLand_SetFState(ch, 0, 0); }
	ch.NetLandAnimAt = iNowA;
	ch.NetLandAnimCur = sPick;
	CharacterPlayAction(ch, sPick);
	if (NetLand_OneShot(sPick))
	{
		int iKnownD = NetLand_AnimDur(sPick);
		if (iKnownD > 400)
		{
			// when the game tells the end itself the timer is only a safety (a learned length cut the round blow at its start)
			int iClearAt = iKnownD - 80;
			if (bNetLandEndActOk) { iClearAt = iKnownD + 400; }
			iNetLandActCounter++;
			ch.NetLandClearId = iNetLandActCounter;
			PostEvent("NetLand_AnimClear", iClearAt, "lll", iNetLandGen, iChr, iNetLandActCounter);
		}
	}
	// a shot: the pistol stays in the hand of the puppet when the shot is over: the weapons are put on again by a timer
	if (findSubStr(sPick, "hot", 0) >= 0)
	{
		iNetLandActCounter++;
		ch.NetLandRearmId = iNetLandActCounter;
		PostEvent("NetLand_Rearm", 1300, "llll", iNetLandGen, iChr, iNetLandActCounter, 0);
	}
}

void NetLand_Rearm()
{
	int iGen = GetEventData();
	int iChr = GetEventData();
	int iId = GetEventData();
	int iTry = GetEventData();
	if (iGen != iNetLandGen || !bNetLandActive || iChr < 0) { return; }
	ref ch = &Characters[iChr];
	if (!IsEntity(ch)) { return; }
	if (!CheckAttribute(ch, "NetLandRearmId")) { return; }
	if (sti(ch.NetLandRearmId) != iId) { return; }		// a newer shot has its own timer
	if (CheckAttribute(ch, "NetLandDead")) { if (sti(ch.NetLandDead)) { return; } }
	if (ch.model.animation == "mushketer") { return; }
	// the puppet still shoots (a long aim) or already swings / reacts: the reset waits, it would break that animation
	string sCurR = "";
	if (CheckAttribute(ch, "NetLandAnimCur")) { sCurR = ch.NetLandAnimCur; }
	bool bBusyR = false;
	if (findSubStr(sCurR, "hot", 0) >= 0) { bBusyR = true; }
	if (NetLand_OneShot(sCurR)) { bBusyR = true; }
	if (bBusyR && iTry == 100) { return; }		// the second reset is not needed when the puppet is busy again
	if (bBusyR && iTry < 8)
	{
		PostEvent("NetLand_Rearm", 300, "llll", iNetLandGen, iChr, iId, iTry + 1);
		return;
	}
	// sometimes the game does not take the first reset (the pistol stayed in the hand): once more a bit later
	int iPkR = NetLand_GetFState(ch);
	Trace("COOP: land puppet " + ch.id + " after the shot: reset (try " + iTry + "), its state was " + (iPkR - (iPkR / 256) * 256) + ", animation " + sCurR);
	NetLand_SetFState(ch, 0, 0);
	if (iTry < 100) { PostEvent("NetLand_Rearm", 800, "llll", iNetLandGen, iChr, iId, 100); }
}

// animations that play once (the puppet plays an action until the next one comes, so it starts the cycle again meanwhile)
bool NetLand_OneShot(string sA)
{
	if (findSubStr(sA, "attack_", 0) == 0) { return true; }
	if (findSubStr(sA, "hit_", 0) == 0) { return true; }
	if (findSubStr(sA, "block_hit", 0) == 0) { return true; }
	if (findSubStr(sA, "parry_", 0) == 0) { return true; }
	if (findSubStr(sA, "straif_", 0) == 0) { return true; }
	if (findSubStr(sA, "Recoil", 0) == 0) { return true; }
	if (findSubStr(sA, "recoil", 0) == 0) { return true; }
	if (findSubStr(sA, "Shot", 0) == 0) { return true; }
	return false;
}

int NetLand_AnimDur(string sA)
{
	if (CheckAttribute(&oNetLandAnimDur, sA)) { return sti(oNetLandAnimDur.(sA)); }
	return 0;
}

// how long a one-shot animation was on the puppet before the next name came: rises fast, falls slowly (a swing that the
// player cut with a block is shorter than a whole one)
void NetLand_LearnAnimDur(string sA, int iMs)
{
	if (iMs < 150 || iMs > 2500) { return; }
	int iOld = NetLand_AnimDur(sA);
	int iNew = iMs;
	if (iOld > 0)
	{
		if (iMs > iOld) { iNew = iOld + (iMs - iOld) / 3; }
		else { iNew = iOld - (iOld - iMs) / 12; }
	}
	oNetLandAnimDur.(sA) = iNew;
}

// the one-shot animation has played to its end on the puppet: back to the stand until the next name comes
void NetLand_AnimClear()
{
	int iGen = GetEventData();
	int iChr = GetEventData();
	int iId = GetEventData();
	if (iGen != iNetLandGen || !bNetLandActive || iChr < 0) { return; }
	ref ch = &Characters[iChr];
	if (!IsEntity(ch)) { return; }
	if (!CheckAttribute(ch, "NetLandClearId")) { return; }
	if (sti(ch.NetLandClearId) != iId) { return; }		// a newer animation has its own timer
	if (CheckAttribute(ch, "NetLandDead")) { if (sti(ch.NetLandDead)) { return; } }
	CharacterPlayAction(ch, "");
}

// a reaction of a puppet that our machine knows before its player's animation comes (our blow in its block)
void NetLand_PuppetPredict(ref ch, string sName, int iHold)
{
	int iChr = sti(ch.index);
	iNetLandActCounter++;
	ch.NetLandPredId = iNetLandActCounter;
	ch.NetLandPredUntil = Net_GetTickCount() + iHold;
	string sCur = "";
	if (CheckAttribute(ch, "NetLandAnimCur")) { sCur = ch.NetLandAnimCur; }
	if (sCur != sName) { NetLand_PuppetPlayName(ch, iChr, sName); }
	PostEvent("NetLand_PredEnd", iHold, "lll", iNetLandGen, iChr, iNetLandActCounter);
}

void NetLand_PredEnd()
{
	int iGen = GetEventData();
	int iChr = GetEventData();
	int iId = GetEventData();
	if (iGen != iNetLandGen || !bNetLandActive || iChr < 0) { return; }
	ref ch = &Characters[iChr];
	if (!IsEntity(ch)) { return; }
	if (!CheckAttribute(ch, "NetLandPredId")) { return; }
	if (sti(ch.NetLandPredId) != iId) { return; }
	DeleteAttribute(ch, "NetLandPredUntil");
	if (CheckAttribute(ch, "NetLandDead")) { if (sti(ch.NetLandDead)) { return; } }
	if (!CheckAttribute(ch, "NetLandAnimLast")) { return; }
	string sLast = ch.NetLandAnimLast;
	string sCur = "";
	if (CheckAttribute(ch, "NetLandAnimCur")) { sCur = ch.NetLandAnimCur; }
	if (sLast != sCur) { NetLand_PuppetPlayName(ch, iChr, sLast); }
}

// the puppet whose hero sends fight states and we can ask for them
bool NetLand_NativePuppet(ref ch)
{
	if (!bNetLandFStateOk) { return false; }
	if (!CheckAttribute(ch, "NetLandFSeen")) { return false; }
	if (CheckAttribute(ch, "NetLandFNativeOff")) { return false; }
	return true;
}

// the game has taken a request of a fight state of this puppet (read back as asked): its states are the game's
bool NetLand_NativeVerified(ref ch)
{
	if (!NetLand_NativePuppet(ch)) { return false; }
	if (!CheckAttribute(ch, "NetLandFOk")) { return false; }
	if (sti(ch.NetLandFOk) > 0) { return true; }
	return false;
}

// only the blows (1..7) are taken from the state machine of the game: its own hit test then works on the blade of the puppet.
// A block, a parry, a hop, a hit reaction are NOT held by the game on a puppet ("did not take the fight state 13, it is in 0"):
// they are played by their names always. Before, one taken blow made every state "native" and the names of the block /
// parry / hop were thrown away, and a refused block counted against the blows (the puppet never got the native blows at all)
bool NetLand_FStateVerifiable(int iSt)
{
	// no state is taken from the game any more: a blow that the game plays on a puppet is played to its end, the block of
	// the player that breaks his swing (block right after a blow) did not stop it, and the puppet of the other screen kept
	// swinging. Every animation is played by its name again: it follows the hero exactly, a block cuts a swing at once
	return false;
}

// 120 ms after a request: did the state machine of the puppet take it?
void NetLand_FStateCheck()
{
	int iGen = GetEventData();
	int iChr = GetEventData();
	int iReq = GetEventData();
	int iSeq = GetEventData();
	if (iGen != iNetLandGen || !bNetLandActive) { return; }
	if (iChr < 0) { return; }
	ref ch = &Characters[iChr];
	if (!IsEntity(ch)) { return; }
	if (!CheckAttribute(ch, "NetLandFReq")) { return; }
	if (sti(ch.NetLandFReq) != iSeq) { return; }		// a newer request
	int iPk = NetLand_GetFState(ch);
	int iCur = iPk - (iPk / 256) * 256;
	int iOk = 0;
	int iBad = 0;
	if (CheckAttribute(ch, "NetLandFOk")) { iOk = sti(ch.NetLandFOk); }
	if (CheckAttribute(ch, "NetLandFBad")) { iBad = sti(ch.NetLandFBad); }
	if (iCur == iReq)
	{
		iOk++;
		ch.NetLandFOk = iOk;
		if (iOk == 1) { Trace("COOP: land puppet " + ch.id + " takes the fight states of the game (state " + iReq + ")"); }
		return;
	}
	iBad++;
	ch.NetLandFBad = iBad;
	if (iBad <= 3) { Trace("COOP: land puppet " + ch.id + " did not take the fight state " + iReq + ", it is in " + iCur); }
	if (iBad >= 4 && iOk == 0)
	{
		ch.NetLandFNativeOff = 1;
		Trace("COOP: land puppet " + ch.id + ": the fight states are not taken, the animations are played by their names");
	}
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
	if (sCtl == "ChrBlock") { bNetLandBlockHeld = true; iNetLandBlockRelAt = 0; }
	if (sCtl == "ChrRun") { bNetLandRunHeld = true; }
	// COOP s30: the chat on land is Y / T like at sea (controls\init_pc.c maps them to the land groups too,
	// client\OnChat.c NetClient_ChatControlActivation opens the line); G is not the chat any more
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
			bool bParryFree = true;
			int iPkP = NetLand_GetFState(mcP);
			if (iPkP != 0)
			{
				int iStP = iPkP - (iPkP / 256) * 256;
				if (iStP != 0 && iStP != 17 && iStP != 18 && iStP != 19) { bParryFree = false; }
			}
			if (NetLand_StunLevel(mcP, Net_GetTickCount()) > 0) { bParryFree = false; }
			if (bParryFree && SendMessage(mcP, "ls", MSG_CHARACTER_EX_MSG, "IsFightMode") != 0)
			{
				iNetLandParryUntil = Net_GetTickCount() + NET_LAND_T_PARRY;
				int iPV = 1 + rand(3);
				NetLand_OnLocalAction(mcP, NET_LANDACT_PARRY + iPV * 16);
			}
		}
	}
}

void NetLand_CtrlOff()
{
	string sCtl = GetEventData();
	if (sCtl == "ChrBlock") { bNetLandBlockHeld = false; iNetLandBlockRelAt = Net_GetTickCount(); }
	if (sCtl == "ChrRun") { bNetLandRunHeld = false; }
	if (sCtl == "ChrForward") { bNetLandFwdHeld = false; }
	if (sCtl == "ChrBackward") { bNetLandBackHeld = false; }
	if (sCtl == "ChrStrafeLeft") { bNetLandLeftHeld = false; }
	if (sCtl == "ChrStrafeRight") { bNetLandRightHeld = false; }
}

// what does our hero do against a blow now: 2 a parry (dodge, Shift + RMB), 1 a block, 0 nothing. The fight state of the game
// is the truth (13 block, 14 a blow in the block, 16 parry): a key that is held in the middle of a swing or a reaction blocks nothing
int NetLand_HeroDefence()
{
	if (bNetLandDead) { return 0; }
	ref mc = GetMainCharacter();
	if (!IsEntity(mc)) { return 0; }
	int iPk = NetLand_GetFState(mc);
	if (iPk != 0)
	{
		int iSt = iPk - (iPk / 256) * 256;
		if (iSt == 16) { return 2; }
		if (iSt == 5) { return 3; }
		if (iSt == 13 || iSt == 14) { return 1; }
		return 0;
	}
	// the dll gives no state: the keys
	int iParryLeft = iNetLandParryUntil - Net_GetTickCount();
	if (iNetLandParryUntil != 0 && iParryLeft > 0) { return 2; }
	if (!bNetLandBlockHeld) { return 0; }
	if (SendMessage(mc, "ls", MSG_CHARACTER_EX_MSG, "IsFightMode") == 0) { return 0; }
	float fEn = Lai_CharacterGetEnergy(mc);
	if (fEn < 1.0) { return 0; }
	return 1;
}

bool NetLand_HeroBlocking()
{
	if (NetLand_HeroDefence() > 0) { return true; }
	return false;
}

// how much of the fight controls must wait now: 2 everything (a hit reaction, thrown back by a dodge, the block broken, our own
// dodge), 1 the swings (a blow in the block: the block stays), 0 nothing
int NetLand_StunLevel(ref mc, int iNow)
{
	if (bNetLandDead || !bNetLandActive) { return 0; }
	int iLvl = 0;
	int iPk = NetLand_GetFState(mc);
	if (iPk != 0)
	{
		int iSt = iPk - (iPk / 256) * 256;
		if (iSt >= 8 && iSt <= 12) { iLvl = 2; }
		if (iSt == 15 || iSt == 16) { iLvl = 2; }
		if (iSt == 14) { iLvl = 1; }
	}
	else
	{
		int iParryLeft = iNetLandParryUntil - iNow;
		if (iNetLandParryUntil != 0 && iParryLeft > 0) { iLvl = 2; }
	}
	int iLeft = iNetLandStunUntil - iNow;
	if (iNetLandStunUntil != 0 && iLeft > 0)
	{
		if (iNetLandStunLvl > iLvl) { iLvl = iNetLandStunLvl; }
	}
	return iLvl;
}

// Loc_ai\LAi_fightparams.c (LAi_ChrAttackAction): true = our hero cannot begin a swing now
bool NetLand_StunGate(aref attack)
{
	if (!bNetLandActive) { return false; }
	if (sti(attack.index) != GetMainCharacterIndex()) { return false; }
	ref mc = GetMainCharacter();
	if (NetLand_StunLevel(mc, Net_GetTickCount()) > 0) { return true; }
	return false;
}

// the fight controls of our hero are locked by the engine itself (the same LockControl that freezes the controls in a dialog)
void NetLand_LockFight(int iLvl)
{
	if (iLvl == iNetLandLockLvl) { return; }
	bool bAtt = false;
	bool bBlk = false;
	if (iLvl >= 1) { bAtt = true; }
	if (iLvl >= 2) { bBlk = true; }
	LockControl("ChrAttackBase", bAtt);
	LockControl("ChrAltAttackBase", bAtt);
	LockControl("ChrAttackBreakBase", bAtt);
	LockControl("ChrAttackChoseBase", bAtt);
	LockControl("ChrAttackFast2", bAtt);
	LockControl("ChrAttackForce2", bAtt);
	LockControl("ChrAttackBreak", bAtt);
	LockControl("ChrAttackRound", bAtt);
	LockControl("ChrAttackFient", bAtt);
	LockControl("ChrParry", bAtt);
	LockControl("ChrFire", bAtt);
	LockControl("ChrBlock", bBlk);
	// the release of SPACE does not come while it is locked: the block is counted by the state of the game anyway
	if (bBlk) { bNetLandBlockHeld = false; }
	if (iNetLandReactLog < 40)
	{
		iNetLandReactLog++;
		Trace("COOP: land hero fight controls locked " + iLvl + " (was " + iNetLandLockLvl + ")");
	}
	iNetLandLockLvl = iLvl;
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
		// a reaction or a dodge of our hero cannot be broken by a block or a swing (single player): the controls wait
		NetLand_LockFight(NetLand_StunLevel(mc, iNow));
		int iFlags = 0;
		if (SendMessage(mc, "ls", MSG_CHARACTER_EX_MSG, "IsFightMode") != 0) { iFlags = iFlags + 1; }
		if (bNetLandDead) { iFlags = iFlags + 2; }
		if (NetLand_HeroBlocking()) { iFlags = iFlags + 4; }
		float fHPNow = LAi_GetCharacterHP(mc);
		float fHPMaxNow = LAi_GetCharacterMaxHP(mc);
		// a bot (the game's own damage) has taken the last health: the hero is immortal, the death is ours
		if (!bNetLandDead && fHPNow < 1.0 && iNetLandProtectUntil - iNow <= 0) { NetLand_LocalDie(DST_INVALID); }
		NetLand_PoisonTick(mc, iNow);
		NetLand_KeepSpeed(iNow);
		int iHP = makeint(fHPNow);
		int iHPMax = makeint(fHPMaxNow);
		int iEnNow = makeint(Lai_CharacterGetEnergy(mc) + 0.5);
		int iEnMaxNow = makeint(LAi_GetCharacterMaxEnergy(mc) + 0.5);

		// our own speed: the others extrapolate with it (the puppet runs ahead of the packet, not behind it)
		// it is measured over 40 ms at least: a tick of 10 ms often has no new frame of the game (the speed was 0, then double,
		// and the puppet turned between the way and the camera every few ticks)
		float fVX = fNetLandVX;
		float fVZ = fNetLandVZ;
		int iDt = iNow - iNetLandPT;
		bool bVReset = false;
		if (iNetLandPT == 0) { bVReset = true; }
		if (iDt < 0 || iDt >= 500) { bVReset = true; }
		if (bVReset)
		{
			fVX = 0.0;
			fVZ = 0.0;
			fNetLandPX = fX;
			fNetLandPZ = fZ;
			iNetLandPT = iNow;
		}
		if (!bVReset && iDt >= 40)
		{
			fVX = (fX - fNetLandPX) * 1000.0 / makefloat(iDt);
			fVZ = (fZ - fNetLandPZ) * 1000.0 / makefloat(iDt);
			fNetLandPX = fX;
			fNetLandPZ = fZ;
			iNetLandPT = iNow;
		}
		fNetLandVX = fVX;
		fNetLandVZ = fVZ;
		// W / S together with A / D: the game turns the model of our hero along the way, his "ay" stays where the camera looks.
		// The others get the angle of the model: the turn is measured on a free run (it is learned for each of the four keys) and
		// kept also against a wall (a slide along it is not a turn). W alone into a wall turns nothing
		float fSendAy = fAy;
		if (NET_LAND_FACE_RUN == 1)
		{
			bool bSideK = false;
			bool bFBK = false;
			if (bNetLandLeftHeld || bNetLandRightHeld) { bSideK = true; }
			if (bNetLandFwdHeld || bNetLandBackHeld) { bFBK = true; }
			if (bSideK && bFBK)
			{
				int iCombo = 0;
				if (bNetLandRightHeld) { iCombo = 1; }
				if (bNetLandBackHeld && !bNetLandFwdHeld) { iCombo = iCombo + 2; }
				// the turn is fixed (W+A -45, W+D +45 degrees; every game measured the same). It was learned on the run before: a
				// measure taken at the moment D was pressed on a run with W (the model only began to turn) got the side wrong once
				// and kept it for the whole game - the puppet ran with W+D turned the other way
				fSendAy = fAy + fNetLandFaceOff[iCombo];
			}
		}
		float fSp = sqrt(fVX * fVX + fVZ * fVZ);
		if (fSp > 12.0 || bNetLandDead) { fVX = 0.0; fVZ = 0.0; fSp = 0.0; }	// a teleport (spawn), not a run
		bool bMoving = false;
		if (fSp > 0.3) { bMoving = true; }
		fNetLandSpEMA = fNetLandSpEMA * 0.6 + fSp * 0.4;
		if (fNetLandSpEMA > NET_LAND_RUN_SPEED) { iFlags = iFlags + 8; }
		NetLand_AnimSample(mc, iNow);
		NetLand_MushHandTick(mc, iNow);
		if (bNetLandAnimOk) { iFlags = iFlags + 64; }
	if (bNetLandFStateOk) { iFlags = iFlags + 128; }
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
		if (iFlags == iNetLandLastFlags && iHP == iNetLandLastHP && iEnNow == iNetLandLastEn) { bSame = true; }
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
			iNetLandLastEn = iEnNow;
			bNetLandLastMoving = bMoving;

			// a swing pushes the hero to his enemy (the game's lunge, about 1 m at 5 m/s): with this speed the others put the
			// puppet even farther ahead (the lead of the packets), into their hero, and the game pushed him away at every blow
			float fSendVX = fVX;
			float fSendVZ = fVZ;
			int iPkV = NetLand_GetFState(mc);
			if (iPkV != 0)
			{
				int iStV = iPkV - (iPkV / 256) * 256;
				if (iStV >= 1 && iStV <= 16) { fSendVX = 0.0; fSendVZ = 0.0; }
			}
			iNetLandSentN++;
			if (bMoving) { iNetLandSentMovN++; }
			int iSentSpan = iNow - iNetLandSentAt;
			if (iSentSpan < 0 || iSentSpan >= 3000)
			{
				if (iNetLandSentAt != 0) { Trace("COOP: land our hero: " + iNetLandSentN + " state packets sent in " + iSentSpan + " ms, " + iNetLandSentMovN + " moving, the animation " + NetLand_GetAnim(mc, 0)); }
				iNetLandSentAt = iNow;
				iNetLandSentN = 0;
				iNetLandSentMovN = 0;
			}
			int iSMsg = NetClient_LandNewMsg(NSC_LAND_STATE);
			NMAddFloat(iSMsg, fX);
			NMAddFloat(iSMsg, fY);
			NMAddFloat(iSMsg, fZ);
			NMAddFloat(iSMsg, fSendAy);
			NMAddByte(iSMsg, iFlags);
			NMAddWord(iSMsg, iHP);
			NMAddWord(iSMsg, iHPMax);
			NMAddFloat(iSMsg, fSendVX);
			NMAddFloat(iSMsg, fSendVZ);
			NMAddWord(iSMsg, iEnNow);
			NMAddWord(iSMsg, iEnMaxNow);
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
	// both crashes of the game were a swing in the block (the tables of actions are written again at every swing): the
	// swing of a hero that holds the block goes by the game's own variant, it is announced by ChrFgtActApply
	if (bNetLandBlockHeld) { Trace("COOP: land swing out of the block: the variant is not set"); return; }
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
	// not the same variant twice in a row: the puppets play the animation by its name, the same name again was not played again
	// (spam of LMB: the damage came, the swing was not seen)
	if (iN > 1 && iVar == NetLand_LoadedVar(iCode))
	{
		iVar = iVar + 1;
		if (iVar > iN) { iVar = 1; }
	}
	iNetLandSwingAt = iNow;
	iNetLandSwingHit = 0;
	ref mc = GetMainCharacter();
	// the tables of the actions are NOT written any more: a block asked for and not yet begun (SPACE right before the click)
	// shows the state 0, and the tables written in that moment crashed the game. The game picks the variant itself, the puppets
	// play the name of the animation that the hero plays
	Trace("COOP: land swing " + sType + " variant " + iVar);
	// the swing goes to the others at once, also if it hits nobody; its hit follows later
	NetLand_OnLocalAction(attack, iCode + iVar * 16);
	NetLand_PreBlockArm(iCode);
}

// the blade of our hero reaches its victim this long after the click (learned, NetLand_GetDelay): a puppet whose player blocks
// is put into the game's own state of a blow in the block (14) just before it. To the game it blocks then: the blow is blocked by
// the game itself, as the block of a player or an NPC (a puppet does not take 13: that one is held by the keys or the AI)
void NetLand_PreBlockArm(int iCode)
{
	if (NET_LAND_PRE_BLOCK != 1 || !bNetLandFStateOk) { return; }
	if (bNetLandHoldOk) { return; }		// the puppets hold their blocks by the game
	if (iCode < 0 || iCode >= 12) { return; }
	// the time to the touch of this kind of swing, measured on our own blows; not known yet: no arming (the script resolves it)
	int iDelay = iNetLandHitD[iCode];
	if (iDelay < 150) { return; }
	int iAt = iDelay - NET_LAND_PRE_LEAD;
	ref mc = GetMainCharacter();
	float fMX = 0.0;
	float fMY = 0.0;
	float fMZ = 0.0;
	if (!GetCharacterPos(mc, &fMX, &fMY, &fMZ)) { return; }
	for (int i=0; i<NET_MAXPLAYERS; i++)
	{
		if (i == wClientID) { continue; }
		int iChr = NetLand_PupIdx(i);
		if (iChr < 0) { continue; }
		if (!IsEntity(&Characters[iChr])) { continue; }
		float fPX = 0.0;
		float fPY = 0.0;
		float fPZ = 0.0;
		if (!GetCharacterPos(&Characters[iChr], &fPX, &fPY, &fPZ)) { continue; }
		float fDD = sqrt((fPX - fMX) * (fPX - fMX) + (fPZ - fMZ) * (fPZ - fMZ));
		if (fDD > 4.5) { continue; }
		PostEvent("NetLand_PreBlock", iAt, "lll", iNetLandGen, iChr, iCode);
	}
}

void NetLand_PreBlock()
{
	int iGen = GetEventData();
	int iChr = GetEventData();
	int iCode = GetEventData();
	if (iGen != iNetLandGen || !bNetLandActive || bNetLandDead || iChr < 0) { return; }
	ref ch = &Characters[iChr];
	if (!IsEntity(ch)) { return; }
	if (CheckAttribute(ch, "NetLandDead")) { if (sti(ch.NetLandDead)) { return; } }
	if (CheckAttribute(ch, "NetLandArmId")) { return; }		// armed already by the swing before
	if (!CheckAttribute(ch, "NetLandHSt") || !CheckAttribute(ch, "NetLandHStAt")) { return; }
	int iAge = Net_GetTickCount() - sti(ch.NetLandHStAt);
	if (iAge < 0 || iAge > 600) { return; }
	int iHSt = sti(ch.NetLandHSt);
	if (iHSt != 13 && iHSt != 14) { return; }
	// our hero still swings
	ref mc = GetMainCharacter();
	int iMPk = NetLand_GetFState(mc);
	int iMSt = iMPk - (iMPk / 256) * 256;
	if (iMSt < 1 || iMSt > 6) { return; }
	int iPk = NetLand_GetFState(ch);
	int iCur = iPk - (iPk / 256) * 256;
	if (iCur >= 8 && iCur <= 12) { return; }
	if (iCur == 14) { return; }
	NetLand_SetFState(ch, 14, 0);
	// the animation of the block hit waits at its first frame for the touch of the blade (NetLand_BladeHit lets it go):
	// else the puppet showed one block hit now and one more at the touch
	NetLand_AnimSpeed(ch, 0, 0);
	NetLand_AnimSpeed(ch, 1, 0);
	NetLand_AnimSpeed(ch, 2, 0);
	iNetLandActCounter++;
	ch.NetLandArmId = iNetLandActCounter;
	PostEvent("NetLand_ArmThaw", NET_LAND_PRE_LEAD + 300, "lll", iNetLandGen, iChr, iNetLandActCounter);
	if (iNetLandPupLog < 30)
	{
		iNetLandPupLog++;
		Trace("COOP: land puppet " + ch.id + " armed for the blow in the block (state was " + iCur + ")");
	}
}

// the blade of our hero has touched an armed puppet: its block hit plays now
void NetLand_ArmRelease(ref ch, bool bTouched)
{
	if (!CheckAttribute(ch, "NetLandArmId")) { return; }
	DeleteAttribute(ch, "NetLandArmId");
	NetLand_AnimSpeed(ch, 0, 100);
	NetLand_AnimSpeed(ch, 1, 100);
	NetLand_AnimSpeed(ch, 2, 100);
	if (bTouched) { return; }
	// the blow did not come (a miss, a broken swing): no block hit at all, back to what its player plays
	int iPk = NetLand_GetFState(ch);
	int iCur = iPk - (iPk / 256) * 256;
	if (iCur == 14) { NetLand_SetFState(ch, 0, 0); }
	string sLastA = "";
	if (CheckAttribute(ch, "NetLandAnimLast")) { sLastA = ch.NetLandAnimLast; }
	ch.NetLandAnimCur = "";
	if (sLastA != "") { NetLand_PuppetPlayName(ch, sti(ch.index), sLastA); }
}

void NetLand_ArmThaw()
{
	int iGen = GetEventData();
	int iChr = GetEventData();
	int iId = GetEventData();
	if (iGen != iNetLandGen || !bNetLandActive || iChr < 0) { return; }
	ref ch = &Characters[iChr];
	if (!IsEntity(ch)) { return; }
	if (!CheckAttribute(ch, "NetLandArmId")) { return; }
	if (sti(ch.NetLandArmId) != iId) { return; }
	NetLand_ArmRelease(ch, false);
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
// the blow of a mirrored puppet hit our hero (the game decided if he blocks): the damage of the swing of that player
// one blow is one hit: whichever of the two ways (the game's own or the packet) comes first counts
bool NetLand_HitRecent(int wAtt)
{
	if (!CheckAttribute(&NCClients[wAtt], "LandHitAt")) { return false; }
	int iAgoH = Net_GetTickCount() - sti(NCClients[wAtt].LandHitAt);
	if (iAgoH >= 0 && iAgoH < 700) { return true; }
	return false;
}

void NetLand_NativeHit(aref attack, string attackType, bool isBlocked)
{
	if (!bNetLandActive || bNetLandDead) { return; }
	int wAttN = NetLand_PuppetID(attack);
	if (wAttN != DST_INVALID) { if (NetLand_IsAlly(wAttN)) { return; } }		// COOP: team duel
	if (wAttN != DST_INVALID)
	{
		if (NetLand_HitRecent(wAttN)) { return; }
		NCClients[wAttN].LandHitAt = Net_GetTickCount();
	}
	int iProtL = iNetLandProtectUntil - Net_GetTickCount();
	if (iNetLandProtectUntil != 0 && iProtL > 0) { return; }
	ref mc = GetMainCharacter();
	if (!IsEntity(mc)) { return; }
	int wAtt = NetLand_PuppetID(attack);
	if (wAtt == DST_INVALID && CheckAttribute(attack, "NetLandBotID")) { wAtt = sti(attack.NetLandBotID); }
	float fDmg = NetLand_BladeDamage(attack, mc, attackType, isBlocked);
	int iD = makeint(fDmg + 0.5);
	if (iD <= 0) { return; }
	float fHPBeforeN = LAi_GetCharacterHP(mc);
	if (isBlocked)
	{
		float fHPOldN = LAi_GetCharacterHP(mc);
		float fHPMaxN = LAi_GetCharacterMaxHP(mc);
		float fHPN = fHPOldN - makefloat(iD);
		if (fHPN < 0.0) { fHPN = 0.0; }
		LAi_SetHP(mc, fHPN, fHPMaxN);
	}
	else { LAi_ApplyCharacterDamage(mc, iD); }
	if (wAtt != DST_INVALID) { NetLand_ReportDmg(wClientID, wAtt, makeint(fHPBeforeN - LAi_GetCharacterHP(mc) + 0.5), mc); }
	if (LAi_GetCharacterHP(mc) < 1.0) { NetLand_LocalDie(wAtt); }
}

// iView: what the attacker saw on his screen at the touch of the blade: 1 the puppet blocked, 2 it dodged (parry), + 4 a break
// blow (goes through a block). It goes in the word that was the delay of the hit (the hit is applied at once now)
void NetLand_SendHit(int wVictim, float fDamage, float fDamageB, int iKind, int iView)
{
	int iDmg = makeint(fDamage + 0.5);
	int iDmgB = makeint(fDamageB + 0.5);
	if (iDmg <= 0 && iDmgB <= 0) { return; }
	if (iDmgB > iDmg) { iDmgB = iDmg; }
	int iSMsg = NetClient_LandNewMsg(NSC_LAND_HIT);
	NMAddClientID(iSMsg, wVictim);
	NMAddWord(iSMsg, iDmg);
	NMAddWord(iSMsg, iDmgB);
	NMAddWord(iSMsg, iView);
	NMAddByte(iSMsg, iKind);
	iNetLandHitSeq = iNetLandHitSeq + 1;
	if (iNetLandHitSeq > 255) { iNetLandHitSeq = 0; }
	NMAddByte(iSMsg, iNetLandHitSeq);
	NetClient_LandSend(iSMsg, false);
	NetClient_LandSend(iSMsg, false);
	NMDelete(iSMsg);
}

// Loc_ai\LAi_fightparams.c: LAi_ApplyCharacterAttackDamage. true = the hit is ours to handle (no single player damage)
bool NetLand_BladeHit(aref attack, aref enemy, string attackType, bool isBlocked)
{
	if (!bNetLandActive) { return false; }
	int iBotH = NetLand_BotBladeHit(attack, enemy, attackType, isBlocked);
	if (iBotH == 1) { return true; }
	if (iBotH == 0) { return false; }
	// a puppet hits: its owner has decided it on his machine already. But the game has just worked out
	// if OUR hero blocks this swing (his real block, angle and timing): the damage will use it
	if (NetLand_PuppetID(attack) != DST_INVALID)
	{
		if (NetLand_IsOurHero(enemy))
		{
			ref rAttP = &Characters[sti(attack.index)];
			if (NetLand_NativeVerified(rAttP) && bNetLandFStateOk)
			{
				// the blow of the mirrored puppet has met our hero: the game has found his real block / parry, the hit
				// animation of the hero and the sounds are the game's, the damage is ours
				NetLand_NativeHit(attack, attackType, isBlocked);
				return true;
			}
			iNetLandBlockedAt = Net_GetTickCount();
			bNetLandBlockedLast = isBlocked;
		}
		return true;
	}

	int wVictim = NetLand_PuppetID(enemy);
	if (wVictim == DST_INVALID) { return false; }
	if (!NetLand_IsOurHero(attack) || bNetLandDead) { return true; }
	if (NetLand_IsAlly(wVictim)) { return true; }		// COOP: team duel: allies do not hurt each other
	if (CheckAttribute(enemy, "NetLandDead") && sti(enemy.NetLandDead)) { return true; }
	ref mcS = GetMainCharacter();
	if (NetLand_StunLevel(mcS, Net_GetTickCount()) >= 2)
	{
		Trace("COOP: land blade hit on " + wVictim + " is not sent: our hero has been hit, his swing is broken");
		return true;
	}

	// the touch: an armed puppet plays its block hit now
	ref rPupA = &Characters[sti(enemy.index)];
	NetLand_ArmRelease(rPupA, true);
	// the time from the click to the first touch of this kind of swing (the arming of the next blocks waits for it)
	int iSwAgo = Net_GetTickCount() - iNetLandSwingAt;
	int iSwCode = NetLand_ActCode(attackType);
	if (iNetLandSwingAt != 0 && iNetLandSwingHit == 0 && iSwAgo > 80 && iSwAgo < 1500 && iSwCode > 0 && iSwCode < 12)
	{
		iNetLandSwingHit = 1;
		int iOldD = iNetLandHitD[iSwCode];
		if (iOldD <= 0) { iNetLandHitD[iSwCode] = iSwAgo; }
		else { iNetLandHitD[iSwCode] = (iOldD + iSwAgo) / 2; }
		Trace("COOP: land " + attackType + " touches " + iSwAgo + " ms after the click (kept " + iNetLandHitD[iSwCode] + ")");
	}
	// the puppet never blocks on our machine: both damages go, the victim picks. The damage of the game takes energy of the
	// victim (HardHitter): not from the puppet (twice), the victim takes it himself
	float fPupEn = Lai_CharacterGetEnergy(enemy);
	float fClean = NetLand_BladeDamage(attack, enemy, attackType, false);
	float fBlock = NetLand_BladeDamage(attack, enemy, attackType, true);
	if (CheckAttribute(enemy, "chr_ai.energy")) { enemy.chr_ai.energy = fPupEn; }
	// single player: the blow into a dodge throws the attacker back AT THE TOUCH, the blow into a block shows the block hit at
	// once. The state of the other player (block / parry) is known here by his own game (it comes with his animation): the
	// result is shown now, the victim gets what we saw and takes it into account (a block or a dodge seen by either side counts)
	ref rPupH = &Characters[sti(enemy.index)];
	int iDef = NetLand_PuppetDefence(rPupH, attackType);
	if (CheckAttribute(rPupH, "NetLandHold"))
	{
		if (sti(rPupH.NetLandHold) == 1)
		{
			// the game has blocked it (the block hit) or broken the block itself (a break blow): nothing to show by the script
			iDef = 0;
			if (isBlocked) { iDef = 1; }
		}
	}
	// what goes to the victim: 1 block, 2 dodge, 3 feint, + 4 a break blow
	int iView = 0;
	if (iDef == 1) { iView = 1; }
	if (iDef == 2) { iView = 2; }
	if (iDef == 4) { iView = 3; }
	if (attackType == "break") { iView = iView + 4; }
	if (CheckCharacterPerk(attack, "HardHitter")) { iView = iView + 8; }
	// the perk Sliding (single player, LAi_CharacterAttack): a blow into the block goes through it with a chance of 20%. Only
	// the damage: the block and its block hit stay. The attacker rolls it (the block itself is decided by the victim)
	if (CheckCharacterPerk(attack, "Sliding"))
	{
		if (rand(99) < 20) { iView = iView + 64; }
	}
	// the poison on the blade (the loadout): the victim is poisoned if the blow hits
	if (NetLand_HeroPoisonBlade()) { if (rand(99) < NET_LAND_POISON_CHANCE) { iView = iView + 128; } }
	// a clean hit: the game puts the puppet into its hit reaction with a variant of its own choice, the victim chose another one:
	// the reactions differed. The variant is chosen here (not the one that plays now), the puppet gets it, the victim plays it
	if (iDef == 0)
	{
		int iHitV = 1 + rand(2);
		string sPupHA = NetLand_GetAnim(rPupH, 0);
		if (sPupHA == "hit_attack_" + iHitV)
		{
			iHitV = iHitV + 1;
			if (iHitV > 3) { iHitV = 1; }
		}
		iView = iView + iHitV * 16;
		// the game has just put the puppet into its own reaction of this kind of blow: the counter blow of a feint gives hit_feint
		// (state 9), the round blow hit_round (11). They cannot be left before their end: the hit_attack of its player (state 8)
		// waited and played half a second later ("the puppet gets the hit, then once more"). Its player plays hit_attack: the
		// request is changed now, before the game begins it
		bool bPupNat = true;
		if (CheckAttribute(rPupH, "NetLandHold")) { if (sti(rPupH.NetLandHold) == 1) { bPupNat = false; } }
		if (SendMessage(rPupH, "ls", MSG_CHARACTER_EX_MSG, "IsFightMode") == 0) { bPupNat = false; }
		if (bPupNat && bNetLandFStateOk)
		{
			int iPkN = NetLand_GetFState(rPupH);
			int iCurN = iPkN - (iPkN / 256) * 256;
			if (iCurN < 13 || iCurN > 16) { NetLand_SetFState(rPupH, 8, iHitV - 1); }
		}
		PostEvent("NetLand_PupHitVar", 1, "lll", iNetLandGen, sti(enemy.index), iHitV);
	}
	// a puppet in the block pose (frozen state 14): the game has blocked it itself, the blow plays at its speed
	if (CheckAttribute(rPupH, "NetLandBlkPose"))
	{
		if (sti(rPupH.NetLandBlkPose) == 1)
		{
			NetLand_AnimSpeed(rPupH, 0, 100);
			NetLand_AnimSpeed(rPupH, 1, 100);
			NetLand_AnimSpeed(rPupH, 2, 100);
			rPupH.NetLandBlkFrozen = 0;
			rPupH.NetLandBlkHitUntil = Net_GetTickCount() + 650;
		}
	}
	NCClients[wVictim].LandPred = iDef;
	NCClients[wVictim].LandPredAt = Net_GetTickCount();
	Trace("COOP: land blade hit on " + wVictim + " dmg " + fClean + "/" + fBlock + " game says blocked " + isBlocked + " we see " + iDef);
	// not inside the attack of the game (it is going through its enemies now): the next frame
	int iGameBlk = 0;
	if (isBlocked) { iGameBlk = 1; }
	if (iDef > 0) { PostEvent("NetLand_AttRes", 1, "llll", iNetLandGen, wVictim, iDef, iGameBlk); }
	int iSeqBefore = iNetLandHitSeq;
	NetLand_SendHit(wVictim, fClean, fBlock, 0, iView);
	if (iDef == 0 && iNetLandHitSeq != iSeqBefore)
	{
		rPupH.NetLandPHitSeq = iNetLandHitSeq;
		rPupH.NetLandPHitAt = Net_GetTickCount();
		rPupH.NetLandPHitAck = 0;
		// the reaction that its player plays for this blow
		rPupH.NetLandPHitName = "hit_attack_" + iHitV;
		if (SendMessage(rPupH, "ls", MSG_CHARACTER_EX_MSG, "IsFightMode") == 0) { rPupH.NetLandPHitName = "HitNoFight"; }
	}
	return true;
}

// Loc_ai\LAi_fightparams.c: LAi_ApplyCharacterFireDamage
bool NetLand_FireHit(aref attack, aref enemy, float kDist)
{
	if (!bNetLandActive) { return false; }
	int iBotF = NetLand_BotFireHit(attack, enemy, kDist);
	if (iBotF == 1) { return true; }
	if (iBotF == 0) { return false; }
	if (NetLand_PuppetID(attack) != DST_INVALID) { return true; }

	int wVictim = NetLand_PuppetID(enemy);
	if (wVictim == DST_INVALID) { return false; }
	if (!NetLand_IsOurHero(attack) || bNetLandDead) { return true; }
	if (NetLand_IsAlly(wVictim)) { return true; }		// COOP: team duel: allies do not hurt each other
	if (CheckAttribute(enemy, "NetLandDead") && sti(enemy.NetLandDead)) { return true; }

	float fP = LAi_GunCalcProbability(attack, kDist);
	if (rand(10000) > fP * 10000) { return true; }
	if (NetLand_ArmorStopsShot(enemy)) { Trace("COOP: land the shot stops on the cuirass of " + wVictim); return true; }
	float fShot = LAi_GunCalcDamage(attack, enemy);
	NetLand_SendHit(wVictim, fShot, fShot, 1, 0);
	return true;
}

// the server: somebody hit us. The packet is sent at the touch of the blade on the attacker's screen; his swing has come to our
// screen as late as this packet (both go the same way): the blade touches our hero on OUR screen now. It is resolved at once
// (it waited for the whole length of the swing before: the block, the dodge and the hit came half a second late)
void NetLand_OnHit(int iMsg)
{
	int wAttacker = NMGetClientID(iMsg);
	int iDmg = NMGetWord(iMsg);
	int iDmgB = NMGetWord(iMsg);
	int iView = NMGetWord(iMsg);
	int iKind = NMGetByte(iMsg);
	int iSeq = NMGetByte(iMsg);

	if (!bNetLandActive || bNetLandDead || !bNetLandLocReady) { return; }
	// the second copy of the packet
	if (wAttacker < 0 || wAttacker >= NET_MAXCLIENTS) { return; }
	if (wAttacker >= NET_MAXPLAYERS && !NetLand_IsLandBot(wAttacker)) { return; }
	if (wAttacker < NET_MAXPLAYERS) { if (NetLand_IsAlly(wAttacker)) { return; } }		// COOP: team duel
	string sSeqKey = "s" + iSeq;
	if (CheckAttribute(&NCClients[wAttacker], "LandHitSeen." + sSeqKey))
	{
		if (Net_GetTickCount() - sti(NCClients[wAttacker].LandHitSeen.(sSeqKey)) < 3000) { return; }
	}
	NCClients[wAttacker].LandHitSeen.(sSeqKey) = Net_GetTickCount();
	iNetLandAckFrom = wAttacker;
	iNetLandAckSeq = iSeq;
	iNetLandAckAt = Net_GetTickCount();
	bNetLandAckPub = false;
	NetLand_ApplyHit(wAttacker, iDmg, iDmgB, iKind, iView);
}

void NetLand_HitApply()
{
	int iGen = GetEventData();
	int wAtt = GetEventData();
	int iD = GetEventData();
	int iDB = GetEventData();
	int iK = GetEventData();
	if (iGen != iNetLandGen || !bNetLandActive || bNetLandDead) { return; }
	NetLand_ApplyHit(wAtt, iD, iDB, iK, 0);
}

void NetLand_ApplyHit(int wAttacker, int iDmg, int iDmgB, int iKind, int iView)
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
	// the game's own hit of this blow has been counted already
	if (iKind == 0)
	{
		if (NetLand_HitRecent(wAttacker)) { return; }
		NCClients[wAttacker].LandHitAt = Net_GetTickCount();
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

	// what the attacker saw (1 block, 2 dodge, + 4 a break blow) and what our game says now: a block or a dodge seen by either
	// side counts, so neither screen shows a blow that went through a block or a dodge it saw
	int iSeen = iView - (iView / 4) * 4;
	bool bBreak = false;
	int iViewHi = iView / 4;
	if (iViewHi - (iViewHi / 2) * 2 == 1) { bBreak = true; }
	bool bHard = false;
	int iView8 = iView / 8;
	if (iView8 - (iView8 / 2) * 2 == 1) { bHard = true; }
	int iView16 = iView / 16;
	int iHitVIn = iView16 - (iView16 / 4) * 4;		// the variant of the hit reaction chosen by the attacker (0: none)
	bool bPierce = false;							// the perk Sliding of the attacker: a blow into the block does its full damage
	int iView64 = iView / 64;
	if (iView64 - (iView64 / 2) * 2 == 1) { bPierce = true; }
	bool bPoisonHit = false;
	int iView128 = iView / 128;
	if (iView128 - (iView128 / 2) * 2 == 1) { bPoisonHit = true; }
	int iOwn = 0;
	int iRes = NET_LANDRES_HIT;
	if (iKind == 0)
	{
		iOwn = NetLand_HeroDefence();
		if (iOwn == 2 || iSeen == 2) { iRes = NET_LANDRES_PARRY; }
		if (iRes == NET_LANDRES_HIT)
		{
			if (iOwn == 3 || iSeen == 3) { iRes = NET_LANDRES_FEINT; }
		}
		if (iRes == NET_LANDRES_HIT && !bBreak)
		{
			if (iOwn == 1 || iSeen == 1) { iRes = NET_LANDRES_BLOCK; }
		}
	}
	int iPkL = NetLand_GetFState(mc);
	Trace("COOP: land hit from " + wAttacker + " dmg " + iDmg + "/" + iDmgB + " kind " + iKind + " seen " + iView + " own " + iOwn + " result " + iRes + " state " + (iPkL - (iPkL / 256) * 256) + " keyheld " + bNetLandBlockHeld + " hp " + LAi_GetCharacterHP(mc));

	if (iRes == NET_LANDRES_PARRY)
	{
		// the dodge: no damage; the attacker is thrown back on his screen (and so on ours: his animation comes here)
		NetLand_SendHitRes(wAttacker, NET_LANDRES_PARRY);
		return;
	}
	if (iRes == NET_LANDRES_FEINT)
	{
		// the feint: no damage, our hero strikes back (attack_feintc of the same number); the counter blow hits by itself
		NetLand_SendHitRes(wAttacker, NET_LANDRES_FEINT);
		NetLand_HeroCounter();
		return;
	}
	if (iRes == NET_LANDRES_BLOCK)
	{
		NetLand_SendHitRes(wAttacker, NET_LANDRES_BLOCK);
		LAi_CharacterPlaySound(mc, "fgt_collision");
		// the blow in the block: the game's own block hit (state 14), also when the hero stands in the block already
		NetLand_HeroFightReact(14, 0, "block_hit", NET_LAND_T_BLOCKHIT);
		int iDmgBUse = iDmgB;
		if (bPierce && iDmg > iDmgB)
		{
			iDmgBUse = iDmg;
			Trace("COOP: land the blow of " + wAttacker + " goes through the block (Sliding): " + iDmg);
		}
		if (iDmgBUse > 0)
		{
			float fHPOld = LAi_GetCharacterHP(mc);
			float fHPMaxB = LAi_GetCharacterMaxHP(mc);
			float fHPB = fHPOld - makefloat(iDmgBUse);
			if (fHPB < 0.0) { fHPB = 0.0; }
			LAi_SetHP(mc, fHPB, fHPMaxB);
			if (bHard) { NetLand_HardHitter(mc); }
			NetLand_ReportDmg(wClientID, wAttacker, makeint(fHPOld - fHPB + 0.5), mc);
		}
	}
	else
	{
		if (iDmg <= 0) { return; }
		NetLand_SendHitRes(wAttacker, NET_LANDRES_HIT);
		float fHPBefore = LAi_GetCharacterHP(mc);
		LAi_ApplyCharacterDamage(mc, iDmg);		// blood and the hit sound like in single player, the hero is immortal there
		NetLand_ReportDmg(wClientID, wAttacker, makeint(fHPBefore - LAi_GetCharacterHP(mc) + 0.5), mc);
		if (bPoisonHit) { NetLand_PoisonHero(wAttacker); }
		if (bHard) { NetLand_HardHitter(mc); }
		if (LAi_GetCharacterHP(mc) >= 1.0)
		{
			bool bReacted = false;
			if (iKind == 1) { NetLand_HeroFightReact(12, 0, "hit_fire", NET_LAND_T_HIT); bReacted = true; }
			if (!bReacted && bBreak && iOwn == 1) { NetLand_HeroFightReact(15, 0, "block_break", NET_LAND_T_HIT); bReacted = true; }
			if (!bReacted)
			{
				int iHV = rand(2);
				// the reaction that plays now is not taken again: the next hit (the second blow of a round blow) gets another one,
				// and its new name shows it on the puppet of the others as well
				string sHA = NetLand_GetAnim(mc, 0);
				if (sHA == "hit_attack_" + (iHV + 1))
				{
					iHV = iHV + 1;
					if (iHV > 2) { iHV = 0; }
				}
				// the attacker has chosen it (his puppet of us plays it already)
				if (iHitVIn >= 1 && iHitVIn <= 3) { iHV = iHitVIn - 1; }
				NetLand_HeroFightReact(8, iHV, "hit_attack_" + (iHV + 1), NET_LAND_T_HIT);
			}
		}
	}
	if (LAi_GetCharacterHP(mc) < 1.0) { NetLand_LocalDie(wAttacker); }
}

// the perk HardHitter of the attacker (LAi_CalcDamageForBlade): a blow that does damage takes a tenth of the energy
void NetLand_HardHitter(ref mc)
{
	if (!CheckAttribute(mc, "chr_ai.energy")) { return; }
	mc.chr_ai.energy = stf(mc.chr_ai.energy) * 0.9;
}

// the counter blow of a feint (single player: a blow into attack_feint_N is answered by attack_feintc_N)
void NetLand_HeroCounter()
{
	ref mc = GetMainCharacter();
	if (!IsEntity(mc) || bNetLandDead) { return; }
	int iPk = NetLand_GetFState(mc);
	int iCur = iPk - (iPk / 256) * 256;
	if (iCur == 6) { return; }		// the game is in the counter blow already
	string sA = NetLand_GetAnim(mc, 0);
	int iVar = 1;
	int iLenA = strlen(sA);
	if (findSubStr(sA, "attack_feint_", 0) == 0 && iLenA > 13) { iVar = sti(strcut(sA, iLenA - 1, iLenA - 1)); }
	if (iVar < 1 || iVar > 4) { iVar = 1; }
	// the variant goes as the index of the state (the tables of the actions are not written in the middle of a feint)
	NetLand_SetFState(mc, 6, iVar - 1);
	int iPk2 = NetLand_GetFState(mc);
	Trace("COOP: land hero counter blow attack_feintc_" + iVar + ", state " + iCur + " -> " + (iPk2 - (iPk2 / 256) * 256));
}

// a reaction of our hero by the game's own fight state (8 a hit, 10 thrown back by a dodge, 12 a shot, 14 a blow in the block,
// 15 the block broken): the game plays it with its sound, and while the state lasts the fight controls wait (NetLand_StunLevel),
// so it cannot be broken by a block or a swing. If the game does not take the state, the animation is played by its name
void NetLand_HeroFightReact(int iState, int iSub, string sName, int iDur)
{
	ref mc = GetMainCharacter();
	if (!IsEntity(mc) || bNetLandDead) { return; }
	int iNow = Net_GetTickCount();
	// the sword is not out (E not pressed): the game's own hit of a character without a weapon (HitNoFight), as in single player.
	// The fight state asked for before waited for the sword (the fight reaction played when it came out), and the name played
	// instead was the hit of a man with a sword in the hand (also on the puppet of the others)
	if (SendMessage(mc, "ls", MSG_CHARACTER_EX_MSG, "IsFightMode") == 0)
	{
		if (iState == 14) { return; }
		if (bNetLandNoFightOk) { SendMessage(mc, "ls", MSG_CHARACTER_EX_MSG, "NoFightHit"); }
		else { NetLand_HeroReact("HitNoFight", 900); }
		if (iNetLandReactLog < 150) { iNetLandReactLog++; Trace("COOP: land hero hit without the sword: HitNoFight"); }
		return;
	}
	// the game takes a reaction only out of some states: in the middle of an own swing ("did not take the reaction state 8, it
	// is in 1") the swing went on under the reaction played by its name and hit the other player as well; another reaction (the
	// counter blow of a feint in hit_feint) was not begun at all. The game's own break of the action first (the same that a block
	// does to a swing), the reaction then begins at once
	if (bNetLandFStateOk && iState != 14)
	{
		int iPkR = NetLand_GetFState(mc);
		int iCurR = iPkR - (iPkR / 256) * 256;
		bool bBreakR = false;
		if (iCurR >= 1 && iCurR <= 7) { bBreakR = true; }
		if (iCurR >= 9 && iCurR <= 12 && iCurR != iState) { bBreakR = true; }
		if (iCurR >= 15) { bBreakR = true; }
		if (bBreakR)
		{
			if (bNetLandBreakOk) { SendMessage(mc, "ls", MSG_CHARACTER_EX_MSG, "BreakFight"); }
			else
			{
				if (iCurR >= 8) { NetLand_SetFState(mc, 0, 0); }
			}
			if (iNetLandReactLog < 150) { iNetLandReactLog++; Trace("COOP: land hero hit in the state " + iCurR + ": it is broken for the reaction " + iState); }
		}
	}
	// the controls wait from this very frame (the game shows the state a frame later)
	iNetLandStunUntil = iNow + 150;
	iNetLandStunLvl = 2;
	if (iState == 14) { iNetLandStunLvl = 1; }
	NetLand_LockFight(NetLand_StunLevel(mc, iNow));
	if (NET_LAND_NATIVE_REACT == 1 && bNetLandFStateOk)
	{
		NetLand_SetFState(mc, iState, iSub);
		iNetLandReactChk++;
		PostEvent("NetLand_ReactCheck", 70, "llllls", iNetLandGen, iNetLandReactChk, iState, iDur, 0, sName);
		return;
	}
	iNetLandStunUntil = iNow + iDur;
	NetLand_HeroReact(sName, iDur);
}

// 60 ms after a reaction was asked from the game: has it taken it?
void NetLand_ReactCheck()
{
	int iGen = GetEventData();
	int iId = GetEventData();
	int iState = GetEventData();
	int iDur = GetEventData();
	int iTry = GetEventData();
	string sName = GetEventData();
	if (iGen != iNetLandGen || !bNetLandActive || bNetLandDead) { return; }
	if (iId != iNetLandReactChk) { return; }		// a newer reaction has its own check
	ref mc = GetMainCharacter();
	if (!IsEntity(mc)) { return; }
	int iPk = NetLand_GetFState(mc);
	int iCur = iPk - (iPk / 256) * 256;
	bool bTaken = false;
	if (iCur == iState) { bTaken = true; }
	if (bTaken)
	{
		if (iNetLandReactLog < 150) { iNetLandReactLog++; Trace("COOP: land hero takes the reaction state " + iState + " (try " + iTry + ")"); }
		return;
	}
	// the game went back to the block at once (the key is held), or did not leave the state it was in: once more out of 0
	if (iTry == 0)
	{
		if (bNetLandBreakOk && iState != 14) { SendMessage(mc, "ls", MSG_CHARACTER_EX_MSG, "BreakFight"); }
		else { NetLand_SetFState(mc, 0, 0); }
		NetLand_SetFState(mc, iState, 0);
		PostEvent("NetLand_ReactCheck", 70, "llllls", iNetLandGen, iId, iState, iDur, 1, sName);
		return;
	}
	Trace("COOP: land hero did not take the reaction state " + iState + ", it is in " + iCur + ": " + sName + " is played by its name");
	// the request is taken back: the game must not begin its own reaction after the one played by the name
	NetLand_SetFState(mc, 0, 0);
	int iLeft = iDur - 140;
	if (iLeft < 150) { iLeft = 150; }
	iNetLandStunUntil = Net_GetTickCount() + iLeft;
	NetLand_HeroReact(sName, iLeft);
}

// the result that the attacker has seen at the touch of his blade (NetLand_BladeHit), shown the next frame
void NetLand_AttRes()
{
	int iGen = GetEventData();
	int wV = GetEventData();
	int iDef = GetEventData();
	int iGameBlk = GetEventData();
	if (iGen != iNetLandGen || !bNetLandActive || bNetLandDead) { return; }
	if (iDef == 4)
	{
		// the blow went into a feint: our hero takes the counter blow reaction (hit_feint), the puppet goes on with its feint
		NetLand_HeroFightReact(9, 0, "hit_feint", NET_LAND_T_STAGGER);
		int iFChr = NetLand_GetPuppet(wV);
		if (iFChr >= 0)
		{
			ref chF = &Characters[iFChr];
			if (IsEntity(chF)) { NetLand_PuppetSkipHit(chF, 2); }
		}
		return;
	}
	if (iDef == 3)
	{
		int iBChr = NetLand_GetPuppet(wV);
		if (iBChr >= 0)
		{
			ref chB = &Characters[iBChr];
			if (IsEntity(chB)) { NetLand_PuppetSkipHit(chB, 3); }
		}
		return;
	}
	if (iDef == 2)
	{
		// the blow went into a dodge: our hero is thrown back (single player: hit_parry)
		NetLand_HeroFightReact(10, 0, "hit_parry", NET_LAND_T_STAGGER);
		// the game has put the dodging puppet into its hit reaction at the touch: it is played through, the dodge goes on
		int iPChr = NetLand_GetPuppet(wV);
		if (iPChr >= 0)
		{
			ref chP = &Characters[iPChr];
			if (IsEntity(chP)) { NetLand_PuppetSkipHit(chP, 2); }
		}
		return;
	}
	if (iDef == 1)
	{
		int iVChr = NetLand_GetPuppet(wV);
		if (iVChr < 0) { return; }
		ref chV = &Characters[iVChr];
		if (!IsEntity(chV)) { return; }
		// the game has blocked it itself (the block pose): nothing to do
		if (iGameBlk == 1)
		{
			if (iNetLandPupLog < 30) { iNetLandPupLog++; Trace("COOP: land puppet " + chV.id + ": the game blocked the blow itself"); }
			return;
		}
		NetLand_PuppetBlockHit(chV);
	}
}

// our blow went into the block of a puppet: to the game the puppet does not block, it has put it into its hit reaction (8) at
// the touch. That state takes no other one, not even 0 (the log: "did not take 14, it is in 8"): it is played through at once
// (the animation very fast), then the puppet gets the game's own blow in the block (14). The blades ring at once
void NetLand_PuppetBlockHit(ref chV)
{
	if (!IsEntity(chV)) { return; }
	LAi_CharacterPlaySound(chV, "fgt_collision");
	NetLand_PuppetSkipHit(chV, 1);
}

// the hit reaction of a puppet in the variant that its player gets as well (NetLand_BladeHit)
void NetLand_PupHitVar()
{
	int iGen = GetEventData();
	int iChr = GetEventData();
	int iV = GetEventData();
	if (iGen != iNetLandGen || !bNetLandActive || iChr < 0) { return; }
	ref ch = &Characters[iChr];
	if (!IsEntity(ch)) { return; }
	if (CheckAttribute(ch, "NetLandDead")) { if (sti(ch.NetLandDead)) { return; } }
	if (CheckAttribute(ch, "NetLandHold")) { if (sti(ch.NetLandHold) == 1) { return; } }
	// the action played by its name before (the hit into a feint, a swing) stays in the character: the game takes it for the idle
	// animation when the reaction is over and played it once more ("the extra hit, clearly later"). It is let go here
	CharacterPlayAction(ch, "");
	// its player has no sword out: the game's hit of an unarmed character (the game has begun it at the touch already); the fight
	// state 8 waited for the sword and played the hit of a fight then
	if (SendMessage(ch, "ls", MSG_CHARACTER_EX_MSG, "IsFightMode") == 0)
	{
		NetLand_SetFState(ch, 0, 0);
		if (bNetLandNoFightOk && NetLand_GetAnim(ch, 0) != "HitNoFight") { SendMessage(ch, "ls", MSG_CHARACTER_EX_MSG, "NoFightHit"); }
		DeleteAttribute(ch, "NetLandClearId");
		ch.NetLandAnimCur = "HitNoFight";
		ch.NetLandAnimAt = Net_GetTickCount();
		return;
	}
	int iPk = NetLand_GetFState(ch);
	int iCur = iPk - (iPk / 256) * 256;
	if (iCur >= 13 && iCur <= 16) { return; }		// a block, a block hit, a dodge: not a hit
	// another reaction of the game has begun all the same: it cannot be left before its end, it is broken
	if (iCur >= 9 && iCur <= 12 && bNetLandBreakOk) { SendMessage(ch, "ls", MSG_CHARACTER_EX_MSG, "BreakFight"); iCur = 0; }
	// the reaction asked for at the blow plays already: not begun again
	bool bSameR = false;
	string sWantR = "hit_attack_" + iV;
	if (iCur == 8 && NetLand_GetAnim(ch, 0) == sWantR) { bSameR = true; }
	if (!bSameR) { NetLand_SetFState(ch, 8, iV - 1); }
	// the name of the reaction of its player (it comes a moment later) is the same: it does not begin it again
	DeleteAttribute(ch, "NetLandClearId");
	ch.NetLandAnimCur = "hit_attack_" + iV;
	ch.NetLandAnimAt = Net_GetTickCount();
}

// iWant: 1 the blow in the block, 2 the dodge goes on
void NetLand_PuppetSkipHit(ref ch, int iWant)
{
	int iChr = sti(ch.index);
	ch.NetLandBlkHitUntil = Net_GetTickCount() + 700;		// the block pose waits for this
	iNetLandActCounter++;
	ch.NetLandSkipId = iNetLandActCounter;
	// the old animation that is on the way does not take the puppet anywhere meanwhile
	if (iWant == 1 || iWant == 3) { ch.NetLandPredUntil = Net_GetTickCount() + NET_LAND_PRED_HOLD + 200; }
	bool bInHit = false;
	if (bNetLandFStateOk)
	{
		int iPk = NetLand_GetFState(ch);
		int iCur = iPk - (iPk / 256) * 256;
		if (iCur >= 8 && iCur <= 12) { bInHit = true; }
	}
	if (!bInHit) { NetLand_PuppetSkipDone(ch, iChr, iWant, false); return; }
	if (bNetLandBreakOk)
	{
		SendMessage(ch, "ls", MSG_CHARACTER_EX_MSG, "BreakFight");
		if (iNetLandPupLog < 40) { iNetLandPupLog++; Trace("COOP: land puppet " + ch.id + " hit reaction broken (it defended)"); }
		NetLand_PuppetSkipDone(ch, iChr, iWant, false);
		return;
	}
	NetLand_AnimSpeed(ch, 0, NET_LAND_SKIP_SPEED);
	NetLand_AnimSpeed(ch, 1, NET_LAND_SKIP_SPEED);
	NetLand_AnimSpeed(ch, 2, NET_LAND_SKIP_SPEED);
	PostEvent("NetLand_SkipPoll", 15, "lllll", iNetLandGen, iChr, iNetLandActCounter, iWant, 0);
}

void NetLand_SkipPoll()
{
	int iGen = GetEventData();
	int iChr = GetEventData();
	int iId = GetEventData();
	int iWant = GetEventData();
	int iTry = GetEventData();
	if (iGen != iNetLandGen || !bNetLandActive || iChr < 0) { return; }
	ref ch = &Characters[iChr];
	if (!IsEntity(ch)) { return; }
	if (!CheckAttribute(ch, "NetLandSkipId")) { return; }
	if (sti(ch.NetLandSkipId) != iId) { return; }
	int iPk = NetLand_GetFState(ch);
	int iCur = iPk - (iPk / 256) * 256;
	if (iCur >= 8 && iCur <= 12 && iTry < 20)
	{
		PostEvent("NetLand_SkipPoll", 15, "lllll", iNetLandGen, iChr, iId, iWant, iTry + 1);
		return;
	}
	if (iNetLandPupLog < 30) { iNetLandPupLog++; Trace("COOP: land puppet " + ch.id + " hit reaction played through in " + (iTry * 15 + 15) + " ms, state now " + iCur); }
	NetLand_PuppetSkipDone(ch, iChr, iWant, true);
}

void NetLand_PuppetSkipDone(ref ch, int iChr, int iWant, bool bSped)
{
	if (bSped)
	{
		NetLand_AnimSpeed(ch, 0, 100);
		NetLand_AnimSpeed(ch, 1, 100);
		NetLand_AnimSpeed(ch, 2, 100);
	}
	if (CheckAttribute(ch, "NetLandDead")) { if (sti(ch.NetLandDead)) { return; } }
	if (iWant == 3)
	{
		// the break blow into the block: the game's own block_break (15)
		if (bNetLandFStateOk) { NetLand_SetFState(ch, 15, 0); }
		ch.NetLandAnimCur = "";
		if (NetLand_PuppetAnimSync(ch)) { NetLand_PuppetPredict(ch, "block_break", NET_LAND_PRED_HOLD); }
		return;
	}
	if (iWant == 1)
	{
		if (bNetLandFStateOk)
		{
			NetLand_SetFState(ch, 14, 0);
			iNetLandReactChk++;
			PostEvent("NetLand_PupReactCheck", 40, "lll", iNetLandGen, iChr, iNetLandReactChk);
		}
		ch.NetLandAnimCur = "";
		if (NetLand_PuppetAnimSync(ch)) { NetLand_PuppetPredict(ch, "block_hit", NET_LAND_PRED_HOLD); }
		else { NetLand_PuppetFlinch(ch, "block_hit", NET_LAND_T_FLINCH); }
		return;
	}
	// the dodge goes on: the newest animation of its player (parry_N) is played again
	DeleteAttribute(ch, "NetLandPredUntil");
	string sAnim = "";
	if (CheckAttribute(ch, "NetLandAnimLast")) { sAnim = ch.NetLandAnimLast; }
	ch.NetLandAnimCur = "";
	if (sAnim != "") { NetLand_PuppetPlayName(ch, iChr, sAnim); }
}

void NetLand_PupReactCheck()
{
	int iGen = GetEventData();
	int iChr = GetEventData();
	int iId = GetEventData();
	if (iGen != iNetLandGen || !bNetLandActive || iChr < 0) { return; }
	ref ch = &Characters[iChr];
	if (!IsEntity(ch)) { return; }
	int iPk = NetLand_GetFState(ch);
	int iCur = iPk - (iPk / 256) * 256;
	if (iCur == 14)
	{
		if (iNetLandPupLog < 10) { iNetLandPupLog++; Trace("COOP: land puppet " + ch.id + " takes the block hit state 14"); }
		return;
	}
	Trace("COOP: land puppet " + ch.id + " did not take the block hit state 14, it is in " + iCur);
}

// what does the player of this puppet do against a blow now, by his own game (the fight state of his hero comes with his
// animation, a change at once and else every 250 ms): 2 a dodge (parry), 1 a block, 0 nothing
int NetLand_PuppetDefence(ref ch, string attackType)
{
	if (!CheckAttribute(ch, "NetLandHSt")) { return 0; }
	if (!CheckAttribute(ch, "NetLandHStAt")) { return 0; }
	int iAge = Net_GetTickCount() - sti(ch.NetLandHStAt);
	if (iAge < 0 || iAge > 600) { return 0; }
	int iSt = sti(ch.NetLandHSt);
	if (iSt == 16) { return 2; }
	if (iSt == 5) { return 4; }		// a feint: the blow into it is answered by the counter blow (single player)
	if (iSt == 13 || iSt == 14 || iSt == 15)
	{
		if (attackType == "break") { return 3; }		// the break blow breaks the block (single player): block_break
		return 1;
	}
	return 0;
}

// does the game itself play a reaction of a hit on our hero now (state 8 hit, 14 block hit, or the animation of it)?
bool NetLand_HeroReacting(ref mc)
{
	int iPk = NetLand_GetFState(mc);
	if (iPk != 0)
	{
		int iNum = iPk - (iPk / 256) * 256;
		if (iNum == 8 || iNum == 14) { return true; }
	}
	string sA = NetLand_GetAnim(mc, 0);
	if (findSubStr(sA, "hit_", 0) == 0 || findSubStr(sA, "block_hit", 0) == 0) { return true; }
	return false;
}

// a reaction that the script plays on our hero: it is cleared by the timer (CharacterPlayAction repeats until it is cleared)
void NetLand_HeroReact(string sAction, int iDur)
{
	ref mc = GetMainCharacter();
	if (!IsEntity(mc) || bNetLandDead) { return; }
	iNetLandReactId++;
	CharacterPlayAction(mc, sAction);
	PostEvent("NetLand_ReactEnd", iDur, "ll", iNetLandGen, iNetLandReactId);
}

void NetLand_ReactEnd()
{
	int iGen = GetEventData();
	int iId = GetEventData();
	if (iGen != iNetLandGen || !bNetLandActive || bNetLandDead) { return; }
	if (iId != iNetLandReactId) { return; }		// a newer reaction has its own timer
	ref mc = GetMainCharacter();
	if (IsEntity(mc)) { CharacterPlayAction(mc, ""); }
}

void NetLand_HitReact()
{
	int iGen = GetEventData();
	int iKind = GetEventData();
	if (iGen != iNetLandGen || !bNetLandActive || bNetLandDead) { return; }
	ref mc = GetMainCharacter();
	if (!IsEntity(mc)) { return; }
	if (NetLand_HeroReacting(mc)) { return; }
	if (iKind == 1)
	{
		int iPkB = NetLand_GetFState(mc);
		int iNumB = iPkB - (iPkB / 256) * 256;
		if (iPkB != 0 && iNumB == 13) { return; }
		NetLand_HeroReact("block_hit", NET_LAND_T_BLOCKHIT);
	}
	else { NetLand_HeroReact("hit_attack_" + (1 + rand(2)), NET_LAND_T_HIT); }
}

void NetLand_LocalDie(int wKiller)
{
	if (bNetLandDead) { return; }
	// COOP s27: immortality of the admin menu: the hero gets the whole health back instead of dying
	if (CheckAttribute(&NCClients[wClientID], "God"))
	{
		if (sti(NCClients[wClientID].God))
		{
			ref mcGod = GetMainCharacter();
			if (IsEntity(mcGod)) { LAi_SetHP(mcGod, LAi_GetCharacterMaxHP(mcGod), LAi_GetCharacterMaxHP(mcGod)); }
			Trace("COOP: land the hero is immortal (admin), no death");
			return;
		}
	}
	Trace("COOP: land our hero dies, the killer " + wKiller);
	NetLand_PoisonOff();		// a dead body is not poisoned any more
	bNetLandDead = true;
	bNetLandFrozen = false;
	iNetLandStunUntil = 0;
	NetLand_LockFight(0);
	iNetLandDeadAt = Net_GetTickCount();
	iNetLandDeadReplay = 0;
	ref mc = GetMainCharacter();
	mc.chr_ai.hp = 0.0;
	// the death task of the game, the very call that makes the puppet of a dead player fall on the other screens: the body
	// falls once and stays lying (a played action repeats until cleared: the old way looped, and the actor type took the
	// sword away first). One of the four deaths, the same one is shown to the others. A body killed by the task cannot get
	// up: the respawn loads the location again (NetLand_OnRespawn)
	// without the sword out the game plays the death of an unarmed man (death_citizen), the puppets the same
	if (SendMessage(mc, "ls", MSG_CHARACTER_EX_MSG, "IsFightMode") != 0) { iNetLandDeadVar = 1 + rand(3); }
	else { iNetLandDeadVar = 5 + rand(1); }
	NetLand_MushUntie(mc);
	Trace("COOP: land our hero dies: death " + iNetLandDeadVar + ", animation " + mc.model.animation + ", model " + mc.model);
	NetLand_SetDeathVariant(mc, iNetLandDeadVar);
	bNetLandDeadTask = true;
	SetCharacterTask_Dead(mc);
	Trace("COOP: land our hero dies: the death task is given");

	int iSMsg = NetClient_LandNewMsg(NSC_LAND_DEAD);
	NMAddClientID(iSMsg, wKiller);
	int iDeadVarSend = iNetLandDeadVar;
	if (bNetLandPoisonDeath) { iDeadVarSend = iDeadVarSend + 32; }		// the kill line of the others says "poison"
	NMAddByte(iSMsg, iDeadVarSend);
	NetClient_LandSend(iSMsg, true);
	NMDelete(iSMsg);

	if (iNetLandRespawn) { NetClient_LogMessageNum("Net_RespawnIn", iNetLandRespawnSec, 0); }
	else { Log_SetStringToLog(LanguageConvertString(iLangNetClient, "Net_LandYouDied")); }
}

// LAi_events.c: the dead hero lies where he fell
bool NetLand_HeroDeadHold(aref chr)
{
	if (!bNetLandActive || !bNetLandDead) { return false; }
	// the dead hero lies by the death task of the game: no template touches him
	if (sti(chr.index) == GetMainCharacterIndex()) { return true; }
	return false;
}

// ---------- the other players ----------
// puppet of a player (created at his first state packet)
// a character of ours that the game has made new (EmptyAllFantomCharacter: InitCharacter of the dead fantoms and of the old
// ones). Its body stayed in the location with no attributes: it stood still, and the script stopped on it with an error
// ("missed attribute netlanddead") at every packet of the player - his puppet froze. The body goes away.
void NetLand_StaleBody(int iP)
{
	if (iP < 0 || iP >= TOTAL_CHARACTERS) { return; }
	if (Characters[iP].id != "0") { return; }		// another character has the place now: not ours to touch
	if (IsEntity(&Characters[iP]))
	{
		SendMessage(&Characters[iP], "lsl", MSG_CHARACTER_EX_MSG, "NetSwim", 0);
		ChangeCharacterAddressGroup(&Characters[iP], "none", "", "");
	}
}

// the character of the puppet of a player, -1 when there is none or the place is not his any more
int NetLand_PupIdx(int wID)
{
	if (wID < 0 || wID >= NET_MAXPLAYERS) { return -1; }
	if (!CheckAttribute(&NCClients[wID], "LandChr")) { return -1; }
	int iP = sti(NCClients[wID].LandChr);
	if (iP >= 0 && iP < TOTAL_CHARACTERS)
	{
		if (CheckAttribute(&Characters[iP], "NetLandID"))
		{
			if (sti(Characters[iP].NetLandID) == wID) { return iP; }
		}
		NetLand_StaleBody(iP);
	}
	Trace("COOP: land the puppet of " + wID + " (character " + iP + ") has been made new by the game: a new puppet");
	DeleteAttribute(&NCClients[wID], "LandChr");
	return -1;
}

int NetLand_GetPuppet(int wID)
{
	if (NetLand_IsLandBot(wID)) { return NetLand_BotChr(wID); }
	if (wID < 0 || wID >= NET_MAXPLAYERS || wID == wClientID) { return -1; }
	int iHas = NetLand_PupIdx(wID);
	if (iHas >= 0) { return iHas; }
	if (!IsEntity(loadedLocation)) { return -1; }

	string sPModel = NetLand_ModelFor(wID);
	iNetLandPupGen++;
	string sPAnim = NetLand_ModelAnim(sPModel);
	if (NetLand_ClientMusket(wID)) { sPAnim = "mushketer"; }
	int iChr = NPC_GenerateCharacter("NetLandP_" + wID + "_" + iNetLandPupGen, sPModel, NetLand_ModelSex(sPModel), sPAnim, 10, PIRATE, 0, false);
	if (iChr < 0) { return -1; }
	ref ch = &Characters[iChr];
	ch.name = NCClients[wID].NickName;
	ch.lastname = "";
	ch.NetLandID = wID;
	ch.NetLandSkin = sPModel;
	ch.RebirthPhantom = true;		// a dead puppet is not made new by the game while it lies in the location
	ch.NetLandMush = 0;
	if (NetLand_ClientMusket(wID)) { ch.NetLandMush = 1; }
	NetLand_SetAnimFile(ch, "");
	if (NetLand_ClientMusket(wID)) { NetLand_SetAnimFile(ch, NetLand_MushAnimFile(sPModel)); }		// COOP s30
	ch.NetLandDead = false;
	DeleteAttribute(ch, "chr_ai.hp");
	NetLand_ApplyPuppetLo(ch, wID);
	LAi_SetImmortal(ch, true);
	LAi_CharacterDisableDialog(ch);
	LAi_SetActorType(ch);
	// every type of the game sets it (LAi_warrior.c...), the actor does not: a puppet fought with bare hands
	SendMessage(ch, "lsl", MSG_CHARACTER_EX_MSG, "SetFightWOWeapon", false);
	LAi_group_MoveCharacter(ch, NetLand_PuppetGroup(wID));		// COOP: team duel: an ally is a friend
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
		// "SetFightWOWeapon" true takes the whole weapon of the character (blade.dll: the sword AND the gun) out of the drawn
		// layer (Location.dll: LayerDel "realize"): the musket that is the gun in the hand was gone in the fight. Only a twin
		// model holds its musket itself
		if (CheckAttribute(ch, "NetLandMushTie")) { if (sti(ch.NetLandMushTie) == 1) { bWO = false; } }
		SendMessage(ch, "lsl", MSG_CHARACTER_EX_MSG, "SetFightWOWeapon", bWO);
		// a musketeer holds the musket already: sending the weapons again takes it out of his hands
		if (ch.model.animation != "mushketer") { NetLand_PuppetWeapons(ch); }
		SendMessage(ch, "lsl", MSG_CHARACTER_EX_MSG, "ChangeFightMode", 1);
		SendMessage(ch, "lsl", MSG_CHARACTER_EX_MSG, "LockFightMode", 1);
	}
	else
	{
		SendMessage(ch, "lsl", MSG_CHARACTER_EX_MSG, "ChangeFightMode", 0);
	}
	NetLand_MushHand(ch);
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
	int iEnR = NMGetWord(iMsg);
	int iEnMaxR = NMGetWord(iMsg);

	if (!bNetLandActive || !bNetLandLocReady) { return; }		// read to the end, used only when the location is loaded
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
				NetLand_SwimSet(ch, 0);
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
	// the energy of the player (the game takes some from the puppet for the blows that it thinks hit it)
	if (iEnMaxR > 0)
	{
		ch.chr_ai.energyMax = makefloat(iEnMaxR);
		ch.chr_ai.energy = makefloat(iEnR);
	}
	if (!CheckAttribute(ch, "NetLandDead")) { ch.NetLandDead = false; }
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
	// how often the packets of this player come (once in 3 s into the log)
	int iNowSt = Net_GetTickCount();
	int iPkN = 0;
	int iPkGap = 0;
	if (CheckAttribute(ch, "NetLandPkN")) { iPkN = sti(ch.NetLandPkN); }
	if (CheckAttribute(ch, "NetLandPkGap")) { iPkGap = sti(ch.NetLandPkGap); }
	if (CheckAttribute(ch, "NetLandLT"))
	{
		int iGapSt = iNowSt - sti(ch.NetLandLT);
		if (iGapSt > iPkGap) { iPkGap = iGapSt; }
	}
	iPkN++;
	if (!CheckAttribute(ch, "NetLandPkAt")) { ch.NetLandPkAt = iNowSt; }
	int iPkSpan = iNowSt - sti(ch.NetLandPkAt);
	if (iPkSpan >= 3000)
	{
		int iSnaps = 0;
		int iTpFail = 0;
		if (CheckAttribute(ch, "NetLandSnapN")) { iSnaps = sti(ch.NetLandSnapN); }
		if (CheckAttribute(ch, "NetLandTpFail")) { iTpFail = sti(ch.NetLandTpFail); }
		Trace("COOP: land puppet of " + wID + ": " + iPkN + " state packets in " + iPkSpan + " ms, the longest gap " + iPkGap + " ms, jumps " + iSnaps + ", failed moves " + iTpFail);
		iPkN = 0;
		iPkGap = 0;
		ch.NetLandPkAt = iNowSt;
		ch.NetLandSnapN = 0;
		ch.NetLandTpFail = 0;
	}
	ch.NetLandPkN = iPkN;
	ch.NetLandPkGap = iPkGap;
	ch.NetLandLT = iNowSt;
	ch.NetLandLFight = iFight;
	int iSyncA = iFlags / 64;
	int iSyncB = iFlags / 128;
	iSyncB = iSyncB * 2;
	int iSyncC = iSyncA - iSyncB;
	if (iSyncC == 1 && !CheckAttribute(ch, "NetLandAnimSync")) { Trace("COOP: land puppet of " + wID + " plays the animation of its hero"); }
	ch.NetLandAnimSync = iSyncC;
	ch.NetLandNative = iFlags / 128;
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
	// a swimmer: the speed measured over 40 ms swings with the waves, the run ahead of the packet made the puppet jerk to and fro
	bool bSwimM = NetLand_PuppetSwims(ch);
	if (bSwimM && iNetLandSwimLead == 0) { fT = 0.0; }
	// the game of this machine decided by its own waves whether the puppet swims: near the depth of the swim it stopped and
	// began again and again (drawn on the bottom, then on the water). The patched Location.dll takes it from here: it swims
	// exactly while its player swims (drawn on the water of this machine)
	if (bSwimM) { NetLand_SwimSet(ch, 1); }
	else { NetLand_SwimSet(ch, 2); }
	if (bSwimM && iNetLandSwimLog < 30)
	{
		int iSwLogAgo = 1000;
		if (CheckAttribute(ch, "NetLandSwimLogAt")) { iSwLogAgo = iNow - sti(ch.NetLandSwimLogAt); }
		if (iSwLogAgo < 0 || iSwLogAgo >= 700)
		{
			ch.NetLandSwimLogAt = iNow;
			iNetLandSwimLog++;
			float fSwX = 0.0;
			float fSwY = 0.0;
			float fSwZ = 0.0;
			GetCharacterPos(ch, &fSwX, &fSwY, &fSwZ);
			Trace("COOP: land puppet " + ch.id + " swims: its y " + fSwY + ", the y of its player " + stf(ch.NetLandLY) + ", the way " + sqrt((fSwX - stf(ch.NetLandLX)) * (fSwX - stf(ch.NetLandLX)) + (fSwZ - stf(ch.NetLandLZ)) * (fSwZ - stf(ch.NetLandLZ))) + ", it plays " + NetLand_GetAnim(ch, 0));
		}
	}
	float fTX = stf(ch.NetLandLX) + fVX * fT;
	float fTY = stf(ch.NetLandLY);
	float fTZ = stf(ch.NetLandLZ);
	fTZ = fTZ + fVZ * fT;
	if (bSwimM && iNetLandSwimDiag < 150)
	{
		if (!CheckAttribute(ch, "NetLandSwDiagEnd")) { ch.NetLandSwDiagEnd = iNow + 3000; ch.NetLandSwDiagAt = 0; }
		int iSwDEnd = sti(ch.NetLandSwDiagEnd);
		if (iNow < iSwDEnd && iNow - sti(ch.NetLandSwDiagAt) >= 40)
		{
			ch.NetLandSwDiagAt = iNow;
			iNetLandSwimDiag++;
			float fDgX = 0.0;
			float fDgY = 0.0;
			float fDgZ = 0.0;
			float fDgA = 0.0;
			GetCharacterPos(ch, &fDgX, &fDgY, &fDgZ);
			GetCharacterAy(ch, &fDgA);
			Trace("COOP: swim " + ch.id + " t " + iNow + " age " + iAgeMs + " cur " + fDgX + " " + fDgY + " " + fDgZ + " " + fDgA + " | pkt " + stf(ch.NetLandLX) + " " + stf(ch.NetLandLY) + " " + stf(ch.NetLandLZ) + " " + stf(ch.NetLandLAy) + " v " + fVX + " " + fVZ + " | " + NetLand_GetAnim(ch, 0));
		}
	}
	if (!bSwimM) { DeleteAttribute(ch, "NetLandSwDiagEnd"); }
	float fAy = stf(ch.NetLandLAy);
	// the game of a player who runs W + A / D (S + A / D) turns his model along the way, but his "ay" stays where the camera
	// looks: the puppet ran straight and moved on the slant. While W / S is held and it runs, it faces the way it goes
	float fVSpF = sqrt(fVX * fVX + fVZ * fVZ);
	int iBlkF = 0;
	if (CheckAttribute(ch, "NetLandBlock")) { iBlkF = sti(ch.NetLandBlock); }
	if (NET_LAND_FACE_RUN == 2 && fVSpF > 1.2 && iBlkF == 0 && !NetLand_PuppetActing(ch))
	{
		int iFwdF = 0;
		int iBackF = 0;
		if (CheckAttribute(ch, "NetLandLFwd")) { iFwdF = sti(ch.NetLandLFwd); }
		if (CheckAttribute(ch, "NetLandLBack")) { iBackF = sti(ch.NetLandLBack); }
		float fRunAy = fAy;
		bool bRunAy = false;
		if (iFwdF == 1) { fRunAy = atan2(fVX, fVZ); bRunAy = true; }
		if (iFwdF == 0 && iBackF == 1) { fRunAy = atan2(-fVX, -fVZ); bRunAy = true; }
		if (bRunAy)
		{
			ch.NetLandRunAy = fRunAy;
			ch.NetLandRunAyAt = iNow;
		}
	}
	// a moment of a slower packet (or a step that the game makes on the way) does not turn the puppet back to the camera
	if (NET_LAND_FACE_RUN == 2 && CheckAttribute(ch, "NetLandRunAyAt"))
	{
		int iRunAyAgo = iNow - sti(ch.NetLandRunAyAt);
		int iFwdK = 0;
		int iBackK = 0;
		if (CheckAttribute(ch, "NetLandLFwd")) { iFwdK = sti(ch.NetLandLFwd); }
		if (CheckAttribute(ch, "NetLandLBack")) { iBackK = sti(ch.NetLandLBack); }
		bool bKeyK = false;
		if (iFwdK == 1 || iBackK == 1) { bKeyK = true; }
		if (bKeyK && iRunAyAgo >= 0 && iRunAyAgo < 300) { fAy = stf(ch.NetLandRunAy); }
	}

	float fCX = 0.0;
	float fCY = 0.0;
	float fCZ = 0.0;
	if (!GetCharacterPos(ch, &fCX, &fCY, &fCZ)) { return; }
	float fCAy = 0.0;
	GetCharacterAy(ch, &fCAy);
	// the puppet is not put into our hero: the game pushes two characters that stand in each other apart, and our hero was pushed
	// away at every blow of the other player (his lunge, the lag of the packets)
	if (!bNetLandDead)
	{
		ref mcN = GetMainCharacter();
		float fHX = 0.0;
		float fHY = 0.0;
		float fHZ = 0.0;
		if (IsEntity(mcN) && GetCharacterPos(mcN, &fHX, &fHY, &fHZ))
		{
			float fOX = fTX - fHX;
			float fOZ = fTZ - fHZ;
			float fOY = fTY - fHY;
			float fOD = sqrt(fOX * fOX + fOZ * fOZ);
			if (fOD < NET_LAND_MIN_DIST && fOY > -1.5 && fOY < 1.5)
			{
				// the side from which it comes (where it stands now) if the point is right in our hero
				if (fOD < 0.05)
				{
					fOX = fCX - fHX;
					fOZ = fCZ - fHZ;
					fOD = sqrt(fOX * fOX + fOZ * fOZ);
				}
				if (fOD >= 0.05)
				{
					fTX = fHX + fOX * NET_LAND_MIN_DIST / fOD;
					fTZ = fHZ + fOZ * NET_LAND_MIN_DIST / fOD;
				}
			}
		}
	}
	float fDX = fTX - fCX;
	float fDZ = fTZ - fCZ;
	float fDist = sqrt(fDX * fDX + fDZ * fDZ);
	float fDA = fAy - fCAy;
	while (fDA > 3.14159265) { fDA = fDA - 6.2831853; }
	while (fDA < -3.14159265) { fDA = fDA + 6.2831853; }

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
		if (fDist > NET_LAND_SNAP)
		{
			int iSnapN = 0;
			if (CheckAttribute(ch, "NetLandSnapN")) { iSnapN = sti(ch.NetLandSnapN); }
			ch.NetLandSnapN = iSnapN + 1;
		}
		if (fDist <= NET_LAND_SNAP)
		{
			float fKM = NET_LAND_SMOOTH_K;
			float fKA = NET_LAND_SMOOTH_K;
			if (bSwimM) { fKM = makefloat(iNetLandSwimK) / 100.0; fKA = 0.3; }
			fNX = fCX + fDX * fKM;
			// the height is the height of the player at once, not a part of the way: the game puts a character on the floor of
			// the place that is the nearest to the height it gets (a ship: the deck or the deck under it). A height that went
			// after the player slowly was nearer to the upper deck, so the puppet of a player under the deck stood on it and was
			// thrown from one deck to the other while he walked
			fNY = fTY;
			// the game draws a swimmer on the waves of this machine (Location.dll: the water height - 0.8 of his height, not his
			// own one) and decides by his own height (the bottom under him) whether he swims: his feet 1.44 m (+-0.03) under the
			// water of THIS machine. The waves here are not the waves of his player: near that depth the puppet stopped and began
			// swimming again and again (drawn on the bottom, then on the water: it jerked). While its player swims it stands a bit
			// deeper than he: it swims for sure, and the picture does not change (a swimmer is drawn by the water, not by his feet)
			fNZ = fCZ + fDZ * fKM;
			fNAy = fCAy + fDA * fKA;
		}
		if (fDist > 0.01 || fDA > 0.01 || fDA < -0.01)
		{
			if (!TeleportCharacterToPosAy(ch, fNX, fNY, fNZ, fNAy))
			{
				int iTpF = 0;
				if (CheckAttribute(ch, "NetLandTpFail")) { iTpF = sti(ch.NetLandTpFail); }
				ch.NetLandTpFail = iTpF + 1;
			}
			else { NetLand_FloorFix(ch, fNY, fNAy); }
		}
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

// the game puts a character on the floor whose height is the nearest to the one it gets, among the floors that are under
// its point. A point on the way to the player (the smoothing, the step away from our hero) can be where his floor has no
// patch (a wall, a corner, the stairs under the deck): there the deck above was the only one and the puppet stood on it for a
// moment. Then it goes to the point of its player himself (he stands on his floor there)
void NetLand_FloorFix(ref ch, float fWantY, float fAyF)
{
	if (!CheckAttribute(ch, "NetLandLX")) { return; }
	float fFX = 0.0;
	float fFY = 0.0;
	float fFZ = 0.0;
	if (!GetCharacterPos(ch, &fFX, &fFY, &fFZ)) { return; }
	if (fFY - fWantY < 0.8) { return; }
	TeleportCharacterToPosAy(ch, stf(ch.NetLandLX), stf(ch.NetLandLY), stf(ch.NetLandLZ), fAyF);
	int iFlF = 0;
	if (CheckAttribute(ch, "NetLandFloorFix")) { iFlF = sti(ch.NetLandFloorFix); }
	ch.NetLandFloorFix = iFlF + 1;
	if (iFlF < 20) { Trace("COOP: land puppet " + ch.id + " was put on a floor " + (fFY - fWantY) + " m above its player: it goes to his point"); }
}

// the block of the player is held on his puppet: the game drops the state of a puppet in a fraction of a second, so while
// the player holds the block it is asked for again (not more often than every 60 ms, and only when it is gone)
void NetLand_PuppetBlockHold(ref ch, int iNow)
{
	if (!NetLand_NativePuppet(ch)) { return; }
	if (!CheckAttribute(ch, "NetLandFSt")) { return; }
	if (sti(ch.NetLandFSt) != 13) { return; }
	int iLast = 0;
	if (CheckAttribute(ch, "NetLandBlkAt")) { iLast = sti(ch.NetLandBlkAt); }
	if (iNow - iLast < 60) { return; }
	ch.NetLandBlkAt = iNow;
	int iPk = NetLand_GetFState(ch);
	int iCur = iPk - (iPk / 256) * 256;
	if (iCur == 13) { return; }
	NetLand_SetFState(ch, 13, 0);
	if (iNetLandBlkLog < 8)
	{
		iNetLandBlkLog++;
		int iPk2 = NetLand_GetFState(ch);
		Trace("COOP: land puppet " + ch.id + " block asked again, state was " + iCur + " now " + (iPk2 - (iPk2 / 256) * 256));
	}
}

// while its player blocks, the puppet stands in the game's own state of a blow in the block (14) frozen at its first frame:
// the game blocks a blow into it itself, like a block of a player or of an NPC (13 is not taken by a puppet: it is held by the
// keys or the AI, the log "did not take the fight state 13"). A blow plays the block hit at its speed, then the pose is taken again
void NetLand_PuppetBlockPose(ref ch, int iNow)
{
	if (NET_LAND_BLOCK_POSE != 1 || !bNetLandFStateOk) { return; }
	if (CheckAttribute(ch, "NetLandPoseOff")) { return; }
	if (!NetLand_PuppetAnimSync(ch)) { return; }
	bool bWant = false;
	if (CheckAttribute(ch, "NetLandHSt") && CheckAttribute(ch, "NetLandHStAt"))
	{
		int iAgeP = iNow - sti(ch.NetLandHStAt);
		int iStP = sti(ch.NetLandHSt);
		if (iAgeP >= 0 && iAgeP < 600)
		{
			if (iStP == 13 || iStP == 14) { bWant = true; }
		}
	}
	int iPose = 0;
	if (CheckAttribute(ch, "NetLandBlkPose")) { iPose = sti(ch.NetLandBlkPose); }
	if (bWant)
	{
		if (CheckAttribute(ch, "NetLandBlkHitUntil"))
		{
			int iHitLeft = sti(ch.NetLandBlkHitUntil) - iNow;
			if (iHitLeft > 0) { return; }		// a blow in the block (or the game's reaction) plays
		}
		int iPk = NetLand_GetFState(ch);
		int iCur = iPk - (iPk / 256) * 256;
		if (iCur >= 8 && iCur <= 12) { return; }
		int iFrozen = 0;
		if (CheckAttribute(ch, "NetLandBlkFrozen")) { iFrozen = sti(ch.NetLandBlkFrozen); }
		if (iCur == 14 && iFrozen == 1) { return; }
		if (iCur != 14)
		{
			NetLand_SetFState(ch, 14, 0);
			int iPk2 = NetLand_GetFState(ch);
			int iCur2 = iPk2 - (iPk2 / 256) * 256;
			if (iCur2 != 14)
			{
				int iFail = 0;
				if (CheckAttribute(ch, "NetLandPoseFail")) { iFail = sti(ch.NetLandPoseFail); }
				iFail++;
				ch.NetLandPoseFail = iFail;
				if (iFail >= 6)
				{
					ch.NetLandPoseOff = 1;
					Trace("COOP: land puppet " + ch.id + " does not take the block pose (it is in " + iCur2 + "): the block is resolved by the script");
				}
				return;
			}
		}
		NetLand_AnimSpeed(ch, 0, 0);
		NetLand_AnimSpeed(ch, 1, 0);
		NetLand_AnimSpeed(ch, 2, 0);
		ch.NetLandBlkFrozen = 1;
		if (iPose == 0)
		{
			ch.NetLandBlkPose = 1;
			if (iNetLandPupLog < 30) { iNetLandPupLog++; Trace("COOP: land puppet " + ch.id + " stands in the block pose"); }
		}
		return;
	}
	if (iPose == 1)
	{
		NetLand_PuppetPoseOff(ch);
		// the newest animation of its player
		string sLastP = "";
		if (CheckAttribute(ch, "NetLandAnimLast")) { sLastP = ch.NetLandAnimLast; }
		ch.NetLandAnimCur = "";
		if (sLastP != "") { NetLand_PuppetPlayName(ch, sti(ch.index), sLastP); }
	}
}

// the block of a puppet by the game itself (Location.dll "SetBlockHold"): while its player blocks, the game keeps the puppet in its
// block (13) as it keeps the player's while SPACE is down. A blow into it is blocked by the game: the block hit, the block break
// of a break blow, the sounds, no hit reaction, no cry, no lost energy. After a blow in the block the game goes to 0: the block
// is asked for again (the player's game does the same while SPACE is down)
void NetLand_PuppetHoldBlock(ref ch, int iNow)
{
	if (!bNetLandHoldOk) { return; }
	if (!NetLand_PuppetAnimSync(ch)) { return; }
	if (CheckAttribute(ch, "NetLandDead")) { if (sti(ch.NetLandDead)) { return; } }
	bool bWant = false;
	if (CheckAttribute(ch, "NetLandHSt") && CheckAttribute(ch, "NetLandHStAt"))
	{
		int iAgeH = iNow - sti(ch.NetLandHStAt);
		int iStH = sti(ch.NetLandHSt);
		if (iAgeH >= 0 && iAgeH < 600)
		{
			if (iStH == 13 || iStH == 14) { bWant = true; }
		}
	}
	if (bWant)
	{
		if (SendMessage(ch, "ls", MSG_CHARACTER_EX_MSG, "IsFightMode") == 0) { bWant = false; }
	}
	int iHold = 0;
	if (CheckAttribute(ch, "NetLandHold")) { iHold = sti(ch.NetLandHold); }
	if (!bWant)
	{
		if (iHold == 1) { NetLand_PuppetHoldOff(ch, true); }
		return;
	}
	if (iHold == 0)
	{
		SendMessage(ch, "lsl", MSG_CHARACTER_EX_MSG, "SetBlockHold", 1);
		ch.NetLandHold = 1;
		// the animation by its name gives way to the block of the game
		DeleteAttribute(ch, "NetLandClearId");
		DeleteAttribute(ch, "NetLandPredUntil");
		CharacterPlayAction(ch, "");
		ch.NetLandAnimCur = "";
		if (iNetLandPupLog < 30) { iNetLandPupLog++; Trace("COOP: land puppet " + ch.id + " holds the block by the game"); }
	}
	int iPk = NetLand_GetFState(ch);
	int iCur = iPk - (iPk / 256) * 256;
	if (iCur >= 13 && iCur <= 15) { return; }		// in the block, a blow in it, the block broken
	if (iCur >= 8 && iCur <= 12 && bNetLandBreakOk)
	{
		// the reaction that our blow gave: its player is in the block already
		SendMessage(ch, "ls", MSG_CHARACTER_EX_MSG, "BreakFight");
		iCur = 0;
	}
	if (iCur >= 1 && iCur <= 12) { return; }		// a swing, a reaction: the block comes after it
	if (CheckAttribute(ch, "NetLandHoldAskAt"))
	{
		int iAskAgo = iNow - sti(ch.NetLandHoldAskAt);
		if (iAskAgo >= 0 && iAskAgo < 40) { return; }
	}
	ch.NetLandHoldAskAt = iNow;
	NetLand_SetFState(ch, 13, 0);
}

void NetLand_PuppetHoldOff(ref ch, bool bReplay)
{
	if (!CheckAttribute(ch, "NetLandHold")) { return; }
	if (sti(ch.NetLandHold) == 0) { return; }
	ch.NetLandHold = 0;
	if (IsEntity(ch)) { SendMessage(ch, "lsl", MSG_CHARACTER_EX_MSG, "SetBlockHold", 0); }
	if (!bReplay) { return; }
	// the newest animation of its player
	string sLastH = "";
	if (CheckAttribute(ch, "NetLandAnimLast")) { sLastH = ch.NetLandAnimLast; }
	ch.NetLandAnimCur = "";
	if (sLastH != "") { NetLand_PuppetPlayName(ch, sti(ch.index), sLastH); }
}

void NetLand_PuppetPoseOff(ref ch)
{
	NetLand_AnimSpeed(ch, 0, 100);
	NetLand_AnimSpeed(ch, 1, 100);
	NetLand_AnimSpeed(ch, 2, 100);
	ch.NetLandBlkPose = 0;
	ch.NetLandBlkFrozen = 0;
	int iPk = NetLand_GetFState(ch);
	int iCur = iPk - (iPk / 256) * 256;
	if (iCur == 14) { NetLand_SetFState(ch, 0, 0); }
}

// COOP s36: the meter of the speed of the game: every 5 s of the clock the time of the game (GetDeltaTime of the frames) is
// compared with the clock; the engine slows the game down when the frames are too long. A line in compile.log then.
int iNetLandMeterAt = 0;
int iNetLandMeterGame = 0;
int iNetLandMeterFrames = 0;
void NetLand_SlowMeter(int iNow)
{
	iNetLandMeterGame = iNetLandMeterGame + GetDeltaTime();
	iNetLandMeterFrames++;
	int iAgo = iNow - iNetLandMeterAt;
	if (iNetLandMeterAt == 0 || iAgo < 0) { iNetLandMeterAt = iNow; iNetLandMeterGame = 0; iNetLandMeterFrames = 0; return; }
	if (iAgo < 5000) { return; }
	float fK = makefloat(iNetLandMeterGame) / makefloat(iAgo);
	bool bWin = false;
	if (CheckAttribute(&InterfaceStates, "Launched")) { if (sti(InterfaceStates.Launched)) { bWin = true; } }
	if (!bWin && fK < fNetLandSpeed * 0.85)
	{
		Trace("COOP: land SLOW: " + iAgo + " ms of the clock, " + iNetLandMeterGame + " ms of the game (x" + fK + "), " + iNetLandMeterFrames + " frames (" + (iNetLandMeterFrames * 1000 / iAgo) + " fps), " + sNetLandLoc + ", " + LAi_numloginedcharacters + " characters");
	}
	iNetLandMeterAt = iNow;
	iNetLandMeterGame = 0;
	iNetLandMeterFrames = 0;
}

void NetLand_Smooth()
{
	int iGen = GetEventData();
	if (iGen != iNetLandGen || !bNetLandActive) { return; }
	int iNow = Net_GetTickCount();
	for (int i=0; i<NET_MAXPLAYERS; i++)
	{
		if (i == wClientID) { continue; }
		int iChr = NetLand_PupIdx(i);
		if (iChr < 0) { continue; }
		ref ch = &Characters[iChr];
		if (!IsEntity(ch)) { continue; }
		if (!CheckAttribute(ch, "NetLandLT")) { continue; }
		if (CheckAttribute(ch, "NetLandDead")) { if (sti(ch.NetLandDead)) { continue; } }
		if (NET_LAND_BLOCK_HOLD == 1) { NetLand_PuppetBlockHold(ch, iNow); }
		NetLand_PuppetBlockPose(ch, iNow);
		NetLand_PuppetHoldBlock(ch, iNow);
		NetLand_PuppetMove(ch, iNow);
		NetLand_MushHandTick(ch, iNow);
	}
	NetLand_BotSmooth(iNow);
	NetLand_NpcTick(iNow);		// COOP s35
	NetLand_SlowMeter(iNow);		// COOP s36
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
	if (!bNetLandActive || bNetLandDead || !bNetLandLocReady) { return; }
	// the second copy of the packet (and the result of the same blow that is told twice)
	if (wVictim < 0 || wVictim >= NET_MAXPLAYERS) { return; }
	if (CheckAttribute(&NCClients[wVictim], "LandResAt") && sti(NCClients[wVictim].LandResKind) == iRes)
	{
		if (Net_GetTickCount() - sti(NCClients[wVictim].LandResAt) < 150) { return; }
	}
	NCClients[wVictim].LandResAt = Net_GetTickCount();
	NCClients[wVictim].LandResKind = iRes;
	// what we have shown at the touch of the blade already (NetLand_BladeHit) is not shown again
	int iPred = -1;
	if (CheckAttribute(&NCClients[wVictim], "LandPredAt"))
	{
		int iPredAgo = Net_GetTickCount() - sti(NCClients[wVictim].LandPredAt);
		if (iPredAgo >= 0 && iPredAgo < 1500) { iPred = sti(NCClients[wVictim].LandPred); }
	}
	DeleteAttribute(&NCClients[wVictim], "LandPredAt");
	if (iRes == NET_LANDRES_BLOCK && iPred != 1)
	{
		int iVChr = NetLand_GetPuppet(wVictim);
		if (iVChr >= 0)
		{
			ref chV = &Characters[iVChr];
			if (IsEntity(chV)) { NetLand_PuppetBlockHit(chV); }
		}
	}
	if (iRes == NET_LANDRES_PARRY && iPred != 2)
	{
		NetLand_HeroFightReact(10, 0, "hit_parry", NET_LAND_T_STAGGER);
	}
	if (iRes == NET_LANDRES_FEINT && iPred != 4)
	{
		NetLand_HeroFightReact(9, 0, "hit_feint", NET_LAND_T_STAGGER);
	}
}

void NetLand_SendHitRes(int wAttacker, int iRes)
{
	int iSMsg = NetClient_LandNewMsg(NSC_LAND_HITRES);
	NMAddClientID(iSMsg, wAttacker);
	NMAddByte(iSMsg, iRes);
	NetClient_LandSend(iSMsg, false);
	NetClient_LandSend(iSMsg, false);
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
	if (!bNetLandActive || !bNetLandLocReady) { return; }
	int iChr = NetLand_GetPuppet(wID);
	if (iChr < 0) { return; }
	ref ch = &Characters[iChr];
	if (!IsEntity(ch)) { return; }
	if (CheckAttribute(ch, "NetLandDead")) { if (sti(ch.NetLandDead)) { return; } }
	if (NetLand_PuppetAnimSync(ch))
	{
		// the actions come as they are played; but the same swing again has the same name and was not played again (spam
		// of LMB: the damage came, the swing was not seen). Its start (this packet) plays it again
		if (iAct < 1 || iAct > 5) { return; }
		int iSeqS1 = -1;
		if (CheckAttribute(ch, "NetLandSeq1")) { iSeqS1 = sti(ch.NetLandSeq1); }
		if (iSeq == iSeqS1) { return; }
		ch.NetLandSeq1 = iSeq;
		if (NetLand_PupHitUnacked(ch)) { return; }
		string sSw1 = NetLand_ActName(iAct, 1);
		int iLenSw = strlen(sSw1);
		string sPreSw = strcut(sSw1, 0, iLenSw - 2);		// "attack_fast_" ...
		string sCurS = "";
		if (CheckAttribute(ch, "NetLandAnimCur")) { sCurS = ch.NetLandAnimCur; }
		if (findSubStr(sCurS, sPreSw, 0) != 0) { return; }		// another animation: the new name comes by the stream
		string sSw = sCurS;
		int iAgoS = 0;
		if (CheckAttribute(ch, "NetLandAnimAt")) { iAgoS = Net_GetTickCount() - sti(ch.NetLandAnimAt); }
		if (iAgoS < 250) { return; }		// it has just begun: this is that swing
		ch.NetLandAnimCur = "";
		NetLand_PuppetPlayName(ch, iChr, sSw);
		return;
	}

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
		if (ch.model.animation != "mushketer")
		{
			SetNewModelToChar(ch);
			SendMessage(ch, "lsl", MSG_CHARACTER_EX_MSG, "SetFightWOWeapon", false);
			NetLand_PuppetWeapons(ch);
		}
	}
}

// the game tells that the action asked by the script (CharacterPlayAction) has played to its end
void NetLand_PuppetEndAction(aref chr)
{
	if (!bNetLandActive) { return; }
	ref ch = &Characters[sti(chr.index)];
	// a one-shot animation of the player (a swing, a reaction, a hop) has played once: the puppet does not begin it again while
	// the next name is on the way (the reactions and the swings were a bit longer, their cycle began anew)
	if (NetLand_PuppetAnimSync(ch) && CheckAttribute(ch, "NetLandAnimCur") && CheckAttribute(ch, "NetLandAnimAt"))
	{
		string sCurE = ch.NetLandAnimCur;
		int iAgeE = Net_GetTickCount() - sti(ch.NetLandAnimAt);
		bool bPoseE = false;
		if (CheckAttribute(ch, "NetLandBlkPose")) { if (sti(ch.NetLandBlkPose) == 1) { bPoseE = true; } }
		if (!bPoseE && NetLand_OneShot(sCurE) && iAgeE > 120)
		{
			if (!bNetLandEndActOk) { Trace("COOP: land the game tells the end of the animations of the puppets (" + sCurE + " " + iAgeE + " ms)"); }
			bNetLandEndActOk = true;
			NetLand_LearnAnimDur(sCurE, iAgeE);
			DeleteAttribute(ch, "NetLandClearId");
			CharacterPlayAction(ch, "");
			return;
		}
	}
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
	bool bByPoison = false;
	if (iDeadVarR >= 32) { bByPoison = true; iDeadVarR = iDeadVarR - 32; }

	// the kill line
	string sDead = NCClients[wDead].NickName;
	string sLine = sDead + " " + LanguageConvertString(iLangNetClient, "Net_LandDied");
	bool bKillerOk = false;
	if (wKiller >= 0 && wKiller < NET_MAXPLAYERS && wKiller != wDead) { bKillerOk = true; }
	if (NetLand_IsLandBot(wKiller)) { bKillerOk = true; }
	if (bKillerOk)
	{
		string sKiller = NCClients[wKiller].NickName;
		sLine = sKiller + " " + LanguageConvertString(iLangNetClient, "Net_LandKilled") + " " + sDead + " (" + iKills + ")";
	}
	if (bByPoison) { sLine = sLine + " - яд"; }
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
		if (CheckAttribute(ch, "NetLandBlkPose")) { if (sti(ch.NetLandBlkPose) == 1) { NetLand_PuppetPoseOff(ch); } }
		NetLand_PuppetHoldOff(ch, false);
		// the death task of the game: the body falls once and stays lying (the played action "death_1" repeated itself)
		if (iDeadVarR < 1) { iDeadVarR = 1; }
		NetLand_SetDeathVariant(ch, iDeadVarR);
		SetCharacterTask_Dead(ch);
	}
}

// the hero that has died by the death task is brought back by loading the location once more (a new body)
void NetLand_ReloadHero(int iSpawn)
{
	bNetLandLocReady = false;		// the location is loaded again: the packets of the others wait for it
	ref mcR = GetMainCharacter();
	if (bNetLandHoldOk && IsEntity(mcR)) { SendMessage(mcR, "lsl", MSG_CHARACTER_EX_MSG, "SetBlockHold", 2); }
	iNetLandSpawn = iSpawn;
	bNetLandDead = false;
	iNetLandGen++;				// the timers of the old load stop
	// the puppets are taken out of the location (only forgetting them left the old ones standing: a copy of every player),
	// then the location is unloaded as at the end of the duel: the dead body goes too, the next load makes everything new
	NetLand_SwimClearAll();
	for (int i=0; i<NET_MAXPLAYERS; i++)
	{
		int iOldChr = NetLand_PupIdx(i);
		DeleteAttribute(&NCClients[i], "LandChr");
		if (iOldChr >= 0) { NetLand_SwimSet(&Characters[iOldChr], 0); ChangeCharacterAddressGroup(&Characters[iOldChr], "none", "", ""); }
	}
	NetLand_BotClear();
	if (IsEntity(loadedLocation)) { UnloadLocation(loadedLocation); }
	iNetLandPT = 0;
	iNetLandLastFlags = -1;
	iNetLandLastHP = -1;
	// nothing of the old life stays: the keys, the swing, the block, the reactions
	bNetLandBlockHeld = false;
	bNetLandRunHeld = false;
	bNetLandFwdHeld = false;
	bNetLandBackHeld = false;
	bNetLandLeftHeld = false;
	bNetLandRightHeld = false;
	bNetLandFrozen = false;
	iNetLandBlockRelAt = 0;
	iNetLandBlockedAt = 0;
	iNetLandParryUntil = 0;
	iNetLandSwingBusyUntil = 0;
	iNetLandSwingPending = 0;
	iNetLandHopAt = 0;
	iNetLandReactId++;
	iNetLandStunUntil = 0;
	NetLand_LockFight(0);
	iNetLandProtectUntil = Net_GetTickCount() + 20000;	// nobody hurts the new body while the location loads
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
		NetLand_PoisonOff();
		if (bNetLandDeadTask)
		{
			// the body that the game has killed does not get up: the location is loaded once more, a new body stands at the spawn
			bNetLandDeadTask = false;
			bNetLandRespawnLoad = true;
			NetLand_ReloadHero(iSpawn);
			return;
		}
		ref mc = GetMainCharacter();
		LAi_SetHP(mc, LAi_GetCharacterMaxHP(mc), LAi_GetCharacterMaxHP(mc));
		bNetLandFrozen = false;
		NetLand_AnimSpeed(mc, 0, 100);
		NetLand_AnimSpeed(mc, 1, 100);
		NetLand_AnimSpeed(mc, 2, 100);
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
		NetLand_PuppetHoldOff(ch, false);
		DeleteAttribute(&NCClients[wID], "LandChr");
		NetLand_SwimSet(ch, 0);
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
	if (wID >= 0 && wID < NET_MAXPLAYERS) { NetLand_BotsForget(wID); }
	if (!bNetLandActive) { return; }
	int iChr = NetLand_PupIdx(wID);
	DeleteAttribute(&NCClients[wID], "LandChr");
	if (iChr >= 0)
	{
		NetLand_PuppetHoldOff(&Characters[iChr], false);
		NetLand_SwimSet(&Characters[iChr], 0);
		ChangeCharacterAddressGroup(&Characters[iChr], "none", "", "");
	}
}

// ================================================================================================
// COOP: land duel loadout ("Снаряжение" in the Esc menu): blade, pistol, charges, potions, level, PIRATES,
// personal skills and perks, paid from the money of the server (credit / 20). The other players get it too: their
// puppets hold the same weapons and have the same defence (the damage is counted on the machine of the attacker).
// ================================================================================================
object oNetLandLo;					// our loadout
object oNetLandLoDefault;			// a player who has not sent his loadout yet

#define NET_LAND_PERKS			13
#define NET_LAND_MUSH2X2_PRICE	25000	// COOP s34: the double-barrelled musket in the loadout (the musket: 10000)
#define NET_LAND_ITEM_MULT		1		// COOP s29: 4 -> 1. the price of an item in the loadout = its single player price * this
#define NET_LAND_LEVEL_K		20		// the level L costs NET_LAND_LEVEL_K * (L - 1) * L in all
										// (COOP s29: the dearest loadout: level 30 17 400, toporAZ 17 500, pistol7 25 000, 100 bullets 1 200,
										// 5 of every healing item 11 900, the poison 5 000, cirass5 30 000 = 108 000; see common/Perks.c)
#define NET_LAND_LO_ROWS		22		// rows of the loadout window: 0 blade, 1 gun, 2 ammo, 3..8 healing, 9 level, 10..14 skills, 23 armor,
										// 15..21 P.I.R.A.T.E.S (NetMenu.ini, net\interfaces\NetMenu.c)
#define NET_LAND_POTS			7		// kinds of healing items (the 7th, the antidote, is row 22 of the loadout window)
#define NET_LAND_POT_MAX		10		// of each kind

// ================================================================================================
// COOP: the armor of the loadout (row 23 of the window): the cuirasses of single player lower the damage like there
// (LAi_ApplyCharacterAttackDamage: blade damage * (1 - CirassLevel), no critical hit; LAi_ApplyCharacterFireDamage: a shot
// stops with the chance CirassLevel / 2). The suits change nothing in the fight. The model changes only where the skin has one
// (the heroes of single player: Blad_N / Resc_N / Pat_N / Port_N, N = the "model" of the item, HeroDescribe.txt; Hero1 /
// Blood5 / Espinosa have 4 cuirasses: _Cirass, _Cirass1.._Cirass3). The damage is counted on the machine of the attacker
// with the cirassId of the puppet: it comes with the loadout
// ================================================================================================
#define NET_LAND_ARMORS		9		// "" and 8 items

string NetLand_ArmorId(int i)
{
	switch (i)
	{
		case 1: return "cirass1"; break;		// 10%
		case 2: return "cirass2"; break;		// 18%
		case 3: return "cirass4"; break;		// 22%
		case 4: return "cirass3"; break;		// 27%
		case 5: return "cirass5"; break;		// 35%
		case 6: return "suit_1"; break;
		case 7: return "suit_2"; break;
		case 8: return "suit_3"; break;
	}
	return "";
}

int NetLand_ArmorNum(string sId)
{
	if (sId == "") { return 0; }
	for (int i=1; i<NET_LAND_ARMORS; i++) { if (NetLand_ArmorId(i) == sId) { return i; } }
	return -1;
}

// the price in the loadout (the cuirass is as dear as a good blade: the damage it takes away is a lot in a duel)
int NetLand_ArmorPrice(string sId)
{
	switch (sId)
	{
		case "cirass1": return 5000; break;
		case "cirass2": return 10000; break;
		case "cirass4": return 15000; break;
		case "cirass3": return 20000; break;
		case "cirass5": return 30000; break;// COOP s29: the prices / 4..5
		case "suit_1": return 1000; break;
		case "suit_2": return 3000; break;
		case "suit_3": return 6000; break;
	}
	return 0;
}

string NetLand_LoArmor(aref lo)
{
	if (!CheckAttribute(lo, "Armor")) { return ""; }
	if (NetLand_ArmorNum(lo.Armor) < 0) { return ""; }
	return lo.Armor;
}

// the protection of the item: 0.0 for a suit
float NetLand_ArmorLevel(string sId)
{
	if (sId == "") { return 0.0; }
	int iIdx = GetItemIndex(sId);
	if (iIdx < 0) { return 0.0; }
	if (CheckAttribute(&Items[iIdx], "Clothes")) { if (sti(Items[iIdx].Clothes)) { return 0.0; } }
	if (!CheckAttribute(&Items[iIdx], "CirassLevel")) { return 0.0; }
	return stf(Items[iIdx].CirassLevel);
}

string NetLand_ArmorNext(string sCur, int iDir)
{
	int iN = NetLand_ArmorNum(sCur);
	if (iN < 0) { iN = 0; }
	iN = iN + iDir;
	if (iN < 0) { iN = NET_LAND_ARMORS - 1; }
	if (iN >= NET_LAND_ARMORS) { iN = 0; }
	return NetLand_ArmorId(iN);
}

// the family of a hero skin that has armor models: "" = none
string NetLand_ArmorFamily(string sSkin)
{
	int iLen = strlen(sSkin);
	if (iLen == 6)
	{
		if (findSubStr(sSkin, "Blad_", 0) == 0) { return "Blad"; }
		if (findSubStr(sSkin, "Resc_", 0) == 0) { return "Resc"; }
		if (findSubStr(sSkin, "Port_", 0) == 0) { return "Port"; }
	}
	if (iLen == 5)
	{
		if (findSubStr(sSkin, "Pat_", 0) == 0) { return "Pat"; }
	}
	if (sSkin == "Hero1" || findSubStr(sSkin, "Hero1_Cirass", 0) == 0) { return "Hero1"; }
	if (sSkin == "Blood5" || findSubStr(sSkin, "Blood5_Cirass", 0) == 0) { return "Blood5"; }
	if (sSkin == "Espinosa" || findSubStr(sSkin, "Espinosa_Cirass", 0) == 0) { return "Espinosa"; }
	return "";
}

// the model of the skin in the armor (the skin itself if it has no such model)
string NetLand_ArmorModel(string sSkin, string sArmor)
{
	if (sArmor == "") { return sSkin; }
	int iA = GetItemIndex(sArmor);
	if (iA < 0) { return sSkin; }
	if (!CheckAttribute(&Items[iA], "model")) { return sSkin; }
	int iK = sti(Items[iA].model);
	string sFam = NetLand_ArmorFamily(sSkin);
	if (sFam == "") { return sSkin; }
	if (sFam == "Blad" || sFam == "Resc" || sFam == "Port" || sFam == "Pat")
	{
		if (iK >= 1 && iK <= 8) { return sFam + "_" + iK; }
		return sSkin;
	}
	if (iK < 1 || iK > 4) { return sSkin; }
	string sVar = sFam + "_Cirass";
	if (iK > 1) { sVar = sVar + (iK - 1); }
	return sVar;
}

// the cuirass of a character for the damage (single player: cirassId)
void NetLand_ApplyArmor(ref chr, aref lo)
{
	string sArm = NetLand_LoArmor(lo);
	if (NetLand_ArmorLevel(sArm) > 0.0) { chr.cirassId = GetItemIndex(sArm); }
	else { DeleteAttribute(chr, "cirassId"); }
}

float NetLand_ArmorOf(aref chr)
{
	if (!CheckAttribute(chr, "cirassId")) { return 0.0; }
	int iCi = sti(chr.cirassId);
	if (iCi < 0) { return 0.0; }
	if (!CheckAttribute(&Items[iCi], "CirassLevel")) { return 0.0; }
	return stf(Items[iCi].CirassLevel);
}

// single player: a shot stops on the cuirass with the chance CirassLevel / 2
bool NetLand_ArmorStopsShot(aref enemy)
{
	float fLv = NetLand_ArmorOf(enemy);
	if (fLv <= 0.0) { return false; }
	if (rand(1000) < fLv * 500) { return true; }
	return false;
}

string NetLand_LoArmorText()
{
	aref lo;
	makearef(lo, oNetLandLo);
	string sArm = NetLand_LoArmor(lo);
	string sT = "Броня: " + NetLand_ItemTitle(sArm);
	if (sArm == "") { return sT; }
	sT = sT + " (" + NetLand_ArmorPrice(sArm) + ")";
	float fLv = NetLand_ArmorLevel(sArm);
	if (fLv > 0.0) { sT = sT + "\nурон -" + makeint(fLv * 100.0 + 0.5) + "%"; }
	else { sT = sT + "\nтолько внешний вид"; }
	// the skin of the player has no model with it: only the protection
	string sSkinA = "";
	if (CheckAttribute(&NetClient, "FaceImage")) { sSkinA = NetLand_FaceModel(NetClient.FaceImage); }
	if (sSkinA == "") { return sT; }
	if (NetLand_ArmorModel(sSkinA, sArm) == sSkinA) { sT = sT + ", на скине не видна"; }
	return sT;
}

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
	for (i=0; i<NET_LAND_POTS; i++) { string sQ = "q" + i; lo.Pot.(sQ) = 0; }
	lo.Pot.q1 = 3;
	lo.Level = 1;
	for (i=0; i<7; i++) { string sS = "s" + i; lo.SP.(sS) = 6; }
	for (i=0; i<5; i++) { string sK = "k" + i; lo.Sk.(sK) = 10; }
	for (i=0; i<NET_LAND_PERKS; i++) { string sP = "p" + i; lo.Perk.(sP) = 0; }
	lo.Poison = 0;
	lo.Armor = "";		// COOP: the cuirass / suit
	lo.Inited = true;
}

void NetLand_LoInit()
{
	aref lo;
	makearef(lo, oNetLandLo);
	if (!CheckAttribute(lo, "Inited"))
	{
		NetLand_LoDefaults(lo);
		if (NetLand_ProfLoadLo(lo)) { Trace("COOP: the loadout of the profile " + NetLand_ProfNick()); }
		if (!CheckAttribute(lo, "Inited")) { NetLand_LoDefaults(lo); }
	}
	if (CheckAttribute(&NCServer, "Credit") || iNetLandBudgetOverride > 0)
	{
		if (NetLand_LoCost(lo) > NetLand_LoBudget())
		{
			Trace("COOP: the loadout costs more than this server gives: the defaults here (the profile keeps it)");
			NetLand_LoDefaults(lo);
		}
	}
	// a loadout of the old version (one kind of potion)
	if (!CheckAttribute(lo, "Pot.q0"))
	{
		for (int iq=0; iq<NET_LAND_POTS; iq++) { string sQo = "q" + iq; lo.Pot.(sQo) = 0; }
		lo.Pot.q1 = 3;
	}
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
	if (sId == "mushket2x2") { return NET_LAND_MUSH2X2_PRICE * NET_LAND_ITEM_MULT; }		// COOP s34: 100000 in the single player
	return sti(Items[iIdx].price) * NET_LAND_ITEM_MULT;
}

int NetLand_LevelCost(int iLevel)
{
	if (iLevel < 1) { iLevel = 1; }
	return NET_LAND_LEVEL_K * (iLevel - 1) * iLevel;
}

// the healing items of single player, the weakest first (healing 25, 40, 50, 130, 150, 175)
string NetLand_PotId(int k)
{
	switch (k)
	{
		case 0: return "potion5"; break;		// grapes
		case 1: return "potion1"; break;		// healing potion
		case 2: return "potionrum"; break;		// rum
		case 3: return "potion4"; break;		// mixture
		case 4: return "potion2"; break;		// elixir
		case 5: return "potionwine"; break;		// wine
		case 6: return "potion3"; break;		// antidote
	}
	return "potion1";
}

int NetLand_LoPotQ(aref lo, int k)
{
	string sQ = "q" + k;
	if (!CheckAttribute(lo, "Pot." + sQ)) { return 0; }
	return sti(lo.Pot.(sQ));
}

int NetLand_LoBudget()
{
	if (iNetLandBudgetOverride > 0) { return iNetLandBudgetOverride; }
	if (!CheckAttribute(&NCServer, "Credit")) { return 300000; }
	return sti(NCServer.Credit);
}

int NetLand_LoCost(aref lo)
{
	int iCost = NetLand_ItemPrice(lo.Blade) + NetLand_ItemPrice(lo.Gun);
	if (lo.Gun != "") { iCost = iCost + sti(lo.Ammo) * 10 * (NetLand_ItemPrice("bullet") + NetLand_ItemPrice("GunPowder")); }
	for (int k=0; k<NET_LAND_POTS; k++) { iCost = iCost + NetLand_LoPotQ(lo, k) * NetLand_ItemPrice(NetLand_PotId(k)); }
	iCost = iCost + NetLand_LevelCost(sti(lo.Level));
	if (CheckAttribute(lo, "Poison")) { if (sti(lo.Poison) == 1) { iCost = iCost + NET_LAND_POISON_PRICE; } }
	iCost = iCost + NetLand_ArmorPrice(NetLand_LoArmor(lo));		// COOP: the armor
	return iCost;
}

// ---------- points ----------
int NetLand_LoSpecialSum(aref lo)
{
	int iSum = 0;
	for (int i=0; i<7; i++) { string sS = "s" + i; iSum = iSum + sti(lo.SP.(sS)); }
	return iSum;
}

// single player (RPGUtilite.c): a level comes with 35 - Intellect points of the skills (GetCharacterRankRate), a point of the
// personal abilities with 40 - Intellect points of the personal skills (GetFreePoints_SelfRate)
int NetLand_LoSkillPerLevel(aref lo)
{
	int iRate = 35 - sti(lo.SP.s4);
	if (iRate < 1) { iRate = 1; }
	return iRate;
}

int NetLand_LoSkillUsed(aref lo)
{
	int iUsed = 0;
	for (int i=0; i<5; i++) { string sK = "k" + i; iUsed = iUsed + sti(lo.Sk.(sK)) - 10; }
	return iUsed;
}

int NetLand_LoSkillFree(aref lo)
{
	return (sti(lo.Level) - 1) * NetLand_LoSkillPerLevel(lo) - NetLand_LoSkillUsed(lo);
}

int NetLand_LoPerkRate(aref lo)
{
	int iRate = 40 - sti(lo.SP.s4);
	if (iRate < 1) { iRate = 1; }
	return iRate;
}

int NetLand_LoPerkFree(aref lo)
{
	int iUsed = 0;
	for (int i=0; i<NET_LAND_PERKS; i++) { string sP = "p" + i; iUsed = iUsed + sti(lo.Perk.(sP)); }
	return NetLand_LoSkillUsed(lo) / NetLand_LoPerkRate(lo) - iUsed;
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
		case 2: return "Пули и порох: " + (sti(lo.Ammo) * 10) + " (" + (sti(lo.Ammo) * 10 * (NetLand_ItemPrice("bullet") + NetLand_ItemPrice("GunPowder"))) + ")"; break;
		case 23: return NetLand_LoArmorText(); break;
		case 9: return "Уровень " + lo.Level + " (+1 = " + (NetLand_LevelCost(sti(lo.Level) + 1) - NetLand_LevelCost(sti(lo.Level))) + ")"; break;
	}
	if (iRow >= 3 && iRow < 9) { return NetLand_ItemTitle(NetLand_PotId(iRow - 3)) + ": " + NetLand_LoPotQ(lo, iRow - 3); }
	if (iRow >= 10 && iRow < 15)
	{
		string sK = "k" + (iRow - 10);
		return NetLand_SkillTitle(iRow - 10) + ": " + lo.Sk.(sK);
	}
	if (iRow >= 15 && iRow < 22)
	{
		string sS = "s" + (iRow - 15);
		return NetLand_SpecialTitle(iRow - 15) + ": " + lo.SP.(sS);
	}
	return "";
}

// for the window (net\interfaces\NetMenu.c)
string NetLand_LoBladeId()
{
	if (!CheckAttribute(&oNetLandLo, "Blade")) { return ""; }
	return oNetLandLo.Blade;
}

string NetLand_LoGunId()
{
	if (iNetLandNoPistol) { return ""; }
	if (!CheckAttribute(&oNetLandLo, "Gun")) { return ""; }
	return oNetLandLo.Gun;
}

bool NetLand_LoHasPerkN(int i)
{
	aref lo;
	makearef(lo, oNetLandLo);
	return NetLand_LoHasPerk(lo, i);
}

// the value of a row for the tables of the window
int NetLand_LoRowValue(int iRow)
{
	aref lo;
	makearef(lo, oNetLandLo);
	if (iRow >= 3 && iRow < 9) { return NetLand_LoPotQ(lo, iRow - 3); }
	if (iRow == 22) { return NetLand_LoPotQ(lo, 6); }
	if (iRow >= 10 && iRow < 15) { string sK = "k" + (iRow - 10); return sti(lo.Sk.(sK)); }
	if (iRow >= 15 && iRow < 22) { string sS = "s" + (iRow - 15); return sti(lo.SP.(sS)); }
	return 0;
}

string NetLand_LoInfoText()
{
	aref lo;
	makearef(lo, oNetLandLo);
	int iLeft = NetLand_LoBudget() - NetLand_LoCost(lo);
	string sText = "Деньги: " + iLeft + " из " + NetLand_LoBudget();
	sText = sText + "\nОчки P.I.R.A.T.E.S: " + (42 - NetLand_LoSpecialSum(lo));
	sText = sText + "\nОчки навыков: " + NetLand_LoSkillFree(lo) + " (" + NetLand_LoSkillPerLevel(lo) + " за уровень)";
	if (!NetLand_LoChangeAllowed()) { sText = sText + "\nСмена снаряжения: только с респавном (или в первые 30 с)"; }
	if (NetLand_PerksChosen()) { sText = sText + "\nОчки умений: " + NetLand_LoPerkFree(lo) + " (1 за " + NetLand_LoPerkRate(lo) + " очков навыков)"; }
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
	string sName = "";
	if (wClientID >= 0 && wClientID < NET_MAXPLAYERS) { if (CheckAttribute(&NCClients[wClientID], "NickName")) { sName = NCClients[wClientID].NickName; } }
	return sName + "\nУровень " + lo.Level + "\nЗдоровье: " + makeint(NetLand_MaxHP(&oTmp, lo)) + "\nЭнергия: " + iEnergy;
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
		case 9:
			lo.Level = sti(lo.Level) + iDir;
			if (sti(lo.Level) < 1) { lo.Level = 1; }
			if (sti(lo.Level) > 30) { lo.Level = 30; }
		break;
		case 23: lo.Armor = NetLand_ArmorNext(NetLand_LoArmor(lo), iDir); break;		// COOP: the armor
	}
	int iPotK = -1;
	if (iRow >= 3 && iRow < 9) { iPotK = iRow - 3; }
	if (iRow == 22) { iPotK = 6; }
	if (iPotK >= 0)
	{
		string sQ = "q" + iPotK;
		int iQ = NetLand_LoPotQ(lo, iPotK) + iDir;
		if (iQ < 0) { iQ = 0; }
		if (iQ > NET_LAND_POT_MAX) { iQ = NET_LAND_POT_MAX; }
		lo.Pot.(sQ) = iQ;
	}
	if (iRow >= 10 && iRow < 15)
	{
		string sK = "k" + (iRow - 10);
		// 5 at a time; the last free points go as they are
		int iStepK = 5;
		if (iDir > 0)
		{
			int iFreeK = NetLand_LoSkillFree(lo);
			if (iFreeK > 0 && iFreeK < iStepK) { iStepK = iFreeK; }
		}
		lo.Sk.(sK) = sti(lo.Sk.(sK)) + iDir * iStepK;
		if (sti(lo.Sk.(sK)) < 10) { lo.Sk.(sK) = 10; }
		if (sti(lo.Sk.(sK)) > 100) { lo.Sk.(sK) = 100; }
	}
	if (iRow >= 15 && iRow < 22)
	{
		string sS = "s" + (iRow - 15);
		lo.SP.(sS) = sti(lo.SP.(sS)) + iDir;
		if (sti(lo.SP.(sS)) < 1) { lo.SP.(sS) = 1; }
		if (sti(lo.SP.(sS)) > SPECIAL_MAX) { lo.SP.(sS) = SPECIAL_MAX; }
	}

	// a level less: the skills and perks above it go away
	int iGuard = 0;
	int k = 4;
	while (NetLand_LoSkillFree(lo) < 0 && iGuard < 3000)
	{
		iGuard++;
		string sKk = "k" + k;
		if (sti(lo.Sk.(sKk)) > 10)
		{
			lo.Sk.(sKk) = sti(lo.Sk.(sKk)) - 1;
		}
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
	if (CheckAttribute(lo, "Gun") && !iNetLandNoPistol) { if (NetLand_IsMushItem(lo.Gun)) { bMusket = true; } }
	// a musket: the skin of the player stays, the animation is the one of the musketeer (the musket needs it, see
	// EquipCharacterByItem); without a musket the animation of the skin comes back
	string sCurAnim = "";
	if (CheckAttribute(chr, "model.animation")) { sCurAnim = chr.model.animation; }
	// single player (SetMainCharacterToMushketer): the model of a hero with a musket is "<model>_mush"
	string sBaseModel = NetLand_SkinOf(chr);
	chr.NetLandSkin = sBaseModel;
	string sDressModel = NetLand_ArmorModel(sBaseModel, NetLand_LoArmor(lo));		// COOP: the cuirass / suit of a hero skin
	string sWantModel = sDressModel;
	string sWantAnim = NetLand_ModelAnim(sBaseModel);
	if (bMusket) { sWantModel = NetLand_MushModel(sDressModel, NetLand_ChrPlayer(chr)); sWantAnim = "mushketer"; }
	else { sWantModel = NetLand_BladeModel(sDressModel); }		// COOP s35: a musketeer skin with a blade and a pistol
	// COOP s30: the musket of a skin that is not a man: its own animation file
	string sWantFile = "";
	if (bMusket) { sWantFile = NetLand_MushAnimFile(sBaseModel); }
	string sCurFile = "";
	if (CheckAttribute(chr, "model.animfile")) { sCurFile = chr.model.animfile; }
	if (sCurFile != sWantFile)
	{
		NetLand_SetAnimFile(chr, sWantFile);
		sCurAnim = "";		// the model is sent again below
	}
	chr.NetLandMushTie = 0;
	if (bMusket && sWantModel == sDressModel && !NetLand_IsMushSkin(sBaseModel)) { chr.NetLandMushTie = 1; }
	if (sti(chr.NetLandMushTie) != 1) { NetLand_MushUntie(chr); }
	if (chr.model != sWantModel || sCurAnim != sWantAnim)
	{
		chr.model = sWantModel;
		chr.model.animation = sWantAnim;
		chr.model.height = NetLand_ModelHeight(sBaseModel);
		if (IsEntity(chr)) { Characters_RefreshModel(chr); }	// before the equipment: the new model has to exist for the musket
	}
	if (bMusket)
	{
		// the musketeer has no sword in the hand (single player: SetMainCharacterToMushketer)
		GiveItem2Character(chr, "unarmed");
		EquipCharacterByItem(chr, "unarmed");
	}
	else
	{
		GiveItem2Character(chr, lo.Blade);
		EquipCharacterByItem(chr, lo.Blade);
	}
	string sGun = lo.Gun;
	if (bMusket && !NetLand_IsMushItem(sGun)) { sGun = "mushket"; }		// COOP s32: the musketeer skin
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
		// the equip messages of the old model are lost when the model is made anew: the musket is put on once more
		if (bMusket)
		{
			PostEvent("NetLand_ReEquipGun", 400, "ls", sti(chr.index), sGun);
			PostEvent("NetLand_ReEquipGun", 1500, "ls", sti(chr.index), sGun);
		}
	}
	NetLand_MushHand(chr);
	NetLand_ApplyArmor(chr, lo);		// COOP: cirassId (the damage of the blows and shots at this character)
	if (bHero)
	{
		for (int iPq=0; iPq<NET_LAND_POTS; iPq++)
		{
			int iPn = NetLand_LoPotQ(lo, iPq);
			if (iPn > 0) { TakeNItems(chr, NetLand_PotId(iPq), iPn); }
		}
	}

	float fMax = NetLand_MaxHP(chr, lo);
	float fCur = fMax;
	if (!bFull)
	{
		if (CheckAttribute(chr, "chr_ai.hp") && IsEntity(chr) && stf(chr.chr_ai.hp) < fMax) { fCur = stf(chr.chr_ai.hp); }
	}
	LAi_SetHP(chr, fCur, fMax);
	SetEnergyToCharacter(chr);
}

// the musket in the hand of a skin without a twin, every 100 ms
void NetLand_MushHandTick(ref chr, int iNow)
{
	if (!CheckAttribute(chr, "NetLandMushTie")) { return; }
	if (sti(chr.NetLandMushTie) != 1) { return; }
	if (CheckAttribute(chr, "NetLandMushAt"))
	{
		int iAgoM = iNow - sti(chr.NetLandMushAt);
		if (iAgoM >= 0 && iAgoM < 100) { return; }
	}
	chr.NetLandMushAt = iNow;
	NetLand_MushHand(chr);
}

void NetLand_ReEquipGun()
{
	int iIdx = GetEventData();
	string sGunID = GetEventData();
	if (!bNetLandActive || iIdx < 0) { return; }
	ref chr = &Characters[iIdx];
	if (!IsEntity(chr)) { return; }
	if (!CheckAttribute(chr, "model.animation") || chr.model.animation != "mushketer") { return; }
	if (!CheckCharacterItem(chr, sGunID)) { return; }
	RemoveCharacterEquip(chr, GUN_ITEM_TYPE);
	EquipCharacterByItem(chr, sGunID);
	NetLand_MushHand(chr);
	int iQ = 1;
	int iGunIdx = GetItemIndex(sGunID);
	if (iGunIdx >= 0)
	{
		if (CheckAttribute(&Items[iGunIdx], "chargeQ")) { iQ = sti(Items[iGunIdx].chargeQ); }
	}
	LAi_GunSetChargeQuant(chr, iQ);
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
		if (NetLand_SkinOf(mc) != sNewModel)
		{
			mc.model = sNewModel;
			mc.model.animation = NetLand_ModelAnim(sNewModel);
			mc.model.height = NetLand_ModelHeight(sNewModel);
			mc.sex = NetLand_ModelSex(sNewModel);
			Characters_RefreshModel(mc);
		}
		// the gun keeps its charge if it is the same gun (the button does not reload it)
		string sOldGun = "";
		if (CheckAttribute(mc, "equip.gun")) { sOldGun = mc.equip.gun; }
		float fOldCharge = -1.0;
		if (CheckAttribute(mc, "chr_ai.charge")) { fOldCharge = stf(mc.chr_ai.charge); }
		NetLand_ApplyLo(mc, lo, true, bUnhurt);
		string sNewGun = "";
		if (CheckAttribute(mc, "equip.gun")) { sNewGun = mc.equip.gun; }
		if (fOldCharge >= 0.0 && sOldGun == sNewGun && sNewGun != "") { mc.chr_ai.charge = fOldCharge; }
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
	int iPoison = 0;
	if (CheckAttribute(lo, "Poison")) { iPoison = sti(lo.Poison); }
	NMAddByte(iSMsg, iPoison);
	NMAddString(iSMsg, NetLand_LoArmor(lo), 12);		// COOP: the armor
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
	lo.Poison = NMGetByte(iMsg);
	lo.Armor = NMGetString(iMsg);		// COOP: the armor
	if (NetLand_ArmorNum(lo.Armor) < 0) { lo.Armor = ""; }
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

	if (!bNetLandActive) { return; }
	int iChr = NetLand_PupIdx(wID);
	if (iChr < 0) { return; }
	// a musket (or a sword again) is another model: the old puppet goes, the next state packet makes a new one
	DeleteAttribute(&NCClients[wID], "LandModel");
	string sPupModel = NetLand_ModelFor(wID);
	int iWantM = 0;
	if (NetLand_ClientMusket(wID)) { iWantM = 1; }
	int iHasM = 0;
	if (CheckAttribute(&Characters[iChr], "NetLandMush")) { iHasM = sti(Characters[iChr].NetLandMush); }
	if (NetLand_SkinOf(&Characters[iChr]) != sPupModel || iWantM != iHasM)
	{
		NetLand_PuppetHoldOff(&Characters[iChr], false);
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
// a musketeer hits with the butt of his musket. The single player gives him the "unarmed" blade (1..15) and a third of the damage
// to the hero: a blow of 0.3 was rounded to nothing (no damage, no hit reaction on the other screen). Here the butt is a heavy
// blade (12..60, the skill of the heavy blades), the other rules of the blow are the game's
bool NetLand_IsMusketeer(aref chr)
{
	if (!CheckAttribute(chr, "model.animation")) { return false; }
	if (chr.model.animation == "mushketer") { return true; }
	return false;
}

float NetLand_MusketMeleeDamage(aref attack, aref enemy, string attackType, bool isBlocked)
{
	float fA = makefloat(GetCharacterSkill(attack, SKILL_F_HEAVY)) / makefloat(SKILL_MAX);
	float fE = LAi_GetCharacterFightLevel(enemy);
	float fDmg = 12.0 + 48.0 * frandSmall(fA);
	if (fA < fE) { fDmg = fDmg * (1.0 + 0.7 * (fA - fE)); }
	float fK = 0.7;
	if (attackType == "force") { fK = 1.0; }
	if (attackType == "round") { fK = 0.6; }
	if (attackType == "break") { fK = 3.0; }
	if (attackType == "feintc") { fK = 0.8; }
	if (attackType == "feint") { fK = 0.5; }
	if (isBlocked)
	{
		fK = 0.0;
		if (attackType == "break") { fK = 1.0; }
	}
	if (fK > 0.0 && CheckCharacterPerk(attack, "HardHitter"))
	{
		if (CheckAttribute(enemy, "chr_ai.energy")) { enemy.chr_ai.energy = stf(enemy.chr_ai.energy) * 0.9; }
	}
	return fDmg * fK;
}

float NetLand_BladeDamage(aref attack, aref enemy, string attackType, bool isBlocked)
{
	float fDmg = 0.0;
	if (NetLand_IsMusketeer(attack)) { fDmg = NetLand_MusketMeleeDamage(attack, enemy, attackType, isBlocked); }
	else { fDmg = LAi_CalcDamageForBlade(attack, enemy, attackType, isBlocked); }
	float fCritical = 0.0;
	if (!isBlocked)
	{
		if (IsCharacterPerkOn(attack, "SwordplayProfessional")) { if (rand(100) <= 15) { fCritical = 1.0; } }
		else { if (IsCharacterPerkOn(attack, "CriticalHit")) { if (rand(100) <= 5) { fCritical = 1.0; } } }
	}
	if (NetLand_ArmorOf(enemy) > 0.0) { fCritical = 0.0; }		// COOP: single player: no critical hit on a cuirass
	if (fCritical > 0.0 && NetLand_IsOurHero(attack)) { Log_SetStringToLog(XI_ConvertString("Critical Hit")); }
	if (IsCharacterPerkOn(attack, "Rush")) { fDmg = fDmg * 3.0; }
	if (IsCharacterPerkOn(enemy, "Rush")) { fDmg = fDmg * 0.5; }
	float fDef = 1.0;
	if (IsCharacterPerkOn(enemy, "BasicDefense")) { fDef = 0.9; }
	if (IsCharacterPerkOn(enemy, "AdvancedDefense")) { fDef = 0.8; }
	if (IsCharacterPerkOn(enemy, "SwordplayProfessional")) { fDef = 0.7; }
	return fDmg * fDef * (1.0 + fCritical) * (1.0 - NetLand_ArmorOf(enemy));		// COOP: the cuirass of the victim
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
	// COOP s27: the quest decks have the picture of the open sea of a quest, the other decks of their kind "sea"
	if (sLoc == "Ship_deck" || sLoc == "Deck_Near_Ship") { return "loading\sea.tga"; }
	if (sLoc == "Clone_location") { return "loading\towns\pearl1.tga"; }		// COOP s27: the farm: the picture of the pirate settlement
	if (NetTdm_IsKey(sLoc)) { return NetTdm_Image(sLoc); }		// COOP s27: the locations of TDM
	int iLocIdx = FindLocation(sLoc);
	if (iLocIdx >= 0)
	{
		if (CheckAttribute(&Locations[iLocIdx], "image")) { return Locations[iLocIdx].image; }
	}
	return "loading\\Town.tga";
}

string NetLand_LocName(string sLoc)
{
	if (sLoc == "Clone_location") { return "Ферма"; }		// COOP s27: the template location of the single player (the farm field)
	if (NetTdm_IsKey(sLoc)) { return NetTdm_Name(sLoc); }		// COOP s27: the locations of TDM
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


// ================================================================================================
// ADMIN MENU in a land duel (the same buttons as at sea: server\Admin.c, server\Bots.c send them here)
// ================================================================================================
// server: true = the command is handled here (a land duel has no ships)
bool NetServer_LandAdmin(int wAdmin, int iAction, int wTarget, float fValue)
{
	if (!NetServer_LandMode()) { return false; }
	if (NetServer_LandBotAdmin(wAdmin, iAction, wTarget, fValue)) { return true; }
	switch (iAction)
	{
		case NET_ADMIN_HEAL:
			NetServer_LandHealOrRespawn(wTarget);
			return true;
		break;
		case NET_ADMIN_ALL_HEAL:
			for (int i=0; i<NET_MAXPLAYERS; i++) { NetServer_LandHealOrRespawn(i); }
			return true;
		break;
		case NET_ADMIN_SPEED:
			NetServer_LandSpeed(fValue);
			return true;
		break;
		case NET_ADMIN_DESTROY:
			NetServer_LandAdminTo(wTarget, NET_LANDADM_KILL, DST_INVALID, 0);
			return true;
		break;
		case NET_ADMIN_TP_TO:
			NetServer_LandAdminTo(wAdmin, NET_LANDADM_TPTO, wTarget, 0);
			return true;
		break;
		case NET_ADMIN_TP_BRING:
			NetServer_LandAdminTo(wTarget, NET_LANDADM_TPTO, wAdmin, 0);
			return true;
		break;
		case NET_ADMIN_BOT_DELETE:   return true; break;
		case NET_ADMIN_BOT_DESTROY:  return true; break;
		// these have no meaning in a land duel, but the command is "handled" (swallowed)
		case NET_ADMIN_RELOAD_SHIPS: return true; break;
		case NET_ADMIN_BOT_REL:      return true; break;
		case NET_ADMIN_BOT_BRING:    return true; break;
		case NET_ADMIN_BOT_GOTO:     return true; break;
		case NET_ADMIN_BOT_HEAL:     return true; break;
		// NET_ADMIN_BOT_RESPAWN: the switch of the server (server\Bots.c), the owners of the land bots use it
	}
	return false;
}

// wTo: a player or DST_ALL
void NetServer_LandAdminTo(int wTo, int iKind, int wWho, int iValue)
{
	if (wTo != DST_ALL)
	{
		if (wTo < 0 || wTo >= NET_MAXPLAYERS) { return; }
		if (!sti(NSClients[wTo].Use) || !sti(NSClients[wTo].InGame)) { return; }
		if (sti(NSClients[wTo].LandDead)) { return; }
	}
	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_LAND);
	NMAddByte(iSMsg, NSC_LAND_ADMIN_R);
	NMAddByte(iSMsg, iKind);
	NMAddClientID(iSMsg, wWho);
	NMAddByte(iSMsg, iValue);
	NetServer_SendMessage(wTo, iSMsg, true);
	NMDelete(iSMsg);
}


// client
void NetLand_OnAdmin(int iMsg)
{
	int iKind = NMGetByte(iMsg);
	int wWho = NMGetClientID(iMsg);
	int iValue = NMGetByte(iMsg);
	if (iKind == NET_LANDADM_SPEED) { NetLand_SetSpeed(iValue); return; }
	if (!bNetLandActive) { return; }
	ref mc = GetMainCharacter();

	if (iKind == NET_LANDADM_BOTADD) { NetLand_BotAdd(iValue); return; }
	if (iKind == NET_LANDADM_BOTCLEAR) { NetLand_BotClear(); return; }
	if (iKind == NET_LANDADM_BOTCMD) { NetLand_BotCmd(wWho, iValue); return; }
	if (bNetLandDead || !IsEntity(mc)) { return; }

	if (iKind == NET_LANDADM_HEAL)
	{
		LAi_SetHP(mc, LAi_GetCharacterMaxHP(mc), LAi_GetCharacterMaxHP(mc));
		Log_SetStringToLog("Администратор вас вылечил");
	}
	if (iKind == NET_LANDADM_KILL)
	{
		Log_SetStringToLog("Администратор вас убил");
		NetLand_LocalDie(DST_INVALID);
	}
	if (iKind == NET_LANDADM_TPTO)
	{
		Trace("COOP: land admin teleports our hero to player " + wWho);
		if (wWho == wClientID) { return; }
		int iChr = NetLand_GetPuppet(wWho);
		if (iChr < 0) { Trace("COOP: land admin teleport: no puppet of " + wWho); return; }
		float fPX = 0.0;
		float fPY = 0.0;
		float fPZ = 0.0;
		float fPAy = 0.0;
		bool bPos = false;
		// where the player is by his last state packet (the puppet may not be on its place yet, or not in the location)
		if (CheckAttribute(&Characters[iChr], "NetLandLX"))
		{
			fPX = stf(Characters[iChr].NetLandLX);
			fPY = stf(Characters[iChr].NetLandLY);
			fPZ = stf(Characters[iChr].NetLandLZ);
			fPAy = stf(Characters[iChr].NetLandLAy);
			bPos = true;
		}
		if (!bPos && IsEntity(&Characters[iChr]))
		{
			if (GetCharacterPos(&Characters[iChr], &fPX, &fPY, &fPZ)) { bPos = true; }
			GetCharacterAy(&Characters[iChr], &fPAy);
		}
		if (!bPos) { Trace("COOP: land admin teleport: the place of " + wWho + " is not known"); return; }
		// a step to the side of him, not into him; a point off the floor (a wall, the rail) is refused by the game: the other side,
		// behind him, in front of him, on his very place
		bool bDone = TeleportCharacterToPosAy(mc, fPX + cos(fPAy) * 1.2, fPY, fPZ - sin(fPAy) * 1.2, fPAy);
		if (!bDone) { bDone = TeleportCharacterToPosAy(mc, fPX - cos(fPAy) * 1.2, fPY, fPZ + sin(fPAy) * 1.2, fPAy); }
		if (!bDone) { bDone = TeleportCharacterToPosAy(mc, fPX - sin(fPAy) * 1.2, fPY, fPZ - cos(fPAy) * 1.2, fPAy); }
		if (!bDone) { bDone = TeleportCharacterToPosAy(mc, fPX + sin(fPAy) * 1.2, fPY, fPZ + cos(fPAy) * 1.2, fPAy); }
		if (!bDone) { bDone = TeleportCharacterToPosAy(mc, fPX, fPY, fPZ, fPAy); }
		Trace("COOP: land admin teleport to " + wWho + ": " + bDone);
		// the others see the jump at once (not a run over the whole deck)
		iNetLandLastSend = 0;
		iNetLandPT = 0;
	}
}

// ---------- bots: every machine makes the same bot (the game's own AI runs on each of them) ----------
int iNetLandBotN = 0;

void NetLand_BotAdd(int iRel)
{
	if (!bNetLandActive || !IsEntity(loadedLocation)) { return; }
	string sBotId = "NetLandBot_" + iNetLandBotN;
	iNetLandBotN++;
	int iChr = NPC_GenerateCharacter(sBotId, "pirate_" + (1 + rand(8)), "man", "man", 10, PIRATE, 0, true);
	if (iChr < 0) { return; }
	ref ch = &Characters[iChr];
	ch.NetLandBot = iRel;
	string sGrp = "NET_LAND_BOT_" + iRel;
	LAi_SetWarriorType(ch);
	LAi_CharacterDisableDialog(ch);
	LAi_group_MoveCharacter(ch, sGrp);
	if (iRel == NET_BOTREL_ALLY) { LAi_group_SetRelation(sGrp, LAI_GROUP_PLAYER, LAI_GROUP_FRIEND); }
	if (iRel == NET_BOTREL_NEUTRAL) { LAi_group_SetRelation(sGrp, LAI_GROUP_PLAYER, LAI_GROUP_NEITRAL); }
	if (iRel == NET_BOTREL_ENEMY) { LAi_group_SetRelation(sGrp, LAI_GROUP_PLAYER, LAI_GROUP_ENEMY); }
	ChangeCharacterAddressGroup(ch, sNetLandLoc, sNetLandEntryGroup, sNetLandEntryLocator);
	PostEvent("NetLand_BotPlace", 300, "l", iChr);
}

void NetLand_BotPlace()
{
	int iChr = GetEventData();
	if (!bNetLandActive || iChr < 0) { return; }
	if (!IsEntity(&Characters[iChr])) { return; }
	NetLand_TeleportToSpawn(&Characters[iChr], rand(40));
}

void NetLand_BotClear()
{
	for (int i=0; i<iNetLandBotN; i++)
	{
		int iIdx = GetCharacterIndex("NetLandBot_" + i);
		if (iIdx < 0) { continue; }
		ChangeCharacterAddressGroup(&Characters[iIdx], "none", "", "");
	}
	iNetLandBotN = 0;
}

// ================================================================================================
// COOP: land bots are bot-players. The admin who adds one (its "owner") runs it: the game's own AI on his machine (a warrior).
// The others get its state (place, animation, health) 20 times a second and show a puppet of it. A blow on the puppet goes
// to the owner (the bot takes it there), a blow of the bot on a player goes to that player from the owner (like a player's).
// The bot has a slot of a client (16..65): its name, skin and relation are known to everyone, the menu "Bots" lists it.
// ================================================================================================
#define NET_LAND_BOT_RATE		50		// ms between the state packets of a bot
#define NET_LANDBOT_KILL		1		// owner: the bot dies
#define NET_LANDBOT_HEAL		2		// owner: the bot gets its health back (a dead one stands up again)
#define NET_LANDBOT_BRING		3		// owner: the bot goes to the admin (value / 16 = the admin)

int iNetLandBotGen = 0;
object oNetLandBotLo;		// the loadout of the bot being made (NetLand_BotLo)
int iNetLandBotSkill = 2;		// the difficulty of the bots (the server: Create game "Bots", the admin menu): 0 harmless .. 3 hard
int iNetLandBotRelAt = 0;
int iNetLandBotHitSeq = 0;

bool NetLand_IsLandBot(int wID)
{
	if (wID < NET_BOT_FIRST || wID >= NET_MAXCLIENTS) { return false; }
	if (!CheckAttribute(&NCClients[wID], "LandBot")) { return false; }
	if (sti(NCClients[wID].LandBot) == 0) { return false; }
	if (CheckAttribute(&NCClients[wID], "BotRemoved")) { if (sti(NCClients[wID].BotRemoved)) { return false; } }
	return true;
}

bool NetLand_BotMine(int wID)
{
	if (!NetLand_IsLandBot(wID)) { return false; }
	return sti(NCClients[wID].LandBotOwner) == wClientID;
}

// the next portrait that has a skin (the same icons as the player chooses)
int NetLand_BotNextFace(int iCur, int iDir)
{
	int iFc = iCur;
	for (int k=0; k<400; k++)
	{
		iFc = iFc + iDir;
		if (iFc > 399) { iFc = 0; }
		if (iFc < 0) { iFc = 399; }
		if (NetLand_FaceToModel(iFc) != "") { return iFc; }
	}
	return iCur;
}

string NetLand_BotModel(int wID)
{
	int iFace = 0;
	if (CheckAttribute(&NCClients[wID], "LandBotFace")) { iFace = sti(NCClients[wID].LandBotFace); }
	string sM = NetLand_FaceToModel(iFace);
	if (sM == "") { sM = "pirate_" + (1 + wID - (wID / 8) * 8); }
	return sM;
}

string NetLand_BotGroup(int iRel, bool bMine)
{
	string sG = "NetLandBP";
	if (bMine) { sG = "NetLandBot"; }
	if (iRel == NET_BOTREL_ALLY) { return sG + "A"; }
	if (iRel == NET_BOTREL_NEUTRAL) { return sG + "N"; }
	if (iRel >= NET_BOTREL_TEAM0) { return sG + "T" + (iRel - NET_BOTREL_TEAM0); }		// COOP s28: a bot of a team
	return sG + "E";
}

// ================================================================================================
// COOP s28: the bots of the teams of the team duel (NET_BOTREL_TEAM0 + 0 / 1): they fight the other team (its players and
// bots) and the enemy bots, their deaths and kills go to the score of the teams, they count for the victory of their team.
// A neutral bot that is hit turns into an enemy (the server changes its relation for everybody).
// ================================================================================================
#define NET_LAND_BOTREL_LAST		5		// NET_BOTREL_TEAM0 + 1: the land duel has 2 teams

int NetLand_BotRelOf(int wBot)
{
	if (wBot < NET_BOT_FIRST || wBot >= NET_MAXCLIENTS) { return 0; }
	if (!CheckAttribute(&NCClients[wBot], "BotRel")) { return 0; }
	return sti(NCClients[wBot].BotRel);
}

// the bot does not fight this player: an ally of the players, a bot of his team
bool NetLand_BotFriendOfPlayer(int wBot, int wPlayer)
{
	int iR = NetLand_BotRelOf(wBot);
	if (iR == NET_BOTREL_ALLY) { return true; }
	if (iR < NET_BOTREL_TEAM0) { return false; }
	if (!NetLand_TeamGame()) { return false; }
	if (wPlayer < 0 || wPlayer >= NET_MAXPLAYERS) { return false; }
	return NetLand_TeamOf(wPlayer) == iR - NET_BOTREL_TEAM0;
}

// two relations of bots that fight each other
bool NetLand_BotRelsHostile(int iA, int iB)
{
	if (iA == iB) { return false; }
	if (iA == NET_BOTREL_ALLY && iB == NET_BOTREL_ENEMY) { return true; }
	if (iA == NET_BOTREL_ENEMY && iB == NET_BOTREL_ALLY) { return true; }
	if (iA >= NET_BOTREL_TEAM0 && iB == NET_BOTREL_ENEMY) { return true; }
	if (iA == NET_BOTREL_ENEMY && iB >= NET_BOTREL_TEAM0) { return true; }
	if (iA >= NET_BOTREL_TEAM0 && iB >= NET_BOTREL_TEAM0) { return true; }
	return false;
}

// the bots of relation iA and our hero (and the puppets of our team)
string NetLand_BotRelHero(int iA)
{
	if (iA == NET_BOTREL_ENEMY) { return LAI_GROUP_ENEMY; }
	if (iA == NET_BOTREL_ALLY) { return LAI_GROUP_FRIEND; }
	if (iA >= NET_BOTREL_TEAM0)
	{
		if (NetLand_TeamGame()) { if (iA - NET_BOTREL_TEAM0 == NetLand_TeamOf(wClientID)) { return LAI_GROUP_FRIEND; } }
		return LAI_GROUP_ENEMY;
	}
	return LAI_GROUP_NEITRAL;
}

// the bots of relation iA and the puppets of the other players (a team duel: the other team)
string NetLand_BotRelOthers(int iA)
{
	if (iA >= NET_BOTREL_TEAM0)
	{
		if (NetLand_TeamGame()) { if (iA - NET_BOTREL_TEAM0 == NetLand_TeamOf(wClientID)) { return LAI_GROUP_ENEMY; } }
		return LAI_GROUP_FRIEND;
	}
	return NetLand_BotRelHero(iA);
}

// the owner of a bot tells the server: NET_LANDBOTEVT_HIT (our neutral bot was hit) / NET_LANDBOTEVT_KILLED (and who killed it)
void NetLand_BotEvt(int wBot, int wOther, int iKind)
{
	if (wOther < 0 || wOther >= NET_MAXCLIENTS) { wOther = DST_INVALID; }
	int iSMsg = NetClient_LandNewMsg(NSC_LAND_BOTEVT);
	NMAddClientID(iSMsg, wBot);
	NMAddClientID(iSMsg, wOther);
	NMAddByte(iSMsg, iKind);
	NetClient_LandSend(iSMsg, true);
	NMDelete(iSMsg);
}

// a blow / a shot on our bot: who did it (the kill goes to him), a neutral bot turns into an enemy
void NetLand_BotHitBy(int wBot, ref ch, int wBy)
{
	if (wBy < 0 || wBy >= NET_MAXCLIENTS || wBy == wBot) { return; }
	ch.NetLandBotLastAtt = wBy;
	if (NetLand_BotRelOf(wBot) == NET_BOTREL_NEUTRAL) { NetLand_BotEvt(wBot, wBy, NET_LANDBOTEVT_HIT); }
}

void NetLand_BotRelations()
{
	string sR;
	int iA;
	int iB;
	// our bots against the players, the puppets of the players and the bots of the others
	// (COOP s28: also the bots of the teams: relations 4, 5 = NET_BOTREL_TEAM0 + team)
	for (iA=1; iA<=NET_LAND_BOTREL_LAST; iA++)
	{
		string sMy = NetLand_BotGroup(iA, true);
		sR = NetLand_BotRelHero(iA);
		string sRO = NetLand_BotRelOthers(iA);
		LAi_group_SetRelation(sMy, LAI_GROUP_PLAYER, sR);
		LAi_group_SetRelation(sMy, NET_LAND_GROUP, sRO);
		LAi_group_SetRelation(sMy, NET_LAND_GROUP_ALLY, sR);		// COOP: team duel: the puppets of our team
		// the game turns a relation back when its alarm goes down (the bot walked away and did not see the players any more):
		// the same relation for the alarm and after it
		LAi_group_SetAlarmReaction(sMy, LAI_GROUP_PLAYER, sR, sR);
		LAi_group_SetAlarmReaction(sMy, NET_LAND_GROUP, sRO, sRO);
		LAi_group_SetAlarmReaction(sMy, NET_LAND_GROUP_ALLY, sR, sR);
		LAi_group_SetLookRadius(sMy, 40.0);
		LAi_group_SetHearRadius(sMy, 30.0);
		for (iB=1; iB<=NET_LAND_BOTREL_LAST; iB++)
		{
			string sR2 = LAI_GROUP_NEITRAL;
			if (NetLand_BotRelsHostile(iA, iB)) { sR2 = LAI_GROUP_ENEMY; }
			if (iA == iB) { sR2 = LAI_GROUP_FRIEND; }
			LAi_group_SetRelation(sMy, NetLand_BotGroup(iB, false), sR2);
			LAi_group_SetAlarmReaction(sMy, NetLand_BotGroup(iB, false), sR2, sR2);
			if (iA != iB)
			{
				LAi_group_SetRelation(sMy, NetLand_BotGroup(iB, true), sR2);
				LAi_group_SetAlarmReaction(sMy, NetLand_BotGroup(iB, true), sR2, sR2);
			}
		}
		// the puppets of the bots of the others: no AI, the relation to our hero only (who may hit them)
		string sPup = NetLand_BotGroup(iA, false);
		LAi_group_SetRelation(sPup, LAI_GROUP_PLAYER, sR);
		LAi_group_SetAlarmReaction(sPup, LAI_GROUP_PLAYER, sR, sR);
	}
}

// the character of a bot on this machine: the bot itself (ours) or its puppet
int NetLand_BotChr(int wID)
{
	if (wID < 0 || wID >= NET_MAXCLIENTS) { return -1; }
	if (!CheckAttribute(&NCClients[wID], "LandBotChr")) { return -1; }
	int iB = sti(NCClients[wID].LandBotChr);
	if (iB >= 0 && iB < TOTAL_CHARACTERS)
	{
		if (CheckAttribute(&Characters[iB], "NetLandBotID"))
		{
			if (sti(Characters[iB].NetLandBotID) == wID) { return iB; }
		}
		NetLand_StaleBody(iB);
	}
	Trace("COOP: land the body of bot " + wID + " (character " + iB + ") has been made new by the game");
	DeleteAttribute(&NCClients[wID], "LandBotChr");
	return -1;
}

void NetLand_BotDrop(int wID)
{
	int iChr = NetLand_BotChr(wID);
	DeleteAttribute(&NCClients[wID], "LandBotChr");
	if (iChr < 0) { return; }
	LAi_SetImmortal(&Characters[iChr], true);
	ChangeCharacterAddressGroup(&Characters[iChr], "none", "", "");
}

// the j-th skill a bot learns: the skill of its blade, the pistol, the luck, the other two blades
int NetLand_BotSkillOrder(int iFirst, int j)
{
	if (j == 0) { return iFirst; }
	if (j == 1) { return 3; }
	if (j == 2) { return 4; }
	int iN = 0;
	for (int k=0; k<3; k++)
	{
		if (k == iFirst) { continue; }
		if (iN == j - 3) { return k; }
		iN++;
	}
	return 1;
}

// the loadout of a bot: its level, blade and gun (the admin chose them); its skill points go like the points of a player
// (35 - Intellect a level): the skill of its blade first, then the pistol, the luck and the other blades; a perk for every
// 40 - Intellect of them (a hard bot has all of them)
void NetLand_BotLo(int wID, aref lo)
{
	NetLand_LoDefaults(lo);
	int iLvl = 1;
	if (CheckAttribute(&NCClients[wID], "LandBotLevel")) { iLvl = sti(NCClients[wID].LandBotLevel); }
	if (iLvl < 1) { iLvl = 1; }
	if (iLvl > 50) { iLvl = 50; }
	lo.Level = iLvl;
	if (CheckAttribute(&NCClients[wID], "LandBotBlade")) { if (NCClients[wID].LandBotBlade != "") { lo.Blade = NCClients[wID].LandBotBlade; } }
	lo.Gun = "";
	if (CheckAttribute(&NCClients[wID], "LandBotGun")) { lo.Gun = NCClients[wID].LandBotGun; }
	lo.Ammo = 5;
	int iPts = (iLvl - 1) * (35 - sti(lo.SP.s4));
	int iUsed = iPts;
	// the order: k0 light, k1 medium, k2 heavy blades, k3 pistol, k4 luck
	int iFirst = 1;
	int iBl = GetItemIndex(lo.Blade);
	if (iBl >= 0)
	{
		if (CheckAttribute(&Items[iBl], "FencingType"))
		{
			if (Items[iBl].FencingType == "FencingLight") { iFirst = 0; }
			if (Items[iBl].FencingType == "FencingHeavy") { iFirst = 2; }
		}
	}
	for (int j=0; j<5; j++)
	{
		if (iPts <= 0) { break; }
		string sK = "k" + NetLand_BotSkillOrder(iFirst, j);
		int iCan = 100 - sti(lo.Sk.(sK));
		if (iCan > iPts) { iCan = iPts; }
		lo.Sk.(sK) = sti(lo.Sk.(sK)) + iCan;
		iPts = iPts - iCan;
	}
	int iPerks = (iUsed - iPts) / (40 - sti(lo.SP.s4));
	if (iNetLandBotSkill >= 3) { iPerks = NET_LAND_PERKS; }
	for (int p=0; p<NET_LAND_PERKS; p++)
	{
		if (p >= iPerks) { break; }
		string sP = "p" + p;
		lo.Perk.(sP) = 1;
	}
}

int NetLand_BotMake(int wID)
{
	if (!bNetLandActive || !bNetLandLocReady || !IsEntity(loadedLocation)) { return -1; }
	NetLand_BotDrop(wID);
	bool bMine = NetLand_BotMine(wID);
	string sModel = NetLand_BotModel(wID);
	iNetLandBotGen++;
	string sId = "NetLandBP_" + wID + "_" + iNetLandBotGen;
	if (bMine) { sId = "NetLandBot_" + wID + "_" + iNetLandBotGen; }
	int iChr = NPC_GenerateCharacter(sId, sModel, NetLand_ModelSex(sModel), NetLand_ModelAnim(sModel), 10, PIRATE, 0, true);
	if (iChr < 0) { return -1; }
	ref ch = &Characters[iChr];
	ch.name = NCClients[wID].NickName;
	ch.lastname = "";
	ch.NetLandSkin = sModel;
	ch.NetLandBotID = wID;
	ch.RebirthPhantom = true;		// its dead body is not made new by the game while it lies in the location
	ch.NetLandBotDead = 0;
	DeleteAttribute(ch, "NetLandBotLastAtt");		// COOP s28
	ch.NetLandBotDiff = iNetLandBotSkill;
	aref lo;
	makearef(lo, oNetLandBotLo);
	NetLand_BotLo(wID, lo);
	NetLand_ApplyLo(ch, lo, false, true);
	// the healing of the medium and the hard bots
	ch.NetLandBotPots = 0;
	if (iNetLandBotSkill == 2) { ch.NetLandBotPots = 3; }
	if (iNetLandBotSkill >= 3) { ch.NetLandBotPots = 5; }
	LAi_CharacterDisableDialog(ch);
	int iRel = sti(NCClients[wID].BotRel);
	if (bMine)
	{
		ch.NetLandBotMine = 1;
		// the harmless bot stands (single player: a citizen); the others fight
		if (iNetLandBotSkill <= 0) { LAi_SetStayType(ch); }
		else { LAi_SetWarriorType(ch); }
		LAi_group_MoveCharacter(ch, NetLand_BotGroup(iRel, true));
	}
	else
	{
		ch.NetLandBotPup = 1;		// no AI of its own (Loc_ai\LAi_fightparams.c): its owner fights with it
		LAi_SetImmortal(ch, true);
		LAi_SetActorType(ch);
		SendMessage(ch, "lsl", MSG_CHARACTER_EX_MSG, "SetFightWOWeapon", false);
		LAi_group_MoveCharacter(ch, NetLand_BotGroup(iRel, false));
	}
	NetLand_BotRelations();
	ChangeCharacterAddressGroup(ch, sNetLandLoc, sNetLandEntryGroup, sNetLandEntryLocator);
	if (bMine)
	{
		if (CheckAttribute(&NCClients[wID], "LandBotKeepX"))
		{
			TeleportCharacterToPosAy(ch, stf(NCClients[wID].LandBotKeepX), stf(NCClients[wID].LandBotKeepY), stf(NCClients[wID].LandBotKeepZ), stf(NCClients[wID].LandBotKeepAy));
			DeleteAttribute(&NCClients[wID], "LandBotKeepX");
			Trace("COOP: land our bot " + wID + " is put where it was");
		}
		else { NetLand_TeleportToSpawn(ch, rand(40)); }
	}
	else
	{
		// like the puppets of the players: the model is made with its blade and gun, the sword is drawn in the fight
		SetNewModelToChar(ch);
		SendMessage(ch, "lsl", MSG_CHARACTER_EX_MSG, "SetFightWOWeapon", false);
		NetLand_PuppetWeapons(ch);
	}
	// the health kept over a load of the location (the respawn of the owner loads it again)
	if (bMine && CheckAttribute(&NCClients[wID], "LandBotKeepHP"))
	{
		float fKeep = stf(NCClients[wID].LandBotKeepHP);
		DeleteAttribute(&NCClients[wID], "LandBotKeepHP");
		if (fKeep >= 1.0 && fKeep < LAi_GetCharacterMaxHP(ch)) { LAi_SetHP(ch, fKeep, LAi_GetCharacterMaxHP(ch)); }
		if (CheckAttribute(&NCClients[wID], "LandBotKeepPots")) { ch.NetLandBotPots = NCClients[wID].LandBotKeepPots; }
	}
	NCClients[wID].LandBotChr = iChr;
	NCClients[wID].LandBotHP = makeint(LAi_GetCharacterHP(ch));
	NCClients[wID].LandBotHPMax = makeint(LAi_GetCharacterMaxHP(ch));
	NCClients[wID].LandBotDead = 0;
	Trace("COOP: land bot " + wID + " (" + NCClients[wID].NickName + ", " + sModel + ") is character " + iChr + ", ours " + bMine);
	return iChr;
}

// the server: the name, skin, blade, relation and owner of a bot (also when it is removed)
void NetLand_OnBotInfo(int iMsg)
{
	int wBot = NMGetClientID(iMsg);
	int wOwner = NMGetClientID(iMsg);
	int iRel = NMGetByte(iMsg);
	int iRemoved = NMGetByte(iMsg);
	int iFace = NMGetWord(iMsg);
	string sBlade = NMGetString(iMsg);
	string sNick = NMGetString(iMsg);
	int iLevel = NMGetByte(iMsg);
	string sGun = NMGetString(iMsg);
	if (wBot < NET_BOT_FIRST || wBot >= NET_MAXCLIENTS) { return; }
	if (iRemoved)
	{
		NetLand_BotDrop(wBot);
		NCClients[wBot].BotRemoved = 1;
		Trace("COOP: land bot " + wBot + " removed");
		return;
	}
	bool bNew = !NetLand_IsLandBot(wBot);
	if (!bNew)
	{
		if (sti(NCClients[wBot].LandBotOwner) != wOwner || sti(NCClients[wBot].LandBotFace) != iFace) { bNew = true; }
	}
	int iOldRel = -1;
	if (CheckAttribute(&NCClients[wBot], "BotRel")) { iOldRel = sti(NCClients[wBot].BotRel); }
	NCClients[wBot].Bot = 1;
	NCClients[wBot].LandBot = 1;
	NCClients[wBot].BotRemoved = 0;
	NCClients[wBot].LandBotOwner = wOwner;
	NCClients[wBot].LandBotFace = iFace;
	NCClients[wBot].LandBotBlade = sBlade;
	NCClients[wBot].LandBotLevel = iLevel;
	NCClients[wBot].LandBotGun = sGun;
	NCClients[wBot].BotRel = iRel;
	NCClients[wBot].NickName = sNick;
	if (!bNetLandActive) { return; }
	int iChr = NetLand_BotChr(wBot);
	if (bNew || iChr < 0)
	{
		// the bot of the others gets its puppet with the first state packet (its place is known then)
		if (wOwner == wClientID) { NetLand_BotMake(wBot); }
		else { NetLand_BotDrop(wBot); }
		return;
	}
	if (iOldRel != iRel)
	{
		ref ch = &Characters[iChr];
		LAi_group_MoveCharacter(ch, NetLand_BotGroup(iRel, wOwner == wClientID));
		NetLand_BotRelations();
		Trace("COOP: land bot " + wBot + " relation " + iRel);
	}
}

// ---------- the owner ----------
void NetLand_BotSendState(int wID, ref ch)
{
	float fX = 0.0;
	float fY = 0.0;
	float fZ = 0.0;
	float fAy = 0.0;
	// the game gives no place of a dead body (the packet of the death never went: the others saw the bot stand in its last
	// animation with full health until a load of the location of its owner). The last place of the living bot goes then
	if (GetCharacterPos(ch, &fX, &fY, &fZ))
	{
		GetCharacterAy(ch, &fAy);
		// the place of the bot: a load of the location (the respawn of its owner) puts the body there (NetLand_BotsAfterLoad)
		ch.NetLandSentX = fX;
		ch.NetLandSentY = fY;
		ch.NetLandSentZ = fZ;
		ch.NetLandSentAy = fAy;
	}
	else
	{
		if (!CheckAttribute(ch, "NetLandSentX")) { return; }
		fX = stf(ch.NetLandSentX);
		fY = stf(ch.NetLandSentY);
		fZ = stf(ch.NetLandSentZ);
		fAy = stf(ch.NetLandSentAy);
	}
	int iFlags = 0;
	if (SendMessage(ch, "ls", MSG_CHARACTER_EX_MSG, "IsFightMode") != 0) { iFlags = iFlags + 1; }
	if (sti(ch.NetLandBotDead)) { iFlags = iFlags + 2; }
	int iHP = makeint(LAi_GetCharacterHP(ch) + 0.5);
	int iHPMax = makeint(LAi_GetCharacterMaxHP(ch) + 0.5);
	if (iHP < 0) { iHP = 0; }
	NCClients[wID].LandBotHP = iHP;
	NCClients[wID].LandBotHPMax = iHPMax;
	string sAnim = "";
	if (!sti(ch.NetLandBotDead)) { sAnim = NetLand_GetAnim(ch, 0); }
	int iDV = 1;
	if (CheckAttribute(ch, "NetLandBotDV")) { iDV = sti(ch.NetLandBotDV); }
	int iEnB = makeint(Lai_CharacterGetEnergy(ch) + 0.5);
	int iEnBMax = makeint(LAi_GetCharacterMaxEnergy(ch) + 0.5);
	if (iEnB < 0) { iEnB = 0; }
	int iSMsg = NetClient_LandNewMsg(NSC_LAND_BOTSTATE);
	NMAddClientID(iSMsg, wID);
	NMAddFloat(iSMsg, fX);
	NMAddFloat(iSMsg, fY);
	NMAddFloat(iSMsg, fZ);
	NMAddFloat(iSMsg, fAy);
	NMAddByte(iSMsg, iFlags);
	NMAddWord(iSMsg, iHP);
	NMAddWord(iSMsg, iHPMax);
	NMAddByte(iSMsg, iDV);
	NMAddWord(iSMsg, iEnB);
	NMAddWord(iSMsg, iEnBMax);
	NMAddString(iSMsg, sAnim, 24);
	// the first packet of its death goes guaranteed (the others did not see the bot die: it stood in its last animation)
	bool bRelB = false;
	if (sti(ch.NetLandBotDead) && !CheckAttribute(ch, "NetLandDeadSent"))
	{
		bRelB = true;
		ch.NetLandDeadSent = 1;
		Trace("COOP: land our bot " + wID + ": its death goes to the others");
	}
	NetClient_LandSend(iSMsg, bRelB);
	NMDelete(iSMsg);
}

// a medium or hard bot drinks an elixir below 40% of its health (single player: the officers do it), every 5 s at most
void NetLand_BotHeal(ref ch, int iNow)
{
	if (!CheckAttribute(ch, "NetLandBotPots")) { return; }
	if (sti(ch.NetLandBotPots) <= 0) { return; }
	float fHP = LAi_GetCharacterHP(ch);
	float fMax = LAi_GetCharacterMaxHP(ch);
	if (fHP > fMax * 0.4) { return; }
	if (CheckAttribute(ch, "NetLandBotPotAt"))
	{
		int iAgo = iNow - sti(ch.NetLandBotPotAt);
		if (iAgo >= 0 && iAgo < 5000) { return; }
	}
	ch.NetLandBotPotAt = iNow;
	ch.NetLandBotPots = sti(ch.NetLandBotPots) - 1;
	// single player (LAi_character.c): a bottle heals 130 over some seconds, not at once
	float fBottle = 0.0;
	if (CheckAttribute(ch, "chr_ai.hp_bottle")) { fBottle = stf(ch.chr_ai.hp_bottle); }
	ch.chr_ai.hp_bottle = fBottle + 130.0;
	ch.chr_ai.hp_dlt_bottle = 10.0;
	Trace("COOP: land our bot " + ch.NetLandBotID + " drinks a potion at " + fHP + ", left " + ch.NetLandBotPots);
}

void NetLand_BotDie(int wID, ref ch)
{
	if (sti(ch.NetLandBotDead)) { return; }
	ch.NetLandBotDead = 1;
	ch.NetLandBotDiedAt = Net_GetTickCount();
	ch.NetLandBotDV = 1 + rand(3);
	NCClients[wID].LandBotDead = 1;
	if (LAi_GetCharacterHP(ch) >= 1.0)
	{
		LAi_SetImmortal(ch, false);
		LAi_KillCharacter(ch);
	}
	Trace("COOP: land our bot " + wID + " is dead");
	// COOP s28: the killer (the team duel counts it): the last one who hit the bot
	if (CheckAttribute(ch, "NetLandBotLastAtt"))
	{
		int wBotKiller = sti(ch.NetLandBotLastAtt);
		DeleteAttribute(ch, "NetLandBotLastAtt");
		if (wBotKiller >= 0 && wBotKiller < NET_MAXCLIENTS && wBotKiller != wID) { NetLand_BotEvt(wID, wBotKiller, NET_LANDBOTEVT_KILLED); }
	}
	if (IsEntity(ch)) { NetLand_BotSendState(wID, ch); }		// at once, not with the next packet
}

void NetLand_BotOwnerTick(int wID, int iNow)
{
	int iChr = NetLand_BotChr(wID);
	if (iChr < 0)
	{
		if (!bNetLandLocReady) { return; }
		// a dead bot (before a load of the location) waits for its respawn
		if (CheckAttribute(&NCClients[wID], "LandBotDeadAt"))
		{
			int iWaitD = iNetLandRespawnSec * 1000;
			if (iWaitD < 3000) { iWaitD = 3000; }
			if (!bNetCliBotRespawn || bNetBoardDuel) { return; }		// COOP s37: the crew of the boarding does not come back
			if (iNow - sti(NCClients[wID].LandBotDeadAt) < iWaitD) { return; }
			DeleteAttribute(&NCClients[wID], "LandBotDeadAt");
			DeleteAttribute(&NCClients[wID], "LandBotKeepHP");
			DeleteAttribute(&NCClients[wID], "LandBotKeepX");
		}
		NetLand_BotMake(wID);
		return;
	}
	ref ch = &Characters[iChr];
	if (!IsEntity(ch))
	{
		if (!CheckAttribute(ch, "NetLandGoneLog")) { ch.NetLandGoneLog = 1; Trace("COOP: land our bot " + wID + ": its body is not in the location"); }
		return;
	}
	if (!sti(ch.NetLandBotDead))
	{
		if (LAi_IsDead(ch) || LAi_GetCharacterHP(ch) < 1.0) { NetLand_BotDie(wID, ch); }
	}
	if (!sti(ch.NetLandBotDead))
	{
		NetLand_BotHeal(ch, iNow);
		NetLand_MushHandTick(ch, iNow);
	}
	else
	{
		// the respawn of the bots (the switch of the menu "Bots"): a new body at a spawn point
		int iDeadAgo = iNow - sti(ch.NetLandBotDiedAt);
		int iWait = iNetLandRespawnSec * 1000;
		if (iWait < 3000) { iWait = 3000; }
		if (bNetCliBotRespawn && !bNetBoardDuel && iDeadAgo > iWait)
		{
			DeleteAttribute(&NCClients[wID], "LandBotKeepX");
			NetLand_BotMake(wID);
			return;
		}
	}
	int iLast = 0;
	if (CheckAttribute(ch, "NetLandBotSent")) { iLast = sti(ch.NetLandBotSent); }
	int iAgo = iNow - iLast;
	if (iAgo >= 0 && iAgo < NET_LAND_BOT_RATE) { return; }
	ch.NetLandBotSent = iNow;
	NetLand_BotSendState(wID, ch);
}

// a blow of somebody else on our bot (it came through the server)
void NetLand_OnBotHit(int iMsg)
{
	int wAtt = NMGetClientID(iMsg);
	int wBot = NMGetClientID(iMsg);
	int iDmg = NMGetWord(iMsg);
	int iDmgB = NMGetWord(iMsg);
	int iKind = NMGetByte(iMsg);
	if (!bNetLandActive || !NetLand_BotMine(wBot)) { return; }
	int iChr = NetLand_BotChr(wBot);
	if (iChr < 0) { return; }
	ref ch = &Characters[iChr];
	if (!IsEntity(ch) || sti(ch.NetLandBotDead)) { return; }
	// an ally of the players is not hurt by them (COOP s28: nor a bot of a team by its team, nor a bot by a bot of its side)
	if (wAtt < NET_MAXPLAYERS) { if (NetLand_BotFriendOfPlayer(wBot, wAtt)) { return; } }
	if (wAtt >= NET_BOT_FIRST && wAtt < NET_MAXCLIENTS) { if (NetLand_BotRelOf(wAtt) == NetLand_BotRelOf(wBot)) { return; } }
	NetLand_BotHitBy(wBot, ch, wAtt);		// COOP s28: the kill goes to him; a neutral bot turns into an enemy
	int iUse = iDmg;
	if (iKind == 0)
	{
		int iDef = NetLand_ChrDefence(ch);
		if (iDef == 2 || iDef == 3) { Trace("COOP: land our bot " + wBot + " dodges the blow of " + wAtt); return; }
		if (iDef == 1) { iUse = iDmgB; LAi_CharacterPlaySound(ch, "fgt_collision"); }
	}
	if (iUse <= 0) { return; }
	float fHP = LAi_GetCharacterHP(ch) - makefloat(iUse);
	Trace("COOP: land our bot " + wBot + " takes " + iUse + " from " + wAtt + ", hp " + fHP);
	if (fHP < 1.0)
	{
		NetLand_ReportDmg(wBot, wAtt, makeint(LAi_GetCharacterHP(ch) + 0.5), ch);
		NetLand_BotDie(wBot, ch);
		NetLand_BotSendState(wBot, ch);
		return;
	}
	LAi_ApplyCharacterDamage(ch, iUse);
	NetLand_ReportDmg(wBot, wAtt, iUse, ch);
	// the bot of the others turns on the one who hits it
	if (sti(NCClients[wBot].BotRel) == NET_BOTREL_NEUTRAL && wAtt < NET_MAXPLAYERS) { LAi_group_MoveCharacter(ch, NetLand_BotGroup(NET_BOTREL_ENEMY, true)); }
}

// the fight state of any character: 1 block, 2 dodge, 3 feint, 0 nothing (NetLand_HeroDefence for our hero)
int NetLand_ChrDefence(ref chr)
{
	int iPk = NetLand_GetFState(chr);
	if (iPk == 0) { return 0; }
	int iSt = iPk - (iPk / 256) * 256;
	if (iSt == 16) { return 2; }
	if (iSt == 5) { return 3; }
	if (iSt == 13 || iSt == 14) { return 1; }
	return 0;
}

// the admin menu, done by the owner
void NetLand_BotCmd(int wBot, int iValue)
{
	if (!NetLand_BotMine(wBot)) { return; }
	int iCmd = iValue - (iValue / 16) * 16;
	int wAdm = iValue / 16;
	int iChr = NetLand_BotChr(wBot);
	if (iCmd == NET_LANDBOT_HEAL)
	{
		DeleteAttribute(&NCClients[wBot], "LandBotDeadAt");
		DeleteAttribute(&NCClients[wBot], "LandBotKeepHP");
		if (iChr < 0) { NetLand_BotMake(wBot); return; }
		if (sti(Characters[iChr].NetLandBotDead)) { NetLand_BotMake(wBot); return; }
		LAi_SetHP(&Characters[iChr], LAi_GetCharacterMaxHP(&Characters[iChr]), LAi_GetCharacterMaxHP(&Characters[iChr]));
		return;
	}
	if (iChr < 0) { return; }
	ref ch = &Characters[iChr];
	if (iCmd == NET_LANDBOT_KILL) { NetLand_BotDie(wBot, ch); NetLand_BotSendState(wBot, ch); return; }
	if (iCmd == NET_LANDBOT_BRING)
	{
		int iTo = GetMainCharacterIndex();
		if (wAdm != wClientID) { iTo = NetLand_GetPuppet(wAdm); }
		if (iTo < 0) { return; }
		ref rTo = &Characters[iTo];
		float fX = 0.0;
		float fY = 0.0;
		float fZ = 0.0;
		float fAy = 0.0;
		if (!GetCharacterPos(rTo, &fX, &fY, &fZ)) { return; }
		GetCharacterAy(rTo, &fAy);
		bool bDone = TeleportCharacterToPosAy(ch, fX + sin(fAy) * 1.5, fY, fZ + cos(fAy) * 1.5, fAy + 3.14159265);
		if (!bDone) { bDone = TeleportCharacterToPosAy(ch, fX + cos(fAy) * 1.2, fY, fZ - sin(fAy) * 1.2, fAy); }
		if (!bDone) { bDone = TeleportCharacterToPosAy(ch, fX, fY, fZ, fAy); }
		ch.NetLandBotSent = 0;
	}
}

// ---------- the others: the puppet of a bot ----------
void NetLand_OnBotState(int iMsg)
{
	int wBot = NMGetClientID(iMsg);
	float fX = NMGetFloat(iMsg);
	float fY = NMGetFloat(iMsg);
	float fZ = NMGetFloat(iMsg);
	float fAy = NMGetFloat(iMsg);
	int iFlags = NMGetByte(iMsg);
	int iHP = NMGetWord(iMsg);
	int iHPMax = NMGetWord(iMsg);
	int iDV = NMGetByte(iMsg);
	int iEnR = NMGetWord(iMsg);
	int iEnMaxR = NMGetWord(iMsg);
	string sAnim = NMGetString(iMsg);
	if (!bNetLandActive || !bNetLandLocReady) { return; }
	if (!NetLand_IsLandBot(wBot) || NetLand_BotMine(wBot)) { return; }
	NCClients[wBot].LandBotHP = iHP;
	NCClients[wBot].LandBotHPMax = iHPMax;
	if (findSubStr(sAnim, "death", 0) == 0) { sAnim = ""; }		// the death is played by the death of the puppet, not by the name
	int iDead = 0;
	if (iFlags >= 2 && iFlags != 1) { iDead = (iFlags / 2) - (iFlags / 4) * 2; }
	int iChr = NetLand_BotChr(wBot);
	if (iDead == 1 && iChr >= 0)
	{
		if (!sti(Characters[iChr].NetLandBotDead)) { Trace("COOP: land the death of bot " + wBot + " has come, its puppet " + iChr + " is in the location " + IsEntity(&Characters[iChr])); }
	}
	if (iChr >= 0)
	{
		// a dead puppet and a living bot: the bot has come back, a new body
		if (sti(Characters[iChr].NetLandBotDead) && iDead == 0) { iChr = -1; }
	}
	if (iChr < 0)
	{
		// a dead bot gets a body too: it falls once and lies like on the screen of the others (the body of the old load was gone
		// after our respawn: the others saw the dead bot, we did not)
		iChr = NetLand_BotMake(wBot);
		if (iChr < 0) { return; }
		TeleportCharacterToPosAy(&Characters[iChr], fX, fY, fZ, fAy);
		if (iDead) { Trace("COOP: land the dead bot " + wBot + " had no body here: character " + iChr + " lies in its place"); }
	}
	ref ch = &Characters[iChr];
	NCClients[wBot].LandBotDead = iDead;
	// the health bar of the game over the bot, and its energy (the second bar)
	if (iDead == 0 && iHPMax > 0) { LAi_SetHP(ch, makefloat(iHP), makefloat(iHPMax)); }
	if (iDead == 0 && iEnMaxR > 0)
	{
		ch.chr_ai.energyMax = makefloat(iEnMaxR);
		ch.chr_ai.energy = makefloat(iEnR);
	}
	ch.NetLandLX = fX;
	ch.NetLandLY = fY;
	ch.NetLandLZ = fZ;
	ch.NetLandLAy = fAy;
	ch.NetLandBotT = Net_GetTickCount();
	if (!IsEntity(ch)) { return; }
	if (iDead)
	{
		if (sti(ch.NetLandBotDead)) { return; }
		ch.NetLandBotDead = 1;
		Trace("COOP: land the puppet of bot " + wBot + " dies");
		if (iDV < 1 || iDV > 4) { iDV = 1; }
		CharacterPlayAction(ch, "");
		DeleteAttribute(ch, "NetLandBotAnim");
		SendMessage(ch, "lsl", MSG_CHARACTER_EX_MSG, "LockFightMode", 0);
		// like the puppet of a dead player (NetLand_OnDead): the death task of the game, the body falls once and lies. The kill
		// of the game (LAi) did nothing to the actor: the puppet stood in its last animation
		NetLand_SetDeathVariant(ch, iDV);
		SetCharacterTask_Dead(ch);
		return;
	}
	int iFight = iFlags - (iFlags / 2) * 2;
	// the game changes the fight mode of a character only while no action of the script plays: the stand of the fight played
	// by its name held the puppet ("game says 0" every time) - the bot swung an empty hand, and the health bar of the game
	// (it is drawn over an enemy with the blade out) never came. The action goes first, the names wait for the blade
	int iEngF = 0;
	if (SendMessage(ch, "ls", MSG_CHARACTER_EX_MSG, "IsFightMode") != 0) { iEngF = 1; }
	if (iEngF != iFight)
	{
		// 1.5 s at most: then the names play anyway (the old way), the puppet does not stand still without them
		int iNowW = Net_GetTickCount();
		if (!CheckAttribute(ch, "NetLandFWait")) { ch.NetLandFWait = iNowW; }
		int iWaitF = iNowW - sti(ch.NetLandFWait);
		bool bWaitF = false;
		if (iWaitF >= 0 && iWaitF < 1500) { bWaitF = true; }
		if (bWaitF && CheckAttribute(ch, "NetLandBotAnim"))
		{
			CharacterPlayAction(ch, "");
			DeleteAttribute(ch, "NetLandBotAnim");
		}
		NetLand_PuppetFight(ch, iFight);
		iEngF = 0;
		if (SendMessage(ch, "ls", MSG_CHARACTER_EX_MSG, "IsFightMode") != 0) { iEngF = 1; }
		if (iEngF == iFight) { DeleteAttribute(ch, "NetLandFWait"); }
		if (iEngF != iFight && bWaitF) { return; }		// the next packet tries again
	}
	else { DeleteAttribute(ch, "NetLandFWait"); }
	string sCur = "";
	if (CheckAttribute(ch, "NetLandBotAnim")) { sCur = ch.NetLandBotAnim; }
	if (sAnim != "" && sAnim != sCur)
	{
		ch.NetLandBotAnim = sAnim;
		CharacterPlayAction(ch, sAnim);
	}
}

// the puppets of the bots go to their places smoothly; the bots of ours send their state
void NetLand_BotSmooth(int iNow)
{
	// the game changes the relations of the groups by itself (a blow on a friend, a death): they are set again every 2 s
	int iRelAgo = iNow - iNetLandBotRelAt;
	if (iRelAgo < 0 || iRelAgo > 2000)
	{
		iNetLandBotRelAt = iNow;
		NetLand_BotRelations();
	}
	for (int i=NET_BOT_FIRST; i<NET_MAXCLIENTS; i++)
	{
		if (!NetLand_IsLandBot(i)) { continue; }
		if (NetLand_BotMine(i)) { NetLand_BotOwnerTick(i, iNow); continue; }
		int iChr = NetLand_BotChr(i);
		if (iChr < 0) { continue; }
		ref ch = &Characters[iChr];
		if (!IsEntity(ch) || sti(ch.NetLandBotDead)) { continue; }
		NetLand_MushHandTick(ch, iNow);
		if (!CheckAttribute(ch, "NetLandLX")) { continue; }
		float fX = 0.0;
		float fY = 0.0;
		float fZ = 0.0;
		float fAy = 0.0;
		if (!GetCharacterPos(ch, &fX, &fY, &fZ)) { continue; }
		GetCharacterAy(ch, &fAy);
		float fTX = stf(ch.NetLandLX);
		float fTY = stf(ch.NetLandLY);
		float fTZ = stf(ch.NetLandLZ);
		float fTAy = stf(ch.NetLandLAy);
		float fD = sqrt((fTX - fX) * (fTX - fX) + (fTZ - fZ) * (fTZ - fZ));
		float fDAy = fTAy - fAy;
		while (fDAy > 3.14159265) { fDAy = fDAy - 6.2831853; }
		while (fDAy < -3.14159265) { fDAy = fDAy + 6.2831853; }
		if (fD < 0.02 && fDAy < 0.02 && fDAy > -0.02) { continue; }
		float fK = 0.35;
		if (fD > 3.0) { fK = 1.0; }
		// the height at once (the floors of a ship: a height on the way to it put the bot on the nearer deck)
		if (TeleportCharacterToPosAy(ch, fX + (fTX - fX) * fK, fTY, fZ + (fTZ - fZ) * fK, fAy + fDAy * fK)) { NetLand_FloorFix(ch, fTY, fAy + fDAy * fK); }
	}
}

// ---------- blows (NetLand_BladeHit / NetLand_FireHit) ----------
bool NetLand_IsBotChr(aref chr)
{
	if (!CheckAttribute(chr, "NetLandBotID")) { return false; }
	return NetLand_IsLandBot(sti(chr.NetLandBotID));
}

// somebody of ours (the hero or a bot of ours) hits the puppet of a bot of the others: its owner applies it
void NetLand_SendBotHit(int wAtt, int wBot, float fDmg, float fDmgB, int iKind)
{
	int iD = makeint(fDmg + 0.5);
	int iDB = makeint(fDmgB + 0.5);
	if (iD <= 0 && iDB <= 0) { return; }
	if (iDB > iD) { iDB = iD; }
	int iSMsg = NetClient_LandNewMsg(NSC_LAND_BOTHIT);
	NMAddClientID(iSMsg, wAtt);
	NMAddClientID(iSMsg, wBot);
	NMAddWord(iSMsg, iD);
	NMAddWord(iSMsg, iDB);
	NMAddByte(iSMsg, iKind);
	NetClient_LandSend(iSMsg, true);
	NMDelete(iSMsg);
}

// our bot hits a player (his puppet here): the player gets it like a blow of a player
void NetLand_SendBotHitP(int wBot, int wVictim, float fDmg, float fDmgB, int iKind, int iView)
{
	int iD = makeint(fDmg + 0.5);
	int iDB = makeint(fDmgB + 0.5);
	if (iD <= 0 && iDB <= 0) { return; }
	if (iDB > iD) { iDB = iD; }
	iNetLandBotHitSeq++;
	if (iNetLandBotHitSeq > 255) { iNetLandBotHitSeq = 0; }
	int iSMsg = NetClient_LandNewMsg(NSC_LAND_BOTHITP);
	NMAddClientID(iSMsg, wBot);
	NMAddClientID(iSMsg, wVictim);
	NMAddWord(iSMsg, iD);
	NMAddWord(iSMsg, iDB);
	NMAddByte(iSMsg, iKind);
	NMAddByte(iSMsg, iNetLandBotHitSeq);
	NMAddWord(iSMsg, iView);
	NetClient_LandSend(iSMsg, false);
	NetClient_LandSend(iSMsg, false);
	NMDelete(iSMsg);
}

// the blow of our bot on the puppet of a player: what our game sees (block / dodge / feint of the puppet), like the blow of our
// hero (NetLand_BladeHit): the victim plays the block hit, the attacker (the bot) is thrown back by a dodge
int NetLand_BotHitView(aref attack, aref enemy, string attackType, bool isBlocked)
{
	ref rPup = &Characters[sti(enemy.index)];
	int iDef = NetLand_PuppetDefence(rPup, attackType);
	if (CheckAttribute(rPup, "NetLandHold"))
	{
		if (sti(rPup.NetLandHold) == 1)
		{
			iDef = 0;
			if (isBlocked) { iDef = 1; }
		}
	}
	if (iDef == 0 && isBlocked) { iDef = 1; }
	int iView = 0;
	if (iDef == 1) { iView = 1; }
	if (iDef == 2) { iView = 2; }
	if (iDef == 4) { iView = 3; }
	if (attackType == "break") { iView = iView + 4; }
	if (CheckCharacterPerk(attack, "HardHitter")) { iView = iView + 8; }
	if (CheckCharacterPerk(attack, "Sliding")) { if (rand(99) < 20) { iView = iView + 64; } }
	if (iDef == 0)
	{
		int iHitV = 1 + rand(2);
		iView = iView + iHitV * 16;
		PostEvent("NetLand_PupHitVar", 1, "lll", iNetLandGen, sti(enemy.index), iHitV);
	}
	// a dodge throws the bot back (single player: hit_parry)
	if (iDef == 2 && bNetLandFStateOk)
	{
		ref rAtt = &Characters[sti(attack.index)];
		NetLand_SetFState(rAtt, 10, 0);
	}
	return iView;
}

// 1 = handled (true for the game), 0 = handled (false for the game), -1 = not a blow of / on a bot
int NetLand_BotBladeHit(aref attack, aref enemy, string attackType, bool isBlocked)
{
	bool bAttBot = NetLand_IsBotChr(attack);
	bool bEnBot = NetLand_IsBotChr(enemy);
	if (!bAttBot && !bEnBot) { return -1; }
	if (bAttBot)
	{
		int wAB = sti(attack.NetLandBotID);
		if (!NetLand_BotMine(wAB)) { return 1; }		// the puppet of a bot of the others: its owner decides
		if (sti(attack.NetLandBotDead)) { return 1; }
		if (NetLand_IsOurHero(enemy))
		{
			if (NetLand_BotFriendOfPlayer(wAB, wClientID)) { return 1; }		// COOP s28: an ally / a bot of our team
			if (bNetLandDead) { return 1; }
			ref rMcB = GetMainCharacter();
			float fCH = NetLand_BladeDamage(attack, rMcB, attackType, false);
			float fBH = NetLand_BladeDamage(attack, rMcB, attackType, true);
			int iViewH = 0;
			if (isBlocked) { iViewH = 1; }
			if (attackType == "break") { iViewH = iViewH + 4; }
			if (CheckCharacterPerk(attack, "HardHitter")) { iViewH = iViewH + 8; }
			if (NetLand_HeroDefence() == 2 && bNetLandFStateOk)
			{
				ref rAttH = &Characters[sti(attack.index)];
				NetLand_SetFState(rAttH, 10, 0);		// the dodge throws the bot back
			}
			NetLand_ApplyHit(wAB, makeint(fCH + 0.5), makeint(fBH + 0.5), 0, iViewH);
			return 1;
		}
		int wV = NetLand_PuppetID(enemy);
		if (wV != DST_INVALID)
		{
			if (NetLand_BotFriendOfPlayer(wAB, wV)) { return 1; }		// COOP s28: an ally / a bot of his team
			if (CheckAttribute(enemy, "NetLandDead")) { if (sti(enemy.NetLandDead)) { return 1; } }
			float fC = NetLand_BladeDamage(attack, enemy, attackType, false);
			float fB = NetLand_BladeDamage(attack, enemy, attackType, true);
			NetLand_SendBotHitP(wAB, wV, fC, fB, 0, NetLand_BotHitView(attack, enemy, attackType, isBlocked));
			return 1;
		}
		if (bEnBot)
		{
			int wEB = sti(enemy.NetLandBotID);
			if (NetLand_BotMine(wEB))
			{
				enemy.NetLandBotLastAtt = wAB;		// COOP s28: the kill goes to it
				return 0;		// two bots of ours: the game
			}
			if (sti(enemy.NetLandBotDead)) { return 1; }
			if (NetLand_BotRelOf(wAB) == NetLand_BotRelOf(wEB)) { return 1; }		// COOP s28: the same side
			NetLand_SendBotHit(wAB, wEB, NetLand_BladeDamage(attack, enemy, attackType, false), NetLand_BladeDamage(attack, enemy, attackType, true), 0);
			return 1;
		}
		return 0;
	}
	// a blow on a bot
	int wB = sti(enemy.NetLandBotID);
	if (NetLand_BotMine(wB))
	{
		// our hero on our bot: the game (the bot is a real character here); a player's puppet: the player sends it himself.
		// An ally is not hit: the game would turn its whole group against the player
		if (NetLand_IsOurHero(attack))
		{
			if (NetLand_BotFriendOfPlayer(wB, wClientID)) { return 1; }		// COOP s28: an ally / a bot of our team
			NetLand_BotHitBy(wB, enemy, wClientID);		// COOP s28: our kill; a neutral bot turns into an enemy
			return 0;
		}
		return 1;
	}
	if (sti(enemy.NetLandBotDead)) { return 1; }
	if (!NetLand_IsOurHero(attack) || bNetLandDead) { return 1; }
	if (NetLand_BotFriendOfPlayer(wB, wClientID)) { return 1; }		// COOP s28: an ally / a bot of our team
	NetLand_SendBotHit(wClientID, wB, NetLand_BladeDamage(attack, enemy, attackType, false), NetLand_BladeDamage(attack, enemy, attackType, true), 0);
	return 1;
}

int NetLand_BotFireHit(aref attack, aref enemy, float kDist)
{
	bool bAttBot = NetLand_IsBotChr(attack);
	bool bEnBot = NetLand_IsBotChr(enemy);
	if (!bAttBot && !bEnBot) { return -1; }
	if (bAttBot)
	{
		int wFB = sti(attack.NetLandBotID);
		if (!NetLand_BotMine(wFB)) { return 1; }
		// COOP s28: an ally / a bot of a team does not shoot its friends (our hero, the puppets of the players, the bots of its side)
		if (!bEnBot)
		{
			int wFP = NetLand_PuppetID(enemy);
			if (NetLand_IsOurHero(enemy)) { wFP = wClientID; }
			if (wFP != DST_INVALID) { if (NetLand_BotFriendOfPlayer(wFB, wFP)) { return 1; } }
		}
		else
		{
			if (NetLand_BotRelOf(wFB) == NetLand_BotRelOf(sti(enemy.NetLandBotID))) { return 1; }
		}
		float fPB = LAi_GunCalcProbability(attack, kDist);
		if (rand(10000) > fPB * 10000) { return 1; }
		if (NetLand_ArmorStopsShot(enemy)) { return 1; }		// COOP: the cuirass
		if (NetLand_IsOurHero(enemy))
		{
			if (bNetLandDead) { return 1; }
			ref rMcF = GetMainCharacter();
			int iShotH = makeint(LAi_GunCalcDamage(attack, rMcF) + 0.5);
			NetLand_ApplyHit(wFB, iShotH, iShotH, 1, 0);
			return 1;
		}
		int wVF = NetLand_PuppetID(enemy);
		if (wVF != DST_INVALID)
		{
			float fShotP = LAi_GunCalcDamage(attack, enemy);
			NetLand_SendBotHitP(wFB, wVF, fShotP, fShotP, 1, 0);
			return 1;
		}
		if (bEnBot)
		{
			int wEF = sti(enemy.NetLandBotID);
			if (NetLand_BotMine(wEF))
			{
				enemy.NetLandBotLastAtt = wFB;		// COOP s28: the kill goes to it
				return 0;
			}
			float fShotB = LAi_GunCalcDamage(attack, enemy);
			NetLand_SendBotHit(wFB, wEF, fShotB, fShotB, 1);
			return 1;
		}
		return 0;
	}
	int wB = sti(enemy.NetLandBotID);
	if (NetLand_BotMine(wB))
	{
		// COOP s28: our hero shoots our bot: not a friend; the kill is ours, a neutral bot turns into an enemy
		if (NetLand_IsOurHero(attack))
		{
			if (NetLand_BotFriendOfPlayer(wB, wClientID)) { return 1; }
			NetLand_BotHitBy(wB, enemy, wClientID);
		}
		return 0;
	}
	if (!NetLand_IsOurHero(attack) || bNetLandDead) { return 1; }
	if (sti(enemy.NetLandBotDead)) { return 1; }
	if (NetLand_BotFriendOfPlayer(wB, wClientID)) { return 1; }		// COOP s28: an ally / a bot of our team
	float fP = LAi_GunCalcProbability(attack, kDist);
	if (rand(10000) > fP * 10000) { return 1; }
	if (NetLand_ArmorStopsShot(enemy)) { return 1; }		// COOP: the cuirass
	float fShot = LAi_GunCalcDamage(attack, enemy);
	NetLand_SendBotHit(wClientID, wB, fShot, fShot, 1);
	return 1;
}

// the duel ends / the player who ran them is gone
void NetLand_BotsForget(int wOwner)
{
	for (int i=NET_BOT_FIRST; i<NET_MAXCLIENTS; i++)
	{
		if (!CheckAttribute(&NCClients[i], "LandBot")) { continue; }
		if (wOwner != DST_INVALID)
		{
			if (sti(NCClients[i].LandBotOwner) != wOwner) { continue; }
		}
		NetLand_BotDrop(i);
		DeleteAttribute(&NCClients[i], "LandBot");
		DeleteAttribute(&NCClients[i], "Bot");
		NCClients[i].BotRemoved = 1;
	}
}

// a new body of the location (a respawn loads it again): the bots of ours stay ours, the puppets come with the next state
void NetLand_BotsAfterLoad()
{
	for (int i=NET_BOT_FIRST; i<NET_MAXCLIENTS; i++)
	{
		if (!NetLand_IsLandBot(i)) { continue; }
		int iChr = NetLand_BotChr(i);
		if (iChr < 0) { continue; }
		if (!IsEntity(&Characters[iChr]))
		{
			DeleteAttribute(&NCClients[i], "LandBotChr");
			if (NetLand_BotMine(i))
			{
				// the bot of ours: its health and its death go on in the new body
				NCClients[i].LandBotKeepHP = Characters[iChr].chr_ai.hp;
				if (CheckAttribute(&Characters[iChr], "NetLandBotPots")) { NCClients[i].LandBotKeepPots = Characters[iChr].NetLandBotPots; }
				if (sti(Characters[iChr].NetLandBotDead)) { NCClients[i].LandBotDeadAt = Characters[iChr].NetLandBotDiedAt; }
				// its place too: the respawn of its owner does not take the bot to a spawn point (the others saw it jump)
				// (a body that is gone)
				DeleteAttribute(&NCClients[i], "LandBotKeepX");
				if (CheckAttribute(&Characters[iChr], "NetLandSentX") && !sti(Characters[iChr].NetLandBotDead))
				{
					NCClients[i].LandBotKeepX = Characters[iChr].NetLandSentX;
					NCClients[i].LandBotKeepY = Characters[iChr].NetLandSentY;
					NCClients[i].LandBotKeepZ = Characters[iChr].NetLandSentZ;
					NCClients[i].LandBotKeepAy = Characters[iChr].NetLandSentAy;
				}
			}
		}
		else
		{
			// the load put the body of our bot at the entry of the location (with our hero): back to its place
			if (NetLand_BotMine(i))
			{
				ref rBk = &Characters[iChr];
				if (CheckAttribute(rBk, "NetLandSentX"))
				{
					TeleportCharacterToPosAy(rBk, stf(rBk.NetLandSentX), stf(rBk.NetLandSentY), stf(rBk.NetLandSentZ), stf(rBk.NetLandSentAy));
					Trace("COOP: land our bot " + i + " stays where it was (the location is loaded again)");
				}
				if (sti(rBk.NetLandBotDead))
				{
					// the load logs the body in as a living character: it got up and fought for the dead bot. It dies again (no
					// new death for the others: the bot is dead for them already; its respawn timer goes on)
					int iDVk = 1;
					if (CheckAttribute(rBk, "NetLandBotDV")) { iDVk = sti(rBk.NetLandBotDV); }
					NetLand_SetDeathVariant(rBk, iDVk);
					LAi_SetImmortal(rBk, false);
					LAi_KillCharacter(rBk);
					Trace("COOP: land our dead bot " + i + " lies again (the location is loaded again)");
				}
			}
		}
	}
	NetLand_BotRelations();
}

// ---------- the server ----------
int NetServer_LandBotInfoMsg(int i)
{
	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_LAND);
	NMAddByte(iSMsg, NSC_LAND_BOTINFO_R);
	NMAddClientID(iSMsg, i);
	NMAddClientID(iSMsg, sti(NSClients[i].LandBotOwner));
	NMAddByte(iSMsg, sti(NSClients[i].LandBotRel));
	NMAddByte(iSMsg, sti(NSClients[i].LandBotRemoved));
	NMAddWord(iSMsg, sti(NSClients[i].LandBotFace));
	NMAddString(iSMsg, NSClients[i].LandBotBlade, 32);
	NMAddString(iSMsg, NSClients[i].LandBotNick, 32);
	NMAddByte(iSMsg, sti(NSClients[i].LandBotLevel));
	NMAddString(iSMsg, NSClients[i].LandBotGun, 32);
	return iSMsg;
}

bool NetServer_IsLandBot(int i)
{
	if (i < NET_BOT_FIRST || i >= NET_MAXCLIENTS) { return false; }
	if (!CheckAttribute(&NSClients[i], "LandBot")) { return false; }
	if (!sti(NSClients[i].LandBot)) { return false; }
	if (sti(NSClients[i].LandBotRemoved)) { return false; }
	int wO = sti(NSClients[i].LandBotOwner);
	if (wO < 0 || wO >= NET_MAXPLAYERS) { return false; }
	if (!sti(NSClients[wO].Use)) { return false; }
	return true;
}

void NetServer_LandBotSendInfo(int i)
{
	int iSMsg = NetServer_LandBotInfoMsg(i);
	NetServer_LandSendOthers(DST_INVALID, iSMsg, true);
	NMDelete(iSMsg);
}

void NetServer_LandBotsTo(int wTo)
{
	for (int i=NET_BOT_FIRST; i<NET_MAXCLIENTS; i++)
	{
		if (!NetServer_IsLandBot(i)) { continue; }
		int iSMsg = NetServer_LandBotInfoMsg(i);
		NetServer_SendMessage(wTo, iSMsg, true);
		NMDelete(iSMsg);
	}
}

void NetServer_LandBotsReset()
{
	for (int i=NET_BOT_FIRST; i<NET_MAXCLIENTS; i++)
	{
		DeleteAttribute(&NSClients[i], "LandBot");
		DeleteAttribute(&NSClients[i], "LandBotRemoved");
	}
	NetServer.LandBotN = 0;
}

void NetServer_LandAdminBotAdd(int wAdmin, int iRel, int iShipW, int iBladeIdx, int iLevel, int iGunLo)
{
	// the window "Bot+" of a land duel: the ship word is the face (9 bits) and the high part of the gun (+1, 0: none)
	int iFace = iShipW - (iShipW / 512) * 512;
	int iGunIdx = (iShipW / 512) * 256 + iGunLo - 1;
	string sGun = "";
	if (iGunIdx >= 0 && iGunIdx < ITEMS_QUANTITY) { sGun = Items[iGunIdx].id; }
	if (iLevel < 1) { iLevel = 1; }
	if (iLevel > 50) { iLevel = 50; }
	if (!NetServer_BotRelValid(iRel)) { iRel = NET_BOTREL_ENEMY; }		// COOP s28: a team of the team duel too
	int iFree = -1;
	for (int i=NET_BOT_FIRST; i<NET_MAXCLIENTS; i++)
	{
		if (CheckAttribute(&NSClients[i], "Bot")) { if (sti(NSClients[i].Bot)) { continue; } }
		if (NetServer_IsLandBot(i)) { continue; }
		iFree = i;
		break;
	}
	if (iFree < 0) { Trace("COOP: land bot: no free slot"); return; }
	string sBlade = "";
	if (iBladeIdx >= 0 && iBladeIdx < ITEMS_QUANTITY) { sBlade = Items[iBladeIdx].id; }
	if (NetLand_FaceToModel(iFace) == "") { iFace = NetLand_BotNextFace(-1, 1); }
	if (!CheckAttribute(&NetServer, "LandBotN")) { NetServer.LandBotN = 0; }
	NetServer.LandBotN = sti(NetServer.LandBotN) + 1;
	NSClients[iFree].LandBot = 1;
	NSClients[iFree].LandBotRemoved = 0;
	NSClients[iFree].LandBotOwner = wAdmin;
	NSClients[iFree].LandBotRel = iRel;
	NSClients[iFree].LandBotFace = iFace;
	NSClients[iFree].LandBotBlade = sBlade;
	NSClients[iFree].LandBotLevel = iLevel;
	NSClients[iFree].LandBotGun = sGun;
	NSClients[iFree].LandBotNick = "Бот " + NetServer.LandBotN;
	Trace("COOP: land bot " + iFree + " added by " + wAdmin + ": face " + iFace + ", blade " + sBlade + ", gun " + sGun + ", level " + iLevel + ", relation " + iRel);
	NetServer_LandBotSendInfo(iFree);
}

// the bot goes from the game: its body and its place in the list (every machine drops its character)
void NetServer_LandBotRemove(int wBot)
{
	NSClients[wBot].LandBotRemoved = 1;
	int iSMsgD = NetServer_LandBotInfoMsg(wBot);
	NetServer_LandSendOthers(DST_INVALID, iSMsgD, true);
	NMDelete(iSMsgD);
}

// a minute after the death of a bot (the bots do not come back): the body and the slot go. Still dead, the same death,
// the respawn of the bots still off
void NetServer_LandBotCorpse()
{
	int wBotC = GetEventData();
	int iRoundC = GetEventData();
	int iDeathC = GetEventData();
	if (!bServerGameStarted || !NetServer_LandMode()) { return; }
	if (iRoundC != sti(NetServer.Round)) { return; }
	if (!NetServer_IsLandBot(wBotC)) { return; }
	if (NetServer_BotRespawnOn()) { return; }
	if (!CheckAttribute(&NSClients[wBotC], "LandBotDeadSeen")) { return; }
	if (sti(NSClients[wBotC].LandBotDeadSeen) == 0) { return; }
	if (!CheckAttribute(&NSClients[wBotC], "LandBotDeathN")) { return; }
	if (sti(NSClients[wBotC].LandBotDeathN) != iDeathC) { return; }
	Trace("COOP: land the dead bot " + wBotC + " is removed (the bots do not come back)");
	NetServer_LandBotRemove(wBotC);
}

// the menu "Bots" in a land duel; true = handled
bool NetServer_LandBotAdmin(int wAdmin, int iAction, int wBot, float fValue)
{
	if (!NetServer_IsLandBot(wBot)) { return false; }
	int wOwner = sti(NSClients[wBot].LandBotOwner);
	switch (iAction)
	{
		case NET_ADMIN_BOT_DELETE:
			NetServer_LandBotRemove(wBot);
			return true;
		break;
		case NET_ADMIN_BOT_DESTROY:
			NetServer_LandAdminTo(wOwner, NET_LANDADM_BOTCMD, wBot, NET_LANDBOT_KILL);
			return true;
		break;
		case NET_ADMIN_BOT_HEAL:
			NetServer_LandAdminTo(wOwner, NET_LANDADM_BOTCMD, wBot, NET_LANDBOT_HEAL);
			return true;
		break;
		case NET_ADMIN_BOT_BRING:
			NetServer_LandAdminTo(wOwner, NET_LANDADM_BOTCMD, wBot, NET_LANDBOT_BRING + 16 * wAdmin);
			return true;
		break;
		case NET_ADMIN_BOT_GOTO:
			NetServer_LandAdminTo(wAdmin, NET_LANDADM_TPTO, wBot, 0);
			return true;
		break;
		case NET_ADMIN_BOT_REL:
			int iRel = makeint(fValue + 0.5);
			if (!NetServer_BotRelValid(iRel)) { return true; }		// COOP s28: a team of the team duel too
			NSClients[wBot].LandBotRel = iRel;
			NetServer_LandBotSendInfo(wBot);
			return true;
		break;
	}
	return false;
}

// NetServer_OnLand: the messages of the bots
void NetServer_OnLandBot(int wID, int iSub, int iMsg)
{
	int iSMsg;
	int wBot;
	if (iSub == NSC_LAND_BOTSTATE)
	{
		wBot = NMGetClientID(iMsg);
		float fX = NMGetFloat(iMsg);
		float fY = NMGetFloat(iMsg);
		float fZ = NMGetFloat(iMsg);
		float fAy = NMGetFloat(iMsg);
		int iFlags = NMGetByte(iMsg);
		int iHP = NMGetWord(iMsg);
		int iHPMax = NMGetWord(iMsg);
		int iDV = NMGetByte(iMsg);
		int iEnS = NMGetWord(iMsg);
		int iEnMaxS = NMGetWord(iMsg);
		string sAnim = NMGetString(iMsg);
		if (!NetServer_IsLandBot(wBot)) { return; }
		if (sti(NSClients[wBot].LandBotOwner) != wID) { return; }
		iSMsg = NMCreate();
		NMAddByte(iSMsg, NC_LAND);
		NMAddByte(iSMsg, NSC_LAND_BOTSTATE_R);
		NMAddClientID(iSMsg, wBot);
		NMAddFloat(iSMsg, fX);
		NMAddFloat(iSMsg, fY);
		NMAddFloat(iSMsg, fZ);
		NMAddFloat(iSMsg, fAy);
		NMAddByte(iSMsg, iFlags);
		NMAddWord(iSMsg, iHP);
		NMAddWord(iSMsg, iHPMax);
		NMAddByte(iSMsg, iDV);
		NMAddWord(iSMsg, iEnS);
		NMAddWord(iSMsg, iEnMaxS);
		NMAddString(iSMsg, sAnim, 24);
		int iDeadS = (iFlags / 2) - (iFlags / 4) * 2;
		bool bRelS = false;
		int iDeadWas = 0;
		if (CheckAttribute(&NSClients[wBot], "LandBotDeadSeen")) { iDeadWas = sti(NSClients[wBot].LandBotDeadSeen); }
		if (iDeadS == 1 && iDeadWas == 0)
		{
			bRelS = true;
			Trace("COOP: land the death of bot " + wBot + " goes to the players");
			int iBotDN = 0;
			if (CheckAttribute(&NSClients[wBot], "LandBotDeathN")) { iBotDN = sti(NSClients[wBot].LandBotDeathN); }
			iBotDN++;
			NSClients[wBot].LandBotDeathN = iBotDN;
			if (!NetServer_BotRespawnOn())
			{
				PostEvent("NetServer_LandBotCorpse", NET_LAND_BOT_CORPSE_MS, "lll", wBot, sti(NetServer.Round), iBotDN);
			}
		}
		NSClients[wBot].LandBotDeadSeen = iDeadS;
		NetServer_LandSendOthers(wID, iSMsg, bRelS);
		NMDelete(iSMsg);
		return;
	}
	if (iSub == NSC_LAND_BOTHIT)
	{
		int wAtt = NMGetClientID(iMsg);
		wBot = NMGetClientID(iMsg);
		int iD = NMGetWord(iMsg);
		int iDB = NMGetWord(iMsg);
		int iK = NMGetByte(iMsg);
		if (!NetServer_IsLandBot(wBot)) { return; }
		// the attacker is the player himself or a bot of his
		if (wAtt != wID)
		{
			if (!NetServer_IsLandBot(wAtt)) { return; }
			if (sti(NSClients[wAtt].LandBotOwner) != wID) { return; }
		}
		int wOwn = sti(NSClients[wBot].LandBotOwner);
		if (wOwn == wID) { return; }
		NetServer_LandBotProvoked(wBot, wAtt);		// COOP s28: a neutral bot that is hit turns into an enemy
		iSMsg = NMCreate();
		NMAddByte(iSMsg, NC_LAND);
		NMAddByte(iSMsg, NSC_LAND_BOTHIT_R);
		NMAddClientID(iSMsg, wAtt);
		NMAddClientID(iSMsg, wBot);
		NMAddWord(iSMsg, iD);
		NMAddWord(iSMsg, iDB);
		NMAddByte(iSMsg, iK);
		NetServer_SendMessage(wOwn, iSMsg, true);
		NMDelete(iSMsg);
		return;
	}
	if (iSub == NSC_LAND_BOTHITP)
	{
		wBot = NMGetClientID(iMsg);
		int wVictim = NMGetClientID(iMsg);
		int iDP = NMGetWord(iMsg);
		int iDBP = NMGetWord(iMsg);
		int iKP = NMGetByte(iMsg);
		int iSeqP = NMGetByte(iMsg);
		int iViewP = NMGetWord(iMsg);
		if (!NetServer_IsLandBot(wBot)) { return; }
		if (sti(NSClients[wBot].LandBotOwner) != wID) { return; }
		if (wVictim < 0 || wVictim >= NET_MAXPLAYERS || wVictim == wID) { return; }
		if (!sti(NSClients[wVictim].Use) || !sti(NSClients[wVictim].InGame)) { return; }
		if (sti(NSClients[wVictim].LandDead)) { return; }
		iSMsg = NMCreate();
		NMAddByte(iSMsg, NC_LAND);
		NMAddByte(iSMsg, NSC_LAND_HIT_R);
		NMAddClientID(iSMsg, wBot);
		NMAddWord(iSMsg, iDP);
		NMAddWord(iSMsg, iDBP);
		NMAddWord(iSMsg, iViewP);
		NMAddByte(iSMsg, iKP);
		NMAddByte(iSMsg, iSeqP);
		NetServer_SendMessage(wVictim, iSMsg, false);
		NetServer_SendMessage(wVictim, iSMsg, false);
		NMDelete(iSMsg);
	}
}

// an underwater location (the city of the lost ships, the temple of Tenochtitlan): the game sets the breath timer of the
// hero and lets crabs in (Loc_ai\LAi_zLoadUtils.c: CreatUnderwater). A duel has neither
void NetLand_UnderwaterClean(ref mc)
{
	if (!IsEntity(loadedLocation)) { return; }
	if (!CheckAttribute(loadedLocation, "type")) { return; }
	if (loadedLocation.type != "underwater") { return; }
	DeleteAttribute(mc, "showTimer");
	DeleteAttribute(mc, "quest.ClearTimer");
	ClearAllLogStrings();		// the number of the timer stays on the screen otherwise (an empty eternal string crashed the game)
	for (int i=1; i<=22; i++)
	{
		int iCrab = GetCharacterIndex("Crab_" + i);
		if (iCrab < 0) { continue; }
		ChangeCharacterAddressGroup(&Characters[iCrab], "none", "", "");
	}
	Trace("COOP: land the underwater location is cleaned (no breath timer, no crabs)");
}

// the difficulty of the bots (the server sends it at the start, at a join and when the admin changes it): the bots of ours take it now
void NetLand_SetBotSkill(int iSkill)
{
	if (iSkill < 0) { iSkill = 0; }
	if (iSkill > 3) { iSkill = 3; }
	iNetLandBotSkill = iSkill;
	Trace("COOP: land bot difficulty " + iSkill);
	if (!bNetLandActive) { return; }
	for (int i=NET_BOT_FIRST; i<NET_MAXCLIENTS; i++)
	{
		if (!NetLand_BotMine(i)) { continue; }
		int iChr = NetLand_BotChr(i);
		if (iChr < 0) { continue; }
		ref ch = &Characters[iChr];
		if (!IsEntity(ch) || sti(ch.NetLandBotDead)) { continue; }
		ch.NetLandBotDiff = iSkill;
		if (iSkill <= 0) { LAi_SetStayType(ch); }
		else { LAi_SetWarriorType(ch); }
		LAi_group_MoveCharacter(ch, NetLand_BotGroup(sti(NCClients[i].BotRel), true));
		if (iSkill == 2 && sti(ch.NetLandBotPots) < 3) { ch.NetLandBotPots = 3; }
		if (iSkill >= 3 && sti(ch.NetLandBotPots) < 5) { ch.NetLandBotPots = 5; }
	}
}

// the AI of the game (Loc_ai\LAi_fightparams.c) for a land duel: the puppets of the bots do nothing by themselves, the bots of
// ours fight by their difficulty. -1.0: the game decides
float NetLand_AiParam(aref chr, string sKind)
{
	if (!bNetLandActive) { return -1.0; }
	if (CheckAttribute(chr, "NetLandBotPup")) { return 0.0; }
	if (!CheckAttribute(chr, "NetLandBotDiff")) { return -1.0; }
	int iD = sti(chr.NetLandBotDiff);
	if (iD <= 0)
	{
		if (sKind == "recoil") { return 1.0; }
		return 0.0;
	}
	if (iD < 3) { return -1.0; }
	// hard: it attacks all the time, defends whenever it is attacked, blocks and dodges much, is not staggered
	switch (sKind)
	{
		case "attack":	return 1.0; break;
		case "fast":	return 40.0; break;
		case "force":	return 30.0; break;
		case "round":	return 15.0; break;
		case "break":	return 25.0; break;
		case "feint":	return 20.0; break;
		case "defence":	return 1.0; break;
		case "block":	return 100.0; break;
		case "parry":	return 60.0; break;
		case "recoil":	return 0.0; break;
		case "fire":	return 0.6; break;
	}
	return -1.0;
}

// the puppets are put at the entry of the hero: a location without it (the deck of the own ship has no rld::loc0) deleted them
// ("error teleportation by location"): the first locator the location has
void NetLand_FixEntry()
{
	if (!IsEntity(loadedLocation)) { return; }
	if (CheckAttribute(loadedLocation, "locators." + sNetLandEntryGroup + "." + sNetLandEntryLocator)) { return; }
	string sOld = sNetLandEntryGroup + "::" + sNetLandEntryLocator;
	aref arG;
	aref arL;
	string sG = "rld";
	for (int g=0; g<8; g++)
	{
		sG = "rld";
		switch (g)
		{
			case 1: sG = "reload"; break;
			case 2: sG = "goto"; break;
			case 3: sG = "randitem"; break;
			case 4: sG = "item"; break;
			case 5: sG = "officers"; break;
			case 6: sG = "sit"; break;
			case 7: sG = "box"; break;
		}
		if (!CheckAttribute(loadedLocation, "locators." + sG)) { continue; }
		makearef(arG, loadedLocation.locators.(sG));
		if (GetAttributesNum(arG) <= 0) { continue; }
		arL = GetAttributeN(arG, 0);
		sNetLandEntryGroup = sG;
		sNetLandEntryLocator = GetAttributeName(arL);
		Trace("COOP: land the entry " + sOld + " is not in the location, the puppets come at " + sG + "::" + sNetLandEntryLocator);
		return;
	}
	Trace("COOP: land the location has no locators for the puppets");
}

// ---------- the loadout during the duel ----------
// a change of the loadout and the skills: with the respawn, or in the first 90 s of the duel while the hero has not fought yet
bool NetLand_LoChangeAllowed()
{
	if (!bNetLandActive) { return true; }
	if (iNetLandRespawn != 0) { return true; }
	// the first 30 s of the duel
	if (iNetLandStartAt != 0 && Net_GetTickCount() - iNetLandStartAt < 30000) { return true; }
	return false;
}

// F2 in the duel: the loadout window (net\interfaces\NetMenu.c)
void NetLand_OpenLoadoutKey()
{
	if (!bNetLandActive) { return; }
	bNetLandOpenLoadout = true;
	LaunchNetMenu();
}

// the hero puts the sword away when the loadout window opens, takes it again when the loadout is applied
void NetLand_LoSheathe(bool bDraw)
{
	if (!bNetLandActive || bNetLandDead) { return; }
	ref mc = GetMainCharacter();
	if (!IsEntity(mc)) { return; }
	LAi_SetFightMode(mc, bDraw);
}

bool NetLand_HeroPoisonBlade()
{
	if (!CheckAttribute(&oNetLandLo, "Poison")) { return false; }
	return sti(oNetLandLo.Poison) == 1;
}

// ---------- poison ----------
void NetLand_PoisonHero(int wBy)
{
	int iNow = Net_GetTickCount();
	int iLeft = iNetLandPoisonUntil - iNow;
	if (iLeft < 0) { iLeft = 0; }
	if (iLeft == 0) { Log_SetStringToLog(XI_ConvertString("You've been poisoned")); iNetLandPoisonAt = iNow; }
	iNetLandPoisonUntil = iNow + iLeft + (30 + rand(20)) * 1000;
	iNetLandPoisonBy = wBy;
	// the mark of the game (LAi_IsPoison): an antidote, the mixture or the rum cure it (DoCharacterUsedItem), the potion key takes
	// an antidote then. The poison of the game itself does nothing to our immortal hero
	ref mcP = GetMainCharacter();
	mcP.chr_ai.poison = makefloat(iNetLandPoisonUntil - iNow) / 1000.0;
	Trace("COOP: land our hero is poisoned by " + wBy + " for " + (iNetLandPoisonUntil - iNow) + " ms");
}

// the poison is over (the time, an antidote, the death, the respawn)
void NetLand_PoisonOff()
{
	iNetLandPoisonUntil = 0;
	fNetLandPoisonAcc = 0.0;
	ref mcO = GetMainCharacter();
	DeleteAttribute(mcO, "chr_ai.poison");
	DeleteAttribute(mcO, "poison.hp");
}

// single player (LAi_character.c): the poison takes 2 health a second
void NetLand_PoisonTick(ref mc, int iNow)
{
	if (iNetLandPoisonUntil == 0) { return; }
	if (bNetLandDead) { NetLand_PoisonOff(); return; }
	if (!CheckAttribute(mc, "chr_ai.poison"))
	{
		// cured: an antidote, the mixture
		NetLand_PoisonOff();
		Trace("COOP: land our hero is cured of the poison");
		return;
	}
	int iDt = iNow - iNetLandPoisonAt;
	if (iDt < 0) { iDt = 0; }
	if (iDt < 250) { return; }
	iNetLandPoisonAt = iNow;
	if (iNow - iNetLandPoisonUntil > 0) { NetLand_PoisonOff(); return; }
	float fD = makefloat(iDt) * 0.002;
	float fHP = LAi_GetCharacterHP(mc) - fD;
	if (fHP < 0.0) { fHP = 0.0; }
	LAi_SetHP(mc, fHP, LAi_GetCharacterMaxHP(mc));
	fNetLandPoisonAcc = fNetLandPoisonAcc + fD;
	if (fNetLandPoisonAcc >= 5.0 || fHP < 1.0)
	{
		NetLand_ReportDmg(wClientID, iNetLandPoisonBy, makeint(fNetLandPoisonAcc + 0.5), mc);
		fNetLandPoisonAcc = 0.0;
	}
	if (fHP < 1.0)
	{
		iNetLandPoisonUntil = 0;
		bNetLandPoisonDeath = true;
		NetLand_LocalDie(iNetLandPoisonBy);
		bNetLandPoisonDeath = false;
	}
}

// ---------- the damage numbers and F1 ----------
// the damage that a victim of ours has taken (our hero, a bot of ours): the number above it for the others, the damage of
// the attacker in the statistics
void NetLand_ReportDmg(int wVictim, int wAtt, int iDmg, ref rV)
{
	if (iDmg <= 0) { return; }
	if (wAtt < 0 || wAtt >= NET_MAXCLIENTS) { wAtt = DST_INVALID; }
	int iHP = makeint(LAi_GetCharacterHP(rV));
	int iHPMax = makeint(LAi_GetCharacterMaxHP(rV));
	if (iHP < 0) { iHP = 0; }
	int iSMsg = NetClient_LandNewMsg(NSC_LAND_DMG);
	NMAddClientID(iSMsg, wVictim);
	NMAddClientID(iSMsg, wAtt);
	NMAddWord(iSMsg, iDmg);
	NMAddWord(iSMsg, iHP);
	NMAddWord(iSMsg, iHPMax);
	NetClient_LandSend(iSMsg, false);
	NMDelete(iSMsg);
}

void NetLand_OnDmg(int iMsg)
{
	int wVictim = NMGetClientID(iMsg);
	int wAtt = NMGetClientID(iMsg);
	int iDmg = NMGetWord(iMsg);
	int iHP = NMGetWord(iMsg);
	int iHPMax = NMGetWord(iMsg);
	if (!bNetLandActive) { return; }
	int iChr = NetLand_GetPuppet(wVictim);
	if (iChr < 0) { return; }
	ref ch = &Characters[iChr];
	if (!IsEntity(ch)) { return; }
	// single player (LAi_ApplyCharacterDamage): the number above the character
	SendMessage(ch, "lfff", MSG_CHARACTER_VIEWDAMAGE, makefloat(iDmg), makefloat(iHP), makefloat(iHPMax));
}

// COOP: team duel: the score of the teams
void NetLand_OnTeamScore(int iMsg)
{
	iNetLandTeamKills0 = NMGetWord(iMsg);
	iNetLandTeamKills1 = NMGetWord(iMsg);
}

// COOP: team duel: a permanent line on the screen: the score and (with a time limit) the time to the end of the match
void NetLand_TimerTick()
{
	int iTimerG = GetEventData();
	if (iTimerG != iNetLandTimerGen || !bNetLandActive) { return; }
	if (!NetLand_TeamGame()) { return; }
	if (bNetBoardDuel) { return; }		// COOP s37: the boarding has no score of the teams
	string sTeamLine = XI_ConvertString("nteam_" + NetTeamName[0]) + " " + iNetLandTeamKills0 + " : " + iNetLandTeamKills1 + " " + XI_ConvertString("nteam_" + NetTeamName[1]);
	if (iNetLandMatchEnd != 0)
	{
		int iLeftT = iNetLandMatchEnd - Net_GetTickCount();
		if (iLeftT <= 0) { sTeamLine = sTeamLine + "   Время матча вышло"; }
		else
		{
			int iSecT = (iLeftT + 999) / 1000;
			int iMinT = iSecT / 60;
			iSecT = iSecT - iMinT * 60;
			string sSecT = "" + iSecT;
			if (iSecT < 10) { sSecT = "0" + iSecT; }
			sTeamLine = sTeamLine + "   До конца матча: " + iMinT + ":" + sSecT;
		}
	}
	Log_SetEternalString(sTeamLine);
	PostEvent("NetLand_TimerTick", 500, "l", iTimerG);
}

void NetLand_OnStat(int iMsg)
{
	int wID = NMGetClientID(iMsg);
	int iKills = NMGetWord(iMsg);
	int iDeaths = NMGetWord(iMsg);
	float fDmg = NMGetFloat(iMsg);
	int iDead = NMGetByte(iMsg);
	if (wID < 0 || wID >= NET_MAXCLIENTS) { return; }
	NCClients[wID].LandStat.Kills = iKills;
	NCClients[wID].LandStat.Deaths = iDeaths;
	NCClients[wID].LandStat.Dmg = makeint(fDmg);
	NCClients[wID].LandStat.Dead = iDead;
}

int NetLand_StatValue(int wID, string sKey)
{
	if (!CheckAttribute(&NCClients[wID], "LandStat." + sKey)) { return 0; }
	return sti(NCClients[wID].LandStat.(sKey));
}

void NetServer_LandSendStat(int wTo, int wID)
{
	if (wID < 0 || wID >= NET_MAXPLAYERS) { return; }
	float fDmg = 0.0;
	if (CheckAttribute(&NSClients[wID], "LandDmg")) { fDmg = stf(NSClients[wID].LandDmg); }
	int iDead = 0;
	if (CheckAttribute(&NSClients[wID], "LandDead")) { if (sti(NSClients[wID].LandDead)) { iDead = 1; } }
	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_LAND);
	NMAddByte(iSMsg, NSC_LAND_STAT_R);
	NMAddClientID(iSMsg, wID);
	NMAddWord(iSMsg, sti(NSClients[wID].LandKills));
	NMAddWord(iSMsg, sti(NSClients[wID].LandDeaths));
	NMAddFloat(iSMsg, fDmg);
	NMAddByte(iSMsg, iDead);
	if (wTo == DST_INVALID) { NetServer_LandSendOthers(DST_INVALID, iSMsg, true); }
	else { NetServer_SendMessage(wTo, iSMsg, true); }
	NMDelete(iSMsg);
}

void NetServer_OnLandDmg(int wID, int iMsg)
{
	int wVictim = NMGetClientID(iMsg);
	int wAtt = NMGetClientID(iMsg);
	int iDmg = NMGetWord(iMsg);
	int iHP = NMGetWord(iMsg);
	int iHPMax = NMGetWord(iMsg);
	// the victim is the sender himself or a bot of his
	if (wVictim != wID)
	{
		if (!NetServer_IsLandBot(wVictim)) { return; }
		if (sti(NSClients[wVictim].LandBotOwner) != wID) { return; }
	}
	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_LAND);
	NMAddByte(iSMsg, NSC_LAND_DMG_R);
	NMAddClientID(iSMsg, wVictim);
	NMAddClientID(iSMsg, wAtt);
	NMAddWord(iSMsg, iDmg);
	NMAddWord(iSMsg, iHP);
	NMAddWord(iSMsg, iHPMax);
	NetServer_LandSendOthers(wID, iSMsg, false);
	NMDelete(iSMsg);
	if (wAtt >= 0 && wAtt < NET_MAXPLAYERS && wAtt != wVictim)
	{
		if (sti(NSClients[wAtt].Use))
		{
			float fOld = 0.0;
			if (CheckAttribute(&NSClients[wAtt], "LandDmg")) { fOld = stf(NSClients[wAtt].LandDmg); }
			NSClients[wAtt].LandDmg = fOld + makefloat(iDmg);
			NetServer_LandSendStat(DST_INVALID, wAtt);
		}
	}
}

// COOP s28: the side of a player / a bot in the team duel (0 / 1), -1 = none (no team duel, an enemy / ally / neutral bot)
int NetServer_LandSideOf(int w)
{
	if (!NetServer_LandTeamGame()) { return -1; }
	if (w >= 0 && w < NET_MAXPLAYERS) { return NetServer_LandTeamOf(w); }
	if (!NetServer_IsLandBot(w)) { return -1; }
	int iRelS = sti(NSClients[w].LandBotRel);
	if (iRelS < NET_BOTREL_TEAM0 || iRelS > NET_BOTREL_TEAM0 + 1) { return -1; }
	return iRelS - NET_BOTREL_TEAM0;
}

// a neutral bot that is hit turns into an enemy of everybody (every machine gets its new relation)
void NetServer_LandBotProvoked(int wBot, int wBy)
{
	if (wBy < 0 || wBy >= NET_MAXCLIENTS || wBy == wBot) { return; }
	if (!NetServer_IsLandBot(wBot)) { return; }
	if (sti(NSClients[wBot].LandBotRel) != NET_BOTREL_NEUTRAL) { return; }
	NSClients[wBot].LandBotRel = NET_BOTREL_ENEMY;
	Trace("COOP: land the neutral bot " + wBot + " is hit by " + wBy + ": now an enemy");
	NetServer_LandBotSendInfo(wBot);
}

// the team duel: a kill of a team bot goes to the team of its killer (a player or a bot of the other team)
void NetServer_LandBotKilled(int wBot, int wKiller)
{
	int iVictimSide = NetServer_LandSideOf(wBot);
	int iKillerSide = NetServer_LandSideOf(wKiller);
	Trace("COOP: land bot " + wBot + " is killed by " + wKiller + ", sides " + iVictimSide + " / " + iKillerSide);
	if (iVictimSide < 0 || iKillerSide < 0 || iVictimSide == iKillerSide) { return; }
	if (sti(NetServer.GameOver)) { return; }
	if (wKiller >= 0 && wKiller < NET_MAXPLAYERS)
	{
		if (!CheckAttribute(&NSClients[wKiller], "LandKills")) { NSClients[wKiller].LandKills = 0; }
		NSClients[wKiller].LandKills = sti(NSClients[wKiller].LandKills) + 1;
		NetServer_LandSendStat(DST_INVALID, wKiller);
	}
	if (iKillerSide == 0) { NetServer.LandTeamKills0 = sti(NetServer.LandTeamKills0) + 1; }
	else { NetServer.LandTeamKills1 = sti(NetServer.LandTeamKills1) + 1; }
	NetServer_LandSendTeamScore(DST_INVALID);
}

// the owner of a bot: NET_LANDBOTEVT_*
void NetServer_OnLandBotEvt(int wID, int iMsg)
{
	int wBotE = NMGetClientID(iMsg);
	int wOtherE = NMGetClientID(iMsg);
	int iKindE = NMGetByte(iMsg);
	if (!NetServer_IsLandBot(wBotE)) { return; }
	if (sti(NSClients[wBotE].LandBotOwner) != wID) { return; }
	if (iKindE == NET_LANDBOTEVT_HIT) { NetServer_LandBotProvoked(wBotE, wOtherE); return; }
	if (iKindE == NET_LANDBOTEVT_KILLED) { NetServer_LandBotKilled(wBotE, wOtherE); return; }
}

// for the windows (net\interfaces): the values of this file
int NetLand_GetBotSkill() { return iNetLandBotSkill; }
int NetLand_LoLevel() { if (!CheckAttribute(&oNetLandLo, "Level")) { return 1; } return sti(oNetLandLo.Level); }
int NetLand_LoPoison() { if (!CheckAttribute(&oNetLandLo, "Poison")) { return 0; } return sti(oNetLandLo.Poison); }
void NetLand_LoSetPoison(int iOn)
{
	oNetLandLo.Poison = 0;
	if (iOn) { oNetLandLo.Poison = 1; }
	aref lo;
	makearef(lo, oNetLandLo);
	if (NetLand_LoCost(lo) > NetLand_LoBudget()) { oNetLandLo.Poison = 0; }
}

// the hero has died by the game itself (Loc_ai\LAi_events.c, LAi_Character_Dead_Process): the death of the duel, never the
// game over of single player (it ended the net game for everybody)
bool NetLand_HeroDiedByGame()
{
	if (!bNetLandActive) { return false; }
	Trace("COOP: land the game says our hero is dead");
	if (!bNetLandDead) { NetLand_LocalDie(DST_INVALID); }
	return true;
}

// the admin menu "Heal": a dead player stands up at once (the respawn), a living one gets his health
void NetServer_LandHealOrRespawn(int wTarget)
{
	if (wTarget < 0 || wTarget >= NET_MAXPLAYERS) { return; }
	if (!sti(NSClients[wTarget].Use)) { return; }
	if (sti(NSClients[wTarget].LandDead))
	{
		int iDeathH = 0;
		if (CheckAttribute(&NSClients[wTarget], "LandDeathN")) { iDeathH = sti(NSClients[wTarget].LandDeathN); }
		PostEvent("NetServer_LandRespawn", 1, "lll", wTarget, sti(NetServer.Round), iDeathH);
		return;
	}
	NetServer_LandAdminTo(wTarget, NET_LANDADM_HEAL, DST_INVALID, 0);
}

// the speed of the game (the admin menu, the +/- keys): the time of the game on every machine (4 = 1x)
void NetServer_LandSpeed(float fDelta)
{
	float fSpeed = 1.0;
	if (CheckAttribute(&NetServer, "GameSpeed")) { fSpeed = stf(NetServer.GameSpeed); }
	if (fDelta > 0.0) { fSpeed = fSpeed + 0.25; } else { fSpeed = fSpeed - 0.25; }
	if (fSpeed < 0.5) { fSpeed = 0.5; }
	if (fSpeed > 3.0) { fSpeed = 3.0; }
	if (fSpeed > 0.9 && fSpeed < 1.1) { fSpeed = 1.0; }
	NetServer.GameSpeed = fSpeed;
	NetServer_LandAdminTo(DST_ALL, NET_LANDADM_SPEED, DST_INVALID, makeint(fSpeed * 4.0 + 0.5));
	NetServer_AdminSendEnvToAdmins();
	Trace("COOP: land game speed " + fSpeed);
}

void NetLand_SetSpeed(int iValue)
{
	float fSpeed = makefloat(iValue) / 4.0;
	if (fSpeed < 0.25) { fSpeed = 1.0; }
	float fOldSp = fNetLandSpeed;
	fNetLandSpeed = fSpeed;
	iNetLandSpeedAt = Net_GetTickCount();
	SetTimeScale(fSpeed);
	Trace("COOP: land the speed of the game x" + fSpeed);
	if (!bNetLandActive) { return; }
	float fSpDiff = fOldSp - fSpeed;
	if (fSpDiff > -0.01 && fSpDiff < 0.01) { return; }		// the same speed (the start of the duel): no line
	Log_SetStringToLog("Скорость игры: x" + fSpeed);
}

// ================================================================================================
// COOP: the loadout window (the nodes *_LO of NetMenu.ini, Buy.ini, PlayerSettings.ini): the same code for the menu of the
// duel (F2), the lobby (the button "Снаряжение персонажа") and the profile ("Персонаж"). The interface shows the window and
// answers OnLoApply / OnLoBack itself; the rest is here
// ================================================================================================

// COOP s32: the texture of the icon of our profile (interfaces\FindGame.c: FG_GetProfileFacePath), "" = none
bool NetLand_TexExists(string sDir, string sName)
{
	object oTexA;
	if (NFFindFiles(&oTexA, sDir, sName + ".tga.tx", false) > 0) { return true; }
	object oTexB;
	if (NFFindFiles(&oTexB, sDir, sName + ".tga", false) > 0) { return true; }
	return false;
}

string NetLand_ProfFaceTex()
{
	string sFace = "";
	if (CheckAttribute(&NetClient, "FaceImage")) { sFace = NetClient.FaceImage; }
	if (sFace == "" && CheckAttribute(&NCClients[wClientID], "UserData.Face")) { sFace = NCClients[wClientID].UserData.Face; }
	if (sFace == "" || sFace == "_emptyface_") { return ""; }
	string sNpcFace = NetLand_NpcFaceTex(sFace);
	if (sNpcFace != "") { return sNpcFace; }
	if (NetLand_TexExists("resource\textures\NET\FACES\", sFace)) { return "NET\FACES\" + sFace + ".tga"; }
	if (NetLand_TexExists("resource\textures\INTERFACES\PORTRAITS\256\", sFace)) { return "INTERFACES\PORTRAITS\256\" + sFace + ".tga"; }
	return "";
}

void NetLand_UiLoUpdate()
{
	if (!bNetLandUiLo) { return; }
	SetFormatedText("TEXT_LO_0", NetLand_LoRowText(0));
	SetFormatedText("TEXT_LO_1", NetLand_LoRowText(1));
	SetFormatedText("TEXT_LO_2", NetLand_LoRowText(2));
	SetFormatedText("TEXT_LO_9", NetLand_LoRowText(9));
	SetFormatedText("TEXT_LO_23", NetLand_LoRowText(23));		// COOP: the armor
	SetFormatedText("TEXT_LO_HP", NetLand_LoHPText());
	SetFormatedText("TEXT_LO_INFO", NetLand_LoInfoText());
	// the portrait of the hero (the icon of the player, the face of the game: NetLand_SetHeroFace)
	// COOP s32: the icon of the profile (as FindGame shows it): the FaceId of a skin without a portrait stayed the one of Blaze
	ref mcF = GetMainCharacter();
	string sLoFace = "";
	if (bNetLandActive) { sLoFace = NetLand_ProfFaceTex(); }
	if (sLoFace == "" && bNetLandActive && CheckAttribute(mcF, "FaceId")) { sLoFace = "interfaces\portraits\256\face_" + mcF.FaceId + ".tga"; }
	if (sLoFace != "") { SetNewPicture("PIC_LO_FACE", sLoFace); }
	else { SetNodeUsing("PIC_LO_FACE", false); }		// the lobby, the profile: the hero of the duel does not exist yet
	// the icons of the blade and the pistol (the same as in the items of single player)
	NetLand_UiLoItemPic("PIC_LO_BLADE", NetLand_LoBladeId());
	NetLand_UiLoItemPic("PIC_LO_GUN", NetLand_LoGunId());
	aref loA;
	makearef(loA, oNetLandLo);
	NetLand_UiLoItemPic("PIC_LO_ARMOR", NetLand_LoArmor(loA));
	NetLand_UiLoFillPots();
	NetLand_UiLoFillSkills();
	NetLand_UiLoFillSpecial();
	NetLand_UiLoFillPerks();
	NetLand_UiLoPoisonUpdate();
}


void NetLand_UiLoPoisonUpdate()
{
	bNetLandUiLoSync = true;
	int iP = 0;
	iP = NetLand_LoPoison();
	CheckButton_SetState("CHECK_LO_POISON", 1, iP == 1);
	bNetLandUiLoSync = false;
}

void NetLand_UiLoItemPic(string sNode, string sId)
{
	int iIdx = -1;
	if (sId != "") { iIdx = GetItemIndex(sId); }
	if (iIdx < 0)
	{
		SetNodeUsing(sNode, false);
		return;
	}
	SetNodeUsing(sNode, true);
	SetNewGroupPicture(sNode, Items[iIdx].picTexture, "itm" + Items[iIdx].picIndex);
}

// the tables like the character screen (F2) of single player: an icon, a name, a value; the arrows stand at the rows
void NetLand_UiLoTableRow(string sTable, int iRow, string sGroup, string sImage, string sText, string sValue)
{
	string sRow = "tr" + iRow;
	if (sGroup != "")
	{
		GameInterface.(sTable).(sRow).td1.icon.group = sGroup;
		GameInterface.(sTable).(sRow).td1.icon.image = sImage;
		GameInterface.(sTable).(sRow).td1.icon.width = 28;
		GameInterface.(sTable).(sRow).td1.icon.height = 28;
		GameInterface.(sTable).(sRow).td1.icon.offset = "2, 0";
	}
	else
	{
		GameInterface.(sTable).(sRow).td1.str = sImage;
		GameInterface.(sTable).(sRow).td1.fontidx = 0;
		GameInterface.(sTable).(sRow).td1.align = "center";
	}
	GameInterface.(sTable).(sRow).td2.str = sText;
	GameInterface.(sTable).(sRow).td2.scale = 0.8;
	GameInterface.(sTable).(sRow).td3.str = sValue;
	GameInterface.(sTable).(sRow).td3.fontidx = 1;
	GameInterface.(sTable).(sRow).td3.scale = 0.9;
	GameInterface.(sTable).(sRow).td3.align = "center";
}

void NetLand_UiLoTableHead(string sTable, string sText)
{
	GameInterface.(sTable).hr.td1.str = "";
	GameInterface.(sTable).hr.td2.str = sText;
	GameInterface.(sTable).hr.td2.scale = 0.85;
	GameInterface.(sTable).hr.td3.str = "";
}

void NetLand_UiLoFillPots()
{
	Table_Clear("TABLE_LO_POT", false, true, false);
	NetLand_UiLoTableHead("TABLE_LO_POT", "Лечение");
	for (int k=0; k<NET_LAND_POTS; k++)
	{
		string sId = NetLand_PotId(k);
		int iRowV = 3 + k;
		if (k == 6) { iRowV = 22; }
		int iIdx = GetItemIndex(sId);
		string sGrp = "";
		string sImg = "";
		if (iIdx >= 0) { sGrp = Items[iIdx].picTexture; sImg = "itm" + Items[iIdx].picIndex; }
		NetLand_UiLoTableRow("TABLE_LO_POT", k + 1, sGrp, sImg, NetLand_ItemTitle(sId) + " (" + NetLand_ItemPrice(sId) + ")", "" + NetLand_LoRowValue(iRowV));
	}
	Table_UpdateWindow("TABLE_LO_POT");
}

void NetLand_UiLoFillSkills()
{
	Table_Clear("TABLE_LO_SKILL", false, true, false);
	NetLand_UiLoTableHead("TABLE_LO_SKILL", "Личные навыки");
	for (int k=0; k<5; k++)
	{
		NetLand_UiLoTableRow("TABLE_LO_SKILL", k + 1, "ICONS_SPEC", NetLand_SkillName(k) + " skill icon", NetLand_SkillTitle(k), "" + NetLand_LoRowValue(10 + k));
	}
	Table_UpdateWindow("TABLE_LO_SKILL");
}

void NetLand_UiLoFillSpecial()
{
	Table_Clear("TABLE_LO_SPECIAL", false, true, false);
	NetLand_UiLoTableHead("TABLE_LO_SPECIAL", "P.I.R.A.T.E.S");
	for (int j=0; j<7; j++)
	{
		// the letter of the characteristic, as on the character screen (XI_ConvertString("StrengthT") and so on)
		int k = NetLand_UiLoSpecAt(j);
		string sName = NetLand_SpecialName(k);
		string sLetter = XI_ConvertString(sName + "T");
		NetLand_UiLoTableRow("TABLE_LO_SPECIAL", j + 1, "", sLetter, XI_ConvertString(sName), "" + NetLand_LoRowValue(15 + k));
		// like TABLE_SPECIAL of the character screen (interface\character.c): the letter is not centred, it sits a little higher
		string sRowS = "tr" + (j + 1);
		DeleteAttribute(&GameInterface, "TABLE_LO_SPECIAL." + sRowS + ".td1.align");
		GameInterface.TABLE_LO_SPECIAL.(sRowS).td1.textoffset = "8,-6";
		GameInterface.TABLE_LO_SPECIAL.(sRowS).td1.scale = 1.0;
		GameInterface.TABLE_LO_SPECIAL.(sRowS).td2.str = sLetter + "  " + XI_ConvertString(sName);
	}
	Table_UpdateWindow("TABLE_LO_SPECIAL");
}

void NetLand_UiLoFillPerks()
{
	int iSelected = 1;
	if (CheckAttribute(&GameInterface, "TABLE_LO_PERKS.select")) { iSelected = sti(GameInterface.TABLE_LO_PERKS.select); }
	Table_Clear("TABLE_LO_PERKS", false, true, false);
	GameInterface.TABLE_LO_PERKS.hr.td1.str = "";
	if (NetLand_PerksChosen()) { GameInterface.TABLE_LO_PERKS.hr.td2.str = "Умения (двойной щелчок)"; }
	else { GameInterface.TABLE_LO_PERKS.hr.td2.str = "Умения: выданы сервером"; }
	GameInterface.TABLE_LO_PERKS.hr.td2.scale = 0.85;
	for (int i=0; i<13; i++)
	{
		string sRow = "tr" + (i + 1);
		string sGrpP = "PERK_DISABLE";
		GameInterface.TABLE_LO_PERKS.(sRow).td2.color = argb(255,170,170,170);
		if (NetLand_LoHasPerkN(i))
		{
			sGrpP = "PERK_ENABLE";
			GameInterface.TABLE_LO_PERKS.(sRow).td2.color = argb(255,255,255,255);
		}
		GameInterface.TABLE_LO_PERKS.(sRow).td1.icon.group = sGrpP;
		GameInterface.TABLE_LO_PERKS.(sRow).td1.icon.image = NetLand_PerkName(i);
		GameInterface.TABLE_LO_PERKS.(sRow).td1.icon.width = 28;
		GameInterface.TABLE_LO_PERKS.(sRow).td1.icon.height = 28;
		GameInterface.TABLE_LO_PERKS.(sRow).td1.icon.offset = "2, 0";
		GameInterface.TABLE_LO_PERKS.(sRow).td2.str = NetLand_PerkTitle(i);
		GameInterface.TABLE_LO_PERKS.(sRow).td2.scale = 0.8;
	}
	if (iSelected < 1 || iSelected > 13) { iSelected = 1; }
	GameInterface.TABLE_LO_PERKS.select = iSelected;
	Table_UpdateWindow("TABLE_LO_PERKS");
}

// the rows of P.I.R.A.T.E.S in the order of the word: P Strength, I Perception, R Agility, A Charisma, T Intellect, E Endurance,
// S Luck (NetLand_SpecialName: S P E C I A L)
int NetLand_UiLoSpecAt(int j)
{
	switch (j)
	{
		case 0: return 0; break;
		case 1: return 1; break;
		case 2: return 5; break;
		case 3: return 3; break;
		case 4: return 4; break;
		case 5: return 2; break;
		case 6: return 6; break;
	}
	return j;
}

void NetLand_UiLoStep(int iRow, int iDir)
{
	if (!bNetLandUiLo) { return; }
	if (iRow >= 15 && iRow <= 21) { iRow = 15 + NetLand_UiLoSpecAt(iRow - 15); }		// the arrows of the table row
	NetLand_LoStep(iRow, iDir);
	NetLand_UiLoUpdate();
}

void NetLand_UiLoPerk()
{
	if (!bNetLandUiLo) { return; }
	int iSelected = 0;
	if (CheckAttribute(&GameInterface, "TABLE_LO_PERKS.select")) { iSelected = sti(GameInterface.TABLE_LO_PERKS.select); }
	NetLand_LoPerkToggle(iSelected - 1);
	NetLand_UiLoUpdate();
}


void NetLand_UiLoH0Dec() { NetLand_UiLoStep(0, -1); }
void NetLand_UiLoH0Inc() { NetLand_UiLoStep(0, 1); }
void NetLand_UiLoH1Dec() { NetLand_UiLoStep(1, -1); }
void NetLand_UiLoH1Inc() { NetLand_UiLoStep(1, 1); }
void NetLand_UiLoH2Dec() { NetLand_UiLoStep(2, -1); }
void NetLand_UiLoH2Inc() { NetLand_UiLoStep(2, 1); }
void NetLand_UiLoH3Dec() { NetLand_UiLoStep(3, -1); }
void NetLand_UiLoH3Inc() { NetLand_UiLoStep(3, 1); }
void NetLand_UiLoH4Dec() { NetLand_UiLoStep(4, -1); }
void NetLand_UiLoH4Inc() { NetLand_UiLoStep(4, 1); }
void NetLand_UiLoH5Dec() { NetLand_UiLoStep(5, -1); }
void NetLand_UiLoH5Inc() { NetLand_UiLoStep(5, 1); }
void NetLand_UiLoH6Dec() { NetLand_UiLoStep(6, -1); }
void NetLand_UiLoH6Inc() { NetLand_UiLoStep(6, 1); }
void NetLand_UiLoH7Dec() { NetLand_UiLoStep(7, -1); }
void NetLand_UiLoH7Inc() { NetLand_UiLoStep(7, 1); }
void NetLand_UiLoH8Dec() { NetLand_UiLoStep(8, -1); }
void NetLand_UiLoH8Inc() { NetLand_UiLoStep(8, 1); }
void NetLand_UiLoH9Dec() { NetLand_UiLoStep(9, -1); }
void NetLand_UiLoH9Inc() { NetLand_UiLoStep(9, 1); }
void NetLand_UiLoH10Dec() { NetLand_UiLoStep(10, -1); }
void NetLand_UiLoH10Inc() { NetLand_UiLoStep(10, 1); }
void NetLand_UiLoH11Dec() { NetLand_UiLoStep(11, -1); }
void NetLand_UiLoH11Inc() { NetLand_UiLoStep(11, 1); }
void NetLand_UiLoH12Dec() { NetLand_UiLoStep(12, -1); }
void NetLand_UiLoH12Inc() { NetLand_UiLoStep(12, 1); }
void NetLand_UiLoH13Dec() { NetLand_UiLoStep(13, -1); }
void NetLand_UiLoH13Inc() { NetLand_UiLoStep(13, 1); }
void NetLand_UiLoH14Dec() { NetLand_UiLoStep(14, -1); }
void NetLand_UiLoH14Inc() { NetLand_UiLoStep(14, 1); }
void NetLand_UiLoH15Dec() { NetLand_UiLoStep(15, -1); }
void NetLand_UiLoH15Inc() { NetLand_UiLoStep(15, 1); }
void NetLand_UiLoH16Dec() { NetLand_UiLoStep(16, -1); }
void NetLand_UiLoH16Inc() { NetLand_UiLoStep(16, 1); }
void NetLand_UiLoH17Dec() { NetLand_UiLoStep(17, -1); }
void NetLand_UiLoH17Inc() { NetLand_UiLoStep(17, 1); }
void NetLand_UiLoH18Dec() { NetLand_UiLoStep(18, -1); }
void NetLand_UiLoH18Inc() { NetLand_UiLoStep(18, 1); }
void NetLand_UiLoH19Dec() { NetLand_UiLoStep(19, -1); }
void NetLand_UiLoH19Inc() { NetLand_UiLoStep(19, 1); }
void NetLand_UiLoH20Dec() { NetLand_UiLoStep(20, -1); }
void NetLand_UiLoH20Inc() { NetLand_UiLoStep(20, 1); }
void NetLand_UiLoH21Dec() { NetLand_UiLoStep(21, -1); }
void NetLand_UiLoH21Inc() { NetLand_UiLoStep(21, 1); }
void NetLand_UiLoH22Dec() { NetLand_UiLoStep(22, -1); }
void NetLand_UiLoH22Inc() { NetLand_UiLoStep(22, 1); }
void NetLand_UiLoH23Dec() { NetLand_UiLoStep(23, -1); }
void NetLand_UiLoH23Inc() { NetLand_UiLoStep(23, 1); }

// the events of the arrows and of the perk button (Buy.c, PlayerSettings.c; NetMenu.c has its own)
void NetLand_UiLoRegister(bool bOn)
{
	if (bOn == bNetLandUiLoReg) { return; }
	bNetLandUiLoReg = bOn;
	if (bOn)
	{
		SetEventHandler("OnLo0Dec", "NetLand_UiLoH0Dec", 0);
		SetEventHandler("OnLo0Inc", "NetLand_UiLoH0Inc", 0);
		SetEventHandler("OnLo1Dec", "NetLand_UiLoH1Dec", 0);
		SetEventHandler("OnLo1Inc", "NetLand_UiLoH1Inc", 0);
		SetEventHandler("OnLo2Dec", "NetLand_UiLoH2Dec", 0);
		SetEventHandler("OnLo2Inc", "NetLand_UiLoH2Inc", 0);
		SetEventHandler("OnLo3Dec", "NetLand_UiLoH3Dec", 0);
		SetEventHandler("OnLo3Inc", "NetLand_UiLoH3Inc", 0);
		SetEventHandler("OnLo4Dec", "NetLand_UiLoH4Dec", 0);
		SetEventHandler("OnLo4Inc", "NetLand_UiLoH4Inc", 0);
		SetEventHandler("OnLo5Dec", "NetLand_UiLoH5Dec", 0);
		SetEventHandler("OnLo5Inc", "NetLand_UiLoH5Inc", 0);
		SetEventHandler("OnLo6Dec", "NetLand_UiLoH6Dec", 0);
		SetEventHandler("OnLo6Inc", "NetLand_UiLoH6Inc", 0);
		SetEventHandler("OnLo7Dec", "NetLand_UiLoH7Dec", 0);
		SetEventHandler("OnLo7Inc", "NetLand_UiLoH7Inc", 0);
		SetEventHandler("OnLo8Dec", "NetLand_UiLoH8Dec", 0);
		SetEventHandler("OnLo8Inc", "NetLand_UiLoH8Inc", 0);
		SetEventHandler("OnLo9Dec", "NetLand_UiLoH9Dec", 0);
		SetEventHandler("OnLo9Inc", "NetLand_UiLoH9Inc", 0);
		SetEventHandler("OnLo10Dec", "NetLand_UiLoH10Dec", 0);
		SetEventHandler("OnLo10Inc", "NetLand_UiLoH10Inc", 0);
		SetEventHandler("OnLo11Dec", "NetLand_UiLoH11Dec", 0);
		SetEventHandler("OnLo11Inc", "NetLand_UiLoH11Inc", 0);
		SetEventHandler("OnLo12Dec", "NetLand_UiLoH12Dec", 0);
		SetEventHandler("OnLo12Inc", "NetLand_UiLoH12Inc", 0);
		SetEventHandler("OnLo13Dec", "NetLand_UiLoH13Dec", 0);
		SetEventHandler("OnLo13Inc", "NetLand_UiLoH13Inc", 0);
		SetEventHandler("OnLo14Dec", "NetLand_UiLoH14Dec", 0);
		SetEventHandler("OnLo14Inc", "NetLand_UiLoH14Inc", 0);
		SetEventHandler("OnLo15Dec", "NetLand_UiLoH15Dec", 0);
		SetEventHandler("OnLo15Inc", "NetLand_UiLoH15Inc", 0);
		SetEventHandler("OnLo16Dec", "NetLand_UiLoH16Dec", 0);
		SetEventHandler("OnLo16Inc", "NetLand_UiLoH16Inc", 0);
		SetEventHandler("OnLo17Dec", "NetLand_UiLoH17Dec", 0);
		SetEventHandler("OnLo17Inc", "NetLand_UiLoH17Inc", 0);
		SetEventHandler("OnLo18Dec", "NetLand_UiLoH18Dec", 0);
		SetEventHandler("OnLo18Inc", "NetLand_UiLoH18Inc", 0);
		SetEventHandler("OnLo19Dec", "NetLand_UiLoH19Dec", 0);
		SetEventHandler("OnLo19Inc", "NetLand_UiLoH19Inc", 0);
		SetEventHandler("OnLo20Dec", "NetLand_UiLoH20Dec", 0);
		SetEventHandler("OnLo20Inc", "NetLand_UiLoH20Inc", 0);
		SetEventHandler("OnLo21Dec", "NetLand_UiLoH21Dec", 0);
		SetEventHandler("OnLo21Inc", "NetLand_UiLoH21Inc", 0);
		SetEventHandler("OnLo22Dec", "NetLand_UiLoH22Dec", 0);
		SetEventHandler("OnLo22Inc", "NetLand_UiLoH22Inc", 0);
		SetEventHandler("OnLo23Dec", "NetLand_UiLoH23Dec", 0);
		SetEventHandler("OnLo23Inc", "NetLand_UiLoH23Inc", 0);
		SetEventHandler("OnLoPerk", "NetLand_UiLoPerk", 0);
	}
	else
	{
		DelEventHandler("OnLo0Dec", "NetLand_UiLoH0Dec");
		DelEventHandler("OnLo0Inc", "NetLand_UiLoH0Inc");
		DelEventHandler("OnLo1Dec", "NetLand_UiLoH1Dec");
		DelEventHandler("OnLo1Inc", "NetLand_UiLoH1Inc");
		DelEventHandler("OnLo2Dec", "NetLand_UiLoH2Dec");
		DelEventHandler("OnLo2Inc", "NetLand_UiLoH2Inc");
		DelEventHandler("OnLo3Dec", "NetLand_UiLoH3Dec");
		DelEventHandler("OnLo3Inc", "NetLand_UiLoH3Inc");
		DelEventHandler("OnLo4Dec", "NetLand_UiLoH4Dec");
		DelEventHandler("OnLo4Inc", "NetLand_UiLoH4Inc");
		DelEventHandler("OnLo5Dec", "NetLand_UiLoH5Dec");
		DelEventHandler("OnLo5Inc", "NetLand_UiLoH5Inc");
		DelEventHandler("OnLo6Dec", "NetLand_UiLoH6Dec");
		DelEventHandler("OnLo6Inc", "NetLand_UiLoH6Inc");
		DelEventHandler("OnLo7Dec", "NetLand_UiLoH7Dec");
		DelEventHandler("OnLo7Inc", "NetLand_UiLoH7Inc");
		DelEventHandler("OnLo8Dec", "NetLand_UiLoH8Dec");
		DelEventHandler("OnLo8Inc", "NetLand_UiLoH8Inc");
		DelEventHandler("OnLo9Dec", "NetLand_UiLoH9Dec");
		DelEventHandler("OnLo9Inc", "NetLand_UiLoH9Inc");
		DelEventHandler("OnLo10Dec", "NetLand_UiLoH10Dec");
		DelEventHandler("OnLo10Inc", "NetLand_UiLoH10Inc");
		DelEventHandler("OnLo11Dec", "NetLand_UiLoH11Dec");
		DelEventHandler("OnLo11Inc", "NetLand_UiLoH11Inc");
		DelEventHandler("OnLo12Dec", "NetLand_UiLoH12Dec");
		DelEventHandler("OnLo12Inc", "NetLand_UiLoH12Inc");
		DelEventHandler("OnLo13Dec", "NetLand_UiLoH13Dec");
		DelEventHandler("OnLo13Inc", "NetLand_UiLoH13Inc");
		DelEventHandler("OnLo14Dec", "NetLand_UiLoH14Dec");
		DelEventHandler("OnLo14Inc", "NetLand_UiLoH14Inc");
		DelEventHandler("OnLo15Dec", "NetLand_UiLoH15Dec");
		DelEventHandler("OnLo15Inc", "NetLand_UiLoH15Inc");
		DelEventHandler("OnLo16Dec", "NetLand_UiLoH16Dec");
		DelEventHandler("OnLo16Inc", "NetLand_UiLoH16Inc");
		DelEventHandler("OnLo17Dec", "NetLand_UiLoH17Dec");
		DelEventHandler("OnLo17Inc", "NetLand_UiLoH17Inc");
		DelEventHandler("OnLo18Dec", "NetLand_UiLoH18Dec");
		DelEventHandler("OnLo18Inc", "NetLand_UiLoH18Inc");
		DelEventHandler("OnLo19Dec", "NetLand_UiLoH19Dec");
		DelEventHandler("OnLo19Inc", "NetLand_UiLoH19Inc");
		DelEventHandler("OnLo20Dec", "NetLand_UiLoH20Dec");
		DelEventHandler("OnLo20Inc", "NetLand_UiLoH20Inc");
		DelEventHandler("OnLo21Dec", "NetLand_UiLoH21Dec");
		DelEventHandler("OnLo21Inc", "NetLand_UiLoH21Inc");
		DelEventHandler("OnLo22Dec", "NetLand_UiLoH22Dec");
		DelEventHandler("OnLo22Inc", "NetLand_UiLoH22Inc");
		DelEventHandler("OnLo23Dec", "NetLand_UiLoH23Dec");
		DelEventHandler("OnLo23Inc", "NetLand_UiLoH23Inc");
		DelEventHandler("OnLoPerk", "NetLand_UiLoPerk");
	}
}

void NetLand_UiLoOpen()
{
	NetLand_LoInit();
	bNetLandUiLo = true;
	SetNodeUsing("PIC_LO_FACE", true);
	NetLand_UiLoUpdate();
	SetCurrentNode("BTN_LO_APPLY");
}

// the poison switch (the CheckButtonChange of the interface)
void NetLand_UiLoCheck(string sNode, int bChecked)
{
	if (sNode != "CHECK_LO_POISON") { return; }
	if (bNetLandUiLoSync) { return; }
	NetLand_LoSetPoison(bChecked);
	NetLand_UiLoUpdate();
}

// ================================================================================================
// COOP: the profile of the player (Profiles.nsv, NCProfiles): his loadout of the duel, his ship of the lobby, the last game
// that he created. A choice that the money of a server does not allow is not used there, the profile keeps it
// ================================================================================================

string NetLand_ProfNick()
{
	if (sNetLandProfNick != "") { return sNetLandProfNick; }
	string sN = "";
	if (CheckAttribute(&NetClient, "NickName")) { sN = NetClient.NickName; }
	if (sN == "") { sN = "default"; }
	return sN;
}

void NetLand_ProfSave()
{
	Net_SaveFile(false, &NCProfiles, "Profiles.nsv");
}

void NetLand_ProfSaveLo()
{
	string sN = NetLand_ProfNick();
	NCProfiles.NetLo.(sN) = "";
	aref arDst;
	makearef(arDst, NCProfiles.NetLo.(sN));
	DeleteAttribute(arDst, "");
	aref arSrc;
	makearef(arSrc, oNetLandLo);
	CopyAttributes(arDst, arSrc);
	NetLand_ProfSave();
	Trace("COOP: the loadout is saved in the profile " + sN);
}

bool NetLand_ProfLoadLo(aref lo)
{
	string sN = NetLand_ProfNick();
	if (!CheckAttribute(&NCProfiles, "NetLo." + sN + ".Inited")) { return false; }
	aref arSrc;
	makearef(arSrc, NCProfiles.NetLo.(sN));
	DeleteAttribute(lo, "");
	CopyAttributes(lo, arSrc);
	return true;
}

// the speed of the game in the duel is set again twice a second while no window is open (the game puts 1x back by itself:
// the windows, the info screens)
void NetLand_KeepSpeed(int iNow)
{
	if (fNetLandSpeed > 0.99 && fNetLandSpeed < 1.01) { return; }
	int iAgo = iNow - iNetLandSpeedAt;
	if (iAgo >= 0 && iAgo < 500) { return; }
	iNetLandSpeedAt = iNow;
	if (CheckAttribute(&InterfaceStates, "Launched")) { if (sti(InterfaceStates.Launched)) { return; } }
	SetTimeScale(fNetLandSpeed);
}

// the hero of this puppet swims (the name of his animation: "swim")
// Location.dll (patched): "NetSwim" decides the swim of a puppet instead of the waves of this machine.
// 0 = the game decides, 1 = it swims, 2 = it does not swim, 3 = all the puppets are forgotten (a table of 32 in the dll)
void NetLand_SwimSet(ref ch, int iMode)
{
	if (!IsEntity(ch)) { return; }
	int iWasM = 0;
	if (CheckAttribute(ch, "NetLandSwimMode")) { iWasM = sti(ch.NetLandSwimMode); }
	if (iWasM == iMode) { return; }
	ch.NetLandSwimMode = iMode;
	int iSwAns = SendMessage(ch, "lsl", MSG_CHARACTER_EX_MSG, "NetSwim", iMode);
	// into the log: the answer of the dll (1 = it knows the message) and the changes of the swim (the first ones)
	if (iNetLandSwimLogM < 40)
	{
		iNetLandSwimLogM++;
		Trace("COOP: land swim mode of " + ch.id + ": " + iWasM + " -> " + iMode + " at " + Net_GetTickCount() + ", the dll answers " + iSwAns);
	}
}

void NetLand_SwimClearAll()
{
	ref mcSw = GetMainCharacter();
	if (IsEntity(mcSw)) { SendMessage(mcSw, "lsl", MSG_CHARACTER_EX_MSG, "NetSwim", 3); }
	for (int iSw=0; iSw<NET_MAXPLAYERS; iSw++)
	{
		if (!CheckAttribute(&NCClients[iSw], "LandChr")) { continue; }
		int iSwC = sti(NCClients[iSw].LandChr);
		if (iSwC >= 0 && iSwC < TOTAL_CHARACTERS) { DeleteAttribute(&Characters[iSwC], "NetLandSwimMode"); }
	}
}

bool NetLand_PuppetSwims(ref ch)
{
	if (!CheckAttribute(ch, "NetLandAnimLast")) { return false; }
	string sSw = ch.NetLandAnimLast;
	if (findSubStr(sSw, "wim", 0) >= 0) { return true; }
	return false;
}
