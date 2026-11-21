// ============================================================================
// Ship load (weight), powder, mines, brander and jettison (throw cargo overboard)
// Rules are the single player ones (AIShip.c, AISeaGoods.c, ShipsUtilites.c)
// ============================================================================

// ---------- tunables ----------

bool bNetMineHitsTeam = false;		// mines / brander hurt the team of the owner
bool bNetBranderAllowed = true;
bool bNetMinesAllowed = true;

object NSMines[NET_MINE_MAX];

// ---------- load ----------
int NetServer_ShipCannonsNum(ref rClient)
{
	ref rShip = Net_GetShipByIndex(sti(rClient.Ship.Type));
	int iNum = sti(rShip.CannonsQuantity) - sti(rClient.Ship.CannonsDumped);
	if (iNum < 0) { iNum = 0; }
	return iNum;
}

// COOP: powder of the hold heavier than NET_POWDER_DETONATE_RATIO of the capacity (single player: powder / 20 > capacity * 0.25)
bool NetServer_PowderOverloaded(ref rClient)
{
	if (!CheckAttribute(rClient, "Ship.Goods.Powder")) { return false; }
	ref rShip = Net_GetShipByIndex(sti(rClient.Ship.Type));
	int iPowderIdx = Net_GetGoodIndex("Powder");
	if (iPowderIdx < 0) { return false; }
	float fPowderWeight = MakeFloat(Net_GetGoodWeight(iPowderIdx, sti(rClient.Ship.Goods.Powder)));
	return fPowderWeight > stf(rShip.Capacity) * NET_POWDER_DETONATE_RATIO;
}

// the powder of the ship goes off (the same blast as the brander, the shooter gets the kill)
void NetServer_PowderExplosion(int wClientID, int wShooterID)
{
	ref rClient = NetServer_GetClient(wClientID);
	if (NetServer_IsClientDead(rClient)) { return; }
	if (!CheckAttribute(rClient, "Ship.BranderActive")) { rClient.Ship.BranderActive = 0; }
	if (sti(rClient.Ship.BranderActive)) { return; }
	rClient.Ship.BranderActive = 1;
	rClient.Ship.BranderArmUntil = 0;
	rClient.Ship.ExplodedBy = wShooterID;
	Trace("COOP: powder of ship " + wClientID + " detonated (powder " + rClient.Ship.Goods.Powder + "), shot by " + wShooterID);
	PostEvent("NetServer_BranderBoom", NET_BRANDER_DELAY, "l", wClientID);
}

float NetServer_CannonsDamageRatio(ref rClient)
{
	ref rShip = Net_GetShipByIndex(sti(rClient.Ship.Type));
	float fBase = stf(rShip.CannonsQuantity);
	if (fBase < 1.0) { return 1.0; }
	return MakeFloat(NetServer_ShipCannonsNum(rClient)) / fBase;
}

// ---------- cannons on the borts ----------
// Ship.Bort.f / b / l / r - cannons standing on the bow, stern, left and right bort (no attribute - all places are full).
// The cannons not standing anywhere are not on board (they were thrown away): Ship.CannonsDumped = places - standing.
int NetServer_BortHave(ref rClient, int iBort)
{
	string sB = Net_BortLetter(iBort);
	if (CheckAttribute(rClient, "Ship.Bort." + sB)) { return sti(rClient.Ship.Bort.(sB)); }
	return Net_GetBortMax(sti(rClient.Ship.Type), iBort);
}

void NetServer_SyncCannonsDumped(ref rClient)
{
	int iSum = 0;
	for (int i=0; i<NET_BORT_NUM; i++) { iSum = iSum + NetServer_BortHave(rClient, i); }
	rClient.Ship.CannonsDumped = Net_GetShipCannonsNum(sti(rClient.Ship.Type)) - iSum;
}

// iCount cannons on board, spread over the borts like the places of the ship
void NetServer_SetBortsByCount(ref rClient, int iCount)
{
	int iType = sti(rClient.Ship.Type);
	for (int i=0; i<NET_BORT_NUM; i++)
	{
		string sB = Net_BortLetter(i);
		rClient.Ship.Bort.(sB) = Net_DistributeCannons(iType, iCount, i);
	}
	NetServer_SyncCannonsDumped(rClient);
}

