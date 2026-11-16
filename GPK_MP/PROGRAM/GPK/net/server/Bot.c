// ================================================================================================
// COOP: computer controlled ships (bots). Admin menu button "Bot+" -> NET_ADMIN_BOT_ADD -> NetServer_BotSpawn.
//
// A bot is a server slot WITHOUT a network connection (Use = false, Bot = true), exactly like the parked ship of a player
// who left the game: the ship entity, the damage / sinking / respawn code, the ship info packets (NetServer_ShipUpdateParameters)
// and the clients' copies of the ship work as for any other ship. The only new part is the "captain": NetServer_BotTick
// writes the same attributes a real captain writes in NetServer_OnShip (TurnLeft / TurnRight / SailState / DoCannonFire).
//
// Behaviour (like the single player AI of a lone Lugger): finds the nearest enemy player ship, comes close, turns the broadside
// to it and fires when the target is in the arc of a bort. Without enemies it sails around. If it stops (island / ship) it
// turns away for a few seconds.
//
// The slot is "burned" for the whole game like every ship slot (client ships are never deleted), so a removed bot only sinks.
// ================================================================================================

#define NET_BOT_MAX				8		// bots at the same time
#define NET_BOT_TICK				200		// ms between two thinks of the bots
#define NET_BOT_SHIP				"Lugger"

// ---- tunables (edit and restart the server) ----
float fNetBotFireDist		= 380.0;	// m: the bot fires when the target is closer than this
float fNetBotCloseDist		= 450.0;	// m: closer than this the bot goes over to a course parallel to the target
float fNetBotKeepDist		= 220.0;	// m: the distance the bot tries to keep while it circles the target
float fNetBotTurnDead		= 0.10;	// rad: heading error that is ignored (no rudder)
float fNetBotSideTol		= 0.45;	// rad: how exactly the target must be abeam to fire the side cannons
int iNetBotFireGap			= 1800;	// ms between two volleys of a bot
int iNetBotRudderSign		= 1;		// 1: Ang.y grows when the ship turns right (checked on this build). Put -1 if bots turn the wrong way.

// ---- switches (each can be turned off here) ----
bool bNetBotAimBallistic	= true;	// the bot aims every cannon by ballistics (distance to the target), not by the deck camera
float fNetBotLead			= 1.0;		// 0 = aim at the place where the target is now, 1 = aim at where it will be when the ball arrives
bool bNetBotUsePerks		= true;	// the bot uses the active perks (only when the server has active perks on)
bool bNetBotAvoidIsland		= true;	// the bot steers away from the places where a ship has hit the island in this game
float fNetBotHazardRadius	= 300.0;	// m: the bot starts to turn away from such a place at this distance

// island contact points of this game (Ship2IslandCollision): oNetBotHaz.h<n>.x / .z
object oNetBotHaz;
int iNetBotHazN = 0;
int iNetBotHazNext = 0;

bool bNetBotTickRunning = false;
int iNetBotSerial = 0;

bool NetServer_IsBot(int i)
{
	if (i < 0 || i >= NET_MAXCLIENTS) { return false; }
	if (!CheckAttribute(&NSClients[i], "Bot")) { return false; }
	return sti(NSClients[i].Bot) != 0;
}

// bots whose ship exists in this game
int NetServer_BotCount()
{
	int n = 0;
	for (int i=0; i<NET_MAXCLIENTS; i++)
	{
		if (NetServer_IsBot(i) && NetServer_HasShip(i)) { n++; }
	}
	return n;
}

// the angle a - b in (-PI, PI]
float NetServer_BotAngDiff(float fA, float fB)
{
	float f = fA - fB;
	while (f > PI) { f = f - PIm2; }
	while (f <= -PI) { f = f + PIm2; }
	return f;
}

// the cannons for the ship: the biggest ordinary cannon that the ship can carry
int NetServer_BotCannonType(ref rShip)
{
	int iBest = 0;
	int iBestCaliber = -1;
	for (int i=0; i<Net_GetCannonsNum(); i++)
	{
		ref rCannon = Net_GetCannonByIndex(i);
		if (rCannon.name == "special") { continue; }
		int iCaliber = sti(rCannon.caliber);
		if (iCaliber > sti(rShip.MaxCaliber)) { continue; }
		if (iCaliber > iBestCaliber) { iBestCaliber = iCaliber; iBest = i; }
	}
	return iBest;
}

