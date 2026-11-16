object NSFortModel, NSFortBlots;
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

// hard fort: active perk ImmediateReload (reload time x0.5) like a captain uses it: Duration seconds in every Delay seconds
bool NetServer_FortBurstOn()
{
	if (!NetServer_FortHard()) { return false; }
	int iDelay = Net_GetActivePerkDelay(NET_APERK_IMMEDIATERELOAD) * 1000;
	int iDur = Net_GetActivePerkDuration(NET_APERK_IMMEDIATERELOAD) * 1000;
	int iPhase = iServerTime - (iServerTime / iDelay) * iDelay;
	return iPhase >= iDelay - iDur;
}

bool bNetFortMove = false;		// COOP: the fort of Bot+ may be built again after it was removed / moved (Bring, Heal, Add after Delete). OFF: in tests the game
									// crashed on the first volley of a NetFort entity that was built again. Set true to try it on a build.
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

			CreateEntity(&NSFortModel, "MODELR");
			string sNSFortModel = rIsland.filespath.models + "\" + arLocator.fort.model;

			SetTexturePath(0, rIsland.filespath.models + "\");

			SendMessage(&NSFortModel, "ls", MSG_MODEL_SET_LIGHT_PATH, NetServer_WhrGetLightingPath());
			SendMessage(&NSFortModel, "ls", MSG_MODEL_LOAD_GEO, sNSFortModel);
			SendMessage(&Island, "li", MSG_ISLAND_ADD_FORT,  &NSFortModel);
			LayerAddObject("fort_cannon_trace", &NSFortModel, 1);

			SetTexturePath(0, "");

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
    if (IsEntity(&NSFortModel)) { DeleteClass(&NSFortModel); }
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

void NetServer_FortCollTick()
{
	int iGen = GetEventData();
	if (iGen != iNetFortInfoGen) { return; }
	if (!CheckAttribute(&NetServer, "FortLocator") || NetServer.FortLocator == "") { return; }

	float fCX = stf(NetServer.FortX);
	float fCZ = stf(NetServer.FortZ);
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

		if (fD < NET_FORT_COLL_R && !sti(rS.Dead))
		{
			if (fD < 1.0) { fDX = 1.0; fDZ = 0.0; fD = 1.0; }
			float fNX = fDX / fD;
			float fNZ = fDZ / fD;
			float fIn = 0.0 - (fVX * fNX + fVZ * fNZ);		// the speed towards the middle
			float fA = NET_FORT_COLL_PUSH * (1.0 - fD / NET_FORT_COLL_R);
			if (fIn > 0.0) { fA = fA + NET_FORT_COLL_K * fIn; }
			if (fA > NET_FORT_COLL_MAX) { fA = NET_FORT_COLL_MAX; }

			// Ship.Impulse.Speed is added to the speed of the ship in ITS OWN axes (x sideways, z along the course), not in the
			// world axes: the wanted push (outwards, fNX/fNZ * fA) is taken along the bow. The sideways part is not used: a hull
			// does not slide sideways, and the sign of the sideways axis is not known. A ship that sails at the fort has the bow to it
			// (the push is against its speed), a ship that sails away from it has the bow from it (the push adds speed).
			float fAy = stf(rS.Ship.Ang.y);
			float fBow = fNX * sin(fAy) + fNZ * cos(fAy);		// the outward direction on the course of the ship
			rS.Ship.Impulse.Rotate.x = 0.0;
			rS.Ship.Impulse.Rotate.y = 0.0;
			rS.Ship.Impulse.Rotate.z = 0.0;
			rS.Ship.Impulse.Speed.x = 0.0;
			rS.Ship.Impulse.Speed.y = 0.0;
			rS.Ship.Impulse.Speed.z = fBow * fA;
			if (!CheckAttribute(rS, "FortPush") || !sti(rS.FortPush)) { Trace("COOP: fort field pushes ship " + j + " (distance " + fD + ", speed towards the fort " + fIn + ", along the bow " + fBow * fA + ")"); }
			rS.FortPush = 1;
		}
		else
		{
			if (CheckAttribute(rS, "FortPush") && sti(rS.FortPush))
			{
				rS.FortPush = 0;
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
    if (iRel < NET_BOTREL_ALLY || iRel > NET_BOTREL_NEUTRAL) { iRel = NET_BOTREL_ENEMY; }        // like for the ships

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
		if (iRel < NET_BOTREL_ALLY || iRel > NET_BOTREL_NEUTRAL) { return; }

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
	if (iServerTime - iNetFortLastShot < 150) { return; }
	
	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_BALL);
	NMAddByte(iSMsg, NSC_BALL_HIT_FORT);
	NMAddWord(iSMsg, iServerCurrentBallIndex);
	NMAddFloat(iSMsg, x);
	NMAddFloat(iSMsg, y);
	NMAddFloat(iSMsg, z);
	NetServer_SendMessage(DST_ALL, iSMsg, false);
	NMDelete(iSMsg);
}