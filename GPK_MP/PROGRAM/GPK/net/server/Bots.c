// ================================================================================================
// COOP: bots - ships of the computer, added by an admin (Esc -> Admin menu -> "Бот+" / "Боты").
//
// A bot lives in a slot NET_BOT_FIRST..NET_MAXCLIENTS-1 (bot N = slot NET_BOT_FIRST + N - 1). It is a slot without a
// network player (Use = 0, like a parked ship of a player who left): Bot = 1, Spawned = 1. The server drives it:
// NetServer_BotsTick sets Ship.TurnLeft / TurnRight / SailState like the keys of a player and fires with the
// "outside camera" fire of the engine (it aims itself at the nearest enemy in the fire zone of every bort).
// The engine takes for an enemy every ship whose Team differs from the FriendlyTeam of the shooter, so for one shot the
// Team of every ship is set to "target" / "not a target" and given back at once (NetServer_FireOutside).
//
// A ship of a client is never deleted on the clients during a game (see client\Time.c), so a slot of a bot is used once
// per battle: a deleted bot sinks, its number comes free in the next battle.
// ================================================================================================

#define NET_BOT_TICK			400			// ms between two decisions of the bots
#define NET_BOT_SPAWN_DIST		400.0		// in front of the bow of the admin
#define NET_BOT_SPAWN_FREE		90.0		// no other ship closer than this to the spawn point of a bot
#define NET_BOT_SPAWN_STEP		110.0		// side step if the point is taken
#define NET_BOT_SEE_DIST		5000.0		// a bot looks for targets this far
#define NET_BOT_HOME_DIST		900.0		// a bot without a target does not sail farther from its spawn point
#define NET_BOT_FOLLOW_DIST		260.0		// an ally sails after its admin if it is farther
#define NET_BOT_TP_DIST			150.0		// teleport of a bot / to a bot: in front of the bow
#define NET_BOT_DEAD_KEEP		20000		// ms: a sunk bot is updated so long (then its packets stop)
#define NET_BOT_TEAM_TARGET		98			// temporary Team values for one shot (NetServer_FireOutside)
#define NET_BOT_TEAM_FRIEND		97

int iNetBotGen = 0;			// generation of the tick (a new game stops the old one)
int iNetBotCount = 0;		// bots created in this game; 0 = the fire of the players works exactly as before
float fNetBotTurnK = 1.0;	// 1: TurnState > 0 (TurnRight) turns the ship to a bigger Ship.Ang.y (checked in the game)
int iNetBotTurnBad = 0;

// ------------------------------------------------------------------------------------------------
bool NetServer_IsBot(int i)
{
	if (i < NET_BOT_FIRST || i >= NET_MAXCLIENTS) { return false; }
	if (!CheckAttribute(&NSClients[i], "Bot")) { return false; }
	return sti(NSClients[i].Bot) != 0;
}

bool NetServer_BotRemoved(int i)
{
	if (!CheckAttribute(&NSClients[i], "BotRemoved")) { return false; }
	return sti(NSClients[i].BotRemoved) != 0;
}

// the bot is in the battle: not removed, not sunk
bool NetServer_BotAlive(int i)
{
	if (!bServerGameStarted) { return false; }
	if (!NetServer_IsBot(i)) { return false; }
	if (NetServer_BotRemoved(i)) { return false; }
	return NetServer_ShipIsAlive(i);
}

int NetServer_BotRel(int i)
{
	if (!CheckAttribute(&NSClients[i], "BotRel")) { return NET_BOTREL_ENEMY; }
	return sti(NSClients[i].BotRel);
}

// Team of a bot on the server (never a team of the players 0..3; COOP s28: a team bot IS in the team of the players)
int NetServer_BotTeamOf(int iRel)
{
	if (iRel >= NET_BOTREL_TEAM0) { return iRel - NET_BOTREL_TEAM0; }
	return 4 + iRel;
}

// COOP s28: the number of the teams a bot can join in this game (0 = no team game: deathmatch, a land duel of everybody)
int NetServer_BotTeamsNum()
{
	if (NetServer_LandMode())
	{
		if (NetServer_LandTeamGame()) { return 2; }
		return 0;
	}
	if (sti(NetServer.GameType) == NETGAME_DEATHMATCH) { return 0; }
	int iN = 2;
	if (CheckAttribute(&NetServer, "NumTeams")) { iN = sti(NetServer.NumTeams); }
	if (iN < 2) { iN = 2; }
	if (iN > 4) { iN = 4; }
	return iN;
}

bool NetServer_BotRelValid(int iRel)
{
	if (iRel >= NET_BOTREL_ALLY && iRel <= NET_BOTREL_NEUTRAL) { return true; }
	if (iRel < NET_BOTREL_TEAM0) { return false; }
	return (iRel - NET_BOTREL_TEAM0) < NetServer_BotTeamsNum();
}

// a bot of a team (at sea)
bool NetServer_IsTeamBot(int i)
{
	if (!NetServer_IsBot(i)) { return false; }
	if (NetServer_BotRemoved(i)) { return false; }
	return NetServer_BotRel(i) >= NET_BOTREL_TEAM0;
}

// a ship of a side of a team game: a player or a team bot (server\games\*.c, NetServer_CountSides, NetServer_TimeUpAnnounce)
bool NetServer_TeamSideSlot(int i)
{
	if (i < 0 || i >= NET_MAXCLIENTS) { return false; }
	if (sti(NSClients[i].Use)) { return true; }
	return NetServer_IsTeamBot(i);
}

// COOP s28: a neutral bot that is attacked (a ball, the fire of it) turns into an enemy of everybody, like a neutral ship of
// single player that is fired at. Every machine gets its new relation (the colour, the targets of the players).
void NetServer_BotProvoked(int i, int wBy)
{
	if (wBy == DST_INVALID || wBy == i) { return; }
	if (!NetServer_IsBot(i)) { return; }
	if (NetServer_BotRemoved(i)) { return; }
	if (NetServer_BotRel(i) != NET_BOTREL_NEUTRAL) { return; }
	ref rB = NetServer_GetClient(i);
	rB.BotRel = NET_BOTREL_ENEMY;
	rB.Team = NetServer_BotTeamOf(NET_BOTREL_ENEMY);
	rB.FriendlyTeam = NetServer_BotTeamOf(NET_BOTREL_ENEMY);
	Trace("COOP: the neutral bot " + i + " is attacked by " + wBy + ": now an enemy");
	int iSMsg = NetServer_BotInfoMessage(i, false);
	NetServer_SendToInGame(iSMsg);
	NMDelete(iSMsg);
}

// COOP s28: a team bot of the attackers of Capture the fort sails to the fort and shoots it when no ship is near
bool NetServer_BotFortGoal(int i)
{
	if (sti(NetServer.GameType) != NETGAME_FORT) { return false; }
	if (NetServer_BotRel(i) < NET_BOTREL_TEAM0) { return false; }
	if (!CheckAttribute(&NetServer, "FortX") || !CheckAttribute(&NetServer, "FortZ")) { return false; }
	if (!CheckAttribute(&NetServer, "FortLocator")) { return false; }
	if (NetServer.FortLocator == "") { return false; }
	if (NetServer_FortGameDead()) { return false; }
	ref rFortG = NetServer_GetCLient(NET_FORTCLIENT);
	if (!CheckAttribute(rFortG, "FriendlyTeam")) { return false; }
	if (!CheckAttribute(rFortG, "Ship.HP")) { return false; }
	if (stf(rFortG.Ship.HP) <= 0.0) { return false; }
	return sti(rFortG.FriendlyTeam) != NetServer_BotRel(i) - NET_BOTREL_TEAM0;
}

float NetServer_BotAbs(float f)
{
	if (f < 0.0) { return -f; }
	return f;
}

// fTo - fFrom in -PI..PI
float NetServer_BotAngDiff(float fTo, float fFrom)
{
	float f = fTo - fFrom;
	if (f > 1000.0 || f < -1000.0) { return 0.0; }		// broken (NaN / INF) angle: no turn
	while (f > PI) { f = f - PIm2; }
	while (f < -PI) { f = f + PIm2; }
	return f;
}

// start and end of a game (server\OnShip.c: NetServer_CreateShipEnvironment / NetServer_DeleteShipEnvironment)
void NetServer_BotsReset()
{
	iNetBotGen++;
	iNetBotCount = 0;
	iNetBotTurnBad = 0;
	NetServer_BotShoreReset();
	for (int i=NET_BOT_FIRST; i<NET_MAXCLIENTS; i++)
	{
		NSClients[i].Bot = false;
		NSClients[i].BotRemoved = false;
	}
}

void NetServer_BotsStart()
{
	NetServer_BotsReset();
	SetEventHandler("NetServer_BotsTick", "NetServer_BotsTick", 0);
	PostEvent("NetServer_BotsTick", NET_BOT_TICK, "l", iNetBotGen);
}

void NetServer_BotsStop()
{
	DelEventHandler("NetServer_BotsTick", "NetServer_BotsTick");
	NetServer_BotsReset();
}

// ------------------------------------------------------------------------------------------------
// messages
// ------------------------------------------------------------------------------------------------
int NetServer_BotInfoMessage(int i, bool bAnnounce)
{
	ref rB = NetServer_GetClient(i);
	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_CLIENT);
	NMAddByte(iSMsg, NSC_CLIENT_BOT);
	NMAddByte(iSMsg, NET_BOTOP_INFO);
	NMAddClientID(iSMsg, i);
	NMAddByte(iSMsg, sti(rB.BotNum));
	NMAddByte(iSMsg, NetServer_BotRel(i));
	NMAddWord(iSMsg, sti(rB.Ship.Type));
	NMAddString(iSMsg, rB.NickName, 24);
	NMAddByte(iSMsg, bAnnounce);
	int iBotFaceW = 0;		// COOP s41: the portrait + 1 (0 = none)
	if (CheckAttribute(rB, "BotFace")) { iBotFaceW = sti(rB.BotFace) + 1; }
	NMAddWord(iSMsg, iBotFaceW);
	return iSMsg;
}

void NetServer_BotNote(int wToClientID, int iNote, int iValue)
{
	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_CLIENT);
	NMAddByte(iSMsg, NSC_CLIENT_BOT);
	NMAddByte(iSMsg, NET_BOTOP_NOTE);
	NMAddByte(iSMsg, iNote);
	NMAddByte(iSMsg, iValue);
	NetServer_SendMessage(wToClientID, iSMsg, true);
	NMDelete(iSMsg);
}

// ------------------------------------------------------------------------------------------------
// creation
// ------------------------------------------------------------------------------------------------
// a slot whose ship never existed in this battle (the clients have no ship entity there)
int NetServer_BotFreeSlot()
{
	for (int i=NET_BOT_FIRST; i<NET_MAXCLIENTS; i++)
	{
		if (sti(NSClients[i].Use)) { continue; }
		if (NetServer_IsBot(i)) { continue; }
		if (NetServer_SlotBurned(i)) { continue; }
		if (NetServer_BDOf(i) > 0) { continue; }		// COOP s37: a fighter of a boarding duel (net\Board.c)
		return i;
	}
	return DST_INVALID;
}

bool NetServer_BotPointFree(float fX, float fZ)
{
	float fFree2 = NET_BOT_SPAWN_FREE * NET_BOT_SPAWN_FREE;
	for (int j=0; j<NET_MAXCLIENTS; j++)
	{
		if (!NetServer_HasShip(j)) { continue; }
		if (sti(NSClients[j].Dead)) { continue; }
		float fDX = stf(NSClients[j].Ship.Pos.x) - fX;
		float fDZ = stf(NSClients[j].Ship.Pos.z) - fZ;
		if (fDX * fDX + fDZ * fDZ < fFree2) { return false; }
	}
	return true;
}

