object NSFortModel, NSFortBlots;
#define NET_FORT_COLL_OX			105.0		// 105 COOP: the middle of the model of the fort is this far from the point that the push uses (world x), see NetServer_FortCollTick
#define NET_FORT_COLL_OZ			8.0		// 8 (the same, world z). The offset of the Barbados fort that is used on every map; 0, 0 = the push is round the locator of the fort
#define NET_FORT_COLL_LEN_K			1.0		// COOP: the field is wider by half of the length of the hull (Ship.BoxSize.z) times this: a manowar stops farther than a tartane
#define NET_FORT_COLL_SPRING			15.0		// COOP: push of the field per unit of depth that a ship went into it (acceleration along the bow)
#define NET_FORT_COLL_TURN				0.5			// COOP: Ship.Impulse.Rotate.y that turns the bow away from the fort; the sign is the direction (0 = no turn)
#define NET_FORT_HIT_MINSPEED			6.0			// COOP: a ship that comes to the fort faster than this (speed towards it) hits it like an island: sound and damage
#define NET_FORT_HIT_REFSPEED			30.0		// COOP: at this speed towards the fort the damage is NET_FORT_HIT_DMG of the maximum hull; it grows with the speed (up to 2x)
#define NET_FORT_HIT_DMG				0.03		// COOP: part of the maximum hull points (0.03 = 3%); a bot takes none, like from an island
#define NET_FORT_BOWSPRIT_COS			0.8			// COOP: the bow breaks the bowsprit only if it points at the fort within this cosine of the angle (0.8 = about 37 degrees); a side hit breaks nothing
#define NET_FORT_BOWSPRIT_MAST			0			// COOP: the bowsprit is mast number 0 in the engine (= ships\<name>\<name>_mast1.gm, the numbers of the engine start from 0: tested, number 1 dropped the main mast of the sloop); Tartane, WarTartane and XebekVML have none
#define NET_SHIP_BOWSPRIT_MINDIST		0.4			// COOP: ship to ship: the touch point must be farther from the middle of the ship than this part of half of its length (BoxSize.z / 2); the sloop touches its bow at 0.46 of it (10.4 of 22.7), the middle of a side is near 0.2
#define NET_BOWSPRIT_BOTS				0			// COOP: 1 = the bots lose the bowsprit too, 0 = only the players do
#define NET_SHIP_BOWSPRIT_MINPOWER		0.5			// COOP: ship to ship: the collision power (the 2nd value of Ship2ShipCollision) from which a bow hit breaks the bowsprit
#define NET_FORT_SPAWN_DIST		700.0		// COOP: the fort of Bot+ stands this far in front of the bow of the admin (m)
// COOP: the difficulty of the bots (Create game screen, "Bots": 0 harmless, 1 easy, 2 medium, 3 hard) is the difficulty of the fort
int NetServer_FortSkillLevel()
{
	int iL = 2;
	if (CheckAttribute(&NetServer, "BotSkill")) { iL = sti(NetServer.BotSkill); }
	return iL;
}
bool NetServer_FortSilent()
{
	// harmless difficulty, or a neutral fort of Bot+: the fort only stands, it does not fire
	if (CheckAttribute(&NetServer, "FortRel") && sti(NetServer.FortRel) == NET_BOTREL_NEUTRAL) { return true; }
	return NetServer_FortSkillLevel() <= 0;
}
bool NetServer_FortHard() { return NetServer_FortSkillLevel() >= 3; }		// hard: aim assist and all the perks that a fort can use

// COOP: the fort of the game mode (Capture the fort) is destroyed - the only fort whose death ends the battle.
// The fort of Bot+ (FortRel > 0) never ends it, and in another game mode a dead fort means nothing.
bool NetServer_FortGameDead()
{
	if (sti(NetServer.GameType) != NETGAME_FORT) { return false; }
	if (!CheckAttribute(&NetServer, "FortLocator") || NetServer.FortLocator == "") { return false; }	// the island of this map has no fort
	if (CheckAttribute(&NetServer, "FortRel") && sti(NetServer.FortRel) > 0) { return false; }		// the fort of Bot+, not the one of the mode

	ref rFort = NetServer_GetCLient(NET_FORTCLIENT);
	if (!CheckAttribute(rFort, "Ship.HP")) { return true; }		// the engine took the fort away, no data is left
	if (stf(rFort.Ship.HP) <= 0.0) { return true; }				// half of the cannons destroyed (the rule of the mode)
	if (CheckAttribute(rFort, "CoopCannonsTotal") && sti(rFort.CoopCannonsTotal) > 0)
	{
		if (sti(rFort.CoopCannonsDamaged) >= sti(rFort.CoopCannonsTotal)) { return true; }	// not a single cannon left
	}
	return false;
}

// hard fort: active perk ImmediateReload (reload time x0.5) like a captain uses it: Duration seconds in every Delay seconds
bool NetServer_FortBurstOn()
{
	if (!NetServer_FortHard()) { return false; }
	int iDelay = Net_GetActivePerkDelay(NET_APERK_IMMEDIATERELOAD) * 1000;
	int iDur = Net_GetActivePerkDuration(NET_APERK_IMMEDIATERELOAD) * 1000;
	int iPhase = iServerTime - (iServerTime / iDelay) * iDelay;
	return iPhase >= iDelay - iDur;
}

bool bNetFortMove = true; 		// COOP: the fort of Bot+ may be built again after it was removed / moved (Bring, Heal, Add after Delete). OFF: in tests the game
									// crashed on the first volley of a NetFort entity that was built again. Set true to try it on a build.
									// (s27: the cause was most likely the deleted model of the fort that the island still traced, see NetServer_FortRemove)
