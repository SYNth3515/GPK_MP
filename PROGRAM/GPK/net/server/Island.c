object NSIsland;

void NetServer_DeleteIslandEnvironment()
{
	DelEventHandler("NetServer_CreateIslandDelayed", "NetServer_CreateIslandDelayed");
	DeleteClass(&NSIsland);
}

// COOP switch: 1 = create the server island at once (old behaviour)
//              2 = create it 4 s after the ships
//              3 = create it when every server ship has a normal height (checked every 250 ms, at most 15 s).
// A NetShip starts with a garbage height (1e28 .. 1e30 or NaN) that dies out in about 3-4 s; an island created at that
// moment froze the game on the host just like it did on the clients.
int iCoopServerIsland = 3;
int iCoopSrvIslandTries = 0;

// COOP: true when at least one ship exists and every server ship has a finite, small height
bool COOP_SrvShipsSane()
{
	int i;
	int iShips = 0;
	for (i=0; i<NET_MAXCLIENTS; i++)
	{
		if (!sti(NSClients[i].Use)) { continue; }
		if (!CheckAttribute(&NSClients[i], "Ship.Pos.y")) { return false; }
		ref rC = NetServer_GetClient(i);
		if (COOP_SrvYBad(rC)) { return false; }
		float fY = stf(rC.Ship.Pos.y);
		if (fY > 50.0 || fY < -50.0) { return false; }
		iShips = iShips + 1;
	}
	return (iShips > 0);
}

void NetServer_CreateIslandDelayed()
{
	iCoopSrvIslandTries = iCoopSrvIslandTries + 1;
	if (iCoopServerIsland == 3 && iCoopSrvIslandTries < 60 && !COOP_SrvShipsSane())
	{
		PostEvent("NetServer_CreateIslandDelayed", 250);
		return;
	}
	DelEventHandler("NetServer_CreateIslandDelayed", "NetServer_CreateIslandDelayed");
	Trace("COOP: srv island delayed create now, polls=" + iCoopSrvIslandTries);
	NetServer_CreateIslandReal();
}

void NetServer_CreateIslandEnvironment(int iSMsg)
{
	if (NetServer.IslandID == "") { return; }
	if (iCoopServerIsland == 2 || iCoopServerIsland == 3)
	{
		Trace("COOP: srv island postponed, mode " + iCoopServerIsland);
		iCoopSrvIslandTries = 0;
		SetEventHandler("NetServer_CreateIslandDelayed", "NetServer_CreateIslandDelayed", 0);
		if (iCoopServerIsland == 2) { PostEvent("NetServer_CreateIslandDelayed", 4000); }
		else { PostEvent("NetServer_CreateIslandDelayed", 1000); }
		return;
	}
	NetServer_CreateIslandReal();
}

void NetServer_CreateIslandReal()
{
	if (NetServer.IslandID == "") { return; }
	int iIslandIndex = Net_GetIslandIndex(NetServer.IslandID);
	if (iIslandIndex != -1)
	{
		NSIsland.Server = true;
		CreateEntity(&NSIsland, "NetIsland");
		NSIsland.LightingPath = NetServer_WhrGetLightingPath();
		NSIsland.ImmersionDistance = Islands[iIslandIndex].ImmersionDistance;			// distance = fRadius * ImmersionDistance, from island begin immersion
		NSIsland.ImmersionDepth = Islands[iIslandIndex].ImmersionDepth;			// immersion depth = (Distance2Camera / (fRadius * ImmersionDistance) - 1.0) * ImmersionDepth
		string sTexturePath = "islands\" + Islands[iIslandIndex].TexturePath + "\";
		SetTexturePath(0, sTexturePath);

		SendMessage(&NSIsland, "lsss", MSG_ISLAND_LOAD_GEO, "islands", Islands[iIslandIndex].filespath.models, Islands[iIslandIndex].model);
		LayerAddObject("mast_island_trace", &NSIsland, 1);

		//Fort_Login(iIslandIndex);	
		SetTexturePath(0, "");
	}

}