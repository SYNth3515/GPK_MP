object NCSea;

void NetClient_WhrDeleteSeaEnvironment()
{
	if (isEntity(&NCSea))
	{
		DeleteClass(&NCSea);
	}
	DeleteAttribute(&NCSea, "");
}
	
void NetClient_WhrCreateSeaEnvironment()
{
	aref	aCurWeather = NetClient_GetCurrentWeather();
	aref	aSea; makearef(aSea, aCurWeather.Sea);
	aref	aSea2; makearef(aSea2, aCurWeather.Sea2);
	aref	aCommon; makearef(aCommon, WhrCommonParams.Sea);
	
	DeleteAttribute(&NCSea, "");

	NCSea.Clear = "";

	NCSea.Server = false;
	NCSea.SimpleSea = sti(InterfaceStates.SimpleSea);	// must be before creating
	// COOP: the sea is NOT deleted when the admin changes the weather in the running game (ships, foam and rigging use it,
	// the single player game never deletes it either): only its data is replaced
	if (!isEntity(&NCSea)) { CreateEntity(&NCSea, "NetSea"); }

	NCSea.Transparency = Whr_GetFloat(aSea,"Transparency");
	NCSea.FrenelCoefficient = Whr_GetFloat(aSea,"FrenelCoefficient");
	NCSea.WaterReflection = Whr_GetFloat(aSea,"WaterReflection");
	NCSea.WaterAttenuation = Whr_GetFloat(aSea,"WaterAttenuation");

	NetClient_SeaDetailSet();
	NCSea.MaxVertices = MaxVertices;
	NCSea.MaxIndices = MaxIndices;
	NCSea.MaxWaveDistance = MaxWaveDistance;
	NCSea.MaxDim = 65536;
	NCSea.MinDim = 128;
	//NCSea.Sea2.GridStep = GridStepPC;

	NCSea.Lights = aCurWeather.Lights;

	NCSea.Water.Color = Whr_GetColor(aSea,"Water.Color");

	string sCurFog = NetClient_WhrGetCurrentFog();
	NCSea.Fog.Color = Whr_GetColor(aCurWeather, sCurFog + ".Color");
	NCSea.Fog.Enable = Whr_GetLong(aCurWeather, sCurFog + ".Enable");
	NCSea.Fog.Start = Whr_GetFloat(aCurWeather, sCurFog + ".Start");
	NCSea.Fog.Density = Whr_GetFloat(aCurWeather, sCurFog + ".Density");
	NCSea.Fog.SeaDensity = Whr_GetFloat(aCurWeather, sCurFog + ".SeaDensity");
	
	NCSea.Bump.Dir = Whr_GetString(aSea, "Bump.Dir");
	NCSea.Bump.Tile = Whr_GetFloat(aSea, "Bump.Tile");
	NCSea.Bump.Ang = Whr_GetFloat(aSea, "Bump.Ang");
	NCSea.Bump.Speed = Whr_GetFloat(aSea, "Bump.Speed");
	NCSea.Bump.Scale = Whr_GetFloat(aSea, "Bump.Scale");
	NCSea.Bump.AnimSpeed = Whr_GetFloat(aSea, "Bump.AnimSpeed");
	NCSea.Bump.MipLevels = 4;
	NCSea.Bump.Size = 128;

	// Sun road section (as in singleplayer WhrSea.c)
	NCSea.Road.Start = Whr_GetFloat(aSea, "SunRoad.Start");
	NCSea.Road.Color1 = Whr_GetColor(aSea, "SunRoad.Color1");
	NCSea.Road.Color2 = Whr_GetColor(aSea, "SunRoad.Color2");
	NCSea.Road.Power = Whr_GetFloat(aSea, "SunRoad.Power");

	NCSea.CubeMap.Size = 512;	
	NCSea.CubeMap.VectorsSize = 256;
	
	NCSea.CubeMap.Format = "r5g6b5";

	NCSea.Sky.Color = Whr_GetColor(aSea, "Sky.Color");

	// Advanced Sea initialize
	NCSea.Sea2.WaterColor = Whr_GetColor(aSea2, "WaterColor");
	NCSea.Sea2.SkyColor = Whr_GetColor(aSea2, "SkyColor");

	NCSea.Sea2.Amp1 = Whr_GetFloat(aSea2, "Amp1");
	NCSea.Sea2.AnimSpeed1 = Whr_GetFloat(aSea2, "AnimSpeed1");
	NCSea.Sea2.Scale1 = Whr_GetFloat(aSea2, "Scale1");
	NCSea.Sea2.MoveSpeed1 = Whr_GetString(aSea2, "MoveSpeed1");

	NCSea.Sea2.Amp2 = Whr_GetFloat(aSea2, "Amp2");
	NCSea.Sea2.AnimSpeed2 = Whr_GetFloat(aSea2, "AnimSpeed2");
	NCSea.Sea2.Scale2 = Whr_GetFloat(aSea2, "Scale2");
	NCSea.Sea2.MoveSpeed2 = Whr_GetString(aSea2, "MoveSpeed2");

	NCSea.Sea2.PosShift = Whr_GetFloat(aSea2, "PosShift");
	NCSea.Sea2.BumpScale = Whr_GetFloat(aSea2, "BumpScale");

	NCSea.Sea2.Reflection = Whr_GetFloat(aSea2, "Reflection");
	NCSea.Sea2.Transparency = Whr_GetFloat(aSea2, "Transparency");
	NCSea.Sea2.Attenuation = Whr_GetFloat(aSea2, "Attenuation");
	NCSea.Sea2.Frenel = Whr_GetFloat(aSea2, "Frenel");

	NCSea.MaxSeaHeight = 50.0;
	
	NCSea.isDone = "";
}

