// COOP s30: the 3D scene of the main menu (MainMenu.gm, the hero of the profile on the rope bridge, the ship of the profile at
// anchor) for the main menu (interface\mainmenu.c: the hero and the ship only) and behind the profile screens
// (net\interfaces\PlayerSettings.c and Buy.c in the mode of the profile: the whole scene, NetPSScene). The main menu deletes its
// own scene when the net menu opens (mainmenu.c: MultiPress), so the profile screen makes this one: the same weather
// (InterfaceStates.mainmenuweather), no menu items. Its camera flies between the hero (view 0) and the ship (view 1).
// Needs the patched XINTERFACE.dll (messages 50, 54, 55, 56 of InterfaceBackScene).
// The globals start at 0 / false / "" (the start values of the globals do not reach the game).

#define NETMM_HERO_X		72.15		// a point in the middle of the rope bridge (MainMenu.gm, "step1")
#define NETMM_HERO_Y		4.27
#define NETMM_HERO_Z		28.25
#define NETMM_HERO_PHI0		-0.729		// the direction from the hero to the camera of the menu (clear air: MainMenu.gm)
// the ship of the profile at anchor, 60 m from the hero in open water; its left side (east) looks at the open bay
#define NETMM_SHIP_X		128.4
#define NETMM_SHIP_Z		17.9
#define NETMM_SHIP_AY		3.37
#define NETMM_ORBIT0		1.8			// the camera of the ship view: east of the ship, at its left side
#define NETMM_ORBIT_MAX		1.3			// rad to each side (beyond it the wrecks of MainMenu.gm stand between)
#define NETMM_CAM_RATE		2.2			// 1/s: how fast the camera comes to its place
#define NETMM_ROT_STEP		0.785		// rad: one press of the arrows that turn the hero
#define NETMM_ORB_STEP		0.35		// rad: one press of the arrows that fly around the ship
#define NETMM_ZOOM_STEP		0.25		// one press of "+" / "-" (0 the whole hero / ship, 1 the face / close)
#define NETMM_SHIP_WL		0.075		// the waterline of the ship: the tan above the middle of the screen (just over the shop)
#define NETMM_PORT_D		1.05		// /portraits: the camera in front of the face (m)
#define NETMM_PORT_YAW		0.38		// /portraits: the hero turns a little (three quarters, like the portraits of the game)

object NetPSScene;
bool bNetPSScene;				// NetPSScene is made
object NetPSEdit;				// the ship settings of the profile that is edited (PlayerSettings.c <-> Buy.c)

// the ship of the menu
bool bNetMMShip;
int iNetMMShipChar;
int iNetMMShipReal;				// its slot of RealShips (freed with the ship: the list of the shop is clicked through)
int iNetMMShipNet;				// its index of NetShips
string sNetMMShipKey;			// what stands: "<NetShips index>|<sail color>|<gerald sail>|<sails upgrade>|<flag>"
string sNetMMShipWant;			// what the screen wants
int iNetMMShipWantNet;
int iNetMMShipWantColor;
string sNetMMShipWantGerald;
int iNetMMShipWantSails;
string sNetMMShipWantFlag;
int iNetMMShipWait;
// the look of the ship of the profile of the main menu (NetMM_HeroSpawn reads it from the profile, MM_ShipCreate uses it)
bool bNetMMHeroSColor;
int iNetMMHeroSColor;
string sNetMMHeroGerald;
int iNetMMHeroSails;
string sNetMMHeroFlag;

// /portraits (PlayerSettings: the name "/portraits" + "Предпросмотр"): every NPC skin in front of the camera, a screenshot
// each (XINTERFACE msg 57 = F8: Corsairs3_NNNN.tga in the folder of the game). _backup_s30\tools\make_portraits.py cuts them
bool bNetPortraits;
int iNetPortN;					// 0 the background (no hero), 1.. npc_N
int iNetPortT;
int iNetPortStage;				// 0 waits for the model, 1 the shot is asked
bool bNetPSNoSeagull;		// /portraits: the profile scene without the seagull (a string global left without a value
							// is not "" in this engine: the old sNetPSWeatherOvr took a wrong weather)

// the hero of the profile screen (net\Land.c: bNetMMHeroOvr, sNetMMFaceOvr, sNetMMNickOvr; NetMM_HeroInfo reads them)
string sNetMMHeroKey;
string sNetMMHeroWant;
int iNetMMHeroWait;
int iNetMMHeroGen;				// + 1: the loadout was changed, the hero is made again