// ------------------------------------------------------------------------------------------------
// messages to the clients (the bot is a normal "player" for them)
// ------------------------------------------------------------------------------------------------
// NSC_CLIENT_CONNECTED of a bot, the same layout as in NetServer_SendConnectAccepted
int NetServer_BotCreateConnectedMessage(int i, int iAnnounce)
{
	ref rB = NetServer_GetClient(i);

	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_CLIENT);
	NMAddByte(iSMsg, NSC_CLIENT_CONNECTED);
	NMAddClientID(iSMsg, i);
	NMAddString(iSMsg, rB.NickName, 24);
	NMAddString(iSMsg, rB.Ship.Name, 24);
	NMAddString(iSMsg, rB.UserData.Face, 12);
	NMAddString(iSMsg, rB.UserData.Flag, 12);
	NMAddString(iSMsg, rB.UserData.Sail, 12);
	int iBotSailColor = argb(255, 255, 255, 255);
	if (CheckAttribute(rB, "Ship.Sails.Color")) { iBotSailColor = sti(rB.Ship.Sails.Color); }
	NMAddDword(iSMsg, iBotSailColor);
	NMAddByte(iSMsg, 1);			// buy ready
	NMAddByte(iSMsg, 0);			// no ship yet on the client
	NMAddByte(iSMsg, iAnnounce);	// chat line "Bot 1 connected"
	return iSMsg;
}

// NSC_PLAYERINFO_TEAMCHANGE: the clients use the team for the colour of the ship and friend / enemy
int NetServer_BotCreateTeamMessage(int i)
{
	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_PLAYERINFO);
	NMAddByte(iSMsg, NSC_PLAYERINFO_TEAMCHANGE);
	NMAddClientID(iSMsg, i);
	NMAddByte(iSMsg, sti(NSClients[i].Team));
	return iSMsg;
}

// a player comes into the running game (NetServer_LateJoin): he gets the bots like the other players
void NetServer_BotSendIntro(int wTo, int i)
{
	int iSMsg = NetServer_BotCreateConnectedMessage(i, 0);
	NetServer_SendMessage(wTo, iSMsg, true);
	NMDelete(iSMsg);

	iSMsg = NetServer_BotCreateTeamMessage(i);
	NetServer_SendMessage(wTo, iSMsg, true);
	NMDelete(iSMsg);
}

// ------------------------------------------------------------------------------------------------
// spawn
// ------------------------------------------------------------------------------------------------
// the team of a new bot: another team than the admin's (team games), in deathmatch it does not matter
int NetServer_BotPickTeam(int wAdmin)
{
	if (sti(NetServer.GameType) == NETGAME_DEATHMATCH) { return 0; }

	int iTeams = sti(NetServer.NumTeams);
	if (iTeams < 2) { iTeams = 2; }
	if (iTeams > 4) { iTeams = 4; }

	int iAdminTeam = 0;
	if (wAdmin >= 0 && wAdmin < NET_MAXCLIENTS) { iAdminTeam = sti(NSClients[wAdmin].Team); }

	int iTeam = iAdminTeam + 1;
	if (iTeam >= iTeams) { iTeam = 0; }
	return iTeam;
}

int NetServer_BotFindFreeSlot()
{
	int iFree = 0;
	int iSlot = DST_INVALID;
	for (int i=0; i<NET_MAXCLIENTS; i++)
	{
		if (sti(NSClients[i].Use)) { continue; }
		if (NetServer_SlotBurned(i)) { continue; }
		if (iSlot == DST_INVALID) { iSlot = i; }
		iFree++;
	}
	// one slot always stays free for a player
	if (iFree < 2) { return DST_INVALID; }
	return iSlot;
}