void NetServer_BotSetGoods(ref rB, int iShipType)
{
	int j;
	int k;
	int iCan = Net_GetShipCannonsNum(iShipType);
	if (iCan < 4) { iCan = 4; }

	int iBalls = iCan * 20;
	int iGrapes = iCan * 6;
	int iKnippels = iCan * 6;
	int iBombs = iCan * 8;

	// the hold must not be overloaded (speed) and the powder must not be a bomb (NetServer_PowderOverloaded)
	for (k=0; k<8; k++)
	{
		for (j=0; j<Net_GetGoodsNum(); j++)
		{
			ref rGood = Net_GetGoodByIndex(j);
			string sGood = rGood.Name;
			rB.Ship.Goods.(sGood) = 0;
			if (sGood == "Balls") { rB.Ship.Goods.(sGood) = iBalls; }
			if (sGood == "Grapes") { rB.Ship.Goods.(sGood) = iGrapes; }
			if (sGood == "Knippels") { rB.Ship.Goods.(sGood) = iKnippels; }
			if (sGood == "Bombs") { rB.Ship.Goods.(sGood) = iBombs; }
			if (sGood == "Powder") { rB.Ship.Goods.(sGood) = iBalls + iGrapes + iKnippels + iBombs; }
		}
		if (NetServer_ShipLoadRatio(rB) <= 0.7 && !NetServer_PowderOverloaded(rB)) { break; }
		iBalls = iBalls / 2;
		iGrapes = iGrapes / 2;
		iKnippels = iKnippels / 2;
		iBombs = iBombs / 2;
	}
}

// all attributes of the slot, like a player after the buy screen (server\Buy.c: NetServer_OnBuy_FullInfo)
void NetServer_BotSetup(int i, int wOwner, int iShipType, int iCannonType, int iCrew, int iHull, int iSails, int iRel)
{
	if (IsEntity(&NSClients[i])) { DeleteClass(&NSClients[i]); }
	DeleteAttribute(&NSClients[i], "");

	ref rB = NetServer_GetClient(i);
	int iNum = i - NET_BOT_FIRST + 1;
	ref rShip = Net_GetShipByIndex(iShipType);

	rB.index = i;
	rB.id = i;
	rB.Use = false;
	rB.Server = true;
	rB.Bot = true;
	rB.BotNum = iNum;
	rB.BotRel = iRel;
	rB.BotOwner = wOwner;
	rB.BotRemoved = false;
	rB.BotDeadAt = 0;
	rB.BuyReady = true;
	rB.StartReady = true;
	rB.Admin = false;
	rB.Muted = false;
	rB.Parked = false;
	rB.InGame = false;
	rB.Reconnect = false;
	rB.JoinRequested = false;
	rB.Kick = false;
	rB.LastPacketIndex = 0;
	rB.LastPingTime = iPingTime;
	rB.Team = NetServer_BotTeamOf(iRel);
	rB.FriendlyTeam = NetServer_BotTeamOf(iRel);
	rB.NickName = "Бот " + iNum;
	rB.NickPassword = "";
	rB.UserData.Face = "";
	// COOP s41: the portrait of the captain of the bot (a face that has a skin, like the ones of the players): its icon in the
	// battle and the skin of its captain on the boarding deck (net\Board.c)
	rB.BotFace = NetLand_RandomPlayerFace(iServerTime + iNum * 37);		// COOP s45: any face of a player, the same chance (net\Land.c)
	rB.UserData.Flag = "";
	rB.UserData.Sail = "";
	rB.Exp = 0;

	rB.Stat.Sunk = 0;
	rB.Stat.Wreck = 0;
	rB.Stat.Misses = 0;
	rB.Stat.Hits = 0;
	rB.Stat.Won = 0;
	rB.Stat.Lost = 0;
	rB.Stat.DamageInflicted = 0;
	rB.Stat.Deaths = 0;

	// the ship: hull and sails are always the usual ones, all the cannons of the ship
	float fMaxHP = stf(rShip.HP) * (1.0 + stf(NetShipHullUpgrades[0].hp) / 100.0);
	rB.Ship.Type = iShipType;
	rB.Ship.Name = rB.NickName;
	rB.Ship.Sails.Color = argb(255, 255, 255, 255);
	rB.Ship.MaxHP = fMaxHP;
	rB.Ship.HP = fMaxHP * makefloat(iHull) / 100.0;
	rB.Ship.SP = makefloat(iSails);
	rB.Ship.Upgrades.Hull = 1;
	rB.Ship.Upgrades.Sails = 1;
	rB.Ship.Upgrades.Cannons = 1;
	rB.Ship.Cannons.Borts = "";
	rB.Ship.Cannons.Type = iCannonType;
	rB.Ship.Cannons.Charge.Type = 0;			// balls
	rB.Ship.Crew.Quantity = Net_GetCrewCount(iShipType, iCrew);
	rB.Ship.CannonsDumped = 0;					// no Ship.Bort: every place of every bort has its cannon
	rB.Ship.MinesDropped = 0;
	rB.Ship.MineCooldown = 0;
	rB.Ship.BranderActive = 0;
	rB.Ship.BranderArmUntil = 0;
	NetServer_BotSetGoods(rB, iShipType);

	// skills: all 10
	rB.Skills.Accuracy = 10;
	rB.Skills.Cannons = 10;
	rB.Skills.Sailing = 10;
	rB.Skills.Defence = 10;
	rB.Skills.Repair = 10;
	rB.Skills.Grappling = 10;
	rB.Skills.Repair.Active = 0;
	rB.Skills.Repair.Cooldown = iServerTime - 60 * 1000;

	rB.Ship.SpeedRateModifier = 1.0 + (stf(NetShipHullUpgrades[0].speedrate) + stf(NetShipSailUpgrade[0].speedrate)) / 100.0;
	rB.Ship.TurnRateModifier = 1.0 + (stf(NetShipHullUpgrades[0].turnrate) + stf(NetShipSailUpgrade[0].turnrate)) / 100.0;
	rB.Ship.SailsDamageModifier = 1.0 + stf(NetShipSailUpgrade[0].dmgtosails) / 100.0;

	int iBotLevel = 2;
	if (CheckAttribute(&NetServer, "BotSkill")) { iBotLevel = sti(NetServer.BotSkill); }
	NetServer_BotApplySkill(rB, iBotLevel);		// skills, perks and the cannon parameters by the difficulty
}

// the ship entity of the server and the clients (like NetServer_SpawnLateShip + the load message of a late player)
void NetServer_BotSpawn(int i, float fX, float fZ, float fAy)
{
	int iSMsg;
	ref rB = NetServer_GetClient(i);

	rB.Dead = false;
	rB.Life = 0;
	rB.Ship.TurnLeft = 0.0;
	rB.Ship.TurnRight = 0.0;
	rB.Ship.TurnState = 0.0;
	rB.Ship.SailState = 0.5;
	rB.BoardSent = -1;
	rB.Ship.Pos.x = fX;
	rB.Ship.Pos.y = 0.0;
	rB.Ship.Pos.z = fZ;
	rB.Ship.Ang.y = NetServer_AdminNormAngle(fAy);
	rB.Ship.Speed.x = 0.0;
	rB.Ship.Speed.y = 0.0;
	rB.Ship.Speed.z = 0.0;
	rB.LastCollisionShip = -500;
	rB.LastCollisionIsland = -500;
	rB.CoopFix = 0;

	NetServer_CreateShipEntity(i);

	rB.Spawned = true;
	rB.SpawnPos.x = stf(rB.Ship.Pos.x);
	rB.SpawnPos.z = stf(rB.Ship.Pos.z);
	rB.SpawnPos.ay = stf(rB.Ship.Ang.y);
	NetServer_SnapshotLoadout(rB);

	// the brain
	rB.BotLastAy = stf(rB.Ship.Ang.y);
	rB.BotLastCmd = 0.0;
	rB.BotPrevCmd = 0.0;
	rB.BotIslandHit = -100000;
	rB.BotBoardLog = -100000;
	rB.BotEvadeAy = 0.0;
	rB.BotSlowSince = iServerTime;
	rB.BotChargeTime = 0;
	rB.BotWanderAy = stf(rB.Ship.Ang.y);
	rB.BotWanderUntil = iServerTime + 10000;

	iNetBotCount++;

	rB.BotLoop = 1;
	PostEvent("NetServer_ShipUpdateParameters", 1, "l", i);
	NetServer_RestartShipWatch();

	// clients: the names and the relation first, then the ship
	iSMsg = NetServer_BotInfoMessage(i, true);
	NetServer_SendToInGame(iSMsg);
	NMDelete(iSMsg);

	iSMsg = NetServer_CreateClientLoadMessage(i);
	NetServer_SendToInGame(iSMsg);
	NMDelete(iSMsg);

	Trace("COOP: bot " + rB.BotNum + " (slot " + i + ") type=" + rB.Ship.Type + " x=" + rB.Ship.Pos.x + " z=" + rB.Ship.Pos.z);
}

// NSC_COMMAND_ADMIN_BOTADD: the "Add" button of the "Бот+" window
void NetServer_OnAdminBotAdd(int wAdmin, int iMsg)
{
	int iShipType = NMGetWord(iMsg);
	int iCannonType = NMGetByte(iMsg);
	int iCrew = NMGetByte(iMsg);
	int iHull = NMGetByte(iMsg);
	int iSails = NMGetByte(iMsg);
	int iRel = NMGetByte(iMsg);

	if (!NetServer_IsAdmin(wAdmin)) { return; }
	// COOP s30: in the lobby the bot waits in the list of the lobby and comes into the next game with the players
	if (!bServerGameStarted) { NetServer_LBotAdd(wAdmin, iShipType, iCannonType, iCrew, iHull, iSails, iRel); return; }
	if (NetServer_LandMode()) { NetServer_LandAdminBotAdd(wAdmin, iRel, iShipType, iHull + iSails * 256, iCannonType, iCrew); return; }		// COOP: a land duel (Land.c)
	if (!NetServer_AdminShipOk(wAdmin)) { NetServer_BotNote(wAdmin, NET_BOTNOTE_NOSHIP, 0); return; }
	if (iShipType == Net_GetShipIndex("FORT")) { NetServer_FortSpawnAdmin(wAdmin, iRel); return; }		// COOP: the last item of the list is the fort, 700 m in front of the admin (server/Fort.c)

	int i = NetServer_BotFreeSlot();
	if (i == DST_INVALID) { NetServer_BotNote(wAdmin, NET_BOTNOTE_LIMIT, NET_BOT_MAX); return; }

	if (iShipType < 0 || iShipType >= Net_GetShipsNum()) { iShipType = 0; }
	if (iCannonType < 0 || iCannonType >= Net_GetCannonsNum()) { iCannonType = 0; }
	if (iCrew < 10) { iCrew = 10; }
	if (iCrew > 100) { iCrew = 100; }
	if (iHull < 10) { iHull = 10; }
	if (iHull > 100) { iHull = 100; }
	if (iSails < 10) { iSails = 10; }
	if (iSails > 100) { iSails = 100; }
	if (!NetServer_BotRelValid(iRel)) { iRel = NET_BOTREL_ENEMY; }		// COOP s28: a team of a team game too

	// NET_BOT_SPAWN_DIST in front of the bow of the admin; a side step while the point is taken by another ship
	ref rA = NetServer_GetClient(wAdmin);
	float fAy = stf(rA.Ship.Ang.y);
	float fSin = sin(fAy);
	float fCos = cos(fAy);
	float fX0 = stf(rA.Ship.Pos.x) + fSin * NET_BOT_SPAWN_DIST;
	float fZ0 = stf(rA.Ship.Pos.z) + fCos * NET_BOT_SPAWN_DIST;
	float fX = fX0;
	float fZ = fZ0;
	for (int iTry=0; iTry<13; iTry++)
	{
		int iStep = (iTry + 1) / 2;
		float fOff = makefloat(iStep) * NET_BOT_SPAWN_STEP;
		if (iTry - (iTry / 2) * 2 == 0) { fOff = -fOff; }
		fX = fX0 + fCos * fOff;			// the right side of the heading is (cos ay, -sin ay)
		fZ = fZ0 - fSin * fOff;
		if (NetServer_BotPointFree(fX, fZ)) { break; }
	}

	NetServer_BotSetup(i, wAdmin, iShipType, iCannonType, iCrew, iHull, iSails, iRel);
	NetServer_BotSpawn(i, fX, fZ, fAy);
}

