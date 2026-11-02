object oGameOver;
int iCoopHb = 0;

// COOP: random start-of-game voice, 5 seconds after the game started (single: interface\_EvEnemy*/_EvShip* events)
void NetClient_StartupSound()
{
	switch (rand(3))
	{
		case 0: PlaySound("interface\_EvEnemy0.wav"); break;
		case 1: PlaySound("interface\_EvEnemy1.wav"); break;
		case 2: PlaySound("interface\_EvShip0.wav"); break;
		case 3: PlaySound("interface\_EvShip1.wav"); break;
	}
}

// COOP: victory / defeat music at the end of the game (single: music_ship_victory / music_ship_dead)
void NetClient_PlayGameOverMusic(int iGameMode, int iWinnerID, int iWinnerTeam)
{
	ref rMe = NetClient_GetThisClient();

	bool bWin = false;
	if (iGameMode == NETGAME_DEATHMATCH) { bWin = (iWinnerID == wClientID); }
	else { bWin = (iWinnerTeam == sti(rMe.Team)); }

	if (bWin)
	{
		FadeOutMusic(2);
		PlayStereoOGG("music_ship_victory");
		return;
	}

	// our ship is already dead -> the defeat music was started when it sank
	if (!sti(rMe.Dead))
	{
		FadeOutMusic(2);
		PlayStereoOGG("music_ship_dead");
	}
}

void NetClient_Heartbeat()
{
	iCoopHb = iCoopHb + 1;
	ref rCoopMe = NetClient_GetClient(wClientID);
	Trace("COOP: hb " + iCoopHb + " cam=" + Camera.Pos.x + "," + Camera.Pos.y + "," + Camera.Pos.z + " camAy=" + Camera.Ang.y + " ship=" + rCoopMe.Ship.Pos.x + "," + rCoopMe.Ship.Pos.y + "," + rCoopMe.Ship.Pos.z);
	PostEvent("NetClient_Heartbeat", 500);
}


void NetClient_OnEndGame()
{
	for (int i=0; i<NET_MAXCLIENTS; i++)
	{
		if (!sti(NCClients[i].Use)) { continue; }
		DeleteClass(&NCClients[i]);
	}

	NetClient_DeleteWeatherEnvironment();
	NetClient_DeleteIslandEnvironment();
	NetClient_DeleteBallsEnvironment();
	NetClient_DeleteRiggingEnvironment();
	NetClient_DeleteCamerasEnvironment();
	NetClient_DeleteShipEnvironment();
	NetClient_DeleteShipLightsEnvironment();
	NetClient_DeleteFortEnvironment();
	NetClient_DeleteTimeEnvironment();
	NetClient_DeleteBattleInterface();
	NetClient_DeleteInterfaceEnvironment();
	NetClient_DeleteShipFoamEnvironment();
	NetClient_DeleteSeaCreaturesEnvironment();
	NetClient_DeleteChatEnvironment();

	DelEventHandler("NetClient_Heartbeat", "NetClient_Heartbeat");
	DelEventHandler("NetClient_StartupSound", "NetClient_StartupSound");
	DelEventHandler("Control Activation", "NetClient_ControlActivation");
	DelEventHandler("NCEvent_Disconnect", "NetClient_OnDisconnect");
	DelEventHandler("NetClient_OnDisconnectStartFindGame", "NetClient_OnDisconnectStartFindGame");
}

void NetClient_OnDisconnectStartFindGame()
{
	DelEventHandler("NetClient_OnDisconnectStartFindGame", "NetClient_OnDisconnectStartFindGame");

	// COOP: go to the main menu instead of the find-game screen (a second connection from there crashed)
	Net_ExitToMainMenuAfterDisconnect();
}

void NetClient_OnDisconnect()
{
	Net_DeleteClient();

	if (sti(InterfaceStates.Launched))
	{
		interfaceResultCommand = RC_INTERFACE_DO_NOTHING;
		EndCancelInterface(true);
	}

	SetEventHandler("NetClient_OnDisconnectStartFindGame", "NetClient_OnDisconnectStartFindGame", 0);
	PostEvent("NetClient_OnDisconnectStartFindGame", 1);
}

