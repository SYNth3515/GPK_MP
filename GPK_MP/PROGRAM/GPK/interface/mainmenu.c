string isUsersName="";

// GPK_MP: links for the main menu buttons (set your real URLs here)
#define MM_URL_GITHUB	"https://github.com/SYNth3515/GPK_MP"
#define MM_URL_SUPPORT	"https://yoomoney.ru/to/4100119665987017/0"
#define MM_URL_VK		"https://vk.ru/gpk_mp"

//ref rCharacter; // boal
int iChar;
#event_handler("Control Activation","TempIControlProcess");

void TempIControlProcess()
{
	string ControlName;
	ControlName = GetEventData();
	if (ControlName == "WhrPrevWeather")
	{
		Sea.MaxSeaHeight = 1.0;
		Sea.isDone = "";
	}
	if (ControlName == "WhrNextWeather")
	{
		Sea.MaxSeaHeight = 1.0;
		Sea.isDone = "";
	}
	if (ControlName == "WhrUpdateWeather")
	{
		Sea.MaxSeaHeight = 1.0;
		Sea.isDone = "";
	}
}

void InitInterface(string iniName)
{
	Event("DoInfoShower","sl","MainMenuLaunch",false);

	aref arScrShoter;
	if( !FindClass(&arScrShoter,"scrshoter") ) {
		CreateScreenShoter();
	}

	CreateBackEnvironment();

    SendMessage(&GameInterface,"ls",MSG_INTERFACE_INIT,iniName);

	// boal ver num -->
    CreateString(true,"VerNum", VERSION_NUMBER1 + GetVerNum(), FONT_NORMAL, COLOR_NORMAL, 780, 580, SCRIPT_ALIGN_RIGHT, 0.9);
    // boal ver num <--

	MM_InitVersionCheck();
	MM_InitCredits();
	
	SetEventHandler("backgroundcommand","ProcCommand",0);
	SetEventHandler("OnMainMenuGithub","MainMenu_OnGithub",0);
	SetEventHandler("OnMainMenuSupport","MainMenu_OnSupport",0);
	SetEventHandler("OnMainMenuVK","MainMenu_OnVK",0);

	GameInterface.SavePath = "SAVE";

 //ResetSoundScheme();
	ResetSound(); // new
	// ВВОД СВОИХ СХЕМ В ЗАВИСИМОСТИ ОТ ПОГОДЫ (BY LOKK)
	if (Whr_IsRain())
	{ if (Whr_IsNight())
		{
			SetSoundScheme("mainmenu_night_rain");
		}
		else
		{
			SetSoundScheme("mainmenu_day_rain");
		}
    }
	else
	{ if (Whr_IsNight())
		{
			SetSoundScheme("mainmenu_night");
		}
		else
		{
			SetSoundScheme("mainmenu_day");
		}

	}
	oldMusicID = 0;
	musicName = "";
	SetMusic("music_MainMenu");
}

void NewGamePress()
{
	IDoExit( RC_INTERFACE_DO_NEW_GAME, true );
}

void LoadPress()
{
	IDoExit( RC_INTERFACE_DO_LOAD_GAME, false );
}

void SavePress()
{
	IDoExit( RC_INTERFACE_DO_SAVE_GAME, false );
}

void OptionsPress()
{
	IDoExit( RC_INTERFACE_DO_OPTIONS, false );
}

void ControlsPress()
{
	IDoExit( RC_INTERFACE_DO_CONTROLS, false );
}

void CreditsPress()
{
	// COOP s29: the video of the credits goes over the menu: the link buttons are hidden like the rest (the button "Мы в ВК"
	// stayed on the screen). The main menu shows them again when it starts after the video.
	SetNodeUsing("BTN_VK", false);
	SetNodeUsing("BTN_GITHUB", false);
	SetNodeUsing("BTN_SUPPORT", false);
	SetNodeUsing("QR_SUPPORT", false);
	IDoExit( RC_INTERFACE_DO_CREDITS, false );

	/*SetEventHandler(EVENT_END_VIDEO,"LaunchMainMenu_afterVideo",0);
	bMainMenuLaunchAfterVideo = true;
	StartPostVideo("credits",1);*/
}

void QuitPress()
{
    DelEventHandler("frame","QuitPress");
	EngineLayersOffOn(false);
	IDoExit(-1, false);
	ExitProgram();
}

void procGameQuit()
{
	ExitProgram();
}