int iNetFortLastShot = -100000;		// COOP: time of the last shot of the fort (its own ball must not play the hit effect at the muzzle)
int iNetFortInfoGen = 0;		// COOP: generation of the NSC_FORT_INFO timer (old timers of a previous game stop)

void NetServer_DeleteFortEnvironment()
{
	DeleteClass(NetServer_GetCLient(NET_FORTCLIENT));
	DelEventHandler("NetServer_FortSelectCannonType", "NetServer_FortSelectCannonType");
	DelEventHandler("NetServer_OnFortHit", "NetServer_OnFortHit");
	DelEventHandler("NetServer_OnFortCannonDamage", "NetServer_OnFortCannonDamage");
	DelEventHandler("NetServer_FortInfoTick", "NetServer_FortInfoTick");
	DelEventHandler("NetServer_FortCollTick", "NetServer_FortCollTick");
	NetServer_FortClearPush();
	iNetFortInfoGen++;
}

void NetServer_CreateFortEnvironment(int iSMsg)
{
	int iIslandIndex = Net_GetIslandIndex(NetServer.IslandID);
	NetServer.FortLocator = "";		// COOP: the same fort for players who join the running game (NetServer_AddEnvironmentData)
	NetServer.FortRel = 0;		// COOP: relation of the fort of Bot+ (NET_BOTREL_*), 0 = the fort of the game mode
	NetServer.FortGone = false;		// COOP: the fort of Bot+ was removed in this game

	if (NetServer.GameType != NETGAME_FORT || iIslandIndex < 0)
	{
		NMAddByte(iSMsg, false);		// no fort
		return;
	}

	bool bSuccess = false;
	ref rIsland = Net_GetIslandByIndex(iIslandIndex);
	aref arReload; makearef(arReload, rIsland.reload);
	int iNumReloads = GetAttributesNum(arReload);
	for (int i=0; i<iNumReloads; i++)
	{
		aref arLocator = GetAttributeN(arReload, i);
		ReloadProgressUpdate();
		if (CheckAttribute(arLocator, "fort"))
		{
			if (!CheckAttribute(arLocator, "fort.model") || arLocator.fort.model == "")
			{
				Trace("Island.id = " + rIsland.id + ", and reload." + GetAttributeName(arLocator) + ", doesn't have fort.model value.");
				continue;
			}

			NetServer_FortBuild(rIsland, arLocator, 0, 0.0, 0.0);

			bSuccess = true;
			NMAddByte(iSMsg, true);
			NMAddString(iSMsg, GetAttributeName(arLocator), 16);
			NMAddString(iSMsg, rIsland.id, 24);
			NMAddFloat(iSMsg, 0.0);		// the shift of the fort (the fort of the game mode stands on its locator)
			NMAddFloat(iSMsg, 0.0);
			NetServer.FortLocator = GetAttributeName(arLocator);
			NetServer.FortIsland = rIsland.id;
			NetServer.FortDX = 0.0;
			NetServer.FortDZ = 0.0;
			NetServer.FortX = stf(arLocator.x);
			NetServer.FortZ = stf(arLocator.z);

			break;
		}
	}

	if (!bSuccess)
	{
		NMAddByte(iSMsg, false);		// no fort
	}
}