// ------------------------------------------------------------------------------------------------
// admin actions on a bot (NSC_COMMAND_ADMIN_CMD, NET_ADMIN_BOT_*)
// ------------------------------------------------------------------------------------------------
void NetServer_BotSetRel(int wAdmin, int i, int iRel)
{
	if (!NetServer_BotRelValid(iRel)) { return; }		// COOP s28: a team of a team game too
	ref rB = NetServer_GetClient(i);
	rB.BotRel = iRel;
	rB.Team = NetServer_BotTeamOf(iRel);
	rB.FriendlyTeam = NetServer_BotTeamOf(iRel);
	if (iRel == NET_BOTREL_ALLY) { rB.BotOwner = wAdmin; }

	int iSMsg = NetServer_BotInfoMessage(i, false);
	NetServer_SendToInGame(iSMsg);
	NMDelete(iSMsg);
}

void NetServer_BotHeal(int i)
{
	ref rB = NetServer_GetClient(i);
	ref rBase = Net_GetShipByIndex(sti(rB.Ship.Type));
	rB.Ship.HP = stf(rB.Ship.MaxHP);
	rB.Ship.SP = 100.0;
	if (stf(rB.Ship.Crew.Quantity) < stf(rBase.MaxCrew)) { rB.Ship.Crew.Quantity = stf(rBase.MaxCrew); }
	NetServer_BotRefill(rB, true);
	NetServer_ShipRebuildHeal(i);		// COOP s27: the fallen masts and the sails with holes come back whole (the ship is created again)
}

// the bot goes in front of the bow of the admin, with the same heading
void NetServer_BotBring(int wAdmin, int i)
{
	if (!NetServer_AdminShipOk(wAdmin)) { return; }
	ref rA = NetServer_GetClient(wAdmin);
	float fAy = stf(rA.Ship.Ang.y);
	float fX = stf(rA.Ship.Pos.x) + sin(fAy) * NET_BOT_TP_DIST;
	float fZ = stf(rA.Ship.Pos.z) + cos(fAy) * NET_BOT_TP_DIST;
	NetServer_AdminPlaceShip(i, fX, fZ, fAy);
	NSClients[i].BotWanderAy = fAy;
	NSClients[i].BotWanderUntil = iServerTime + 10000;
}

// the admin goes beside the bot (abeam, the same course; the side where he is now)
void NetServer_BotGoTo(int wAdmin, int i)
{
	if (!NetServer_AdminShipOk(wAdmin)) { return; }
	ref rB = NetServer_GetClient(i);
	NetServer_AdminSidePoint(NetServer_GetClient(wAdmin), rB);
	NetServer_AdminPlaceShip(wAdmin, fNetAdminSideX, fNetAdminSideZ, fNetAdminSideAy);
}

// ------------------------------------------------------------------------------------------------
// COOP: respawn of the bots (checkbox in the "Bots" window of the admin menu). A killed bot is "wrecked" like a player
// (server\OnShip.c: NetServer_SetClientWrecked) and comes back after the respawn time of the game at a spawn point
// (NetServer_RespawnShip); the bots that sank before the switch stay sunk.
// ------------------------------------------------------------------------------------------------
bool NetServer_BotRespawnOn()
{
	if (!CheckAttribute(&NetServer, "BotRespawn")) { return false; }
	return sti(NetServer.BotRespawn) != 0;
}

// this bot comes back after its death
bool NetServer_BotWillRespawn(int i)
{
	if (!NetServer_BotRespawnOn()) { return false; }
	if (!NetServer_IsBot(i)) { return false; }
	return !NetServer_BotRemoved(i);
}

void NetServer_BotSendOpts(int wTo)
{
	int iOn = 0;
	if (NetServer_BotRespawnOn()) { iOn = 1; }
	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_CLIENT);
	NMAddByte(iSMsg, NSC_CLIENT_BOT);
	NMAddByte(iSMsg, NET_BOTOP_OPTS);
	NMAddByte(iSMsg, iOn);
	int iSk = 2;
	if (CheckAttribute(&NetServer, "BotSkill")) { iSk = sti(NetServer.BotSkill); }
	NMAddByte(iSMsg, iSk);
	int iLate = 0;
	if (NetServer_LateJoinOn()) { iLate = 1; }
	NMAddByte(iSMsg, iLate);		// COOP s30: "Join during the game" (the admin menu shows it)
	NetServer_SendMessage(wTo, iSMsg, true);
	NMDelete(iSMsg);
}

// COOP s30: new players can join the running game (Create game screen; the admin menu changes it: NET_ADMIN_LATEJOIN)
bool NetServer_LateJoinOn()
{
	if (!CheckAttribute(&NetServer, "LateJoin")) { return true; }
	return sti(NetServer.LateJoin) != 0;
}

void NetServer_SetLateJoin(int iOn)
{
	if (iOn != 0) { NetServer.LateJoin = true; } else { NetServer.LateJoin = false; }
	NetServer_BotSendOpts(DST_ALL);
	Trace("COOP: late join = " + NetServer.LateJoin);
}

// COOP: the admin menu "Bots": the difficulty of all the bots now (the sea bots, the forts, the land bots)
void NetServer_BotSetSkill(int iSkill)
{
	if (iSkill < 0) { iSkill = 0; }
	if (iSkill > 3) { iSkill = 3; }
	NetServer.BotSkill = iSkill;
	if (bServerGameStarted && !NetServer_LandMode())
	{
		for (int i=0; i<NET_MAXCLIENTS; i++)
		{
			if (!NetServer_IsBot(i)) { continue; }
			ref rB = NetServer_GetClient(i);
			NetServer_BotApplySkill(rB, iSkill);
		}
	}
	NetServer_BotSendOpts(DST_ALL);
	Trace("COOP: bot difficulty = " + iSkill);
}

// ------------------------------------------------------------------------------------------------
// COOP s30: the bots of the lobby. The "Bot+" window of the lobby (interfaces\Buy.c) puts a bot into this list; every lobby
// shows the list (NET_BOTOP_LOBBY) as rows of the players table. When the game starts each of them comes in like a bot of
// "Bot+" (the sea: at the places of its team, server\OnStart.c NetServer_PickSpawn; the land: as the land "Bot+").
// The list stays for the next games (the lobby after a game shows it again).
// ------------------------------------------------------------------------------------------------
object NSLobbyBots;		// n, b0..: ship, cannon, crew, hull, sails, rel, admin

int NetServer_LBotsNum()
{
	if (!CheckAttribute(&NSLobbyBots, "n")) { return 0; }
	return sti(NSLobbyBots.n);
}

void NetServer_LBotAdd(int wAdmin, int iShip, int iCannon, int iCrew, int iHull, int iSails, int iRel)
{
	int n = NetServer_LBotsNum();
	if (n >= NET_MAXCLIENTS - NET_BOT_FIRST) { NetServer_BotNote(wAdmin, NET_BOTNOTE_LIMIT, NET_MAXCLIENTS - NET_BOT_FIRST); return; }
	if (!NetServer_BotRelValid(iRel)) { iRel = NET_BOTREL_ENEMY; }
	string sB = "b" + n;
	NSLobbyBots.(sB).ship = iShip;
	NSLobbyBots.(sB).cannon = iCannon;
	NSLobbyBots.(sB).crew = iCrew;
	NSLobbyBots.(sB).hull = iHull;
	NSLobbyBots.(sB).sails = iSails;
	NSLobbyBots.(sB).rel = iRel;
	NSLobbyBots.(sB).admin = wAdmin;
	NSLobbyBots.n = n + 1;
	Trace("COOP: lobby bot " + n + " added: ship " + iShip + " rel " + iRel);
	NetServer_LBotsSend(DST_ALL);
}

void NetServer_LBotDel(int k)
{
	int n = NetServer_LBotsNum();
	if (k < 0 || k >= n) { return; }
	for (int j=k; j<n-1; j++)
	{
		string sTo = "b" + j;
		string sFrom = "b" + (j + 1);
		NSLobbyBots.(sTo).ship = NSLobbyBots.(sFrom).ship;
		NSLobbyBots.(sTo).cannon = NSLobbyBots.(sFrom).cannon;
		NSLobbyBots.(sTo).crew = NSLobbyBots.(sFrom).crew;
		NSLobbyBots.(sTo).hull = NSLobbyBots.(sFrom).hull;
		NSLobbyBots.(sTo).sails = NSLobbyBots.(sFrom).sails;
		NSLobbyBots.(sTo).rel = NSLobbyBots.(sFrom).rel;
		NSLobbyBots.(sTo).admin = NSLobbyBots.(sFrom).admin;
	}
	DeleteAttribute(&NSLobbyBots, "b" + (n - 1));
	NSLobbyBots.n = n - 1;
	Trace("COOP: lobby bot " + k + " removed");
	NetServer_LBotsSend(DST_ALL);
}

void NetServer_LBotsSend(int wTo)
{
	int n = NetServer_LBotsNum();
	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_CLIENT);
	NMAddByte(iSMsg, NSC_CLIENT_BOT);
	NMAddByte(iSMsg, NET_BOTOP_LOBBY);
	NMAddByte(iSMsg, n);
	for (int k=0; k<n; k++)
	{
		string sB = "b" + k;
		int iRel = sti(NSLobbyBots.(sB).rel);
		if (!NetServer_BotRelValid(iRel)) { iRel = NET_BOTREL_ENEMY; }		// the teams of the game may have changed
		NMAddByte(iSMsg, iRel);
		NMAddWord(iSMsg, sti(NSLobbyBots.(sB).ship));
		NMAddByte(iSMsg, sti(NSLobbyBots.(sB).cannon));
		NMAddByte(iSMsg, sti(NSLobbyBots.(sB).crew));
		NMAddByte(iSMsg, sti(NSLobbyBots.(sB).hull));
		NMAddByte(iSMsg, sti(NSLobbyBots.(sB).sails));
	}
	NetServer_SendMessage(wTo, iSMsg, true);
	NMDelete(iSMsg);
}