void MultiPress()
{
	Trace("COOP_PATCH: MultiPress called");
	//PrepareNetEnvironment();
	DeleteBackEnvironment();
	IDoExit(RC_INTERFACE_NET_FINDGAME, false);
	InterfaceStates.BackEnvironmentIsCreated = false;
}

// GPK_MP: OpenURL(string) is a new engine function (patched engine.exe), it opens an
// http:// or https:// link in the default browser. Other strings are ignored.
void MainMenu_OpenURL(string sUrl)
{
	OpenURL(sUrl);
}

void MainMenu_OnGithub()
{
	MainMenu_OpenURL(MM_URL_GITHUB);
}

void MainMenu_OnSupport()
{
	MainMenu_OpenURL(MM_URL_SUPPORT);
}

void MainMenu_OnVK()
{
	MainMenu_OpenURL(MM_URL_VK);
}

void IDoExit(int exitCode, bool bClear)
{
	InterfaceStates.BackEnvironmentIsCreated = true;

	DelEventHandler("backgroundcommand","ProcCommand");
	DelEventHandler("OnMainMenuGithub","MainMenu_OnGithub");
	DelEventHandler("OnMainMenuSupport","MainMenu_OnSupport");
	DelEventHandler("OnMainMenuVK","MainMenu_OnVK");
	DelEventHandler("frame","MM_CreditsFrame");
	
	interfaceResultCommand = exitCode;
	EndCancelInterface(bClear);
}

void ExitCancel()
{
	IDoExit(-1,true);
}

void ProcCommand()
{
	string comName = GetEventData();

	switch(comName)
	{
	case "New":			NewGamePress(); break;
	case "Load":		LoadPress(); break;
	case "Save":		SavePress(); break;
	case "Options":		OptionsPress(); break;
	case "Multiplayer":	MultiPress(); break;
	case "Credits":		CreditsPress(); break;
	case "Exit":		SetEventHandler("frame","QuitPress",0);  break;
	}
}

