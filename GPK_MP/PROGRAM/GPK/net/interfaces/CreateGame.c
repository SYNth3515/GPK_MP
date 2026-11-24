int iGameType;
// COOP s27: the lists of the maps (the islands, the land locations) are sorted by the name: CS_SortLandRows
#define CS_SORT_MAX 800
string sCSSortName[CS_SORT_MAX];
string sCSSortLoc[CS_SORT_MAX];
int iCSSortKey0[CS_SORT_MAX];
int iCSSortKey1[CS_SORT_MAX];
int iCSSortKey2[CS_SORT_MAX];
int iCSSortKey3[CS_SORT_MAX];
int iCSSortIdx[CS_SORT_MAX];
int iCSSortTmp[CS_SORT_MAX];
int iCSSortNum = 0;
bool bCSDlOnly = false;		// COOP s47: the button "Загрузки": only the maps of RESOURCE\Downloads in the lists (net\NetDl.c)
int iCSSortFort[CS_SORT_MAX];		// the islands: 1 = it has forts (the list of the fort capture)
int CS_GAMETYPE_LAND = 4;		// COOP: land duel (net\Land.c): the 5th game type, a deathmatch in a location
int iMaxNumPlayers, iTimeLimit, iStartPosition, iCredit, iMaxShipClass;
float fGameSpeed = 1.0;	// COOP: the game speed of the server: 0.5 / 0.75 / 1 / 2 ... 8, the same values as in the admin menu
int iGameTime = 0; 
int iWeatherType = 0;
int iNumTeams = 2;
int iLandTeams = 1;		// COOP: land duel: 1 = everybody against everybody (the usual duel), 2 = two teams (team deathmatch)
int iBotSkill = 2;		// COOP: difficulty of the bots 0 harmless, 1 easy, 2 medium, 3 hard (server\Bots.c)
int iRespawnTime = 30;		// COOP: seconds between the death and the respawn (slider SBAR_RESPAWN, shown only with respawn on)

int iCsSelLog = 0;		// COOP: traces of the first events of the interface
// COOP: a slider tells its change with an event of the next frame, also when the script has moved it. The events of the
// moves at the start (the default values, then the ones of the last game) came after each other, each answer moved the
// slider again and made a new event: the two values took turns without end (the game hung). Now the answer to a slider
// never moves it, the events of the first moment are not taken and the event of a move by the script is skipped
bool bCsSlidersMute = true;		// the first moment of the interface: the events of the sliders are the ones of the script
int iCsSlidersMuted = 0;
float fCsCreditSet = -1.0;		// the position the script gave to the slider (its event is skipped); -1 = none
float fCsRespawnSet = -1.0;

void InitInterface(string iniName)
{
	GameInterface.title = "titleNetCreateGame";

	GameInterface.TABLE_GAMETYPE.tr1.td1.str = XI_ConvertString("txtCS_" + NetModes[NETGAME_DEATHMATCH].Name);
	GameInterface.TABLE_GAMETYPE.tr2.td1.str = XI_ConvertString("txtCS_" + NetModes[NETGAME_NAVAL].Name);
	GameInterface.TABLE_GAMETYPE.tr3.td1.str = XI_ConvertString("txtCS_" + NetModes[NETGAME_CONVOY].Name);
	GameInterface.TABLE_GAMETYPE.tr4.td1.str = XI_ConvertString("txtCS_" + NetModes[NETGAME_FORT].Name);
	GameInterface.TABLE_GAMETYPE.tr5.td1.str = XI_ConvertString("txtCS_LandDuel");

	GameInterface.TABLE_GAMETIME.tr1.td1.str = XI_ConvertString("txtCS_Morning");
	GameInterface.TABLE_GAMETIME.tr2.td1.str = XI_ConvertString("txtCS_Day");
	GameInterface.TABLE_GAMETIME.tr3.td1.str = XI_ConvertString("txtCS_Evening");
	GameInterface.TABLE_GAMETIME.tr4.td1.str = XI_ConvertString("txtCS_Night");

	GameInterface.TABLE_WEATHER.tr1.td1.str = XI_ConvertString("txtCS_Calm");
	GameInterface.TABLE_WEATHER.tr2.td1.str = XI_ConvertString("txtCS_Windy");
	GameInterface.TABLE_WEATHER.tr3.td1.str = XI_ConvertString("txtCS_Heavy");
	GameInterface.TABLE_WEATHER.tr4.td1.str = XI_ConvertString("txtCS_Storm");
	GameInterface.TABLE_WEATHER.tr5.td1.str = XI_ConvertString("txtCS_Tornado");

	SendMessage(&GameInterface, "ls", MSG_INTERFACE_INIT, iniName);

	// COOP: default server name, so a server can be created quickly
	GameInterface.EDITBOX_GAMENAME.str = "GPK_MP";
	GameInterface.EDITBOX_ADMINPASSWORD.str = "";

	// select checkbox LAN game; COOP: the Internet switch is on by default too
	CheckButton_SetState("CHECKBUTTON_LAN", 1, true);
	CheckButton_SetState("CHECKBUTTON_INET", 1, true);

	// P2P: server port (default 29020) and the P2P switch, which appears together with the Internet switch
	GameInterface.EDITBOX_SERVERPORT.str = "" + NET_DEFAULT_SERVER_PORT;
	CheckButton_SetState("CHECKBUTTON_P2P", 1, true);
	SetNodeUsing("CHECKBUTTON_P2P", true);

	// COOP: new switches, all off by default (old behaviour; the brander needs "All active perks")
	CheckButton_SetState("CHECKBUTTON_RESPAWN", 1, false);
	CheckButton_SetState("CHECKBUTTON_NOBOARDING", 1, false);
	CheckButton_SetState("CHECKBUTTON_MELEEONLY", 1, false);
	SetNodeUsing("CHECKBUTTON_MELEEONLY", false);
	CheckButton_SetState("CHECKBUTTON_PASSIVEPERKS", 1, false);
	CheckButton_SetState("CHECKBUTTON_LATEJOIN", 1, true);		// COOP s30: on by default
	CheckButton_SetState("CHECKBUTTON_PISTOLRELOAD", 1, false);		// COOP s31: a land duel: the pistols load in the fight
	// COOP s37: the fast boarding (the old auto boarding) is on by default; off: the captains fight on the deck (net\Board.c)
	CheckButton_SetState("CHECKBUTTON_FASTBOARDING", 1, true);
	CheckButton_SetState("CHECKBUTTON_ACTIVEPERKS", 1, false);

	iMaxNumPlayers = NET_MAXPLAYERS;		
	fGameSpeed = 1.0;
	iTimeLimit = 0;
	iStartPosition = 0;
	iCredit = 300000;
	iMaxShipClass = 1;
	
	CS_SetNumTeams();
	CS_SetNumPlayers();
	CS_SetGameSpeed();
	CS_SetTimeLimit();
	CS_SetStartPosition();
	// COOP: the sliders are moved once: with the values of the last game (CS_LoadLast) or with the default ones
	bool bCsLast = CheckAttribute(&NCProfiles, "LastCreate.GameType");
	if (bCsLast) { CS_ShowCredit(); }
	else { CS_SetCredit(); }
	CS_SetMaxShipClass();
	CS_SetBotSkill();

	iRespawnTime = NET_RESPAWN_DELAY / 1000;
	if (bCsLast) { CS_ShowRespawnText(); }
	else { CS_SetRespawnTime(); }
	CS_ShowRespawnTime(false);		// the respawn switch is off by default

	SetEventHandler("OnCreate", "OnCreate", 0);

	SetEventHandler("OnCancel", "OnCancel", 0);

	SetEventHandler("TableSelectChange", "CS_TableSelectChange", 0);
	SetEventHandler("eSlideChange", "CS_SlideChange", 0);	

	SetEventHandler("AddPlayer", "CS_AddPlayer", 0);
	SetEventHandler("DecPlayer", "CS_DecPlayer", 0);

	SetEventHandler("AddGameSpeed", "CS_AddGameSpeed", 0);
	SetEventHandler("DecGameSpeed", "CS_DecGameSpeed", 0);

	SetEventHandler("AddTimeLimit", "CS_AddTimeLimit", 0);
	SetEventHandler("DecTimeLimit", "CS_DecTimeLimit", 0);

	SetEventHandler("AddStartPosition", "CS_AddStartPosition", 0);
	SetEventHandler("DecStartPosition", "CS_DecStartPosition", 0);

	SetEventHandler("AddCredit", "CS_AddCredit", 0);
	SetEventHandler("DecCredit", "CS_DecCredit", 0);

	SetEventHandler("AddTeams", "CS_AddTeams", 0);
	SetEventHandler("DecTeams", "CS_DecTeams", 0);

	SetEventHandler("AddMaxShipClass", "CS_AddMaxShipClass", 0);
	SetEventHandler("DecMaxShipClass", "CS_DecMaxShipClass", 0);
	SetEventHandler("AddBotSkill", "CS_AddBotSkill", 0);
	SetEventHandler("DecBotSkill", "CS_DecBotSkill", 0);

	SetEventHandler("CheckButtonChange", "CS_CheckButtonChange", 0);
	SetEventHandler("OnDownloads", "CS_OnDownloads", 0);		// COOP s47

	NetDl_Scan();		// COOP s47: the maps of RESOURCE\Downloads
	FillIslandsList();

	// disable OK window
	XI_WindowShow("WND_OK", false);
	XI_WindowDisable("WND_OK", true);

	// disable RECREATE window
	XI_WindowShow("WND_RECREATE", false);
	XI_WindowDisable("WND_RECREATE", true);

	CS_LoadLast();		// COOP: the settings of the last game created
	CS_EditModeInit();		// COOP s30: "Game settings" of the lobby
	SetEventHandler("CS_SlidersOn", "CS_SlidersOn", 0);
	PostEvent("CS_SlidersOn", 300);
	Trace("COOP: create game: InitInterface done");

	//CheckButton_SetDisable("CHECKBUTTON_INET", 1, true);
}