// the cannon menu of the jettison screen: new numbers on the borts; the removed cannons that are not placed go overboard
void NetServer_ShipCannonsSet(int wClientID, int iMsg)
{
	int i;
	int iNew[4];
	for (i=0; i<NET_BORT_NUM; i++) { iNew[i] = NMGetByte(iMsg); }

	ref rClient = NetServer_GetClient(wClientID);
	if (NetServer_IsClientDead(rClient)) { return; }

	int iType = sti(rClient.Ship.Type);
	int iOnBoard = NetServer_ShipCannonsNum(rClient);
	int iSum = 0;
	for (i=0; i<NET_BORT_NUM; i++)
	{
		if (iNew[i] < 0) { iNew[i] = 0; }
		if (iNew[i] > Net_GetBortMax(iType, i)) { iNew[i] = Net_GetBortMax(iType, i); }
		iSum = iSum + iNew[i];
	}
	if (iSum > iOnBoard) { NetServer_ShipSendLoadInfo(wClientID); return; }		// no cannons from nowhere

	for (i=0; i<NET_BORT_NUM; i++)
	{
		string sB = Net_BortLetter(i);
		rClient.Ship.Bort.(sB) = iNew[i];
	}
	NetServer_SyncCannonsDumped(rClient);
	NetServer_CannonRecalculateParameters(rClient);
	Trace("COOP: cannons of " + wClientID + " set f=" + iNew[0] + " b=" + iNew[1] + " l=" + iNew[2] + " r=" + iNew[3] + ", thrown " + (iOnBoard - iSum));

	if (iSum < iOnBoard) { NetServer_SendSplash(stf(rClient.Ship.Pos.x), 0.0, stf(rClient.Ship.Pos.z)); }
	NetServer_ShipSendLoadInfo(wClientID);
}

// COOP: pieces of a good on board: in the hold + held back from the engine (charges locked: no powder / no cannons)
int NetServer_GoodTotal(ref rClient, string sGood)
{
	int iNum = sti(rClient.Ship.Goods.(sGood));
	if (CheckAttribute(rClient, "Ship.GoodsHeld." + sGood)) { iNum = iNum + sti(rClient.Ship.GoodsHeld.(sGood)); }
	return iNum;
}

// the charges go out of reach of the engine (it loads the cannons from Ship.Goods), they stay on board
void NetServer_HoldCharges(ref rClient)
{
	for (int i=0; i<4; i++)
	{
		string sGood = NetGoods[i].Name;
		int iNum = sti(rClient.Ship.Goods.(sGood));
		if (iNum <= 0) { continue; }
		int iHeld = 0;
		if (CheckAttribute(rClient, "Ship.GoodsHeld." + sGood)) { iHeld = sti(rClient.Ship.GoodsHeld.(sGood)); }
		rClient.Ship.GoodsHeld.(sGood) = iHeld + iNum;
		rClient.Ship.Goods.(sGood) = 0;
	}
}

int NetServer_ShipLoad(ref rClient)
{
	int i;
	int iLoad = 0;
	int iCannonType = sti(rClient.Ship.Cannons.Type);
	if (iCannonType >= 0)
	{
		ref rCannon = Net_GetCannonByIndex(iCannonType);
		iLoad = iLoad + NetServer_ShipCannonsNum(rClient) * sti(rCannon.SPWeight);
	}
	iLoad = iLoad + sti(rClient.Ship.Crew.Quantity);
	for (i=0; i<Net_GetGoodsNum(); i++)
	{
		string sGood = NetGoods[i].Name;
		iLoad = iLoad + Net_GetGoodWeight(i, NetServer_GoodTotal(rClient, sGood));
	}
	return iLoad;
}

float NetServer_ShipLoadRatio(ref rClient)
{
	ref rShip = Net_GetShipByIndex(sti(rClient.Ship.Type));
	float fCapacity = stf(rShip.Capacity);
	if (fCapacity < 1.0) { fCapacity = 1.0; }
	return Clampf(MakeFloat(NetServer_ShipLoad(rClient)) / fCapacity);
}

// ---------- powder ----------
int NetServer_AmmoSum(ref rClient)
{
	int iSum = 0;
	for (int i=0; i<4; i++)		// Balls, Grapes, Knippels, Bombs
	{
		string sGood = NetGoods[i].Name;
		iSum = iSum + sti(rClient.Ship.Goods.(sGood));
	}
	return iSum;
}

