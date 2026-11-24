// COOP: NaN guard for client camera data
bool COOP_SrvFloatOk(float f)
{
	return (f > -100000.0) && (f < 100000.0);
}

#define NET_REPAIR_COOLDOWN			60

// COOP s27: the sunk ships are deleted (the safe way of "Delete" of the admin menu, client\OnClient.c NetClient_ShipEntityDelete)
#define NET_WRECK_GONE_TIME			180		// s after the sinking (no respawn) the ship is deleted on the clients
#define NET_WRECK_SINK				1		// 1 = a wrecked ship (respawn mode) sinks and is created again at the respawn; 0 = it stays afloat (old)
#define NET_REPAIR_MASTS			1		// 1 = the repair brings back the fallen masts (the ship is created again on the server and the clients)

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
	DelEventHandler("NetServer_MinesTick", "NetServer_MinesTick");
	DelEventHandler("NetServer_BranderBoom", "NetServer_BranderBoom");
	DelEventHandler("NetServer_RespawnEvent", "NetServer_RespawnEvent");
	DelEventHandler("NetServer_WreckGoneEvent", "NetServer_WreckGoneEvent");
	DelEventHandler("NetServer_ShipRecreateEvent", "NetServer_ShipRecreateEvent");
	DelEventHandler("NetServer_PerkTick", "NetServer_PerkTick");
	NetServer_BotsStop();		// COOP: bots (server\Bots.c)

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
	SetEventHandler("NetServer_MinesTick", "NetServer_MinesTick", 0);
	SetEventHandler("NetServer_BranderBoom", "NetServer_BranderBoom", 0);
	SetEventHandler("NetServer_RespawnEvent", "NetServer_RespawnEvent", 0);
	SetEventHandler("NetServer_WreckGoneEvent", "NetServer_WreckGoneEvent", 0);
	SetEventHandler("NetServer_ShipRecreateEvent", "NetServer_ShipRecreateEvent", 0);
	SetEventHandler("NetServer_PerkTick", "NetServer_PerkTick", 0);
	NetServer_BotsStart();		// COOP: bots (server\Bots.c)
	NetServer_MinesClear();
	PostEvent("NetServer_MinesTick", 500);

	// collision
	SetEventHandler("NetServer_Ship2ShipCollision", "NetServer_Ship2ShipCollision", 0);
	SetEventHandler("NetServer_Ship2IslandCollision", "NetServer_Ship2IslandCollision", 0);
}

void NetServer_OnShip(int wNetClientID, int iMsg)
{
	int iSubCode = NMGetByte(iMsg);

	ref rClient = NetServer_GetClient(wNetClientID);

	// COOP: a ship that is not in the game (lobby player), or a wrecked / sinking ship does not obey the captain any more
	if (wNetClientID < 0 || wNetClientID >= NET_MAXCLIENTS) { return; }
	if (!NetServer_HasShip(wNetClientID)) { return; }
	if (NetServer_IsClientDead(rClient) && iSubCode != NSC_SHIP_LOAD_REQUEST) { return; }
	// COOP s37: the captain fights on the deck (net\Board.c): the ship stands with the sails down, no shot, no order
	// COOP s41: the same while he chooses what to do with his prize (or his ship is the prize of another one)
	if (NetServer_ShipHeld(wNetClientID) && iSubCode != NSC_SHIP_LOAD_REQUEST && iSubCode != NSC_SHIP_PRIZE_DONE)
	{
		rClient.Ship.SailState = 0.0;
		rClient.Ship.TurnLeft = 0;
		rClient.Ship.TurnRight = 0;
		rClient.Ship.TurnState = 0.0;
		return;
	}

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
				// COOP: with bots in the game the engine must know which bots are enemies (server\Bots.c)
				NetServer_FireOutside(wNetClientID);		// COOP: lead aim (server\Bots.c)
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
		case NSC_SHIP_DROP_MINE:
			NetServer_ShipDropMine(wNetClientID);
		break;
		case NSC_SHIP_BRANDER:
			NetServer_ShipBrander(wNetClientID);
		break;
		case NSC_SHIP_BRANDER_ARM:
			NetServer_ShipBranderArm(wNetClientID);
		break;
		case NSC_SHIP_JETTISON:
			NetServer_ShipJettison(wNetClientID, iMsg);
		break;
		case NSC_SHIP_LOAD_REQUEST:
			NetServer_ShipSendLoadInfo(wNetClientID);
		break;
		case NSC_SHIP_CANNONS_SET:
			NetServer_ShipCannonsSet(wNetClientID, iMsg);
		break;
		case NSC_SHIP_PERK_USE:
			NetServer_UseActivePerk(wNetClientID, NMGetByte(iMsg));
		break;
		case NSC_SHIP_PRIZE_DONE:
			NetServer_PrizeDone(wNetClientID, iMsg);		// COOP s41: net\Board.c
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
	// COOP: a player, or a bot in the battle (server\Bots.c)
	bool bSlotUsed = false;
	if (CheckAttribute(rC, "Use")) { if (sti(rC.Use)) { bSlotUsed = true; } }
	if (NetServer_BotAlive(wClientID)) { bSlotUsed = true; }
	if (!bSlotUsed) { return false; }
	if (!NetServer_HasShip(wClientID)) { return false; }		// COOP: a lobby player in the running game has no ship
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
	if (NetServer_ShipHeld(wAttackerID) || NetServer_ShipHeld(wTargetID)) { return false; }		// COOP s37: on the deck now (s41: or a prize)
	if (stf(rA.Ship.HP) <= 0.0) { return false; }
	if (stf(rT.Ship.HP) <= 0.0) { return false; }

	// COOP: bots board only whom they fight; a player does not board an allied bot
	if (NetServer_IsBot(wAttackerID))
	{
		if (!NetServer_ShouldTarget(wAttackerID, wTargetID)) { return false; }
	}
	else
	{
		if (NetServer_IsBot(wTargetID))
		{
			if (NetServer_BotRel(wTargetID) == NET_BOTREL_ALLY) { return false; }
		}
	}

	// teammates cannot board each other (everybody is an enemy in deathmatch)
	if (sti(NetServer.GameType) != NETGAME_DEATHMATCH && sti(rA.Team) == sti(rT.Team)) { return false; }

	return true;
}

