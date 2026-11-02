// COOP: NaN guard for client camera data
bool COOP_SrvFloatOk(float f)
{
	return (f > -100000.0) && (f < 100000.0);
}

#define NET_REPAIR_COOLDOWN			60

// ============================================================================
// Auto boarding settings. Perks of single player that are switched on for EVERY captain by default.
// Set a variable to false to turn that perk off (then it is taken from Perks.<name> of the client, which the lobby does not set yet).
// ============================================================================
bool bNetBoardPerkLongRangeGrappling	= true;	// boarding distance x1.15
bool bNetBoardPerkGrapplingProfessional	= true;	// boarding distance x1.25 and no angle / speed test (like in single player)
bool bNetBoardPerkMusketsShoot			= true;	// musket volley before the boarding (LAi_boarding.c)

void NetServer_DeleteShipEnvironment()
{
	DelEventHandler("NetServer_ShipUpdateParameters", "NetServer_ShipUpdateParameters");
	DelEventHandler("NetServer_OnShipMastDamage", "NetServer_OnShipMastDamage");
	DelEventHandler("NetServer_OnShipHullHit", "NetServer_OnShipHullHit");
	DelEventHandler("NetServer_ShipFireDamage", "NetServer_ShipFireDamage");
	DelEventHandler("NetServer_OnShipRepair", "NetServer_OnShipRepair");

	// collision
	DelEventHandler("NetServer_Ship2ShipCollision", "NetServer_Ship2ShipCollision");
	DelEventHandler("NetServer_Ship2IslandCollision", "NetServer_Ship2IslandCollision");
}

void NetServer_CreateShipEnvironment(int iSMsg)
{
	SetEventHandler("NetServer_ShipUpdateParameters", "NetServer_ShipUpdateParameters", 0);
	SetEventHandler("NetServer_OnShipMastDamage", "NetServer_OnShipMastDamage", 0);
	SetEventHandler("NetServer_OnShipHullHit", "NetServer_OnShipHullHit", 0);
	SetEventHandler("NetServer_ShipFireDamage", "NetServer_ShipFireDamage", 0);
	SetEventHandler("NetServer_OnShipRepair", "NetServer_OnShipRepair", 0);

	// collision
	SetEventHandler("NetServer_Ship2ShipCollision", "NetServer_Ship2ShipCollision", 0);
	SetEventHandler("NetServer_Ship2IslandCollision", "NetServer_Ship2IslandCollision", 0);
}

void NetServer_OnShip(int wNetClientID, int iMsg)
{
	int iSubCode = NMGetByte(iMsg);

	ref rClient = NetServer_GetClient(wNetClientID);

	int iOldSailState = makeint(stf(rClient.Ship.SailState) * 2.0);
	int iOldTurnState = makeint(stf(rClient.Ship.TurnState) * 2.0);

	switch (iSubCode)
	{
		case NSC_SHIP_TURNLEFT_ON:
			rClient.Ship.TurnLeft = 1;
		break;
		case NSC_SHIP_TURNRIGHT_ON:
			rClient.Ship.TurnRight = 1;
		break;
		case NSC_SHIP_TURNLEFT_OFF:
			rClient.Ship.TurnLeft = 0;
		break;
		case NSC_SHIP_TURNRIGHT_OFF:
			rClient.Ship.TurnRight = 0;
		break;
		case NSC_SHIP_SETSAILSTATE:
			rClient.Ship.SailState = Clampf(NMGetByte(iMsg) / 2.0);
		break;
		case NSC_SHIP_SAILUP:
			rClient.Ship.SailState = stf(rClient.Ship.SailState) + 0.5;
			Trace("COOP: srv sailup client=" + wNetClientID + " sail=" + rClient.Ship.SailState);
		break;
		case NSC_SHIP_SAILDOWN:
			rClient.Ship.SailState = stf(rClient.Ship.SailState) - 0.5;
		break;
		case NSC_SHIP_FIRE:
			bool bDeckCamera = NMGetByte(iMsg);

			if (bDeckCamera)
			{
				EntityUpdate(0);
				rClient.Camera.Pos.x = NMGetFloat(iMsg);
				rClient.Camera.Pos.y = NMGetFloat(iMsg);
				rClient.Camera.Pos.z = NMGetFloat(iMsg);

				rClient.Camera.Dir.x = NMGetFloat(iMsg);
				rClient.Camera.Dir.y = NMGetFloat(iMsg);
				rClient.Camera.Dir.z = NMGetFloat(iMsg);
				EntityUpdate(1);
				Trace("COOP: srv fire client=" + wNetClientID + " camPos=" + rClient.Camera.Pos.x + "," + rClient.Camera.Pos.y + "," + rClient.Camera.Pos.z + " camDir=" + rClient.Camera.Dir.x + "," + rClient.Camera.Dir.y + "," + rClient.Camera.Dir.z + " shipPos=" + rClient.Ship.Pos.x + "," + rClient.Ship.Pos.y + "," + rClient.Ship.Pos.z);

				if (COOP_SrvFloatOk(stf(rClient.Camera.Pos.x)) && COOP_SrvFloatOk(stf(rClient.Camera.Pos.y)) && COOP_SrvFloatOk(stf(rClient.Camera.Pos.z)) && COOP_SrvFloatOk(stf(rClient.Camera.Dir.x)) && COOP_SrvFloatOk(stf(rClient.Camera.Dir.y)) && COOP_SrvFloatOk(stf(rClient.Camera.Dir.z)))
				{
					rClient.DoCannonFire = "";
				}
				else
				{
					Trace("COOP: srv fire ignored, camera is not finite, client=" + wNetClientID);
				}
			}
			else
			{
				rClient.DoCannonFireCameraOutside = "";
			}
		break;
		case NSC_SHIP_CHANGE_CHARGE:
			NetServer_ShipChangeCharge(wNetClientID, NMGetString(iMsg));
		break;
		case NSC_SHIP_START_REPAIR:
			NetServer_StartRepair(wNetClientID);
		break;
		case NSC_SHIP_BOARD_REQUEST:
			NetServer_TryBoardPlayer(wNetClientID, NMGetClientID(iMsg));
		break;
	}

	rClient.Ship.SailState = Clampf(stf(rClient.Ship.SailState));
	rClient.Ship.TurnState = stf(rClient.Ship.TurnRight) - stf(rClient.Ship.TurnLeft);
}