// COOP s30: the screen edits the game of the running server (the lobby: "Game settings"). The name and the passwords are
// those of the server; the port and the LAN / Internet switches can not change without a new server
void CS_EditModeInit()
{
	if (!bNetCSEdit) { return; }
	if (!NetServer_IsStarted()) { bNetCSEdit = false; return; }
	GameInterface.EDITBOX_GAMENAME.str = NetServer.ServerName;
	GameInterface.EDITBOX_GAMEPASSWORD.str = NetServer.ServerPassword;
	GameInterface.EDITBOX_ADMINPASSWORD.str = NetServer.AdminPassword;
	GameInterface.EDITBOX_SERVERPORT.str = "" + sti(NetServer.ServerPort);
	SetNodeUsing("EDITBOX_SERVERPORT", false);
	SetNodeUsing("CAPTION_SERVERPORT", false);
	SetNodeUsing("CHECKBUTTON_LAN", false);
	SetNodeUsing("CHECKBUTTON_INET", false);
	SetNodeUsing("CHECKBUTTON_P2P", false);
	if (CheckAttribute(&NetServer, "LateJoin")) { CheckButton_SetState("CHECKBUTTON_LATEJOIN", 1, sti(NetServer.LateJoin)); }
	if (CheckAttribute(&NetServer, "PistolReload")) { CheckButton_SetState("CHECKBUTTON_PISTOLRELOAD", 1, sti(NetServer.PistolReload)); }
	if (CheckAttribute(&NetServer, "FastBoarding")) { CheckButton_SetState("CHECKBUTTON_FASTBOARDING", 1, sti(NetServer.FastBoarding)); }
	Button_SetText("BTN_CREATE", "#Применить");
	Trace("COOP: create game screen: game settings of the running server");
}

int CS_IsShift(int iShift, int iNoShift, int iAlt)
{
	if (XI_IsKeyPressed("shift")) { return iShift; }
	if (XI_IsKeyPressed("alt")) { return iAlt; }
	return iNoShift;
}

void CS_SetNumTeams()
{
	if (iLandTeams > 2) { iLandTeams = 2; }
	if (iLandTeams < 1) { iLandTeams = 1; }
	if (iNumTeams > 4) { iNumTeams = 4; }
	if (iNumTeams < 2) { iNumTeams = 2; }

	switch (iGameType)
	{
		case NETGAME_DEATHMATCH:	StringCollection_SetText("STRINGS_VALUE", 1, "#-");				break;
		case NETGAME_NAVAL:			StringCollection_SetTextValue("STRINGS_VALUE", 1, iNumTeams);	break;
		case NETGAME_CONVOY:		StringCollection_SetText("STRINGS_VALUE", 1, "#2");				break;
		case NETGAME_FORT:			StringCollection_SetText("STRINGS_VALUE", 1, "#2");				break;
	}
	// the land duel: 1 team = the usual duel ("-"), 2 = a team duel
	if (iGameType == CS_GAMETYPE_LAND)
	{
		if (iLandTeams >= 2) { StringCollection_SetTextValue("STRINGS_VALUE", 1, iLandTeams); }
		else { StringCollection_SetText("STRINGS_VALUE", 1, "#-"); }		// "#" alone shows "(null)"
	}
}
void CS_AddTeams() { if (iGameType == CS_GAMETYPE_LAND) { iLandTeams++; } else { iNumTeams++; } CS_SetNumTeams(); }
void CS_DecTeams() { if (iGameType == CS_GAMETYPE_LAND) { iLandTeams--; } else { iNumTeams--; } CS_SetNumTeams(); }

void CS_SetNumPlayers()
{
	if (iMaxNumPlayers > NET_MAXPLAYERS) { iMaxNumPlayers = NET_MAXPLAYERS; }
	if (iMaxNumPlayers < 2) { iMaxNumPlayers = 2; }

	StringCollection_SetTextValue("STRINGS_VALUE", 2, iMaxNumPlayers);
	//Gameinterface.strings.MaxPlayers = iMaxNumPlayers;
}
void CS_AddPlayer() { iMaxNumPlayers++; CS_SetNumPlayers(); }
void CS_DecPlayer() { iMaxNumPlayers--; CS_SetNumPlayers(); }

// COOP: 1x / 2x / ... / 8x and below 1x the step is 0.25 (0.75x, 0.5x): the same ladder as in the admin menu
void CS_SetGameSpeed()
{
	if (fGameSpeed > 8.0) { fGameSpeed = 8.0; }
	if (fGameSpeed < 0.5) { fGameSpeed = 0.5; }

	StringCollection_SetText("STRINGS_VALUE", 3, "#" + NetClient_AdminSpeedText(fGameSpeed) + "x");
}
void CS_AddGameSpeed()
{
	if (fGameSpeed < 1.0) { fGameSpeed = fGameSpeed + 0.25; }	// 0.5 -> 0.75 -> 1
	else { fGameSpeed = fGameSpeed + 1.0; }
	CS_SetGameSpeed();
}
void CS_DecGameSpeed()
{
	if (fGameSpeed <= 1.0) { fGameSpeed = fGameSpeed - 0.25; }	// 1 -> 0.75 -> 0.5
	else { fGameSpeed = fGameSpeed - 1.0; }
	CS_SetGameSpeed();
}

void CS_SetTimeLimit()
{
	if (iTimeLimit <= 0) 
	{ 
		iTimeLimit = 0; 
		StringCollection_SetText("STRINGS_VALUE", 4, "#нет");		// no limit ("#" alone shows "(null)")
	}
	else
	{
		StringCollection_SetTextValue("STRINGS_VALUE", 4, iTimeLimit);
	}
	//Gameinterface.strings.TimeLimit = iTimeLimit;
}
void CS_AddTimeLimit() { iTimeLimit = iTimeLimit + 5; CS_SetTimeLimit(); }
void CS_DecTimeLimit() { iTimeLimit = iTimeLimit - 5; CS_SetTimeLimit(); }

void CS_SetStartPosition()
{
	if (iStartPosition < 0) { iStartPosition = 0; }
	if (iStartPosition > 2) { iStartPosition = 2; }
	StringCollection_SetText("STRINGS_VALUE", 5, "#" + XI_ConvertString("txtNetCS_StartPosition" + iStartPosition));
	//Gameinterface.strings.StartPosition = XI_ConvertString("txtNetCS_StartPosition" + iStartPosition);
}
void CS_AddStartPosition() { iStartPosition++; CS_SetStartPosition(); }
void CS_DecStartPosition() { iStartPosition--; CS_SetStartPosition(); }

void CS_ShowCredit()
{
    if (iCredit < 1000) { iCredit = 1000; }
    if (iCredit > 1000000) { iCredit = 1000000; }
    StringCollection_SetTextValue("STRINGS_VALUE", 6, iCredit);
}

void CS_SetCredit()
{
    CS_ShowCredit();

    float fCredit = (iCredit - 1000.0) / 999000.0;

    GameInterface.nodes.SBAR_CREDIT.value = fCredit;
    fCsCreditSet = fCredit;

    SendMessage(&GameInterface, "lslf",
        MSG_INTERFACE_MSG_TO_NODE, "SBAR_CREDIT", 0, fCredit);
}

void CS_AddCredit()
{
    iCredit += 1000 * CS_IsShift(10, 1, 100);
    CS_SetCredit();
}