// COOP s43: the relative speed of a boarding. The 3..5 of the single player never came true in the net game: its ships go
// 40..60 (Ship.MaxSpeedZ); a ship could be boarded only when it nearly stood. Now 25..40 % of the top speed of the
// attacker (the grappling skill), never less than the old 3..5.
float NetServer_BoardMaxRelSpeed(ref rA, float fGrappling)
{
	float fMax = Bring2Range(3.0, 5.0, 0.0, 1.0, fGrappling);
	if (CheckAttribute(rA, "Ship.MaxSpeedZ"))
	{
		float fTop = stf(rA.Ship.MaxSpeedZ) * Bring2Range(0.25, 0.40, 0.0, 1.0, fGrappling);
		if (fTop > fMax) { fMax = fTop; }
	}
	return fMax;
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

	float fMaxDistance = NET_BOARD_DIST_MULT * NetServer_CalcGrapplingPerk(rA) * Bring2Range(15.0, 40.0, 0.0, 1.0, fGrappling);		// COOP s45: x NET_BOARD_DIST_MULT
	float fMaxAngle = Bring2Range(0.70, 0.45, 0.0, 1.0, fGrappling);
	float fMaxSpeed = NetServer_BoardMaxRelSpeed(rA, fGrappling);

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
	if (NetServer_IsBot(wClientID)) { return; }		// COOP: a bot has no player
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
	if (NetServer_IsBot(wClientID)) { return; }		// COOP: a bot has no player
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

// COOP s43: diagnostics: a player near an enemy ship (hulls closer than 60 m) that can not be boarded: why (every 3 s in the log)
void NetServer_BoardWhyNot(int wA)
{
	if (!NetServer_BoardSlotReady(wA)) { return; }
	ref rA = NetServer_GetClient(wA);
	int iT = 0;
	if (CheckAttribute(rA, "BoardWhyT")) { iT = sti(rA.BoardWhyT); }
	iT = iT + 1;
	rA.BoardWhyT = iT;
	if (iT < 6) { return; }		// 6 x 4 ship updates of 120 ms: ~3 s
	for (int i=0; i<NET_MAXCLIENTS; i++)
	{
		if (i == wA) { continue; }
		if (!NetServer_BoardSlotReady(i)) { continue; }
		ref rT = NetServer_GetClient(i);
		float fDX = stf(rA.Ship.Pos.x) - stf(rT.Ship.Pos.x);
		float fDZ = stf(rA.Ship.Pos.z) - stf(rT.Ship.Pos.z);
		float fDist = sqrt(fDX * fDX + fDZ * fDZ);
		float fHull = fDist - NetServer_GetShipHalfWidth(rA) - NetServer_GetShipHalfWidth(rT);
		if (fHull > 60.0) { continue; }
		rA.BoardWhyT = 0;
		bool bAble = NetServer_IsBoardable(wA, i);
		float fGr = NetServer_GetGrappling(rA);
		float fMaxD = NET_BOARD_DIST_MULT * NetServer_CalcGrapplingPerk(rA) * Bring2Range(15.0, 40.0, 0.0, 1.0, fGr);
		float fMaxA = Bring2Range(0.70, 0.45, 0.0, 1.0, fGr);
		float fMaxS = NetServer_BoardMaxRelSpeed(rA, fGr);
		float fDot = GetDotProduct(stf(rA.Ship.Ang.y), stf(rT.Ship.Ang.y));
		if (fDot < 0.0) { fDot = -fDot; }
		float fSA = stf(rA.Ship.Speed.z);
		float fST = stf(rT.Ship.Speed.z);
		float fRel2 = fSA * fSA + fST * fST - 2.0 * fSA * fST * GetDotProduct(stf(rA.Ship.Ang.y), stf(rT.Ship.Ang.y));
		if (fRel2 < 0.0) { fRel2 = 0.0; }
		float fRel = sqrt(fRel2);
		int iHeld = 0;
		if (NetServer_ShipHeld(wA) || NetServer_ShipHeld(i)) { iHeld = 1; }
		Trace("COOP: board why not " + wA + " -> " + i + ": boardable " + bAble + " held " + iHeld + " hull gap " + fHull + " (max " + fMaxD + ") parallel " + fDot + " (need > " + fMaxA + ") rel speed " + fRel + " (max " + fMaxS + ") half widths " + NetServer_GetShipHalfWidth(rA) + " / " + NetServer_GetShipHalfWidth(rT));
	}
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

	int iTarget = -1;
	if (!sti(NetServer.NoBoarding)) { iTarget = NetServer_FindBoardTarget(wClientID); }		// COOP: boarding can be switched off
	if (iTarget < 0 && wClientID < NET_MAXPLAYERS && !sti(NetServer.NoBoarding)) { NetServer_BoardWhyNot(wClientID); }		// COOP s43

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
	int iGodSide;
	bool bAttWins;

	if (sti(NetServer.NoBoarding)) { return; }		// COOP: boarding is switched off on the Create game screen
	if (!NetServer_IsBoardable(wAttackerID, wTargetID)) { return; }

	rAttacker = NetServer_GetClient(wAttackerID);
	rTarget = NetServer_GetClient(wTargetID);

	// distance / angle / speed / skills / perks, checked on the server (do not trust the client)
	if (!NetServer_CheckBoardPair(rAttacker, rTarget, true)) { return; }

	// the target already pressed the brander once (armed) or it is about to explode: no boarding, the brander goes off
	if (NetServer_BranderIsActive(rTarget)) { return; }
	if (NetServer_BranderIsArmed(rTarget))
	{
		Trace("COOP: boarding attacker=" + wAttackerID + " target=" + wTargetID + " -> target brander is armed, it goes off");
		NetServer_ShipBrander(wTargetID);
		if (NetServer_BranderIsActive(rTarget)) { return; }
		// the brander was refused (not enough powder): the boarding goes on as usual
	}

	fAttackCrew = stf(rAttacker.Ship.Crew.Quantity);
	fDefendCrew = stf(rTarget.Ship.Crew.Quantity);

	// COOP s27: immortality of the admin menu: the immortal side (only one of the two) loses nobody, by the muskets too, and
	// always wins: the other ship loses its whole crew. 0 = none / both, 1 = the attacker is immortal, 2 = the target
	iGodSide = 0;
	if (NetServer_IsGod(wAttackerID) == 1 && NetServer_IsGod(wTargetID) == 0) { iGodSide = 1; }
	if (NetServer_IsGod(wTargetID) == 1 && NetServer_IsGod(wAttackerID) == 0) { iGodSide = 2; }

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
		if (iGodSide == 2) { fKill = 0.0; }		// COOP s27: the immortal target loses nobody
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
		if (iGodSide == 1) { fKill = 0.0; }		// COOP s27: the immortal attacker loses nobody
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

	// COOP s37: the fast boarding is off (Create game): two players fight on the deck with their crews (net\Board.c);
	// COOP s41: a player and a bot too (the captain and the crew of the bot are land bots of the player); only two bots
	// board each other the fast way. The immortality of the admin wins at once (as below).
	if (NetServer_BoardRealOn() && iGodSide == 0)
	{
		if (!NetServer_IsBot(wAttackerID) || !NetServer_IsBot(wTargetID))
		{
			if (NetServer_BDStart(wAttackerID, wTargetID)) { return; }
		}
	}

	// crew strength like Ship_AutoAbordage (crew experience and morale do not exist in the net mod,
	// they are the same for both ships there, so they do not change who wins)
	fAttackPower = 0.1 + NetServer_GetGrappling(rAttacker) * fAttackCrew;
	fDefendPower = 0.1 + NetServer_GetGrappling(rTarget) * fDefendCrew;

	// small random spread (+1..5% for each side, rolled separately): with about equal forces the result is random,
	// with a clear advantage the stronger side still wins. The losses are counted from these powers, so they spread too.
	fAttackPower = fAttackPower * (1.0 + NET_BOARD_RAND_MIN + Random() * (NET_BOARD_RAND_MAX - NET_BOARD_RAND_MIN));
	fDefendPower = fDefendPower * (1.0 + NET_BOARD_RAND_MIN + Random() * (NET_BOARD_RAND_MAX - NET_BOARD_RAND_MIN));

	bAttWins = false;
	if (fAttackPower >= fDefendPower) { bAttWins = true; }
	if (iGodSide == 1) { bAttWins = true; }		// COOP s27: the immortal side wins
	if (iGodSide == 2) { bAttWins = false; }
	if (bAttWins)
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

	if (iGodSide != 0) { fDeadCrew = 0.0; }		// COOP s27: the immortal winner loses nobody

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

	Trace("COOP: boarding attacker=" + wAttackerID + " target=" + wTargetID + " crew " + fAttackCrew + " vs " + fDefendCrew + " power " + fAttackPower + " vs " + fDefendPower + " winner=" + wWinnerID + " lost=" + fDeadCrew + " god=" + iGodSide);

	NetServer_BoardWon(wWinnerID, sti(rLoser.ID));		// COOP s41: the prize of a player (net\Board.c), else it goes down
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

	// COOP: a hit with the BOW breaks the bowsprit (both ships are checked: the bow of each one). server\Fort.c
	NetServer_BowspritShipTouch(rOurClient, sti(arOurClient.ID), x, z, fPower);
	NetServer_BowspritShipTouch(rEnemyClient, sti(arEnemyClient.ID), x, z, fPower);

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

	NetServer_BotShoreAdd(x, z, 90.0);		// COOP: the bots learn the shore from every touch (server\Bots.c)
	if (NetServer_IsBot(sti(arClient.ID))) { NetServer_BotIslandHit(sti(arClient.ID), x, z); }		// COOP: away from the shore

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

	if (NetServer_IsBot(sti(arClient.ID))) { return; }		// COOP: a bot takes no damage from an island

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

	// COOP debug: which mast of which hull is hit (to find the bowsprit mast number of each hull)
	if (NetServer_HasShip(wOurClientID)) { ref rDbgS = NetServer_GetClient(wOurClientID); ref rDbgBase = Net_GetShipByIndex(sti(rDbgS.Ship.Type)); Trace("COOP mast hit: hull " + rDbgBase.Name + ", mast " + iMastNum + ", type " + iDamageType + ", damage " + fDamage); }

	switch (iDamageType)
	{
		case NETSHIP_MAST_TOUCH_ISLAND:
			if (NetServer_IsBot(wOurClientID)) { fDamage = 0.0; }		// COOP: a bot does not lose masts on an island
			else { fDamage = fDamage + 0.25; }
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
		if (NetServer_HasShip(wOurClientID)) { NetServer_ShipMastFallen(NetServer_GetClient(wOurClientID), iMastNum); }
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
	if (NET_REPAIR_MASTS) { NetServer_ShipRebuildMasts(wNetClientID); }		// COOP s27: the fallen masts come back
}

// COOP s27: a mast of the ship fell (the engine, the bowsprit): remembered for the repair
void NetServer_ShipMastFallen(ref rS, int iMastNum)
{
	string sM = "m" + iMastNum;
	rS.Ship.MastsFallen.(sM) = 1;
}

bool NetServer_ShipHasFallenMasts(ref rS)
{
	if (!CheckAttribute(rS, "Ship.MastsFallen")) { return false; }
	aref arF; makearef(arF, rS.Ship.MastsFallen);
	return GetAttributesNum(arF) > 0;
}

// COOP s27: the sails of the ship lose their damage (the attributes of server\Rigging.c: ship.sails.<rey>.<group>): bAll = all
// of them ("Heal"), else only the sails of the fallen masts (they come back with the mast). Ship.Sails.Color stays.
void NetServer_ShipSailsHeal(ref rC, bool bAll)
{
	if (!CheckAttribute(rC, "Ship.Sails")) { return; }
	aref arRoot, arGroup, arSail;
	makearef(arRoot, rC.Ship.Sails);
	int i, j;
	for (i=GetAttributesNum(arRoot)-1; i>=0; i--)
	{
		arGroup = GetAttributeN(arRoot, i);
		int n = GetAttributesNum(arGroup);
		if (n == 0) { continue; }		// Color
		for (j=n-1; j>=0; j--)
		{
			arSail = GetAttributeN(arGroup, j);
			if (bAll || CheckAttribute(arSail, "mastFall")) { DeleteAttribute(arGroup, GetAttributeName(arSail)); }
		}
		if (GetAttributesNum(arGroup) == 0) { DeleteAttribute(arRoot, GetAttributeName(arGroup)); }
	}
	rC.Ship.SP = Net_CalculateShipSP(rC);
}

// COOP s27: the ship has holes in its sails (a sail with damage)
bool NetServer_ShipSailsDamaged(ref rC)
{
	if (!CheckAttribute(rC, "Ship.Sails")) { return false; }
	aref arRoot, arGroup;
	makearef(arRoot, rC.Ship.Sails);
	for (int i=0; i<GetAttributesNum(arRoot); i++)
	{
		arGroup = GetAttributeN(arRoot, i);
		if (GetAttributesNum(arGroup) > 0) { return true; }
	}
	return false;
}

// COOP s27: the engine can not put a fallen mast back, so the ship is deleted and created again at its place: on the server
// (NetServer_ShipRecreateLater) and on the clients (NSC_SHIP_REBUILD -> NetClient_ShipEntityRecreate)
void NetServer_ShipRebuildMasts(int wID)
{
	NetServer_ShipRebuild(wID, false);
}

// COOP s27: "Heal" of the admin: the ship is created again also when its sails have holes (the new sails are whole), and all
// the sail damage goes
void NetServer_ShipRebuildHeal(int wID)
{
	NetServer_ShipRebuild(wID, true);
}

void NetServer_ShipRebuild(int wID, bool bHeal)
{
	if (!NetServer_HasShip(wID)) { return; }
	ref rC = NetServer_GetClient(wID);
	if (sti(rC.Dead)) { return; }
	bool bMasts = NetServer_ShipHasFallenMasts(rC);
	bool bSails = false;
	if (bHeal) { bSails = NetServer_ShipSailsDamaged(rC); }
	NetServer_ShipSailsHeal(rC, bHeal);
	if (!bMasts && !bSails) { return; }
	if (!IsEntity(rC)) { return; }		// created again already (the new ship comes in some frames)

	float x = stf(rC.Ship.Pos.x);
	float z = stf(rC.Ship.Pos.z);
	float ay = stf(rC.Ship.Ang.y);
	if (!COOP_SrvFloatOk(x) || !COOP_SrvFloatOk(z) || !COOP_SrvFloatOk(ay)) { return; }

	int iTrM = 0;
	int iTrS = 0;
	if (bMasts) { iTrM = 1; }
	if (bSails) { iTrS = 1; }
	DeleteAttribute(rC, "Ship.masts");
	DeleteAttribute(rC, "Ship.MastsFallen");
	NetServer_ShipRecreateLater(wID, x, z, ay);

	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_SHIP);
	NMAddByte(iSMsg, NSC_SHIP_REBUILD);
	NMAddClientID(iSMsg, wID);
	NMAddFloat(iSMsg, stf(rC.Ship.Pos.x));
	NMAddFloat(iSMsg, stf(rC.Ship.Pos.z));
	NMAddFloat(iSMsg, stf(rC.Ship.Ang.y));
	NetServer_SendToInGame(iSMsg);
	NMDelete(iSMsg);
	Trace("COOP: ship " + wID + " rebuilt (masts " + iTrM + ", sails " + iTrS + ") at x=" + rC.Ship.Pos.x + " z=" + rC.Ship.Pos.z);
}

// COOP s27: the server ship is created again at the SAME place (the reload of the admin, the repair of the masts, "Heal").
// The engine deletes an entity only at the end of the frame: the new ship created in the same frame stood in the old one, the
// ship hit itself (log "ship touch: ship N", the ship jumped) and the game froze at the second reload. The old ship is deleted
// now and the new one is created NET_RECREATE_DELAY ms later (NetServer_ShipRecreateEvent).
#define NET_RECREATE_DELAY		60
void NetServer_ShipRecreateLater(int wID, float fX, float fZ, float fAy)
{
	ref rC = NetServer_GetClient(wID);
	float fAng = fAy;
	while (fAng >= PIm2) { fAng = fAng - PIm2; }
	while (fAng < 0.0) { fAng = fAng + PIm2; }
	rC.Ship.Pos.x = fX;
	rC.Ship.Pos.y = 0.0;		// a new ship must never start with an old / NaN height
	rC.Ship.Pos.z = fZ;
	rC.Ship.Ang.y = fAng;
	rC.Ship.Speed.x = 0.0;
	rC.Ship.Speed.y = 0.0;
	rC.Ship.Speed.z = 0.0;
	rC.LastCollisionShip = -500;
	rC.LastCollisionIsland = -500;
	rC.CoopFix = 0;
	int iGen = 1;
	if (CheckAttribute(rC, "RecreateGen")) { iGen = sti(rC.RecreateGen) + 1; }
	rC.RecreateGen = iGen;
	DeleteClass(rC);
	PostEvent("NetServer_ShipRecreateEvent", NET_RECREATE_DELAY, "lll", wID, iGen, sti(NetServer.Round));
	Trace("COOP: server ship " + wID + " deleted, created again in " + NET_RECREATE_DELAY + " ms");
}

void NetServer_ShipRecreateEvent()
{
	int wID = GetEventData();
	int iGen = GetEventData();
	int iRound = GetEventData();
	if (!bServerGameStarted) { return; }
	if (iRound != sti(NetServer.Round)) { return; }
	if (wID < 0 || wID >= NET_MAXCLIENTS) { return; }
	ref rC = NetServer_GetClient(wID);
	if (!CheckAttribute(rC, "RecreateGen")) { return; }
	if (sti(rC.RecreateGen) != iGen) { return; }		// a newer recreation comes
	if (IsEntity(rC)) { return; }		// created already (the respawn)
	if (!NetServer_HasShip(wID)) { return; }		// the bot is deleted
	NetServer_CreateShipEntity(wID);
	NetServer_RestartShipWatch();
	Trace("COOP: server ship " + wID + " created again");
}

// COOP s27: the sunk ship is deleted on the clients some time after its death (it stays dead on the server: the respawn,
// the "Heal" of a bot and a reconnect create it again)
void NetServer_WreckGoneEvent()
{
	int wID = GetEventData();
	int iLife = GetEventData();
	int iRound = GetEventData();
	int iSec = GetEventData();

	if (!bServerGameStarted) { return; }
	if (iRound != sti(NetServer.Round)) { return; }
	if (wID < 0 || wID >= NET_MAXCLIENTS) { return; }
	if (iLife != NetServer_GetShipLife(wID)) { return; }
	ref rC = NetServer_GetClient(wID);
	if (!CheckAttribute(rC, "Dead") || !sti(rC.Dead)) { return; }
	if (CheckAttribute(rC, "ShipGone") && sti(rC.ShipGone)) { return; }
	rC.ShipGone = true;

	// COOP s27: a sunk bot that will not come back leaves its slot, the way of "Delete" of the admin menu (server\Bots.c)
	if (NetServer_IsBot(wID))
	{
		if (!NetServer_BotWillRespawn(wID) && !NetServer_BotRemoved(wID))
		{
			Trace("COOP: sunk bot " + wID + " leaves its slot (" + iSec + " s after the death)");
			NetServer_BotRemove(wID);
			return;
		}
	}

	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_SHIP);
	NMAddByte(iSMsg, NSC_SHIP_GONE);
	NMAddClientID(iSMsg, wID);
	NetServer_SendToInGame(iSMsg);
	NMDelete(iSMsg);
	Trace("COOP: sunk ship " + wID + " deleted on the clients (" + iSec + " s after the death)");
}