// ============================================================================
// Auto boarding player vs player (no land fight).
// Rules are the single player ones:
//   availability  - Ship_CheckMainCharacter: distance minus half widths of both ships, angle between
//                   the ships, relative speed; Grappling skill and perks LongRangeGrappling /
//                   GrapplingProfessional (the last one ignores angle and speed)
//   result        - Ship_AutoAbordage: strength = 0.1 + grappling * crew, bigger strength wins,
//                   winner loses crew * loserStrength / (winnerStrength * 1.8), loser ship is sunk
// The server is authoritative: clients only get "can board X" and send a request.
// ============================================================================
bool NetServer_HasPerk(ref rClient, string sPerk)
{
	if (sPerk == "LongRangeGrappling" && bNetBoardPerkLongRangeGrappling) { return true; }
	if (sPerk == "GrapplingProfessional" && bNetBoardPerkGrapplingProfessional) { return true; }
	if (sPerk == "MusketsShoot" && bNetBoardPerkMusketsShoot) { return true; }
	string sAttr = "Perks." + sPerk;
	if (!CheckAttribute(rClient, sAttr)) { return false; }
	return sti(rClient.Perks.(sPerk)) != 0;
}

// 0..1, like TmpSkill.Grappling in single player (skill 1..10 in the lobby)
float NetServer_GetGrappling(ref rClient)
{
	if (!CheckAttribute(rClient, "Skills.Grappling")) { return 1.0 / NETSKILL_MAX; }
	return stf(rClient.Skills.Grappling) / NETSKILL_MAX;
}

float NetServer_CalcGrapplingPerk(ref rClient)
{
	float fPerk = 1.0;
	if (NetServer_HasPerk(rClient, "LongRangeGrappling")) { fPerk = 1.15; }
	if (NetServer_HasPerk(rClient, "GrapplingProfessional")) { fPerk = 1.25; }
	return fPerk;
}

float NetServer_GetShipHalfWidth(ref rClient)
{
	if (CheckAttribute(rClient, "Ship.BoxSize.x")) { return stf(rClient.Ship.BoxSize.x) / 2.0; }
	return 10.0;
}

// slot is a connected player with a spawned ship (all attributes the boarding code reads exist)
bool NetServer_BoardSlotReady(int wClientID)
{
	if (wClientID < 0 || wClientID >= NET_MAXCLIENTS) { return false; }
	ref rC = NetServer_GetClient(wClientID);
	if (!CheckAttribute(rC, "Use") || !sti(rC.Use)) { return false; }
	if (!CheckAttribute(rC, "Ship.HP")) { return false; }
	if (!CheckAttribute(rC, "Ship.Pos.x") || !CheckAttribute(rC, "Ship.Pos.z")) { return false; }
	if (!CheckAttribute(rC, "Ship.Crew.Quantity")) { return false; }
	if (!CheckAttribute(rC, "Team")) { return false; }
	return true;
}

// can be boarded at all (alive, not a fort, not a teammate)
bool NetServer_IsBoardable(int wAttackerID, int wTargetID)
{
	if (wAttackerID == wTargetID) { return false; }
	if (wAttackerID < 0 || wAttackerID >= NET_MAXCLIENTS) { return false; }
	if (wTargetID < 0 || wTargetID >= NET_MAXCLIENTS) { return false; }

	// NSClients[i] has attributes (index, id, Use...) for EVERY slot, so NetServer_IsClient() is true for empty slots too.
	// Reading Dead / Ship.* of an empty slot spammed error.log (hundreds of lines per second) and froze the host.
	if (!NetServer_BoardSlotReady(wAttackerID)) { return false; }
	if (!NetServer_BoardSlotReady(wTargetID)) { return false; }

	ref rA = NetServer_GetClient(wAttackerID);
	ref rT = NetServer_GetClient(wTargetID);

	if (CheckAttribute(rA, "Dead") && sti(rA.Dead)) { return false; }
	if (CheckAttribute(rT, "Dead") && sti(rT.Dead)) { return false; }
	if (stf(rA.Ship.HP) <= 0.0) { return false; }
	if (stf(rT.Ship.HP) <= 0.0) { return false; }

	// teammates cannot board each other (everybody is an enemy in deathmatch)
	if (sti(NetServer.GameType) != NETGAME_DEATHMATCH && sti(rA.Team) == sti(rT.Team)) { return false; }

	return true;
}

// bLoose: used for the final check when the request arrives (ships moved a bit since the button was shown)
bool NetServer_CheckBoardPair(ref rA, ref rT, bool bLoose)
{
	float fDX = stf(rA.Ship.Pos.x) - stf(rT.Ship.Pos.x);
	float fDZ = stf(rA.Ship.Pos.z) - stf(rT.Ship.Pos.z);
	float fDistance = sqrt(fDX * fDX + fDZ * fDZ);
	if (fDistance > 400.0) { return false; }

	float fGrappling = NetServer_GetGrappling(rA);
	bool bProfessional = NetServer_HasPerk(rA, "GrapplingProfessional");

	float fMaxDistance = NetServer_CalcGrapplingPerk(rA) * Bring2Range(15.0, 40.0, 0.0, 1.0, fGrappling);
	float fMaxAngle = Bring2Range(0.70, 0.45, 0.0, 1.0, fGrappling);
	float fMaxSpeed = Bring2Range(3.0, 5.0, 0.0, 1.0, fGrappling);

	if (bLoose)
	{
		fMaxDistance = fMaxDistance * 1.25 + 8.0;
		fMaxAngle = fMaxAngle - 0.15;
		fMaxSpeed = fMaxSpeed + 1.5;
	}

	// distance between hulls, not between centers: bigger ships can board from further away
	float fAbordageDistance = fDistance - NetServer_GetShipHalfWidth(rA) - NetServer_GetShipHalfWidth(rT);
	if (fAbordageDistance >= fMaxDistance) { return false; }

	if (!bProfessional)
	{
		float fSpeedA = stf(rA.Ship.Speed.z);
		float fSpeedT = stf(rT.Ship.Speed.z);

		// dot(ay, ay): ~1 when the ships go side by side (same or opposite course), ~0 when nose to nose at an angle
		float fDot = GetDotProduct(stf(rA.Ship.Ang.y), stf(rT.Ship.Ang.y));
		if (fDot < 0.0) { fDot = -fDot; }
		if (fDot <= fMaxAngle) { return false; }

		float fRel2 = fSpeedA * fSpeedA + fSpeedT * fSpeedT - 2.0 * fSpeedA * fSpeedT * GetDotProduct(stf(rA.Ship.Ang.y), stf(rT.Ship.Ang.y));
		if (fRel2 < 0.0) { fRel2 = 0.0; }
		if (sqrt(fRel2) >= fMaxSpeed) { return false; }
	}

	// as in Ship_AutoAbordage: do not attack a ship that has much more crew
	if (stf(rA.Ship.Crew.Quantity) < stf(rT.Ship.Crew.Quantity) * NET_BOARD_MIN_CREW_RATIO) { return false; }

	return true;
}

// 0..1 like TmpSkill.Defence in single player
float NetServer_GetDefenceSkill(ref rClient)
{
	if (!CheckAttribute(rClient, "Skills.Defence")) { return 1.0 / NETSKILL_MAX; }
	return stf(rClient.Skills.Defence) / NETSKILL_MAX;
}