// COOP: морские эффекты из одиночки (sea.c: Sharks, SinkEffect, PeopleOnShip); графика у каждого игрока сво€, не синхронизируетс€
int iCoopFxSharks = 1;			// акулы (самосто€тельный класс Sharks); в тесте по€вл€лись под игроком и сто€ли на месте
int iCoopFxSinkEffect = 0;		// 0 по умолчанию: класс SINKEFFECT работает с корабл€ми движка ship.dll, а в мультиплеере корабли NetShip из net.dll
int iCoopFxPeopleOnShip = 0;	// 0 по умолчанию: PEOPLE_ON_SHIP тоже прив€зан к корабл€м и персонажам одиночной игры

void NetClient_DeleteSeaCreaturesEnvironment()
{
	if (isEntity(&Sharks)) { DeleteClass(&Sharks); }
	if (isEntity(&SinkEffect)) { DeleteClass(&SinkEffect); }
	if (isEntity(&PeopleOnShip)) { DeleteClass(&PeopleOnShip); }
}

void NetClient_CreateSeaCreaturesEnvironment()
{
	if (iCoopFxSharks != 0)
	{
		Trace("COOP: cli sharks");
		Sharks.execute = "net_execute";
		Sharks.realize = "net_realize";
		Sharks.executeModels = 75;
		Sharks.realizeModels = 75;
		Sharks.executeParticles = 78;
		Sharks.realizeParticles = 100001;
		CreateEntity(&Sharks, "Sharks");
	}

	if (iCoopFxSinkEffect != 0)
	{
		Trace("COOP: cli sink effect");
		CreateEntity(&SinkEffect, "SINKEFFECT");
		LayerAddObject("net_execute", &SinkEffect, 65532);
		LayerAddObject("net_realize", &SinkEffect, 65532);
	}

	if (iCoopFxPeopleOnShip != 0)
	{
		Trace("COOP: cli people on ship");
		CreateEntity(&PeopleOnShip, "PEOPLE_ON_SHIP");
		LayerAddObject("net_execute", &PeopleOnShip, 100);
		LayerAddObject("net_realize", &PeopleOnShip, 100);
	}
}

void NetClient_DeleteShipFoamEnvironment()
{
	DeleteClass(&Seafoam);
}

void NetClient_CreateShipFoamEnvironment(int iMsg)
{
	Seafoam.Server = false;
	Trace("COOP: cli foam: before CreateEntity Seafoam");
	CreateEntity(&Seafoam, "Seafoam");				
	Trace("COOP: cli foam: after CreateEntity Seafoam");
	
	LayerAddObject("net_execute", &Seafoam, -1);
	LayerAddObject("net_realize", &Seafoam, -1);
	
	if (NetClient_WhrIsStorm()) { Seafoam.storm = "true"; }
}