// The engine takes the charge (ball) from Ship.Goods when a cannon is loaded and gives it back when unloaded.
// Powder is taken / given back in the same amount (SP: Cannon_LoadBall takes 1 ball and 1 powder).
// No powder - the charges are locked and the ship cannot load cannons any more.
void NetServer_PowderTrack(ref rClient)
{
	// COOP: no cannons on board (all thrown away / none bought): the charges are held, the cannons can not be loaded
	if (NetServer_ShipCannonsNum(rClient) <= 0)
	{
		NetServer_HoldCharges(rClient);
		rClient.Ship.AmmoLast = 0;
		return;
	}

	int iSum = NetServer_AmmoSum(rClient);
	if (!CheckAttribute(rClient, "Ship.AmmoLast")) { rClient.Ship.AmmoLast = iSum; return; }

	int iDelta = sti(rClient.Ship.AmmoLast) - iSum;		// > 0 - charges loaded to cannons, < 0 - unloaded
	int iPowder = sti(rClient.Ship.Goods.Powder);
	if (iDelta != 0)
	{
		iPowder = iPowder - iDelta;
		if (iPowder < 0) { iPowder = 0; }
		rClient.Ship.Goods.Powder = iPowder;
	}

	if (iPowder <= 0 && iSum > 0)
	{
		NetServer_HoldCharges(rClient);		// COOP: added to the held ones (a second lock does not lose the first)
		iSum = 0;
	}
	rClient.Ship.AmmoLast = iSum;
}

// ---------- mines ----------
void NetServer_MinesClear()
{
	for (int i=0; i<NET_MINE_MAX; i++) { NSMines[i].Use = 0; }
}

void NetServer_MineSend(int iCode, int iSlot, float x, float z, int wOwner)
{
	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_SHIP);
	NMAddByte(iSMsg, iCode);
	NMAddByte(iSMsg, iSlot);
	NMAddFloat(iSMsg, x);
	NMAddFloat(iSMsg, z);
	NMAddClientID(iSMsg, wOwner);
	NetServer_SendMessage(DST_ALL, iSMsg, true);
	NMDelete(iSMsg);
}

// splash of something thrown into the water: every player near the place hears it (3D sound, distance is set by the alias)
void NetServer_SendSplash(float x, float y, float z)
{
	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_SHIP);
	NMAddByte(iSMsg, NSC_SHIP_SPLASH);
	NMAddFloat(iSMsg, x);
	NMAddFloat(iSMsg, y);
	NMAddFloat(iSMsg, z);
	NetServer_SendMessage(DST_ALL, iSMsg, true);
	NMDelete(iSMsg);
}

// tells the player why a mine / brander was refused (and writes it to compile.log)
void NetServer_ShipActionDenied(int wClientID, int iReason, int iHave, int iNeed)
{
	Trace("COOP: ship action denied client=" + wClientID + " reason=" + iReason + " have=" + iHave + " need=" + iNeed);

	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_SHIP);
	NMAddByte(iSMsg, NSC_SHIP_ACTION_DENIED);
	NMAddByte(iSMsg, iReason);
	NMAddWord(iSMsg, iHave);
	NMAddWord(iSMsg, iNeed);
	NetServer_SendMessage(wClientID, iSMsg, true);
	NMDelete(iSMsg);
}