// Musket volley info for ONE captain (the two captains of the boarding get their own message, nobody else).
// Everything is from the point of view of the receiver: "our" = his ship, "enemy" = the other ship.
void NetServer_SendBoardVolley(int wClientID, int iOurShot, int iEnemyShot, int iKilledByUs, int iKilledByEnemy)
{
	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_SHIP);
	NMAddByte(iSMsg, NSC_SHIP_BOARD_VOLLEY);
	NMAddByte(iSMsg, iOurShot);
	NMAddByte(iSMsg, iEnemyShot);
	NMAddWord(iSMsg, iKilledByUs);
	NMAddWord(iSMsg, iKilledByEnemy);
	NetServer_SendMessage(wClientID, iSMsg, true);
	NMDelete(iSMsg);
}

// Crew lost in the boarding fight itself, for ONE captain, from his point of view
void NetServer_SendBoardResult(int wClientID, int iOurLost, int iEnemyLost)
{
	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_SHIP);
	NMAddByte(iSMsg, NSC_SHIP_BOARD_RESULT);
	NMAddWord(iSMsg, iOurLost);
	NMAddWord(iSMsg, iEnemyLost);
	NetServer_SendMessage(wClientID, iSMsg, true);
	NMDelete(iSMsg);
}

// nearest ship that can be boarded by wAttackerID, -1 if none
int NetServer_FindBoardTarget(int wAttackerID)
{
	int iBest = -1;
	float fBestDistance = 1000000.0;
	if (!NetServer_BoardSlotReady(wAttackerID)) { return -1; }
	ref rA = NetServer_GetClient(wAttackerID);

	for (int i=0; i<NET_MAXCLIENTS; i++)
	{
		if (!NetServer_IsBoardable(wAttackerID, i)) { continue; }

		ref rT = NetServer_GetClient(i);
		if (!NetServer_CheckBoardPair(rA, rT, false)) { continue; }

		float fDX = stf(rA.Ship.Pos.x) - stf(rT.Ship.Pos.x);
		float fDZ = stf(rA.Ship.Pos.z) - stf(rT.Ship.Pos.z);
		float fDistance = sqrt(fDX * fDX + fDZ * fDZ);
		if (fDistance < fBestDistance)
		{
			fBestDistance = fDistance;
			iBest = i;
		}
	}
	return iBest;
}

// called from the ship update tick: tell the client what it can board, only when that changes
void NetServer_UpdateBoardState(int wClientID)
{
	if (wClientID < 0 || wClientID >= NET_MAXCLIENTS) { return; }
	ref rClient = NetServer_GetClient(wClientID);

	int iTick = 0;
	if (CheckAttribute(rClient, "BoardTick")) { iTick = sti(rClient.BoardTick); }
	rClient.BoardTick = iTick + 1;
	if (iTick + 1 < NET_BOARD_STATE_TICKS) { return; }
	rClient.BoardTick = 0;

	int iTarget = NetServer_FindBoardTarget(wClientID);

	int iSent = -1;
	if (CheckAttribute(rClient, "BoardSent")) { iSent = sti(rClient.BoardSent); }
	if (iTarget == iSent) { return; }
	rClient.BoardSent = iTarget;

	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_SHIP);
	NMAddByte(iSMsg, NSC_SHIP_BOARD_STATE);
	if (iTarget >= 0)
	{
		NMAddByte(iSMsg, 1);
		NMAddClientID(iSMsg, iTarget);
	}
	else
	{
		NMAddByte(iSMsg, 0);
		NMAddClientID(iSMsg, wClientID);
	}
	NetServer_SendMessage(wClientID, iSMsg, true);
	NMDelete(iSMsg);
}

void NetServer_TryBoardPlayer(int wAttackerID, int wTargetID)
{
	ref rAttacker;
	ref rTarget;
	ref rWinner;
	ref rLoser;
	int wWinnerID;
	float fAttackCrew;
	float fDefendCrew;
	float fAttackPower;
	float fDefendPower;
	float fDeadCrew;
	float fWinnerCrew;
	float fNewCrew;
	float fKill;
	float fLoserCrew;
	bool bVolley;
	int iAttackerShot;
	int iTargetShot;
	int iKilledByAttacker;
	int iKilledByTarget;
	int iWinnerLost;
	int iLoserLost;

	if (!NetServer_IsBoardable(wAttackerID, wTargetID)) { return; }

	rAttacker = NetServer_GetClient(wAttackerID);
	rTarget = NetServer_GetClient(wTargetID);

	// distance / angle / speed / skills / perks, checked on the server (do not trust the client)
	if (!NetServer_CheckBoardPair(rAttacker, rTarget, true)) { return; }

	fAttackCrew = stf(rAttacker.Ship.Crew.Quantity);
	fDefendCrew = stf(rTarget.Ship.Crew.Quantity);

	// musket volley (LAi_boarding.c, perk MusketsShoot): before the fight a side with the perk shoots 25% of the
	// enemy crew (not more than its own crew), the enemy Defence skill lowers the losses. Attacker shoots first.
	bVolley = false;
	iAttackerShot = 0;
	iTargetShot = 0;
	iKilledByAttacker = 0;
	iKilledByTarget = 0;
	if (NetServer_HasPerk(rAttacker, "MusketsShoot"))
	{
		fKill = makeint(fDefendCrew * 0.25);
		if (fKill > fAttackCrew) { fKill = fAttackCrew; }
		fKill = makeint(fKill * (2.1 - NetServer_GetDefenceSkill(rTarget)) / 2.0);
		fDefendCrew = fDefendCrew - fKill;
		iKilledByAttacker = makeint(fKill);
		iAttackerShot = 1;
		bVolley = true;
	}
	if (NetServer_HasPerk(rTarget, "MusketsShoot"))
	{
		fKill = makeint(fAttackCrew * 0.25);
		if (fKill > fDefendCrew) { fKill = fDefendCrew; }
		fKill = makeint(fKill * (2.1 - NetServer_GetDefenceSkill(rAttacker)) / 2.0);
		fAttackCrew = fAttackCrew - fKill;
		iKilledByTarget = makeint(fKill);
		iTargetShot = 1;
		bVolley = true;
	}
	if (fAttackCrew < 1.0) { fAttackCrew = 1.0; }
	if (fDefendCrew < 1.0) { fDefendCrew = 1.0; }
	rAttacker.Ship.Crew.Quantity = makeint(fAttackCrew);
	rTarget.Ship.Crew.Quantity = makeint(fDefendCrew);
	if (bVolley)
	{
		NetServer_SendBoardVolley(wAttackerID, iAttackerShot, iTargetShot, iKilledByAttacker, iKilledByTarget);
		NetServer_SendBoardVolley(wTargetID, iTargetShot, iAttackerShot, iKilledByTarget, iKilledByAttacker);
	}

	// crew strength like Ship_AutoAbordage (crew experience and morale do not exist in the net mod,
	// they are the same for both ships there, so they do not change who wins)
	fAttackPower = 0.1 + NetServer_GetGrappling(rAttacker) * fAttackCrew;
	fDefendPower = 0.1 + NetServer_GetGrappling(rTarget) * fDefendCrew;

	if (fAttackPower >= fDefendPower)
	{
		rWinner = rAttacker; wWinnerID = wAttackerID;
		rLoser = rTarget;
		fWinnerCrew = fAttackCrew;
		fLoserCrew = fDefendCrew;
		fDeadCrew = fAttackCrew * fDefendPower / (fAttackPower * NET_BOARD_LOSS_DIVIDER);
	}
	else
	{
		rWinner = rTarget; wWinnerID = wTargetID;
		rLoser = rAttacker;
		fWinnerCrew = fDefendCrew;
		fLoserCrew = fAttackCrew;
		fDeadCrew = fDefendCrew * fAttackPower / (fDefendPower * NET_BOARD_LOSS_DIVIDER);
	}

	// winner loses sailors (replicated to clients with the regular ship update)
	fNewCrew = fWinnerCrew - fDeadCrew;
	if (fNewCrew < 1.0) { fNewCrew = 1.0; }
	rWinner.Ship.Crew.Quantity = makeint(fNewCrew);

	// casualties of the boarding fight: the winner loses the sailors counted above, the defeated ship loses everybody
	iWinnerLost = makeint(fWinnerCrew) - makeint(fNewCrew);
	iLoserLost = makeint(fLoserCrew);
	NetServer_SendBoardResult(wWinnerID, iWinnerLost, iLoserLost);
	NetServer_SendBoardResult(sti(rLoser.ID), iLoserLost, iWinnerLost);

	// the defeated ship sinks with its whole crew (the clients show 0 crew on the sinking ship)
	rLoser.Ship.Crew.Quantity = 0;

	Trace("COOP: boarding attacker=" + wAttackerID + " target=" + wTargetID + " crew " + fAttackCrew + " vs " + fDefendCrew + " power " + fAttackPower + " vs " + fDefendPower + " winner=" + wWinnerID + " lost=" + fDeadCrew);

	NetServer_SetClientDead(rLoser, NETKILL_BY_BOARDING, wWinnerID);
}

