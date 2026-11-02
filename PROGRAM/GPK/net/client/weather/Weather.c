#include "GPK\net\client\weather\Lightning.c"
#include "GPK\net\client\weather\Rain.c"
#include "GPK\net\client\weather\Sea.c"
#include "GPK\net\client\weather\Sky.c"
#include "GPK\net\client\weather\Sun.c"
#include "GPK\net\client\weather\Tornado.c"
#include "GPK\net\client\weather\Astronomy.c"

object	NCWeather;
int		iClientCurWeatherNum = -1;
string	sClientCurrentFog;
bool	bClientWeatherIsNight = false;
bool	bClientWeatherIsLight = false;
bool	bClientWeatherIsRain = false;
bool	bClientWeatherIsStorm = false;
string	sClientLightingPath;
// COOP switch: 1 = wind is synced from the server (client frame handler + periodic packets), 0 = wind stays fixed at the base value
int		iCoopWindSync = 0;
int		iCoopWindSyncCount = 0;
float	fClientWeatherDelta = 0.0;
float	fClientWeatherAngle = 0.0;
float	fClientWeatherSpeed = 3.0;

//#event_handler("EWhr_GetWindAngle", "NetClient_WhrGetWindAngle");
//#event_handler("EWhr_GetWindSpeed", "NetClient_WhrGetWindSpeed");
//#event_handler("EWhr_GetFogDensity", "NetClient_WhrGetFogDensity");

string NetClient_WhrGetCurrentFog()
{
	return sClientCurrentFog;
}

void NetClient_DeleteWeatherEnvironment()
{
	if (isEntity(&NCWeather))
	{
		DeleteClass(&NCWeather);
	}
	DeleteAttribute(&NCWeather, "");

	NetClient_WhrDeleteRainEnvironment();
	NetClient_WhrDeleteSkyEnvironment();
	NetClient_WhrDeleteSunGlowEnvironment();
	NetClient_WhrDeleteLightningEnvironment();
	//NetClient_WhrDeleteTornadoEnvironment();
	NetClient_WhrDeleteSeaEnvironment();
	NetClient_WhrDeleteAstronomyEnvironment();

	DelEventHandler("NetClient_WhrOnCalcFogColor", "NetClient_WhrOnCalcFogColor");
	DelEventHandler("WhrCalcFogColor", "NetClient_WhrOnCalcFogColor");
	DelEventHandler("frame", "NetClient_WhrOnWindChange");
	DelEventHandler("frame", "NetClient_UpdateSky");

	bWeatherLoaded = false;
}

aref NetClient_GetCurrentWeather()
{
	aref arWeather; 
	makearef(arWeather, Weathers[iClientCurWeatherNum]);
	return arWeather;
}