void CS_DecCredit()
{
    iCredit -= 1000 * CS_IsShift(10, 1, 100);
    CS_SetCredit();
}

void CS_SlideChange()
{
    string sNodeName = GetEventData();
    int nVal = GetEventData();
    float fVal = GetEventData();
    if (iCsSelLog < 60) { iCsSelLog++; Trace("COOP: create game: slider " + sNodeName + " " + fVal); }
    if (sNodeName != "SBAR_RESPAWN" && sNodeName != "SBAR_CREDIT") { return; }

    // COOP: the first moment: the events of the moves of the script (they are not taken; after 20 of them the sliders
    // work anyway, if the event of the end of the first moment did not come)
    if (bCsSlidersMute)
    {
        iCsSlidersMuted++;
        if (iCsSlidersMuted < 20) { return; }
        bCsSlidersMute = false;
    }

    if (fVal < 0.0) { fVal = 0.0; }
    if (fVal > 1.0) { fVal = 1.0; }
    float fDiff = 0.0;

    // COOP: respawn time slider
    if (sNodeName == "SBAR_RESPAWN")
    {
        // the event of the position given by the script (the slider rounds it a little) is skipped
        if (fCsRespawnSet >= 0.0)
        {
            fDiff = fVal - fCsRespawnSet;
            if (fDiff > -0.02 && fDiff < 0.02) { return; }
            fCsRespawnSet = -1.0;
        }
        iRespawnTime = NET_RESPAWN_TIME_MIN + makeint(fVal * (NET_RESPAWN_TIME_MAX - NET_RESPAWN_TIME_MIN) + 0.5);
        CS_ShowRespawnText();		// only the text: the slider is where the player put it
        return;
    }

    if (fCsCreditSet >= 0.0)
    {
        fDiff = fVal - fCsCreditSet;
        if (fDiff > -0.02 && fDiff < 0.02) { return; }
        fCsCreditSet = -1.0;
    }
    iCredit = 1000 + makeint(fVal * 999000.0);
    iCredit = (iCredit / 1000) * 1000;		// whole thousands
    if (iCredit < 1000) { iCredit = 1000; }
    CS_ShowCredit();		// only the text
}

// COOP: the end of the first moment of the interface: the sliders are taken from now on
void CS_SlidersOn()
{
    bCsSlidersMute = false;
    Trace("COOP: create game: the sliders work");
}

// COOP: respawn time (seconds) - text and slider position
void CS_ShowRespawnText()
{
    if (iRespawnTime < NET_RESPAWN_TIME_MIN) { iRespawnTime = NET_RESPAWN_TIME_MIN; }
    if (iRespawnTime > NET_RESPAWN_TIME_MAX) { iRespawnTime = NET_RESPAWN_TIME_MAX; }

    SetFormatedText("TEXT_RESPAWN_TIME", XI_ConvertString("capNetCS_Respawn") + ": " + iRespawnTime + " сек.");
}

void CS_SetRespawnTime()
{
    CS_ShowRespawnText();

    float fPos = MakeFloat(iRespawnTime - NET_RESPAWN_TIME_MIN) / MakeFloat(NET_RESPAWN_TIME_MAX - NET_RESPAWN_TIME_MIN);
    GameInterface.nodes.SBAR_RESPAWN.value = fPos;
    fCsRespawnSet = fPos;
    SendMessage(&GameInterface, "lslf", MSG_INTERFACE_MSG_TO_NODE, "SBAR_RESPAWN", 0, fPos);
}

// the slider and its text are shown only while the respawn switch is on
void CS_ShowRespawnTime(bool bShow)
{
    SetNodeUsing("SBAR_RESPAWN", bShow);
    SetNodeUsing("TEXT_RESPAWN_TIME", bShow);
}

void CS_SetMaxShipClass()
{
	if (iMaxShipClass < 1) { iMaxShipClass = 1; }
	if (iMaxShipClass > 7) { iMaxShipClass = 7; }
	StringCollection_SetTextValue("STRINGS_VALUE", 7, iMaxShipClass);
	//Gameinterface.strings.MaxShipClass = iMaxShipClass;
}
// COOP: difficulty of the bots
void CS_SetBotSkill()
{
	if (iBotSkill < 0) { iBotSkill = 0; }
	if (iBotSkill > 3) { iBotSkill = 3; }
	StringCollection_SetText("STRINGS_VALUE", 8, "#" + XI_ConvertString("txtCS_BotSkill" + iBotSkill));
}
void CS_AddBotSkill() { iBotSkill++; CS_SetBotSkill(); }
void CS_DecBotSkill() { iBotSkill--; CS_SetBotSkill(); }

void CS_AddMaxShipClass() { iMaxShipClass++; CS_SetMaxShipClass(); }
void CS_DecMaxShipClass() { iMaxShipClass--; CS_SetMaxShipClass(); }

// COOP s47: the rows of a table are made again (the switch "Загрузки")
void CS_ClearRows(string sTable, int iFrom)
{
	for (int i=iFrom; i<=CS_SORT_MAX + 2; i++)
	{
		string sRow = "tr" + i;
		if (!CheckAttribute(&GameInterface, sTable + "." + sRow)) { break; }
		DeleteAttribute(&GameInterface, sTable + "." + sRow);
	}
}

// COOP s47: the button "Загрузки" / "Все карты"
void CS_OnDownloads()
{
	NetDl_Scan();
	bCSDlOnly = !bCSDlOnly;
	if (bCSDlOnly) { SendMessage(&GameInterface, "lsls", MSG_INTERFACE_MSG_TO_NODE, "BTN_DOWNLOADS", 0, "btnNetCS_AllMaps"); }
	else { SendMessage(&GameInterface, "lsls", MSG_INTERFACE_MSG_TO_NODE, "BTN_DOWNLOADS", 0, "btnNetCS_Downloads"); }
	FillIslandsList();
}

string CS_IslandPicture(string sIslId)
{
	if (NetDl_IsKey(sIslId)) { return NetDl_Picture(); }
	return "Interfaces\Net\Islands\" + sIslId + ".tga";
}

void FillIslandsList()
{
	int iForts = 1;
	CS_ClearRows("TABLE_MAPLIST", 1);		// COOP s47
	CS_ClearRows("TABLE_MAPLIST_FORT", 1);
	CS_ClearRows("TABLE_MAPLIST_LAND", 1);

	int iSeaFirst = 1;		// COOP s47: "Загрузки": only the downloaded maps, no open sea
	if (!bCSDlOnly)
	{
		GameInterface.TABLE_MAPLIST.tr1.td1.str = XI_ConvertString("OpenSea");
		GameInterface.TABLE_MAPLIST.tr1.td1.IslandID = "";
		GameInterface.TABLE_MAPLIST.tr1.td1.my_texture = "Interfaces\Net\Islands\OpenSea.tga";
		iSeaFirst = 2;
	}
	GameInterface.TABLE_MAPLIST.select = 1;

	GameInterface.TABLE_MAPLIST_FORT.select = 1;

	// COOP s27: the islands in the order of the names (the open sea is always the first row)
	iCSSortNum = 0;
	for (int i=0; i<Net_GetIslandsNum(); i++)
	{
		if (bCSDlOnly) { break; }		// COOP s47: only the downloaded maps (below)
		ref rIsland = Net_GetIslandByIndex(i);
		if (iCSSortNum >= CS_SORT_MAX) { break; }
		sCSSortName[iCSSortNum] = Net_GetIslandName(rIsland.ID);
		sCSSortLoc[iCSSortNum] = rIsland.ID;
		iCSSortFort[iCSSortNum] = 0;
		if (Net_IsIslandHaveForts(rIsland)) { iCSSortFort[iCSSortNum] = 1; }
		iCSSortNum++;
	}
	// COOP s47: the sea maps of RESOURCE\Downloads (net\NetDl.c: they are put into an island slot when the game starts)
	for (int iDl=0; iDl<NetDl_MapsNum(); iDl++)
	{
		if (NetDl_MapTypeN(iDl) != "sea") { continue; }
		if (iCSSortNum >= CS_SORT_MAX) { break; }
		sCSSortName[iCSSortNum] = NetDl_MapNameN(iDl);
		sCSSortLoc[iCSSortNum] = NetDl_MapKeyN(iDl);
		iCSSortFort[iCSSortNum] = 0;
		iCSSortNum++;
	}
	CS_SortLandRows();
	for (int iSorted=0; iSorted<iCSSortNum; iSorted++)
	{
		int iSrc = iCSSortIdx[iSorted];
		string sIslId = sCSSortLoc[iSrc];
		string sTexture = CS_IslandPicture(sIslId);

		string sTest = "tr" + (iSorted + iSeaFirst);
		GameInterface.TABLE_MAPLIST.(sTest).td1.str = sCSSortName[iSrc];
		GameInterface.TABLE_MAPLIST.(sTest).td1.IslandID = sIslId;
		GameInterface.TABLE_MAPLIST.(sTest).td1.my_texture = sTexture;

		if (iCSSortFort[iSrc] != 0)
		{
			sTest = "tr" + iForts;
			GameInterface.TABLE_MAPLIST_FORT.(sTest).td1.str = sCSSortName[iSrc];
			GameInterface.TABLE_MAPLIST_FORT.(sTest).td1.IslandID = sIslId;
			GameInterface.TABLE_MAPLIST_FORT.(sTest).td1.my_texture = sTexture;
			iForts++;
		}
	}

	// COOP: land duel (net\Land.c): the last rows of the list are locations on land
	CS_AddLandRows("TABLE_MAPLIST_LAND", 1);
	GameInterface.TABLE_MAPLIST_LAND.select = 1;
	GameInterface.TABLE_MAPLIST_LAND.top = 0;		// COOP s47: the list starts from its first row (the last game may have scrolled it)
	GameInterface.TABLE_MAPLIST.top = 0;
	GameInterface.TABLE_MAPLIST_FORT.top = 0;
	Table_UpdateWindow("TABLE_MAPLIST_LAND");

	Table_UpdateWindow("TABLE_MAPLIST");
	Table_UpdateWindow("TABLE_MAPLIST_FORT");

	CS_SelectNewGameType();
}

