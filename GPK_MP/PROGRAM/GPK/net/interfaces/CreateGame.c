int iGameType;
int CS_GAMETYPE_LAND = 4;		// COOP: land duel (net\Land.c): the 5th game type, a deathmatch in a location
int iMaxNumPlayers, iGameSpeed, iTimeLimit, iStartPosition, iCredit, iMaxShipClass;
int iGameTime = 0; 
int iWeatherType = 0;
int iNumTeams = 2;
int iBotSkill = 2;		// COOP: difficulty of the bots 0 harmless, 1 easy, 2 medium, 3 hard (server\Bots.c)
int iRespawnTime = 30;		// COOP: seconds between the death and the respawn (slider SBAR_RESPAWN, shown only with respawn on)

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

	// select checkbox LAN game
	CheckButton_SetState("CHECKBUTTON_LAN", 1, true);

	// P2P: server port (default 29020) and the P2P switch, which appears together with the Internet switch
	GameInterface.EDITBOX_SERVERPORT.str = "" + NET_DEFAULT_SERVER_PORT;
	CheckButton_SetState("CHECKBUTTON_P2P", 1, false);
	SetNodeUsing("CHECKBUTTON_P2P", false);

	// COOP: new switches, all off by default (old behaviour; the brander needs "All active perks")
	CheckButton_SetState("CHECKBUTTON_RESPAWN", 1, false);
	CheckButton_SetState("CHECKBUTTON_NOBOARDING", 1, false);
	CheckButton_SetState("CHECKBUTTON_PASSIVEPERKS", 1, false);
	CheckButton_SetState("CHECKBUTTON_ACTIVEPERKS", 1, false);

	iMaxNumPlayers = NET_MAXPLAYERS;		
	iGameSpeed = 1;
	iTimeLimit = 0;
	iStartPosition = 0;
	iCredit = 300000;
	iMaxShipClass = 1;
	
	CS_SetNumTeams();
	CS_SetNumPlayers();
	CS_SetGameSpeed();
	CS_SetTimeLimit();
	CS_SetStartPosition();
	CS_SetCredit();
	CS_SetMaxShipClass();
	CS_SetBotSkill();

	iRespawnTime = NET_RESPAWN_DELAY / 1000;
	CS_SetRespawnTime();
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

	FillIslandsList();

	// disable OK window
	XI_WindowShow("WND_OK", false);
	XI_WindowDisable("WND_OK", true);

	// disable RECREATE window
	XI_WindowShow("WND_RECREATE", false);
	XI_WindowDisable("WND_RECREATE", true);

	//CheckButton_SetDisable("CHECKBUTTON_INET", 1, true);
}

int CS_IsShift(int iShift, int iNoShift, int iAlt)
{
	if (XI_IsKeyPressed("shift")) { return iShift; }
	if (XI_IsKeyPressed("alt")) { return iAlt; }
	return iNoShift;
}

void CS_SetNumTeams()
{
	if (iNumTeams > 4) { iNumTeams = 4; }
	if (iNumTeams < 2) { iNumTeams = 2; }

	switch (iGameType)
	{
		case NETGAME_DEATHMATCH:	StringCollection_SetText("STRINGS_VALUE", 1, "#");				break;
		case NETGAME_NAVAL:			StringCollection_SetTextValue("STRINGS_VALUE", 1, iNumTeams);	break;
		case NETGAME_CONVOY:		StringCollection_SetText("STRINGS_VALUE", 1, "#2");				break;
		case NETGAME_FORT:			StringCollection_SetText("STRINGS_VALUE", 1, "#2");				break;
	}
	if (iGameType == CS_GAMETYPE_LAND) { StringCollection_SetText("STRINGS_VALUE", 1, "#"); }
}
void CS_AddTeams() { iNumTeams++; CS_SetNumTeams() }
void CS_DecTeams() { iNumTeams--; CS_SetNumTeams() }

void CS_SetNumPlayers()
{
	if (iMaxNumPlayers > NET_MAXPLAYERS) { iMaxNumPlayers = NET_MAXPLAYERS; }
	if (iMaxNumPlayers < 2) { iMaxNumPlayers = 2; }

	StringCollection_SetTextValue("STRINGS_VALUE", 2, iMaxNumPlayers);
	//Gameinterface.strings.MaxPlayers = iMaxNumPlayers;
}
void CS_AddPlayer() { iMaxNumPlayers++; CS_SetNumPlayers(); }
void CS_DecPlayer() { iMaxNumPlayers--; CS_SetNumPlayers(); }