void NetServer_Ship2ShipCollision()
{
	aref arOurClient = GetEventData();
	aref arEnemyClient = GetEventData();
	float fPower = GetEventData();
	float fSlide = GetEventData();
	int iTouchPoint = GetEventData();
	float x = GetEventData();
	float y = GetEventData();
	float z = GetEventData();

	ref rOurClient = NetServer_GetClient(sti(arOurClient.ID));
	ref rEnemyClient = NetServer_GetClient(sti(arEnemyClient.ID));

	if (fPower > 1.0) 
	{ 
		int iLastCollTime1 = sti(rOurClient.LastCollisionShip);
		int iLastCollTime2 = sti(rEnemyClient.LastCollisionShip);
		if ((iServerTime - iLastCollTime1 >= 800) && (iServerTime - iLastCollTime2 >= 800))
		{
			int iSMsg = NMCreate();
			NMAddByte(iSMsg, NC_SHIP);
			NMAddByte(iSMsg, NSC_SHIP_SHIP_COLLISION);
			NMAddFloat(iSMsg, x);
			NMAddFloat(iSMsg, y);
			NMAddFloat(iSMsg, z);
			NetServer_SendMessage(DST_ALL, iSMsg, false);
			NMDelete(iSMsg);

			rOurClient.LastCollisionShip = iServerTime;
			rEnemyClient.LastCollisionShip = iServerTime;
		}
	}

	float fHP = (1.0 - fSlide) * fPower * 3.0;
	
	if (NetServer_IsClientDead(rEnemyClient))
		{ NetServer_ShipApplyHullHitpoints(rOurClient, fHP, NETKILL_BY_TOUCH_DEAD, sti(rEnemyClient.ID));	}
	else
		{ NetServer_ShipApplyHullHitpoints(rOurClient, fHP, NETKILL_BY_TOUCH_ALIVE, sti(rEnemyClient.ID)); }
}

void NetServer_Ship2IslandCollision()
{
	aref arClient = GetEventData();
	float fPower = GetEventData();
	float fSlide = GetEventData();
	int iTouchPoint = GetEventData();
	float x = GetEventData();
	float y = GetEventData();
	float z = GetEventData();

	ref rOurClient = NetServer_GetClient(sti(arClient.ID));

	if (fPower > 1.0) 
	{
		int iLastCollTime = sti(rOurClient.LastCollisionIsland);
		if (iServerTime - iLastCollTime >= 800)
		{
			int iSMsg = NMCreate();
			NMAddByte(iSMsg, NC_SHIP);
			NMAddByte(iSMsg, NSC_SHIP_ISLAND_COLLISION);
			NMAddFloat(iSMsg, x);
			NMAddFloat(iSMsg, y);
			NMAddFloat(iSMsg, z);
			NetServer_SendMessage(DST_ALL, iSMsg, false);
			NMDelete(iSMsg);

			rOurClient.LastCollisionIsland = iServerTime;
		}
	}

	float fHP = (1.0 - fSlide) * fPower * 3.0;
	NetServer_ShipApplyHullHitpoints(rOurClient, fHP, NETKILL_BY_ISLAND, DST_INVALID);
}

float NetServer_OnShipMastDamage()
{
	int iDamageType = GetEventData();
	int iMastNum = GetEventData();
	float x = GetEventData();
	float y = GetEventData();
	float z = GetEventData();
	float fDamage = GetEventData();
	int wOurClientID = GetEventData();
	int wBallClientID = DST_INVALID;

	switch (iDamageType)
	{
		case NETSHIP_MAST_TOUCH_ISLAND:
			fDamage = fDamage + 0.25;
		break;
		case NETSHIP_MAST_TOUCH_SHIP:
			fDamage = fDamage + 0.2;
		break;
		case NETSHIP_MAST_TOUCH_BALL:
			wBallClientID = GetEventData();

			switch (NSBalls.CurrentBallType)
			{
				case "Balls":		fDamage = fDamage + 0.1; break;
				case "Grapes":		fDamage = fDamage + 0.05; break;
				case "Knippels":	fDamage = fDamage + 0.25; break;
				case "Bombs":		fDamage = fDamage + 0.15; break;
			}
			
			// statistic
			ref rBallClient = NetServer_GetCLient(wBallClientID);
			rBallClient.Stat.Hits = sti(rBallClient.Stat.Hits) + 1;
		break;
	}

	fDamage = Clampf(fDamage);

	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_BALL);
	NMAddByte(iSMsg, NSC_BALL_HIT_MAST);
	NMAddWord(iSMsg, iServerCurrentBallIndex);
	NMAddClientID(iSMsg, wOurClientID);
	NMAddClientID(iSMsg, wBallClientID);
	NMAddFloat(iSMsg, x);
	NMAddFloat(iSMsg, y);
	NMAddFloat(iSMsg, z);
	NMAddByte(iSMsg, iDamageType);
	NMAddByte(iSMsg, iMastNum);
	NetServer_SendMessage(DST_ALL, iSMsg, false);
	NMDelete(iSMsg);

	if (fDamage >= 1.0) 
	{ 
		iSMsg = NMCreate();
		NMAddByte(iSMsg, NC_SHIP);
		NMAddByte(iSMsg, NSC_SHIP_MAST_FALL);
		NMAddClientID(iSMsg, wOurClientID);
		NMAddClientID(iSMsg, wBallClientID);
		NMAddByte(iSMsg, iDamageType);
		NMAddByte(iSMsg, iMastNum);
		NetServer_SendMessage(DST_ALL, iSMsg, false);
		NMDelete(iSMsg);
	}

	return fDamage;
}