// COOP: the fort entity of the server on the locator of the island (the old body of NetServer_CreateFortEnvironment).
// iFriendlyTeam: the fort fires at every ship whose Team differs from it (0 = the fort game: the fort defends team 0).
void NetServer_FortBuild(ref rIsland, aref arLocator, int iFriendlyTeam, float fDX, float fDZ)
{
	float fFortDY = 0.0;
	{
		{
			ref rFort = NetServer_GetCLient(NET_FORTCLIENT);

			rFort.Server = true;
			CreateEntity(rFort, "NetFort");
			LayerAddObject("net_execute", rFort, 1);

			string sNSFortModel = rIsland.filespath.models + "\" + arLocator.fort.model;
			bool bReuseModel = false;
			if (IsEntity(&NSFortModel))
			{
				if (CheckAttribute(&NSFortModel, "CoopPath"))
				{
					if (NSFortModel.CoopPath == sNSFortModel) { bReuseModel = true; }
				}
			}
			if (bReuseModel)
			{
				// COOP s27: the model of the removed fort (hidden under the sea, NetServer_FortRemove) comes back: the island knows it
				LayerAddObject("fort_cannon_trace", &NSFortModel, 1);
				SendMessage(&NSFortModel, "lffffffffffff", MSG_MODEL_SET_POSITION, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0);
				Trace("COOP: srv fort model used again");
			}
			else
			{
				if (IsEntity(&NSFortModel)) { DeleteClass(&NSFortModel); }		// another model (never in one game: one island)
				CreateEntity(&NSFortModel, "MODELR");
				NSFortModel.CoopPath = sNSFortModel;

				SetTexturePath(0, rIsland.filespath.models + "\");

				SendMessage(&NSFortModel, "ls", MSG_MODEL_SET_LIGHT_PATH, NetServer_WhrGetLightingPath());
				SendMessage(&NSFortModel, "ls", MSG_MODEL_LOAD_GEO, sNSFortModel);
				SendMessage(&Island, "li", MSG_ISLAND_ADD_FORT,  &NSFortModel);
				LayerAddObject("fort_cannon_trace", &NSFortModel, 1);

				SetTexturePath(0, "");
			}

			NSFortModel.Pos.x = arLocator.x;
			NSFortModel.Pos.z = arLocator.z;
			fFortDY = 0.0;
			if (fDX * fDX + fDZ * fDZ > 0.01)
			{
				// COOP: the fort of Bot+ is shifted from the locator it was taken from: the model here (its geometry is in the coordinates
				// of its island, so a shift is a translation of the model matrix), the cannons in net.dll (fort_dx / fort_dy / fort_dz)
				fFortDY = NET_FORT_DROP;
				if (!NET_FORT_TRACE_SHIFT)
				{
					NSFortModel.Pos.x = 0.0;
					NSFortModel.Pos.z = 0.0;
					SendMessage(&NSFortModel, "lffffffffffff", MSG_MODEL_SET_POSITION, fDX, fFortDY, fDZ, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0);
				}
			}

			rFort.MaxCannonDamageDistance = MIN_CANNON_DAMAGE_DISTANCE;		// in meters

			rFort.Fort.Cannons.Type.Cannon = NETCANNON_CANNON;
			rFort.Fort.Cannons.Type.Culverine = NETCANNON_CULVERINE;
			rFort.Fort.Cannons.Type.Mortar = NETCANNON_MORTAR;

			ref rShip = Net_GetShipByName("FORT");
			rFort.Ship.Type = Net_GetShipIndex("FORT");
			
			rFort.Ship.HP = 100.0;
			rFort.Ship.Crew.Quantity = sti(rShip.MaxCrew);
			rFort.Ship.Crew.MinRatio = 1.0;
			rFort.Ship.Crew.Morale = 70;
			rFort.Ship.Name = arLocator.fortname;

			rFort.Ship.Cannons.Charge.Type = 3;	// bombs

			rFort.FriendlyTeam = iFriendlyTeam;		// friendly team index

			// skills
			// COOP: the accuracy of the fort follows the difficulty of the bots (Create game screen, "Bots"): 0..3 -> accuracy 1 / 4 / 7 / 10.
			// The fort has no aim assist (OnBall.c), so the accuracy is its whole dispersion: 10 = the old fixed value (hard).
			int iFortSkill = 2;
			if (CheckAttribute(&NetServer, "BotSkill")) { iFortSkill = sti(NetServer.BotSkill); }
			rFort.Skills.Accuracy = NetServer_BotSkillValue(iFortSkill);
			Trace("COOP: fort accuracy " + rFort.Skills.Accuracy + " (bot difficulty " + iFortSkill + ")");
			rFort.Skills.Cannons = 10;
			rFort.Skills.Sailing = 5;
			rFort.Skills.Defence = 5;
			rFort.Skills.Repair = 5;

			// setup goods for fort
			for (int j=0; j<Net_GetGoodsNum(); j++)
			{
				ref rGood = Net_GetGoodByIndex(j);
				string sGood = rGood.Name;
				rFort.Ship.Goods.(sGood) = 10000;
			}
			rFort.Perks.Gunner = 0;
			rFort.Perks.GunnerCamicadze = 0;
			rFort.Perks.GunnerSniper = 0;
			rFort.Perks.BoardingMaster = 0;
			rFort.Perks.Musketeer = 0;
			rFort.Perks.BoardingDevil = 0;

			rFort.fort_dx = fDX;		// COOP: read by the patched net.dll when the cannons of the fort are created
			rFort.fort_dz = fDZ;
			rFort.fort_dy = fFortDY;
			// the rays of the balls are moved to the model, which stays where it was loaded (net.dll: fort_tx / fort_ty / fort_tz)
			rFort.fort_tx = 0.0;
			rFort.fort_ty = 0.0;
			rFort.fort_tz = 0.0;
			if (NET_FORT_TRACE_SHIFT && fDX * fDX + fDZ * fDZ > 0.01)
			{
				rFort.fort_tx = fDX;
				rFort.fort_ty = fFortDY;
				rFort.fort_tz = fDZ;
			}
			SendMessage(rFort, "laaii", MSG_NET_ADD_FORT, rIsland, arLocator, &NSFortModel, &NSFortBlots);

			//SendMessage(&Flag, "lil", MSG_FLAG_INIT, &NSFortModel, iNation); // install flag on fort

			SetEventHandler("NetServer_FortSelectCannonType", "NetServer_FortSelectCannonType", 0);
			SetEventHandler("NetServer_OnFortHit", "NetServer_OnFortHit", 0);
			SetEventHandler("NetServer_OnFortCannonDamage", "NetServer_OnFortCannonDamage", 0);

			// COOP: exact cannons / crew of the fort for the spyglass of every client
			rFort.CoopCannonsTotal = 0;		// unknown until the first cannon of the fort is hit (the engine tells the total only then)
			rFort.CoopCannonsDamaged = 0;
			iNetFortInfoGen++;
			SetEventHandler("NetServer_FortInfoTick", "NetServer_FortInfoTick", 0);
			SetEventHandler("NetServer_FortCollTick", "NetServer_FortCollTick", 0);
			PostEvent("NetServer_FortCollTick", 1000, "l", iNetFortInfoGen);
			PostEvent("NetServer_FortInfoTick", 3000, "l", iNetFortInfoGen);
		}
	}
}

// COOP: "Bot+" -> the last item of the ship list (Fort) -> a fort appears NET_FORT_SPAWN_DIST in front of the bow of the admin.
// The model and the cannons are taken from the first fort of this island, or, where it has none (the open sea), of any island.
// The model is shifted by Pos, the cannons by net.dll (rFort.fort_dx / fort_dz: patched cannon creation).
// Relation: enemy = the fort fires at the players (and at the allied bots); ally = the fort belongs to the team of the admin
// and fires at the enemy bots; neutral = the fort only stands.
int NetServer_FortTeamOf(int wAdmin, int iRel)
{
	if (iRel == NET_BOTREL_ENEMY) { return NetServer_BotTeamOf(NET_BOTREL_ENEMY); }
	if (iRel >= NET_BOTREL_TEAM0) { return iRel - NET_BOTREL_TEAM0; }		// COOP s28: the fort of a team
	return sti(NSClients[wAdmin].Team);
}

// net index of the island whose fort model is used: always the first island that has a fort, so the fort looks the same on every map
int NetServer_FortDonorIsland()
{
    int iNum = Net_GetIslandsNum();
    for (int j=0; j<iNum; j++)
    {
        ref rAny = Net_GetIslandByIndex(j);
        if (Net_IsIslandHaveForts(rAny)) { return j; }
    }
    return -1;
}

// the fort of Bot+ with the middle at fX, fZ; wAdmin = the admin whose team the fort joins as an ally
bool NetServer_FortPlace(int wAdmin, int iRel, float fX, float fZ)
{
    int iDonor = NetServer_FortDonorIsland();
    if (iDonor < 0) { return false; }

    ref rIsland = Net_GetIslandByIndex(iDonor);
    aref arReload; makearef(arReload, rIsland.reload);
    int iNumReloads = GetAttributesNum(arReload);
    for (int i=0; i<iNumReloads; i++)
    {
        aref arLocator = GetAttributeN(arReload, i);
        if (!CheckAttribute(arLocator, "fort")) { continue; }
        if (!CheckAttribute(arLocator, "fort.model") || arLocator.fort.model == "") { continue; }

        float fDX = fX - stf(arLocator.x);
        float fDZ = fZ - stf(arLocator.z);

        int iTeam = NetServer_FortTeamOf(wAdmin, iRel);
        NetServer_FortBuild(rIsland, arLocator, iTeam, fDX, fDZ);
        NetServer.FortLocator = GetAttributeName(arLocator);
        NetServer.FortIsland = rIsland.id;
        NetServer.FortDX = fDX;
        NetServer.FortDZ = fDZ;
        NetServer.FortX = fX;
        NetServer.FortZ = fZ;
        NetServer.FortRel = iRel;

        int iSMsg = NMCreate();
        NMAddByte(iSMsg, NC_FORT);
        NMAddByte(iSMsg, NSC_FORT_SPAWN);
        NMAddString(iSMsg, NetServer.FortLocator, 16);
        NMAddString(iSMsg, NetServer.FortIsland, 24);
        NMAddFloat(iSMsg, fDX);
        NMAddFloat(iSMsg, fDZ);
        NetServer_SendToInGame(iSMsg);
        NMDelete(iSMsg);

        Trace("COOP: fort (island '" + NetServer.FortIsland + "', locator '" + NetServer.FortLocator + "') at x=" + fX + " z=" + fZ + " shift " + fDX + "," + fDZ + ", relation " + iRel + ", FriendlyTeam " + iTeam);
        return true;
    }
    return false;
}

// the fort of Bot+ is removed everywhere (Delete / Destroy in the Bots table, or before it is built again)
void NetServer_FortRemove()
{
    if (!CheckAttribute(&NetServer, "FortLocator") || NetServer.FortLocator == "") { return; }

    NetServer_DeleteFortEnvironment();
    // COOP s27: the model is NOT deleted: the island keeps a pointer to it (MSG_ISLAND_ADD_FORT has no opposite) and the first
    // ball after the deletion crashed the game in the trace of the island. It is hidden deep under the sea and used again.
    if (IsEntity(&NSFortModel))
    {
        LayerDelObject("fort_cannon_trace", &NSFortModel);
        SendMessage(&NSFortModel, "lffffffffffff", MSG_MODEL_SET_POSITION, 0.0, -3000.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0);
    }
    NetServer.FortLocator = "";
    NetServer.FortRel = 0;
    NetServer.FortGone = true;

    int iSMsg = NMCreate();
    NMAddByte(iSMsg, NC_FORT);
    NMAddByte(iSMsg, NSC_FORT_REMOVE);
    NetServer_SendToInGame(iSMsg);
    NMDelete(iSMsg);
    Trace("COOP: fort removed");
}

// the field around the middle of every fort: a ship does not get closer than NET_FORT_COLL_R. net.dll adds the attribute
// Ship.Impulse (Speed = an acceleration, per second) to the movement of the ship; the patched net.dll reads Speed from Speed
// (the original read it from Rotate). A ship that moves towards the fort is slowed down in proportion to that speed, the middle
// pushes a ship out slowly.
void NetServer_FortClearPush()
{
	for (int j=0; j<NET_MAXCLIENTS; j++)
	{
		ref rS = NetServer_GetClient(j);
		if (!CheckAttribute(rS, "FortPush") || !sti(rS.FortPush)) { continue; }
		rS.FortPush = 0;
		rS.Ship.Impulse.Rotate.x = 0.0;
		rS.Ship.Impulse.Rotate.y = 0.0;
		rS.Ship.Impulse.Rotate.z = 0.0;
		rS.Ship.Impulse.Speed.x = 0.0;
		rS.Ship.Impulse.Speed.y = 0.0;
		rS.Ship.Impulse.Speed.z = 0.0;
	}
}

// COOP s27: the index of the bowsprit in the mast array of the engine (= the order of the mastN labels of the "geometry" group in the .gm of the hull, not the N of mastN); -1 = no bowsprit
// read from RESOURCE\MODELS\Ships\<name>\<name>.gm; Barque, Tartane, WarTartane and XebekVML have no bowsprit
int NetServer_BowspritIndex(ref rS)
{
	ref rBaseShip = Net_GetShipByIndex(sti(rS.Ship.Type));
	string sName = rBaseShip.Name;
	switch (sName)
	{
		case "Barque": return -1; break;
		case "Tartane": return -1; break;
		case "WarTartane": return -1; break;
		case "XebekVML": return -1; break;
		case "Brigantine": return 2; break;
		case "Brig": return 2; break;
		case "BrigQeen": return 2; break;
		case "BrigSW": return 2; break;
		case "Santisima": return 2; break;
		case "Caravel": return 3; break;
		case "Galeon_l": return 3; break;
		case "Corvette_quest": return 3; break;
		case "Lineship": return 3; break;
		case "Battleship": return 3; break;
		case "Corvette": return 4; break;
	}
	return 0;
}

bool NetServer_ShipHasBowsprit(ref rS)
{
	if (NetServer_BowspritIndex(rS) < 0) { return false; }
	return true;
}

// COOP: breaks the bowsprit of a ship (once): the same message that the engine mast damage sends; the clients drop the mast by MastFall
void NetServer_BreakBowsprit(ref rS, int iClientID, int iTouchType)
{
	if (NetServer_IsBot(iClientID) && NET_BOWSPRIT_BOTS == 0) { return; }
	if (sti(rS.Dead)) { return; }
	if (!NetServer_ShipHasBowsprit(rS)) { return; }
	int iBowIdx = NetServer_BowspritIndex(rS);
	string sBowMast = "mast" + iBowIdx;
	if (CheckAttribute(rS, "Ship.Masts." + sBowMast) && sti(rS.Ship.Masts.(sBowMast)) == 1) { return; }
	rS.Ship.Masts.(sBowMast) = 1;
	NetServer_ShipMastFallen(rS, iBowIdx);		// COOP s27: the repair brings it back (server\OnShip.c)

	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_SHIP);
	NMAddByte(iSMsg, NSC_SHIP_MAST_FALL);
	NMAddClientID(iSMsg, iClientID);
	NMAddClientID(iSMsg, DST_INVALID);
	NMAddByte(iSMsg, iTouchType);
	NMAddByte(iSMsg, iBowIdx);
	NetServer_SendMessage(DST_ALL, iSMsg, false);
	NMDelete(iSMsg);
	Trace("COOP: bowsprit of ship " + iClientID + " (" + sBowMast + ") broke (touch type " + iTouchType + ")");
}