void CS_SetGameSpeed()
{
	if (iGameSpeed > 4) { iGameSpeed = 4; }
	if (iGameSpeed < 1) { iGameSpeed = 1; }

	StringCollection_SetText("STRINGS_VALUE", 3, "#" + iGameSpeed + "x");
	//Gameinterface.strings.GameSpeed = iGameSpeed + "x";
}
void CS_AddGameSpeed() { iGameSpeed++; CS_SetGameSpeed(); }
void CS_DecGameSpeed() { iGameSpeed--; CS_SetGameSpeed(); }

void CS_SetTimeLimit()
{
	if (iTimeLimit <= 0) 
	{ 
		iTimeLimit = 0; 
		StringCollection_SetText("STRINGS_VALUE", 4, "#");
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

void CS_SetCredit()
{
    if (iCredit < 1000) { iCredit = 1000; }
    if (iCredit > 1000000) { iCredit = 1000000; }

    StringCollection_SetTextValue("STRINGS_VALUE", 6, iCredit);

    float fCredit = (iCredit - 1000.0) / 999000.0;

    GameInterface.nodes.SBAR_CREDIT.value = fCredit;

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

    // COOP: respawn time slider
    if (sNodeName == "SBAR_RESPAWN")
    {
        if (fVal < 0.0) { fVal = 0.0; }
        if (fVal > 1.0) { fVal = 1.0; }
        iRespawnTime = NET_RESPAWN_TIME_MIN + makeint(fVal * (NET_RESPAWN_TIME_MAX - NET_RESPAWN_TIME_MIN) + 0.5);
        CS_SetRespawnTime();
        return;
    }

    if (sNodeName != "SBAR_CREDIT") { return; }

    if (fVal < 0.0) { fVal = 0.0; }
    if (fVal > 1.0) { fVal = 1.0; }

    iCredit = 1000 + makeint(fVal * 999000.0);

    CS_SetCredit();
}

// COOP: respawn time (seconds) - text and slider position
void CS_SetRespawnTime()
{
    if (iRespawnTime < NET_RESPAWN_TIME_MIN) { iRespawnTime = NET_RESPAWN_TIME_MIN; }
    if (iRespawnTime > NET_RESPAWN_TIME_MAX) { iRespawnTime = NET_RESPAWN_TIME_MAX; }

    SetFormatedText("TEXT_RESPAWN_TIME", XI_ConvertString("capNetCS_Respawn") + ": " + iRespawnTime + " сек.");

    float fPos = MakeFloat(iRespawnTime - NET_RESPAWN_TIME_MIN) / MakeFloat(NET_RESPAWN_TIME_MAX - NET_RESPAWN_TIME_MIN);
    GameInterface.nodes.SBAR_RESPAWN.value = fPos;
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

void FillIslandsList()
{
	int iForts = 1;

	GameInterface.TABLE_MAPLIST.tr1.td1.str = XI_ConvertString("OpenSea");
	GameInterface.TABLE_MAPLIST.tr1.td1.IslandID = "";
	GameInterface.TABLE_MAPLIST.tr1.td1.my_texture = "Interfaces\Net\Islands\OpenSea.tga";
	GameInterface.TABLE_MAPLIST.select = 1;

	GameInterface.TABLE_MAPLIST_FORT.select = 1;

	for (int i=0; i<Net_GetIslandsNum(); i++)
	{
		ref rIsland = Net_GetIslandByIndex(i);

		string sTexture = "Interfaces\Net\Islands\" + rIsland.ID + ".tga";

		string sTest = "tr" + (i + 2);
		GameInterface.TABLE_MAPLIST.(sTest).td1.str = Net_GetIslandName(rIsland.ID);
		GameInterface.TABLE_MAPLIST.(sTest).td1.IslandID = rIsland.ID;
		GameInterface.TABLE_MAPLIST.(sTest).td1.my_texture = sTexture;

		if (Net_IsIslandHaveForts(rIsland))
		{
			sTest = "tr" + iForts;
			GameInterface.TABLE_MAPLIST_FORT.(sTest).td1.str = Net_GetIslandName(rIsland.ID);
			GameInterface.TABLE_MAPLIST_FORT.(sTest).td1.IslandID = rIsland.ID;
			GameInterface.TABLE_MAPLIST_FORT.(sTest).td1.my_texture = sTexture;
			iForts++;
		}
	}

	// COOP: land duel (net\Land.c): the last rows of the list are locations on land
	CS_AddLandRows("TABLE_MAPLIST_LAND", 1);
	GameInterface.TABLE_MAPLIST_LAND.select = 1;
	Table_UpdateWindow("TABLE_MAPLIST_LAND");

	Table_UpdateWindow("TABLE_MAPLIST");
	Table_UpdateWindow("TABLE_MAPLIST_FORT");

	CS_SelectNewGameType();
}

// COOP: a row of a land duel location (it exists in this build: FindLocation)
int CS_AddLandRow(string sTable, int iRow, string sName, string sLoc)
{
	string sRow = "tr" + iRow;
	GameInterface.(sTable).(sRow).td1.str = sName;
	GameInterface.(sTable).(sRow).td1.IslandID = "";
	GameInterface.(sTable).(sRow).td1.LandLoc = sLoc;
	GameInterface.(sTable).(sRow).td1.my_texture = "Interfaces\Net\Islands\OpenSea.tga";
	return 1;
}

void CS_AddLandRows(string sTable, int iFirstRow)
{
	int iLandRow = iFirstRow;
	iLandRow = iLandRow + CS_AddLandRow(sTable, iLandRow, "Дуэль: каюта", "Cabin");
	iLandRow = iLandRow + CS_AddLandRow(sTable, iLandRow, "Дуэль: средняя каюта", "Cabin_Medium");
	iLandRow = iLandRow + CS_AddLandRow(sTable, iLandRow, "Дуэль: большая каюта", "Cabin_Huge");
	iLandRow = iLandRow + CS_AddLandRow(sTable, iLandRow, "Дуэль: палуба (малая)", "BOARDING_SMALL_DECK");
	iLandRow = iLandRow + CS_AddLandRow(sTable, iLandRow, "Дуэль: палуба (средняя)", "BOARDING_MEDIUM_DECK");
	iLandRow = iLandRow + CS_AddLandRow(sTable, iLandRow, "Дуэль: палуба (большая)", "BOARDING_BIG_DECK");

	// all the other locations of the game: towns, jungles, shores, forts, caves, dungeons, shops, shipyards, taverns,
	// brothels, churches, residences... (the ones without a way in or out, the decks / cabins above and the underwater are left out)
	for (int i=0; i<nLocationsNum; i++)
	{
		if (!CheckAttribute(&Locations[i], "id")) { continue; }
		string sLocId = Locations[i].id;
		if (sLocId == "") { continue; }
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
		iLandRow = iLandRow + CS_AddLandRow(sTable, iLandRow, sLocName, sLocId);
	}
}

void CS_SelectNewIsland()
{
	string sControl = CS_MapControl();
	string sRow = "tr" + sti(GameInterface.(sControl).select);
	string sTexture = GameInterface.(sControl).(sRow).td1.my_texture;

	Picture_SetPicture("SERVER_MAP_PICTURE", sTexture);

	// COOP: land duel: no ships - the class of the ships is hidden
	bool bLand = false;
	if (CS_GetCurrentLand() != "") { bLand = true; }
	SetNodeUsing("SBAR_MAXSHIPCLASS", !bLand);
	SetNodeUsing("CAPTION_MAXSHIPCLASS", !bLand);
	if (bLand) { StringCollection_SetText("STRINGS_VALUE", 7, "#"); }
	else { CS_SetMaxShipClass(); }
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

	SetEventHandler("TableSelectChange", "CS_TableSelectChange", 0);	

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
	DelEventHandler("eSlideChange", "CS_SlideChange");

	interfaceResultCommand = exitCode;
	EndCancelInterface(true);
}

void CS_CheckButtonChange()
{
	string sNode = GetEventData();
	int iButtonNumber = GetEventData();
	int bChecked = GetEventData();
	
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

string CS_GetCurrentIsland()
{
	string sControl = CS_MapControl();
	string sRow = "tr" + sti(GameInterface.(sControl).select);
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
	if (iGameType == CS_GAMETYPE_LAND) { oSrvParams.GameType = NETGAME_DEATHMATCH; iCurrentNumTeams = 1; oSrvParams.NumTeams = 1; }		// COOP: land duel
	oSrvParams.NumTeams = iCurrentNumTeams;
	oSrvParams.MaxPlayers = iMaxNumPlayers;
	oSrvParams.GameSpeed = iGameSpeed;
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
	oSrvParams.PassivePerks = CS_GetCheck("CHECKBUTTON_PASSIVEPERKS");
	oSrvParams.ActivePerks = CS_GetCheck("CHECKBUTTON_ACTIVEPERKS");

	Trace("COOP: CS_CreateServer: params ready");
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
	IDoExit(RC_INTERFACE_NET_FINDGAME);
}