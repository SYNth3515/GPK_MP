void NetClient_DeleteIslandEnvironment()
{
	// COOP: the game may end while the island is still waiting for sane ship height; do not leave the handler behind
	DelEventHandler("NetClient_CreateIslandDelayed", "NetClient_CreateIslandDelayed");
	DeleteClass(&NCIsland);
	DeleteClass(&NCCoastFoam);

	// COOP: эффекты острова из одиночки
	if (isEntity(&IslandReflModel)) { DeleteClass(&IslandReflModel); }
	if (isEntity(&sLightModel)) { DeleteClass(&sLightModel); }
	if (isEntity(&lighthouseLightModel)) { DeleteClass(&lighthouseLightModel); }
	if (isEntity(&objGrass)) { DeleteClass(&objGrass); }
	DeleteEntitiesByType("Grass");

	// вернуть обработчик подсветки травы из одиночки
	if (iCoopGrassEventSwapped != 0)
	{
		DelEventHandler("GOpt_isGrassLightsOn", "NetClient_GrassLightsOff");
		SetEventHandler("GOpt_isGrassLightsOn", "EventReturn_isGrassLightsOn", 0);
		iCoopGrassEventSwapped = 0;
	}
}

// Трава (Location.dll) выбирает технику по событию GOpt_isGrassLightsOn: 1 = GrassEx (modulate2x, «светится», бледная), 0 = GrassEx_dark.
// В одиночке EventReturn_isGrassLightsOn (globals.c) в море (bSeaActive) всегда возвращает 0; в мультиплеере bSeaActive = false,
// и днём (часы 9..18 по игровому времени) возвращается 1, отсюда бледная растительность. Подменяем обработчик на время жизни острова.
int iCoopGrassEventSwapped = 0;
int NetClient_GrassLightsOff() { return 0; }

// COOP: переключатели эффектов острова (1 = включено, 0 = выключено), графика у каждого игрока своя, между игроками не синхронизируется
int iCoopFxIslandRefl = 1;	// отражение острова в воде (IslandReflModel, как Sea_LoadIsland в одиночке)
int iCoopGrassTest = 1;		// ТЕСТ бледной травы: 1 = как сейчас, 2 = ветер для травы обнулён, 3 = приоритет 4, 4 = через 5 секунд трава перестаёт получать Execute (как при паузе), остаётся только Realize

void NetClient_GrassFreeze()
{
	DelEventHandler("NetClient_GrassFreeze", "NetClient_GrassFreeze");
	if (isEntity(&objGrass)) { LayerDelObject("net_execute", &objGrass); }
	Trace("COOP: cli grass execute frozen");
}
int iCoopFxLighthouse = 1;	// свет маяка (sLightModel и lighthouseLightModel, как Sea_CreateLighthouse в одиночке)
int iCoopFxGrass = 1;		// спрайты джунглей (junglesprites); без них остров голый