// COOP: a row of a land duel location (it exists in this build: FindLocation)

int CS_AddLandRow(string sTable, int iRow, string sName, string sLoc)
{
	if (iCSSortNum < CS_SORT_MAX)
	{
		sCSSortName[iCSSortNum] = sName;
		sCSSortLoc[iCSSortNum] = sLoc;
		iCSSortNum++;
	}
	return 1;
}

void CS_WriteLandRow(string sTable, int iRow, string sName, string sLoc)
{
	string sRow = "tr" + iRow;
	GameInterface.(sTable).(sRow).td1.str = sName;
	GameInterface.(sTable).(sRow).td1.IslandID = "";
	GameInterface.(sTable).(sRow).td1.LandLoc = sLoc;
	GameInterface.(sTable).(sRow).td1.my_texture = NetLand_LoadingImage(sLoc);		// the loading screen of the location
}

// COOP s27: the land locations are sorted by the name the player sees: cyrillic (a..ya), then latin (a..z); the case is ignored,
// the spaces and the signs go before the digits, the digits before the letters

// 1 = a space / a sign, 2..11 = a digit, 20.. = a cyrillic letter, 60.. = a latin letter (0 = the end of the name)
int CS_SortCharRank(string sCh)
{
	if (sCh == "") { return 0; }
	int iR = findSubStr("0123456789", sCh, 0);
	if (iR >= 0) { return 2 + iR; }
	iR = findSubStr("абвгдеёжзийклмнопрстуфхцчшщъыьэюя", sCh, 0);
	if (iR >= 0) { return 20 + iR; }
	iR = findSubStr("АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ", sCh, 0);
	if (iR >= 0) { return 20 + iR; }
	iR = findSubStr("abcdefghijklmnopqrstuvwxyz", sCh, 0);
	if (iR >= 0) { return 60 + iR; }
	iR = findSubStr("ABCDEFGHIJKLMNOPQRSTUVWXYZ", sCh, 0);
	if (iR >= 0) { return 60 + iR; }
	return 1;
}

// the ranks of the 4 letters from iFrom in one number (7 bits each)
int CS_SortKey(string sName, int iFrom)
{
	int iLen = strlen(sName);
	int iKey = 0;
	for (int k=0; k<4; k++)
	{
		int iRank = 0;
		if (iFrom + k < iLen) { iRank = CS_SortCharRank(GetSymbol(sName, iFrom + k)); }
		iKey = iKey * 128 + iRank;
	}
	return iKey;
}

// < 0: the row iA goes first
int CS_SortCmp(int iA, int iB)
{
	if (iCSSortKey0[iA] != iCSSortKey0[iB]) { return iCSSortKey0[iA] - iCSSortKey0[iB]; }
	if (iCSSortKey1[iA] != iCSSortKey1[iB]) { return iCSSortKey1[iA] - iCSSortKey1[iB]; }
	if (iCSSortKey2[iA] != iCSSortKey2[iB]) { return iCSSortKey2[iA] - iCSSortKey2[iB]; }
	if (iCSSortKey3[iA] != iCSSortKey3[iB]) { return iCSSortKey3[iA] - iCSSortKey3[iB]; }
	string sA = sCSSortName[iA];
	string sB = sCSSortName[iB];
	int iLA = strlen(sA);
	int iLB = strlen(sB);
	int iN = iLA;
	if (iLB < iN) { iN = iLB; }
	for (int i=16; i<iN; i++)
	{
		int iRA = CS_SortCharRank(GetSymbol(sA, i));
		int iRB = CS_SortCharRank(GetSymbol(sB, i));
		if (iRA != iRB) { return iRA - iRB; }
	}
	return iLA - iLB;
}

// the merge sort of iCSSortIdx (stable: the equal names keep their order)
void CS_SortLandRows()
{
	int i, iLeft, iMid, iRight, iA, iB, iOut;
	bool bTakeA;
	for (i=0; i<iCSSortNum; i++)
	{
		iCSSortIdx[i] = i;
		iCSSortKey0[i] = CS_SortKey(sCSSortName[i], 0);
		iCSSortKey1[i] = CS_SortKey(sCSSortName[i], 4);
		iCSSortKey2[i] = CS_SortKey(sCSSortName[i], 8);
		iCSSortKey3[i] = CS_SortKey(sCSSortName[i], 12);
	}
	int iWidth = 1;
	while (iWidth < iCSSortNum)
	{
		iLeft = 0;
		while (iLeft < iCSSortNum)
		{
			iMid = iLeft + iWidth;
			if (iMid > iCSSortNum) { iMid = iCSSortNum; }
			iRight = iLeft + iWidth * 2;
			if (iRight > iCSSortNum) { iRight = iCSSortNum; }
			iA = iLeft;
			iB = iMid;
			iOut = iLeft;
			while (iOut < iRight)
			{
				bTakeA = false;
				if (iB >= iRight) { bTakeA = true; }
				else
				{
					if (iA < iMid)
					{
						if (CS_SortCmp(iCSSortIdx[iA], iCSSortIdx[iB]) <= 0) { bTakeA = true; }
					}
				}
				if (bTakeA) { iCSSortTmp[iOut] = iCSSortIdx[iA]; iA++; }
				else { iCSSortTmp[iOut] = iCSSortIdx[iB]; iB++; }
				iOut++;
			}
			iLeft = iRight;
		}
		for (i=0; i<iCSSortNum; i++) { iCSSortIdx[i] = iCSSortTmp[i]; }
		iWidth = iWidth * 2;
	}
}

