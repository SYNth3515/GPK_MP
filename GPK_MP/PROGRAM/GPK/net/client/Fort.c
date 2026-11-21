object NCFortModel, NCFortBlots;

void NetClient_DeleteFortEnvironment()
{
	DeleteClass(NetClient_GetCLient(NET_FORTCLIENT));
	DeleteClass(&NCFortModel);
	DeleteClass(&NCFortBlots);
}

// COOP s27: the fort is removed during the game. The model must NOT be deleted: the island keeps a pointer to it
// (MSG_ISLAND_ADD_FORT, there is no message to take it back) and the first ball after the deletion crashed the game in the
// trace of the island. So the model is hidden deep under the sea, and the next fort uses it again.
void NetClient_FortRemoveKeepModel()
{
	DeleteClass(NetClient_GetCLient(NET_FORTCLIENT));
	DeleteClass(&NCFortBlots);
	if (IsEntity(&NCFortModel))
	{
		LayerDelObject("fort_cannon_trace", &NCFortModel);
		SendMessage(&NCFortModel, "lffffffffffff", MSG_MODEL_SET_POSITION, 0.0, -3000.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0);
		Trace("COOP: cli fort model hidden (kept for the island)");
	}
}

void NetClient_CreateFortEnvironment(int iMsg)
{
	bool bFortLoad = NMGetByte(iMsg);
	if (!bFortLoad) { return; }

	NetClient_OnFortSpawn(iMsg);
}

// COOP: the fort of the game mode (NetClient_CreateFortEnvironment), the fort of a player who joins a running game and the fort
// that the admin adds in a running game (NSC_FORT_SPAWN) are all described by the same data:
// locator and island of the fort model, shift of the fort from that locator (0, 0 = it stands on the locator)
void NetClient_OnFortSpawn(int iMsg)
{
	string sLoc = NMGetString(iMsg);
	string sIsl = NMGetString(iMsg);
	float fDX = NMGetFloat(iMsg);
	float fDZ = NMGetFloat(iMsg);
	NetClient_FortCreateByLocator(sLoc, sIsl, fDX, fDZ);
}