float NetServer_ShipGetTotalFireTime(ref rClient)
{
	return MakeFloat(5 + 3 * (10 - sti(rClient.Skills.Repair)));
}

void NetServer_StartRepair(int wNetClientID)
{
	ref	rClient = NetServer_GetCLient(wNetClientID);
	if (iServerTime - sti(rClient.Skills.Repair.Cooldown) < NET_REPAIR_COOLDOWN * 1000) { return; }

	rClient.Skills.Repair.Active = 30;
	rClient.Skills.Repair.Cooldown = iServerTime;
	
	Event("NetServer_OnShipRepair", "l", wNetClientID);
}

void NetServer_OnShipRepair()
{
	bool bNotEnoughPlanks = true;
	bool bNotEnoughSailcloth = true;

	int wOurClientID = GetEventData();
	ref	rOurClient = NetServer_GetCLient(wOurClientID);
	ref rBaseShip = Net_GetShipByIndex(sti(rOurClient.Ship.Type));
	int iActiveSeconds = sti(rOurClient.Skills.Repair.Active) - 1;
	int iSkillRepair = sti(rOurClient.Skills.Repair);

	// restore HP
	float fPercentHPRestore = Bring2Range(0.0, 0.5, 0.1, 1.0, iSkillRepair / NETSKILL_MAX) / 30.0;
	float fMaxHP = stf(rOurClient.Ship.MaxHP);
	float fCurHP = stf(rOurClient.Ship.HP);
	if (fCurHP < fMaxHP)
	{
		// check for enough planks
		int iPlanksNum = sti(rOurClient.Ship.Goods.Planks);
		if (iPlanksNum >= 20)
		{
			rOurClient.Ship.Goods.Planks = iPlanksNum - 20;

			fCurHP = fCurHP + fPercentHPRestore * fMaxHP;
			if (fCurHP > fMaxHP) { fCurHP = fMaxHP; }
			rOurClient.Ship.HP = fCurHP;
			bNotEnoughPlanks = false;
		}
	}

	// restore SP
	bNotEnoughSailcloth = NetServer_OnShipSailRepair(wOurClientID);

	rOurClient.Skills.Repair.Active = iActiveSeconds;
	if (iActiveSeconds > 0)
	{
		PostEvent("NetServer_OnShipRepair", 1000, "l", wOurClientID);
	}
}

void NetServer_OnShipHullHit()
{
	float	fHP;

	ref		rBall = Net_GetGoodByIndex(sti(NSBalls.CurrentBallType));
	int		iBallType = sti(rBall.Index);

	aref	rShipObject = GetEventData();

	int		wBallClientID = GetEventData();
	int		wOurClientID = GetEventData();

	ref	rOurClient = NetServer_GetCLient(wOurClientID);
	ref	rBallClient = NetServer_GetCLient(wBallClientID);	

	//rOurClient.Ship.LastBallCharacter = wBallClientID;

	int		bTeam = NetServer_IsTeam(wBallClientID, wOurClientID);

	float	x = GetEventData();
	float	y = GetEventData();
	float	z = GetEventData();

	int		iFirePlaceIndex = GetEventData();   
	float	fFirePlaceDistance = GetEventData();

	bool	bDead = NetServer_IsClientDead(rOurClient);

	bool	bSeriousBoom = false;
	bool	bInflame = false;

	// Cannon damage multiply
	ref rCannon = Net_GetCannonByIndex(sti(rBallClient.Ship.Cannons.Type));
	float fCannonDamageMultiply = stf(rCannon.DamageMultiply);

	//if (sti(rBallClient.Perks.CriticalShoot) && rand(19)==10) { bSeriousBoom = true; }		// +5%
	//if (sti(rBallClient.Perks.CannonProfessional) && rand(9)==4) { bSeriousBoom = true; }		// +10%

	switch (rBall.Name)
	{
		case "Balls":
			if (rand(160) == 80) { bSeriousBoom = true; }
			if (rand(25) == 12) { bInflame = true; }
		break;
		case "Grapes":
			bSeriousBoom = false;
		break;
		case "Knippels":
			bSeriousBoom = false;
		break;
		case "Bombs":
			if (rand(100) == 50) { bSeriousBoom = true; }
			if (rand(8) == 4) { bInflame = true; }
		break;
	}

	//if (sti(rOurClient.Perks.ShipDefenseProfessional)) { bSeriousBoom = false; }				// no seriouse boom

	if (!bDead)
	{
		float fCrewDamage = stf(rBall.DamageCrew) * fCannonDamageMultiply;// * NetServer_IsPerkUse(rBallClient.Perks.CrewDamageUp, 1.0, 1.15);
		//float fRigDamage = stf(rBall.DamageRig) * fCannonDamageMultiply * AIShip_isPerksUse(aBallClient.Perks.SailsDamageUp, 1.0, 1.15);

		if (bSeriousBoom)
		{ 
			fCrewDamage = fCrewDamage * 4.0;
			fHP = fCannonDamageMultiply * stf(rBall.DamageHull) * (16.0 + frnd() * 8.0);
			NetServer_ShipApplyHullHitpoints(rOurClient, fHP, NETKILL_BY_BALL, wBallClientID);
			NetServer_ShipApplyCrewHitpoints(rOurClient, fCrewDamage * 1.5, wBallClientID);
			if (!bTeam) { NetServer_AddClientExp(rBallClient, 150); }
		}
		else
		{ 
			fHP = fCannonDamageMultiply * stf(rBall.DamageHull);
			NetServer_ShipApplyHullHitpoints(rOurClient, fHP, NETKILL_BY_BALL, wBallClientID); 
			if (!bTeam) { NetServer_AddClientExp(rBallClient, 20); }
		}

		NetServer_ShipApplyCrewHitpoints(rOurClient, fCrewDamage, wBallClientID);
		//Ship_ApplyRigHitpoints(rOurClient, fRigDamage);
	}

	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_BALL);
	NMAddByte(iSMsg, NSC_BALL_HIT_HULL);
	NMAddWord(iSMsg, iServerCurrentBallIndex);
	NMAddClientID(iSMsg, wOurClientID);
	NMAddClientID(iSMsg, wBallClientID);
	NMAddByte(iSMsg, iBallType);
	NMAddFloat(iSMsg, x);
	NMAddFloat(iSMsg, y);
	NMAddFloat(iSMsg, z);
	NMAddByte(iSMsg, bSeriousBoom);

	if (bInflame == true && fFirePlaceDistance < 4.0 && iFirePlaceIndex >= 0)
	{
		int iRandStartTime = 100 + rand(1000);
		float fTotalFireTime = NetServer_ShipGetTotalFireTime(rOurClient); 

		NMAddByte(iSMsg, true);		// true - activate fire place
		NMAddWord(iSMsg, iFirePlaceIndex);
		NMAddDword(iSMsg, iRandStartTime + iServerTime);
		NMAddFloat(iSMsg, fTotalFireTime);

		PostEvent("NetServer_ShipFireDamage", iRandStartTime, "lllf", wOurClientID, wBallClientID, iFirePlaceIndex, fTotalFireTime);
	}
	else
	{
		NMAddByte(iSMsg, false);		// false - no activate fire place
	}

	NetServer_SendMessage(DST_ALL, iSMsg, false);
	NMDelete(iSMsg);

	float fTmpCannonDamage = fCannonDamageMultiply * stf(rBall.DamageHull);

	if (bSeriousBoom == true) 
	{ 
		fTmpCannonDamage = fTmpCannonDamage * 4.0;
	}	

	//SendMessage(&AISea, "laffff", AI_MESSAGE_CANNONS_BOOM_CHECK, rOurClient, fTmpCannonDamage, x, y, z);

	// statistics
	rBallClient.Stat.Hits = sti(rBallClient.Stat.Hits) + 1;
}