// the camera of NetPSScene: the eye in polar coordinates around the ship (angle, distance, height), the point it looks at
int iNetPSView;					// 0 the hero, 1 the ship
bool bNetPSCamSet;
float fNetPSCamA;
float fNetPSCamR;
float fNetPSCamY;
float fNetPSTgtX;
float fNetPSTgtY;
float fNetPSTgtZ;
float fNetPSRot;				// the turn of the hero (rad from NETMM_HERO_PHI0)
float fNetPSRotGoal;
float fNetPSZoomGoal;			// the hero: 0 the whole hero .. 1 the face
float fNetPSOrbGoal;			// the ship: rad from NETMM_ORBIT0
float fNetPSSZoomGoal;			// the ship: 0 the whole ship .. 1 close
float fNetPSGoalA;				// the goal of the camera (NetPS_CamGoal)
float fNetPSGoalR;
float fNetPSGoalY;
float fNetPSGoalTX;
float fNetPSGoalTY;
float fNetPSGoalTZ;

// ================================================================================================
// the hero
// ================================================================================================
void NetMM_HeroSetYaw(float fA)
{
	if (!IsEntity(&NetMMHero)) { return; }
	SendMessage(&NetMMHero, "lffffffffffff", 20506, NETMM_HERO_X, NETMM_HERO_Y, NETMM_HERO_Z, cos(fA), 0.0, 0.0 - sin(fA), 0.0, 1.0, 0.0, sin(fA), 0.0, cos(fA));
}