object InterfaceBackScene;
void CreateBackEnvironment()
{
	LayerFreeze("execute",false);
	LayerFreeze("realize",false);

	if( CheckAttribute(&InterfaceStates,"BackEnvironmentIsCreated") && InterfaceStates.BackEnvironmentIsCreated=="1" ) {
		return;
	}

	// сбрасываем цвет фона на 0
	Render.BackColor = 0;
	// выключаем эффект моря
	Render.SeaEffect = false;
	// 14.07.2007 - отключаем подводную часть
	Sea.UnderWater = false;
	bMainCharacterInFire = false;
	bMainMenu = true;

	// create weather
	ICreateWeather();

	CreateEntity(&InterfaceBackScene,"InterfaceBackScene");
	LayerAddObject("execute", &InterfaceBackScene, -1);
	LayerAddObject("realize", &InterfaceBackScene, 1000);

	SendMessage(&InterfaceBackScene, "ls", 0, "MainMenu\MainMenu"); // set model
	SendMessage(&InterfaceBackScene, "ls", 1, "camera"); // set camera

	aref arMenu;
	makearef(arMenu,InterfaceBackScene.menu);
	arMenu.l1.locname = "menu02";	arMenu.l1.sel = "mainmenu\menu02_active";	arMenu.l1.norm = "mainmenu\menu02_passive"; arMenu.l1.event = "New";			arMenu.l1.path = LanguageGetLanguage();
	arMenu.l2.locname = "menu03";	arMenu.l2.sel = "mainmenu\menu03_active";	arMenu.l2.norm = "mainmenu\menu03_passive"; arMenu.l2.event = "Load";			arMenu.l2.path = LanguageGetLanguage();
	arMenu.l3.locname = "menu04";	arMenu.l3.sel = "mainmenu\menu04_active";	arMenu.l3.norm = "mainmenu\menu04_passive"; arMenu.l3.event = "Options";		arMenu.l3.path = LanguageGetLanguage();
	//arMenu.l4.locname = "menu05";	arMenu.l4.sel = "mainmenu\menu05_active";	arMenu.l4.norm = "mainmenu\menu05_passive"; arMenu.l4.event = "Multiplayer";	arMenu.l4.path = LanguageGetLanguage();
	//arMenu.l5.locname = "menu06";	arMenu.l5.sel = "mainmenu\menu06_active";	arMenu.l5.norm = "mainmenu\menu06_passive"; arMenu.l5.event = "Credits";		arMenu.l5.path = LanguageGetLanguage();
	//arMenu.l6.locname = "menu07";	arMenu.l6.sel = "mainmenu\menu07_active";	arMenu.l6.norm = "mainmenu\menu07_passive"; arMenu.l6.event = "Exit";			arMenu.l6.path = LanguageGetLanguage();
	// GPK_MP: 6 menu items. Slot menu05 = Network game (new), menu06 = Credits (original models), menu07 = Exit (original models)
	arMenu.l4.locname = "menu05";	arMenu.l4.sel = "mainmenu\MenuNet_active";	arMenu.l4.norm = "mainmenu\MenuNet_passive"; arMenu.l4.event = "Multiplayer";	arMenu.l4.path = LanguageGetLanguage();
	arMenu.l5.locname = "menu06";	arMenu.l5.sel = "mainmenu\menu05_active";	arMenu.l5.norm = "mainmenu\menu05_passive"; arMenu.l5.event = "Credits";		arMenu.l5.path = LanguageGetLanguage();
	arMenu.l6.locname = "menu07";	arMenu.l6.sel = "mainmenu\menu06_active";	arMenu.l6.norm = "mainmenu\menu06_passive"; arMenu.l6.event = "Exit";			arMenu.l6.path = LanguageGetLanguage();
	SendMessage(&InterfaceBackScene, "lla", 3, 1, arMenu ); // set menu
	//SendMessage(&InterfaceBackScene, "lll", 7, 0, true ); // set menu
	//SendMessage(&InterfaceBackScene, "lll", 7, 1, true ); // set menu
	//SendMessage(&InterfaceBackScene, "lll", 7, 2, true ); // set menu
	//SendMessage(&InterfaceBackScene, "lll", 7, 3, true ); // set menu
	//SendMessage(&InterfaceBackScene, "lll", 7, 4, true ); // set menu
	//SendMessage(&InterfaceBackScene, "lll", 7, 5, true ); // set menu

	// create ship
	MainMenu_CreateShip();

	if( LanguageGetLanguage() != "Russian" )
	{
		MenuCreateLogo();
	}

	if( Whr_IsNight() ) {
		InterfaceBackScene.light.turnon = true;
		InterfaceBackScene.light.model = "mainmenu\Fonar_night";
		InterfaceBackScene.light.lightcolormin = argb(0,200,200,120);//argb(255,114,114,80);
		InterfaceBackScene.light.lightcolormax = argb(48,255,255,180);
		InterfaceBackScene.light.colorperiod = 0.4;
		InterfaceBackScene.light.addcolorperiod = 1.0;
		InterfaceBackScene.light.rangemin = 10.0;
		InterfaceBackScene.light.rangemax = 14.0;
		InterfaceBackScene.light.rangeperiod = 1.5;
		InterfaceBackScene.light.locator = "Light";
		InterfaceBackScene.light.lightlocator = "fonar";
		InterfaceBackScene.light.flarelocator = "fire";
		InterfaceBackScene.light.flaresize = 0.5;
		InterfaceBackScene.light.minflarecolor = 120.0;
		InterfaceBackScene.light.maxflarecolor = 200.0;
	} else {
		MainMenu_CreateAnimals();

		InterfaceBackScene.light.turnon = false;
		InterfaceBackScene.light.model = "mainmenu\Fonar_day";
		InterfaceBackScene.light.locator = "Light";
		InterfaceBackScene.light.lightlocator = "fonar";
	}
	SendMessage(&InterfaceBackScene, "ls", 8, "light" );

	MM_HeroCreate();		// COOP s30: the hero of the profile on the rope bridge

	if( Whr_IsNight() )
	{
		// create particles
		InitParticles();
		CreateParticleSystem("candle", stf(InterfaceBackScene.lightpos.x),stf(InterfaceBackScene.lightpos.y),stf(InterfaceBackScene.lightpos.z), 0.0,0.0,0.0, 0);
	}
	bMainMenu = false;
}