bool NetServer_BotSpawn(int wAdmin)
{
	int i;

	if (!bServerGameStarted) { return false; }
	if (NetServer_BotCount() >= NET_BOT_MAX) { return false; }

	int iSlot = NetServer_BotFindFreeSlot();
	if (iSlot == DST_INVALID) { Trace("COOP: bot: no free slot"); return false; }

	int iType = Net_GetShipIndex(NET_BOT_SHIP);
	if (iType < 0) { return false; }
	ref rShip = Net_GetShipByIndex(iType);

	// clean slot (data of an older game). Index / id / Server of the slot stay.
	DeleteAttribute(&NSClients[iSlot], "Ship");
	DeleteAttribute(&NSClients[iSlot], "Skills");
	DeleteAttribute(&NSClients[iSlot], "APerk");
	DeleteAttribute(&NSClients[iSlot], "Killer");
	DeleteAttribute(&NSClients[iSlot], "SpawnProtect");

	ref rBot = NetServer_GetClient(iSlot);

	iNetBotSerial++;
	string sName = "Bot " + iNetBotSerial;

	rBot.Use = false;				// no network connection: nothing is ever sent to this slot
	rBot.Bot = true;
	rBot.NickName = sName;
	rBot.NickPassword = "";
	rBot.BotWanderUntil = 0;
	rBot.BotStuck = 0;
	rBot.BotEscapeUntil = 0;
	rBot.BotLastFire = 0;

	// faces / flag / sails: the "empty" pictures every client has (client\OnClient.c uses them when a texture is missing)
	NetServer_InitNewClientSlot(rBot, sName, "_emptyface_", "_emptyflag_", "_emptysail_", argb(255, 255, 255, 255));
	rBot.Team = NetServer_BotPickTeam(wAdmin);
	rBot.BuyReady = true;
	rBot.StartReady = true;
	rBot.Server = true;

	// the loadout: the same fields NetServer_OnBuy_FullInfo fills for a player
	float fMaxHP = stf(rShip.HP) * (1.0 + stf(NetShipHullUpgrades[0].hp) / 100.0);
	rBot.Ship.Type = iType;
	rBot.Ship.HP = fMaxHP;
	rBot.Ship.MaxHP = fMaxHP;
	rBot.Ship.SP = 100.0;
	rBot.Ship.Upgrades.Hull = 1;
	rBot.Ship.Upgrades.Sails = 1;
	rBot.Ship.Upgrades.Cannons = 1;
	rBot.Ship.Cannons.Borts = "";
	rBot.Ship.Cannons.Type = NetServer_BotCannonType(rShip);
	rBot.Ship.Cannons.Charge.Type = 0;			// balls
	rBot.Ship.Crew.Quantity = Net_GetCrewCount(iType, 100);

	for (i=0; i<Net_GetGoodsNum(); i++)
	{
		ref rGood = Net_GetGoodByIndex(i);
		string sGood = rGood.Name;
		rBot.Ship.Goods.(sGood) = 0;
	}
	rBot.Ship.Goods.Balls = 300;
	rBot.Ship.Goods.Grapes = 50;
	rBot.Ship.Goods.Powder = 400;
	rBot.Ship.Goods.Planks = 30;
	rBot.Ship.Goods.Sailcloth = 30;

	DeleteAttribute(rBot, "Ship.ShotAcc");
	NetServer_SetBortsByCount(rBot, Net_GetCannonsCountByPercent(iType, 100));
	rBot.Ship.MinesDropped = 0;
	rBot.Ship.MineCooldown = 0;
	rBot.Ship.BranderActive = 0;
	rBot.Ship.BranderArmUntil = 0;

	// a bot captain: all skills on maximum (the passive perks of the server apply to the bot like to any other ship,
	// the active ones the bot uses by itself: NetServer_BotUsePerks)
	rBot.Skills.Accuracy = makeint(NETSKILL_MAX);
	rBot.Skills.Cannons = makeint(NETSKILL_MAX);
	rBot.Skills.Sailing = makeint(NETSKILL_MAX);
	rBot.Skills.Defence = makeint(NETSKILL_MAX);
	rBot.Skills.Repair = makeint(NETSKILL_MAX);
	rBot.Skills.Grappling = makeint(NETSKILL_MAX);
	rBot.Skills.Repair.Active = 0;
	rBot.Skills.Repair.Cooldown = iServerTime - 60 * 1000;

	rBot.Ship.SpeedRateModifier = 1.0 + (stf(NetShipHullUpgrades[0].speedrate) + stf(NetShipSailUpgrade[0].speedrate)) / 100.0;
	rBot.Ship.TurnRateModifier = 1.0 + (stf(NetShipHullUpgrades[0].turnrate) + stf(NetShipSailUpgrade[0].turnrate)) / 100.0;
	rBot.Ship.SailsDamageModifier = 1.0 + stf(NetShipSailUpgrade[0].dmgtosails) / 100.0;

	NetServer_CannonRecalculateParameters(rBot);
	NetServer_CalculateFireTime(rBot);
	NetServer_CalculateReloadTime(rBot);

	// the ship entity of the server at a free spawn point (the same code as for a player who joins the running game);
	// it also sets Spawned and takes the snapshot of the loadout for the respawn
	NetServer_SpawnLateShip(iSlot);
	rBot.InGame = false;		// never "in game" as a client: no ship loads / perk states are sent to it

	// the clients: a new player and his ship
	int iSMsg = NetServer_BotCreateConnectedMessage(iSlot, 1);
	NetServer_SendToInGame(iSMsg);
	NMDelete(iSMsg);

	iSMsg = NetServer_BotCreateTeamMessage(iSlot);
	NetServer_SendToInGame(iSMsg);
	NMDelete(iSMsg);

	iSMsg = NetServer_CreateClientLoadMessage(iSlot);
	NetServer_SendToInGame(iSMsg);
	NMDelete(iSMsg);

	NetServer_BotStartTick();
	Trace("COOP: bot " + sName + " spawned, slot " + iSlot + " team " + rBot.Team + " by admin " + wAdmin);
	return true;
}