void CS_AddLandRows(string sTable, int iFirstRow)
{
	iCSSortNum = 0;
	int iLandRow = iFirstRow;
	// COOP s47: the land maps of RESOURCE\Downloads (net\NetDl.c: they are put into the slot of the TDM locations)
	int iDlLand = 0;
	for (int iDl=0; iDl<NetDl_MapsNum(); iDl++)
	{
		if (NetDl_MapTypeN(iDl) != "land") { continue; }
		iLandRow = iLandRow + CS_AddLandRow(sTable, iLandRow, NetDl_MapNameN(iDl), NetDl_MapKeyN(iDl));
		iDlLand++;
	}
	if (bCSDlOnly)		// COOP s47: "Загрузки": only the downloaded maps (an empty list when there are none)
	{
		CS_SortLandRows();
		for (int iDlSorted=0; iDlSorted<iCSSortNum; iDlSorted++)
		{
			int iDlSrc = iCSSortIdx[iDlSorted];
			CS_WriteLandRow(sTable, iFirstRow + iDlSorted, sCSSortName[iDlSrc], sCSSortLoc[iDlSrc]);
		}
		return;
	}
	iLandRow = iLandRow + CS_AddLandRow(sTable, iLandRow, "Дуэль: каюта", "Cabin");
	iLandRow = iLandRow + CS_AddLandRow(sTable, iLandRow, "Дуэль: средняя каюта", "Cabin_Medium");
	iLandRow = iLandRow + CS_AddLandRow(sTable, iLandRow, "Дуэль: большая каюта", "Cabin_Huge");
	iLandRow = iLandRow + CS_AddLandRow(sTable, iLandRow, "Дуэль: палуба (малая)", "BOARDING_SMALL_DECK");
	iLandRow = iLandRow + CS_AddLandRow(sTable, iLandRow, "Дуэль: палуба (средняя)", "BOARDING_MEDIUM_DECK");
	iLandRow = iLandRow + CS_AddLandRow(sTable, iLandRow, "Дуэль: палуба (большая)", "BOARDING_BIG_DECK");
	iLandRow = iLandRow + CS_AddLandRow(sTable, iLandRow, "Корабль: кают-компания (кубрик)", "My_Campus");
	iLandRow = iLandRow + CS_AddLandRow(sTable, iLandRow, "Корабль: орудийная палуба", "My_Deck_Medium");
	iLandRow = iLandRow + CS_AddLandRow(sTable, iLandRow, "Корабль: трюм", "My_Deck");
	iLandRow = iLandRow + CS_AddLandRow(sTable, iLandRow, "Каюта: малая", "Cabin_Small");
	iLandRow = iLandRow + CS_AddLandRow(sTable, iLandRow, "Каюта: Флайинг Датчман", "CabineFDM");
	iLandRow = iLandRow + CS_AddLandRow(sTable, iLandRow, "Каюта: квестовая", "Cabin_Quest");
	iLandRow = iLandRow + CS_AddLandRow(sTable, iLandRow, "Палуба своего корабля (малая)", "Ship_deck_Low");
	iLandRow = iLandRow + CS_AddLandRow(sTable, iLandRow, "Палуба своего корабля (средняя)", "Ship_deck_Medium");
	iLandRow = iLandRow + CS_AddLandRow(sTable, iLandRow, "Палуба своего корабля (большая)", "Ship_deck_Big");
	iLandRow = iLandRow + CS_AddLandRow(sTable, iLandRow, "Палуба своего корабля (квестовая)", "Ship_deck");
	iLandRow = iLandRow + CS_AddLandRow(sTable, iLandRow, "Палуба у борта, шлюпка (малая)", "Deck_Near_Ship_Low");
	iLandRow = iLandRow + CS_AddLandRow(sTable, iLandRow, "Палуба у борта, шлюпка (средняя)", "Deck_Near_Ship_Medium");
	iLandRow = iLandRow + CS_AddLandRow(sTable, iLandRow, "Палуба у борта, шлюпка (большая)", "Deck_Near_Ship_Big");
	iLandRow = iLandRow + CS_AddLandRow(sTable, iLandRow, "Палуба у борта, шлюпка (квестовая)", "Deck_Near_Ship");
	iLandRow = iLandRow + CS_AddLandRow(sTable, iLandRow, "Под водой: Город потерянных кораблей", "UnderWater");
	iLandRow = iLandRow + CS_AddLandRow(sTable, iLandRow, "Под водой: храм Теночтитлана (левый)", "Temple_Underwater_left");
	iLandRow = iLandRow + CS_AddLandRow(sTable, iLandRow, "Под водой: храм Теночтитлана (правый)", "Temple_Underwater_right");

	// all the other locations of the game: towns, jungles, shores, forts, caves, dungeons, shops, shipyards, taverns,
	// brothels, churches, residences... (the ones without a way in or out, the decks / cabins above and the underwater are left out)
	for (int i=0; i<nLocationsNum; i++)
	{
		if (!CheckAttribute(&Locations[i], "id")) { continue; }
		string sLocId = Locations[i].id;
		if (sLocId == "") { continue; }
		if (sLocId == "My_Campus" || sLocId == "My_Deck_Medium" || sLocId == "My_Deck") { continue; }
		if (findSubStr(sLocId, "OldWorld_", 0) == 0) { continue; }		// COOP s27: the models of OldWorld are not in this build (a dark screen)
		if (CheckAttribute(&Locations[i], "boarding")) { continue; }
		if (CheckAttribute(&Locations[i], "CabinType")) { continue; }
		if (CheckAttribute(&Locations[i], "underwater")) { if (sti(Locations[i].underwater)) { continue; } }
		if (!CheckAttribute(&Locations[i], "reload")) { continue; }
		aref arLocReload;
		makearef(arLocReload, Locations[i].reload);
		if (GetAttributesNum(arLocReload) <= 0) { continue; }

		string sLocLabel = sLocId;
		if (CheckAttribute(&Locations[i], "id.label")) { sLocLabel = GetConvertStr(Locations[i].id.label, "LocLables.txt"); }
		string sLocName = sLocLabel;
		int iUnder = findSubStr(sLocId, "_", 0);
		if (iUnder > 0)
		{
			string sLocTown = strcut(sLocId, 0, iUnder - 1);
			sLocName = sLocTown + ": " + sLocLabel;
		}
		if (sLocId == "Clone_location") { sLocName = NetLand_LocName(sLocId); }		// COOP s27: "Ферма"
		iLandRow = iLandRow + CS_AddLandRow(sTable, iLandRow, sLocName, sLocId);
	}

	// COOP s27: the locations of the addon TDM (net\LandTdm.c: they are put into a slot when the duel starts)
	for (int iTdm=0; iTdm<NetTdm_Count(); iTdm++)
	{
		iLandRow = iLandRow + CS_AddLandRow(sTable, iLandRow, NetTdm_NameN(iTdm), NetTdm_KeyN(iTdm));
	}

	// COOP s27: the rows in the order of the names
	CS_SortLandRows();
	for (int iSorted=0; iSorted<iCSSortNum; iSorted++)
	{
		int iSrc = iCSSortIdx[iSorted];
		CS_WriteLandRow(sTable, iFirstRow + iSorted, sCSSortName[iSrc], sCSSortLoc[iSrc]);
	}
}

void CS_SelectNewIsland()
{
	string sControl = CS_MapControl();
	string sRow = "tr" + sti(GameInterface.(sControl).select);
	string sTexture = NetDl_Picture();		// COOP s47: an empty list ("Загрузки" without maps)
	if (CheckAttribute(&GameInterface, sControl + "." + sRow + ".td1.my_texture")) { sTexture = GameInterface.(sControl).(sRow).td1.my_texture; }

	// COOP: the picture is loaded only when it is another one
	bool bNewPic = true;
	if (CheckAttribute(&GameInterface, "CS_MapTexture")) { if (GameInterface.CS_MapTexture == sTexture) { bNewPic = false; } }
	if (bNewPic)
	{
		GameInterface.CS_MapTexture = sTexture;
		Picture_SetPicture("SERVER_MAP_PICTURE", sTexture);
	}

	// COOP: land duel: no ships - the class of the ships is hidden
	bool bLand = false;
	if (CS_GetCurrentLand() != "") { bLand = true; }
	SetNodeUsing("SBAR_MAXSHIPCLASS", !bLand);
	SetNodeUsing("CAPTION_MAXSHIPCLASS", !bLand);
	if (bLand) { StringCollection_SetText("STRINGS_VALUE", 7, "# "); }		// "" and "#" show (null)
	else { CS_SetMaxShipClass(); }
	// "No boarding" is "Melee only" in a land duel (no pistols): the same value, another caption
	bool bWasLand = false;
	if (CheckAttribute(&GameInterface, "CS_MeleeShown")) { bWasLand = sti(GameInterface.CS_MeleeShown); }
	if (bLand != bWasLand)
	{
		if (bLand) { CheckButton_SetState("CHECKBUTTON_MELEEONLY", 1, CS_GetCheck("CHECKBUTTON_NOBOARDING")); }
		else { CheckButton_SetState("CHECKBUTTON_NOBOARDING", 1, CS_GetCheck("CHECKBUTTON_MELEEONLY")); }
	}
	GameInterface.CS_MeleeShown = bLand;
	SetNodeUsing("CHECKBUTTON_NOBOARDING", !bLand);
	SetNodeUsing("CHECKBUTTON_MELEEONLY", bLand);
	// COOP s37: the pistols in the fight on land: a land duel and the real boarding (a row of its own now, the class of the ships
	// is shown at sea); the fast boarding is a switch of the sea
	SetNodeUsing("CHECKBUTTON_PISTOLRELOAD", true);
	SetNodeUsing("CHECKBUTTON_FASTBOARDING", !bLand);
}