void NetServer_ShipFireDamage()
{
	int wOurClientID = GetEventData();
	int wBallClientID = GetEventData();

	int iFirePlaceIndex = GetEventData();
	float fTotalFireTime = GetEventData();

	ref rOurClient = NetServer_GetClient(wOurClientID);
	ref rShip = Net_GetShipByIndex(sti(rOurClient.Ship.Type));
	float fHP = (8 - stf(rShip.Class)) * (0.5 + frnd() * 0.5);
	int iTime = 1000 + rand(500);

	NetServer_ShipApplyHullHitpoints(rOurClient, fHP, NETKILL_BY_BALL, wBallClientID); 
	fTotalFireTime = fTotalFireTime - MakeFloat(iTime) / 1000.0;

	if (fTotalFireTime > 0.0)
	{
		PostEvent("NetServer_ShipFireDamage", iTime, "lllf", wOurClientID, wBallClientID, iFirePlaceIndex, fTotalFireTime);
	}
}

void NetServer_ShipApplyHullHitpoints(ref rOurClient, float fHP, int iKillStatus, int wBallClientID)
{
	ref rBallClient;
	float fCurHP;
	float fMinus = 0.0;
	float fPlus = 0.0;

	if (NetServer_IsClientDead(rOurClient)) { return; }

	if (fHP <= 0.0) { return; }

	if (wBallClientID != DST_INVALID)
	{
		rBallClient = NetServer_GetClient(wBallClientID);
	}

	float fDamageHP = fHP * (1.0 + fPlus - fMinus);
	fCurHP = stf(rOurClient.Ship.HP) - fDamageHP;
	if (fCurHP <= 0.0)
	{
		fCurHP = 0.0;
		NetServer_SetClientDead(rOurClient, iKillStatus, wBallClientID);
	}

	rOurClient.Ship.HP = fCurHP;

	// statistic
	if (wBallClientID != DST_INVALID)
	{
		rBallClient = NetServer_GetClient(wBallClientID);
		rBallClient.Stat.DamageInflicted = stf(rBallClient.Stat.DamageInflicted) + fDamageHP;
	}
}

void NetServer_SetClientDead(ref rOurClient, int iKillStatus, int wKillerClientID)
{
	ref rKillerClient;
	ref rBaseShip, rKillerBaseShip;

	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_SHIP);
	NMAddByte(iSMsg, NSC_SHIP_DEAD);
	NMAddClientID(iSMsg, sti(rOurClient.ID));
	NMAddByte(iSMsg, iKillStatus);
	NMAddClientID(iSMsg, wKillerClientID);

	bool bKillerClient = NetServer_IsClient(wKillerClientID);

	rBaseShip = Net_GetShipByIndex(sti(rOurClient.Ship.Type));

	if (bKillerClient)
	{
		rKillerClient = NetServer_GetClient(wKillerClientID);
		rKillerBaseShip = Net_GetShipByIndex(sti(rKillerClient.Ship.Type));

		// sunk ships count
		rKillerClient.Stat.Sunk = sti(rKillerClient.Stat.Sunk) + 1;
	}

	float x = stf(rOurClient.Ship.Pos.x);
	float y = stf(rOurClient.Ship.Pos.y);
	float z = stf(rOurClient.Ship.Pos.z);

	rOurClient.Ship.HP = 0.0;
	rOurClient.Killer.Status = iKillStatus;
	rOurClient.Killer.ClientID = wKillerClientID;

	rOurClient.Stat.Wreck = 1;

	if (bKillerClient) 
	{ 
		int iExp = 2 * MakeInt(stf(rOurClient.Ship.MaxHP) * stf(rKillerBaseShip.Class) / stf(rBaseShip.Class)); 
		NetServer_AddClientExp(rKillerClient, iExp);
	}

	bool bDetonate = false;
	if (rand(10) >= 6 && sti(rBaseShip.Class) <= 6) 
	{ 
		switch (iKillStatus)
		{
			case NETKILL_BY_BALL: if (rand(2) == 1) { bDetonate = true; } break;
			case NETKILL_BY_ISLAND: if (rand(20) == 10) { bDetonate = true; } break;
			case NETKILL_BY_TOUCH_DEAD: if (rand(20) == 10) { bDetonate = true; } break;
			case NETKILL_BY_TOUCH_ALIVE: if (rand(20) == 10) { bDetonate = true; } break;
		}
	}

	NMAddByte(iSMsg, bDetonate); // detonate ship

	// set attributes for sinking effect
	rOurClient.Ship.Sink.Speed.y = 0.35;							// speed of sink y
	rOurClient.Ship.Sink.Speed.x = 0.017 * (frnd() * 2.0 - 1.0);	// speed sink angle rotate around x
	rOurClient.Ship.Sink.Speed.z = 0.017 * (frnd() * 2.0 - 1.0);	// speed sink angle rotate around z

	NMAddFloat(iSMsg, stf(rOurClient.Ship.Sink.Speed.x));
	NMAddFloat(iSMsg, stf(rOurClient.Ship.Sink.Speed.y));
	NMAddFloat(iSMsg, stf(rOurClient.Ship.Sink.Speed.z));

	NetServer_SendMessage(DST_ALL, iSMsg, true);
	NMDelete(iSMsg);

	rOurClient.Dead = true;

	// Event
	//Event(SHIP_DEAD, "l", iDeadCharacterIndex);
}

