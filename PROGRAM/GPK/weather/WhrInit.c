#include "GPK\Weather\Init\Common.c"
#include "GPK\Weather\Init\Day.c"
#include "GPK\Weather\Init\Evening.c"
#include "GPK\Weather\Init\Morning.c"
#include "GPK\Weather\Init\Night.c"
#include "GPK\Weather\Init\DayStorm.c"
#include "GPK\Weather\Init\Special.c"
#include "GPK\Weather\Init\Rain.c" //navy

// ====================================================================
// GRAPHICS TUNE (cosmetic only).
// Changes only colors and water reflection. Waves (Amp, Scale, AnimSpeed, MoveSpeed,
// PosShift, BumpScale), wind and weather ids are NOT touched, so the host and the
// clients stay in sync even if their files differ. To disable everything:
// set bTune = false in Whr_TuneWeather.
// ====================================================================
int Whr_TuneColor(int iColor, float fSat, float fMul, float fR, float fG, float fB)
{
	int iA = and(shr(iColor, 24), 255);
	float r = MakeFloat(and(shr(iColor, 16), 255)) / 255.0;
	float g = MakeFloat(and(shr(iColor, 8), 255)) / 255.0;
	float b = MakeFloat(and(iColor, 255)) / 255.0;
	float gray = 0.30 * r + 0.59 * g + 0.11 * b;

	r = (gray + (r - gray) * fSat) * fMul * fR;
	g = (gray + (g - gray) * fSat) * fMul * fG;
	b = (gray + (b - gray) * fSat) * fMul * fB;

	return argb(iA, MakeInt(255.0 * Clampf(r)), MakeInt(255.0 * Clampf(g)), MakeInt(255.0 * Clampf(b)));
}

void Whr_TuneWeather(int i)
{
	bool bTune = true;
	if (!bTune) { return; }

	// special weathers (inside / underwater) are left untouched
	if (Weathers[i].id == "Inside") { return; }
	if (Weathers[i].id == "Underwater") { return; }

	bool bStorm = false;
	bool bNight = false;
	if (CheckAttribute(&Weathers[i], "Storm")) { bStorm = sti(Weathers[i].Storm); }
	if (CheckAttribute(&Weathers[i], "Night")) { bNight = sti(Weathers[i].Night); }

	float fRefl = stf(Weathers[i].Sea2.Reflection);

	if (bStorm)
	{
		// storm: less brown, a bit more light and cold contrast
		Weathers[i].Fog.Color = Whr_TuneColor(sti(Weathers[i].Fog.Color), 0.85, 1.10, 0.90, 1.00, 1.12);
		Weathers[i].Sun.Color = Whr_TuneColor(sti(Weathers[i].Sun.Color), 0.90, 1.30, 0.95, 1.00, 1.08);
		Weathers[i].Sun.Ambient = Whr_TuneColor(sti(Weathers[i].Sun.Ambient), 0.90, 1.35, 0.92, 1.00, 1.12);
		Weathers[i].Sea2.WaterColor = Whr_TuneColor(sti(Weathers[i].Sea2.WaterColor), 1.15, 1.05, 0.92, 1.00, 1.08);
		return;
	}

	if (bNight) { return; }

	// day / morning / evening
	Weathers[i].Sea2.WaterColor = Whr_TuneColor(sti(Weathers[i].Sea2.WaterColor), 1.25, 1.10, 0.95, 1.00, 1.05);
	fRefl = fRefl + 0.10;
	if (fRefl > 0.95) { fRefl = 0.95; }
	Weathers[i].Sea2.Reflection = fRefl;

	Weathers[i].Fog.Color = Whr_TuneColor(sti(Weathers[i].Fog.Color), 1.15, 1.00, 0.97, 1.00, 1.05);
	Weathers[i].Sun.Color = Whr_TuneColor(sti(Weathers[i].Sun.Color), 1.10, 1.00, 1.04, 1.00, 0.94);
	Weathers[i].Sun.Ambient = Whr_TuneColor(sti(Weathers[i].Sun.Ambient), 1.00, 1.05, 0.95, 0.99, 1.08);
}

int InitWeather()
{
	int n = 0;

	n = Whr_InitNight(n);
	n = Whr_InitMorning(n);
	n = Whr_InitDay(n);
	n = Whr_InitEvening(n);
	n = Whr_InitDayStorm(n);
	n = Whr_InitSpecial(n);
	//n = Whr_InitRain(n);

	Whr_InitCommon();

	Trace("Init weathers complete. " + n + " weathers found.");

	for (int i=0; i<n; i++)
	{
		if (!CheckAttribute(&Weathers[i], "Stars.Enable")) { Weathers[i].Stars.Enable = false; }
		if (!CheckAttribute(&Weathers[i], "Planets.Enable")) { Weathers[i].Planets.Enable = false; }
		if (!CheckAttribute(&Weathers[i], "Sun.Reflection.Enable")) { Weathers[i].Sun.Reflection.Enable = false; }
		if (!CheckAttribute(&Weathers[i], "Sounds.Sea.Postfix")) { Weathers[i].Sounds.Sea.Postfix = ""; }
		if (!CheckAttribute(&Weathers[i], "Sun.Moon")) { Weathers[i].Sun.Moon = false; }

		if (!CheckAttribute(&Weathers[i], "Rain.DropsNearNum")) { Weathers[i].Rain.DropsNearNum = 0; };
		if (!CheckAttribute(&Weathers[i], "Rain.DropsFarNum")) { Weathers[i].Rain.DropsFarNum = 0; };
		if (!CheckAttribute(&Weathers[i], "Rain.DropsNearRadius")) { Weathers[i].Rain.DropsNearRadius = 12.0; };
		if (!CheckAttribute(&Weathers[i], "Rain.DropsFarRadius")) { Weathers[i].Rain.DropsFarRadius = 55.0; };
		if (!CheckAttribute(&Weathers[i], "Rain.DropsLifeTime")) { Weathers[i].Rain.DropsLifeTime = 0.25; };
		if (!CheckAttribute(&Weathers[i], "Rain.DropsSize")) { Weathers[i].Rain.DropsSize = 0.07; };
		if (!CheckAttribute(&Weathers[i], "Rain.DropsTexture")) { Weathers[i].Rain.DropsTexture = "weather\rain_drops.tga"; };
		if (!CheckAttribute(&Weathers[i], "Rain.DropsColor")) { Weathers[i].Rain.DropsColor = argb(63, 255, 255, 255); };

		if (!CheckAttribute(&Weathers[i], "Sea2.FoamK")) { Weathers[i].Sea2.FoamK = 0.0; };
		if (!CheckAttribute(&Weathers[i], "Sea2.FoamV")) { Weathers[i].Sea2.FoamV = 3.0; };
		if (!CheckAttribute(&Weathers[i], "Sea2.FoamUV")) { Weathers[i].Sea2.FoamUV = 3.0; };
		if (!CheckAttribute(&Weathers[i], "Sea2.FoamTexDisturb")) { Weathers[i].Sea2.FoamTexDisturb = 0.7; };

		Whr_TuneWeather(i);
	}

	return n;
}
