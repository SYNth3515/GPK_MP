object NCSky;

void NetClient_WhrDeleteSkyEnvironment()
{
	if (isEntity(&NCSky))
	{
		DeleteClass(&NCSky);
	}
	DeleteAttribute(&NCSky, "");
}

void NetClient_WhrCreateSkyEnvironment()
{
	aref aCurWeather = NetClient_GetCurrentWeather();
	aref aSky; makearef(aSky,aCurWeather.Sky);

	DeleteAttribute(&NCSky, "");

	NCSky.Server = false;
	CreateEntity(&NCSky, "NetSky");
	LayerAddObject("sea_reflection", &NCSky, 1);

	// NetSky читает Dir только как обычную строку-папку (формат одиночной игры Dir.dN + время не подходит:
	// получается путь вида "11.0000000SKY_BACK.TGA")
	NCSky.Dir = Whr_GetString(aSky, "Dir");
	NCSky.Color = Whr_GetColor(aSky, "Color");
	NCSky.RotateSpeed = Whr_GetFloat(aSky, "Rotate");
	NCSky.Angle = Whr_GetFloat(aSky, "Angle");
	NCSky.Size = Whr_GetFloat(aSky, "Size");

	NCSky.isDone = "";
}

void NetClient_MoveSkyToLayers(string sExecuteLayer, string sRealizeLayer)
{
	LayerAddObject(sExecuteLayer, &NCSky, 2);
	LayerAddObject(sRealizeLayer, &NCSky, 2);
}