void MainMenu_CreateShip()
{
	//int
	iChar = GenerateCharacter(rand(4), WITHOUT_SHIP, "citizen", MAN, 0, WARRIOR);   //PIRATE
	ref	rCharacter = &characters[iChar];
	int nChoosedBaseShipType = rand(SHIP_GALEON_H) + 2;//SHIP_TARTANE;SHIP_LUGGER;SHIP_CARAVEL;
	rCharacter.ship.type = GenerateShip(nChoosedBaseShipType, false);
	SetBaseShipData( rCharacter );
	ref refBaseShip = GetRealShip( sti(rCharacter.ship.type) );
	refBaseShip.SpeedRate = 3.0; // чтоб не летал
	Ship_SetFantomData( rCharacter );
	Ship_ClearImpulseStrength( rCharacter );
	Ship_SetLightsAndFlares( rCharacter );
	Ship_SetTrackSettings( rCharacter );

	rCharacter.Features.GeraldSails = true;
	rCharacter.Ship.Speed.z = 1.0;
	//rCharacter.Ship.Stopped = true;
	rCharacter.Ship.Pos.Mode = SHIP_SAIL;
	rCharacter.Ship.Pos.x = 0.0;
	rCharacter.Ship.Pos.z = 0.0;
	rCharacter.Ship.Ang.y = 0.0;
	rCharacter.Ship.Strand = false;
	rCharacter.Ship.Strand.SndID = -1;
	
	rCharacter.Ship.LastBallCharacter = -1;
	rCharacter.Ship.WindDotShip = 1.0;
	
	rCharacter.TmpPerks = "";
	aref aTmpPerks; makearef(aTmpPerks, rCharacter.TmpPerks);
	aTmpPerks.StormProfessional		= 0;
	
	SendMessage(&InterfaceBackScene, "lsa", 2, "ship", &rCharacter); // set ship position
	CreateEntity( rCharacter, "ship" );
	ref rBaseShip = GetRealShip(sti(rCharacter.ship.type));
	SendMessage( rCharacter, "laa", MSG_SHIP_CREATE, &rCharacter, &rBaseShip );
}

void DeleteBackEnvironment()
{
	LayerDelObject("execute", &InterfaceBackScene);
	LayerDelObject("realize", &InterfaceBackScene);

	MainMenu_DeleteAnimals();
	MM_HeroDelete();		// COOP s30
	DeleteClass( GetCharacter(iChar));//  boal fix FANTOM_CHARACTERS) );
	DeleteShipEnvironment();
	DeleteWeather();
	DeleteSea();
	DeleteClass( InterfaceBackScene );
	DeleteCoastFoamEnvironment();

	iNextWeatherNum = -1;
	iCurWeatherHour = -1;
}

void MainMenu_CreateAnimals()
{
	InterfaceBackScene.seagull.locator = "seagull";
	InterfaceBackScene.seagull.model = "mainmenu\seagull";
	InterfaceBackScene.seagull.animation = "seagull";
	InterfaceBackScene.seagull.aniaction = "idle";
	SendMessage(&InterfaceBackScene, "ls", 9, "seagull" );
}

void MenuCreateLogo()
{
	InterfaceBackScene.Logo.locator = "";
	InterfaceBackScene.Logo.model = "mainmenu\AOPBoard";
	if( !Whr_IsNight() ) {
		InterfaceBackScene.Logo.technique = "InterfaceBackScene_Logo";
	}
	SendMessage(&InterfaceBackScene, "ls", 10, "Logo" );
}

void MainMenu_DeleteAnimals()
{
	if (IsEntity(Animals))
		DeleteClass(Animals);
}

void ICreateWeather()
{
	// choose random weather
	int n = 0;
	if( CheckAttribute(&InterfaceStates,"mainmenuweather") )
	{
		n = sti(InterfaceStates.mainmenuweather);
	}
	else
	{
		n = rand(iTotalNumWeathers-1);
		int oldN=n;
		while (true)
		{
			// COOP s30: not the weathers of the cabins and of the sea bottom (weather\Init\Special.c: a dark brown light
			// under a twilight sky, a green one)
			bool bMMWhrOk = true;
			if (Weathers[n].id == "Inside") { bMMWhrOk = false; }
			if (Weathers[n].id == "Underwater") { bMMWhrOk = false; }
			if (CheckAttribute(&Weathers[n], "skip"))
			{
				if (Weathers[n].skip == "1") { bMMWhrOk = false; }
			}
			if (bMMWhrOk) { break; }
			//candle
			n++;
			if( n==iTotalNumWeathers ) {n = 0;}
			if( n==oldN ) {break;}
		}
	}
	if (n < 0 || n >= iTotalNumWeathers) n = 0;
	Trace("COOP: main menu weather " + Weathers[n].id + ", the clock was " + GetHour());
	// COOP s30: the clock goes to the hour of this weather. The light and the sun come from the chosen weather, but the sky
	// (weather\WhrSky.c, FillSkyDir) is the sky of the hour of the clock: at the first start Whr_LoadNextWeather returns at
	// once (no Weather entity yet) and does not move the clock, so a night lit scene stood under a day sky.
	if (CheckAttribute(&Weathers[n], "Hour.Min")) { SetCurrentTime(makeint(stf(Weathers[n].Hour.Min)), 0); }
	SetNextWeather(Weathers[n].id);
	iBlendWeatherNum = -1; // залоченная погода
	Whr_LoadNextWeather(0);
	iBlendWeatherNum = -1; // залоченная погода
	InterfaceBackScene.current_weather = n;
	InterfaceStates.mainmenuweather    = n;

	CreateSea("execute","realize");
	CreateWeather("execute","realize");
	CreateShipEnvironment();
	Sea.MaxSeaHeight = 1.0;
	Sea.isDone = "";

	CreateCoastFoamEnvironment("MainMenu", "execute", "realize");
	iBlendWeatherNum = -1; // залоченная погода
}