// ------------------------------------------------------------------------------------------------
// remove (admin: select the bot in the table and press "Kick"): the ship sinks, the player entry disappears.
// ------------------------------------------------------------------------------------------------
void NetServer_BotRemove(int i)
{
	if (!NetServer_IsBot(i)) { return; }

	ref rBot = NetServer_GetClient(i);
	if (NetServer_HasShip(i) && !sti(rBot.Dead))
	{
		// no killer, no score: the same as a lightning strike
		if (sti(NetServer.Respawn)) { NetServer.Respawn = 0; NetServer_SetClientDead(rBot, NETKILL_BY_LIGHTNING, DST_INVALID); NetServer.Respawn = 1; }
		else { NetServer_SetClientDead(rBot, NETKILL_BY_LIGHTNING, DST_INVALID); }
	}

	// out of the lists of the clients (the ship stays and sinks, like the ship of a player who left)
	rBot.Bot = false;				// the tick does not control it any more, a respawn event does not bring it back (Use = false)
	rBot.Parked = false;
	NetServer_SendDropTo(DST_ALL, i);
}

// ------------------------------------------------------------------------------------------------
// the captain
// ------------------------------------------------------------------------------------------------
void NetServer_BotStartTick()
{
	if (bNetBotTickRunning) { return; }
	bNetBotTickRunning = true;
	PostEvent("NetServer_BotTick", NET_BOT_TICK, "l", sti(NetServer.Round));
}

// the nearest enemy player ship (bots do not fight each other), DST_INVALID if there is none
int NetServer_BotFindTarget(int i)
{
	ref rB = NetServer_GetClient(i);
	int iBest = DST_INVALID;
	float fBest = 1000000000.0;

	for (int j=0; j<NET_MAXCLIENTS; j++)
	{
		if (j == i) { continue; }
		if (NetServer_IsBot(j)) { continue; }
		if (!sti(NSClients[j].Use)) { continue; }
		if (!NetServer_ShipIsAlive(j)) { continue; }
		if (sti(NetServer.GameType) != NETGAME_DEATHMATCH && sti(NSClients[j].Team) == sti(rB.Team)) { continue; }

		float fDX = stf(NSClients[j].Ship.Pos.x) - stf(rB.Ship.Pos.x);
		float fDZ = stf(NSClients[j].Ship.Pos.z) - stf(rB.Ship.Pos.z);
		float fD = fDX * fDX + fDZ * fDZ;
		if (fD < fBest) { fBest = fD; iBest = j; }
	}
	return iBest;
}