void CS_SelectNewGameType()
{
	iGameType = sti(GameInterface.TABLE_GAMETYPE.select) - 1;

	SetNodeUsing("TABLE_MAPLIST", false);
	SetNodeUsing("SCROLL_MAPLIST", false);
	SetNodeUsing("TABLE_MAPLIST_FORT", false);
	SetNodeUsing("SCROLL_MAPLIST_FORT", false);
	SetNodeUsing("TABLE_MAPLIST_LAND", false);
	SetNodeUsing("SCROLL_MAPLIST_LAND", false);

	string sMapControl = CS_MapControl();
	SetNodeUsing(sMapControl, true);
	if (sMapControl == "TABLE_MAPLIST") { SetNodeUsing("SCROLL_MAPLIST", true); }
	if (sMapControl == "TABLE_MAPLIST_FORT") { SetNodeUsing("SCROLL_MAPLIST_FORT", true); }
	if (sMapControl == "TABLE_MAPLIST_LAND") { SetNodeUsing("SCROLL_MAPLIST_LAND", true); }

	CS_SelectNewIsland();
	CS_SetNumTeams();
}

void CS_TableSelectChange()
{
	string sControl = GetEventData();
	int iSelected = GetEventData();
	if (iCsSelLog < 60) { iCsSelLog++; Trace("COOP: create game: " + sControl + " row " + iSelected); }
	
	if (sControl == "TABLE_GAMETYPE") { CS_SelectNewGameType(); return; }
	if (sControl == "TABLE_MAPLIST" || sControl == "TABLE_MAPLIST_FORT") { CS_SelectNewIsland(); return; }
	if (sControl == "TABLE_MAPLIST_LAND") { CS_SelectNewIsland(); return; }

	if (sControl == "TABLE_GAMETIME") { iGameTime = iSelected - 1; return; }
	if (sControl == "TABLE_WEATHER") { iWeatherType = iSelected - 1; return; }
}

void IDoExit(int exitCode)
{
	DelEventHandler("OnCreate", "OnCreate");
	DelEventHandler("OnCancel", "OnCancel");

	DelEventHandler("TableSelectChange", "CS_TableSelectChange");

	DelEventHandler("AddPlayer", "CS_AddPlayer");
	DelEventHandler("DecPlayer", "CS_DecPlayer");

	DelEventHandler("AddGameSpeed", "CS_AddGameSpeed");
	DelEventHandler("DecGameSpeed", "CS_DecGameSpeed");

	DelEventHandler("AddTimeLimit", "CS_AddTimeLimit");
	DelEventHandler("DecTimeLimit", "CS_DecTimeLimit");

	DelEventHandler("AddStartPosition", "CS_AddStartPosition");
	DelEventHandler("DecStartPosition", "CS_DecStartPosition");

	DelEventHandler("AddCredit", "CS_AddCredit");
	DelEventHandler("DecCredit", "CS_DecCredit");

	DelEventHandler("AddTeams", "CS_AddTeams");
	DelEventHandler("DecTeams", "CS_DecTeams");

	DelEventHandler("AddMaxShipClass", "CS_AddMaxShipClass");
	DelEventHandler("DecMaxShipClass", "CS_DecMaxShipClass");
	DelEventHandler("AddBotSkill", "CS_AddBotSkill");
	DelEventHandler("DecBotSkill", "CS_DecBotSkill");

	DelEventHandler("CheckButtonChange", "CS_CheckButtonChange");
	DelEventHandler("OnDownloads", "CS_OnDownloads");		// COOP s47
	DelEventHandler("eSlideChange", "CS_SlideChange");
	DelEventHandler("CS_SlidersOn", "CS_SlidersOn");

	interfaceResultCommand = exitCode;
	EndCancelInterface(true);
}

void CS_CheckButtonChange()
{
	string sNode = GetEventData();
	int iButtonNumber = GetEventData();
	int bChecked = GetEventData();
	if (iCsSelLog < 60) { iCsSelLog++; Trace("COOP: create game: switch " + sNode + " " + bChecked); }
	
	// COOP: respawn time slider next to the respawn switch
	if (sNode == "CHECKBUTTON_RESPAWN")
	{
		CS_ShowRespawnTime(bChecked);
		if (bChecked) { CS_SetRespawnTime(); }
	}

	// P2P: the switch exists only for Internet servers and is on by default there
	if (sNode == "CHECKBUTTON_INET")
	{
		SetNodeUsing("CHECKBUTTON_P2P", bChecked);
		CheckButton_SetState("CHECKBUTTON_P2P", 1, bChecked);
	}

	if (sNode == "CHECKBUTTON_LAN" && !bChecked)
	{
		//CheckButton_SetState("CHECKBUTTON_LAN", 1, true);
	}
}

// COOP: land duel location of the selected row ("" = a sea battle)
// COOP: the list of maps of the current game type
string CS_MapControl()
{
	if (iGameType == NETGAME_FORT) { return "TABLE_MAPLIST_FORT"; }
	if (iGameType == CS_GAMETYPE_LAND) { return "TABLE_MAPLIST_LAND"; }
	return "TABLE_MAPLIST";
}

string CS_GetCurrentLand()
{
	if (iGameType != CS_GAMETYPE_LAND) { return ""; }
	string sControl = "TABLE_MAPLIST_LAND";
	string sRow = "tr" + sti(GameInterface.(sControl).select);
	if (!CheckAttribute(&GameInterface, sControl + "." + sRow + ".td1.LandLoc")) { return ""; }
	return GameInterface.(sControl).(sRow).td1.LandLoc;
}

// COOP s47: false = the list of the maps is empty ("Загрузки" without maps)
bool CS_HasCurrentMap()
{
	string sControl = CS_MapControl();
	string sRow = "tr" + sti(GameInterface.(sControl).select);
	return CheckAttribute(&GameInterface, sControl + "." + sRow + ".td1.str");
}

string CS_GetCurrentIsland()
{
	string sControl = CS_MapControl();
	string sRow = "tr" + sti(GameInterface.(sControl).select);
	if (!CheckAttribute(&GameInterface, sControl + "." + sRow + ".td1.IslandID")) { return ""; }
	return GameInterface.(sControl).(sRow).td1.IslandID;
}

string CS_GetCurrentWeather()
{
	switch (iGameTime)
	{
		case 0: 
			switch (iWeatherType)
			{
				case 0:	return "6 Hours"; break;
				case 1:	return "8 Hour"; break;
				case 2:	return "10 Hours"; break;
				case 3:	return "Storm01"; break;
				case 4:	return "Storm01"; break;	// Tornado: the storm sky + the tornado flag (oSrvParams.Tornado)
			}
		break;
		case 1: 
			switch (iWeatherType)
			{
				case 0:	return "11 Hour"; break;
				case 1:	return "14 Hour"; break;
				case 2:	return "16 Nour"; break;
				case 3:	return "Storm02"; break;
				case 4:	return "Storm02"; break;	// Tornado: the storm sky + the tornado flag (oSrvParams.Tornado)
			}
		break;
		case 2: 
			switch (iWeatherType)
			{
				case 0:	return "19 Hour"; break;
				case 1:	return "22 Hour"; break;
				case 2:	return "23 Hour"; break;
				case 3:	return "Storm03"; break;
				case 4:	return "Storm03"; break;	// Tornado: the storm sky + the tornado flag (oSrvParams.Tornado)
			}
		break;
		case 3: 
			switch (iWeatherType)
			{
				case 0:	return "0 Hours"; break;
				case 1:	return "1 hour"; break;
				case 2:	return "3 Hours"; break;
				case 3:	return "Storm04"; break;
				case 4:	return "Storm04"; break;	// Tornado: the storm sky + the tornado flag (oSrvParams.Tornado)
			}
		break;
	}
}