void NetServer_ShipApplyCrewHitpoints(ref rOurClient, float fCrewHP, int wBallClientID)
{
	ref rBaseShip = Net_GetShipByIndex(sti(rOurClient.Ship.Type));
	float fMultiply = Bring2Range(1.0, 0.2, 0.0, 1.0, stf(rOurClient.Skills.Defence) / NETSKILL_MAX);
	float fNewCrewQuantity = stf(rOurClient.Ship.Crew.Quantity) - fCrewHP * fMultiply;
	float f5Percent = stf(rBaseShip.MinCrew) * 0.05;
	if (fNewCrewQuantity < f5Percent) { fNewCrewQuantity = f5Percent; }
	if (fNewCrewQuantity < 1.0) { fNewCrewQuantity = 1.0; }
	rOurClient.Ship.Crew.Quantity = fNewCrewQuantity;
}

void NetServer_ShipUpdateParameters()
{
	int wOurClientID = GetEventData();
	ref rClient = NetServer_GetClient(wOurClientID);
	float fGameSpeed = stf(NetServer.GameSpeed);

	ref rBall = Net_GetGoodByIndex(sti(rClient.Ship.Cannons.Charge.Type));
	string sBallName = rBall.name;

	// some of ship parameters
		ref rShip = Net_GetShipByIndex(sti(rClient.Ship.Type));

		float fMaxSpeedZ = stf(rShip.SpeedRate) * stf(rClient.Ship.SpeedRateModifier);
		float fMaxSpeedY = stf(rShip.TurnRate) * stf(rClient.Ship.TurnRateModifier) / 444.444;		// cool magic number :))
		float fSpeedFromHP = 0.7 + (stf(rClient.Ship.HP) / stf(rClient.Ship.MaxHP)) / 3.34;
		float fCurrentSpeedZ = stf(rClient.Ship.Speed.z);
		float fLoad = 0.1;

	// Calc min crew ratio
		float fCrewMin = stf(rShip.MinCrew);
		float fCrewMax = stf(rShip.MaxCrew);
		float fCrewCur = stf(rClient.Ship.Crew.Quantity);
		float fCrewMin25Percent = stf(rShip.MinCrew);
		float fMinRatio = Bring2Range(0.0, 1.0, fCrewMin25Percent, fCrewMax, fCrewCur);

	// wind
		float fWindPower = NetServer_WhrGetWindSpeed() / WIND_NORMAL_POWER;
		float fWindDotShip = GetDotProduct(NetServer_WhrGetWindAngle(), stf(rClient.Ship.Ang.y));		// Wind.ay | Ship.ay

	// calculate MaxSpeedZ
		float fTRFromSkill = Bring2Range(0.1, 1.0, 0.0, 2.0, 2.0 * stf(rClient.Skills.Sailing) / NETSKILL_MAX);
		fTRFromSKill = 0.7 + fTRFromSkill / 3.34;
		//float fSpeedPerk = NetServer_IsPerkUse(rClient.Perks.ShipSpeedUp, 1.0, 1.15); 
		//fSpeedPerk = NetServer_IsPerkUse(rClient.Perks.SailingProfessional, fSpeedPerk, 1.20); 

		float	fTRFromWeight = Clampf(1.0 - stf(rShip.TurnDependWeight) * fLoad);
		float	fTRFromSpeed = 1.0 - stf(rShip.TurnDependSpeed) * (1.0 - Clampf(fCurrentSpeedZ / fMaxSpeedZ));
		float	fTRFromPeople = Bring2Range(0.1, 1.0, 0.0, 1.0, fMinRatio);
		float	fTRFromSailDamage = Bring2Range(0.1, 1.0, 0.1, 100.0, stf(rClient.Ship.SP));

		rClient.Ship.MaxSpeedZ = fGameSpeed * fMaxSpeedZ * fWindPower * fTRFromWeight * fTRFromSailDamage * fTRFromSkill * fSpeedFromHp;// * fSpeedPerk;

		/*if (fWindDotShip < -0.70)
		{
			rClient.Ship.MaxSpeedZ = stf(rClient.Ship.MaxSpeedZ) * (fWindDotShip + 1.0);
		}*/

	// Turn rate depend from sail State
		float	fTRFromSailState = 1.0;
		float	fTRResult;
		switch (MakeInt(stf(rClient.Ship.SailState) * 2.0))
		{
			case 0: fTRFromSailState = 0.1; break;
			case 1: fTRFromSailState = 1.0; break;
			case 2: fTRFromSailState = 0.5; break;
		}
		fTRResult = Bring2Range(0.1, 1.0, 0.00001, 1.0, fTRFromWeight * fTRFromSpeed * fTRFromSkill * fTRFromPeople * fTRFromSailDamage * fTRFromSailState * fSpeedFromHp);

		//float fTurnPerk = NetServer_IsPerkUse(rClient.Perks.ShipTurnRateUp, 1.0, 1.15); 
		//fTurnPerk = NetServer_IsPerkUse(rClient.Perks.SailingProfessional, fTurnPerk, 1.20); 
		//float fFastTurn180 = NetServer_IsPerkUse(rClient.Perks.Turn180, 1.0, 8.0); 
		rClient.Ship.MaxSpeedY = fGameSpeed * 2.0 * fTRResult * fMaxSpeedY;// * fTurnPerk * fFastTurn180;

	//rClient.Ship.MaxSpeedZ = 30.0;
	//rClient.Ship.MaxSpeedY = 2.6;

	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_SHIP);
	NMAddByte(iSMsg, NSC_SHIP_INFO);
	NMAddClientID(iSMsg, wOurClientID);

	NMAddDword(iSMsg, iServerTime);

	NMAddByte(iSMsg, makeint(2.0 * stf(rClient.Ship.SailState)));

	// COOP debug: log ship position about every 3 s per client (find where NaN appears)
	rClient.DbgN = sti(rClient.DbgN) + 1;
	if (sti(rClient.DbgN) - makeint(sti(rClient.DbgN) / 25) * 25 == 1)
	{
		Trace("COOP: pos client=" + wOurClientID + " x=" + rClient.Ship.Pos.x + " z=" + rClient.Ship.Pos.z + " ay=" + rClient.Ship.Ang.y + " spdz=" + rClient.Ship.Speed.z + " y=" + rClient.Ship.Pos.y + " maxz=" + rClient.Ship.MaxSpeedZ + " wind=" + NetServer_WhrGetWindSpeed() + " sail=" + rClient.Ship.SailState);
	}
	NMAddFloat(iSMsg, stf(rClient.Ship.Pos.x));
	NMAddFloat(iSMsg, stf(rClient.Ship.Ang.y));
	NMAddFloat(iSMsg, stf(rClient.Ship.Pos.z));

	int wSpeedX = makeint(stf(rClient.Ship.Speed.x) * 255.0);
	int wSpeedY = makeint(stf(rClient.Ship.Speed.y) * (65535.0));
	int wSpeedZ = makeint(stf(rClient.Ship.Speed.z) * 255.0);

	//NMAddWord(iSMsg, wSpeedX);	// side speed
	//NMAddWord(iSMsg, wSpeedY);	// rotate speed y
	//NMAddWord(iSMsg, wSpeedZ);	// forward speed

	NMAddFloat(iSMsg, stf(rClient.Ship.Speed.x));	// side speed
	NMAddFloat(iSMsg, stf(rClient.Ship.Speed.y));	// rotate speed y
	NMAddFloat(iSMsg, stf(rClient.Ship.Speed.z));	// forward speed

	NMAddWord(iSMsg, sti(rClient.Ship.HP));		// HP

	NMAddWord(iSMsg, sti(rClient.Ship.Crew.Quantity));			// Crew quantity
	NMAddWord(iSMsg, sti(rClient.Ship.Goods.(sBallName)));		// current charge quantity 
	
	NMAddWord(iSMsg, sti(rClient.Stat.Hits));
	NMAddWord(iSMsg, sti(rClient.Stat.Misses));
	NMAddFloat(iSMsg, stf(rClient.Stat.DamageInflicted));

	// repair info send
	NMAddByte(iSMsg, sti(rClient.Skills.Repair.Active));	// if repair is active
	//if (sti(rClient.Skills.Repair.Active))
	//{
		int iCurrentCooldownTime = i_min(NET_REPAIR_COOLDOWN, (iServerTime - sti(rClient.Skills.Repair.Cooldown)) / 1000);

		NMAddWord(iSMsg, NET_REPAIR_COOLDOWN);		// total cooldown
		NMAddWord(iSMsg, iCurrentCooldownTime);		// current cooldown
	//}

	// COOP: sail state in percent (the client does not simulate sail damage), used by the bars over the ships
	int iSailPercent = 100;
	if (CheckAttribute(rClient, "Ship.SP")) { iSailPercent = makeint(stf(rClient.Ship.SP)); }
	if (iSailPercent < 0) { iSailPercent = 0; }
	if (iSailPercent > 100) { iSailPercent = 100; }
	NMAddByte(iSMsg, iSailPercent);

	NetServer_SendMessage(DST_ALL, iSMsg, false);
	NMDelete(iSMsg);

	// ================
	// send borts info
	// ================
	iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_SHIP);
	NMAddByte(iSMsg, NSC_SHIP_BORTS_INFO);
	for (int i=0; i<4; i++)
	{
		aref arBorts; makearef(arBorts, rClient.Ship.Cannons.Borts);

		string sCurBort = sNetBortNames[i];
		NMAddWord(iSMsg, makeint(stf(arBorts.(sCurBort).MaxFireDistance))); 
		NMAddByte(iSMsg, makeint(stf(arBorts.(sCurBort).ChargeRatio) * 255.0)); 
		NMAddByte(iSMsg, makeint(stf(arBorts.(sCurBort).DamageRatio) * 255.0)); 
	}
	NetServer_SendMessage(wOurClientID, iSMsg, false);
	NMDelete(iSMsg);

	// auto boarding availability (checked every NET_BOARD_STATE_TICKS updates, sent to the client only on change)
	NetServer_UpdateBoardState(wOurClientID);

	PostEvent("NetServer_ShipUpdateParameters", 120, "l", wOurClientID);
}