// the bot has no ammo / powder: it is refilled (the bot never runs dry)
void NetServer_BotRestock(ref rB)
{
	if (!CheckAttribute(rB, "Ship.Start.Goods.Powder")) { return; }

	int iAmmo = NetServer_GoodTotal(rB, "Balls");
	if (sti(rB.Ship.Goods.Powder) > 60 && iAmmo > 20) { return; }

	for (int k=0; k<Net_GetGoodsNum(); k++)
	{
		ref rGood = Net_GetGoodByIndex(k);
		string sGood = rGood.Name;
		rB.Ship.Goods.(sGood) = sti(rB.Ship.Start.Goods.(sGood));
	}
	DeleteAttribute(rB, "Ship.GoodsHeld");
	DeleteAttribute(rB, "Ship.AmmoLast");
}

// ------------------------------------------------------------------------------------------------
// island hazards: the places where a ship hit the island in this game (NetServer_Ship2IslandCollision calls this).
// The scripts have no ray test against the island, so the bots learn: after the first hit nobody sails there again.
// ------------------------------------------------------------------------------------------------
void NetServer_BotHazardAdd(float fX, float fZ)
{
	if (fX < -100000.0 || fX > 100000.0 || fZ < -100000.0 || fZ > 100000.0) { return; }
	int k;
	for (k=0; k<iNetBotHazN; k++)
	{
		string sK = "h" + k;
		float fDX = fX - stf(oNetBotHaz.(sK).x);
		float fDZ = fZ - stf(oNetBotHaz.(sK).z);
		if (fDX * fDX + fDZ * fDZ < 60.0 * 60.0) { return; }		// the same place
	}
	int iSlot = iNetBotHazN;
	if (iSlot >= 40) { iSlot = iNetBotHazNext; iNetBotHazNext = iNetBotHazNext + 1; if (iNetBotHazNext >= 40) { iNetBotHazNext = 0; } }
	else { iNetBotHazN = iNetBotHazN + 1; }
	string sS = "h" + iSlot;
	oNetBotHaz.(sS).x = fX;
	oNetBotHaz.(sS).z = fZ;
	Trace("COOP: bot hazard " + iSlot + " x=" + fX + " z=" + fZ);
}

// the nearest hazard in front of the ship: BotHazW 0..1 (0 = none, 1 = right in front), BotHazRel = side (> 0: on the right)
void NetServer_BotHazardCheck(ref rB, float fX, float fZ, float fAy)
{
	rB.BotHazW = 0.0;
	rB.BotHazRel = 0.0;
	if (!bNetBotAvoidIsland) { return; }

	float fBestW = 0.0;
	for (int k=0; k<iNetBotHazN; k++)
	{
		string sK = "h" + k;
		float fDX = stf(oNetBotHaz.(sK).x) - fX;
		float fDZ = stf(oNetBotHaz.(sK).z) - fZ;
		float fD = sqrt(fDX * fDX + fDZ * fDZ);
		if (fD > fNetBotHazardRadius) { continue; }
		float fRel = NetServer_BotAngDiff(GetAngleY(fDX, fDZ), fAy);
		float fAbs = fRel;
		if (fAbs < 0.0) { fAbs = 0.0 - fAbs; }
		if (fAbs > 1.3) { continue; }		// not ahead
		float fW = (1.0 - fD / fNetBotHazardRadius) * (1.0 - fAbs / 1.5);
		if (fD < 90.0) { fW = 1.0; }
		if (fW > fBestW) { fBestW = fW; rB.BotHazW = fW; rB.BotHazRel = fRel; }
	}
}