void NetServer_ShipDropMine(int wClientID)
{
	if (!bNetMinesAllowed) { return; }
	ref rClient = NetServer_GetClient(wClientID);
	if (NetServer_IsClientDead(rClient)) { return; }

	// the attributes are created here too: a ship that did not pass the lobby code of this build must not break the request
	if (!CheckAttribute(rClient, "Ship.MineCooldown")) { rClient.Ship.MineCooldown = 0; }

	if (iServerTime < sti(rClient.Ship.MineCooldown))
	{
		NetServer_ShipActionDenied(wClientID, NET_DENY_MINE_COOLDOWN, (sti(rClient.Ship.MineCooldown) - iServerTime + 999) / 1000, 0);
		return;
	}

	int iPowder = sti(rClient.Ship.Goods.Powder);
	if (iPowder < NET_MINE_POWDER)
	{
		NetServer_ShipActionDenied(wClientID, NET_DENY_POWDER, iPowder, NET_MINE_POWDER);
		return;
	}

	int iSlot = -1;
	for (int i=0; i<NET_MINE_MAX; i++) { if (!sti(NSMines[i].Use)) { iSlot = i; break; } }
	if (iSlot < 0)
	{
		NetServer_ShipActionDenied(wClientID, NET_DENY_MINE_LIMIT, 0, NET_MINE_MAX);
		return;
	}

	ref rShip = Net_GetShipByIndex(sti(rClient.Ship.Type));
	float fBack = 14.0 + (7.0 - stf(rShip.Class)) * 4.0;		// behind the stern
	float ay = stf(rClient.Ship.Ang.y);
	float x = stf(rClient.Ship.Pos.x) - sin(ay) * fBack;
	float z = stf(rClient.Ship.Pos.z) - cos(ay) * fBack;

	rClient.Ship.Goods.Powder = sti(rClient.Ship.Goods.Powder) - NET_MINE_POWDER;
	rClient.Ship.MineCooldown = iServerTime + NET_MINE_COOLDOWN;

	NSMines[iSlot].Use = 1;
	NSMines[iSlot].x = x;
	NSMines[iSlot].z = z;
	NSMines[iSlot].Owner = wClientID;
	NSMines[iSlot].Born = iServerTime;
	NSMines[iSlot].Expire = iServerTime + NET_MINE_LIFETIME;

	Trace("COOP: mine dropped client=" + wClientID + " slot=" + iSlot + " x=" + x + " z=" + z + " powderLeft=" + sti(rClient.Ship.Goods.Powder));
	NetServer_MineSend(NSC_SHIP_MINE_ADD, iSlot, x, z, wClientID);
	NetServer_SendSplash(x, 0.0, z);
}

void NetServer_MinesTick()
{
	int i, j;
	for (i=0; i<NET_MINE_MAX; i++)
	{
		if (!sti(NSMines[i].Use)) { continue; }
		float mx = stf(NSMines[i].x);
		float mz = stf(NSMines[i].z);
		int wOwner = sti(NSMines[i].Owner);

		if (iServerTime >= sti(NSMines[i].Expire))
		{
			NSMines[i].Use = 0;
			NetServer_MineSend(NSC_SHIP_MINE_DEL, i, mx, mz, wOwner);
			continue;
		}

		for (j=0; j<NET_MAXCLIENTS; j++)
		{
			if (!NetServer_HasShip(j)) { continue; }		// COOP: lobby players of the running game have no ship (parked ships do)
			ref rVictim = &NSClients[j];
			if (NetServer_IsClientDead(rVictim)) { continue; }
			if (!CheckAttribute(rVictim, "Ship.Pos.x")) { continue; }
			if (j == wOwner && iServerTime < sti(NSMines[i].Born) + NET_MINE_ARM_TIME) { continue; }
			if (j != wOwner && !bNetMineHitsTeam && NetServer_IsTeam(wOwner, j)) { continue; }

			ref rShip = Net_GetShipByIndex(sti(rVictim.Ship.Type));
			float fR = 8.0 + (7.0 - stf(rShip.Class)) * 3.0;
			float dx = stf(rVictim.Ship.Pos.x) - mx;
			float dz = stf(rVictim.Ship.Pos.z) - mz;
			if (dx * dx + dz * dz > fR * fR) { continue; }

			NSMines[i].Use = 0;
			NetServer_MineSend(NSC_SHIP_MINE_BOOM, i, mx, mz, wOwner);
			// COOP: own mine / mine of another player (the kill feed has its own lines for both)
			int iMineKill = NETKILL_BY_MINE;
			int wKiller = wOwner;
			if (wKiller == j) { iMineKill = NETKILL_BY_OWN_MINE; wKiller = DST_INVALID; }
			if (!NetServer_IsClient(wKiller)) { wKiller = DST_INVALID; }
			NetServer_ShipApplyHullHitpoints(rVictim, NET_MINE_DAMAGE, iMineKill, wKiller);
			break;
		}
	}
	PostEvent("NetServer_MinesTick", 150);
}

// ---------- brander ----------
// COOP: the brander is an active perk of single player: the "All active perks" switch or (s28) the perk bought in the lobby
bool NetServer_BranderAllowed(int wClientID)
{
	if (!bNetBranderAllowed) { return false; }
	return NetServer_SeaPerk(wClientID, NET_SPERK_BRANDER);
}

// first press: the brander is armed for NET_BRANDER_ARM_TIME ms. If the ship is boarded in this time, the brander goes off
void NetServer_ShipBranderArm(int wClientID)
{
	if (!NetServer_BranderAllowed(wClientID)) { return; }
	ref rClient = NetServer_GetClient(wClientID);
	if (NetServer_IsClientDead(rClient)) { return; }
	rClient.Ship.BranderArmUntil = iServerTime + NET_BRANDER_ARM_TIME;
}