void NetClient_FortCreateByLocator(string sLoc, string sIsl, float fDX, float fDZ)
{
	int iNum = Net_GetIslandsNum();
	int iFound = -1;
	for (int i=0; i<iNum; i++)
	{
		ref rTest = Net_GetIslandByIndex(i);
		if (rTest.id == sIsl) { iFound = i; break; }
	}
	if (iFound < 0) { return; }

	ref rIsland = Net_GetIslandByIndex(iFound);
	aref arReload; makearef(arReload, rIsland.reload);

	if (!CheckAttribute(arReload, sLoc)) { return; }
	aref arLocator; makearef(arLocator, arReload.(sLoc));
	if (!CheckAttribute(arLocator, "fort.model") || arLocator.fort.model == "") { return; }

	ref rFort = NetClient_GetClient(NET_FORTCLIENT);
	if (IsEntity(rFort)) { NetClient_FortRemoveKeepModel(); }		// COOP: built again (Heal / Bring of the Bots table): the old one goes (s27: the model stays)

	rFort.Server = false;
	rFort.fort_dx = fDX;		// COOP: read by the patched net.dll when the cannons of the fort are created
	rFort.fort_dz = fDZ;
	float fDY = 0.0;
	if (fDX * fDX + fDZ * fDZ > 0.01) { fDY = NET_FORT_DROP; }
	rFort.fort_dy = fDY;
	rFort.fort_tx = 0.0;		// COOP: the rays of the balls are moved on the server only
	rFort.fort_ty = 0.0;
	rFort.fort_tz = 0.0;
	CreateEntity(rFort, "NetFort");
	LayerAddObject("net_realize", rFort, 10000);
	LayerAddObject("sun_trace", rFort, 1);

	// create fort model
	string sNCFortModel = rIsland.filespath.models + "\" + arLocator.fort.model;
	bool bReuseModel = false;
	if (IsEntity(&NCFortModel))
	{
		if (CheckAttribute(&NCFortModel, "CoopPath"))
		{
			if (NCFortModel.CoopPath == sNCFortModel) { bReuseModel = true; }
		}
	}
	if (bReuseModel)
	{
		// COOP s27: the model of the removed fort (hidden under the sea) comes back: the island knows it already
		LayerAddObject("fort_cannon_trace", &NCFortModel, 1);
		SendMessage(&NCFortModel, "lffffffffffff", MSG_MODEL_SET_POSITION, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0);
		Trace("COOP: cli fort model used again");
	}
	else
	{
		if (IsEntity(&NCFortModel)) { DeleteClass(&NCFortModel); }		// another model (never in one game: one island)
		CreateEntity(&NCFortModel, "MODELR");
		NCFortModel.CoopPath = sNCFortModel;

		SetTexturePath(0, rIsland.filespath.models + "\");

		SendMessage(&NCFortModel, "ls", MSG_MODEL_SET_LIGHT_PATH, NetClient_WhrGetLightingPath());
		SendMessage(&NCFortModel, "ls", MSG_MODEL_LOAD_GEO, sNCFortModel);
		SendMessage(&Island, "li", MSG_ISLAND_ADD_FORT,  &NCFortModel);
		LayerAddObject("fort_cannon_trace", &NCFortModel, 1);

		SetTexturePath(0, "");
	}

	if (fDX * fDX + fDZ * fDZ > 0.01)
	{
		// the model is authored in the coordinates of its island: the shift moves it where the cannons are (net.dll shifts them)
		NCFortModel.Pos.x = 0.0;
		NCFortModel.Pos.z = 0.0;
		SendMessage(&NCFortModel, "lffffffffffff", MSG_MODEL_SET_POSITION, fDX, fDY, fDZ, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0);
	}

	// create fort blot
	/*CreateEntity(&NCFortBlots, "blots");
	SendMessage(&NCFortBlots, "lia", MSG_BLOTS_SETMODEL, &NCFortModel, NCFort);
	LayerAddObject("net_execute", &NCFortBlots, 10001);
	LayerAddObject("net_realize", &NCFortBlots, 10001);*/

	SendMessage(rFort, "laaii", MSG_NET_ADD_FORT, rIsland, arLocator, &NCFortModel, &NCFortBlots);

	// COOP: data of the fort for the spyglass (own attributes only, the NetFort entity does not read them)
	rFort.SpyGlass.Name = "";
	if (CheckAttribute(arLocator, "fortname")) { rFort.SpyGlass.Name = arLocator.fortname; }
	rFort.SpyGlass.x = stf(arLocator.x) + fDX;
	rFort.SpyGlass.z = stf(arLocator.z) + fDZ;
	rFort.SpyGlass.CannonsTotal = 0;		// 0 = not known yet, the spyglass hides the cannons
	rFort.SpyGlass.CannonsDamaged = 0;
	rFort.SpyGlass.Crew = -1;                // -1 = not received yet
	Trace("COOP: cli spyglass fort '" + rFort.SpyGlass.Name + "' x=" + rFort.SpyGlass.x + " z=" + rFort.SpyGlass.z + " shift " + fDX + "," + fDZ);
}

void NetClient_OnFortCannonDestroy(int iMsg)
{
	float x = NMGetFloat(iMsg);
	float y = NMGetFloat(iMsg);
	float z = NMGetFloat(iMsg);

	//string sFortFireParticle = "FortFire";
	//if (rand(2) == 1) { sFortFireParticle = "ShipFire"; }
	//CreateParticleSystem(sFortFireParticle, x, y, z, NetClient_WhrGetWindSpeed() / 48.0, NetClient_WhrGetWindAngle(), 0.0, 0);

	//CreateParticleSystem("fort_fire" + rand(2), x, y, z, NetClient_WhrGetWindSpeed() / 48.0, NetClient_WhrGetWindAngle(), 0.0, 0);

	string sFortFireParticle = "FortFire_net1";
	int iRand = rand(6);
	if (iRand == 3) { sFortFireParticle = "FortFire_net0"; }
	if (iRand == 2) { sFortFireParticle = "FortFire_net2"; }
	CreateParticleSystem(sFortFireParticle, x, y, z, NetClient_WhrGetWindSpeed() / 48.0, NetClient_WhrGetWindAngle(), 0.0, 0);
}

// COOP: NSC_FORT_INFO, see server\Fort.c
void NetClient_OnFortInfo(int iMsg)
{
	int iTotal = NMGetWord(iMsg);
	int iDamaged = NMGetWord(iMsg);
	int iCrew = NMGetWord(iMsg);
	int iRel = NMGetByte(iMsg);

	ref rFort = NetClient_GetClient(NET_FORTCLIENT);
	rFort.CoopRel = iRel;		// COOP: for the Bots table (0 = the fort of the game mode)
	rFort.SpyGlass.CannonsTotal = iTotal;
	rFort.SpyGlass.CannonsDamaged = iDamaged;
	rFort.SpyGlass.Crew = iCrew;
}

// COOP: the fort of Bot+ was removed by the admin (Delete / Destroy of the Bots table)
void NetClient_OnFortRemove()
{
	NetClient_FortRemoveKeepModel();		// COOP s27: the model stays (hidden), see NetClient_FortRemoveKeepModel
	ref rFortR = NetClient_GetClient(NET_FORTCLIENT);
	rFortR.CoopRel = 0;
}

void NetClient_OnFort(int iMsg)
{
	int iSubCode = NMGetByte(iMsg);

	switch (iSubCode)
	{
		case NSC_FORT_CANNON_DESTROY:		
			NetClient_OnFortCannonDestroy(iMsg); 		
		break;

		case NSC_FORT_INFO:
			NetClient_OnFortInfo(iMsg);
		break;

		case NSC_FORT_REMOVE:
			NetClient_OnFortRemove();
		break;

		case NSC_FORT_SPAWN:
			NetClient_OnFortSpawn(iMsg);
		break;
	}
}