// ------------------------------------------------------------------------------------------------
// ballistics: called from NetServer_OnCannonFire for every cannon of a bot (OnBall.c). The engine aims the cannons
// by the camera ray; a bot has no camera, so the elevation and the direction are calculated here.
// Returns true and sets arClient.BotShotDir / BotShotAng when the shot is aimed.
// ------------------------------------------------------------------------------------------------
bool NetServer_BotAimShot(aref arClient, float x, float z, float fV0)
{
	if (!bNetBotAimBallistic) { return false; }
	if (!CheckAttribute(arClient, "BotAim.x")) { return false; }
	if (iServerTime > sti(arClient.BotAimUntil)) { return false; }
	if (fV0 < 1.0) { return false; }

	ref rCannon = Net_GetCannonByIndex(sti(arClient.Ship.Cannons.Type));
	float fAngMax = stf(rCannon.FireAngMax);
	float fAngMin = stf(rCannon.FireAngMin);

	float fTX = stf(arClient.BotAim.x);
	float fTZ = stf(arClient.BotAim.z);
	float fVX = stf(arClient.BotAim.vx);
	float fVZ = stf(arClient.BotAim.vz);

	float fAng = 0.0;
	float fDX = fTX - x;
	float fDZ = fTZ - z;
	float fD = sqrt(fDX * fDX + fDZ * fDZ);

	// the target moves while the ball flies: two passes (distance -> elevation -> flight time -> new target point)
	float fMult = stf(NSBalls.SpeedMultiply);
	if (fMult < 0.1) { fMult = 1.0; }
	for (int k=0; k<2; k++)
	{
		float fS = 9.81 * fD / (fV0 * fV0);
		if (fS > 1.0) { fS = 1.0; }
		fAng = 0.5 * atan2(fS, sqrt(1.0 - fS * fS));
		if (fAng > fAngMax) { fAng = fAngMax; }
		if (fAng < fAngMin) { fAng = fAngMin; }

		float fFly = fD / (fV0 * cos(fAng) * fMult);
		float fLX = fTX + fVX * fFly * fNetBotLead;
		float fLZ = fTZ + fVZ * fFly * fNetBotLead;
		fDX = fLX - x;
		fDZ = fLZ - z;
		fD = sqrt(fDX * fDX + fDZ * fDZ);
	}

	arClient.BotShotDir = GetAngleY(fDX, fDZ);
	arClient.BotShotAng = fAng;
	return true;
}

// ------------------------------------------------------------------------------------------------
// active perks (only when the server has them on): the bot presses them when they make sense
// ------------------------------------------------------------------------------------------------
bool NetServer_BotPerkReady(ref rB, int iPerk)
{
	string sP = "p" + iPerk;
	if (CheckAttribute(rB, "APerk." + sP + ".Ready"))
	{
		if (iServerTime < sti(rB.APerk.(sP).Ready)) { return false; }
	}
	return true;
}

void NetServer_BotUsePerks(int i, ref rB, float fTurnErr, bool bFight)
{
	if (!bNetBotUsePerks) { return; }
	if (!sti(NetServer.ActivePerks)) { return; }

	float fMaxHP = stf(rB.Ship.MaxHP);
	float fHP = stf(rB.Ship.HP);
	float fLife = 1.0;
	if (fMaxHP > 1.0) { fLife = fHP / fMaxHP; }

	// reload faster when there is a target in range
	if (bFight && NetServer_BotPerkReady(rB, NET_APERK_IMMEDIATERELOAD)) { NetServer_UseActivePerk(i, NET_APERK_IMMEDIATERELOAD); }
	// sharp turn
	if (fTurnErr > 1.0 && NetServer_BotPerkReady(rB, NET_APERK_TURN180)) { NetServer_UseActivePerk(i, NET_APERK_TURN180); }
	// repairs
	if (fLife < 0.95 && NetServer_BotPerkReady(rB, NET_APERK_LIGHTREPAIR)) { NetServer_UseActivePerk(i, NET_APERK_LIGHTREPAIR); }
	if (fLife < 0.65 && NetServer_BotPerkReady(rB, NET_APERK_INSTANTREPAIR)) { NetServer_UseActivePerk(i, NET_APERK_INSTANTREPAIR); }
}

// the volley: the cannons are aimed like a deck camera at the target (the same data a player's NSC_SHIP_FIRE carries)
void NetServer_BotFire(ref rB, float fTX, float fTZ, float fTVX, float fTVZ)
{
	float fCX = stf(rB.Ship.Pos.x);
	float fCY = 6.0;
	float fCZ = stf(rB.Ship.Pos.z);

	float fDX = fTX - fCX;
	float fDY = 0.0 - fCY;
	float fDZ = fTZ - fCZ;
	float fLen = sqrt(fDX * fDX + fDY * fDY + fDZ * fDZ);
	if (fLen < 1.0) { return; }

	EntityUpdate(0);
	rB.Camera.Pos.x = fCX;
	rB.Camera.Pos.y = fCY;
	rB.Camera.Pos.z = fCZ;
	rB.Camera.Dir.x = fDX / fLen;
	rB.Camera.Dir.y = fDY / fLen;
	rB.Camera.Dir.z = fDZ / fLen;
	EntityUpdate(1);

	// the aim for the ballistics of NetServer_BotAimShot (the cannons of a volley fire during the next seconds)
	rB.BotAim.x = fTX;
	rB.BotAim.z = fTZ;
	rB.BotAim.vx = fTVX;
	rB.BotAim.vz = fTVZ;
	rB.BotAimUntil = iServerTime + 3000;

	Trace("COOP: bot fire pos=" + fCX + "," + fCZ + " target=" + fTX + "," + fTZ + " dist=" + fLen + " ay=" + rB.Ship.Ang.y);

	rB.DoCannonFire = "";
	rB.BotLastFire = iServerTime;
}