// the game has started (server\OnStart.c: NetServer_StartServerGame): the bots of the lobby come in a little later (after
// the ships / the location of the players)
void NetServer_LBotsSpawnEvent()
{
	int iRound = GetEventData();
	if (!bServerGameStarted) { return; }
	if (iRound != sti(NetServer.Round)) { return; }
	int n = NetServer_LBotsNum();
	for (int k=0; k<n; k++)
	{
		string sB = "b" + k;
		int iShip = sti(NSLobbyBots.(sB).ship);
		int iCannon = sti(NSLobbyBots.(sB).cannon);
		int iCrew = sti(NSLobbyBots.(sB).crew);
		int iHull = sti(NSLobbyBots.(sB).hull);
		int iSails = sti(NSLobbyBots.(sB).sails);
		int iRel = sti(NSLobbyBots.(sB).rel);
		int wAdmin = sti(NSLobbyBots.(sB).admin);
		if (!NetServer_BotRelValid(iRel)) { iRel = NET_BOTREL_ENEMY; }
		if (wAdmin < 0 || wAdmin >= NET_MAXCLIENTS) { wAdmin = 0; }
		if (!sti(NSClients[wAdmin].Use)) { wAdmin = NetServer_LBotAnyPlayer(); }
		if (NetServer_LandMode())
		{
			NetServer_LandAdminBotAdd(wAdmin, iRel, iShip, iHull + iSails * 256, iCannon, iCrew);
			continue;
		}
		if (iShip == Net_GetShipIndex("FORT"))
		{
			if (NetServer_AdminShipOk(wAdmin)) { NetServer_FortSpawnAdmin(wAdmin, iRel); }
			continue;
		}
		int i = NetServer_BotFreeSlot();
		if (i == DST_INVALID) { Trace("COOP: lobby bots: no free slot"); return; }
		if (iShip < 0 || iShip >= Net_GetShipsNum()) { iShip = 0; }
		if (iCannon < 0 || iCannon >= Net_GetCannonsNum()) { iCannon = 0; }
		NetServer_BotSetup(i, wAdmin, iShip, iCannon, iCrew, iHull, iSails, iRel);
		// the places of its team (a deathmatch: of every side)
		NSClients[i].Team = 0;
		if (iRel >= NET_BOTREL_TEAM0) { NSClients[i].Team = iRel - NET_BOTREL_TEAM0; }
		NetServer_PickSpawn(i);
		NetServer_BotSpawn(i, stf(NSSpawn.x), stf(NSSpawn.z), stf(NSSpawn.ay));
		Trace("COOP: lobby bot " + k + " in the game: slot " + i + " rel " + iRel);
	}
}

int NetServer_LBotAnyPlayer()
{
	for (int i=0; i<NET_MAXCLIENTS; i++)
	{
		if (sti(NSClients[i].Use)) { return i; }
	}
	return 0;
}

void NetServer_BotSetRespawn(int iOn)
{
	// COOP s38: the lobby has the switch too (before the game): it stays for the game
	if (iOn != 0) { NetServer.BotRespawn = true; } else { NetServer.BotRespawn = false; }
	NetServer_BotSendOpts(DST_ALL);
	Trace("COOP: bot respawn = " + NetServer.BotRespawn);
}

// the bot is back (server\OnShip.c: NetServer_RespawnShip): the brain starts again, the packets of its ship go on
void NetServer_BotAfterRespawn(int i)
{
	ref rB = NetServer_GetClient(i);
	rB.BotDeadAt = 0;
	rB.BotLastAy = stf(rB.Ship.Ang.y);
	rB.BotLastCmd = 0.0;
	rB.BotPrevCmd = 0.0;
	rB.BotIslandHit = -100000;
	rB.BotEvadeAy = 0.0;
	rB.BotSlowSince = iServerTime;
	rB.BotChargeTime = 0;
	rB.BotWanderAy = stf(rB.Ship.Ang.y);
	rB.BotWanderUntil = iServerTime + 10000;
	rB.SpawnPos.x = stf(rB.Ship.Pos.x);
	rB.SpawnPos.z = stf(rB.Ship.Pos.z);
	rB.SpawnPos.ay = stf(rB.Ship.Ang.y);
	rB.Ship.SailState = 0.5;
	if (!sti(rB.BotLoop))
	{
		rB.BotLoop = 1;
		PostEvent("NetServer_ShipUpdateParameters", 1, "l", i);
	}
	Trace("COOP: bot " + rB.BotNum + " respawned");
}

// TEST: "Delete" of the admin really deletes the NetShip entity of the bot (on the server here, on the clients when they get
// NET_BOTOP_REMOVED with the last byte 1: NetClient_BotShipDelete). false = the old way: the bot sinks and stays
bool bNetBotRealDelete = true;

// "Delete": the bot sinks at once, without a kill line; its number comes free in the next battle
void NetServer_BotRemove(int i)
{
	ref rB = NetServer_GetClient(i);
	if (NetServer_BotRemoved(i)) { return; }
	NetServer_BDLeft(i);			// COOP s41: its captain was on the boarding deck: the player wins
	NetServer_PrizeSlotGone(i);		// COOP s41: a prize / a captor that waits

	rB.BotRemoved = true;
	rB.BotDeadAt = iServerTime;
	rB.Ship.TurnLeft = 0.0;
	rB.Ship.TurnRight = 0.0;
	rB.Ship.TurnState = 0.0;
	rB.Ship.SailState = 0.0;

	if (bNetBotRealDelete)
	{
		// ship.dll asserts (ship.cpp: pASinkSpeed) when Dead is set and Ship.Sink.Speed does not exist yet
		rB.Ship.Sink.Speed.x = 0.0;
		rB.Ship.Sink.Speed.y = 0.9;
		rB.Ship.Sink.Speed.z = 0.0;
		rB.Dead = true;
		rB.Spawned = false;		// NetServer_HasShip() is false now: no brain, no packets, not a target
		int iDMsg = NMCreate();
		NMAddByte(iDMsg, NC_CLIENT);
		NMAddByte(iDMsg, NSC_CLIENT_BOT);
		NMAddByte(iDMsg, NET_BOTOP_REMOVED);
		NMAddClientID(iDMsg, i);
		NMAddFloat(iDMsg, 0.0);
		NMAddFloat(iDMsg, 0.0);
		NMAddFloat(iDMsg, 0.0);
		NMAddByte(iDMsg, 1);		// 1 = the ship is deleted on the clients too
		NetServer_SendToInGame(iDMsg);
		NMDelete(iDMsg);
		Trace("COOP: bot " + rB.BotNum + " delete: DeleteClass of the server NetShip");
		if (IsEntity(&NSClients[i])) { DeleteClass(&NSClients[i]); }
		Trace("COOP: bot " + rB.BotNum + " deleted (entity)");
		return;
	}

	float fSX = 0.01 * (frnd() * 2.0 - 1.0);
	float fSY = 0.9;
	float fSZ = 0.01 * (frnd() * 2.0 - 1.0);
	if (!sti(rB.Dead))
	{
		rB.Ship.HP = 0.0;
		rB.Ship.Crew.Quantity = 0;
		rB.Life = NetServer_GetShipLife(i) + 1;
		// the sink speed must exist before Dead (net.dll starts the sinking when Dead is set)
		rB.Ship.Sink.Speed.x = fSX;
		rB.Ship.Sink.Speed.y = fSY;
		rB.Ship.Sink.Speed.z = fSZ;
		rB.Dead = true;
	}

	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_CLIENT);
	NMAddByte(iSMsg, NSC_CLIENT_BOT);
	NMAddByte(iSMsg, NET_BOTOP_REMOVED);
	NMAddClientID(iSMsg, i);
	NMAddFloat(iSMsg, fSX);
	NMAddFloat(iSMsg, fSY);
	NMAddFloat(iSMsg, fSZ);
	NMAddByte(iSMsg, 0);
	NetServer_SendToInGame(iSMsg);
	NMDelete(iSMsg);
	Trace("COOP: bot " + rB.BotNum + " removed");
}

void NetServer_AdminBotAction(int wAdmin, int iAction, int i, float fValue)
{
	if (!bServerGameStarted) { return; }
	if (i == NET_FORTCLIENT) { NetServer_AdminFortAction(wAdmin, iAction, fValue); return; }		// COOP: the fort of Bot+ in the table
	if (!NetServer_IsBot(i)) { return; }
	if (NetServer_BotRemoved(i)) { return; }

	if (iAction == NET_ADMIN_BOT_DELETE) { NetServer_BotRemove(i); return; }
	// COOP: "Heal" brings a sunk bot back
	if (iAction == NET_ADMIN_BOT_HEAL && !NetServer_BotAlive(i)) { NetServer_RespawnShip(i); NetServer_BotAfterRespawn(i); return; }
	if (!NetServer_BotAlive(i)) { return; }

	switch (iAction)
	{
		case NET_ADMIN_BOT_REL:		NetServer_BotSetRel(wAdmin, i, makeint(fValue + 0.5));		break;
		case NET_ADMIN_BOT_BRING:	NetServer_BotBring(wAdmin, i);								break;
		case NET_ADMIN_BOT_GOTO:	NetServer_BotGoTo(wAdmin, i);								break;
		case NET_ADMIN_BOT_HEAL:	NetServer_BotHeal(i);										break;
		case NET_ADMIN_BOT_DESTROY:	NetServer_SetClientDead(NetServer_GetClient(i), NETKILL_BY_LIGHTNING, DST_INVALID);	break;
	}
}

// ------------------------------------------------------------------------------------------------
// fire
// ------------------------------------------------------------------------------------------------
// does the ship s shoot at the ship t (both have a ship)
bool NetServer_ShouldTarget(int s, int t)
{
	bool bTBot = NetServer_IsBot(t);
	int iTRel = 0;
	if (bTBot)
	{
		if (NetServer_BotRemoved(t)) { return false; }
		iTRel = NetServer_BotRel(t);
	}

	// COOP: the fort fires at every ship of another team than its FriendlyTeam, like the engine does (aim assist of the hard fort)
	if (s == NET_FORTCLIENT)
	{
		if (!CheckAttribute(&NSClients[NET_FORTCLIENT], "FriendlyTeam")) { return false; }
		return sti(NSClients[t].Team) != sti(NSClients[NET_FORTCLIENT].FriendlyTeam);
	}

	if (NetServer_IsBot(s))
	{
		int iSRel = NetServer_BotRel(s);
		if (iSRel == NET_BOTREL_ALLY)
		{
			if (bTBot && iTRel == NET_BOTREL_ENEMY) { return true; }
			return false;
		}
		if (iSRel == NET_BOTREL_ENEMY)
		{
			if (!bTBot) { return true; }
			if (iTRel == NET_BOTREL_ALLY) { return true; }
			if (iTRel >= NET_BOTREL_TEAM0) { return true; }		// COOP s28: the team bots are like the players
			return false;
		}
		if (iSRel >= NET_BOTREL_TEAM0)
		{
			// COOP s28: a team bot: the players and the team bots of the other teams, the enemy bots
			if (bTBot)
			{
				if (iTRel == NET_BOTREL_ENEMY) { return true; }
				if (iTRel >= NET_BOTREL_TEAM0) { return iTRel != iSRel; }
				return false;
			}
			return sti(NSClients[t].Team) != iSRel - NET_BOTREL_TEAM0;
		}
		return false;
	}

	// a player: enemy bots only (allies and neutrals are not aimed at), players as the engine does it
	if (bTBot)
	{
		if (iTRel == NET_BOTREL_ENEMY) { return true; }
		if (iTRel >= NET_BOTREL_TEAM0) { return sti(NSClients[s].Team) != iTRel - NET_BOTREL_TEAM0; }		// COOP s28
		return false;
	}
	if (sti(NetServer.GameType) == NETGAME_DEATHMATCH) { return true; }
	return sti(NSClients[s].Team) != sti(NSClients[t].Team);
}