// COOP: ship to ship: the touch point (x, z) is in front of the bow of rS (a cone round the course) and at the end of the hull: the bowsprit pokes into the other ship
void NetServer_BowspritShipTouch(ref rS, int iClientID, float x, float z, float fPower)
{
	float fDX = x - stf(rS.Ship.Pos.x);
	float fDZ = z - stf(rS.Ship.Pos.z);
	float fD = sqrt(fDX * fDX + fDZ * fDZ);
	if (fD < 1.0) { return; }
	float fHalfLen = 0.0;
	if (CheckAttribute(rS, "Ship.BoxSize.z")) { fHalfLen = stf(rS.Ship.BoxSize.z) / 2.0; }
	float fAy = stf(rS.Ship.Ang.y);
	float fCos = (fDX * sin(fAy) + fDZ * cos(fAy)) / fD;

	// debug (not more than twice a second for a ship): why a touch did or did not break the bowsprit
	int iLastDbg = -100000;
	if (CheckAttribute(rS, "BowDbgT")) { iLastDbg = sti(rS.BowDbgT); }
	if (iServerTime - iLastDbg >= 500)
	{
		rS.BowDbgT = iServerTime;
		Trace("COOP: ship touch: ship " + iClientID + ", power " + fPower + ", distance " + fD + " (half length " + fHalfLen + "), bow cos " + fCos);
	}

	if (fPower < NET_SHIP_BOWSPRIT_MINPOWER) { return; }
	if (fD < fHalfLen * NET_SHIP_BOWSPRIT_MINDIST) { return; }
	if (fCos < NET_FORT_BOWSPRIT_COS) { return; }
	NetServer_BreakBowsprit(rS, iClientID, NETSHIP_MAST_TOUCH_SHIP);
}