// the hero of the profile on the rope bridge, drawn by the scene rScene (an InterfaceBackScene: message 55, with its lights),
// he looks along fYaw. Returns the ship of the profile (the index of NetShips, -1 none)
int NetMM_HeroSpawn(ref rScene, float fYaw)
{
	NetMM_HeroDelete(rScene);
	object oH;
	NetMM_HeroInfo(&oH);			// net\Land.c: the skin (in the armor of the loadout), the animation, the blade, the gun, the ship
	string sModel = oH.model;
	string sAni = oH.anim;
	CreateEntity(&NetMMHero, "MODELR");
	SetTexturePath(0, "Characters\");
	SendMessage(&NetMMHero, "ls", 20500, "Characters\" + sModel);
	SetTexturePath(0, "");
	SendMessage(&NetMMHero, "ls", 20501, "mm_" + sAni);			// RESOURCE\ANIMATION\mm_*.ani: the idle that loops
	NetMM_HeroSetYaw(fYaw);
	LayerAddObject("execute", &NetMMHero, 0);
	SendMessage(rScene, "lis", 54, &NetMMHero, "mm_idle");
	SendMessage(rScene, "lli", 55, 0, &NetMMHero);
	string sBlade = oH.blade;
	string sGun = oH.gun;
	if (sBlade != "" || sGun != "")
	{
		CreateEntity(&NetMMBlade, "blade");
		if (sBlade != "") { SendMessage(&NetMMBlade, "llisfll", 21000, 0, &NetMMHero, sBlade, 0.25, argb(64, 64, 64, 64), argb(0, 32, 32, 32)); }
		if (sGun != "") { SendMessage(&NetMMBlade, "lis", 21007, &NetMMHero, sGun); }
		// the musket of a skin without a musketeer twin: in the right hand (the patched blade.dll, like NetLand_MushHand)
		if (sti(oH.tie) == 1) { SendMessage(&NetMMBlade, "ll", 21011, NetMM_MushCode(sAni)); }
		LayerAddObject("execute", &NetMMBlade, 0);
		SendMessage(rScene, "lli", 55, 1, &NetMMBlade);
	}
	Trace("COOP: menu hero " + sModel + " (" + sAni + "), blade <" + sBlade + ">, gun <" + sGun + ">, tie " + oH.tie);
	bNetMMHeroSColor = sti(oH.hascolor);
	iNetMMHeroSColor = sti(oH.scolor);
	sNetMMHeroGerald = oH.gerald;
	iNetMMHeroSails = sti(oH.sails);
	sNetMMHeroFlag = oH.flag;
	return sti(oH.ship);
}

void NetMM_HeroDelete(ref rScene)
{
	if (IsEntity(rScene))
	{
		SendMessage(rScene, "ll", 56, 0);
		SendMessage(rScene, "ll", 56, 1);
	}
	if (IsEntity(&NetMMBlade)) { DeleteClass(&NetMMBlade); }
	if (IsEntity(&NetMMHero)) { DeleteClass(&NetMMHero); }
}

// ================================================================================================
// the ship of the profile at anchor: the net index (NetShips) goes to the ship type of the single player by its name.
// bColor: iColor is the color of the sails (argb); sGerald: the picture of the gerald sail of the profile (NET\Sails), "" none
// ================================================================================================
// the flag of the profile (net\flags\fN) as the nation of the ship: the ship of the menu is a ship of the single player, its
// flags are the flags of the nations (the net flag is a texture of the net client)
int NetMM_FlagNation(string sFlag)
{
	if (sFlag == "f1") { return HOLLAND; }
	if (sFlag == "f2") { return ENGLAND; }
	if (sFlag == "f3") { return FRANCE; }
	if (sFlag == "f5") { return SPAIN; }
	return PIRATE;
}

// the places of the gerald on the sails like the net game (client\Rigging.c: GeraldSails or NetGeraldSails of the net type)
void NetMM_ShipGeraldPlaces(ref refBase, int iNetShip)
{
	aref aSrc, aDst;
	if (CheckAttribute(&NetShips[iNetShip], "GeraldSails"))
	{
		makearef(aSrc, NetShips[iNetShip].GeraldSails);
	}
	else
	{
		if (!CheckAttribute(&NetShips[iNetShip], "NetGeraldSails")) { return; }
		makearef(aSrc, NetShips[iNetShip].NetGeraldSails);
	}
	refBase.GeraldSails = "";
	makearef(aDst, refBase.GeraldSails);
	CopyAttributes(aDst, aSrc);
	// the sails of the single player (rigging.dll) need the scale of the gerald: the net types often have none (the Manowar
	// without it crashed the game)
	aref aG;
	int iGn = GetAttributesNum(aDst);
	for (int i=0; i<iGn; i++)
	{
		aG = GetAttributeN(aDst, i);
		if (!CheckAttribute(aG, "hscale")) { aG.hscale = 0.9; }
		if (!CheckAttribute(aG, "vscale")) { aG.vscale = 0.9; }
	}
}

// iSails: the sails upgrade of the net game (1 common, 2 pat, 3 silk, 4 the Flying Dutchman), sFlag: the flag of the profile
void NetMM_ShipSpawn(int iNetShip, bool bColor, int iColor, string sGerald, int iSails, string sFlag)
{
	// another ship instead of the old one: the sails, the ropes and the flags (sea_ai\AIShip.c) are made again with it. The
	// single player never deletes one ship of them alone: the national flags of the old ship (the nation of the flag of the
	// profile) crashed the game at the next ship (the Flying Dutchman -> the Manowar)
	NetMM_ShipDelete();
	if (iNetShip < 0) { return; }
	if (iNetShipsNum <= 0) { Net_InitShips(); }		// net\common\Ships.c: only the table (the net is not started yet)
	if (iNetShip >= iNetShipsNum) { return; }
	string sName = NetShips[iNetShip].Name;
	int iType = -1;
	for (int i=0; i<SHIP_TYPES_QUANTITY; i++)
	{
		if (!CheckAttribute(&ShipsTypes[i], "name")) { continue; }
		if (ShipsTypes[i].name == sName) { iType = i; break; }
	}
	if (iType < 0) { Trace("COOP: menu ship " + sName + ": no such ship type"); return; }
	int iCh = GenerateCharacter(NetMM_FlagNation(sFlag), WITHOUT_SHIP, "citizen", MAN, 0, WARRIOR);
	ref rCh = &characters[iCh];
	int iReal = GenerateShip(iType, false);
	if (iReal == SHIP_NOTUSED) { Trace("COOP: menu ship " + sName + ": no free ship slot"); return; }
	rCh.ship.type = iReal;
	SetBaseShipData(rCh);
	ref refBase = GetRealShip(iReal);
	refBase.SpeedRate = 0.01;		// at anchor
	// the sails like the net game (client\Rigging.c): 1..3 the canvas of the upgrade with the gerald on the places of the net
	// type, 4 the torn sails of the Flying Dutchman without a gerald (the single player type has its own black sails)
	int iSl = iSails;
	string sGer = sGerald;
	if (iSl < 1 || iSl > 4) { iSl = 1; }
	DeleteAttribute(refBase, "ShipSails");
	DeleteAttribute(refBase, "EmblemedSails");
	refBase.ship.upgrades.sails = 1;
	if (iSl == 4)
	{
		refBase.EmblemedSails.normalTex = "ships\parus_common_torn.tga";
		sGer = "";
	}
	else
	{
		refBase.ship.upgrades.sails = iSl;
		// the places of the gerald of the single player type (with their scale); the net ones only when it has none
		if (!CheckAttribute(refBase, "GeraldSails")) { NetMM_ShipGeraldPlaces(refBase, iNetShip); }
	}
	if (bColor) { refBase.ShipSails.SailsColor = iColor; }
	// the gerald sail of the profile: battle_interface\BattleInterface.c (procGetSailTextureData) loads "Ships\Gerald\<name>.tga"
	if (sGer != "")
	{
		if (sGer != "_emptysail_") { refBase.ShipSails.gerald_name = "..\..\" + sUserSailsPath + sGer; }
	}
	Ship_SetFantomData(rCh);
	Ship_ClearImpulseStrength(rCh);
	Ship_SetLightsAndFlares(rCh);
	Ship_SetTrackSettings(rCh);
	rCh.Features.GeraldSails = true;
	rCh.Ship.Speed.z = 0.0;
	rCh.Ship.Pos.Mode = SHIP_SAIL;
	rCh.Ship.Pos.x = NETMM_SHIP_X;
	rCh.Ship.Pos.z = NETMM_SHIP_Z;
	rCh.Ship.Ang.y = NETMM_SHIP_AY;
	rCh.Ship.Strand = false;
	rCh.Ship.Strand.SndID = -1;
	rCh.Ship.LastBallCharacter = -1;
	rCh.Ship.WindDotShip = 1.0;
	rCh.TmpPerks = "";
	aref aShTmpPerks;
	makearef(aShTmpPerks, rCh.TmpPerks);
	aShTmpPerks.StormProfessional = 0;
	CreateEntity(rCh, "ship");
	SendMessage(rCh, "laa", MSG_SHIP_CREATE, &rCh, &refBase);
	bNetMMShip = true;
	iNetMMShipChar = iCh;
	iNetMMShipReal = iReal;
	iNetMMShipNet = iNetShip;
	Trace("COOP: menu ship " + sName + " (type " + iType + "), sails " + iSl + ", gerald <" + sGer + ">, flag <" + sFlag + ">");
}

void NetMM_ShipDelete()
{
	if (!bNetMMShip) { return; }
	bNetMMShip = false;
	// COOP s35: the sails, the ropes and the flags of the ship stay in the rigging entities after the ship is deleted (a mast
	// of sails hung in the air of the main menu): they are made again together with the ship, whoever deletes it
	bool bRig = IsEntity(&Sail);
	if (bRig) { DeleteRiggingEnvironment(); }
	DeleteClass(GetCharacter(iNetMMShipChar));
	characters[iNetMMShipChar].ship.type = SHIP_NOTUSED;
	DeleteAttribute(&RealShips[iNetMMShipReal], "");		// the slot is free again
	if (bRig) { CreateRiggingEnvironment(); }
}

// the length of the ship of the menu (m): the box of its hull (the ship entity writes Ship.BoxSize), else by its class
float NetMM_ShipLen()
{
	if (!bNetMMShip) { return 40.0; }
	ref rCh = GetCharacter(iNetMMShipChar);
	if (CheckAttribute(rCh, "Ship.BoxSize.z"))
	{
		float fZ = stf(rCh.Ship.BoxSize.z);
		if (fZ > 5.0) { return fZ * 1.15; }		// the bowsprit is not in the box of the hull
	}
	int iClass = sti(NetShips[iNetMMShipNet].Class);
	return 80.0 - 9.0 * iClass;
}

// ================================================================================================
// NetPSScene: the scene behind the profile screens
// ================================================================================================
void NetPS_SceneShow()
{
	LayerFreeze("realize", false);
	LayerFreeze("execute", false);
}

void NetPS_CreateWeather()
{
	// the weather of the main menu (interface\mainmenu.c: ICreateWeather), the same one while the game runs
	// COOP s30: the profile screens are always in a clear day "11 Hour" (the main menu keeps its own random weather:
	// interface\mainmenu.c, InterfaceStates.mainmenuweather is not touched)
	int n = FindWeather("11 Hour");
	if (n < 0 || n >= iTotalNumWeathers) { n = 0; }
	// weather\WhrWeather.c (CreateWeatherEnvironment) takes InterfaceStates.mainmenuweather while bMainMenu (it is on here,
	// NetPS_SceneEnsure): the day one for this scene, the weather of the main menu comes back after it
	bool bMMWhrHad = CheckAttribute(&InterfaceStates, "mainmenuweather");
	int iMMWhrOld = -1;
	if (bMMWhrHad) { iMMWhrOld = sti(InterfaceStates.mainmenuweather); }
	InterfaceStates.mainmenuweather = n;
	if (CheckAttribute(&Weathers[n], "Hour.Min")) { SetCurrentTime(makeint(stf(Weathers[n].Hour.Min)), 0); }
	// the Weather of the main menu is still alive here: Whr_LoadNextWeather(0) takes iCurWeatherNum (the weather of the menu,
	// a night one: a day sky over a dark light), so it is the day one before, and once more before CreateWeather
	iCurWeatherNum = n;
	SetNextWeather(Weathers[n].id);
	iBlendWeatherNum = -1;
	Whr_LoadNextWeather(0);
	iBlendWeatherNum = -1;
	NetPSScene.current_weather = n;

	CreateSea("execute", "realize");
	iCurWeatherNum = n;
	SetNextWeather(Weathers[n].id);
	CreateWeather("execute", "realize");
	CreateShipEnvironment();
	Sea.MaxSeaHeight = 1.0;
	Sea.isDone = "";

	CreateCoastFoamEnvironment("MainMenu", "execute", "realize");
	iBlendWeatherNum = -1;
	if (bMMWhrHad) { InterfaceStates.mainmenuweather = iMMWhrOld; }
	else { DeleteAttribute(&InterfaceStates, "mainmenuweather"); }
	Trace("COOP: profile scene weather " + Weathers[n].id + ", the weather made " + Weathers[iCurWeatherNum].id + ", rain " + makeint(bWeatherIsRain));
}

void NetPS_SceneEnsure()
{
	NetPS_SceneShow();
	if (bNetPSScene) { return; }
	Render.BackColor = 0;
	Render.SeaEffect = false;
	Sea.UnderWater = false;
	bMainCharacterInFire = false;
	bMainMenu = true;

	NetPS_CreateWeather();

	CreateEntity(&NetPSScene, "InterfaceBackScene");
	LayerAddObject("execute", &NetPSScene, -1);
	LayerAddObject("realize", &NetPSScene, 1000);
	SendMessage(&NetPSScene, "ls", 0, "MainMenu\MainMenu");		// the model
	SendMessage(&NetPSScene, "ls", 1, "camera");					// the camera (message 50 moves it then)
	if (Whr_IsNight())
	{
		NetPSScene.light.turnon = true;
		NetPSScene.light.model = "mainmenu\Fonar_night";
		NetPSScene.light.lightcolormin = argb(0, 200, 200, 120);
		NetPSScene.light.lightcolormax = argb(48, 255, 255, 180);
		NetPSScene.light.colorperiod = 0.4;
		NetPSScene.light.addcolorperiod = 1.0;
		NetPSScene.light.rangemin = 10.0;
		NetPSScene.light.rangemax = 14.0;
		NetPSScene.light.rangeperiod = 1.5;
		NetPSScene.light.locator = "Light";
		NetPSScene.light.lightlocator = "fonar";
		NetPSScene.light.flarelocator = "fire";
		NetPSScene.light.flaresize = 0.5;
		NetPSScene.light.minflarecolor = 120.0;
		NetPSScene.light.maxflarecolor = 200.0;
	}
	else
	{
		if (!bNetPSNoSeagull)
		{
			NetPSScene.seagull.locator = "seagull";
			NetPSScene.seagull.model = "mainmenu\seagull";
			NetPSScene.seagull.animation = "seagull";
			NetPSScene.seagull.aniaction = "idle";
			SendMessage(&NetPSScene, "ls", 9, "seagull");
		}
		NetPSScene.light.turnon = false;
		NetPSScene.light.model = "mainmenu\Fonar_day";
		NetPSScene.light.locator = "Light";
		NetPSScene.light.lightlocator = "fonar";
	}
	SendMessage(&NetPSScene, "ls", 8, "light");
	bMainMenu = false;

	bNetPSScene = true;
	bNetPSCamSet = false;
	iNetPSView = 0;
	fNetPSRot = 0.0;
	fNetPSRotGoal = 0.0;
	fNetPSZoomGoal = 0.0;
	fNetPSOrbGoal = 0.0;
	fNetPSSZoomGoal = 0.0;
	sNetMMHeroKey = "";
	sNetMMShipKey = "";
	iNetMMHeroWait = 0;
	iNetMMShipWait = 0;
	SetEventHandler("frame", "NetPS_Frame", 0);
	Trace("COOP: profile scene made");
}

// the profile screens go to the net menu (or to a preview): the scene goes away
void NetPS_SceneDelete()
{
	bNetMMHeroOvr = false;
	bNetPortraits = false;
	if (!bNetPSScene) { return; }
	bNetPSScene = false;
	DelEventHandler("frame", "NetPS_Frame");
	NetMM_HeroDelete(&NetPSScene);
	NetMM_ShipDelete();
	LayerDelObject("execute", &NetPSScene);
	LayerDelObject("realize", &NetPSScene);
	DeleteShipEnvironment();
	DeleteWeather();
	DeleteSea();
	DeleteClass(&NetPSScene);
	DeleteAttribute(&NetPSScene, "");
	DeleteCoastFoamEnvironment();
	iNextWeatherNum = -1;
	iCurWeatherHour = -1;
	Trace("COOP: profile scene deleted");
}

// the screen says what it shows: the hero of the face sFace with the loadout of the profile sNick
void NetPS_WantHero(string sFace, string sNick)
{
	sNetMMFaceOvr = sFace;
	sNetMMNickOvr = sNick;
	sNetMMHeroWant = sFace + "|" + sNick + "|" + iNetMMHeroGen;
}

// the ship of the shop (the index of NetShips), the color of the sails (argb), the gerald sail (NET\Sails)
void NetPS_WantShip(int iNetShip, int iColor, string sGerald, int iSails, string sFlag)
{
	iNetMMShipWantNet = iNetShip;
	iNetMMShipWantColor = iColor;
	sNetMMShipWantGerald = sGerald;
	iNetMMShipWantSails = iSails;
	sNetMMShipWantFlag = sFlag;
	sNetMMShipWant = "" + iNetShip + "|" + iColor + "|" + sGerald + "|" + iSails + "|" + sFlag;
}

// the ship of the profile sNick as it is saved (the ship of the shop of the profile) with the sails of NetPSEdit
void NetPS_WantProfShip(string sNick)
{
	int iShip = -1;
	if (CheckAttribute(&NCProfiles, "NetShip." + sNick + ".Ship")) { iShip = sti(NCProfiles.NetShip.(sNick).Ship); }
	int iRig = 1;
	if (CheckAttribute(&NCProfiles, "NetShip." + sNick + ".Rig")) { iRig = sti(NCProfiles.NetShip.(sNick).Rig) + 1; }
	string sFlg = "";
	if (CheckAttribute(&NetPSEdit, "ImageFlag")) { sFlg = NetPSEdit.ImageFlag; }
	NetPS_WantShip(iShip, NetPS_EditSailColor(), NetPS_EditGerald(), iRig, sFlg);
}

int NetPS_EditSailColor()
{
	int iC = 0;
	if (CheckAttribute(&NetPSEdit, "ColorSail")) { iC = sti(NetPSEdit.ColorSail); }
	if (iC < 0 || iC >= SAILS_COLOR_QUANTITY) { iC = 0; }
	return sti(SailsColors[iC].color);
}

string NetPS_EditGerald()
{
	if (CheckAttribute(&NetPSEdit, "ImageSail")) { return NetPSEdit.ImageSail; }
	return "";
}

void NetPS_SetView(int iView)
{
	iNetPSView = iView;
}

// the arrows and the "+" / "-" of the screens: the hero (view 0) turns, the camera flies around the ship (view 1)
void NetPS_Turn(int iDir)
{
	if (iNetPSView == 0)
	{
		fNetPSRotGoal = fNetPSRotGoal + iDir * NETMM_ROT_STEP;
		return;
	}
	fNetPSOrbGoal = fNetPSOrbGoal + iDir * NETMM_ORB_STEP;
	if (fNetPSOrbGoal > NETMM_ORBIT_MAX) { fNetPSOrbGoal = NETMM_ORBIT_MAX; }
	if (fNetPSOrbGoal < 0.0 - NETMM_ORBIT_MAX) { fNetPSOrbGoal = 0.0 - NETMM_ORBIT_MAX; }
}

void NetPS_Zoom(int iDir)
{
	if (iNetPSView == 0)
	{
		fNetPSZoomGoal = fNetPSZoomGoal + iDir * NETMM_ZOOM_STEP;
		if (fNetPSZoomGoal > 1.0) { fNetPSZoomGoal = 1.0; }
		if (fNetPSZoomGoal < 0.0) { fNetPSZoomGoal = 0.0; }
		return;
	}
	// the ship: -1 (twice as far as the start) .. 0 (the start: the whole ship) .. 1 (twice as close as before)
	fNetPSSZoomGoal = fNetPSSZoomGoal + iDir * NETMM_ZOOM_STEP;
	if (fNetPSSZoomGoal > 1.0) { fNetPSSZoomGoal = 1.0; }
	if (fNetPSSZoomGoal < -1.0) { fNetPSSZoomGoal = -1.0; }
}

// the eye at the world point (x, y, z): its polar coordinates around the ship (the camera flies on them: from the hero to the
// ship it goes around the north of the ship over open water, not through it)
void NetPS_GoalEye(float fX, float fY, float fZ)
{
	float fDX = fX - NETMM_SHIP_X;
	float fDZ = fZ - NETMM_SHIP_Z;
	fNetPSGoalA = atan2(fDX, fDZ);
	fNetPSGoalR = sqrt(fDX * fDX + fDZ * fDZ);
	fNetPSGoalY = fY;
}

void NetPS_CamGoal()
{
	float fD, fZm;
	if (iNetPSView == 0)
	{
		// the hero in the middle of the screen between the panels (0) .. his face almost close (1). The camera stays on the line
		// NETMM_HERO_PHI0 (no wreck between it and the hero), the hero turns
		fZm = fNetPSZoomGoal;
		fD = 4.2 - 3.45 * fZm;
		NetPS_GoalEye(NETMM_HERO_X + fD * sin(NETMM_HERO_PHI0), NETMM_HERO_Y + 1.30 + 0.36 * fZm, NETMM_HERO_Z + fD * cos(NETMM_HERO_PHI0));
		fNetPSGoalTX = NETMM_HERO_X;
		fNetPSGoalTY = NETMM_HERO_Y + 1.17 + 0.47 * fZm;
		fNetPSGoalTZ = NETMM_HERO_Z;
		return;
	}
	// the side of the ship, the whole ship (the masts too) in the free upper part of the screen over the panels of the shop
	float fL = NetMM_ShipLen();
	fZm = fNetPSSZoomGoal;
	// 0: 3 L (the start), 1: 0.525 L (it was 1.05 L), -1: 6 L
	if (fZm >= 0.0) { fD = 3.0 * fL * (1.0 - 0.825 * fZm); }
	else { fD = 3.0 * fL * (1.0 - fZm); }
	fNetPSGoalA = NETMM_ORBIT0 + fNetPSOrbGoal;
	fNetPSGoalR = fD;
	float fEyeY = 0.4 * fL * (1.0 - 0.4 * fZm) + 2.0;
	fNetPSGoalY = fEyeY;
	// the waterline of the ship stays on one line of the screen just over the panels of the shop: closer = a bigger ship,
	// its masts go out at the top, the hull is never behind the panels
	float fPitch = atan2(fEyeY - 1.0, fD) + atan2(NETMM_SHIP_WL, 1.0);
	fNetPSGoalTX = NETMM_SHIP_X;
	fNetPSGoalTY = fEyeY - fD * sin(fPitch) / cos(fPitch);
	fNetPSGoalTZ = NETMM_SHIP_Z;
}

void NetPS_CamApply()
{
	float fCX = NETMM_SHIP_X + fNetPSCamR * sin(fNetPSCamA);
	float fCZ = NETMM_SHIP_Z + fNetPSCamR * cos(fNetPSCamA);
	float fCY = fNetPSCamY;
	float fDX = fNetPSTgtX - fCX;
	float fDZ = fNetPSTgtZ - fCZ;
	float fDY = fNetPSTgtY - fCY;
	float fH = sqrt(fDX * fDX + fDZ * fDZ);
	float fAY = atan2(fDX, fDZ);
	float fAX = atan2(0.0 - fDY, fH);			// > 0: down
	SendMessage(&NetPSScene, "lffffff", 50, fCX, fCY, fCZ, fAX, fAY, 0.0);
}

void NetPS_Frame()
{
	if (!bNetPSScene) { return; }
	if (bNetPortraits) { NetPS_PortFrame(); return; }
	int iDt = GetDeltaTime();
	float fDt = iDt * 0.001;
	if (fDt > 0.1) { fDt = 0.1; }
	if (fDt < 0.0) { fDt = 0.0; }

	// the hero (again when the face, the nick or the loadout changes: a short wait, the list of the faces is clicked through)
	if (sNetMMHeroWant != sNetMMHeroKey)
	{
		iNetMMHeroWait = iNetMMHeroWait + iDt;
		if (iNetMMHeroWait >= 200 || sNetMMHeroKey == "")
		{
			iNetMMHeroWait = 0;
			sNetMMHeroKey = sNetMMHeroWant;
			bNetMMHeroOvr = true;
			NetMM_HeroSpawn(&NetPSScene, NETMM_HERO_PHI0 + fNetPSRot);
		}
	}
	// the ship
	if (sNetMMShipWant != sNetMMShipKey)
	{
		iNetMMShipWait = iNetMMShipWait + iDt;
		if (iNetMMShipWait >= 250 || sNetMMShipKey == "")
		{
			iNetMMShipWait = 0;
			sNetMMShipKey = sNetMMShipWant;
			NetMM_ShipSpawn(iNetMMShipWantNet, true, iNetMMShipWantColor, sNetMMShipWantGerald, iNetMMShipWantSails, sNetMMShipWantFlag);
		}
	}

	float fK = fDt * NETMM_CAM_RATE;
	if (fK > 1.0) { fK = 1.0; }
	// the turn of the hero
	float fKR = fDt * 4.0;
	if (fKR > 1.0) { fKR = 1.0; }
	fNetPSRot = fNetPSRot + (fNetPSRotGoal - fNetPSRot) * fKR;
	NetMM_HeroSetYaw(NETMM_HERO_PHI0 + fNetPSRot);

	NetPS_CamGoal();
	if (!bNetPSCamSet)
	{
		// the first frame: the camera starts where the camera of the main menu was (interface\mainmenu.c: the middle of
		// its flight, R 8, H 1.75, the hero right of the middle of the screen) and flies to its place
		bNetPSCamSet = true;
		float fPhi = -0.58;
		float fEX = NETMM_HERO_X + 8.0 * sin(fPhi);
		float fEZ = NETMM_HERO_Z + 8.0 * cos(fPhi);
		NetPS_GoalEye(fEX, NETMM_HERO_Y + 1.75, fEZ);
		fNetPSCamA = fNetPSGoalA;
		fNetPSCamR = fNetPSGoalR;
		fNetPSCamY = fNetPSGoalY;
		float fLook = atan2(NETMM_HERO_X - fEX, NETMM_HERO_Z - fEZ) - 0.21;
		fNetPSTgtX = fEX + 8.0 * sin(fLook);
		fNetPSTgtY = NETMM_HERO_Y + 1.4;
		fNetPSTgtZ = fEZ + 8.0 * cos(fLook);
		NetPS_CamGoal();
	}
	fNetPSCamA = fNetPSCamA + (fNetPSGoalA - fNetPSCamA) * fK;
	fNetPSCamR = fNetPSCamR + (fNetPSGoalR - fNetPSCamR) * fK;
	fNetPSCamY = fNetPSCamY + (fNetPSGoalY - fNetPSCamY) * fK;
	fNetPSTgtX = fNetPSTgtX + (fNetPSGoalTX - fNetPSTgtX) * fK;
	fNetPSTgtY = fNetPSTgtY + (fNetPSGoalTY - fNetPSTgtY) * fK;
	fNetPSTgtZ = fNetPSTgtZ + (fNetPSGoalTZ - fNetPSTgtZ) * fK;
	NetPS_CamApply();
}

// ================================================================================================
// /portraits: the icons of the NPC skins (they have no portrait). The scene in the day weather, no seagull; the camera stands in
// front of the face, every skin (npc_1 .. npc_N, without the loadout) gets a screenshot, the first one is the background alone
// (make_portraits.py finds the head by the difference with it). The interface hides itself (PlayerSettings: PS_Portraits*)
// ================================================================================================
void NetPS_PortraitsStart()
{
	bNetPSNoSeagull = true;
	NetPS_SceneDelete();
	NetPS_SceneEnsure();
	bNetPSNoSeagull = false;
	bNetPortraits = true;
	iNetPortN = 0;
	iNetPortT = 0;
	iNetPortStage = 0;
	Trace("COOP: portraits: start, " + NetLand_NpcCount() + " skins");
}

void NetPS_PortraitsEnd()
{
	bNetPortraits = false;
	NetMM_HeroDelete(&NetPSScene);
	sNetMMHeroKey = "";				// the hero of the profile comes back
	bNetPSCamSet = false;
	Trace("COOP: portraits: done");
	Event("NetPS_PortraitsDone");
}

void NetPS_PortCam()
{
	float fEX = NETMM_HERO_X + NETMM_PORT_D * sin(NETMM_HERO_PHI0);
	float fEZ = NETMM_HERO_Z + NETMM_PORT_D * cos(NETMM_HERO_PHI0);
	NetPS_GoalEye(fEX, NETMM_HERO_Y + 1.60, fEZ);
	fNetPSCamA = fNetPSGoalA;
	fNetPSCamR = fNetPSGoalR;
	fNetPSCamY = fNetPSGoalY;
	fNetPSTgtX = NETMM_HERO_X;
	fNetPSTgtY = NETMM_HERO_Y + 1.50;
	fNetPSTgtZ = NETMM_HERO_Z;
	NetPS_CamApply();
}

void NetPS_PortFrame()
{
	iNetPortT = iNetPortT + GetDeltaTime();
	NetPS_PortCam();
	if (iNetPortStage == 0)
	{
		int iWait = 700;			// the model and its idle settle
		if (iNetPortN == 0) { iWait = 2500; }		// the scene, the sea and the sky; the player moves the mouse away
		if (iNetPortT < iWait) { return; }
		SendMessage(&NetPSScene, "l", 57);		// XINTERFACE s30d: the screenshot of this frame
		string sShot = "background";
		if (iNetPortN > 0) { sShot = "npc_" + iNetPortN; }
		Trace("COOP: portrait shot " + sShot);
		iNetPortStage = 1;
		iNetPortT = 0;
		return;
	}
	if (iNetPortT < 200) { return; }		// the file is written at the end of the frame
	iNetPortN++;
	if (iNetPortN > NetLand_NpcCount()) { NetPS_PortraitsEnd(); return; }
	bNetMMHeroOvr = true;
	sNetMMFaceOvr = "npc_" + iNetPortN;
	sNetMMNickOvr = "~portrait~";			// no profile: the bare skin, no weapons
	NetMM_HeroSpawn(&NetPSScene, NETMM_HERO_PHI0 + NETMM_PORT_YAW);
	iNetPortStage = 0;
	iNetPortT = 0;
}
