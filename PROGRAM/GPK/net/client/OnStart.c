object oGameOver;
int iCoopHb = 0;

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
	NetClient_DeleteChatEnvironment();

	DelEventHandler("NetClient_Heartbeat", "NetClient_Heartbeat");
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