// ============================================================================
// GPK_MP: line "Нет обновлений / Версия устарела"
// Local date: "от dd.mm.yy" in VERSION_NUMBER2 (_LSC_on_off.h).
// Latest release name must also contain a date dd.mm.yy.
// ============================================================================
#define MM_STATUS_Y		165

#define MM_UPD_WAIT		30000	// ms: no answer from GitHub in that time -> "Проверка обновлений недоступна"
#define MM_UPD_POLL		1000	// ms between the reads of gpk_update.ini

int iMMUpdTime = 0;
int iMMUpdPoll = 0;

// COOP s30: the patched engine.exe runs OpenURL("gpk:<command line>") hidden (CreateProcessA, no window). Once per start
// of the game a PowerShell asks the GitHub API for the latest release and writes gpk_update.ini ([Update] Latest=<name>)
// next to engine.exe. The old engine ignores such a string (only http:// and https:// are opened).
void MM_StartUpdateDownload()
{
	if (CheckAttribute(&InterfaceStates, "GpkUpdStarted")) { return; }
	InterfaceStates.GpkUpdStarted = true;
	string sCmd = "gpk:powershell -NoProfile -NonInteractive -ExecutionPolicy Bypass -Command ";
	sCmd = sCmd + "$ErrorActionPreference='Stop';[Net.ServicePointManager]::SecurityProtocol=3072;";
	sCmd = sCmd + "$r=Invoke-RestMethod -Uri 'https://api.github.com/repos/SYNth3515/GPK_MP/releases/latest' -TimeoutSec 20 -UseBasicParsing;";
	sCmd = sCmd + "Set-Content -Path 'gpk_update.tmp' -Value @('[Update]',('Latest='+$r.name)) -Encoding Default;";
	sCmd = sCmd + "Move-Item -Path 'gpk_update.tmp' -Destination 'gpk_update.ini' -Force";
	OpenURL(sCmd);
}

// Returns the name of the latest GitHub release (e.g. "GPK_MP 16.11.2026 ALPHA") or "" if unknown.
string MM_GetLatestReleaseName()
{
	// NetIni_GetString crashes the engine when the file is missing, so check it first
	if (NFExist("gpk_update.ini") == 0) { return ""; }
	return NetIni_GetString("gpk_update.ini", "Update", "Latest");
}

// true when the iCnt chars of s from iFrom are digits
bool MM_IsDigits(string s, int iFrom, int iCnt)
{
	int k;
	for (k = 0; k < iCnt; k++)
	{
		if (findsubstr("0123456789", strcut(s, iFrom + k, iFrom + k), 0) < 0) { return false; }
	}
	return true;
}

// "dd.mm.yy" or "dd.mm.yyyy" anywhere in the string -> yymmdd as a number (so dates compare as numbers), -1 if not found
int MM_ParseDate(string s)
{
	int i, n, len, yy;
	len = strlen(s);
	for (i = 0; i <= len - 8; i++)
	{
		if (strcut(s, i + 2, i + 2) != ".") { continue; }
		if (strcut(s, i + 5, i + 5) != ".") { continue; }
		if (!MM_IsDigits(s, i, 2)) { continue; }
		if (!MM_IsDigits(s, i + 3, 2)) { continue; }
		if (!MM_IsDigits(s, i + 6, 2)) { continue; }
		yy = sti(strcut(s, i + 6, i + 7));
		if (i + 9 < len)
		{
			if (MM_IsDigits(s, i + 8, 2)) { yy = sti(strcut(s, i + 8, i + 9)); }		// a year of 4 digits (the GitHub release)
		}
		n = yy * 10000 + sti(strcut(s, i + 3, i + 4)) * 100 + sti(strcut(s, i, i + 1));
		if (n > 0) { return n; }
	}
	return -1;
}