void NetServer_CalculateReloadTime(ref rClient)
{
	ref rShip = Net_GetShipByIndex(sti(rClient.Ship.Type));
	ref rCannon = Net_GetCannonByIndex(sti(rClient.Ship.Cannons.Type));

	float fCannonSkill = stf(rClient.Skills.Cannons) / NETSKILL_MAX;

	float fMultiply = (6.0 - NetServer_GetSpeedModifier()) / 5.0; // NetServer_IsPerkUse(rClient.Perks.FastReload, 1.0, 0.8); 
	//fMultiply = NetServer_IsPerkUse(rClient.Perks.ImmediateReload, fMultiply, 0.5); 
	//if (!bArcadeCannonsReload) { fMultiply = fMultiply * 12.0; }

	float fCrewQ = sti(rClient.Ship.Crew.Quantity);
	float fCrewMax = sti(rShip.MaxCrew);

	float fReloadTime = sti(rCannon.ReloadTime) * (1 + (1 - Clampf(fCrewQ / fCrewMax)));
	fReloadTime = fMultiply * (fReloadTime - fCannonSkill * (fReloadTime / 2.0));

	rClient.Ship.Cannons.ReloadTime.Min = fReloadTime;
	rClient.Ship.Cannons.ReloadTime.Max = fReloadTime;
}

void NetServer_CalculateFireTime(ref rClient)
{
	ref rShip = Net_GetShipByIndex(sti(rClient.Ship.Type));

	// make 10 seconds random delay between fire from fort cannons
	if (sti(rClient.ID) == NET_FORTCLIENT) 
	{ 
		rClient.Ship.Cannons.FireTime = frnd() * 12.0;
		return; 
	}

	float fMultiply = 6.0 - NetServer_GetSpeedModifier();

	float fCrewQ = sti(rClient.Ship.Crew.Quantity);
	float fCrewMax = sti(rShip.MaxCrew);

	float fCannonSkill = stf(rClient.Skills.Cannons) / NETSKILL_MAX;
	float fFireTime = fMultiply * (1.5 - fCannonSkill) * (1 + (1 - Clampf(fCrewQ / fCrewMax)));

	rClient.Ship.Cannons.FireTime = fFireTime;
}

void NetServer_ShipChangeCharge(int wNetClientID, string sNewCharge)
{
	ref rClient = NetServer_GetClient(wNetClientID);
	ref rGood = Net_GetGoodByName(sNewCharge);
	ref rShip = Net_GetShipByIndex(sti(rClient.Ship.Type));

	rClient.CannonsReload = 1;	// unload, and start reload on next frame

	rClient.Ship.Cannons.Charge.Type = sti(rGood.index);
	NetServer_CannonRecalculateParameters(rClient);

	// 
	int iSMSg = NMCreate();
	NMAddByte(iSMsg, NC_SHIP);
	NMAddByte(iSMsg, NSC_SHIP_CHANGE_CHARGE);
	NMAddClientID(iSMsg, wNetClientID);
	NMAddByte(iSMsg, sti(rGood.index));
	NetServer_SendMessage(DST_ALL, iSMsg, false);
	NMDelete(iSMsg);
}

void NetServer_CannonRecalculateParameters(ref rClient)
{
	ref	rCannon = Net_GetCannonByIndex(sti(rClient.Ship.Cannons.Type));
	ref	rBall = Net_GetGoodByIndex(sti(rClient.Ship.Cannons.Charge.Type));

	rClient.Ship.Cannons.SpeedV0 = stf(rCannon.SpeedV0) * stf(rBall.SpeedV0);// * NetServer_IsPerkUse(rClient.Perks.LongRangeShoot, 1.0, 1.15);
}