void CS_CreateServer()
{
	if (!CS_HasCurrentMap()) { return; }		// COOP s47: no map is chosen
	Object oSrvParams;

	int iCurrentNumTeams = 2;

	switch (iGameType)
	{
		case NETGAME_DEATHMATCH:	iCurrentNumTeams = 1;			break;
		case NETGAME_NAVAL:			iCurrentNumTeams = iNumTeams;	break;
		case NETGAME_CONVOY:		break;
		case NETGAME_FORT:			break;
	}

	oSrvParams.ServerName = GameInterface.EDITBOX_GAMENAME.str;
	oSrvParams.ServerPort = sti(GameInterface.EDITBOX_SERVERPORT.str);
	oSrvParams.ServerPassword = GameInterface.EDITBOX_GAMEPASSWORD.str;
	oSrvParams.AdminPassword = GameInterface.EDITBOX_ADMINPASSWORD.str;	// COOP: empty = nobody can become an admin
	oSrvParams.Dedicated = 0;
	oSrvParams.P2P = sti(GameInterface.CHECKBUTTON_INET.state1) && sti(GameInterface.CHECKBUTTON_P2P.state1);
	oSrvParams.LanServer = sti(GameInterface.CHECKBUTTON_LAN.state1);
	oSrvParams.InetServer = sti(GameInterface.CHECKBUTTON_INET.state1);
	oSrvParams.UpdateMasterServer = true;
	oSrvParams.MaxFps = 100;
	oSrvParams.GameType = iGameType;
	if (iGameType == CS_GAMETYPE_LAND) { oSrvParams.GameType = NETGAME_DEATHMATCH; iCurrentNumTeams = iLandTeams; oSrvParams.NumTeams = iLandTeams; }		// COOP: land duel (teams: 1 = everybody against everybody, 2 = two teams)
	oSrvParams.NumTeams = iCurrentNumTeams;
	oSrvParams.MaxPlayers = iMaxNumPlayers;
	oSrvParams.GameSpeed = fGameSpeed;
	oSrvParams.TimeLimit = iTimeLimit;
	oSrvParams.StartPosition = iStartPosition;
	oSrvParams.Credit = iCredit;
	oSrvParams.MaxShipClass = iMaxShipClass;
	oSrvParams.BotSkill = iBotSkill;		// COOP: server\Bots.c
	oSrvParams.IslandID = CS_GetCurrentIsland();
	oSrvParams.LandLoc = CS_GetCurrentLand();		// COOP: land duel (net\Land.c)
	oSrvParams.WeatherID = CS_GetCurrentWeather();
	oSrvParams.Tornado = 0;							// COOP: the Tornado weather
	if (iWeatherType == 4) { oSrvParams.Tornado = 1; }
	oSrvParams.AllowUpload = true;
	oSrvParams.AllowDownload = true;

	// COOP: respawn, boarding off, all passive / active sea perks of single player
	oSrvParams.Respawn = CS_GetCheck("CHECKBUTTON_RESPAWN");
	oSrvParams.RespawnTime = iRespawnTime;
	oSrvParams.NoBoarding = CS_GetCheck("CHECKBUTTON_NOBOARDING");
	if (CS_GetCurrentLand() != "") { oSrvParams.NoBoarding = CS_GetCheck("CHECKBUTTON_MELEEONLY"); }
	oSrvParams.PassivePerks = CS_GetCheck("CHECKBUTTON_PASSIVEPERKS");
	oSrvParams.ActivePerks = CS_GetCheck("CHECKBUTTON_ACTIVEPERKS");
	oSrvParams.LateJoin = CS_GetCheck("CHECKBUTTON_LATEJOIN");		// COOP s30
	oSrvParams.PistolReload = CS_GetCheck("CHECKBUTTON_PISTOLRELOAD");		// COOP s37: a land duel and the real boarding
	oSrvParams.FastBoarding = true;		// COOP s37: a land duel has no boarding
	if (CS_GetCurrentLand() == "") { oSrvParams.FastBoarding = CS_GetCheck("CHECKBUTTON_FASTBOARDING"); }

	Trace("COOP: CS_CreateServer: params ready");
	CS_SaveLast();
	// COOP s30: "Game settings" of the lobby: the running server takes the new game, back to the lobby
	if (bNetCSEdit)
	{
		bNetCSEdit = false;
		NetServer_ApplySettings(&oSrvParams);
		IDoExit(RC_INTERFACE_NET_BUY);
		return;
	}
	Net_CreateServer(&oSrvParams);
	Trace("COOP: CS_CreateServer: Net_CreateServer returned");

	IDoExit(RC_INTERFACE_NET_FINDGAME);
	Trace("COOP: CS_CreateServer: IDoExit done");
}

// COOP: state of a check button with one section (0 / 1)
int CS_GetCheck(string sNode)
{
	if (!CheckAttribute(&GameInterface, sNode + ".state1")) { return 0; }
	if (sti(GameInterface.(sNode).state1)) { return 1; }
	return 0;
}

void CS_OnMustOk() { XI_WindowDisable("WND_NETCREATEGAME", false); }

void CS_RestartOk() { XI_WindowDisable("WND_NETCREATEGAME", false); CS_CreateServer(); }
void CS_RestartCancel() { XI_WindowDisable("WND_NETCREATEGAME", false); }

void CS_OnOk()
{
	XI_WindowDisable("WND_NETCREATEGAME", false);

	XI_WindowShow("WND_OK", false);
	XI_WindowDisable("WND_OK", true);

	XI_WindowShow("WND_RECREATE", false);
	XI_WindowDisable("WND_RECREATE", true);
}

void CS_EventCreateServer()
{
	DelEventHandler("CS_EventCreateServer", "CS_EventCreateServer");
	CS_OnOk();
	CS_CreateServer();
}

void CS_RecreateOk()
{
	Net_DeleteServer();
	SetEventHandler("CS_EventCreateServer", "CS_EventCreateServer", 0);
	PostEvent("CS_EventCreateServer", 1);
}

void CS_RecreateCancel()
{
	CS_OnOk();
}

// Острова, на которых можно играть только в части режимов.
// Город Потерянных Кораблей: только Deathmatch и Team Deathmatch (нет фортов и точек конвоя).
bool CS_IsIslandAllowedForMode(string sIslandID, int iMode)
{
	if (sIslandID == "LostShipsCity")
	{
		if (iMode == NETGAME_DEATHMATCH || iMode == NETGAME_NAVAL) { return true; }
		return false;
	}
	return true;
}

void OnCreate()
{
	bool bError = false;

	// if server name not exist
	if (GameInterface.EDITBOX_GAMENAME.str == "")
	{
		bError = true;

		SetFormatedText("TEXT_OK", XI_ConvertString("Net_YouNeedtoEnterServerName"));

		XI_WindowDisable("WND_NETCREATEGAME", true);

		XI_WindowShow("WND_OK", true);
		XI_WindowDisable("WND_OK", false);

		SetEventHandler("OnOk", "CS_OnOk", 0);

		return;
	}

	// остров доступен не во всех режимах
	if (iGameType != CS_GAMETYPE_LAND && !CS_IsIslandAllowedForMode(CS_GetCurrentIsland(), iGameType))
	{
		SetFormatedText("TEXT_OK", "Этот остров доступен только в режимах Deathmatch и Team Deathmatch.");

		XI_WindowDisable("WND_NETCREATEGAME", true);

		XI_WindowShow("WND_OK", true);
		XI_WindowDisable("WND_OK", false);

		SetEventHandler("OnOk", "CS_OnOk", 0);

		return;
	}

	// COOP s30: "Game settings" of the lobby: the server is running, the port and the modes stay
	if (bNetCSEdit) { CS_CreateServer(); return; }

	// P2P: server port must be a number 1024..65535
	int iPortValue = sti(GameInterface.EDITBOX_SERVERPORT.str);
	if ("" + iPortValue != GameInterface.EDITBOX_SERVERPORT.str || iPortValue < 1024 || iPortValue > 65535)
	{
		SetFormatedText("TEXT_OK", "Порт должен быть числом от 1024 до 65535.");

		XI_WindowDisable("WND_NETCREATEGAME", true);

		XI_WindowShow("WND_OK", true);
		XI_WindowDisable("WND_OK", false);

		SetEventHandler("OnOk", "CS_OnOk", 0);

		return;
	}

	// if LanServer == false && InetServer == false 
	if (sti(GameInterface.CHECKBUTTON_LAN.state1) == false && sti(GameInterface.CHECKBUTTON_INET.state1) == false)
	{
		SetFormatedText("TEXT_OK", XI_ConvertString("Net_YouNeedtoChoseServerMode"));

		XI_WindowDisable("WND_NETCREATEGAME", true);

		XI_WindowShow("WND_OK", true);
		XI_WindowDisable("WND_OK", false);

		SetEventHandler("OnOk", "CS_OnOk", 0);

		return;
	}

	if (NetServer_IsStarted())
	{
		SetFormatedText("TEXT_RECREATE", XI_ConvertString("Net_ServerAlreadyRun"));

		XI_WindowDisable("WND_NETCREATEGAME", true);

		XI_WindowShow("WND_RECREATE", true);
		XI_WindowDisable("WND_RECREATE", false);

		SetEventHandler("OnRecreateOk", "CS_RecreateOk", 0);
		SetEventHandler("OnRecreateCancel", "CS_RecreateCancel", 0);

		return;
	}

	CS_CreateServer();
}

void OnCancel()
{
	// COOP s30: "Game settings" of the lobby: back to the lobby, nothing changes
	if (bNetCSEdit)
	{
		bNetCSEdit = false;
		IDoExit(RC_INTERFACE_NET_BUY);
		return;
	}
	IDoExit(RC_INTERFACE_NET_FINDGAME);
}