void MM_InitVersionCheck()
{
	iMMUpdTime = 0;
	iMMUpdPoll = 0;
	MM_StartUpdateDownload();
	CreateString(true, "VerStatus", "", FONT_NORMAL, argb(255, 180, 180, 180), 415, MM_STATUS_Y, SCRIPT_ALIGN_RIGHT, 0.9);
	MM_UpdateVersionStatus();
}

// COOP s30: called at the start and then every MM_UPD_POLL ms (MM_CreditsFrame) until MM_UPD_WAIT: the answer of GitHub comes
// a few seconds after the start of the game
void MM_UpdateVersionStatus()
{
	int iLocal, iLatest, iCol;
	string sText;
	iLocal = MM_ParseDate(VERSION_NUMBER2);
	iLatest = MM_ParseDate(MM_GetLatestReleaseName());
	sText = "Проверка обновлений...";
	if (iMMUpdTime >= MM_UPD_WAIT) { sText = "Проверка обновлений недоступна"; }
	iCol = argb(255, 180, 180, 180);
	if (iLocal > 0 && iLatest > 0)
	{
		if (iLatest > iLocal)
		{
			sText = "Версия устарела";
			iCol = argb(255, 255, 90, 90);
		}
		else
		{
			sText = "Нет обновлений";
			iCol = argb(255, 110, 230, 110);
		}
	}
	GameInterface.strings.VerStatus = sText;
	ChangeStringColor("VerStatus", iCol);
}

// ============================================================================
// GPK_MP: credits line "Специальная благодарность: <name>" above the version string
// To add a person: raise MM_THANKS_COUNT and add one "case N" in MM_GetThanksText.
// ============================================================================
#define MM_THANKS_COUNT		3		// how many names
#define MM_THANKS_PERIOD	5000	// ms per name
#define MM_THANKS_FADE		600		// ms of fade in / fade out
#define MM_THANKS_X			550		// where the changing text starts (label ends 8 px left of it)
#define MM_THANKS_Y			558

int iMMThanksIdx = 0;
int iMMThanksTime = 0;
int iMMThanksAlpha = -1;

string MM_GetThanksText(int n)
{
	string sRes = "";
	switch (n)
	{
	case 0: sRes = "Разработка: Дамир Халилуллин (@tomaskdev)"; break;
	case 1: sRes = "Тестирование: Вы"; break;
	case 2: sRes = "Продам гараж"; break;
	}
	return sRes;
}

void MM_InitCredits()
{
	iMMThanksIdx = 0;
	iMMThanksTime = 0;
	iMMThanksAlpha = 0;
	CreateString(true, "ThanksLabel", "Специальная благодарность:", FONT_NORMAL, COLOR_NORMAL, MM_THANKS_X - 8, MM_THANKS_Y, SCRIPT_ALIGN_RIGHT, 0.9);
	CreateString(true, "ThanksText", MM_GetThanksText(0), FONT_NORMAL, argb(0, 255, 255, 255), MM_THANKS_X, MM_THANKS_Y, SCRIPT_ALIGN_LEFT, 0.9);
	SetEventHandler("frame", "MM_CreditsFrame", 0);
}

// ============================================================================
// COOP s30: the hero of the profile stands on the rope bridge, the camera flies around him slowly (back and forth: the scene
// is built for one side only). The 3D items of the menu fly with the camera, so they stay at their place on the screen.
// Needs the patched modules\XINTERFACE.dll (messages 50, 51, 52 of InterfaceBackScene, models "Characters\..."); the old dll
// ignores these messages: the hero then stands at the seagull and the camera does not move.
// bMMHeroScene = false: the old menu.
// ============================================================================
bool bMMHeroScene = true;

