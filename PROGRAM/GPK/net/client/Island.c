void NetClient_DeleteIslandEnvironment()
{
	// COOP: the game may end while the island is still waiting for sane ship height; do not leave the handler behind
	DelEventHandler("NetClient_CreateIslandDelayed", "NetClient_CreateIslandDelayed");
	DeleteClass(&NCIsland);
	DeleteClass(&NCCoastFoam);
}

// COOP test switch: 0 = no island on client (test), 1 = normal, 2 = create island 4 s after ships
int iCoopClientIsland = 2;

int iCoopIslandTries = 0;

// COOP: true when at least one ship exists and every existing ship has a normal (finite, small) height
bool COOP_CliShipsSane()
{
	int i;
	int iShips = 0;
	for (i=0; i<NET_TOTALCLIENTS; i++)
	{
		if (!COOP_CliSlotLive(i)) { continue; }
		if (!sti(NCClients[i].Use)) { continue; }
		if (sti(NCClients[i].Ship.Type) < 0) { continue; }
		iShips = iShips + 1;
		// COOP: a ship that is not created yet (see NetClient_ShipCreateLater) counts as not ready
		if (!CheckAttribute(&NCClients[i], "CoopReady")) { return false; }
		if (!sti(NCClients[i].CoopReady)) { return false; }
		ref rC = NetClient_GetClient(i);
		if (COOP_CliYBad(rC)) { return false; }
		float fY = stf(rC.Ship.Pos.y);
		if (fY > 50.0 || fY < -50.0) { return false; }
	}
	return (iShips > 0);
}

void NetClient_CreateIslandDelayed()
{
	iCoopIslandTries = iCoopIslandTries + 1;
	// poll every 250 ms (up to 15 s) until ship height is sane, then create the island
	if (iCoopIslandTries < 60 && !COOP_CliShipsSane())
	{
		PostEvent("NetClient_CreateIslandDelayed", 250);
		return;
	}
	DelEventHandler("NetClient_CreateIslandDelayed", "NetClient_CreateIslandDelayed");
	Trace("COOP: cli island delayed create now, polls=" + iCoopIslandTries);
	NetClient_CreateIslandReal();
}

void NetClient_CreateIslandEnvironment(int iMsg)
{
	if (iCoopClientIsland == 0) { Trace("COOP: cli island SKIPPED (test)"); return; }
	if (iCoopClientIsland == 2)
	{
		Trace("COOP: cli island postponed until ship height is sane");
		iCoopIslandTries = 0;
		SetEventHandler("NetClient_CreateIslandDelayed", "NetClient_CreateIslandDelayed", 0);
		PostEvent("NetClient_CreateIslandDelayed", 1000);
		return;
	}
	NetClient_CreateIslandReal();
}

void NetClient_CreateIslandReal()
{
	string sIslandID = NCServer.IslandID;
	if (sIslandID == "") { return; }

	int iIslandIndex = Net_GetIslandIndex(sIslandID);
	if (iIslandIndex == -1) { return; }

	NCIsland.Server = false;
	CreateEntity(&NCIsland, "NetIsland");
	NCIsland.LightingPath = NetClient_WhrGetLightingPath();
	NCIsland.ImmersionDistance = Islands[iIslandIndex].ImmersionDistance;	// distance = fRadius * ImmersionDistance, from island begin immersion
	NCIsland.ImmersionDepth = Islands[iIslandIndex].ImmersionDepth;			// immersion depth = (Distance2Camera / (fRadius * ImmersionDistance) - 1.0) * ImmersionDepth
	string sTexturePath = "islands\" + Islands[iIslandIndex].TexturePath + "\";
	SetTexturePath(0, sTexturePath);
	NCIsland.FogDensity = NCWeather.Fog.IslandDensity;

	SendMessage(&NCIsland, "lsss", MSG_ISLAND_LOAD_GEO, "islands", Islands[iIslandIndex].filespath.models, Islands[iIslandIndex].model);
	LayerAddObject("net_realize", &NCIsland, 3);
	LayerAddObject("sun_trace", &NCIsland, 1);

	SetTexturePath(0, "");

	NCCoastFoam.Id = sIslandID;
	NCCoastFoam.Server = false;
	CreateEntity(&NCCoastFoam, "NetCoastFoam");
	LayerAddObject("net_realize", &NCCoastFoam, 65534);
	LayerAddObject("net_execute", &NCCoastFoam, 65535);
}