// ================================================================================================
// COOP: the settings of the last game created (Profiles.nsv: NCProfiles.LastCreate), set again the next time. The passwords
// are not kept
// ================================================================================================
void CS_SaveLast()
{
	DeleteAttribute(&NCProfiles, "LastCreate");
	NCProfiles.LastCreate.Name = GameInterface.EDITBOX_GAMENAME.str;
	NCProfiles.LastCreate.Port = GameInterface.EDITBOX_SERVERPORT.str;
	NCProfiles.LastCreate.AdminPass = GameInterface.EDITBOX_ADMINPASSWORD.str;
	NCProfiles.LastCreate.Lan = CS_GetCheck("CHECKBUTTON_LAN");
	NCProfiles.LastCreate.Inet = CS_GetCheck("CHECKBUTTON_INET");
	NCProfiles.LastCreate.P2P = CS_GetCheck("CHECKBUTTON_P2P");
	NCProfiles.LastCreate.Respawn = CS_GetCheck("CHECKBUTTON_RESPAWN");
	NCProfiles.LastCreate.NoBoarding = CS_GetCheck("CHECKBUTTON_NOBOARDING");
	NCProfiles.LastCreate.MeleeOnly = CS_GetCheck("CHECKBUTTON_MELEEONLY");
	NCProfiles.LastCreate.PassivePerks = CS_GetCheck("CHECKBUTTON_PASSIVEPERKS");
	NCProfiles.LastCreate.ActivePerks = CS_GetCheck("CHECKBUTTON_ACTIVEPERKS");
	NCProfiles.LastCreate.LateJoin = CS_GetCheck("CHECKBUTTON_LATEJOIN");
	NCProfiles.LastCreate.PistolReload = CS_GetCheck("CHECKBUTTON_PISTOLRELOAD");
	NCProfiles.LastCreate.FastBoarding = CS_GetCheck("CHECKBUTTON_FASTBOARDING");		// COOP s37
	NCProfiles.LastCreate.GameType = iGameType;
	NCProfiles.LastCreate.Teams = iNumTeams;
	NCProfiles.LastCreate.LandTeams = iLandTeams;
	NCProfiles.LastCreate.Players = iMaxNumPlayers;
	NCProfiles.LastCreate.Speed = fGameSpeed;
	NCProfiles.LastCreate.TimeLimit = iTimeLimit;
	NCProfiles.LastCreate.StartPos = iStartPosition;
	NCProfiles.LastCreate.Credit = iCredit;
	NCProfiles.LastCreate.MaxClass = iMaxShipClass;
	NCProfiles.LastCreate.BotSkill = iBotSkill;
	NCProfiles.LastCreate.RespawnTime = iRespawnTime;
	NCProfiles.LastCreate.GameTime = iGameTime;
	NCProfiles.LastCreate.Weather = iWeatherType;
	NCProfiles.LastCreate.MapKey = CS_GetCurrentIsland();
	if (CS_GetCurrentLand() != "") { NCProfiles.LastCreate.MapKey = CS_GetCurrentLand(); }
	Net_SaveFile(false, &NCProfiles, "Profiles.nsv");
}

void CS_LoadLast()
{
	if (!CheckAttribute(&NCProfiles, "LastCreate.GameType")) { return; }
	aref al;
	makearef(al, NCProfiles.LastCreate);
	GameInterface.EDITBOX_GAMENAME.str = al.Name;
	GameInterface.EDITBOX_SERVERPORT.str = al.Port;
	if (CheckAttribute(al, "AdminPass")) { GameInterface.EDITBOX_ADMINPASSWORD.str = al.AdminPass; }
	CheckButton_SetState("CHECKBUTTON_LAN", 1, sti(al.Lan));
	CheckButton_SetState("CHECKBUTTON_INET", 1, sti(al.Inet));
	SetNodeUsing("CHECKBUTTON_P2P", sti(al.Inet));
	CheckButton_SetState("CHECKBUTTON_P2P", 1, sti(al.P2P));
	CheckButton_SetState("CHECKBUTTON_RESPAWN", 1, sti(al.Respawn));
	CheckButton_SetState("CHECKBUTTON_NOBOARDING", 1, sti(al.NoBoarding));
	CheckButton_SetState("CHECKBUTTON_MELEEONLY", 1, sti(al.MeleeOnly));
	CheckButton_SetState("CHECKBUTTON_PASSIVEPERKS", 1, sti(al.PassivePerks));
	CheckButton_SetState("CHECKBUTTON_ACTIVEPERKS", 1, sti(al.ActivePerks));
	if (CheckAttribute(al, "LateJoin")) { CheckButton_SetState("CHECKBUTTON_LATEJOIN", 1, sti(al.LateJoin)); }
	if (CheckAttribute(al, "PistolReload")) { CheckButton_SetState("CHECKBUTTON_PISTOLRELOAD", 1, sti(al.PistolReload)); }
	if (CheckAttribute(al, "FastBoarding")) { CheckButton_SetState("CHECKBUTTON_FASTBOARDING", 1, sti(al.FastBoarding)); }
	iNumTeams = sti(al.Teams);
	if (CheckAttribute(al, "LandTeams")) { iLandTeams = sti(al.LandTeams); }
	iMaxNumPlayers = sti(al.Players);
	fGameSpeed = stf(al.Speed);
	iTimeLimit = sti(al.TimeLimit);
	iStartPosition = sti(al.StartPos);
	iCredit = sti(al.Credit);
	iMaxShipClass = sti(al.MaxClass);
	iBotSkill = sti(al.BotSkill);
	iRespawnTime = sti(al.RespawnTime);
	CS_SetNumTeams();
	CS_SetNumPlayers();
	CS_SetGameSpeed();
	CS_SetTimeLimit();
	CS_SetStartPosition();
	CS_SetCredit();
	CS_SetMaxShipClass();
	CS_SetBotSkill();
	CS_SetRespawnTime();
	CS_ShowRespawnTime(sti(al.Respawn));
	// the game type, its map, the time and the weather (the rows of the tables)
	Trace("COOP: last game: type " + al.GameType);
	GameInterface.TABLE_GAMETYPE.select = sti(al.GameType) + 1;
	Table_UpdateWindow("TABLE_GAMETYPE");
	CS_SelectNewGameType();
	string sMap = CS_MapControl();
	// the map of the last game (found by its name: the rows of the locations can change between versions), shown at
	// the top of the list. (The hang of this window came from the sliders - see CS_SlideChange - not from the list)
	string sKey = "";
	if (CheckAttribute(al, "MapKey")) { sKey = al.MapKey; }
	int iKeyRow = CS_FindMapRow(sMap, sKey);
	Trace("COOP: last game: map " + sKey + ", row " + iKeyRow);
	if (iKeyRow < 1) { iKeyRow = 1; }
	int iMapRows = CS_FindMapRow(sMap, "#count");
	int iTop = iKeyRow - 1;
	if (iTop > iMapRows - 9) { iTop = iMapRows - 9; }		// 9 rows are seen
	if (iTop < 0) { iTop = 0; }
	GameInterface.(sMap).select = iKeyRow;
	GameInterface.(sMap).top = iTop;
	Table_UpdateWindow(sMap);
	CS_SelectNewIsland();
	Trace("COOP: last game: the map is set");
	iGameTime = sti(al.GameTime);
	iWeatherType = sti(al.Weather);
	GameInterface.TABLE_GAMETIME.select = iGameTime + 1;
	GameInterface.TABLE_WEATHER.select = iWeatherType + 1;
	Table_UpdateWindow("TABLE_GAMETIME");
	Table_UpdateWindow("TABLE_WEATHER");
	// the melee switch has its own state in a land duel (CS_SelectNewIsland copies the boarding one when it changes)
	if (CS_GetCurrentLand() != "") { CheckButton_SetState("CHECKBUTTON_MELEEONLY", 1, sti(al.MeleeOnly)); }
	Trace("COOP: the settings of the last game are set again");
}

// COOP: the row of a map (a land location or an island) in its list; 0 = not there ("#count": the number of the rows)
int CS_FindMapRow(string sTable, string sKey)
{
	if (sKey == "") { return 0; }
	string sTr = "";
	for (int i=1; i<3000; i++)
	{
		sTr = "tr" + i;
		if (!CheckAttribute(&GameInterface, sTable + "." + sTr + ".td1"))
		{
			if (sKey == "#count") { return i - 1; }		// the number of the rows
			return 0;
		}
		if (CheckAttribute(&GameInterface, sTable + "." + sTr + ".td1.LandLoc"))
		{
			if (GameInterface.(sTable).(sTr).td1.LandLoc == sKey) { return i; }
		}
		if (CheckAttribute(&GameInterface, sTable + "." + sTr + ".td1.IslandID"))
		{
			if (GameInterface.(sTable).(sTr).td1.IslandID == sKey) { return i; }
		}
	}
	return 0;
}