void NetClient_CreateWeatherEnvironment(int iMsg)
{
	iClientCurWeatherNum = 0;
	string sWeatherID = NMGetString(iMsg);
	Trace("COOP: cli weather id=" + sWeatherID);

	// WIND SYNC: base wind from the server (written right after the weather id in NetServer_CreateWeatherEnvironment)
	fClientWeatherAngle = NMGetFloat(iMsg);
	fClientWeatherSpeed = NMGetFloat(iMsg);
	fClientWeatherDelta = 0.0;

	Trace("COOP: cli base wind angle=" + fClientWeatherAngle + " speed=" + fClientWeatherSpeed);
	if (COOP_WindBad(fClientWeatherAngle) || COOP_WindBad(fClientWeatherSpeed))
	{
		Trace("COOP: cli base wind is bad (NaN/INF), using table values, check that host and client have the same files");
		fClientWeatherAngle = 0.0;
		fClientWeatherSpeed = 3.0;
	}

	for (int i=0; i<iTotalNumWeathers; i++) 
	{
		if (sWeatherID == Weathers[i].id)
		{
			iClientCurWeatherNum = i;
			break;
		}
	}

	if (iClientCurWeatherNum < 0)
	{
		Trace("NCError: Can't find weather with id " + sWeatherID);
		iClientCurWeatherNum = 0;
	}

	// create main module Weather
	DeleteAttribute(&NCWeather, "");
	NCWeather.Server = false;
	// COOP: when the admin changes the weather in the running game the entity stays alive (like Whr_UpdateWeather in the
	// single player game), only its data is replaced
	if (!isEntity(&NCWeather)) { CreateEntity(&NCWeather, "NetWeather"); }

	aref aCurWeather = NetClient_GetCurrentWeather();

	sClientLightingPath = Whr_GetString(aCurWeather,"Lighting");

	bClientWeatherIsNight = Whr_GetLong(aCurWeather, "Night");
	bClientWeatherIsLight = Whr_GetLong(aCurWeather, "Lights");

	NCWeather.WindAngle = fClientWeatherAngle;
	NCWeather.WindSpeed = fClientWeatherSpeed;
	if (iCoopWindSync != 0) { SetEventHandler("frame", "NetClient_WhrOnWindChange", 0); }

	sClientCurrentFog = "Fog";
	if (CheckAttribute(aCurWeather, "SpecialSeaFog")) { sClientCurrentFog = "SpecialSeaFog"; }	

	string sCurFog = NetClient_WhrGetCurrentFog();
	NCWeather.Fog.Enable = Whr_GetLong(aCurWeather, sCurFog + ".Enable");
	NCWeather.Fog.Start = Whr_GetFloat(aCurWeather, sCurFog + ".Start");
	NCWeather.Fog.Density = Whr_GetFloat(aCurWeather, sCurFog + ".Density");
	NCWeather.Fog.Color = Whr_GetColor(aCurWeather, sCurFog + ".Color");
	NCWeather.Fog.IslandDensity = Whr_GetFloat(aCurWeather, sCurFog + ".IslandDensity");

	// COOP: движковые классы (Grass в Location.dll и др.) спрашивают события EWhr_GetFogDensity, EWhr_GetWindAngle, EWhr_GetWindSpeed.
	// Их обработчики из одиночки (Whr_GetFogDensity, Whr_GetWindAngle, Whr_GetWindSpeed) зарегистрированы на этапе компиляции и читают
	// глобальную fFogDensity и атрибуты Weather.Wind, которые в мультиплеере пустые (туман 0, ветер по умолчанию). Подставляем мультиплеерные значения.
	fFogDensity = stf(NCWeather.Fog.Density);
	Weather.Wind.Angle = fClientWeatherAngle;
	Weather.Wind.Speed = fClientWeatherSpeed;

	NCWeather.Sun.Color = Whr_GetColor(aCurWeather,"Sun.Color");
	NCWeather.Sun.HeightAngle = Whr_GetFloat(aCurWeather,"Sun.HeightAngle");
	NCWeather.Sun.AzimuthAngle = Whr_GetFloat(aCurWeather,"Sun.AzimuthAngle");
	NCWeather.Sun.Ambient = Whr_GetColor(aCurWeather,"Sun.Ambient");

	float x, y, z;

	x = 5000.0; 
	y = 0.0;
	RotateAroundY(&x, &y, cos(-stf(NCWeather.Sun.HeightAngle)), sin(-stf(NCWeather.Sun.HeightAngle)));
	
	z = 0.0;
	RotateAroundY(&x, &z, cos(stf(NCWeather.Sun.AzimuthAngle)), sin(stf(NCWeather.Sun.AzimuthAngle)));

	NCWeather.Sun.x = x;
	NCWeather.Sun.y = y;
	NCWeather.Sun.z = z;

	NCWeather.isDone = "";

	SetEventHandler("NetClient_WhrOnCalcFogColor", "NetClient_WhrOnCalcFogColor", 0);
	// движковый класс Sky (WEATHER.dll) на каждый вертекс вызывает событие WhrCalcFogColor и падает на assert (SKY.cpp line 84),
	// если у события нет обработчика; в одиночке его вешает Whr_OnCalcFogColor, здесь вешаем клиентский аналог
	SetEventHandler("WhrCalcFogColor", "NetClient_WhrOnCalcFogColor", 0);

	NetClient_WhrCreateRainEnvironment();
	NetClient_WhrCreateSunGlowEnvironment();
	NetClient_WhrCreateLightningEnvironment();
	Trace("COOP: cli before sky");
	NetClient_WhrCreateSkyEnvironment();
	Trace("COOP: cli after sky");
	SetEventHandler("frame", "NetClient_UpdateSky", 0);
	NetClient_WhrCreateSeaEnvironment();
	NetClient_WhrCreateAstronomyEnvironment();
	//if (bWhrTornado) { NetClient_WhrCreateTornadoEnvironment(); }

	Particles.windpower = 0.05 * Clampf(5.0 * fClientWeatherSpeed / WIND_NORMAL_POWER);
	Particles.winddirection.x = sin(fClientWeatherAngle);
	Particles.winddirection.z = cos(fClientWeatherAngle);

	bWeatherLoaded = true;

	NCIsland.LightingPath = GetLightingPath();
	NCIsland.FogDensity = NCWeather.Fog.IslandDensity;
	SendMessage(&IslandReflModel, "lllf", MSG_MODEL_SET_FOG, 1, 1, stf(NCWeather.Fog.IslandDensity));

	NetClient_MoveWeatherToLayers("net_execute", "net_realize");
}

float NetClient_WhrGetFogDensity() 
{ 
	return stf(NCWeather.Fog.Density); 
}