void NetServer_FortCollTick()
{
	int iGen = GetEventData();
	if (iGen != iNetFortInfoGen) { return; }
	if (!CheckAttribute(&NetServer, "FortLocator") || NetServer.FortLocator == "") { return; }

	// the push is round the middle of the MODEL, which is not the locator: the locator is where net.dll puts the cannons from
	float fCX = stf(NetServer.FortX) + NET_FORT_COLL_OX;
	float fCZ = stf(NetServer.FortZ) + NET_FORT_COLL_OZ;
	for (int j=0; j<NET_MAXCLIENTS; j++)
	{
		if (!NetServer_HasShip(j)) { continue; }
		ref rS = NetServer_GetClient(j);
		float fPX = stf(rS.Ship.Pos.x);
		float fPZ = stf(rS.Ship.Pos.z);

		// speed of the ship from its two last positions
		float fVX = 0.0;
		float fVZ = 0.0;
		if (CheckAttribute(rS, "FortPT") && iServerTime > sti(rS.FortPT))
		{
			float fDT = (iServerTime - sti(rS.FortPT)) * 0.001;
			fVX = (fPX - stf(rS.FortPX)) / fDT;
			fVZ = (fPZ - stf(rS.FortPZ)) / fDT;
		}
		rS.FortPX = fPX;
		rS.FortPZ = fPZ;
		rS.FortPT = iServerTime;

		float fDX = fPX - fCX;
		float fDZ = fPZ - fCZ;
		float fD = sqrt(fDX * fDX + fDZ * fDZ);

		// the length of the hull: the field starts farther for a long ship (half of the length of this hull, Ship.BoxSize.z)
		float fHalfLen = 0.0;
		if (CheckAttribute(rS, "Ship.BoxSize.z")) { fHalfLen = stf(rS.Ship.BoxSize.z) / 2.0; }
		float fREff = NET_FORT_COLL_R + fHalfLen * NET_FORT_COLL_LEN_K;

		if (fD < fREff && !sti(rS.Dead))
		{
			if (fD < 1.0) { fDX = 1.0; fDZ = 0.0; fD = 1.0; }
			float fNX = fDX / fD;
			float fNZ = fDZ / fD;
			float fIn = 0.0 - (fVX * fNX + fVZ * fNZ);		// the speed towards the middle
			float fDepth = fREff - fD;                          // how far the ship went into the field
			float fA = NET_FORT_COLL_PUSH * fDepth / fREff + NET_FORT_COLL_SPRING * fDepth;
			if (fIn > 0.0) { fA = fA + NET_FORT_COLL_K * fIn; }
			if (fA > NET_FORT_COLL_MAX) { fA = NET_FORT_COLL_MAX; }

			// Ship.Impulse.Speed is added to the speed of the ship in ITS OWN axes (x sideways, z along the course), not in the
			// world axes: the wanted push (outwards, fNX/fNZ * fA) is taken along the bow. The sideways part is not used: a hull
			// does not slide sideways, and the sign of the sideways axis is not known. A ship that sails at the fort has the bow to it
			// (the push is against its speed), a ship that sails away from it has the bow from it (the push adds speed).
			float fAy = stf(rS.Ship.Ang.y);
			float fBow = fNX * sin(fAy) + fNZ * cos(fAy);		// the outward direction on the course of the ship

			// the bow is turned away from the fort, like by a shore: Ship.Impulse.Rotate.y, + turns the course to the right (ay grows)
			float fTurn = 0.0;
			if (fIn > 1.0 || fDepth > 5.0)
			{
				float fDelta = atan2(fNX * cos(fAy) - fNZ * sin(fAy), fBow);		// from the bow to the outward direction
				fTurn = fDelta / 1.5;
				if (fTurn > 1.0) { fTurn = 1.0; }
				if (fTurn < -1.0) { fTurn = -1.0; }
				fTurn = fTurn * NET_FORT_COLL_TURN;
			}

			rS.Ship.Impulse.Rotate.x = 0.0;
			rS.Ship.Impulse.Rotate.y = fTurn;
			rS.Ship.Impulse.Rotate.z = 0.0;
			rS.Ship.Impulse.Speed.x = 0.0;
			rS.Ship.Impulse.Speed.y = 0.0;
			rS.Ship.Impulse.Speed.z = fBow * fA;
			if (!CheckAttribute(rS, "FortPush") || !sti(rS.FortPush)) { Trace("COOP: fort field pushes ship " + j + " (distance " + fD + " of " + fREff + ", half length " + fHalfLen + ", speed towards the fort " + fIn + ", along the bow " + fBow * fA + ", turn " + fTurn + ")"); }
			rS.FortPush = 1;

			// a hit like an island: the sound / effect of the client at the touch point, and the damage of the hull
			if (fIn >= NET_FORT_HIT_MINSPEED)
			{
				int iLastHit = -100000;
				if (CheckAttribute(rS, "FortHitT")) { iLastHit = sti(rS.FortHitT); }
				if (iServerTime - iLastHit >= 800)
				{
					rS.FortHitT = iServerTime;

					int iSMsg = NMCreate();
					NMAddByte(iSMsg, NC_SHIP);
					NMAddByte(iSMsg, NSC_SHIP_ISLAND_COLLISION);
					NMAddFloat(iSMsg, fPX - fNX * fHalfLen);		// the bow of the ship, towards the fort
					NMAddFloat(iSMsg, stf(rS.Ship.Pos.y));
					NMAddFloat(iSMsg, fPZ - fNZ * fHalfLen);
					NetServer_SendMessage(DST_ALL, iSMsg, false);
					NMDelete(iSMsg);

					if (!NetServer_IsBot(j))
					{
						float fK = fIn / NET_FORT_HIT_REFSPEED;
						if (fK > 2.0) { fK = 2.0; }
						NetServer_ShipApplyHullHitpoints(rS, stf(rS.Ship.MaxHP) * NET_FORT_HIT_DMG * fK, NETKILL_BY_ISLAND, DST_INVALID);
					}
					Trace("COOP: fort hit ship " + j + ", speed " + fIn);

					// COOP: a hit with the BOW breaks the bowsprit, like in single when the bowsprit touches an island or a ship.
					// fBow < 0: the bow looks at the fort (fBow is the outward direction along the course); a side hit is not a bow hit
					if (fBow <= 0.0 - NET_FORT_BOWSPRIT_COS)
					{
						NetServer_BreakBowsprit(rS, j, NETSHIP_MAST_TOUCH_ISLAND);	// server checks: bot, dead, no bowsprit, already broken
					}
				}
			}
		}
		else
		{
			if (CheckAttribute(rS, "FortPush") && sti(rS.FortPush))
			{
				rS.FortPush = 0;
				rS.Ship.Impulse.Rotate.y = 0.0;
				rS.Ship.Impulse.Speed.x = 0.0;
				rS.Ship.Impulse.Speed.y = 0.0;
				rS.Ship.Impulse.Speed.z = 0.0;
			}
		}
	}

	PostEvent("NetServer_FortCollTick", 50, "l", iNetFortInfoGen);
}