// the "outside camera" fire of the engine (it aims every loaded bort at the nearest enemy in its zone) with our idea of
// an enemy: for this one call the Team of every other ship is "target" or "not a target", then it is given back
void NetServer_FireOutsideEngine(int s)
{
	int j;
	ref rS = NetServer_GetClient(s);

	for (j=0; j<NET_MAXCLIENTS; j++)
	{
		if (j == s) { continue; }
		if (!NetServer_HasShip(j)) { continue; }
		NSClients[j].FireTgt = NetServer_ShouldTarget(s, j);
		if (sti(NSClients[j].Dead)) { NSClients[j].FireTgt = false; }		// COOP s27: a sunk ship is no target of the third person fire
	}
	for (j=0; j<NET_MAXCLIENTS; j++)
	{
		if (j == s) { continue; }
		if (!NetServer_HasShip(j)) { continue; }
		NSClients[j].FireTeamSave = sti(NSClients[j].Team);
		if (sti(NSClients[j].FireTgt)) { NSClients[j].Team = NET_BOT_TEAM_TARGET; }
		else { NSClients[j].Team = NET_BOT_TEAM_FRIEND; }
	}

	int iFriendly = sti(rS.FriendlyTeam);
	rS.FriendlyTeam = NET_BOT_TEAM_FRIEND;
	rS.DoCannonFireCameraOutside = "";
	rS.FriendlyTeam = iFriendly;

	for (j=0; j<NET_MAXCLIENTS; j++)
	{
		if (j == s) { continue; }
		if (!NetServer_HasShip(j)) { continue; }
		if (!CheckAttribute(&NSClients[j], "FireTeamSave")) { continue; }
		NSClients[j].Team = sti(NSClients[j].FireTeamSave);
	}
}

// ------------------------------------------------------------------------------------------------
// Lead aim. The "outside camera" fire of the engine aims at the place where the target IS, so a sailing ship is
// always missed behind its stern (players and bots alike). Here every quarter of the shooter (bow, stern, left, right)
// fires at the nearest target in range, aimed at the place where it WILL BE when the balls come down: the engine
// fire "from the deck camera" is used, its camera looks along the shot with the elevation of the needed range.
// false = no target for us (the fire of the engine is used then: the fort, ships out of range).
// ------------------------------------------------------------------------------------------------
bool bNetLeadAim = true;		// false = the fire of the engine as before

float NetServer_Asin(float f)
{
	if (f > 1.0) { f = 1.0; }
	if (f < -1.0) { f = -1.0; }
	return atan2(f, sqrt(1.0 - f * f));
}

void NetServer_FireAt(int s, int t)
{
	ref rS = NetServer_GetClient(s);
	ref rT = NetServer_GetClient(t);
	float fV0 = stf(rS.Ship.Cannons.SpeedV0);
	float fX = stf(rS.Ship.Pos.x);
	float fZ = stf(rS.Ship.Pos.z);
	float fTX = stf(rT.Ship.Pos.x);
	float fTZ = stf(rT.Ship.Pos.z);

	// the place of the target when the balls come down (server\OnBall.c: velocity measured from the positions, turn rate)
	float fPX = fTX;
	float fPZ = fTZ;
	float fTheta = 0.0;
	for (int k=0; k<3; k++)
	{
		float fD = sqrt((fPX - fX) * (fPX - fX) + (fPZ - fZ) * (fPZ - fZ));
		fTheta = 0.5 * NetServer_Asin(9.81 * fD / (fV0 * fV0));
		float fFlight = 2.0 * fV0 * sin(fTheta) / 9.81;
		fFlight = fFlight / NetServer_BallMult();		// the balls fly SpeedMultiply times faster than the plain physics
		NetServer_AimPredict(t, fFlight);
		fPX = stf(NSAim.PX);
		fPZ = stf(NSAim.PZ);
	}
	// a strange speed must not throw the shot away: the lead is not longer than a third of the distance
	float fLead = sqrt((fPX - fTX) * (fPX - fTX) + (fPZ - fTZ) * (fPZ - fTZ));
	float fDist0 = sqrt((fTX - fX) * (fTX - fX) + (fTZ - fZ) * (fTZ - fZ));
	if (fLead > fDist0 * 0.35 && fLead > 0.01)
	{
		float fK = fDist0 * 0.35 / fLead;
		fPX = fTX + (fPX - fTX) * fK;
		fPZ = fTZ + (fPZ - fTZ) * fK;
	}
	float fA = atan2(fPX - fX, fPZ - fZ);

	EntityUpdate(0);
	rS.Camera.Pos.x = fX;
	rS.Camera.Pos.y = stf(rS.Ship.Pos.y) + 10.0;
	rS.Camera.Pos.z = fZ;
	rS.Camera.Dir.x = sin(fA) * cos(fTheta);
	rS.Camera.Dir.y = sin(fTheta);
	rS.Camera.Dir.z = cos(fA) * cos(fTheta);
	EntityUpdate(1);
	rS.DoCannonFire = "";
}

// COOP s28: the fire of a team bot at the fort of Capture the fort: the deck camera of the engine looks at the middle of the
// fort with the elevation of the distance (the fort does not move), only when the fort is abeam (the side cannons).
// The "outside camera" fire of the engine did not take the fort for a target (the bots fired a little, then never again).
bool NetServer_BotFireAtFort(int s)
{
	ref rS = NetServer_GetClient(s);
	if (!CheckAttribute(rS, "Ship.Cannons.SpeedV0")) { return false; }
	float fV0 = stf(rS.Ship.Cannons.SpeedV0);
	if (fV0 < 1.0) { return false; }
	float fX = stf(rS.Ship.Pos.x);
	float fZ = stf(rS.Ship.Pos.z);
	float fDX = stf(NetServer.FortX) - fX;
	float fDZ = stf(NetServer.FortZ) - fZ;
	float fD = sqrt(fDX * fDX + fDZ * fDZ);
	float fA = atan2(fDX, fDZ);
	float fRel = NetServer_BotAbs(NetServer_BotAngDiff(fA, stf(rS.Ship.Ang.y)));
	if (fRel < PI * 0.3 || fRel > PI * 0.7) { return false; }
	float fTheta = 0.5 * NetServer_Asin(9.81 * fD / (fV0 * fV0));

	EntityUpdate(0);
	rS.Camera.Pos.x = fX;
	rS.Camera.Pos.y = stf(rS.Ship.Pos.y) + 10.0;
	rS.Camera.Pos.z = fZ;
	rS.Camera.Dir.x = sin(fA) * cos(fTheta);
	rS.Camera.Dir.y = sin(fTheta);
	rS.Camera.Dir.z = cos(fA) * cos(fTheta);
	EntityUpdate(1);
	rS.DoCannonFire = "";
	return true;
}

bool NetServer_FireLead(int s)
{
	if (!bNetLeadAim) { return false; }
	ref rS = NetServer_GetClient(s);
	if (!CheckAttribute(rS, "Ship.Cannons.SpeedV0")) { return false; }
	float fV0 = stf(rS.Ship.Cannons.SpeedV0);
	if (fV0 < 1.0) { return false; }
	float fRange = fV0 * fV0 / 9.81;
	float fX = stf(rS.Ship.Pos.x);
	float fZ = stf(rS.Ship.Pos.z);
	float fAy = stf(rS.Ship.Ang.y);

	DeleteAttribute(rS, "FireQ");
	for (int j=0; j<NET_MAXCLIENTS; j++)
	{
		if (j == s) { continue; }
		if (!NetServer_ShipIsAlive(j)) { continue; }
		if (!NetServer_ShouldTarget(s, j)) { continue; }
		float fDX = stf(NSClients[j].Ship.Pos.x) - fX;
		float fDZ = stf(NSClients[j].Ship.Pos.z) - fZ;
		float fD = sqrt(fDX * fDX + fDZ * fDZ);
		if (fD > fRange) { continue; }

		float fRel = NetServer_BotAbs(NetServer_BotAngDiff(atan2(fDX, fDZ), fAy));
		// only abeam (the side cannons); bow / stern targets are left to the fire of the engine
		if (fRel < PI * 0.3 || fRel > PI * 0.7) { continue; }
		string sQ = "q2";
		if (NetServer_BotAngDiff(atan2(fDX, fDZ), fAy) > 0.0) { sQ = "q3"; }
		if (CheckAttribute(rS, "FireQ." + sQ + ".d"))
		{
			if (stf(rS.FireQ.(sQ).d) <= fD) { continue; }
		}
		rS.FireQ.(sQ).d = fD;
		rS.FireQ.(sQ).id = j;
	}

	bool bFired = false;
	for (int q=0; q<4; q++)
	{
		string sQq = "q" + q;
		if (!CheckAttribute(rS, "FireQ." + sQq + ".id")) { continue; }
		NetServer_FireAt(s, sti(rS.FireQ.(sQq).id));
		bFired = true;
	}
	DeleteAttribute(rS, "FireQ");
	return bFired;
}

// outside camera fire of a player (server\OnShip.c) or of a bot: lead aim, the fire of the engine if there is no target
void NetServer_FireOutside(int s)
{
	bool bLead = true;
	if (NetServer_IsBot(s)) { if (NetServer_BotSkillLevel(NetServer_GetClient(s)) <= 1) { bLead = false; } }
	if (bLead) { NetServer_FireLead(s); }
	NetServer_FireOutsideEngine(s);		// the cannons still loaded (bow, stern, sides without a lead target)
}

// ------------------------------------------------------------------------------------------------
// the brain
// ------------------------------------------------------------------------------------------------
void NetServer_BotsTick()
{
	int iGen = GetEventData();
	if (iGen != iNetBotGen) { return; }

	if (bServerGameStarted && iNetBotCount > 0)
	{
		for (int iTr=0; iTr<NET_MAXCLIENTS; iTr++)
		{
			if (NetServer_ShipIsAlive(iTr)) { NetServer_AimTrack(iTr); }		// velocities of the ships for the lead (server\OnBall.c)
		}
		for (int i=NET_BOT_FIRST; i<NET_MAXCLIENTS; i++)
		{
			if (!NetServer_IsBot(i)) { continue; }
			NetServer_BotThink(i);
		}
	}

	PostEvent("NetServer_BotsTick", NET_BOT_TICK, "l", iGen);
}

// the ship info packets of a sunk bot stop after a while (server\OnShip.c: NetServer_ShipUpdateParameters)
bool NetServer_BotKeepUpdating(int i)
{
	if (!NetServer_IsBot(i)) { return false; }
	if (!NetServer_HasShip(i)) { return false; }
	if (!sti(NSClients[i].Dead)) { return true; }
	if (NetServer_BotWillRespawn(i)) { return true; }		// COOP: a wreck that comes back
	if (sti(NSClients[i].BotDeadAt) <= 0) { NSClients[i].BotDeadAt = iServerTime; }
	int iDeadFor = iServerTime - sti(NSClients[i].BotDeadAt);
	if (iDeadFor < NET_BOT_DEAD_KEEP) { return true; }
	return false;
}

// "always enough charges": the charges and the powder come back when they run low
void NetServer_BotRefill(ref rB, bool bForce)
{
	int j;
	bool bLow = bForce;
	for (j=0; j<4; j++)
	{
		ref rGood = Net_GetGoodByIndex(j);
		string sGood = rGood.Name;
		if (!CheckAttribute(rB, "Ship.Start.Goods." + sGood)) { continue; }
		if (sti(rB.Ship.Start.Goods.(sGood)) <= 0) { continue; }
		if (sti(rB.Ship.Goods.(sGood)) < sti(rB.Ship.Start.Goods.(sGood)) / 5) { bLow = true; }
	}
	if (!CheckAttribute(rB, "Ship.Goods.Powder") || sti(rB.Ship.Goods.Powder) < 10) { bLow = true; }
	if (!bLow) { return; }

	DeleteAttribute(rB, "Ship.GoodsHeld");
	for (j=0; j<Net_GetGoodsNum(); j++)
	{
		ref rG = Net_GetGoodByIndex(j);
		string sG = rG.Name;
		if (!CheckAttribute(rB, "Ship.Start.Goods." + sG)) { continue; }
		rB.Ship.Goods.(sG) = sti(rB.Ship.Start.Goods.(sG));
	}
	rB.Ship.AmmoLast = NetServer_AmmoSum(rB);		// the powder tracking must not take the refill for unloaded cannons
}