int NetClient_WhrOnCalcFogColor()
{
	int		iAlpha, iColor;
	float	x, y, z;
	aref	aCurWeather = NetClient_GetCurrentWeather();

	x = GetEventData();
	y = GetEventData();
	z =	GetEventData();

	iAlpha = 255 - MakeInt(255.0 * Clampf(y / stf(aCurWeather.(sClientCurrentFog).Height)));

	//iColor = argb(0,0,0,0);
	int iFogColor = sti(NCWeather.Fog.Color);
	iColor = or(shl(iAlpha, 24), iFogColor);
	return iColor;
}

void NetClient_MoveWeatherToLayers(string sExecuteLayer, string sRealizeLayer)
{
	// COOP: this is also called when the weather is changed in the running game: never add the object to a layer twice
	LayerDelObject(sExecuteLayer, &NCWeather);
	LayerDelObject(sRealizeLayer, &NCWeather);
	LayerAddObject(sExecuteLayer, &NCWeather, 1000000);
	LayerAddObject(sRealizeLayer, &NCWeather, 65530);

	NetClient_MoveRainToLayers(sExecuteLayer, sRealizeLayer);
	NetClient_MoveLightningToLayers(sExecuteLayer, sRealizeLayer);
	NetClient_MoveSkyToLayers(sExecuteLayer, sRealizeLayer);
	NetClient_MoveSunGlowToLayers(sExecuteLayer, sRealizeLayer);
	NetClient_MoveAstronomyToLayers(sExecuteLayer, sRealizeLayer);
	NetClient_MoveSeaToLayers(sExecuteLayer, sRealizeLayer);
}

float NetClient_WhrGetWindAngle()
{
	if (!CheckAttribute(&NCWeather,"WindAngle")) { return 0.0; }
	return stf(NCWeather.WindAngle);
}

float NetClient_WhrGetWindSpeed()
{
	if (!CheckAttribute(&NCWeather,"WindSpeed")) { return 0.0; }
	return stf(NCWeather.WindSpeed);
}

// same formula as the server (NetServer_WhrOnWindChange), so the wind is smooth and identical on all machines
// COOP: true if the float is NaN / INF (string form contains N or #)
bool COOP_WindBad(float f)
{
	string sF = "" + f;
	if (findSubStr(&sF, "N", 0) >= 0) { return true; }
	if (findSubStr(&sF, "#", 0) >= 0) { return true; }
	return false;
}

void NetClient_WhrOnWindChange()
{
	float fDeltaTime = MakeFloat(GetDeltaTime()) * 0.001;
	fClientWeatherDelta = fClientWeatherDelta + fDeltaTime;

	float fVar = sin(fClientWeatherDelta) + sin(0.2 * fClientWeatherDelta) + sin(PI + 0.8 * fClientWeatherDelta) + cos(1.5 * fClientWeatherDelta);
	float fSpd = fClientWeatherSpeed + (fClientWeatherSpeed / 6.0) * 0.1 * fVar;
	float fAng = fClientWeatherAngle + 0.02 * fVar;

	NCWeather.WindAngle = fAng;
	NCWeather.WindSpeed = fSpd;
	Weather.Wind.Angle = fAng;
	Weather.Wind.Speed = fSpd;

	Particles.windpower = 0.05 * Clampf(5.0 * fSpd / WIND_NORMAL_POWER);
	Particles.winddirection.x = sin(fAng);
	Particles.winddirection.z = cos(fAng);
}

// periodic correction from the server
void NetClient_WhrOnWindSync(int iMsg)
{
	float fDelta = NMGetFloat(iMsg);
	float fAngle = NMGetFloat(iMsg);
	float fSpeed = NMGetFloat(iMsg);

	if (!bWeatherLoaded) { return; }
	if (iCoopWindSync == 0) { return; }

	iCoopWindSyncCount = iCoopWindSyncCount + 1;
	if (iCoopWindSyncCount <= 3) { Trace("COOP: cli wind sync #" + iCoopWindSyncCount + " delta=" + fDelta + " angle=" + fAngle + " speed=" + fSpeed); }
	if (COOP_WindBad(fDelta) || COOP_WindBad(fAngle) || COOP_WindBad(fSpeed)) { return; }

	fClientWeatherDelta = fDelta;
	fClientWeatherAngle = fAngle;
	fClientWeatherSpeed = fSpeed;
}

string NetClient_WhrGetLightingPath() { return sClientLightingPath; }

bool NetClient_WhrIsDay() { return !bClientWeatherIsNight; }
bool NetClient_WhrIsNight() { return bClientWeatherIsNight; }
bool NetClient_WhrIsLight() { return bClientWeatherIsLight; }
bool NetClient_WhrIsRain() { return bClientWeatherIsRain; }
bool NetClient_WhrIsStorm() { return bClientWeatherIsStorm; }