#define MM_HERO_X		72.15		// a point in the middle of the rope bridge (RESOURCE\MODELS\MainMenu\MainMenu.gm, "step1")
#define MM_HERO_Y		4.27
#define MM_HERO_Z		28.25
#define MM_HERO_PHI0	-0.729		// the direction from the hero to the camera of the menu
#define MM_CAM_PHI_C	-0.58		// the middle of the flight of the camera (the clear air around the bridge: MainMenu.gm)
#define MM_CAM_R		8.0			// m from the hero (nothing of MainMenu.gm between him and the camera on the whole flight)
#define MM_CAM_H		1.75		// m over his feet: the camera at the height of his face
#define MM_CAM_LOOK_H	1.4			// the camera looks at this height of the hero (almost level)
#define MM_CAM_SWING	0.6			// rad to each side
#define MM_CAM_PERIOD	75.0		// s for one swing there and back
#define MM_CAM_SHIFT	0.21		// rad: the hero to the right of the screen, the menu is on the left

float fMMCamT = 0.0;

// The hero is a model of the script (NetMMHero, net\Land.c) with the blade and the gun of the loadout of the profile, drawn
// inside the scene (with its lights). The code of the hero and of the ship is shared with the profile screens
// (net\MenuScene.c: NetMM_HeroSpawn, NetMM_ShipSpawn).
void MM_HeroCreate()
{
	MM_HeroDelete();
	if (!bMMHeroScene) { return; }
	int iShip = NetMM_HeroSpawn(&InterfaceBackScene, MM_HERO_PHI0);
	InterfaceBackScene.mmheroidx = 0;
	MM_ShipCreate(iShip);
	fMMCamT = 0.0;
	MM_HeroFrame(0);
}

void MM_HeroDelete()
{
	DeleteAttribute(&InterfaceBackScene, "mmheroidx");
	NetMM_HeroDelete(&InterfaceBackScene);
	MM_ShipDelete();
}

// COOP s30: the ship of the lobby of the profile stands at anchor in the bay, 60 m away on the left of the screen (behind the
// menu: the right side is all the big wreck, it hid the ship). net\MenuScene.c: NETMM_SHIP_*
void MM_ShipCreate(int iNetShip)
{
	// COOP s30: the look of the ship of the profile (NetMM_HeroSpawn read it: the sails upgrade, the color, the gerald, the flag)
	NetMM_ShipSpawn(iNetShip, bNetMMHeroSColor, iNetMMHeroSColor, sNetMMHeroGerald, iNetMMHeroSails, sNetMMHeroFlag);
}

void MM_ShipDelete()
{
	NetMM_ShipDelete();
}

// one item of the menu: its place in the frame of the camera (computed from MainMenu_locators.gm: camera, Menu02..Menu07)
void MM_HeroMenuItem(int i, float fRX, float fRY, float fRZ, float fCX, float fCY, float fCZ, aref vX, aref vY, aref vZ)
{
	float fPX = fCX + fRX * stf(vX.x) + fRY * stf(vY.x) + fRZ * stf(vZ.x);
	float fPY = fCY + fRX * stf(vX.y) + fRY * stf(vY.y) + fRZ * stf(vZ.y);
	float fPZ = fCZ + fRX * stf(vX.z) + fRY * stf(vY.z) + fRZ * stf(vZ.z);
	SendMessage(&InterfaceBackScene, "llffffffffffff", 51, i, stf(vX.mx), stf(vX.my), stf(vX.mz), stf(vY.mx), stf(vY.my), stf(vY.mz), stf(vZ.mx), stf(vZ.my), stf(vZ.mz), fPX, fPY, fPZ);
}