// nearest ship this bot fights (DST_INVALID - none), its distance in BotTargetDist
// COOP: the ally and the players: an ally attacks only the enemies that are within NET_ALLY_LEASH of the nearest player (the
// radius round the bot itself is unlimited, but an enemy far from every player is not a reason to leave them), and when no
// enemy is near it escorts the nearest player.
#define NET_ALLY_LEASH			2500.0

float fNetBotNearPX = 0.0;
float fNetBotNearPZ = 0.0;

// the nearest alive player of the bot i -> fNetBotNearPX / PZ; false when there is none
bool NetServer_BotNearestPlayer(int i)
{
	ref rB = NetServer_GetClient(i);
	float fX = stf(rB.Ship.Pos.x);
	float fZ = stf(rB.Ship.Pos.z);
	float fBest = 1000000.0;
	bool bFound = false;
	for (int j=0; j<NET_MAXPLAYERS; j++)
	{
		if (!sti(NSClients[j].Use)) { continue; }
		if (!NetServer_ShipIsAlive(j)) { continue; }
		float fD = GetDistance2D(fX, fZ, stf(NSClients[j].Ship.Pos.x), stf(NSClients[j].Ship.Pos.z));
		if (fD < fBest)
		{
			fBest = fD;
			fNetBotNearPX = stf(NSClients[j].Ship.Pos.x);
			fNetBotNearPZ = stf(NSClients[j].Ship.Pos.z);
			bFound = true;
		}
	}
	return bFound;
}

int NetServer_BotFindTarget(int i)
{
	ref rB = NetServer_GetClient(i);
	float fX = stf(rB.Ship.Pos.x);
	float fZ = stf(rB.Ship.Pos.z);
	int wBest = DST_INVALID;
	float fBest = NetServer_BotSeeDist(NetServer_BotSkillLevel(rB));
	bool bLeash = false;
	if (NetServer_BotRel(i) == NET_BOTREL_ALLY) { bLeash = NetServer_BotNearestPlayer(i); }

	for (int j=0; j<NET_MAXCLIENTS; j++)
	{
		if (j == i) { continue; }
		if (!NetServer_ShipIsAlive(j)) { continue; }
		if (!NetServer_ShouldTarget(i, j)) { continue; }
		if (bLeash)
		{
			if (GetDistance2D(fNetBotNearPX, fNetBotNearPZ, stf(NSClients[j].Ship.Pos.x), stf(NSClients[j].Ship.Pos.z)) > NET_ALLY_LEASH) { continue; }
		}
		float fD = GetDistance2D(fX, fZ, stf(NSClients[j].Ship.Pos.x), stf(NSClients[j].Ship.Pos.z));
		if (fD < fBest)
		{
			fBest = fD;
			wBest = j;
		}
	}
	rB.BotTargetDist = fBest;
	return wBest;
}

// distance of the fire of the bot
float NetServer_BotRange(ref rB)
{
	float fRange = 0.0;
	if (CheckAttribute(rB, "Ship.Cannons.Borts.cannonl.MaxFireDistance")) { fRange = stf(rB.Ship.Cannons.Borts.cannonl.MaxFireDistance); }
	if (fRange < 50.0)
	{
		ref rCannon = Net_GetCannonByIndex(sti(rB.Ship.Cannons.Type));
		if (CheckAttribute(rCannon, "FireRange")) { fRange = stf(rCannon.FireRange); }
	}
	if (fRange < 50.0) { fRange = 400.0; }
	return fRange;
}

// grapes close to the enemy, balls farther
void NetServer_BotCharge(int i, float fDist, float fRange)
{
	ref rB = NetServer_GetClient(i);
	if (iServerTime < sti(rB.BotChargeTime)) { return; }

	string sWant = "Balls";
	if (fDist < fRange * 0.35) { sWant = "Grapes"; }

	ref rCur = Net_GetGoodByIndex(sti(rB.Ship.Cannons.Charge.Type));
	string sCur = rCur.Name;
	if (sCur == sWant) { return; }
	if (!CheckAttribute(rB, "Ship.Goods." + sWant)) { return; }
	if (sti(rB.Ship.Goods.(sWant)) <= 0) { return; }

	rB.BotChargeTime = iServerTime + 20000;
	NetServer_ShipChangeCharge(i, sWant);
}

// the turn keys of the bot (TurnState = TurnRight - TurnLeft like NetServer_OnShip)
void NetServer_BotSteer(ref rB, float fDesired, float fAy)
{
	float fD = NetServer_BotAngDiff(fDesired, fAy);
	float fCmd = 0.0;
	if (fD > 0.06) { fCmd = fNetBotTurnK; }
	if (fD < -0.06) { fCmd = -fNetBotTurnK; }

	if (fCmd > 0.0)
	{
		rB.Ship.TurnRight = 1;
		rB.Ship.TurnLeft = 0;
	}
	else
	{
		rB.Ship.TurnRight = 0;
		rB.Ship.TurnLeft = 0;
		if (fCmd < 0.0) { rB.Ship.TurnLeft = 1; }
	}
	rB.Ship.TurnState = stf(rB.Ship.TurnRight) - stf(rB.Ship.TurnLeft);
	rB.BotPrevCmd = stf(rB.BotLastCmd);
	rB.BotLastCmd = stf(rB.Ship.TurnState);
	rB.BotLastAy = fAy;
}

// Which way does TurnState > 0 turn a ship? Learned from the bots themselves: a key held for two ticks must turn the ship
// to the side we wanted. Five wrong answers in a row (not the inertia of one turn) flip the sign for all bots.
void NetServer_BotLearnTurn(ref rB, float fAy)
{
	float fCmd = stf(rB.BotLastCmd);
	if (fCmd == 0.0) { return; }
	if (stf(rB.BotPrevCmd) != fCmd) { return; }

	float fDelta = NetServer_BotAngDiff(fAy, stf(rB.BotLastAy));
	if (fDelta < 0.03 && fDelta > -0.03) { return; }

	// we wanted the angle to change by the sign of fCmd * fNetBotTurnK
	if (fDelta * fCmd * fNetBotTurnK < 0.0)
	{
		iNetBotTurnBad++;
		if (iNetBotTurnBad >= 8)
		{
			fNetBotTurnK = -fNetBotTurnK;
			iNetBotTurnBad = 0;
			Trace("COOP: bots turn sign is now " + fNetBotTurnK);
		}
	}
	else
	{
		iNetBotTurnBad = 0;
	}
}

// server\OnShip.c: NetServer_Ship2IslandCollision
void NetServer_BotIslandHit(int i, float x, float z)
{
	ref rB = NetServer_GetClient(i);
	if (iServerTime - sti(rB.BotIslandHit) < 1500) { return; }
	rB.BotIslandHit = iServerTime;
	rB.BotEvadeAy = atan2(stf(rB.Ship.Pos.x) - x, stf(rB.Ship.Pos.z) - z);		// away from the shore
}

// ------------------------------------------------------------------------------------------------
// difficulty (Create game screen: "Боты": 0 безобидный, 1 легкий, 2 средний, 3 тяжелый)
// ------------------------------------------------------------------------------------------------
int NetServer_BotSkillLevel(ref rB)
{
	if (CheckAttribute(rB, "BotSkill")) { return sti(rB.BotSkill); }
	return 2;
}

int NetServer_BotSkillValue(int iLevel)
{
	switch (iLevel)
	{
		case 0: return 1; break;
		case 1: return 4; break;
		case 2: return 7; break;
	}
	return 10;
}

float NetServer_BotSeeDist(int iLevel)
{
	switch (iLevel)
	{
		case 0: return 0.0; break;
		case 1: return 1000000.0; break;		// COOP: a bot always knows where its enemies are
		case 2: return 1000000.0; break;
	}
	return 1000000.0;
}

// the distance a bot keeps from its target, part of its range of fire
float NetServer_BotKeepPart(int iLevel)
{
	switch (iLevel)
	{
		case 1: return 0.5; break;
		case 2: return 0.38; break;
	}
	return 0.28;
}

// it boards when its crew is so many times bigger than the enemy crew (0 = never)
float NetServer_BotBoardRatio(int iLevel)
{
	switch (iLevel)
	{
		case 2: return 1.6; break;
		case 3: return 1.2; break;
	}
	return 0.0;
}

// angle in -PI..PI (the ships turn many times: the heading of a ship is not limited)
float NetServer_BotNormAng(float f)
{
	if (f > 1000.0 || f < -1000.0) { return 0.0; }
	while (f > PI) { f = f - PIm2; }
	while (f < -PI) { f = f + PIm2; }
	return f;
}

// a bot boards when it wins by the strength of the boarding fight (the same formula as the real fight: skill * crew),
// the margin: 1.0 = it is enough to be stronger (0 = never)
float NetServer_BotBoardMargin(int iLevel)
{
	switch (iLevel)
	{
		case 1: return 1.5; break;
		case 2: return 1.1; break;
		case 3: return 1.0; break;
	}
	return 0.0;
}

bool NetServer_BotWinsBoarding(ref rB, ref rT, int iLevel)
{
	float fMargin = NetServer_BotBoardMargin(iLevel);
	if (fMargin <= 0.0) { return false; }
	if (sti(NetServer.NoBoarding)) { return false; }
	float fCrewB = stf(rB.Ship.Crew.Quantity);
	float fCrewT = stf(rT.Ship.Crew.Quantity);
	if (fCrewB < 10.0) { return false; }
	float fPB = 0.1 + NetServer_GetGrappling(rB) * fCrewB;
	float fPT = 0.1 + NetServer_GetGrappling(rT) * fCrewT;
	return fPB > fPT * fMargin;
}

// sails by the time to the place, not by metres: the speed of the ships depends on the game speed.
// more than fSecFull seconds to go: full sails, more than fSecHalf: half, less: the sails are down (it coasts)
float NetServer_BotSailByTime(float fGap, float fSpeed, float fSecFull, float fSecHalf)
{
	if (fSpeed < 5.0) { fSpeed = 5.0; }
	float fSec = fGap / fSpeed;
	if (fSec > fSecFull) { return 1.0; }
	if (fSec > fSecHalf) { return 0.5; }
	return 0.0;
}

void NetServer_BotApplySkill(ref rB, int iLevel)
{
	int iV = NetServer_BotSkillValue(iLevel);
	rB.BotSkill = iLevel;
	rB.Skills.Accuracy = iV;
	rB.Skills.Cannons = iV;
	rB.Skills.Sailing = iV;
	rB.Skills.Defence = iV;
	rB.Skills.Repair = iV;
	rB.Skills.Grappling = iV;
	if (iLevel >= 2) { rB.Skills.Grappling = 10; rB.Perks.GrapplingProfessional = 1; rB.Perks.LongRangeGrappling = 1; }		// COOP: boarding bots grapple from far, whatever the angle and the speed
	NetServer_CannonRecalculateParameters(rB);
	NetServer_CalculateFireTime(rB);
	NetServer_CalculateReloadTime(rB);
}