bool NetServer_BranderIsArmed(ref rClient)
{
	if (!NetServer_BranderAllowed(sti(rClient.ID))) { return false; }
	if (!CheckAttribute(rClient, "Ship.BranderArmUntil")) { return false; }
	return iServerTime < sti(rClient.Ship.BranderArmUntil);
}

bool NetServer_BranderIsActive(ref rClient)
{
	if (!CheckAttribute(rClient, "Ship.BranderActive")) { return false; }
	return sti(rClient.Ship.BranderActive) != 0;
}

void NetServer_ShipBrander(int wClientID)
{
	if (!NetServer_BranderAllowed(wClientID)) { return; }
	ref rClient = NetServer_GetClient(wClientID);
	if (NetServer_IsClientDead(rClient)) { return; }
	if (!CheckAttribute(rClient, "Ship.BranderActive")) { rClient.Ship.BranderActive = 0; }
	if (sti(rClient.Ship.BranderActive)) { return; }

	int iPowder = sti(rClient.Ship.Goods.Powder);
	if (iPowder < NET_BRANDER_MIN_POWDER)
	{
		NetServer_ShipActionDenied(wClientID, NET_DENY_POWDER, iPowder, NET_BRANDER_MIN_POWDER);
		return;
	}
	rClient.Ship.BranderActive = 1;
	rClient.Ship.BranderArmUntil = 0;
	PostEvent("NetServer_BranderBoom", NET_BRANDER_DELAY, "l", wClientID);
}

void NetServer_BranderBoom()
{
	int wClientID = GetEventData();
	ref rClient = NetServer_GetClient(wClientID);
	if (NetServer_IsClientDead(rClient)) { return; }

	int iPowder = sti(rClient.Ship.Goods.Powder);
	ref rShip = Net_GetShipByIndex(sti(rClient.Ship.Type));
	float fMaxDamage = 1000.0 + MakeFloat(iPowder) * 4.0;					// same as Ship_Detonate in SP
	float fBranderDistance = 1.9 * (45.0 + (7.0 - stf(rShip.Class)) * 15.0);
	float bx = stf(rClient.Ship.Pos.x);
	float by = stf(rClient.Ship.Pos.y);
	float bz = stf(rClient.Ship.Pos.z);

	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_SHIP);
	NMAddByte(iSMsg, NSC_SHIP_BRANDER_BOOM);
	NMAddFloat(iSMsg, bx);
	NMAddFloat(iSMsg, by);
	NMAddFloat(iSMsg, bz);
	NetServer_SendMessage(DST_ALL, iSMsg, true);
	NMDelete(iSMsg);

	for (int j=0; j<NET_MAXCLIENTS; j++)
	{
		if (j == wClientID) { continue; }
		if (!NetServer_HasShip(j)) { continue; }		// COOP: lobby players of the running game have no ship
		ref rVictim = &NSClients[j];
		if (NetServer_IsClientDead(rVictim)) { continue; }
		if (!bNetMineHitsTeam && NetServer_IsTeam(wClientID, j)) { continue; }
		float dx = stf(rVictim.Ship.Pos.x) - bx;
		float dz = stf(rVictim.Ship.Pos.z) - bz;
		float fDist = sqrt(dx * dx + dz * dz);
		if (fDist > fBranderDistance) { continue; }
		if (fDist < 10.0) { fDist = 10.0; }
		float fBlast = (fBranderDistance - fDist) / (fBranderDistance - 10.0);		// 1.0 at the epicenter .. 0.0 at the edge
		float fDamage = fMaxDamage * fBlast;
		// the blast kills the crew too (before the hull damage: a ship sunk by it loses the whole crew anyway)
		NetServer_ShipApplyCrewHitpoints(rVictim, stf(rVictim.Ship.Crew.Quantity) * NET_BRANDER_CREW_DAMAGE * fBlast, wClientID);
		NetServer_ShipApplyHullHitpoints(rVictim, fDamage, NETKILL_BY_BRANDER, wClientID);
	}

	rClient.ForceDetonate = 1;
	rClient.Ship.Goods.Powder = 0;
	rClient.Ship.Crew.Quantity = 0;		// the brander crew dies in its own explosion
	// COOP: far more than the hull (the passive perk ShipDefenseProfessional takes 35% of any damage)
	DeleteAttribute(rClient, "SpawnProtect");
	// the own brander: no killer; the powder blown up by a hit: the shooter sank the ship
	int iSelfKill = NETKILL_BY_OWN_BRANDER;
	int wSelfKiller = DST_INVALID;
	if (CheckAttribute(rClient, "Ship.ExplodedBy"))
	{
		wSelfKiller = sti(rClient.Ship.ExplodedBy);
		iSelfKill = NETKILL_BY_BALL;
		DeleteAttribute(rClient, "Ship.ExplodedBy");
	}
	NetServer_ShipApplyHullHitpoints(rClient, stf(rClient.Ship.HP) * 10.0 + 100.0, iSelfKill, wSelfKiller);
}