void NetServer_FortSpawnAdmin(int wAdmin, int iRel)
{
    if (!NetServer_BotRelValid(iRel)) { iRel = NET_BOTREL_ENEMY; }        // like for the ships (COOP s28: a team too)

    if (CheckAttribute(&NetServer, "FortLocator") && NetServer.FortLocator != "") { NetServer_BotNote(wAdmin, NET_BOTNOTE_FORTEXISTS, 0); return; }
    if (CheckAttribute(&NetServer, "FortGone") && sti(NetServer.FortGone) && !bNetFortMove) { NetServer_BotNote(wAdmin, NET_BOTNOTE_FORTGONE, 0); return; }

    ref rA = NetServer_GetClient(wAdmin);
    float fAy = stf(rA.Ship.Ang.y);
    float fX = stf(rA.Ship.Pos.x) + sin(fAy) * NET_FORT_SPAWN_DIST;
    float fZ = stf(rA.Ship.Pos.z) + cos(fAy) * NET_FORT_SPAWN_DIST;

    if (!NetServer_FortPlace(wAdmin, iRel, fX, fZ)) { NetServer_BotNote(wAdmin, NET_BOTNOTE_NOFORT, 0); return; }
    NetServer_BotNote(wAdmin, NET_BOTNOTE_FORTADDED, iRel);
}