// ------------------------------------------------------------------------------------------------
// islands. net.dll has no "is there land" question for the scripts, so the bots keep a map of the shore:
// the ports of the island (their reload points are on the coast), a circle inside the island around them and
// every point where a ship (any ship) has touched the island. A bot looks ahead along its course and turns away.
// ------------------------------------------------------------------------------------------------
object NSBotShore;

void NetServer_BotShoreReset()
{
	DeleteAttribute(&NSBotShore, "");
	NSBotShore.Num = 0;
	NSBotShore.NearNum = 0;
	NSBotShore.Ready = false;
}

void NetServer_BotShoreAdd(float x, float z, float fRadius)
{
	if (!CheckAttribute(&NSBotShore, "Num")) { NSBotShore.Num = 0; }
	// the collision event comes every frame while a ship touches the shore: the last point is checked first
	if (CheckAttribute(&NSBotShore, "lastx"))
	{
		float fLX = stf(NSBotShore.lastx) - x;
		float fLZ = stf(NSBotShore.lastz) - z;
		if (fLX * fLX + fLZ * fLZ < 6400.0) { return; }
	}
	NSBotShore.lastx = x;
	NSBotShore.lastz = z;
	int iNum = sti(NSBotShore.Num);
	// one point per 80 m is enough
	for (int k=0; k<iNum; k++)
	{
		string sK = "p" + k;
		float fDX = stf(NSBotShore.(sK).x) - x;
		float fDZ = stf(NSBotShore.(sK).z) - z;
		if (fDX * fDX + fDZ * fDZ < 6400.0) { return; }
	}
	if (iNum >= 150) { return; }
	string sN = "p" + iNum;
	NSBotShore.(sN).x = x;
	NSBotShore.(sN).z = z;
	NSBotShore.(sN).r = fRadius;
	NSBotShore.Num = iNum + 1;
}

void NetServer_BotShoreInit()
{
	if (CheckAttribute(&NSBotShore, "Ready") && sti(NSBotShore.Ready)) { return; }
	if (!CheckAttribute(&NSBotShore, "Num")) { NSBotShore.Num = 0; }
	NSBotShore.Ready = true;
	NSBotShore.HasCircle = false;
	if (NetServer.IslandID == "") { return; }

	ref rIsl = Net_GetIsland(NetServer.IslandID);
	if (!CheckAttribute(rIsl, "reload")) { return; }
	aref arRel;
	makearef(arRel, rIsl.reload);
	int iNum = GetAttributesNum(arRel);
	float fSX = 0.0;
	float fSZ = 0.0;
	int iPoints = 0;
	int k;
	for (k=0; k<iNum; k++)
	{
		aref arL = GetAttributeN(arRel, k);
		if (!CheckAttribute(arL, "x") || !CheckAttribute(arL, "z")) { continue; }
		fSX = fSX + stf(arL.x);
		fSZ = fSZ + stf(arL.z);
		iPoints++;
		NetServer_BotShoreAdd(stf(arL.x), stf(arL.z), 140.0);
	}
	if (iPoints < 2) { return; }
	float fCX = fSX / makefloat(iPoints);
	float fCZ = fSZ / makefloat(iPoints);
	float fMin = 100000.0;
	for (k=0; k<iNum; k++)
	{
		aref arL2 = GetAttributeN(arRel, k);
		if (!CheckAttribute(arL2, "x") || !CheckAttribute(arL2, "z")) { continue; }
		float fD = GetDistance2D(fCX, fCZ, stf(arL2.x), stf(arL2.z));
		if (fD < fMin) { fMin = fD; }
	}
	NSBotShore.HasCircle = true;
	NSBotShore.cx = fCX;
	NSBotShore.cz = fCZ;
	NSBotShore.cr = fMin * 0.85;
	Trace("COOP: bots shore: " + iPoints + " ports, island circle r=" + NSBotShore.cr);
}

// is there land (as far as we know) at this point; fMargin = safety distance
bool NetServer_BotIsLand(float x, float z, float fMargin)
{
	if (CheckAttribute(&NSBotShore, "HasCircle") && sti(NSBotShore.HasCircle))
	{
		if (GetDistance2D(x, z, stf(NSBotShore.cx), stf(NSBotShore.cz)) < stf(NSBotShore.cr) + fMargin) { return true; }
	}
	// only the shore points near the bot (NetServer_BotAvoidLand fills the list)
	int iNum = sti(NSBotShore.NearNum);
	for (int k=0; k<iNum; k++)
	{
		string sN = "n" + k;
		string sK = NSBotShore.Near.(sN);
		float fDX = stf(NSBotShore.(sK).x) - x;
		float fDZ = stf(NSBotShore.(sK).z) - z;
		float fR = stf(NSBotShore.(sK).r) + fMargin;
		if (fDX * fDX + fDZ * fDZ < fR * fR) { return true; }
	}
	return false;
}

bool NetServer_BotCourseFree(float x, float z, float fAy)
{
	float fSin = sin(fAy);
	float fCos = cos(fAy);
	if (NetServer_BotIsLand(x + fSin * 120.0, z + fCos * 120.0, 30.0)) { return false; }
	if (NetServer_BotIsLand(x + fSin * 260.0, z + fCos * 260.0, 30.0)) { return false; }
	if (NetServer_BotIsLand(x + fSin * 420.0, z + fCos * 420.0, 20.0)) { return false; }
	return true;
}

// the nearest free course to the wanted one (steps of 20 degrees to both sides)
float NetServer_BotAvoidLand(float x, float z, float fDesired)
{
	// the shore points closer than 650 m (the probes look 420 m ahead)
	DeleteAttribute(&NSBotShore, "Near");
	int iNear = 0;
	int iNum = sti(NSBotShore.Num);
	for (int p=0; p<iNum; p++)
	{
		string sP = "p" + p;
		float fPX = stf(NSBotShore.(sP).x) - x;
		float fPZ = stf(NSBotShore.(sP).z) - z;
		if (fPX * fPX + fPZ * fPZ > 422500.0) { continue; }
		string sN = "n" + iNear;
		NSBotShore.Near.(sN) = sP;
		iNear++;
	}
	NSBotShore.NearNum = iNear;

	bool bCircleNear = false;
	if (CheckAttribute(&NSBotShore, "HasCircle") && sti(NSBotShore.HasCircle))
	{
		if (GetDistance2D(x, z, stf(NSBotShore.cx), stf(NSBotShore.cz)) < stf(NSBotShore.cr) + 650.0) { bCircleNear = true; }
	}
	if (iNear == 0 && !bCircleNear) { return fDesired; }

	if (NetServer_BotCourseFree(x, z, fDesired)) { return fDesired; }
	for (int k=1; k<=9; k++)
	{
		float fStep = 0.35 * makefloat(k);
		if (NetServer_BotCourseFree(x, z, fDesired + fStep)) { return fDesired + fStep; }
		if (NetServer_BotCourseFree(x, z, fDesired - fStep)) { return fDesired - fStep; }
	}
	return fDesired + PI;
}