void NetServer_BotThink(int i)
{
	if (!NetServer_HasShip(i)) { return; }

	ref rB = NetServer_GetClient(i);

	// wreck / sinking ship: no captain
	if (sti(rB.Dead))
	{
		rB.Ship.TurnLeft = 0;
		rB.Ship.TurnRight = 0;
		rB.Ship.TurnState = 0.0;
		return;
	}

	NetServer_BotRestock(rB);

	float fX = stf(rB.Ship.Pos.x);
	float fZ = stf(rB.Ship.Pos.z);
	float fAy = stf(rB.Ship.Ang.y);

	float fWantAy = fAy;
	float fSail = 1.0;
	bool bFireOk = false;
	float fTX = 0.0;
	float fTZ = 0.0;
	float fTVX = 0.0;
	float fTVZ = 0.0;
	float fTurnErrAll = 0.0;
	bool bFight = false;

	int iTarget = NetServer_BotFindTarget(i);
	if (iTarget == DST_INVALID)
	{
		// nobody to fight: sail around, a new course every 8..16 seconds
		if (iServerTime > sti(rB.BotWanderUntil))
		{
			rB.BotWander = frnd() * PIm2;
			rB.BotWanderUntil = iServerTime + 8000 + rand(8000);
		}
		fWantAy = stf(rB.BotWander);
		fSail = 0.5;
	}
	else
	{
		ref rT = NetServer_GetClient(iTarget);
		fTX = stf(rT.Ship.Pos.x);
		fTZ = stf(rT.Ship.Pos.z);
		float fTSpd = stf(rT.Ship.Speed.z);
		fTVX = sin(stf(rT.Ship.Ang.y)) * fTSpd;
		fTVZ = cos(stf(rT.Ship.Ang.y)) * fTSpd;

		float fDX = fTX - fX;
		float fDZ = fTZ - fZ;
		float fDist = sqrt(fDX * fDX + fDZ * fDZ);
		bFight = (fDist < fNetBotFireDist + 150.0);
		float fBear = GetAngleY(fDX, fDZ);				// course to the target
		float fRel = NetServer_BotAngDiff(fBear, fAy);	// > 0: the target is on the right

		// far: straight at the target; closer than fNetBotCloseDist: a parallel course. The angle to the bearing is 90 degrees
		// at the keep distance, smaller when the target is further (the bot closes in), bigger when it is nearer (it opens out)
		float fSide = 0.0;
		if (fDist < fNetBotCloseDist + 100.0)
		{
			float fKeepT = (fDist - fNetBotKeepDist) / (fNetBotCloseDist - fNetBotKeepDist);
			if (fKeepT > 1.0) { fKeepT = 1.0; }
			if (fKeepT < -1.0) { fKeepT = -1.0; }
			fSide = PI / 2.0 - 0.8 * fKeepT;
			if (fDist > fNetBotCloseDist) { fSide = fSide * (1.0 - (fDist - fNetBotCloseDist) / 100.0); }
		}
		float fOffset = fSide;
		if (fRel < 0.0) { fOffset = 0.0 - fSide; }		// the target is on the left: keep it on the left
		fWantAy = fBear - fOffset;

		// fire: the target is abeam (left / right cannons), or in front / behind (bow / stern cannons)
		if (fDist < fNetBotFireDist)
		{
			float fAbsRel = fRel;
			if (fAbsRel < 0.0) { fAbsRel = 0.0 - fAbsRel; }
			float fAbeam = fAbsRel - PI / 2.0;
			if (fAbeam < 0.0) { fAbeam = 0.0 - fAbeam; }

			string sBort = "";
			if (fAbeam < fNetBotSideTol)
			{
				sBort = "cannonr";
				if (fRel < 0.0) { sBort = "cannonl"; }
			}
			if (sBort == "" && fAbsRel < 0.25) { sBort = "cannonf"; }
			if (sBort == "" && fAbsRel > PI - 0.25) { sBort = "cannonb"; }

			// the bort must be loaded (the engine reloads it by itself)
			if (sBort != "" && CheckAttribute(rB, "Ship.Cannons.Borts." + sBort + ".ChargeRatio"))
			{
				if (stf(rB.Ship.Cannons.Borts.(sBort).ChargeRatio) >= 0.99) { bFireOk = true; }
			}
		}

		// a sharp turn is made with half sails (SP: the turn rate is the best at half sails)
		float fTurnErr = NetServer_BotAngDiff(fWantAy, fAy);
		if (fTurnErr < 0.0) { fTurnErr = 0.0 - fTurnErr; }
		fTurnErrAll = fTurnErr;
		if (fTurnErr > 0.6) { fSail = 0.5; }
	}

	// a place where a ship has hit the island: turn away from it in time
	NetServer_BotHazardCheck(rB, fX, fZ, fAy);
	if (stf(rB.BotHazW) > 0.0)
	{
		float fHTurn = 0.7 + 0.9 * stf(rB.BotHazW);
		if (stf(rB.BotHazRel) >= 0.0) { fWantAy = fAy - fHTurn; }
		else { fWantAy = fAy + fHTurn; }
		if (stf(rB.BotHazW) > 0.5) { fSail = 0.5; }
		fTurnErrAll = fHTurn;
	}

	// stuck (an island, another ship): turn away for 3 seconds
	if (iServerTime < sti(rB.BotEscapeUntil))
	{
		fWantAy = fAy + 1.2;
		fSail = 0.5;
		bFireOk = false;
	}
	else
	{
		if (stf(rB.Ship.Speed.z) < 1.0 && stf(rB.Ship.SailState) > 0.4) { rB.BotStuck = sti(rB.BotStuck) + 1; }
		else { rB.BotStuck = 0; }

		// 10 thinks in a row (2 s) without speed: the ship does not move
		if (sti(rB.BotStuck) >= 10)
		{
			rB.BotStuck = 0;
			rB.BotEscapeUntil = iServerTime + 3000;
		}
	}

	// the rudder
	float fDiff = NetServer_BotAngDiff(fWantAy, fAy) * makefloat(iNetBotRudderSign);
	rB.Ship.TurnLeft = 0;
	rB.Ship.TurnRight = 0;
	if (fDiff > fNetBotTurnDead) { rB.Ship.TurnRight = 1; }
	if (fDiff < 0.0 - fNetBotTurnDead) { rB.Ship.TurnLeft = 1; }
	rB.Ship.TurnState = stf(rB.Ship.TurnRight) - stf(rB.Ship.TurnLeft);

	// the sails (NetServer_OnShip: SailState is 0 / 0.5 / 1)
	rB.Ship.SailState = Clampf(fSail);

	// fire
	if (bFireOk && iServerTime - sti(rB.BotLastFire) >= iNetBotFireGap)
	{
		NetServer_BotFire(rB, fTX, fTZ, fTVX, fTVZ);
	}

	NetServer_BotUsePerks(i, rB, fTurnErrAll, bFight);
}

void NetServer_BotTick()
{
	int iRound = GetEventData();

	// an event of an older game
	if (iRound != sti(NetServer.Round)) { return; }
	if (!bServerGameStarted) { bNetBotTickRunning = false; return; }

	int iBots = 0;
	for (int i=0; i<NET_MAXCLIENTS; i++)
	{
		if (!NetServer_IsBot(i)) { continue; }
		iBots++;
		NetServer_BotThink(i);
	}

	if (iBots <= 0) { bNetBotTickRunning = false; return; }
	PostEvent("NetServer_BotTick", NET_BOT_TICK, "l", iRound);
}

// the game is over: all bots go (NetServer_EndGame deletes the ship entities)
void NetServer_BotReset()
{
	bNetBotTickRunning = false;
	iNetBotSerial = 0;
	iNetBotHazN = 0;
	iNetBotHazNext = 0;
	for (int i=0; i<NET_MAXCLIENTS; i++)
	{
		if (!CheckAttribute(&NSClients[i], "Bot")) { continue; }
		NSClients[i].Bot = false;
	}
}