void NetClient_OnStartGame(int iMsg)
{
	int iSubCode = NMGetByte(iMsg);

	switch (iSubCode)
	{
		case NSC_STARTGAME_GETBUYINFO:
			Event("NCEvent_StartGame_GetBuyInfo");
		break;
		case NSC_STARTGAME_ENVIRONMENT:
			Event("NCEvent_StartGame");

			Trace("COOP: cli NetClient_CreateWeatherEnvironment");
			NetClient_CreateWeatherEnvironment(iMsg);
			Trace("COOP: cli NetClient_CreateIslandEnvironment");
			NetClient_CreateIslandEnvironment(iMsg);
			Trace("COOP: cli NetClient_CreateBallsEnvironment");
			NetClient_CreateBallsEnvironment(iMsg);
			Trace("COOP: cli NetClient_CreateRiggingEnvironment");
			NetClient_CreateRiggingEnvironment(iMsg);
			Trace("COOP: cli NetClient_CreateCamerasEnvironment");
			NetClient_CreateCamerasEnvironment(iMsg);
			Trace("COOP: cli NetClient_CreateShipEnvironment");
			NetClient_CreateShipEnvironment(iMsg);
			Trace("COOP: cli NetClient_CreateShipLightsEnvironment");
			NetClient_CreateShipLightsEnvironment(iMsg);
			Trace("COOP: cli NetClient_CreateFortEnvironment");
			NetClient_CreateFortEnvironment(iMsg);
			Trace("COOP: cli NetClient_CreateTimeEnvironment");
			NetClient_CreateTimeEnvironment(iMsg);
			Trace("COOP: cli NetClient_CreateShipFoamEnvironment");
			NetClient_CreateShipFoamEnvironment(iMsg);
			Trace("COOP: cli NetClient_CreateSeaCreaturesEnvironment");
			NetClient_CreateSeaCreaturesEnvironment();

			// particles init last because crashed if deleted first when ALT-F4 pressed
			Trace("COOP: cli before InitParticles");
			InitParticles(); MoveParticlesToLayers("net_execute", "net_realize");
			Trace("COOP: cli after InitParticles");

			SetEventHandler("Control Activation", "NetClient_ControlActivation", 0);

			Trace("COOP: cli before PauseAllSounds");
			PauseAllSounds();
			Trace("COOP: cli before ResetSoundScheme");
			ResetSoundScheme();
			Trace("COOP: cli before SetMusic");
			SetMusic("music_sea_battle");
			Trace("COOP: cli OnStartGame ENVIRONMENT done");
			iCoopHb = 0;
			SetEventHandler("NetClient_Heartbeat", "NetClient_Heartbeat", 0);
			PostEvent("NetClient_Heartbeat", 500);

			// random voice 5 seconds after the start of the game
			SetEventHandler("NetClient_StartupSound", "NetClient_StartupSound", 0);
			PostEvent("NetClient_StartupSound", 5000);
		break;
		case NSC_STARTGAME_GAMEOVER:
			NetClient_OnGameOver(iMsg);
		break;
	}
}

void NetClient_OnGameOver(int iMsg)
{
	int iGameMode = NMGetByte(iMsg);
	int iWinnerID = NMGetClientID(iMsg);
	int iWinnerTeam = NMGetByte(iMsg);

	string sTeam = "";

	switch (iGameMode)
	{
		case NETGAME_DEATHMATCH:
			ref rClient = NetClient_GetClient(iWinnerID);
			string sNick = rClient.NickName;
			Event(PARSE_STRING, "asls", &oGameOver, LanguageConvertString(iLangNetClient, "NetEG_DeathMatch"), 1, sNick);
		break;
		case NETGAME_NAVAL:
			sTeam = XI_ConvertString("nteam_" + NetTeamName[iWinnerTeam]);
			Event(PARSE_STRING, "asls", &oGameOver, LanguageConvertString(iLangNetClient, "NetEG_Team"), 1, sTeam);
		break;
		case NETGAME_CONVOY:
			sTeam = XI_ConvertString("nteam_" + NetTeamName[iWinnerTeam]);
			Event(PARSE_STRING, "asls", &oGameOver, LanguageConvertString(iLangNetClient, "NetEG_Team"), 1, sTeam);
		break;
		case NETGAME_FORT:
			sTeam = XI_ConvertString("nteam_" + NetTeamName[iWinnerTeam]);
			Event(PARSE_STRING, "asls", &oGameOver, LanguageConvertString(iLangNetClient, "NetEG_Team"), 1, sTeam);
		break;
	}

	NetClient_PlayGameOverMusic(iGameMode, iWinnerID, iWinnerTeam);

	LaunchNetEndGameScreen();
}


void NetClient_ControlActivation()
{
	string sControl = GetEventData();
	
	switch (sControl)
	{
		case "Net_Menu":
			LaunchNetMenu();
		break;
	}
}