// COOP s30: the detail of the waves. The sea of the engine (the same in net.dll and sea.dll) is a grid of blocks fixed in
// the world: the cell of a block is GridStep * 2^lod, lod = log2(distance * sqrt(LodScale)), at least 4, and a block
// changes its lod at once (no smooth change): the tops of the waves jump there. The single player game has the same jumps.
// A smaller LodScale moves every change of the lod farther (x1.4 for each half) and makes the far cells smaller; each half
// doubles the vertices. The engine draws at most 65500 vertices: above that blocks are left out (holes, near the ship too:
// LodScale 0.125 with the old GridStep had them). The distance is from the CAMERA (with its height, ~30 m), so the water
// 20 m from the ship is ~70 m away and changes its lod already. The vertices go as 1 / (GridStep^2 * LodScale): 2 and 3 make
// the cells near the camera bigger to pay for the far changes of the lod (the same vertices as 1).
// 0 = as the single player open sea (LodScale 0.5): lod 4 (cell 16 * GridStep) to ~45 m, ~1x vertices
// 1 = LodScale 0.25: lod 4 to ~64 m, ~2x vertices (the default)
// 2 = LodScale 0.125, GridStep x1.41: lod 4 to ~90 m, ~2x vertices
// 3 = LodScale 0.0625, GridStep x2: lod 4 to ~128 m, ~2x vertices
// The chat command "/sea 0|1|2|3" changes it in the running game (net\Land.c).
int iNetSeaDetail = 1;

void NetClient_SeaDetailSet()
{
	float fLod = 0.25;
	float fGridK = 1.0;
	if (iNetSeaDetail <= 0) { fLod = 0.5; }
	if (iNetSeaDetail == 2) { fLod = 0.125; fGridK = 1.41; }
	if (iNetSeaDetail >= 3) { fLod = 0.0625; fGridK = 2.0; }
	NCSea.Sea2.LodScale = fLod;
	NCSea.Sea2.GridStep = fGridK * (0.07 + 0.1 * (1.0 - stf(InterfaceStates.SeaDetails)));
}

void NetClient_SeaDetailCmd(int iLevel)
{
	if (iLevel < 0) { iLevel = 0; }
	if (iLevel > 3) { iLevel = 3; }
	iNetSeaDetail = iLevel;
	if (isEntity(&NCSea)) { NetClient_SeaDetailSet(); }
	Log_SetStringToLog("Детализация волн: " + iNetSeaDetail + " (0 - как в сингле, 1..3 - переключение сетки дальше от камеры)");
	Trace("COOP: sea detail " + iNetSeaDetail + " LodScale " + NCSea.Sea2.LodScale + " GridStep " + NCSea.Sea2.GridStep);
}

void NetClient_MoveSeaToLayers(string sExecuteLayer, string sRealizeLayer)
{
	LayerDelObject(sExecuteLayer, &NCSea);
	LayerDelObject(sRealizeLayer, &NCSea);
	LayerAddObject(sExecuteLayer, &NCSea, 65530);
	LayerAddObject(sRealizeLayer, &NCSea, 65530);
}