void MM_HeroFrame(int iDelta)
{
	if (!bMMHeroScene) { return; }
	if (!CheckAttribute(&InterfaceBackScene, "mmheroidx")) { return; }
	fMMCamT = fMMCamT + iDelta * 0.001;
	float fPhi = MM_CAM_PHI_C + MM_CAM_SWING * sin(fMMCamT * 6.28319 / MM_CAM_PERIOD);
	float fCX = MM_HERO_X + MM_CAM_R * sin(fPhi);
	float fCZ = MM_HERO_Z + MM_CAM_R * cos(fPhi);
	float fCY = MM_HERO_Y + MM_CAM_H + 0.2 * sin(fMMCamT * 0.31);
	float fDX = MM_HERO_X - fCX;
	float fDZ = MM_HERO_Z - fCZ;
	float fDY = MM_HERO_Y + MM_CAM_LOOK_H - fCY;
	float fH = sqrt(fDX * fDX + fDZ * fDZ);
	float fAY = atan2(fDX, fDZ) - MM_CAM_SHIFT;
	float fAX = atan2(-fDY, fH);			// > 0: down
	SendMessage(&InterfaceBackScene, "lffffff", 50, fCX, fCY, fCZ, fAX, fAY, 0.0);

	// the axes of the camera (vx right, vy up, vz forward) and of the items (their axes in the frame of the camera)
	float fSX = sin(fAX);
	float fKX = cos(fAX);
	float fSY = sin(fAY);
	float fKY = cos(fAY);
	object oV;
	oV.x.x = fKY;			oV.x.y = 0.0;			oV.x.z = -fSY;
	oV.y.x = fSY * fSX;		oV.y.y = fKX;			oV.y.z = fKY * fSX;
	oV.z.x = fSY * fKX;		oV.z.y = -fSX;			oV.z.z = fKY * fKX;
	aref vX;
	aref vY;
	aref vZ;
	makearef(vX, oV.x);
	makearef(vY, oV.y);
	makearef(vZ, oV.z);
	// the axes of the items: ax (0.99958, 0.0034, -0.02883), ay (-0.00464, 0.99907, -0.04282), az (0.02866, 0.04294, 0.99867)
	vX.mx = 0.99958 * fKY + 0.0034 * stf(vY.x) - 0.02883 * stf(vZ.x);
	vX.my = 0.0034 * fKX - 0.02883 * stf(vZ.y);
	vX.mz = -0.99958 * fSY + 0.0034 * stf(vY.z) - 0.02883 * stf(vZ.z);
	vY.mx = -0.00464 * fKY + 0.99907 * stf(vY.x) - 0.04282 * stf(vZ.x);
	vY.my = 0.99907 * fKX - 0.04282 * stf(vZ.y);
	vY.mz = 0.00464 * fSY + 0.99907 * stf(vY.z) - 0.04282 * stf(vZ.z);
	vZ.mx = 0.02866 * fKY + 0.04294 * stf(vY.x) + 0.99867 * stf(vZ.x);
	vZ.my = 0.04294 * fKX + 0.99867 * stf(vZ.y);
	vZ.mz = -0.02866 * fSY + 0.04294 * stf(vY.z) + 0.99867 * stf(vZ.z);
	MM_HeroMenuItem(0, -0.58091, 0.10986, 1.85801, fCX, fCY, fCZ, vX, vY, vZ);
	MM_HeroMenuItem(1, -0.46396, -0.03422, 1.82642, fCX, fCY, fCZ, vX, vY, vZ);
	MM_HeroMenuItem(2, -0.5539, -0.1661, 1.79876, fCX, fCY, fCZ, vX, vY, vZ);
	MM_HeroMenuItem(3, -0.5546, -0.3029, 1.76759, fCX, fCY, fCZ, vX, vY, vZ);
	MM_HeroMenuItem(4, -0.56319, -0.44011, 1.74117, fCX, fCY, fCZ, vX, vY, vZ);
	MM_HeroMenuItem(5, -0.54527, -0.57708, 1.70481, fCX, fCY, fCZ, vX, vY, vZ);
}

void MM_CreditsFrame()
{
	int a;
	MM_HeroFrame(GetDeltaTime());
	if (iMMUpdTime < MM_UPD_WAIT)		// COOP s30: the version check reads gpk_update.ini again while it waits for GitHub
	{
		iMMUpdTime = iMMUpdTime + GetDeltaTime();
		iMMUpdPoll = iMMUpdPoll + GetDeltaTime();
		if (iMMUpdPoll >= MM_UPD_POLL || iMMUpdTime >= MM_UPD_WAIT)
		{
			iMMUpdPoll = 0;
			MM_UpdateVersionStatus();
		}
	}
	iMMThanksTime = iMMThanksTime + GetDeltaTime();
	if (iMMThanksTime >= MM_THANKS_PERIOD)
	{
		iMMThanksTime = iMMThanksTime - MM_THANKS_PERIOD;
		iMMThanksIdx++;
		if (iMMThanksIdx >= MM_THANKS_COUNT) { iMMThanksIdx = 0; }
		GameInterface.strings.ThanksText = MM_GetThanksText(iMMThanksIdx);
	}
	a = 255;
	if (iMMThanksTime < MM_THANKS_FADE) { a = 255 * iMMThanksTime / MM_THANKS_FADE; }
	if (iMMThanksTime > MM_THANKS_PERIOD - MM_THANKS_FADE) { a = 255 * (MM_THANKS_PERIOD - iMMThanksTime) / MM_THANKS_FADE; }
	if (a != iMMThanksAlpha)
	{
		iMMThanksAlpha = a;
		ChangeStringColor("ThanksText", argb(a, 255, 255, 255));
	}
}