// COOP: the admin goes to the fort (for any fort of the game): beside it, on the side where he is now (the course is the
// tangent, so the broadside looks at the fort); fNetAdminFortDist metres from the centre of the fort
float fNetAdminFortDist = 350.0;

void NetServer_AdminTeleportToFort(int wAdmin)
{
	if (!NetServer_AdminShipOk(wAdmin)) { return; }
	if (!CheckAttribute(&NetServer, "FortLocator") || NetServer.FortLocator == "") { NetServer_BotNote(wAdmin, NET_BOTNOTE_NOFORT, 0); return; }

	ref rA = NetServer_GetClient(wAdmin);
	float fFX = stf(NetServer.FortX);
	float fFZ = stf(NetServer.FortZ);
	float fDX = stf(rA.Ship.Pos.x) - fFX;
	float fDZ = stf(rA.Ship.Pos.z) - fFZ;
	float fLen = sqrt(fDX * fDX + fDZ * fDZ);
	if (fLen < 1.0) { fDX = 0.0; fDZ = 1.0; fLen = 1.0; }

	float fX = fFX + fDX / fLen * fNetAdminFortDist;
	float fZ = fFZ + fDZ / fLen * fNetAdminFortDist;
	float fAy = atan2(fDX, fDZ) + PId2;
	NetServer_AdminPlaceShip(wAdmin, fX, fZ, fAy);
}

// COOP: "Bring" and "Heal" of the Bots table build the fort again (NetServer_FortRemove + NetServer_FortPlace): the cannons are
// whole again, the crew too. In a test the game crashed right after the first volley of a fort that was moved by building the
// NetFort entity again, so it is OFF until it is tried on a build: set true to try.
float fNetFortToAdminDist = 450.0;

// the fort of Bot+ in the "Bots" table of the admin: the same buttons as for the bots
void NetServer_AdminFortAction(int wAdmin, int iAction, float fValue)
{
	if (iAction == NET_ADMIN_BOT_GOTO) { NetServer_AdminTeleportToFort(wAdmin); return; }        // any fort of the game

	if (!CheckAttribute(&NetServer, "FortRel") || sti(NetServer.FortRel) <= 0)
	{
		// the fort of the game mode stays where it is
		if (iAction == NET_ADMIN_BOT_BRING) { NetServer_BotNote(wAdmin, NET_BOTNOTE_FORTFIXED, 0); }
		return;
	}
	if (!CheckAttribute(&NetServer, "FortLocator") || NetServer.FortLocator == "") { return; }
	int iRel = sti(NetServer.FortRel);

	if (iAction == NET_ADMIN_BOT_REL)
	{
		iRel = makeint(fValue + 0.5);
		if (!NetServer_BotRelValid(iRel)) { return; }		// COOP s28: a team too

		ref rFort = NetServer_GetCLient(NET_FORTCLIENT);
		rFort.FriendlyTeam = NetServer_FortTeamOf(wAdmin, iRel);
		NetServer.FortRel = iRel;
		Trace("COOP: fort relation " + iRel + ", FriendlyTeam " + rFort.FriendlyTeam);
		return;
	}

	if (iAction == NET_ADMIN_BOT_DELETE || iAction == NET_ADMIN_BOT_DESTROY) { NetServer_FortRemove(); return; }

	if (iAction == NET_ADMIN_BOT_HEAL)
	{
		if (!bNetFortMove) { NetServer_BotNote(wAdmin, NET_BOTNOTE_FORTMOVEOFF, 0); return; }
		// built again on the same place
		float fHX = stf(NetServer.FortX);
		float fHZ = stf(NetServer.FortZ);
		NetServer_FortRemove();
		NetServer_FortPlace(wAdmin, iRel, fHX, fHZ);
		return;
	}

	if (iAction == NET_ADMIN_BOT_BRING)
	{
		if (!bNetFortMove) { NetServer_BotNote(wAdmin, NET_BOTNOTE_FORTMOVEOFF, 0); return; }
		if (!NetServer_AdminShipOk(wAdmin)) { return; }

		// the fort comes beside the admin, on the side where it is now, fNetFortToAdminDist metres from his ship
		ref rA = NetServer_GetClient(wAdmin);
		float fAy = stf(rA.Ship.Ang.y);
		float fRX = cos(fAy);
		float fRZ = 0.0 - sin(fAy);
		float fSide = 1.0;
		float fDot = (stf(NetServer.FortX) - stf(rA.Ship.Pos.x)) * fRX + (stf(NetServer.FortZ) - stf(rA.Ship.Pos.z)) * fRZ;
		if (fDot < 0.0) { fSide = -1.0; }
		float fX = stf(rA.Ship.Pos.x) + fRX * fSide * fNetFortToAdminDist;
		float fZ = stf(rA.Ship.Pos.z) + fRZ * fSide * fNetFortToAdminDist;
		NetServer_FortRemove();
		NetServer_FortPlace(wAdmin, iRel, fX, fZ);
		return;
	}
}