// Вызывается из NetClient_CreateIslandReal, пока ещё действует SetTexturePath острова (как в одиночке)
void NetClient_CreateIslandEffects(int iIslandIndex, string sIslandID)
{
	// ---- трава
	int iHasJungle = 0;
	if (iCoopFxGrass != 0) { if (CheckAttribute(&Islands[iIslandIndex], "jungle")) { iHasJungle = 1; } }
	if (iHasJungle == 1)
	{
		Trace("COOP: cli island grass");
		float fJungleScale = 10.0;
		if (CheckAttribute(&Islands[iIslandIndex], "jungle.scale")) { fJungleScale = stf(Islands[iIslandIndex].jungle.scale); }
		if (iCoopGrassEventSwapped == 0)
		{
			DelEventHandler("GOpt_isGrassLightsOn", "EventReturn_isGrassLightsOn");
			SetEventHandler("GOpt_isGrassLightsOn", "NetClient_GrassLightsOff", 0);
			iCoopGrassEventSwapped = 1;
		}
		int iGrassPrio = 1000;
		if (iCoopGrassTest == 2) { Weather.Wind.Speed = 0.0; Weather.Wind.Angle = 0.0; }
		if (iCoopGrassTest == 3) { iGrassPrio = 4; }
		Trace("COOP: cli grass test=" + iCoopGrassTest + " prio=" + iGrassPrio + " islandFog=" + NCWeather.Fog.IslandDensity + " seaFog=" + NCWeather.Fog.Density);
		CreateEntity(&objGrass, "Grass");
		LayerAddObject("net_execute", &objGrass, 1000);
		LayerAddObject("net_realize", &objGrass, iGrassPrio);
		if (iCoopGrassTest == 4)
		{
			SetEventHandler("NetClient_GrassFreeze", "NetClient_GrassFreeze", 0);
			PostEvent("NetClient_GrassFreeze", 5000);
		}
		SendMessage(objGrass, "ls", 41666, "Grass\" + Islands[iIslandIndex].jungle.texture + ".tga");	// MSG_GRASS_SET_TEXTURE
		SendMessage(objGrass, "lffffff", 42666, fJungleScale, 20.0, 200.0, 100.0, 1000.0, 0.6);			// MSG_GRASS_SET_PARAM
		SendMessage(objGrass, "ls", 40666, "resource\models\islands\" + Islands[iIslandIndex].id + "\" + Islands[iIslandIndex].jungle.patch + ".grs");	// MSG_GRASS_LOAD_DATA
	}

	// ---- отражение острова в воде
	// NetSea (net.dll) рисует только слой sea_reflection, слоя sea_reflection2 из одиночки у него нет
	int iHasRefl = 0;
	if (iCoopFxIslandRefl != 0) { if (CheckAttribute(&Islands[iIslandIndex], "refl_model")) { iHasRefl = 1; } }
	if (iHasRefl == 1)
	{
		Trace("COOP: cli island reflection");
		CreateEntity(&IslandReflModel, "MODELR");
		string sReflModel = Islands[iIslandIndex].filespath.models + "\" + Islands[iIslandIndex].refl_model;
		SendMessage(&IslandReflModel, "ls", MSG_MODEL_SET_LIGHT_PATH, NetClient_WhrGetLightingPath());
		SendMessage(&IslandReflModel, "ls", MSG_MODEL_LOAD_GEO, sReflModel);
		SendMessage(&IslandReflModel, "lllf", MSG_MODEL_SET_FOG, 1, 1, stf(NCWeather.Fog.IslandDensity));
		LayerAddObject("sea_reflection", &IslandReflModel, 2);
	}

	// ---- свет маяка
	if (iCoopFxLighthouse != 0)
	{
		if (CheckAttribute(&Islands[iIslandIndex], "mayak"))
		{
			Trace("COOP: cli island mayak model");
			string sLhModel;
			if (NetClient_WhrIsDay()) { sLhModel = Islands[iIslandIndex].filespath.models + "\" + Islands[iIslandIndex].mayak.model_day; }
			else { sLhModel = Islands[iIslandIndex].filespath.models + "\" + Islands[iIslandIndex].mayak.model_night; }
			CreateEntity(&sLightModel, "MODELR");
			SendMessage(&sLightModel, "ls", MSG_MODEL_LOAD_GEO, sLhModel);
			SendMessage(&NCIsland, "li", MSG_ISLAND_ADD_FORT, &sLightModel);
		}

		// объёмный свет маяка есть только у Ямайки и Кубы, ночью
		int iLhIsland = 0;
		if (sIslandID == "Jamaica") { iLhIsland = 1; }
		if (sIslandID == "Cuba1") { iLhIsland = 1; }
		if (sIslandID == "Cuba2") { iLhIsland = 1; }
		int iLhNight = 0;
		if (NetClient_WhrIsNight()) { iLhNight = 1; }
		if (iLhNight == 1 && iLhIsland == 1)
		{
			Trace("COOP: cli island lighthouse light");
			CreateEntity(&lighthouseLightModel, "MODELR");
			SendMessage(&lighthouseLightModel, "ls", MSG_MODEL_LOAD_GEO, "lighthouse_volumeLight");
			SendMessage(&lighthouseLightModel, "ls", MSG_MODEL_SET_TECHNIQUE, "LighthouseLight");
			LayerAddObject("net_execute", &lighthouseLightModel, 65549);
			LayerAddObject("net_realize", &lighthouseLightModel, 65549);
			if (sIslandID == "Jamaica") { SendMessage(&lighthouseLightModel, "lffffffffffff", MSG_MODEL_SET_POSITION, 3000.0, 24.5, -1679.1, 1, 0, 0, 0, 1, 0, 0, 0, 1); }
			if (sIslandID == "Cuba1") { SendMessage(&lighthouseLightModel, "lffffffffffff", MSG_MODEL_SET_POSITION, 627.0, 24.0, -2170.5, 1, 0, 0, 0, 1, 0, 0, 0, 1); }
			if (sIslandID == "Cuba2") { SendMessage(&lighthouseLightModel, "lffffffffffff", MSG_MODEL_SET_POSITION, -413.0, 65.5, 815.1, 1, 0, 0, 0, 1, 0, 0, 0, 1); }
		}
	}
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

	NetClient_CreateIslandEffects(iIslandIndex, sIslandID);

	SetTexturePath(0, "");

	NCCoastFoam.Id = sIslandID;
	NCCoastFoam.Server = false;
	CreateEntity(&NCCoastFoam, "NetCoastFoam");
	LayerAddObject("net_realize", &NCCoastFoam, 65534);
	LayerAddObject("net_execute", &NCCoastFoam, 65535);
}