// ------------------------------------------------------------------------------------------------
// the brain
// ------------------------------------------------------------------------------------------------
void NetServer_BotThink(int i)
{
	ref rB = NetServer_GetClient(i);
	if (!NetServer_HasShip(i)) { return; }

	if (sti(rB.Dead) || NetServer_BotRemoved(i))
	{
		if (sti(rB.BotDeadAt) <= 0) { rB.BotDeadAt = iServerTime; }
		rB.Ship.TurnLeft = 0.0;
		rB.Ship.TurnRight = 0.0;
		rB.Ship.TurnState = 0.0;
		return;
	}

	// COOP s41: the captain fights on the boarding deck / the ship is a prize waiting for the choice of its captor: it stands
	if (NetServer_ShipHeld(i))
	{
		rB.Ship.TurnLeft = 0.0;
		rB.Ship.TurnRight = 0.0;
		rB.Ship.TurnState = 0.0;
		rB.Ship.SailState = 0.0;
		return;
	}

	int iLevel = NetServer_BotSkillLevel(rB);
	if (iLevel <= 0)
	{
		// harmless: stands and does nothing
		rB.Ship.TurnLeft = 0.0;
		rB.Ship.TurnRight = 0.0;
		rB.Ship.TurnState = 0.0;
		rB.Ship.SailState = 0.0;
		return;
	}

	NetServer_BotShoreInit();

	float fX = stf(rB.Ship.Pos.x);
	float fZ = stf(rB.Ship.Pos.z);
	float fAy = NetServer_BotNormAng(stf(rB.Ship.Ang.y));
	float fSpeed = stf(rB.Ship.Speed.z);
	if (fSpeed > 1000.0 || fSpeed < -1000.0) { fSpeed = 0.0; }
	float fDX = 0.0;
	float fDZ = 0.0;
	float fD = 0.0;

	NetServer_BotRefill(rB, false);

	int iRel = NetServer_BotRel(i);
	float fDesired = fAy;
	float fSail = 0.5;
	bool bFire = false;
	bool bBoarding = false;

	int wT = DST_INVALID;
	if (iRel != NET_BOTREL_NEUTRAL) { wT = NetServer_BotFindTarget(i); }

	if (wT != DST_INVALID)
	{
		ref rT = NetServer_GetClient(wT);
		fD = stf(rB.BotTargetDist);
		fDX = stf(rT.Ship.Pos.x) - fX;
		fDZ = stf(rT.Ship.Pos.z) - fZ;
		float fBear = atan2(fDX, fDZ);
		float fRange = NetServer_BotRange(rB);
		float fKeep = fRange * NetServer_BotKeepPart(iLevel);

		// the place where the target will be when the bot gets there: it sails to it, not behind the stern
		float fTSpd = stf(rT.Ship.Speed.z);
		if (fTSpd > 500.0 || fTSpd < -500.0) { fTSpd = 0.0; }
		float fTAy = stf(rT.Ship.Ang.y);
		float fVBot = fSpeed;
		if (fVBot < 20.0) { fVBot = 20.0; }
		float fTLead = fD / fVBot;
		if (fTLead > 6.0) { fTLead = 6.0; }
		float fIX = stf(rT.Ship.Pos.x) + sin(fTAy) * fTSpd * fTLead;
		float fIZ = stf(rT.Ship.Pos.z) + cos(fTAy) * fTSpd * fTLead;
		float fIntercept = atan2(fIX - fX, fIZ - fZ);

		// boarding: it attacks the ship (a player or an ally of a player) when it sees that it wins the fight
		bBoarding = NetServer_BotWinsBoarding(rB, rT, iLevel);

		if (bBoarding)
		{
			// to the boarding distance (hull to hull, 40 m is within the reach of a bot) at full sails; it slows down
			// by the closing speed (not by its own: a target that sails away must be caught up), and only close to the target
			float fBGap = fD - NetServer_GetShipHalfWidth(rB) - NetServer_GetShipHalfWidth(rT);
			float fBDist = fD;
			if (fBDist < 1.0) { fBDist = 1.0; }
			float fBClose = ((sin(fAy) * fSpeed - sin(fTAy) * fTSpd) * fDX + (cos(fAy) * fSpeed - cos(fTAy) * fTSpd) * fDZ) / fBDist;
			fDesired = fIntercept;
			if (fBGap < 130.0)
			{
				// alongside: the course of the target, a little towards it
				fDesired = fTAy + 0.6 * NetServer_BotAngDiff(fBear, fTAy);
			}
			fSail = 1.0;
			if (fBGap < 400.0 && fBClose > 15.0)
			{
				float fBSec = (fBGap - 40.0) / fBClose;
				if (fBSec < 2.0) { fSail = 0.0; }
				else
				{
					if (fBSec < 5.0) { fSail = 0.5; }
				}
			}
			if (fBGap < 70.0 && fSpeed > fTSpd + 5.0) { fSail = 0.5; }
			if (iServerTime - sti(rB.BotBoardLog) > 4000)
			{
				rB.BotBoardLog = iServerTime;
				Trace("COOP: bot " + i + " boards " + wT + " gap=" + makeint(fBGap) + " closing=" + makeint(fBClose) + " sail=" + fSail + " crew=" + makeint(stf(rB.Ship.Crew.Quantity)) + "/" + makeint(stf(rT.Ship.Crew.Quantity)));
			}
			if (NetServer_IsBoardable(i, wT))
			{
				if (NetServer_CheckBoardPair(rB, rT, false)) { NetServer_TryBoardPlayer(i, wT); }
			}
			if (sti(rB.Dead)) { return; }
			if (fD < fRange * 1.05) { bFire = true; }
			NetServer_BotCharge(i, fD, fRange);		// grapes on the crew when close
		}
		else
		{
			if (fD > fKeep * 1.4)
			{
				// closer, as fast as it can stop in time
				fDesired = fIntercept;
				fSail = NetServer_BotSailByTime(fD - fKeep, fSpeed, 6.0, 2.5);
			}
			else
			{
				// circle the target broadside on, at the distance of its level: closer than it -> turn out, farther -> in
				float fCorr = (fD - fKeep) / fKeep;
				if (fCorr > 0.7) { fCorr = 0.7; }
				if (fCorr < -0.7) { fCorr = -0.7; }
				float fOff = PId2 - fCorr;
				float fA1 = fBear + fOff;
				float fA2 = fBear - fOff;
				fDesired = fA1;
				if (NetServer_BotAbs(NetServer_BotAngDiff(fA2, fAy)) < NetServer_BotAbs(NetServer_BotAngDiff(fA1, fAy))) { fDesired = fA2; }
				// full sails (medium and hard) unless it is too fast for the circle: then half (a fast ship sails out of the circle)
				fSail = 1.0;
				if (iLevel <= 1) { fSail = 0.5; }
				if (fSpeed > fKeep * 0.35) { fSail = 0.5; }
			}
			if (iServerTime - sti(rB.BotBoardLog) > 6000)
			{
				rB.BotBoardLog = iServerTime;
				Trace("COOP: bot " + i + " no boarding of " + wT + " crew=" + makeint(stf(rB.Ship.Crew.Quantity)) + "/" + makeint(stf(rT.Ship.Crew.Quantity)) + " skill=" + iLevel + " grappling=" + rB.Skills.Grappling + "/" + rT.Skills.Grappling);
			}
			float fFirePart = 1.05;
			if (iLevel <= 1) { fFirePart = 0.8; }
			if (iLevel >= 3) { fFirePart = 0.75; }		// hard: no shots into the distance where even a perfect lead is a lottery
			if (fD < fRange * fFirePart) { bFire = true; }
			NetServer_BotCharge(i, fD, fRange);
		}
	}
	else
	{
		bool bFollow = false;
		int wO = DST_INVALID;
		if (iRel == NET_BOTREL_ALLY)
		{
			// an ally follows the NEAREST player (not only the admin who called it); it keeps the one it follows
			// unless another one is much nearer, so it does not jump between two players
			float fFNear = 1000000.0;
			float fFD = 0.0;
			int wFPrev = DST_INVALID;
			if (CheckAttribute(rB, "BotFollow")) { wFPrev = sti(rB.BotFollow); }
			for (int jf=0; jf<NET_MAXPLAYERS; jf++)
			{
				if (!sti(NSClients[jf].Use)) { continue; }
				if (!NetServer_ShipIsAlive(jf)) { continue; }
				fFD = GetDistance2D(fX, fZ, stf(NSClients[jf].Ship.Pos.x), stf(NSClients[jf].Ship.Pos.z));
				if (fFD < fFNear) { fFNear = fFD; wO = jf; }
			}
			if (wO != DST_INVALID && wFPrev >= 0 && wFPrev < NET_MAXPLAYERS && wFPrev != wO)
			{
				if (sti(NSClients[wFPrev].Use) && NetServer_ShipIsAlive(wFPrev))
				{
					fFD = GetDistance2D(fX, fZ, stf(NSClients[wFPrev].Ship.Pos.x), stf(NSClients[wFPrev].Ship.Pos.z));
					if (fFD < fFNear * 1.25) { wO = wFPrev; }
				}
			}
			rB.BotFollow = wO;
			if (wO != DST_INVALID) { bFollow = true; }
		}

		if (bFollow)
		{
			// far: catch up with the player (to the place where he will be, the sails go down when the bot closes in fast);
			// near: escort him: his course and his sails, so a player who stands is not left by a bot that sails away
			ref rO = NetServer_GetClient(wO);
			fDX = stf(rO.Ship.Pos.x) - fX;
			fDZ = stf(rO.Ship.Pos.z) - fZ;
			fD = sqrt(fDX * fDX + fDZ * fDZ);
			float fOAy = stf(rO.Ship.Ang.y);
			float fOSpd = stf(rO.Ship.Speed.z);
			if (fOSpd > 500.0 || fOSpd < -500.0) { fOSpd = 0.0; }
			if (fD > NET_BOT_FOLLOW_DIST)
			{
				float fFSpeed = fSpeed;
				if (fFSpeed < 20.0) { fFSpeed = 20.0; }
				float fFT = fD / fFSpeed;
				if (fFT > 6.0) { fFT = 6.0; }
				fDesired = atan2(stf(rO.Ship.Pos.x) + sin(fOAy) * fOSpd * fFT - fX, stf(rO.Ship.Pos.z) + cos(fOAy) * fOSpd * fFT - fZ);
				fSail = 1.0;
				float fFDist = fD;
				if (fFDist < 1.0) { fFDist = 1.0; }
				float fFClose = ((sin(fAy) * fSpeed - sin(fOAy) * fOSpd) * fDX + (cos(fAy) * fSpeed - cos(fOAy) * fOSpd) * fDZ) / fFDist;
				if (fFClose > 15.0 && fD < 800.0)
				{
					float fFSec = (fD - NET_BOT_FOLLOW_DIST * 0.6) / fFClose;
					if (fFSec < 2.0) { fSail = 0.0; }
					else
					{
						if (fFSec < 5.0) { fSail = 0.5; }
					}
				}
			}
			else
			{
				fDesired = fOAy;
				fSail = stf(rO.Ship.SailState);
				// not ahead of the player, not far behind him
				float fAhead = (0.0 - fDX) * sin(fOAy) + (0.0 - fDZ) * cos(fOAy);
				if (fAhead > 60.0 && fSail > 0.0) { fSail = fSail - 0.5; }
				if (fAhead < -120.0 && fSail < 1.0) { fSail = fSail + 0.5; }
			}
		}
		else
		{
			// an enemy without a target goes to the nearest player it may fight; otherwise sails around
			int wNear = DST_INVALID;
			float fNear = 100000.0;
			int iNearMax = NET_MAXPLAYERS;
			if (iRel >= NET_BOTREL_TEAM0) { iNearMax = NET_MAXCLIENTS; }		// COOP s28: a team bot looks for the team bots too
			for (int j=0; j<iNearMax; j++)
			{
				if (j == i) { continue; }
				if (!NetServer_ShipIsAlive(j)) { continue; }
				if (!NetServer_ShouldTarget(i, j)) { continue; }
				float fDj = GetDistance2D(fX, fZ, stf(NSClients[j].Ship.Pos.x), stf(NSClients[j].Ship.Pos.z));
				if (fDj < fNear) { fNear = fDj; wNear = j; }
			}
			// COOP s28: the attackers of the fort go for a ship only when it is nearer than the fort
			if (wNear != DST_INVALID && NetServer_BotFortGoal(i))
			{
				if (GetDistance2D(fX, fZ, stf(NetServer.FortX), stf(NetServer.FortZ)) < fNear) { wNear = DST_INVALID; }
			}
			if (wNear != DST_INVALID && iLevel >= 1)
			{
				fDesired = atan2(stf(NSClients[wNear].Ship.Pos.x) - fX, stf(NSClients[wNear].Ship.Pos.z) - fZ);
				fSail = 1.0;
			}
			else
			{
				if (NetServer_BotFortGoal(i) && iLevel >= 1)
				{
					// COOP s28: to the fort, broadside on at 60% of the range, fire in the range
					fDX = stf(NetServer.FortX) - fX;
					fDZ = stf(NetServer.FortZ) - fZ;
					fD = sqrt(fDX * fDX + fDZ * fDZ);
					float fGoRange = NetServer_BotRange(rB);
					float fGoKeep = fGoRange * 0.6;
					float fGoBear = atan2(fDX, fDZ);
					fDesired = fGoBear;
					fSail = 1.0;
					if (fD < fGoKeep)
					{
						float fGoA1 = fGoBear + PId2;
						float fGoA2 = fGoBear - PId2;
						fDesired = fGoA1;
						if (NetServer_BotAbs(NetServer_BotAngDiff(fGoA2, fAy)) < NetServer_BotAbs(NetServer_BotAngDiff(fGoA1, fAy))) { fDesired = fGoA2; }
						fSail = 0.5;
					}
					bool bGoFired = false;
					if (fD < fGoRange) { bGoFired = NetServer_BotFireAtFort(i); }
					if (iServerTime - sti(rB.BotBoardLog) > 5000)
					{
						rB.BotBoardLog = iServerTime;
						Trace("COOP: bot " + i + " goes for the fort: dist=" + makeint(fD) + " range=" + makeint(fGoRange) + " fired=" + bGoFired);
					}
				}
				else
				{
				fDX = stf(rB.SpawnPos.x) - fX;
				fDZ = stf(rB.SpawnPos.z) - fZ;
				fD = sqrt(fDX * fDX + fDZ * fDZ);
				if (fD > NET_BOT_HOME_DIST)
				{
					fDesired = atan2(fDX, fDZ);
					rB.BotWanderAy = fDesired;
					rB.BotWanderUntil = iServerTime + 8000;
				}
				else
				{
					if (iServerTime > sti(rB.BotWanderUntil))
					{
						rB.BotWanderAy = fAy + (frnd() - 0.5) * 1.6;
						rB.BotWanderUntil = iServerTime + 15000 + rand(15000);
					}
					fDesired = stf(rB.BotWanderAy);
				}
				fSail = 0.5;
				}
			}
		}
	}

	// stuck (at the shore, against a ship): a new heading
	if (fSail > 0.0 && stf(rB.Ship.Speed.z) < 0.3)
	{
		if (iServerTime - sti(rB.BotSlowSince) > 5000)
		{
			rB.BotIslandHit = iServerTime;
			rB.BotEvadeAy = fAy + PI * (0.6 + frnd() * 0.8);
			rB.BotSlowSince = iServerTime;
		}
	}
	else
	{
		rB.BotSlowSince = iServerTime;
	}

	// away from the shore for a short while after a touch (a bot takes no damage from islands, so it only has to get free),
	// otherwise a course that does not run into the known shore
	if (iServerTime - sti(rB.BotIslandHit) < 2500)
	{
		fDesired = stf(rB.BotEvadeAy);
		fSail = 1.0;
	}
	else
	{
		if (!bBoarding || fD > 200.0) { fDesired = NetServer_BotAvoidLand(fX, fZ, fDesired); }
	}

	// the rudder sign is known (1, checked in the game); NetServer_BotLearnTurn flipped it by mistake after collisions
	NetServer_BotSteer(rB, fDesired, fAy);
	rB.Ship.SailState = Clampf(fSail);

	if (bFire) { NetServer_FireOutside(i); }
}