// COOP: fort state for the spyglass of the clients (every 2 s, also reaches the players who joined later)
void NetServer_FortInfoTick()
{
	int iGen = GetEventData();
	if (iGen != iNetFortInfoGen) { return; }

	ref rFort = NetServer_GetCLient(NET_FORTCLIENT);

	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_FORT);
	NMAddByte(iSMsg, NSC_FORT_INFO);
	NMAddWord(iSMsg, sti(rFort.CoopCannonsTotal));
	NMAddWord(iSMsg, sti(rFort.CoopCannonsDamaged));
	NMAddWord(iSMsg, sti(rFort.Ship.Crew.Quantity));
	NMAddByte(iSMsg, sti(NetServer.FortRel));		// COOP: 0 = the fort of the game mode, else NET_BOTREL_* (the Bots table)
	NetServer_SendMessage(DST_ALL, iSMsg, false);
	NMDelete(iSMsg);

	PostEvent("NetServer_FortInfoTick", 2000, "l", iNetFortInfoGen);
}

float NetServer_FortSelectCannonType()
{
	ref rFort = NetServer_GetCLient(NET_FORTCLIENT);

	rFort.Ship.Cannons.Type = GetEventData();

	NetServer_CannonRecalculateParameters(rFort);
	NetServer_CalculateFireTime(rFort);
	NetServer_CalculateReloadTime(rFort);

	return stf(rFort.Ship.Cannons.SpeedV0);
}

float NetServer_OnFortCannonDamage()
{
	int iCannonType = GetEventData();
	int iTotalCannons = GetEventData();
	int iDamagedCannons = GetEventData();
	int iBallClientID = GetEventData();
	float fDamage = GetEventData();
	float fDistanceRatio = GetEventData();
	float x = GetEventData();
	float y = GetEventData();
	float z = GetEventData();

	if (iBallClientID == NET_FORTCLIENT) { return fDamage; }

	ref rFort = NetServer_GetCLient(NET_FORTCLIENT);

	// COOP: for the spyglass (NSC_FORT_INFO)
	rFort.CoopCannonsTotal = iTotalCannons;
	rFort.CoopCannonsDamaged = iDamagedCannons;

	fDamage += 0.12 * fDistanceRatio;

	if (fDamage >= 1.0)
	{
		fDamage = 1.0;
		int iSMsg = NMCreate();
		NMAddByte(iSMsg, NC_FORT);
		NMAddByte(iSMsg, NSC_FORT_CANNON_DESTROY);
		NMAddFloat(iSMsg, x);
		NMAddFloat(iSMsg, y);
		NMAddFloat(iSMsg, z);
		NetServer_SendMessage(DST_ALL, iSMsg, false);
		NMDelete(iSMsg);

		iDamagedCannons++;
		rFort.CoopCannonsDamaged = iDamagedCannons;
		
		rFort.Ship.HP = 100.0 * (1.0 - (iDamagedCannons / iTotalCannons));
		Trace("COOP: fort cannon destroyed, " + iDamagedCannons + " of " + iTotalCannons + ", fort HP " + rFort.Ship.HP);

		// check for fort defeat
		if (iDamagedCannons >= iTotalCannons / 2)
		{
			rFort.Ship.HP = -1.0;
		}
	}

	return fDamage;
}

void NetServer_OnFortHit()
{
	float x = GetEventData();
	float y = GetEventData();
	float z = GetEventData();

	// COOP: the event has no owner of the ball. A ball that has just left a cannon of the fort touches the model of the fort and
	// plays the hit effect (barrels, men) at the muzzle instead of the smoke: no hit effect right after a shot of the fort
	// (the damage of the cannons is another event and is not touched)
	// COOP s27: the message is ALWAYS sent (before, nothing was sent then and the ball of the client flew on through the fort),
	// and with the byte "silent" that the client reads (it was missing): 1 = only delete the ball, 0 = the hit effect
	int iSilent = 0;
	if (iServerTime - iNetFortLastShot < 150) { iSilent = 1; }
	// COOP s27: a ball of the fort itself (by its index, whatever the time): no effect
	string sFortBallSlot = "s" + (iServerCurrentBallIndex - (iServerCurrentBallIndex / 64) * 64);
	if (CheckAttribute(&NSFortBalls, sFortBallSlot))
	{
		if (sti(NSFortBalls.(sFortBallSlot)) == iServerCurrentBallIndex) { iSilent = 1; }
	}

	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_BALL);
	NMAddByte(iSMsg, NSC_BALL_HIT_FORT);
	NMAddWord(iSMsg, iServerCurrentBallIndex);
	NMAddFloat(iSMsg, x);
	NMAddFloat(iSMsg, y);
	NMAddFloat(iSMsg, z);
	NMAddByte(iSMsg, iSilent);
	NetServer_SendMessage(DST_ALL, iSMsg, false);
	NMDelete(iSMsg);
}