// COOP s27: the dead ship leaves the layer that the cannons of the engine trace (the aim from the third person found the sunk ship)
void NetServer_ShipNoCannonTarget(ref rS)
{
	if (!IsEntity(rS)) { return; }
	LayerDelObject("ship_cannon_trace", rS);
	Trace("COOP: dead ship " + rS.ID + " is no cannon target any more");
}

void NetServer_WreckGoneLater(int wID, int iSec)
{
	PostEvent("NetServer_WreckGoneEvent", iSec * 1000 + 500, "llll", wID, NetServer_GetShipLife(wID), sti(NetServer.Round), iSec);
	Trace("COOP: ship " + wID + " will be deleted in " + iSec + " s");
}

void NetServer_OnShipRepair()
{
	bool bNotEnoughPlanks = true;
	bool bNotEnoughSailcloth = true;

	int wOurClientID = GetEventData();
	ref	rOurClient = NetServer_GetCLient(wOurClientID);
	if (NetServer_IsClientDead(rOurClient)) { rOurClient.Skills.Repair.Active = 0; return; }		// COOP: wrecked / sunk
	ref rBaseShip = Net_GetShipByIndex(sti(rOurClient.Ship.Type));
	int iActiveSeconds = sti(rOurClient.Skills.Repair.Active) - 1;
	int iSkillRepair = sti(rOurClient.Skills.Repair);

	// restore HP
	float fPercentHPRestore = Bring2Range(0.0, 0.5, 0.1, 1.0, iSkillRepair / NETSKILL_MAX) / 30.0;
	int iPlanksPerTick = 20;
	if (NetServer_SeaPerk(wOurClientID, NET_SPERK_CARPENTER)) { fPercentHPRestore = fPercentHPRestore * 1.1; }		// COOP: passive perk Carpenter (repair skill x1.1)
	if (NetServer_SeaPerk(wOurClientID, NET_SPERK_BUILDER)) { iPlanksPerTick = 18; }		// COOP: passive perk Builder (materials x0.9)
	float fMaxHP = stf(rOurClient.Ship.MaxHP);
	float fCurHP = stf(rOurClient.Ship.HP);
	if (fCurHP < fMaxHP)
	{
		// check for enough planks
		int iPlanksNum = sti(rOurClient.Ship.Goods.Planks);
		if (iPlanksNum >= iPlanksPerTick)
		{
			rOurClient.Ship.Goods.Planks = iPlanksNum - iPlanksPerTick;

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
	float fCannonDamageMultiply = stf(rCannon.DamageMultiply);		// COOP: missing (thrown / not bought) cannons do not fire at all (server\OnBall.c)

	// COOP: passive perks CriticalShoot (+5%) and CannonProfessional (+10%) of the shooter (sea_ai\AIShip.c)
	// COOP s28: every perk is bought by itself (common\Perks.c)
	if (NetServer_SeaPerk(wBallClientID, NET_SPERK_CRITICAL) && rand(19) == 10) { bSeriousBoom = true; }
	if (NetServer_SeaPerk(wBallClientID, NET_SPERK_CANNONPRO) && rand(9) == 4) { bSeriousBoom = true; }

	// COOP s45: the chances of Ship_HullHitEvent of the single player (sea_ai\AIShip.c)
	switch (rBall.Name)
	{
		case "Balls":
			if (rand(85) == 17) { bSeriousBoom = true; }
			if (rand(30) == 15) { bInflame = true; }
		break;
		case "Grapes":
			bSeriousBoom = false;
		break;
		case "Knippels":
			bSeriousBoom = false;
		break;
		case "Bombs":
			if (rand(50) == 10) { bSeriousBoom = true; }
			if (rand(5) == 1) { bInflame = true; }
		break;
	}

	// COOP s45: the hull damage falls with the distance of the flight of the ball like in the single player (1.2 at the cannon,
	// 0.25 at the end of the range), when the engine gives the two distances of the ball
	float fDistanceDamageMultiply = 1.0;
	if (CheckAttribute(&NSBalls, "CurrentMaxBallDistance") && CheckAttribute(&NSBalls, "CurrentBallDistance"))
	{
		float fBallMaxD = stf(NSBalls.CurrentMaxBallDistance);
		if (fBallMaxD > 1.0) { fDistanceDamageMultiply = Bring2Range(1.2, 0.25, 0.0, fBallMaxD, stf(NSBalls.CurrentBallDistance)); }
	}

	// COOP: passive perk ShipDefenseProfessional of the target: 70% of the critical hits do not happen
	if (bSeriousBoom && NetServer_SeaPerk(wOurClientID, NET_SPERK_DEFENCE) && rand(1000) < 700) { bSeriousBoom = false; }

	if (!bDead)
	{
		float fCrewDamage = stf(rBall.DamageCrew) * fCannonDamageMultiply;
		if (NetServer_SeaPerk(wBallClientID, NET_SPERK_CREWDAMAGE)) { fCrewDamage = fCrewDamage * 1.15; }		// COOP: passive perk CrewDamageUp
		//float fRigDamage = stf(rBall.DamageRig) * fCannonDamageMultiply * AIShip_isPerksUse(aBallClient.Perks.SailsDamageUp, 1.0, 1.15);

		// COOP s45: a critical hit like in the single player: the crew x7 (it was x4 and x6 more, x10), the hull x8..12 (it was x16..24)
		if (bSeriousBoom)
		{
			fCrewDamage = fCrewDamage * 7.0;
			fHP = fDistanceDamageMultiply * fCannonDamageMultiply * stf(rBall.DamageHull) * (8.0 + frnd() * 4.0);
			NetServer_ShipApplyHullHitpoints(rOurClient, fHP, NETKILL_BY_BALL, wBallClientID);
			if (!bTeam) { NetServer_AddClientExp(rBallClient, 150); }
		}
		else
		{
			fHP = fDistanceDamageMultiply * fCannonDamageMultiply * stf(rBall.DamageHull);
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

		PostEvent("NetServer_ShipFireDamage", iRandStartTime, "llllf", wOurClientID, wBallClientID, iFirePlaceIndex, NetServer_GetShipLife(wOurClientID), fTotalFireTime);
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

		// COOP: single player (boal 27.09.05): a critical hit into a ship that carries too much powder blows the whole stock up
		if (!NetServer_IsClientDead(rOurClient) && NetServer_PowderOverloaded(rOurClient) && rand(99) < NET_POWDER_DETONATE_CHANCE)
		{
			NetServer_PowderExplosion(wOurClientID, wBallClientID);
		}
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
	int iFireLife = GetEventData();
	float fTotalFireTime = GetEventData();

	ref rOurClient = NetServer_GetClient(wOurClientID);
	// COOP: the fire of a ship that was wrecked and came back (respawn) / of a slot of another game burns out
	if (NetServer_IsClientDead(rOurClient)) { return; }
	if (iFireLife != NetServer_GetShipLife(wOurClientID)) { return; }
	ref rShip = Net_GetShipByIndex(sti(rOurClient.Ship.Type));
	float fHP = (8 - stf(rShip.Class)) * (0.5 + frnd() * 0.5);
	int iTime = 1000 + rand(500);

	NetServer_ShipApplyHullHitpoints(rOurClient, fHP, NETKILL_BY_BALL, wBallClientID); 
	fTotalFireTime = fTotalFireTime - MakeFloat(iTime) / 1000.0;

	if (fTotalFireTime > 0.0)
	{
		PostEvent("NetServer_ShipFireDamage", iTime, "llllf", wOurClientID, wBallClientID, iFirePlaceIndex, iFireLife, fTotalFireTime);
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
	if (NetServer_IsGod(sti(rOurClient.ID))) { return; }		// COOP s27: immortality of the admin menu

	// COOP: a respawned ship is safe for a moment
	if (CheckAttribute(rOurClient, "SpawnProtect") && iServerTime < sti(rOurClient.SpawnProtect)) { return; }

	NetServer_BotProvoked(sti(rOurClient.ID), wBallClientID);		// COOP s28: a neutral bot that is hit becomes an enemy

	if (wBallClientID != DST_INVALID)
	{
		rBallClient = NetServer_GetClient(wBallClientID);
	}

	// COOP: passive perks like Ship_ApplyHullHitpoints of single player: CannonProfessional of the attacker +30%,
	// ShipDefenseProfessional of the target -35% (they include HullDamageUp and Basic / AdvancedBattleState)
	if (wBallClientID != DST_INVALID && wBallClientID != sti(rOurClient.ID) && NetServer_SeaPerk(wBallClientID, NET_SPERK_CANNONPRO)) { fPlus = 0.3; }
	if (NetServer_SeaPerk(sti(rOurClient.ID), NET_SPERK_DEFENCE)) { fMinus = 0.35; }

	float fDamageHP = fHP * (1.0 + fPlus - fMinus);
	fCurHP = stf(rOurClient.Ship.HP) - fDamageHP;
	if (fCurHP <= 0.0)
	{
		fCurHP = 0.0;
		// COOP: with respawn the ship does not sink (it is wrecked and comes back), its hull keeps 1 point
		// COOP: bots sink, they have no respawn
		if (sti(NetServer.Respawn))
		{
			if (sti(rOurClient.ID) < NET_BOT_FIRST) { fCurHP = 1.0; rOurClient.Ship.HP = 1.0; }
		}
		// COOP: the fort has its own kill line
		if (iKillStatus == NETKILL_BY_BALL && wBallClientID == NET_FORTCLIENT) { iKillStatus = NETKILL_BY_FORT; }
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

	if (NetServer_IsGod(sti(rOurClient.ID))) { return; }		// COOP s27: immortality of the admin menu

	// COOP s47f: the captor goes down while he chooses: his prize goes down, his window closes
	if (CheckAttribute(rOurClient, "PrizeTake"))
	{
		Trace("COOP: prize: the captor " + rOurClient.ID + " goes down while he chooses");
		NetServer_PrizeCaptorGone(sti(rOurClient.ID));
	}

	// COOP: respawn mode: the ship is wrecked for a while and comes back (server\OnShip.c: NetServer_SetClientWrecked)
	// COOP: bots sink, they have no respawn
	bool bWreckOnly = false;
	if (sti(NetServer.Respawn))
	{
		if (sti(rOurClient.ID) < NET_BOT_FIRST) { bWreckOnly = true; }
	}
	if (NetServer_BotWillRespawn(sti(rOurClient.ID))) { bWreckOnly = true; }		// COOP: the bot respawn of the admin menu
	if (bWreckOnly)
	{
		NetServer_SetClientWrecked(rOurClient, iKillStatus, wKillerClientID);
		return;
	}

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
	rOurClient.Ship.Crew.Quantity = 0;		// COOP: the crew goes down with the ship
	rOurClient.Killer.Status = iKillStatus;
	rOurClient.Killer.ClientID = wKillerClientID;

	rOurClient.Stat.Wreck = 1;
	rOurClient.Stat.Deaths = sti(rOurClient.Stat.Deaths) + 1;
	rOurClient.Life = NetServer_GetShipLife(sti(rOurClient.ID)) + 1;	// COOP: pending fire / perk events of this ship stop

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
			case NETKILL_BY_FORT: if (rand(2) == 1) { bDetonate = true; } break;
			case NETKILL_BY_MINE: if (rand(2) == 1) { bDetonate = true; } break;
			case NETKILL_BY_OWN_MINE: if (rand(2) == 1) { bDetonate = true; } break;
			case NETKILL_BY_BRANDER: if (rand(2) == 1) { bDetonate = true; } break;
			case NETKILL_BY_ISLAND: if (rand(20) == 10) { bDetonate = true; } break;
			case NETKILL_BY_TOUCH_DEAD: if (rand(20) == 10) { bDetonate = true; } break;
			case NETKILL_BY_TOUCH_ALIVE: if (rand(20) == 10) { bDetonate = true; } break;
		}
	}

	if (CheckAttribute(rOurClient, "ForceDetonate")) { bDetonate = true; DeleteAttribute(rOurClient, "ForceDetonate"); }
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
	NetServer_WreckGoneLater(sti(rOurClient.ID), NET_WRECK_GONE_TIME);		// COOP s27
	NetServer_ShipNoCannonTarget(rOurClient);		// COOP s27: the cannons do not aim at a sunk ship

	// Event
	//Event(SHIP_DEAD, "l", iDeadCharacterIndex);
}

void NetServer_ShipApplyCrewHitpoints(ref rOurClient, float fCrewHP, int wBallClientID)
{
	if (NetServer_IsGod(sti(rOurClient.ID))) { return; }		// COOP s27: immortality of the admin menu
	NetServer_BotProvoked(sti(rOurClient.ID), wBallClientID);		// COOP s28: grapes on a neutral bot
	ref rBaseShip = Net_GetShipByIndex(sti(rOurClient.Ship.Type));
	// COOP s45: like Ship_ApplyCrewHitpoints of the single player: 1 - 0.75 * Defence (it was 1 - 0.8 * Defence), and the cannons
	// do not take the last 25 % of the minimal crew (it was 5 %)
	float fMultiply = 1.0 - 0.75 * Clampf(stf(rOurClient.Skills.Defence) / NETSKILL_MAX);
	if (NetServer_SeaPerk(sti(rOurClient.ID), NET_SPERK_DOCTOR)) { fMultiply = fMultiply * 0.8; }		// COOP: passive perk Doctor2 (includes Doctor1)
	float fNewCrewQuantity = stf(rOurClient.Ship.Crew.Quantity) - fCrewHP * fMultiply;
	float f5Percent = stf(rBaseShip.MinCrew) * 0.25;
	if (fNewCrewQuantity < f5Percent) { fNewCrewQuantity = f5Percent; }
	if (fNewCrewQuantity < 1.0) { fNewCrewQuantity = 1.0; }
	if (fNewCrewQuantity >= stf(rOurClient.Ship.Crew.Quantity)) { return; }		// COOP s45: a hit never adds sailors (a crew below the limit)
	rOurClient.Ship.Crew.Quantity = fNewCrewQuantity;
}

// COOP s45: a value of the ship of the same name in the table of the single player (ShipsTypes, ships\ships_init.c: the
// game loads it at the start), the default when it has no such ship or value
float NetServer_SPShipAttr(ref rNetShip, string sAttr, float fDef)
{
	int iSP = -2;
	if (CheckAttribute(rNetShip, "SPIdx")) { iSP = sti(rNetShip.SPIdx); }
	if (iSP == -2)
	{
		iSP = -1;
		string sNm = "";
		if (CheckAttribute(rNetShip, "Name")) { sNm = rNetShip.Name; }
		for (int i=0; i<SHIP_TYPES_QUANTITY; i++)
		{
			if (iSP >= 0) { continue; }
			if (!CheckAttribute(&ShipsTypes[i], "Name")) { continue; }
			if (ShipsTypes[i].Name == sNm) { iSP = i; }
		}
		rNetShip.SPIdx = iSP;
		Trace("COOP: the ship " + sNm + " of the net is the ship " + iSP + " of the single player");
	}
	if (iSP < 0) { return fDef; }
	if (!CheckAttribute(&ShipsTypes[iSP], sAttr)) { return fDef; }
	return stf(ShipsTypes[iSP].(sAttr));
}

void NetServer_ShipUpdateParameters()
{
	int wOurClientID = GetEventData();
	ref rClient = NetServer_GetClient(wOurClientID);

	// COOP: a bot: the packets stop when it is gone (server\Bots.c)
	bool bBotShip = false;
	if (wOurClientID >= NET_BOT_FIRST)
	{
		if (wOurClientID < NET_MAXCLIENTS) { bBotShip = true; }
	}
	if (bBotShip)
	{
		if (!NetServer_BotKeepUpdating(wOurClientID)) { rClient.BotLoop = 0; return; }
	}
	float fGameSpeed = stf(NetServer.GameSpeed);

	ref rBall = Net_GetGoodByIndex(sti(rClient.Ship.Cannons.Charge.Type));
	string sBallName = rBall.name;

	// some of ship parameters
		ref rShip = Net_GetShipByIndex(sti(rClient.Ship.Type));

		float fMaxSpeedZ = stf(rShip.SpeedRate) * stf(rClient.Ship.SpeedRateModifier);
		float fMaxSpeedY = stf(rShip.TurnRate) * stf(rClient.Ship.TurnRateModifier) / 444.444;		// cool magic number :))
		float fSpeedFromHP = 0.7 + (stf(rClient.Ship.HP) / stf(rClient.Ship.MaxHP)) / 3.34;
		float fCurrentSpeedZ = stf(rClient.Ship.Speed.z);
		float fLoad = NetServer_ShipLoadRatio(rClient);
		NetServer_PowderTrack(rClient);

	// Calc min crew ratio
		float fCrewMin = stf(rShip.MinCrew);
		float fCrewMax = stf(rShip.MaxCrew);
		float fCrewCur = stf(rClient.Ship.Crew.Quantity);
		float fCrewMin25Percent = stf(rShip.MinCrew);
		float fMinRatio = Bring2Range(0.0, 1.0, fCrewMin25Percent, fCrewMax, fCrewCur);

	// wind
		// COOP s46: the wind of the single player (m/s of the weather, 4..13). NetServer_WhrGetWindSpeed() is x5 of it (for the sails
		// of the engine): with the formula of the single player (s45) the ships went 5 times as fast (a xebec 40 knots)
		float fWindPower = stf(NSWeather.WindSpeed) / WIND_NORMAL_POWER;
		float fWindDotShip = GetDotProduct(NetServer_WhrGetWindAngle(), stf(rClient.Ship.Ang.y));		// Wind.ay | Ship.ay

	// calculate MaxSpeedZ
		float fTRFromSkill = Bring2Range(0.1, 1.0, 0.0, 2.0, 2.0 * stf(rClient.Skills.Sailing) / NETSKILL_MAX);
		fTRFromSKill = 0.7 + fTRFromSkill / 3.34;
		//float fSpeedPerk = NetServer_IsPerkUse(rClient.Perks.ShipSpeedUp, 1.0, 1.15); 
		//fSpeedPerk = NetServer_IsPerkUse(rClient.Perks.SailingProfessional, fSpeedPerk, 1.20); 

		float	fTRFromWeight = Clampf(1.03 - NET_TURN_WEIGHT_MULT * stf(rShip.TurnDependWeight) * fLoad);
		float	fSpeedFromWeight = Clampf(1.03 - stf(rShip.SpeedDependWeight) * fLoad);
		float	fSpeedFromPeople = 0.85 + 0.15 * Clampf(fCrewCur / (fCrewMax + 0.001));
		float	fTRFromSpeed = 1.0 - stf(rShip.TurnDependSpeed) * (1.0 - Clampf(fCurrentSpeedZ / fMaxSpeedZ));
		float	fTRFromPeople = Bring2Range(0.1, 1.0, 0.0, 1.0, fMinRatio);
		float	fTRFromSailDamage = Bring2Range(0.1, 1.0, 0.1, 100.0, stf(rClient.Ship.SP));

		// COOP: passive perk SailingProfessional (includes ShipSpeedUp): x1.20, like SpeedBySkill of single player
		float fSpeedPerk = 1.0;
		if (NetServer_SeaPerk(wOurClientID, NET_SPERK_SAILING)) { fSpeedPerk = 1.20; }

		// COOP s45: the speed like the single player with arcade sails (scripts\ShipsUtilites.c FindShipSpeed / SpeedBySkill,
		// characters\GeneratorUtilite.c Sea_ApplyMaxSpeedZ, sea_ai\AIShip.c Ship_UpdateParameters x2.5). The speed rate, its weight
		// factor and WindAgainstSpeed are the ones of the table of the single player. The net formula had no course to the wind
		// and no x2.5: the net ships were about 1.7 times slower.
		float fSZRate = NetServer_SPShipAttr(rShip, "SpeedRate", stf(rShip.SpeedRate)) * stf(rClient.Ship.SpeedRateModifier);
		float fSZWeightK = NetServer_SPShipAttr(rShip, "SpeedDependWeight", 0.15);
		float fSZAgainst = NetServer_SPShipAttr(rShip, "WindAgainstSpeed", 3.0);
		float fSZHp = 0.8 + 0.3 * Clampf(stf(rClient.Ship.HP) / stf(rClient.Ship.MaxHP));
		if (fSZHp > 1.0) { fSZHp = 1.0; }
		float fSZSails = Bring2Range(0.1, 1.0, 0.1, 100.0, stf(rClient.Ship.SP));
		float fSZWeight = Clampf(1.03 - fSZWeightK * fLoad);
		float fSZSkill = (0.7 + 0.03 * 10.0 * stf(rClient.Skills.Sailing) / NETSKILL_MAX) * fSpeedPerk;
		float fSZOpt = fCrewMax * 0.8;		// OptCrew of the single player, a crew of the full experience
		if (fSZOpt < 1.0) { fSZOpt = 1.0; }
		float fSZPeople = 0.85 + 0.15 * Clampf(fCrewCur / fSZOpt);
		float fSZCruise = fSZRate * fSZWeight * fSZSkill * fSZHp * fSZSails * fSZPeople * fWindPower;		// fShipSpeed of the single player
		float fSZTop = fSZCruise;
		if (fWindDotShip >= -0.1) { fSZTop = fSZTop * (0.81 + fWindDotShip / (1.9 + pow(fSZAgainst, 0.33))); }
		else { fSZTop = fSZTop * (0.75 - fWindDotShip / 3.2 - pow(abs(fWindDotShip), fSZAgainst)); }
		if (fSZTop < 0.0) { fSZTop = 0.0; }
		rClient.Ship.MaxSpeedZ = fGameSpeed * 2.5 * fSZTop;		// x2.5: the arcade sails

		/*if (fWindDotShip < -0.70)
		{
			rClient.Ship.MaxSpeedZ = stf(rClient.Ship.MaxSpeedZ) * (fWindDotShip + 1.0);
		}*/

	// COOP s45: the turn like the single player with arcade sails (iArcadeSails = 1: sea_ai\AIShip.c Ship_UpdateParameters,
	// scripts\ShipsUtilites.c FindShipTurnRate and TurnBySkill). The turn rate and its weight factor are the ones of the table of
	// the single player (NetServer_SPShipAttr). The net formula (TurnRate / 444.444, the factors in 0.1..1 of it) turned the ships
	// about 3.5 times slower.
		float fSPTurnRate = NetServer_SPShipAttr(rShip, "TurnRate", stf(rShip.TurnRate));
		float fSPTurnWeight = NetServer_SPShipAttr(rShip, "TurnDependWeight", 0.2);
		float fTRMaxY = fSPTurnRate * stf(rClient.Ship.TurnRateModifier) / 244.444;
		float fTRHp = 0.7 + 0.3 * Clampf(stf(rClient.Ship.HP) / stf(rClient.Ship.MaxHP));
		float fTRWeight = Clampf(1.03 - fSPTurnWeight * fLoad);
		float fTROpt = fCrewMax * 0.8;		// OptCrew of the single player, a crew of the full experience
		if (fTROpt < 1.0) { fTROpt = 1.0; }
		float fTRPeople = 0.3 + 0.7 * Clampf(fCrewCur / fTROpt);
		float fTRSkill = 0.5 + 0.05 * 10.0 * stf(rClient.Skills.Sailing) / NETSKILL_MAX;
		float fTRSails = Bring2Range(0.05, 1.0, 0.1, 100.0, stf(rClient.Ship.SP));
		// COOP: passive perk SailingProfessional (includes ShipTurnRateUp) x1.20, active perk Turn180 x4 (TurnBySkill)
		float fTurnPerk = 1.0;
		if (NetServer_SeaPerk(wOurClientID, NET_SPERK_SAILING)) { fTurnPerk = 1.20; }
		if (NetServer_ActivePerkIsOn(wOurClientID, NET_APERK_TURN180)) { fTurnPerk = fTurnPerk * 4.0; }
		float fTRShip = fTRWeight * fTRSkill * fTurnPerk * fTRPeople * fTRSails * fTRHp;

		float fTRFromSailState = 1.0;
		int iTRSail = MakeInt(stf(rClient.Ship.SailState) * 2.0);
		if (iTRSail <= 0) { fTRFromSailState = 0.05; }
		if (iTRSail >= 2) { fTRFromSailState = 0.68; }

		// slower = worse turn: a player 0.5, a bot 0.51 - rate / 100 (the difficulty of the bots)
		float fTRSpdK = 0.5;
		if (bBotShip) { fTRSpdK = 0.51 - makefloat(NetServer_BDCapRate()) * 0.01; }
		float fTRTop = fGameSpeed * fSZCruise;		// the single player compares with the speed before the wind course and the x2.5
		float fTRSpd = 1.0 - fTRSpdK;
		if (fTRTop > 0.01) { fTRSpd = 1.0 - fTRSpdK * (1.0 - Clampf(fCurrentSpeedZ / fTRTop)); }

		float fTRResult = Bring2Range(0.07, 0.95, 0.00001, 1.0, fTRMaxY * fTRShip * fTRFromSailState * fTRSpd);
		rClient.Ship.MaxSpeedY = fGameSpeed * 2.0 * fTRResult;		// x2: the arcade sails
		// COOP s37: the ship of a captain who fights on the deck stands still (s41: a prize and its captor too)
		if (NetServer_ShipHeld(wOurClientID))
		{
			rClient.Ship.MaxSpeedZ = 0.0;
			rClient.Ship.MaxSpeedY = 0.0;
			rClient.Ship.SailState = 0.0;
		}

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

	// COOP: score for the F1 table (ships sunk / deaths), also right for players who joined later
	NMAddWord(iSMsg, sti(rClient.Stat.Sunk));
	NMAddWord(iSMsg, sti(rClient.Stat.Deaths));
	int iShipState = 0;										// 0 afloat, 1 wrecked (respawn), 2 sunk
	if (NetServer_IsClientDead(rClient))
	{
		iShipState = 2;
		if (sti(NetServer.Respawn))
		{
			if (wOurClientID < NET_BOT_FIRST) { iShipState = 1; }
		}
		if (CheckAttribute(rClient, "WreckedNow")) { if (sti(rClient.WreckedNow)) { iShipState = 1; } }		// COOP: a wreck of a bot that comes back
	}
	NMAddByte(iSMsg, iShipState);

	// COOP: cannons really standing on the ship (cannons bought in the lobby), for the spyglass of the others
	NMAddWord(iSMsg, NetServer_ShipCannonsNum(rClient));

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
		// COOP: a bort without cannons is shown empty (nothing to load there)
		float fBortCharge = stf(arBorts.(sCurBort).ChargeRatio);
		if (NetServer_ShipCannonsNum(rClient) <= 0) { fBortCharge = 0.0; }
		int iUiBort = 2;		// sNetBortNames: cannonl, cannonr, cannonf, cannonb
		if (i == 1) { iUiBort = 3; }
		if (i == 2) { iUiBort = 0; }
		if (i == 3) { iUiBort = 1; }
		if (Net_GetBortMax(sti(rClient.Ship.Type), iUiBort) > 0 && NetServer_BortHave(rClient, iUiBort) <= 0) { fBortCharge = 0.0; }
		NMAddWord(iSMsg, makeint(stf(arBorts.(sCurBort).MaxFireDistance))); 
		NMAddByte(iSMsg, makeint(fBortCharge * 255.0)); 
		NMAddByte(iSMsg, makeint(stf(arBorts.(sCurBort).DamageRatio) * 255.0)); 
	}
	if (!bBotShip) { NetServer_SendMessage(wOurClientID, iSMsg, false); }		// COOP: a bot has no player
	NMDelete(iSMsg);

	// auto boarding availability (checked every NET_BOARD_STATE_TICKS updates, sent to the client only on change)
	if (!bBotShip) { NetServer_UpdateBoardState(wOurClientID); }

	PostEvent("NetServer_ShipUpdateParameters", 120, "l", wOurClientID);
}

void NetServer_CalculateReloadTime(ref rClient)
{
	ref rShip = Net_GetShipByIndex(sti(rClient.Ship.Type));
	ref rCannon = Net_GetCannonByIndex(sti(rClient.Ship.Cannons.Type));

	float fCannonSkill = stf(rClient.Skills.Cannons) / NETSKILL_MAX;

	float fMultiply = (6.0 - NetServer_GetSpeedModifier()) / 5.0;
	// COOP: passive perk FastReload x0.9, active perk ImmediateReload x0.5 (instead of it), like AICannon.c of single player
	if (sti(rClient.ID) < NET_MAXCLIENTS)
	{
		if (NetServer_ActivePerkIsOn(sti(rClient.ID), NET_APERK_IMMEDIATERELOAD)) { fMultiply = fMultiply * 0.5; }
		else { if (NetServer_SeaPerk(sti(rClient.ID), NET_SPERK_FASTRELOAD)) { fMultiply = fMultiply * 0.9; } }
	}
	else
	{
		// COOP: the fort of the hard difficulty: passive FastReload x0.9, and ImmediateReload x0.5 in its bursts (NetServer_FortBurstOn)
		if (sti(rClient.ID) == NET_FORTCLIENT && NetServer_FortHard())
		{
			if (NetServer_FortBurstOn()) { fMultiply = fMultiply * 0.5; }
			else { fMultiply = fMultiply * 0.9; }
		}
	}
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

	rClient.Ship.Cannons.SpeedV0 = stf(rCannon.SpeedV0) * stf(rBall.SpeedV0);
	// COOP: passive perk LongRangeShoot (the fort of the hard difficulty has it too)
	bool bLongRange = false;
	if (sti(rClient.ID) < NET_MAXCLIENTS) { bLongRange = NetServer_SeaPerk(sti(rClient.ID), NET_SPERK_LONGRANGE); }
	if (sti(rClient.ID) == NET_FORTCLIENT) { bLongRange = NetServer_SeaPerk(NET_FORTCLIENT, NET_SPERK_LONGRANGE); }
	if (bLongRange) { rClient.Ship.Cannons.SpeedV0 = stf(rClient.Ship.Cannons.SpeedV0) * 1.15; }
}


// ================================================================================================
// COOP: perks of single player for every captain (switches of the Create game screen)
// ================================================================================================

// all passive sea perks (server\..\common\Perks.c has the list): only player ships, the fort has none
bool NetServer_PassivePerksFor(int wClientID)
{
	// COOP: the fort of the hard difficulty has the perks that a fort can use (they do not depend on the lobby switch):
	// FastReload, LongRangeShoot, CriticalShoot, CannonProfessional (+30% hull damage, +10% crits), CrewDamageUp
	if (wClientID == NET_FORTCLIENT) { return NetServer_FortHard(); }
	if (!sti(NetServer.PassivePerks)) { return false; }
	if (wClientID < 0 || wClientID >= NET_MAXCLIENTS) { return false; }
	return true;
}

// COOP s28: one sea perk of a ship (common\Perks.c: NET_SPERK_*): the switches of the server give all of them to everybody,
// otherwise the perks the captain has bought (the bots and the fort have none of their own)
bool NetServer_SeaPerk(int wClientID, int iPerk)
{
	if (wClientID == NET_FORTCLIENT)
	{
		if (iPerk < NET_SPERK_PASSIVE_NUM) { return NetServer_FortHard(); }
		return false;
	}
	if (wClientID < 0 || wClientID >= NET_MAXCLIENTS) { return false; }
	if (iPerk < NET_SPERK_PASSIVE_NUM)
	{
		if (sti(NetServer.PassivePerks)) { return true; }
	}
	else
	{
		if (sti(NetServer.ActivePerks)) { return true; }
	}
	if (!CheckAttribute(&NSClients[wClientID], "SeaPerks")) { return false; }
	return Net_SeaPerkBit(sti(NSClients[wClientID].SeaPerks), iPerk);
}

// the ship has at least one active perk (the command menu, the cooldown states)
bool NetServer_AnyActivePerk(int wClientID)
{
	for (int iAP=0; iAP<NET_APERK_NUM; iAP++)
	{
		if (NetServer_SeaPerk(wClientID, NET_SPERK_ACTIVE0 + iAP)) { return true; }
	}
	return false;
}

int NetServer_GetShipLife(int wClientID)
{
	if (wClientID < 0 || wClientID >= NET_TOTALCLIENTS) { return 0; }
	if (!CheckAttribute(&NSClients[wClientID], "Life")) { return 0; }
	return sti(NSClients[wClientID].Life);
}

bool NetServer_ActivePerkIsOn(int wClientID, int iPerk)
{
	if (!NetServer_SeaPerk(wClientID, NET_SPERK_ACTIVE0 + iPerk)) { return false; }		// COOP s28: bought or the switch
	if (wClientID < 0 || wClientID >= NET_MAXCLIENTS) { return false; }
	string sAttr = "APerk.p" + iPerk + ".Until";
	if (!CheckAttribute(&NSClients[wClientID], sAttr)) { return false; }
	string sP = "p" + iPerk;
	return iServerTime < sti(NSClients[wClientID].APerk.(sP).Until);
}

void NetServer_SendPerkState(int wClientID, int iPerk)
{
	if (!NetServer_IsInGame(wClientID)) { return; }
	string sP = "p" + iPerk;
	int iUntil = 0;
	int iReady = 0;
	if (CheckAttribute(&NSClients[wClientID], "APerk." + sP + ".Until")) { iUntil = sti(NSClients[wClientID].APerk.(sP).Until); }
	if (CheckAttribute(&NSClients[wClientID], "APerk." + sP + ".Ready")) { iReady = sti(NSClients[wClientID].APerk.(sP).Ready); }

	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_SHIP);
	NMAddByte(iSMsg, NSC_SHIP_PERK_STATE);
	NMAddByte(iSMsg, iPerk);
	NMAddDword(iSMsg, iUntil);
	NMAddDword(iSMsg, iReady);
	NetServer_SendMessage(wClientID, iSMsg, true);
	NMDelete(iSMsg);
}

void NetServer_SendAllPerkStates(int wClientID)
{
	if (!NetServer_AnyActivePerk(wClientID)) { return; }		// COOP s28
	for (int i=0; i<NET_APERK_NUM; i++) { NetServer_SendPerkState(wClientID, i); }
}

// client pressed an active perk in the command menu
void NetServer_UseActivePerk(int wClientID, int iPerk)
{
	if (iPerk < 0 || iPerk >= NET_APERK_NUM) { return; }
	if (!NetServer_SeaPerk(wClientID, NET_SPERK_ACTIVE0 + iPerk)) { return; }		// COOP s28: bought or the switch
	if (!NetServer_ShipIsAlive(wClientID)) { return; }

	ref rClient = NetServer_GetClient(wClientID);
	string sP = "p" + iPerk;

	int iReady = 0;
	if (CheckAttribute(rClient, "APerk." + sP + ".Ready")) { iReady = sti(rClient.APerk.(sP).Ready); }
	if (iServerTime < iReady)
	{
		int iSMsg = NMCreate();
		NMAddByte(iSMsg, NC_SHIP);
		NMAddByte(iSMsg, NSC_SHIP_PERK_DENIED);
		NMAddByte(iSMsg, iPerk);
		NMAddWord(iSMsg, (iReady - iServerTime + 999) / 1000);
		NetServer_SendMessage(wClientID, iSMsg, true);
		NMDelete(iSMsg);
		return;
	}

	rClient.APerk.(sP).Until = iServerTime + Net_GetActivePerkDuration(iPerk) * 1000;
	rClient.APerk.(sP).Ready = iServerTime + Net_GetActivePerkDelay(iPerk) * 1000;
	rClient.APerk.(sP).Life = NetServer_GetShipLife(wClientID);
	Trace("COOP: active perk " + sNetActivePerkName[iPerk] + " client=" + wClientID);

	switch (iPerk)
	{
		case NET_APERK_IMMEDIATERELOAD:
			NetServer_CalculateReloadTime(rClient);
			// back to the normal reload time when the perk is over
			PostEvent("NetServer_PerkTick", Net_GetActivePerkDuration(iPerk) * 1000 + 50, "lll", wClientID, iPerk, NetServer_GetShipLife(wClientID));
		break;
		case NET_APERK_LIGHTREPAIR:
			PostEvent("NetServer_PerkTick", 1000, "lll", wClientID, iPerk, NetServer_GetShipLife(wClientID));
		break;
		case NET_APERK_INSTANTREPAIR:
			PostEvent("NetServer_PerkTick", 1000, "lll", wClientID, iPerk, NetServer_GetShipLife(wClientID));
		break;
	}

	NetServer_SendPerkState(wClientID, iPerk);
}

// planks for 1% of the hull (GetHullPPP of single player, Builder x0.9)
float NetServer_HullPlanksPerPercent(ref rClient)
{
	ref rShip = Net_GetShipByIndex(sti(rClient.Ship.Type));
	float fRet = 8.0 - stf(rShip.Class);
	float fHP = stf(rShip.HP) / 700.0;
	if (fHP > fRet) { fRet = fHP; }
	if (NetServer_SeaPerk(sti(rClient.ID), NET_SPERK_BUILDER)) { fRet = fRet * 0.9; }
	return fRet;
}

// sailcloth for 1% of the sails (GetSailSPP of single player, Builder x0.9)
float NetServer_SailClothPerPercent(ref rClient)
{
	ref rShip = Net_GetShipByIndex(sti(rClient.Ship.Type));
	float fRet = 8.0 - stf(rShip.Class);
	if (NetServer_SeaPerk(sti(rClient.ID), NET_SPERK_BUILDER)) { fRet = fRet * 0.9; }
	return fRet;
}

// one second of LightRepair (no materials, up to 10%, 1% a second) / InstantRepair (up to 65%, 5% a second, materials),
// numbers from battle_interface\utils.c of single player
void NetServer_PerkRepairStep(ref rClient, float fUpTo, float fStep, bool bMaterials)
{
	float fMaxHP = stf(rClient.Ship.MaxHP);
	if (fMaxHP < 1.0) { fMaxHP = 1.0; }
	float fHullPct = 100.0 * stf(rClient.Ship.HP) / fMaxHP;
	if (fHullPct < fUpTo)
	{
		float fRepH = fUpTo - fHullPct;
		if (fRepH > fStep) { fRepH = fStep; }
		if (bMaterials)
		{
			float fPPP = NetServer_HullPlanksPerPercent(rClient);
			int iPlanks = sti(rClient.Ship.Goods.Planks);
			if (fPPP > 0.0 && fRepH * fPPP > makefloat(iPlanks)) { fRepH = makefloat(iPlanks) / fPPP; }
			int iUsed = makeint(fRepH * fPPP + 0.5);
			if (iUsed > iPlanks) { iUsed = iPlanks; }
			rClient.Ship.Goods.Planks = iPlanks - iUsed;
		}
		if (fRepH > 0.0)
		{
			float fNewHP = stf(rClient.Ship.HP) + fRepH * fMaxHP / 100.0;
			if (fNewHP > fMaxHP) { fNewHP = fMaxHP; }
			rClient.Ship.HP = fNewHP;
		}
	}

	float fSailPct = 100.0;
	if (CheckAttribute(rClient, "Ship.SP")) { fSailPct = stf(rClient.Ship.SP); }
	if (fSailPct < fUpTo)
	{
		float fRepS = fUpTo - fSailPct;
		if (fRepS > fStep) { fRepS = fStep; }
		if (bMaterials)
		{
			float fSPP = NetServer_SailClothPerPercent(rClient);
			int iCloth = sti(rClient.Ship.Goods.Sailcloth);
			if (fSPP > 0.0 && fRepS * fSPP > makefloat(iCloth)) { fRepS = makefloat(iCloth) / fSPP; }
			if (fRepS > 0.0)
			{
				float fDone = NetServer_ShipSailRepairBy(rClient, fRepS);
				int iClothUsed = makeint(fDone * fSPP + 0.5);
				if (iClothUsed > iCloth) { iClothUsed = iCloth; }
				rClient.Ship.Goods.Sailcloth = iCloth - iClothUsed;
			}
		}
		else
		{
			NetServer_ShipSailRepairBy(rClient, fRepS);
		}
	}
}

// timer of the active perks: LightRepair / InstantRepair once a second, end of ImmediateReload
void NetServer_PerkTick()
{
	int wClientID = GetEventData();
	int iPerk = GetEventData();
	int iLife = GetEventData();

	if (wClientID < 0 || wClientID >= NET_MAXCLIENTS) { return; }
	if (iLife != NetServer_GetShipLife(wClientID)) { return; }		// wrecked / sunk since then
	ref rClient = NetServer_GetClient(wClientID);
	if (NetServer_IsClientDead(rClient)) { return; }

	switch (iPerk)
	{
		case NET_APERK_IMMEDIATERELOAD:
			NetServer_CalculateReloadTime(rClient);
		break;
		case NET_APERK_LIGHTREPAIR:
			if (!NetServer_ActivePerkIsOn(wClientID, iPerk)) { return; }
			NetServer_PerkRepairStep(rClient, 10.0, 1.0, false);
			PostEvent("NetServer_PerkTick", 1000, "lll", wClientID, iPerk, iLife);
		break;
		case NET_APERK_INSTANTREPAIR:
			if (!NetServer_ActivePerkIsOn(wClientID, iPerk)) { return; }
			NetServer_PerkRepairStep(rClient, 65.0, 5.0, true);		// 65% = InstantRepairRATE of single player
			PostEvent("NetServer_PerkTick", 1000, "lll", wClientID, iPerk, iLife);
		break;
	}
}

// ================================================================================================
// COOP: respawn (CS-like). The ship is not sunk: it is "wrecked" (stops, burns, takes no damage, does not obey),
// after NET_RESPAWN_DELAY it comes back at a spawn point with the loadout bought in the lobby. Score: Stat.Sunk / Stat.Deaths.
// ================================================================================================
void NetServer_SetClientWrecked(ref rOurClient, int iKillStatus, int wKillerClientID)
{
	int wDeadID = sti(rOurClient.ID);
	if (NetServer_IsClientDead(rOurClient)) { return; }

	// score: a kill for the killer (not for a suicide / brander / island), a death for the victim
	if (NetServer_IsClient(wKillerClientID) && wKillerClientID != wDeadID && wKillerClientID < NET_MAXCLIENTS)
	{
		ref rKiller = NetServer_GetClient(wKillerClientID);
		rKiller.Stat.Sunk = sti(rKiller.Stat.Sunk) + 1;
		ref rKillerBase = Net_GetShipByIndex(sti(rKiller.Ship.Type));
		ref rDeadBase = Net_GetShipByIndex(sti(rOurClient.Ship.Type));
		NetServer_AddClientExp(rKiller, 2 * MakeInt(stf(rOurClient.Ship.MaxHP) * stf(rKillerBase.Class) / stf(rDeadBase.Class)));
	}
	rOurClient.Stat.Deaths = sti(rOurClient.Stat.Deaths) + 1;
	rOurClient.Stat.Wreck = sti(rOurClient.Stat.Wreck) + 1;

	rOurClient.Killer.Status = iKillStatus;
	rOurClient.Killer.ClientID = wKillerClientID;

	bool bDetonate = (rand(2) == 1);
	if (CheckAttribute(rOurClient, "ForceDetonate")) { bDetonate = true; DeleteAttribute(rOurClient, "ForceDetonate"); }

	// the wreck: no crew, no control, no repair / perks, 1 point of hull (a hull of 0 could start the sinking of the engine)
	// net.dll: when the "Dead" attribute becomes non-zero the ship entity of the server starts sinking and needs Ship.Sink.Speed
	// (Assert pASinkSpeed, ship.cpp line 508 - the host crashed). Zero speeds: the wreck stays afloat; the respawn deletes
	// Ship.Sink and creates the entity again (NetServer_PlaceShipEntity)
	rOurClient.Ship.Sink.Speed.x = 0.0;
	rOurClient.Ship.Sink.Speed.y = 0.0;
	rOurClient.Ship.Sink.Speed.z = 0.0;
	int iWreckSink = 0;
	if (NET_WRECK_SINK)
	{
		// COOP s27: the wreck sinks like a ship of the game without respawn; the respawn creates it again (server and clients)
		iWreckSink = 1;
		rOurClient.Ship.Sink.Speed.y = 0.35;
		rOurClient.Ship.Sink.Speed.x = 0.017 * (frnd() * 2.0 - 1.0);
		rOurClient.Ship.Sink.Speed.z = 0.017 * (frnd() * 2.0 - 1.0);
	}
	rOurClient.WreckedNow = true;
	if (iWreckSink) { NetServer_ShipNoCannonTarget(rOurClient); }		// COOP s27: a sinking wreck is not a target (the respawn creates it again)
	rOurClient.Dead = true;
	rOurClient.Life = NetServer_GetShipLife(wDeadID) + 1;
	rOurClient.Ship.HP = 1.0;
	rOurClient.Ship.Crew.Quantity = 0;
	rOurClient.Ship.SailState = 0.0;
	rOurClient.Ship.TurnLeft = 0;
	rOurClient.Ship.TurnRight = 0;
	rOurClient.Ship.TurnState = 0.0;
	rOurClient.Skills.Repair.Active = 0;
	rOurClient.Ship.BranderActive = 0;
	rOurClient.Ship.BranderArmUntil = 0;
	DeleteAttribute(rOurClient, "SpawnProtect");
	for (int i=0; i<NET_APERK_NUM; i++)
	{
		string sP = "p" + i;
		if (CheckAttribute(rOurClient, "APerk." + sP + ".Until")) { rOurClient.APerk.(sP).Until = 0; }
	}
	NetServer_CalculateReloadTime(rOurClient);

	int iSeconds = NetServer_GetRespawnTime();

	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_SHIP);
	NMAddByte(iSMsg, NSC_SHIP_WRECKED);
	NMAddClientID(iSMsg, wDeadID);
	NMAddByte(iSMsg, iKillStatus);
	NMAddClientID(iSMsg, wKillerClientID);
	NMAddByte(iSMsg, bDetonate);
	NMAddByte(iSMsg, iSeconds);
	NMAddByte(iSMsg, iWreckSink);
	NMAddFloat(iSMsg, stf(rOurClient.Ship.Sink.Speed.x));
	NMAddFloat(iSMsg, stf(rOurClient.Ship.Sink.Speed.y));
	NMAddFloat(iSMsg, stf(rOurClient.Ship.Sink.Speed.z));
	NetServer_SendToInGame(iSMsg);
	NMDelete(iSMsg);

	Trace("COOP: ship " + wDeadID + " wrecked by " + wKillerClientID + ", respawn in " + iSeconds + " s");

	PostEvent("NetServer_RespawnEvent", iSeconds * 1000, "lll", wDeadID, NetServer_GetShipLife(wDeadID), sti(NetServer.Round));
	// COOP s27: the respawn creates the ship again; if it does not come (the player left, the bot respawn was switched off) the wreck goes
	NetServer_WreckGoneLater(wDeadID, iSeconds);
}

// COOP: seconds between the death and the respawn (slider of the Create game screen, 10..180)
int NetServer_GetRespawnTime()
{
	int iSec = NET_RESPAWN_DELAY / 1000;
	if (CheckAttribute(&NetServer, "RespawnTime")) { iSec = sti(NetServer.RespawnTime); }
	if (iSec < NET_RESPAWN_TIME_MIN) { iSec = NET_RESPAWN_TIME_MIN; }
	if (iSec > NET_RESPAWN_TIME_MAX) { iSec = NET_RESPAWN_TIME_MAX; }
	return iSec;
}

void NetServer_RespawnEvent()
{
	int wID = GetEventData();
	int iLife = GetEventData();
	int iRound = GetEventData();

	if (!bServerGameStarted) { return; }
	if (iRound != sti(NetServer.Round)) { return; }
	if (wID < 0 || wID >= NET_MAXCLIENTS) { return; }
	if (iLife != NetServer_GetShipLife(wID)) { return; }
	if (!NetServer_HasShip(wID)) { return; }
	if (!sti(NSClients[wID].Dead)) { return; }

	// the player has left: the wreck waits, he gets the ship back when he reconnects (NetServer_LateJoin)
	// COOP: a bot has no player, it comes back by itself (unless the admin deleted it)
	if (NetServer_IsBot(wID))
	{
		if (NetServer_BotRemoved(wID)) { return; }
	}
	else
	{
		if (!sti(NSClients[wID].Use)) { return; }
	}

	NetServer_RespawnShip(wID);
}

// gives the ship back: full hull and sails, the bought crew / goods / charge, a new place
void NetServer_RespawnShip(int wID)
{
	int j;
	ref rC = NetServer_GetClient(wID);
	if (!NetServer_HasShip(wID)) { return; }

	bool bPrizeBack = NetServer_PrizeRestoreOrig(wID);		// COOP s44: it sank as the old ship of a captor (net\Board.c)
	// COOP s46: the Esc menu "Корабль" chose a new loadout: it comes now (server\Buy.c NetServer_OnBuy_Respawn)
	if (!NetServer_IsBot(wID) && CheckAttribute(rC, "RespawnBuy.Type"))
	{
		aref arRB;
		makearef(arRB, rC.RespawnBuy);
		NetServer_BuyApply(wID, arRB);
		DeleteAttribute(rC, "RespawnBuy");
		DeleteAttribute(rC, "PrizeOrig");
		NetServer_SnapshotLoadout(rC);		// the bought crew, charge, goods and cannons on the borts (read just below)
		bPrizeBack = true;		// every machine creates the new type at the respawn place (NSC_SHIP_PRIZE_SWAP)
		Trace("COOP: respawn of " + wID + " with the new loadout, ship type " + rC.Ship.Type);
	}
	rC.Life = NetServer_GetShipLife(wID) + 1;
	rC.Dead = false;
	rC.WreckedNow = false;
	DeleteAttribute(rC, "Killer");
	DeleteAttribute(rC, "Ship.Sink");
	// COOP s48j: "Ship.sails" holds Sails.Color too (a reconnect crashed the client without it)
	int iSailColR = argb(255, 255, 255, 255);
	if (CheckAttribute(rC, "Ship.Sails.Color")) { iSailColR = sti(rC.Ship.Sails.Color); }
	DeleteAttribute(rC, "Ship.sails");		// sail damage of the old ship (the new entity has whole sails)
	rC.Ship.Sails.Color = iSailColR;
	DeleteAttribute(rC, "Ship.masts");
	DeleteAttribute(rC, "Ship.MastsFallen");
	rC.ShipGone = false;
	DeleteAttribute(rC, "Ship.GoodsHeld");
	DeleteAttribute(rC, "Ship.AmmoLast");

	rC.Ship.HP = stf(rC.Ship.MaxHP);
	rC.Ship.SP = 100.0;
	rC.Ship.Crew.Quantity = sti(rC.Ship.Start.Crew);
	rC.Ship.Cannons.Charge.Type = sti(rC.Ship.Start.Charge);
	for (j=0; j<Net_GetGoodsNum(); j++)
	{
		ref rGood = Net_GetGoodByIndex(j);
		string sGood = rGood.Name;
		rC.Ship.Goods.(sGood) = sti(rC.Ship.Start.Goods.(sGood));
	}
	rC.Ship.CannonsDumped = 0;
	DeleteAttribute(rC, "Ship.Bort");
	DeleteAttribute(rC, "Ship.ShotAcc");
	if (CheckAttribute(rC, "Ship.Start.Bort"))		// COOP: cannons on the borts as bought in the lobby
	{
		for (j=0; j<NET_BORT_NUM; j++)
		{
			string sSB = Net_BortLetter(j);
			rC.Ship.Bort.(sSB) = sti(rC.Ship.Start.Bort.(sSB));
		}
		NetServer_SyncCannonsDumped(rC);
	}
	rC.Ship.MinesDropped = 0;
	rC.Ship.MineCooldown = 0;
	rC.Ship.BranderActive = 0;
	rC.Ship.BranderArmUntil = 0;
	rC.Ship.TurnLeft = 0;
	rC.Ship.TurnRight = 0;
	rC.Ship.TurnState = 0.0;
	rC.Ship.SailState = 0.0;
	rC.Skills.Repair.Active = 0;
	rC.Skills.Repair.Cooldown = iServerTime - NET_REPAIR_COOLDOWN * 1000;
	rC.BoardSent = -1;
	rC.SpawnProtect = iServerTime + NET_RESPAWN_PROTECT;

	NetServer_CannonRecalculateParameters(rC);
	NetServer_CalculateFireTime(rC);
	NetServer_CalculateReloadTime(rC);

	NetServer_PickSpawn(wID);
	NetServer_PlaceShipEntity(wID, stf(NSSpawn.x), stf(NSSpawn.z), stf(NSSpawn.ay));

	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_SHIP);
	NMAddByte(iSMsg, NSC_SHIP_RESPAWN);
	NMAddClientID(iSMsg, wID);
	NMAddFloat(iSMsg, stf(rC.Ship.MaxHP));
	NMAddWord(iSMsg, sti(rC.Ship.Crew.Quantity));
	for (j=0; j<iNetGoodsNum; j++)
	{
		ref rGood2 = Net_GetGoodByIndex(j);
		string sGood2 = rGood2.Name;
		NMAddWord(iSMsg, sti(rC.Ship.Goods.(sGood2)));
	}
	NMAddByte(iSMsg, sti(rC.Ship.Cannons.Charge.Type));
	NMAddFloat(iSMsg, stf(rC.Ship.Pos.x));
	NMAddFloat(iSMsg, stf(rC.Ship.Pos.z));
	NMAddFloat(iSMsg, stf(rC.Ship.Ang.y));
	NetServer_SendToInGame(iSMsg);
	NMDelete(iSMsg);

	// COOP s44: the machines have the old ship of the captor in this slot: the own one is created after the respawn (alive)
	if (bPrizeBack) { NetServer_PrizeSendSwap(wID, stf(rC.Ship.Pos.x), stf(rC.Ship.Pos.z), stf(rC.Ship.Ang.y)); }

	NetServer_SendAllPerkStates(wID);
	if (NetServer_IsBot(wID)) { NetServer_BotAfterRespawn(wID); }		// COOP: the brain and the packets of a bot (server\Bots.c)
	Trace("COOP: ship " + wID + " respawned at x=" + rC.Ship.Pos.x + " z=" + rC.Ship.Pos.z);
}
