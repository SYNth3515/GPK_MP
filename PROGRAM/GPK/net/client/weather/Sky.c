object NCSky;

void NetClient_WhrDeleteSkyEnvironment()
{
	if (isEntity(&NCSky))
	{
		DeleteClass(&NCSky);
	}
	DeleteAttribute(&NCSky, "");
}

// Используем тот же движковый класс "Sky" (WEATHER.dll), что и в одиночной игре,
// а не NetSky из net.dll (у NetSky другие имена файлов текстур и другой порядок граней).
// Требует, чтобы на событие WhrCalcFogColor был повешен обработчик (см. Weather.c).
// Старый вариант на NetSky: CreateEntity(&NCSky, "NetSky"), NCSky.Server = false, NCSky.Dir = <папка>.
void NetClient_WhrCreateSkyEnvironment()
{
	aref aCurWeather = NetClient_GetCurrentWeather();
	aref aSky; makearef(aSky, aCurWeather.Sky);

	DeleteAttribute(&NCSky, "");

	Trace("COOP: sky dir=" + Whr_GetString(aSky, "Dir") + " size=" + Whr_GetFloat(aSky, "Size"));
	CreateEntity(&NCSky, "Sky");
	LayerAddObject("sea_reflection", &NCSky, 1);

	// формат одиночки: Dir.d1 = папка, значение Dir = время; набор один, поэтому блендинга нет
	NCSky.Dir.d1 = Whr_GetString(aSky, "Dir");
	NCSky.Dir = 12.0;

	NCSky.Color = Whr_GetColor(aSky, "Color");
	NCSky.RotateSpeed = Whr_GetFloat(aSky, "Rotate");
	NCSky.Angle = Whr_GetFloat(aSky, "Angle");
	NCSky.Size = Whr_GetFloat(aSky, "Size");

	NCSky.isDone = "";
	Trace("COOP: sky created");
}

// Вращение неба от ветра, как UpdateSky() в одиночной игре (там Sky.RotateSpeed = ветер / 1000).
// Время в мультиплеере не ускоряется, поэтому делитель по таймскейлу не нужен.
// Хендлер вешается на событие frame в NetClient_CreateWeatherEnvironment, снимается при удалении погоды.
void NetClient_UpdateSky()
{
	if (!isEntity(&NCSky)) { return; }
	NCSky.RotateSpeed = NetClient_WhrGetWindSpeed() / 1000.0;
}

void NetClient_MoveSkyToLayers(string sExecuteLayer, string sRealizeLayer)
{
	LayerDelObject("execute", &NCSky);
	LayerDelObject("realize", &NCSky);
	LayerDelObject(sExecuteLayer, &NCSky);
	LayerDelObject(sRealizeLayer, &NCSky);

	LayerAddObject(sExecuteLayer, &NCSky, 2);
	// приоритет строго меньше острова (NCIsland в net_realize = 3), иначе небо рисуется поверх острова
	LayerAddObject(sRealizeLayer, &NCSky, 2);
}