// ---------- jettison ----------
void NetServer_ShipSendLoadInfo(int wClientID)
{
	ref rClient = NetServer_GetClient(wClientID);
	ref rShip = Net_GetShipByIndex(sti(rClient.Ship.Type));
	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_SHIP);
	NMAddByte(iSMsg, NSC_SHIP_LOAD_INFO);
	NMAddWord(iSMsg, NetServer_ShipLoad(rClient));
	NMAddWord(iSMsg, sti(rShip.Capacity));
	NMAddWord(iSMsg, sti(rClient.Ship.Crew.Quantity));
	NMAddWord(iSMsg, NetServer_ShipCannonsNum(rClient));
	for (int i=0; i<Net_GetGoodsNum(); i++)
	{
		string sGood = NetGoods[i].Name;
		NMAddWord(iSMsg, NetServer_GoodTotal(rClient, sGood));
	}
	for (int b=0; b<NET_BORT_NUM; b++)		// COOP: cannons on the borts and their places
	{
		NMAddByte(iSMsg, NetServer_BortHave(rClient, b));
		NMAddByte(iSMsg, Net_GetBortMax(sti(rClient.Ship.Type), b));
	}
	NetServer_SendMessage(wClientID, iSMsg, true);
	NMDelete(iSMsg);
}

void NetServer_ShipJettison(int wClientID, int iMsg)
{
	int i;
	int iGoods[8];
	for (i=0; i<Net_GetGoodsNum(); i++) { iGoods[i] = NMGetWord(iMsg); }
	int iCannons = NMGetWord(iMsg);
	int iCrew = NMGetWord(iMsg);

	ref rClient = NetServer_GetClient(wClientID);
	if (NetServer_IsClientDead(rClient)) { return; }

	bool bThrown = false;
	for (i=0; i<Net_GetGoodsNum(); i++)
	{
		string sGood = NetGoods[i].Name;
		int iHave = NetServer_GoodTotal(rClient, sGood);
		if (iGoods[i] > iHave) { iGoods[i] = iHave; }
		if (iGoods[i] > 0)
		{
			int iInHold = sti(rClient.Ship.Goods.(sGood));
			int iFromHold = iGoods[i];
			if (iFromHold > iInHold) { iFromHold = iInHold; }
			rClient.Ship.Goods.(sGood) = iInHold - iFromHold;
			if (iGoods[i] > iFromHold) { rClient.Ship.GoodsHeld.(sGood) = sti(rClient.Ship.GoodsHeld.(sGood)) - (iGoods[i] - iFromHold); }
			bThrown = true;
		}
	}
	// charges thrown away are not shot: do not take powder for them
	rClient.Ship.AmmoLast = NetServer_AmmoSum(rClient);

	int iCannonsHave = NetServer_ShipCannonsNum(rClient);
	if (iCannons > iCannonsHave) { iCannons = iCannonsHave; }
	if (iCannons > 0)
	{
		NetServer_SetBortsByCount(rClient, iCannonsHave - iCannons);		// COOP: the cannons stand on the borts
		bThrown = true;
	}

	int iCrewHave = sti(rClient.Ship.Crew.Quantity);
	if (iCrew > iCrewHave - 1) { iCrew = iCrewHave - 1; }
	if (iCrew > 0) { rClient.Ship.Crew.Quantity = iCrewHave - iCrew; bThrown = true; }

	// splash for everybody around (single player: Ships\jakor_002.wav)
	if (bThrown) { NetServer_SendSplash(stf(rClient.Ship.Pos.x), 0.0, stf(rClient.Ship.Pos.z)); }

	NetServer_ShipSendLoadInfo(wClientID);
}
