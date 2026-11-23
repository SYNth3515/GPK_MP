int iSkillAccuracy = 0, iSkillCannons = 0, iSkillSailing = 0, iSkillDefence = 0, iSkillRepair = 0, iSkillGrappling = 0;

int BuyClients[NET_TOTALCLIENTS];

int iCurrentCredit = 0, iMaxCredit = 0;
int iSkillsQuantity[8];
int iNetGoodsQuantity[8];
int iCrewPercent = 100;
int iCannonsPercent = 100;		// COOP: percent of the cannons of the ship that are bought (0 = no cannons at all)
int iPerks[6];
int iBuySeaPerk[17];		// COOP s28: the sea perks bought here (common\Perks.c: NET_SPERK_*), 1 = bought
int iCurrentShipType = -1;
int iCurrentShipUpgradeHull = -1;
int iCurrentShipUpgradeRigging = -1;
int iCurrentCannonType = -1;
int iCurrentCannonUpgrade = -1;
int iSelectedTeam = -1;
int iNumClients = 0;

int iClientStartTimer = 10000;
bool bStartCountDown = false;
bool bSelectNewTeamWindow = false;

// COOP: host "Start" button and lobby chat
bool bBuyHostStartSent = false;
int iBuyHostStartTimer = 0;
bool bBuyChatOpen = false;
int iBuyChatUnread = 0;
int iBuyChatUnreadShown = -1;

// COOP: admin login in the lobby
bool bBuyAdminOpen = false;
bool bBuyAdminShown = false;

float fServerInfoPiecesLoaded = 0.0;

bool bServerInfoReady = false;
bool bBuyProfLists = false;		// COOP s30: the lists of the window "Герб/Паруса/Флаг" are filled (the ship screen of the profile)

// COOP s38: the switch of the bot respawn of the lobby (Buy_BotRespFrame)
bool bBuyBotRespSync = false;
int iBuyBotRespShown = -1;
int iBuyBotRespAdm = -1;

void InitInterface(string iniName)
{
	int i;

	bServerInfoReady = false;

	bBuyHostStartSent = false;
	bBuyChatOpen = false;
	iBuyChatUnread = 0;
	iBuyChatUnreadShown = -1;
	bNetLobbyChatActive = true;
	bBuyAdminOpen = false;
	bBuyAdminShown = false;

	GameInterface.title = "titleNetBuy";

	Buy_InitTable("TABLE_PLAYERS");
	Buy_InitTable("TABLE_STARTGAME_WAIT_PLAYERS");

	GameInterface.TABLE_SELECTTEAM.hr.td1.str = "";
	GameInterface.TABLE_SELECTTEAM.hr.td2.str = XI_ConvertString("capNetBuy_TableSelectTeamQuantity");

	// COOP s30: the sails and the flag of the profile (the window "Герб/Паруса/Флаг" of the ship screen of the profile)
	// (filled in the lobby too, like the old profile screen did: an empty SCROLLIMAGE is not tried)
	bBuyProfLists = true;
	if (bBuyProfLists)
	{
		EnumerateIcons("resource\textures\" + sUserSailsPath, "*.tga.tx", "SAILSLIST", 0);
		EnumerateIcons("resource\textures\" + sUserFlagsPath, "*.tga.tx", "FLAGSLIST", 0);
		for (i=0; i<SAILS_COLOR_QUANTITY; i++)
		{
			string sClrAttr = "pic" + (i + 1);
			GameInterface.COLORSLIST.(sClrAttr).name1 = SailsColors[i].icon;
		}
		GameInterface.COLORSLIST.ListSize = SAILS_COLOR_QUANTITY;
		SetEventHandler("GetInterfaceTexture", "Buy_ScrollGetTexture", 0);
	}

	SendMessage(&GameInterface, "ls", MSG_INTERFACE_INIT, iniName);

	SetEventHandler("CheckButtonChange", "Buy_CheckButtonChange", 0);

	SetEventHandler("OnTmpStart", "OnTmpStart", 0);
	SetEventHandler("OnReset", "OnReset", 0);
	SetEventHandler("OnDisconnect", "OnDisconnect", 0);

	SetEventHandler("ShipLeft", "Buy_ShipLeft", 0);
	SetEventHandler("ShipRight", "Buy_ShipRight", 0);

	SetEventHandler("AddUpgradeHull", "Buy_AddUpgradeHull", 0);			SetEventHandler("DecUpgradeHull", "Buy_DecUpgradeHull", 0);
	SetEventHandler("AddUpgradeRigging", "Buy_AddUpgradeRigging", 0);	SetEventHandler("DecUpgradeRigging", "Buy_DecUpgradeRigging", 0);

	SetEventHandler("CannonLeft", "Buy_CannonLeft", 0);
	SetEventHandler("CannonRight", "Buy_CannonRight", 0);

	SetEventHandler("AddAccuracy", "Buy_AddAccuracy", 0);	SetEventHandler("DecAccuracy", "Buy_DecAccuracy", 0);
	SetEventHandler("NetInfoShow", "Buy_InfoShow", 0);		// COOP s31: the right click: a description (net/NetInfo.c)
	SetEventHandler("MouseRClickUp", "NetInfo_Hide", 0);
	SetEventHandler("AddCannons", "Buy_AddCannons", 0);		SetEventHandler("DecCannons", "Buy_DecCannons", 0);
	SetEventHandler("AddSailing", "Buy_AddSailing", 0);		SetEventHandler("DecSailing", "Buy_DecSailing", 0);
	SetEventHandler("AddDefence", "Buy_AddDefence", 0);		SetEventHandler("DecDefence", "Buy_DecDefence", 0);
	SetEventHandler("AddRepair", "Buy_AddRepair", 0);		SetEventHandler("DecRepair", "Buy_DecRepair", 0);
	SetEventHandler("AddGrappling", "Buy_AddGrappling", 0);	SetEventHandler("DecGrappling", "Buy_DecGrappling", 0);

	SetEventHandler("AddBalls", "Buy_AddBalls", 0);			SetEventHandler("DecBalls", "Buy_DecBalls", 0);
	SetEventHandler("AddGrapes", "Buy_AddGrapes", 0);		SetEventHandler("DecGrapes", "Buy_DecGrapes", 0);
	SetEventHandler("AddKnippels", "Buy_AddKnippels", 0);	SetEventHandler("DecKnippels", "Buy_DecKnippels", 0);
	SetEventHandler("AddBombs", "Buy_AddBombs", 0);			SetEventHandler("DecBombs", "Buy_DecBombs", 0);
	SetEventHandler("AddPlanks", "Buy_AddPlanks", 0);		SetEventHandler("DecPlanks", "Buy_DecPlanks", 0);
	SetEventHandler("AddSailcloth", "Buy_AddSailcloth", 0);	SetEventHandler("DecSailcloth", "Buy_DecSailcloth", 0);
	SetEventHandler("AddPowder", "Buy_AddPowder", 0);	SetEventHandler("DecPowder", "Buy_DecPowder", 0);
	SetEventHandler("AddCrew", "Buy_AddCrew", 0);	SetEventHandler("DecCrew", "Buy_DecCrew", 0);
	SetEventHandler("AddCannonsPercent", "Buy_AddCannonsPercent", 0);	SetEventHandler("DecCannonsPercent", "Buy_DecCannonsPercent", 0);

	SetEventHandler("OnTableClick", "Buy_OnTableClick", 0);
	SetEventHandler("NCEvent_PlayerInfo_ShipType", "Buy_PlayerInfo_ShipType", 0);
	SetEventHandler("NCEvent_PlayerInfo_BuyReady", "Buy_PlayerInfo_BuyReady", 0);
	SetEventHandler("NCEvent_PlayerInfo_Team", "Buy_PlayerInfo_Team", 0);
	SetEventHandler("NCEvent_Client_Connect", "Buy_NewClientConnect", 0);

	SetEventHandler("NCEvent_StartGame", "Buy_StartGame", 0);
	SetEventHandler("NCEvent_StartGame_GetBuyInfo", "Buy_GetBuyInfo", 0);
	SetEventHandler("NCEvent_PlayerInfo_StartCountdown", "Buy_StartCountdown", 0);
	SetEventHandler("NCEvent_PlayerInfo_StopCountdown",  "Buy_StopCountdown", 0);
	SetEventHandler("NCEvent_Disconnect", "Buy_Disconnect", 0);

	SetEventHandler("NCEvent_ClientDrop", "Buy_ClientDrop", 0);

	SetEventHandler("NCEvent_LobbyChat", "Buy_OnLobbyChat", 0);
	SetEventHandler("OnChatOpen", "Buy_ChatOpen", 0);
	SetEventHandler("OnChatClose", "Buy_ChatClose", 0);
	SetEventHandler("OnChatSend", "Buy_ChatSend", 0);
	SetEventHandler("EditExit", "Buy_ChatEditExit", 0);
	SetEventHandler("OnAdminOpen", "Buy_AdminOpen", 0);
	// COOP: the loadout of the land duel (net\Land.c: NetLand_UiLo*)
	SetEventHandler("OnLobbyLoadout", "Buy_LoOpen", 0);
	SetEventHandler("OnSeaPerk", "Buy_OnSeaPerk", 0);		// COOP s28: the table of the sea perks
	SetEventHandler("OnProfOk", "Buy_ProfOk", 0);
	SetEventHandler("OnProfBack", "Buy_ProfLeave", 0);
	SetEventHandler("OnProfPreview", "Buy_ProfPreview", 0);		// COOP: the preview of the ship
	SetEventHandler("Buy_PreviewGo", "Buy_PreviewGo", 0);
	SetEventHandler("OnLoApply", "Buy_LoApply", 0);
	SetEventHandler("OnLoBack", "Buy_LoBack", 0);
	NetLand_UiLoRegister(true);
	XI_WindowShow("WND_LOADOUT", false);
	XI_WindowDisable("WND_LOADOUT", true);
	SetNodeUsing("BTN_LOBBY_LOADOUT", false);
	SetEventHandler("OnAdminClose", "Buy_AdminClose", 0);
	SetEventHandler("OnAdminSend", "Buy_AdminSend", 0);
	SetEventHandler("NCEvent_AdminStatus", "Buy_OnAdminStatus", 0);

	SetEventHandler("frame", "NetBuy_OnFrame", 0);

	SetEventHandler("OnClickTeam", "NetBuy_OnClickTeam", 0);

	Button_SetEnable("BTN_START", false);
	Button_SetEnable("BTN_RESET", false);

	XI_WindowDisable("WND_SELECT_TEAM", true);
	XI_WindowShow("WND_SELECT_TEAM", false);

	XI_WindowDisable("WND_NETCHAT", true);
	XI_WindowShow("WND_NETCHAT", false);

	XI_WindowDisable("WND_NETADMINLOGIN", true);
	XI_WindowShow("WND_NETADMINLOGIN", false);

	// COOP s27: the window "Подробно" (the parameters of the server; the profile screen has no server)
	XI_WindowDisable("WND_DETAILS", true);
	XI_WindowShow("WND_DETAILS", false);
	SetNodeUsing("BTN_DETAILS", !bNetBuyProfile);
	SetEventHandler("OnDetails", "Buy_DetailsOpen", 0);
	// COOP s30: "Game settings" of the host (the Create game screen for the running server) and its result for every lobby
	SetEventHandler("OnGameSettings", "Buy_OnGameSettings", 0);
	// COOP s30: the bots of the lobby (the button "Боты", the window "Бот+", the rows of the players table)
	SetEventHandler("OnLobbyBots", "Buy_LBotOpen", 0);
	// COOP s30: the choice of another player / a bot (a click on its row; read only)
	SetEventHandler("OnPeekClose", "Buy_PeekClose", 0);
	bBuyPeekOpen = false;
	XI_WindowShow("WND_PEEK", false);
	XI_WindowDisable("WND_PEEK", true);
	sBuyChoiceSent = "";
	iBuyChoiceTimer = 600;
	SetEventHandler("NCEvent_LobbyBots", "Buy_OnLobbyBots", 0);
	SetEventHandler("OnBotShipPrev", "Buy_LBotShipPrev", 0);
	SetEventHandler("OnBotShipNext", "Buy_LBotShipNext", 0);
	SetEventHandler("OnBotCannonPrev", "Buy_LBotCannonPrev", 0);
	SetEventHandler("OnBotCannonNext", "Buy_LBotCannonNext", 0);
	SetEventHandler("OnBotCrewDec", "Buy_LBotCrewDec", 0);
	SetEventHandler("OnBotCrewInc", "Buy_LBotCrewInc", 0);
	SetEventHandler("OnBotHullDec", "Buy_LBotHullDec", 0);
	SetEventHandler("OnBotHullInc", "Buy_LBotHullInc", 0);
	SetEventHandler("OnBotSailsDec", "Buy_LBotSailsDec", 0);
	SetEventHandler("OnBotSailsInc", "Buy_LBotSailsInc", 0);
	SetEventHandler("OnBotGunPrev", "Buy_LBotGunPrev", 0);
	SetEventHandler("OnBotGunNext", "Buy_LBotGunNext", 0);
	SetEventHandler("OnBotRelPrev", "Buy_LBotRelPrev", 0);
	SetEventHandler("OnBotRelNext", "Buy_LBotRelNext", 0);
	SetEventHandler("OnBotDoAdd", "Buy_LBotDoAdd", 0);
	SetEventHandler("OnBotBack", "Buy_LBotClose", 0);
	bBuyLBotOpen = false;
	XI_WindowShow("WND_BOTADD", false);
	XI_WindowDisable("WND_BOTADD", true);
	iBuyLBotsCapShown = -2;
	SetNodeUsing("BTN_LOBBY_BOTS", !bNetBuyProfile);
	SetNodeUsing("CHECKBUTTON_LOBBY_BOTRESPAWN", !bNetBuyProfile);		// COOP s38
	iBuyBotRespShown = -1;
	iBuyBotRespAdm = -1;
	SetEventHandler("NCEvent_ServerChanged", "Buy_OnServerChanged", 0);
	iBuyGsShown = -1;
	SetNodeUsing("BTN_GAMESETTINGS", false);
	SetEventHandler("OnDetailsClose", "Buy_DetailsClose", 0);
	SetEventHandler("NCEvent_SrvDetails", "Buy_OnSrvDetails", 0);

	SetNodeUsing("BTN_PROF_OK", bNetBuyProfile);
	SetNodeUsing("BTN_PROF_BACK", bNetBuyProfile);
	SetNodeUsing("BTN_PROF_PREVIEW", bNetBuyProfile);
	// COOP s30: the ship screen of the profile: the 3D ship above the panels, its name, the camera, the sails and the flag
	SetNodeUsing("FRAME_PROF_NAME", bNetBuyProfile);
	SetNodeUsing("TEXT_PROF_NAMECAP", bNetBuyProfile);
	SetNodeUsing("EDITBOX_PROF_SHIPNAME", bNetBuyProfile);
	SetNodeUsing("BTN_PROF_ROTL", bNetBuyProfile);
	SetNodeUsing("BTN_PROF_ROTR", bNetBuyProfile);
	SetNodeUsing("BTN_PROF_ZOOMIN", bNetBuyProfile);
	SetNodeUsing("BTN_PROF_ZOOMOUT", bNetBuyProfile);
	SetNodeUsing("TEXT_PROF_MONEY", bNetBuyProfile);
	SetNodeUsing("BTN_PROF_FLAGS", bNetBuyProfile);
	XI_WindowShow("WND_PROF_FLAGS", false);
	XI_WindowDisable("WND_PROF_FLAGS", true);
	SetEventHandler("OnProfRotL", "Buy_ProfRotL", 0);
	SetEventHandler("OnProfRotR", "Buy_ProfRotR", 0);
	SetEventHandler("OnProfZoomIn", "Buy_ProfZoomIn", 0);
	SetEventHandler("OnProfZoomOut", "Buy_ProfZoomOut", 0);
	SetEventHandler("OnProfFlags", "Buy_ProfFlagsOpen", 0);
	SetEventHandler("OnPflClose", "Buy_ProfFlagsClose", 0);
	if (bNetBuyRespawn)
	{
		SetNodeUsing("BTN_PROF_OK", true);
		SetNodeUsing("BTN_PROF_BACK", true);
		SetNodeUsing("BTN_PROF_PREVIEW", false);
		Buy_RespModeInit();
	}
	else
	{
		if (bNetBuyProfile) { Buy_ProfModeInit(); }
		else { Buy_GetFullServerInfo(); }
	}

	// temporary disable
	for (i=0; i<Net_GetPerksNum(); i++)
	{
		CheckButton_SetDisable("CHECKBUTTONS_PERKS", i + 1, true);
	}
}

void NetBuy_OnClickTeam()
{
	int iTmp = GetEventData();
	string sControl = GetEventData();

	int iNewTeam = -1;
	switch (sControl)
	{
		case "RECTANGLE_TEAM0":	iNewTeam = 0; break;
		case "RECTANGLE_TEAM1": iNewTeam = 1; break;
		case "RECTANGLE_TEAM2": iNewTeam = 2; break;
		case "RECTANGLE_TEAM3": iNewTeam = 3; break;
	}

	if (iNewTeam < sti(NCServer.NumTeams))
	{
		iSelectedTeam = iNewTeam;
		NetBuy_SelectTeamUpdate();
	}
}

void NetBuy_SelectTeamUpdate()
{
	int i;
	// fill current team colors
	for (i=0; i<4; i++)
	{
		string sTeam = "team" + i;

		int iFrameColor = argb(255, 128, 128, 128);

		SendMessage(&GameInterface, "lslls", MSG_INTERFACE_MSG_TO_NODE, "PICTURES_SELECTTEAM", 4, i, sTeam);
		SendMessage(&GameInterface, "lslll", MSG_INTERFACE_MSG_TO_NODE, "PICTURES_SELECTTEAM", 3, i, argb(255, 128, 128, 128));
		if (i > sti(NCServer.NumTeams) - 1)
		{
			SendMessage(&GameInterface, "lslll", MSG_INTERFACE_MSG_TO_NODE, "PICTURES_SELECTTEAM", 3, i, argb(255, 30, 30, 30));
			//SendMessage(&GameInterface, "lslss", MSG_INTERFACE_MSG_TO_NODE, "PICTURES_SELECTTEAM", 3, argb(0,0,0,0));	
			iFrameColor = argb(255, 28, 28, 28);
		}

		if (iSelectedTeam == i) { iFrameColor = argb(255, 255, 255, 255); }

		string sTeamFrame = "FRAME_TEAM" + i;
		SendMessage(&GameInterface, "lsll", MSG_INTERFACE_MSG_TO_NODE, sTeamFrame, 0, iFrameColor);
	}
}

void NetBuy_SelectTeamOk()
{
	if (bNetBuyProfile) { return; }		// COOP: the ship editor of the profile: no server
	// send change team code to server
	int iSMsg = NMCreate();
	NMAddClientID(iSMsg, wClientID);
	NMAddByte(iSMsg, NC_BUY);
	NMAddByte(iSMsg, NSC_BUY_TEAMCHANGE);
	NMAddByte(iSMsg, iSelectedTeam);
	NetClient_SendMessage(iSMsg, true);
	NMDelete(iSMsg);
	
	NetBuy_SelectTeamCancel();
}

void NetBuy_SelectTeamCancel()
{
	DelEventHandler("OnSelectTeamOk", "NetBuy_SelectTeamOk");
	DelEventHandler("OnSelectTeamCancel", "NetBuy_SelectTeamCancel");

	XI_WindowDisable("WND_SELECT_TEAM", true);
	XI_WindowShow("WND_SELECT_TEAM", false);

	XI_WindowDisable("WND_NETBUY", false);

	bSelectNewTeamWindow = false;
}

void NetBuy_OpenSelectTeam()
{
	if (bStartCountDown) { return; }

	ref rOurClient = NetClient_GetThisClient();
	iSelectedTeam = sti(rOurClient.Team);

	SetEventHandler("OnSelectTeamOk", "NetBuy_SelectTeamOk", 0);
	SetEventHandler("OnSelectTeamCancel", "NetBuy_SelectTeamCancel", 0);

	XI_WindowDisable("WND_NETBUY", true);

	XI_WindowDisable("WND_SELECT_TEAM", false);
	XI_WindowShow("WND_SELECT_TEAM", true);

	bSelectNewTeamWindow = true;

	SetFormatedText("TEXT_SELECTTEAM", XI_ConvertString("capNetBuy_SelectFreeTeam"));

	NetBuy_SelectTeamUpdate();

	NetBuy_OnFrame();	// update some parameters
}

// COOP: can the host start the game right now? Same rules as the server: not alone, at least 2 teams in team games.
bool NetBuy_HostCanStart()
{
	// COOP: the host may start alone (other players join the running game later), a team game with one team too.
	// The old rule (at least 2 players, at least 2 teams) is kept below for reference.
	return true;
}

/* old rule:
bool NetBuy_HostCanStart_Old()
{
	if (NET_DEBUG) { return true; }

	int i;
	int iTeamUsed[4];
	int iPlayers = 0;
	int iTeamsUsed = 0;

	for (i=0; i<4; i++) { iTeamUsed[i] = 0; }
	for (i=0; i<NET_MAXCLIENTS; i++)
	{
		if (!sti(NCClients[i].Use)) { continue; }

		iPlayers++;

		int iClientTeam = sti(NCClients[i].Team);
		if (iClientTeam >= 0 && iClientTeam < 4) { iTeamUsed[iClientTeam] = 1; }
	}
	for (i=0; i<4; i++) { iTeamsUsed = iTeamsUsed + iTeamUsed[i]; }

	if (iPlayers < 2) { return false; }
	if (sti(NCServer.GameType) != NETGAME_DEATHMATCH && iTeamsUsed < 2) { return false; }

	return true;
}

*/

// ================= lobby chat =================

void Buy_ChatUpdate()
{
	int i;

	if (!CheckAttribute(&NCLobbyChat, "Num")) { return; }

	int iNum = sti(NCLobbyChat.Num);
	int iFirst = iNum - 10;			// only the last 10 messages fit into the window
	if (iFirst < 0) { iFirst = 0; }

	string sText = "";
	for (i=iFirst; i<iNum; i++)
	{
		string sAttr = "l" + i;
		string sLine = NCLobbyChat.Lines.(sAttr);
		if (sText != "") { sText = sText + "\n"; }
		sText = sText + sLine;
	}

	SetFormatedText("TEXT_NETCHAT", sText);
}

void Buy_OnLobbyChat()
{
	if (bBuyChatOpen)
		{ Buy_ChatUpdate(); }
	else
		{ iBuyChatUnread++; }
}

void Buy_ChatOpen()
{
	if (bBuyChatOpen) { return; }
	if (bSelectNewTeamWindow) { return; }

	bBuyChatOpen = true;
	iBuyChatUnread = 0;

	XI_WindowDisable("WND_NETBUY", true);

	XI_WindowShow("WND_NETCHAT", true);
	XI_WindowDisable("WND_NETCHAT", false);

	Buy_ChatUpdate();

	SetCurrentNode("EDITBOX_CHAT");
}

void Buy_ChatClose()
{
	if (!bBuyChatOpen) { return; }

	bBuyChatOpen = false;

	XI_WindowDisable("WND_NETCHAT", true);
	XI_WindowShow("WND_NETCHAT", false);

	XI_WindowDisable("WND_NETBUY", false);

	SetCurrentNode("BTN_CHAT");
}

void Buy_ChatSend()
{
	string sMessage = GameInterface.EDITBOX_CHAT.str;
	if (sMessage == "") { return; }

	int iSMsg = NMCreate();
	NMAddClientID(iSMsg, wClientID);
	NMAddByte(iSMsg, NC_SAY);
	NMAddByte(iSMsg, NSC_SAY_CHAT);
	NMAddString(iSMsg, sMessage, 128);
	NetClient_SendMessage(iSMsg, true);
	NMDelete(iSMsg);

	GameInterface.EDITBOX_CHAT.str = "";
	SetCurrentNode("EDITBOX_CHAT");
}

void Buy_ChatEditExit()
{
	string sEditControl = GetEventData();

	if (sEditControl == "EDITBOX_ADMINLOGIN") { Buy_AdminSend(); return; }
	if (sEditControl != "EDITBOX_CHAT") { return; }

	Buy_ChatSend();
}

// ================= COOP s27: the window "Подробно": all the parameters of the server =================
bool bBuyDetailsOpen = false;

void Buy_DetailsOpen()
{
	if (bBuyDetailsOpen) { return; }
	if (bBuyAdminOpen) { return; }
	if (bBuyChatOpen) { return; }
	if (bSelectNewTeamWindow) { return; }
	if (bNetBuyProfile || bNetBuyRespawn) { return; }
	bBuyDetailsOpen = true;

	XI_WindowDisable("WND_NETBUY", true);
	XI_WindowShow("WND_DETAILS", true);
	XI_WindowDisable("WND_DETAILS", false);
	SetCurrentNode("BTN_DETAILS_CLOSE");
	SetFormatedText("TEXT_DETAILS", "Запрос параметров...");

	int iSMsg = NMCreate();
	NMAddClientID(iSMsg, wClientID);
	NMAddByte(iSMsg, NC_SERVERINFO);
	NMAddByte(iSMsg, NSC_SERVERINFO_DETAILS);
	NetClient_SendMessage(iSMsg, true);
	NMDelete(iSMsg);
}

void Buy_OnSrvDetails()
{
	string sCode = GetEventData();
	if (!bBuyDetailsOpen) { return; }
	Net_DetailsShow("TEXT_DETAILS", sCode);
}

void Buy_DetailsClose()
{
	if (!bBuyDetailsOpen) { return; }
	bBuyDetailsOpen = false;
	XI_WindowDisable("WND_DETAILS", true);
	XI_WindowShow("WND_DETAILS", false);
	XI_WindowDisable("WND_NETBUY", false);
	SetCurrentNode("BTN_DETAILS");
}

// ================= admin login (the password is set by the host on the Create game screen) =================

void Buy_AdminOpen()
{
	if (bBuyAdminOpen) { return; }
	if (bBuyChatOpen) { return; }
	if (bSelectNewTeamWindow) { return; }
	if (bNetClientIsAdmin) { return; }

	bBuyAdminOpen = true;

	SetFormatedText("TEXT_ADMINLOGIN_RESULT", "");
	GameInterface.EDITBOX_ADMINLOGIN.str = "";

	XI_WindowDisable("WND_NETBUY", true);

	XI_WindowShow("WND_NETADMINLOGIN", true);
	XI_WindowDisable("WND_NETADMINLOGIN", false);

	SetCurrentNode("EDITBOX_ADMINLOGIN");
}

void Buy_AdminClose()
{
	if (!bBuyAdminOpen) { return; }

	bBuyAdminOpen = false;

	XI_WindowDisable("WND_NETADMINLOGIN", true);
	XI_WindowShow("WND_NETADMINLOGIN", false);

	XI_WindowDisable("WND_NETBUY", false);

	SetCurrentNode("BTN_ADMIN");
}

void Buy_AdminSend()
{
	string sPassword = GameInterface.EDITBOX_ADMINLOGIN.str;
	if (sPassword == "") { return; }

	NetClient_AdminLogin(sPassword);

	GameInterface.EDITBOX_ADMINLOGIN.str = "";
	SetCurrentNode("EDITBOX_ADMINLOGIN");
}

// the answer of the server
void Buy_OnAdminStatus()
{
	int iAdmin = GetEventData();

	if (!bBuyAdminOpen) { return; }

	if (iAdmin)
	{
		Buy_AdminClose();
	}
	else
	{
		SetFormatedText("TEXT_ADMINLOGIN_RESULT", XI_ConvertString("capNetAdmin_WrongPassword"));
	}
}

void NetBuy_OnFrame()
{
	int i;
	int iTeamQuantity[4];
	if (bNetBuyProfile) { Buy_ProfFrame(); return; }		// COOP: the ship of a profile: no server
	if (bNetBuyRespawn) { return; }		// COOP s46: the ship of the next respawn: no lobby

	if (bStartCountDown)
	{
		iClientStartTimer = iClientStartTimer - iRealDeltaTime;
	}
	int iCurTime = iClientStartTimer / 1000;
	if (iCurTime < 0) { iCurTime = 0; }

	string sMinutes, sSeconds;

	int iMinutes = iCurTime / 60;
	int iSeconds = iCurTime - iMinutes * 60;
	if (iMinutes < 10) { sMinutes = "0" + iMinutes; } else { sMinutes = iMinutes; }
	if (iSeconds < 10) { sSeconds = "0" + iSeconds; } else { sSeconds = iSeconds; }
	string sValue = "#" + XI_ConvertString("btnNetBuy_Start") + " " + sMinutes + ":" + sSeconds;

	// COOP: the game is already running: "Join the battle" for everybody
	if (sti(NCServer.GameRunning))
	{
		sValue = XI_ConvertString("btnNetBuy_Join");
		if (sValue == "" || sValue == "btnNetBuy_Join") { sValue = "Присоединиться"; }		// the key is missing in the text ini
		sValue = "#" + sValue;
		if (bBuyHostStartSent)
		{
			iBuyHostStartTimer = iBuyHostStartTimer - iRealDeltaTime;
			if (iBuyHostStartTimer <= 0) { bBuyHostStartSent = false; }
		}
		Button_SetEnable("BTN_START", bServerInfoReady && !bBuyHostStartSent);
	}
	else
	{
	// COOP: the host (the machine that runs the server) can start the game at any moment, without waiting for the countdown
	if (NetServer_IsStarted() || bNetClientIsAdmin)
	{
		if (!bStartCountDown) { sValue = "#" + XI_ConvertString("btnNetBuy_Start"); }

		// the request was sent but the server did not answer (it refused): unlock the button again
		if (bBuyHostStartSent)
		{
			iBuyHostStartTimer = iBuyHostStartTimer - iRealDeltaTime;
			if (iBuyHostStartTimer <= 0) { bBuyHostStartSent = false; }
		}

		Button_SetEnable("BTN_START", bServerInfoReady && !bBuyHostStartSent && NetBuy_HostCanStart());
	}
	}

	Button_SetText("BTN_START", sValue);
	Buy_GameSettingsShow();
	Buy_LBotsCaption();
	Buy_ChoiceTick();

	// lobby chat button: show the number of unread messages
	if (iBuyChatUnread != iBuyChatUnreadShown)
	{
		iBuyChatUnreadShown = iBuyChatUnread;
		string sChatBtn = "#" + XI_ConvertString("btnNetBuy_Chat");
		if (iBuyChatUnread > 0) { sChatBtn = sChatBtn + " (" + iBuyChatUnread + ")"; }
		Button_SetText("BTN_CHAT", sChatBtn);
	}

	// admin button: after a successful login it only shows that you are an admin
	if (bNetClientIsAdmin != bBuyAdminShown)
	{
		bBuyAdminShown = bNetClientIsAdmin;
		if (bNetClientIsAdmin) { NetClient_AdminSend(NET_ADMIN_SYNC, DST_INVALID, 0.0); }		// COOP s38: the bot respawn switch
		if (bNetClientIsAdmin)
		{
			Button_SetText("BTN_ADMIN", "#" + XI_ConvertString("btnNetBuy_AdminOn"));
			Button_SetEnable("BTN_ADMIN", false);
		}
	}

	// COOP s38: the switch "the bots respawn" beside "Bots": the admin changes it, the others see it
	Buy_BotRespFrame();

	// set selectteam button
	ref rOurClient = NetClient_GetThisClient();
	Button_SetEnable("BTN_SELECTTEAM_OK", !(iSelectedTeam == sti(rOurClient.Team)));

	// update team count
	for (i=0; i<4; i++) { iTeamQuantity[i] = 0;	}
	for (i=0; i<NET_MAXCLIENTS; i++) 
	{
		if ( sti(NCClients[i].Use) )
		{
			int iTeam = sti(NCClients[i].Team);
			if (iTeam >= 0)	{ iTeamQuantity[iTeam] = iTeamQuantity[iTeam] + 1; }
		}
	}

	for (i=0; i<4; i++) 
	{ 
		if (i >= sti(NCServer.NumTeams)) 
			{ StringCollection_SetText("STRINGS_SELECTTEAM_QUANTITY", i + 1, "#"); }
		else 
			{ StringCollection_SetTextValue("STRINGS_SELECTTEAM_QUANTITY", i + 1, iTeamQuantity[i]); }
	}
}

void Buy_InitTable(string sTable)
{
	GameInterface.(sTable).hr.td1.str = XI_ConvertString("capNetBuy_TableFace");
	GameInterface.(sTable).hr.td1.scale = 0.85;
	GameInterface.(sTable).hr.td2.str = XI_ConvertString("capNetBuy_TableShip");
	GameInterface.(sTable).hr.td2.scale = 0.85;
	GameInterface.(sTable).hr.td3.str = XI_ConvertString("capNetBuy_TableNames");
	GameInterface.(sTable).hr.td3.scale = 0.85;
	GameInterface.(sTable).hr.td4.str = "";
	GameInterface.(sTable).hr.td5.str = XI_ConvertString("capNetBuy_TableTeams");
	GameInterface.(sTable).hr.td5.scale = 0.85;
	GameInterface.(sTable).hr.td6.str = XI_ConvertString("capNetBuy_TableReady");
	GameInterface.(sTable).hr.td6.scale = 0.85;
	GameInterface.(sTable).hr.td7.str = XI_ConvertString("capNetBuy_TableKick");
	GameInterface.(sTable).hr.td7.scale = 0.85;
}

void Buy_ClientDrop()
{
	Buy_FillPlayersTable("TABLE_PLAYERS");
	Buy_UpdatePlayersQuantity();
}

void Buy_NewClientConnect()
{
	Buy_FillPlayersTable("TABLE_PLAYERS");
	Buy_UpdatePlayersQuantity();
}

void Buy_PlayerInfo_BuyReady()
{
	Buy_FillPlayersTable("TABLE_PLAYERS");
}

void Buy_PlayerInfo_StartReady()
{
	Buy_FillPlayersTable("TABLE_STARTGAME_WAIT_PLAYERS");
}

void Buy_PlayerInfo_Team()
{
	Buy_FillPlayersTable("TABLE_PLAYERS");
}

void Buy_PlayerInfo_ShipType()
{
	int wNetClientID = GetEventData();
	int iShipType = GetEventData();

	Buy_FillPlayersTable("TABLE_PLAYERS");
}

void Buy_GetFullServerInfo()
{
	XI_WindowDisable("WND_NETBUY", true);

	XI_WindowShow("WND_STARTGAME_WAIT", false);
	XI_WindowDisable("WND_STARTGAME_WAIT", true);

	XI_WindowShow("WND_NETBUY_WAITINFO", true);
	XI_WindowDisable("WND_NETBUY_WAITINFO", false);

	SetCurrentNode("BTN_WAITINFO_DISCONNECT");

	int iSMsg = NMCreate();
	NMAddClientID(iSMsg, wClientID);
	NMAddByte(iSMsg, NC_SERVERINFO);
	NMAddByte(iSMsg, NSC_SERVERINFO_GET);
	NetClient_SendMessage(iSMsg, true);
	NMDelete(iSMsg);

	fServerInfoPiecesLoaded = 0;
	SetEventHandler("NetClient_OnServerInfo_Event", "Buy_OnServerInfo", 0);
}

void Buy_OnServerInfo()
{
	int i;
	int iCode = GetEventData();
	
	fServerInfoPiecesLoaded = fServerInfoPiecesLoaded + 1.0;
	float fMaxPieces = 5.0 + stf(NCServer.NumClients) + stf(Net_GetShipsNum());

	int left = 313;		// COOP s35: the window is in the middle of the wide screen (net\Buy.ini +133)
	int right = MakeInt(left + (773 - left) * (fServerInfoPiecesLoaded / fMaxPieces));

	SendMessage(&GameInterface, "lslllll", MSG_INTERFACE_MSG_TO_NODE, "RECTANGLE_WAITINFO_PROGRESS", 0, left, 273, right, 299);

	if (fServerInfoPiecesLoaded >= fMaxPieces)
	{
		bServerInfoReady = true;

		iMaxCredit = sti(NCServer.Credit);

		XI_WindowDisable("WND_NETBUY", false);

		XI_WindowShow("WND_NETBUY_WAITINFO", false);
		XI_WindowDisable("WND_NETBUY_WAITINFO", true);

		// update skills costs 
		for (i=0; i<Net_GetSkillsNum(); i++)	
		{ 
			iSkillsQuantity[i] = 1;
			StringCollection_SetTextValue("STRINGS_SKILLS_QUANTITY", i + 1, iSkillsQuantity[i]); 
			StringCollection_SetTextValue("STRINGS_SKILLS_MONEY", i + 1, Net_GetSkillCost(i, 1)); 
		}

		// update perks costs 
		for (i=0; i<6; i++)	
		{ 
			iPerks[i] = 0; 
			StringCollection_SetTextValue("STRINGS_PERKS_MONEY", i + 1, Net_GetPerkCost(i)); 
		}

		// update goods costs 
		iCrewPercent = 100;
		StringCollection_SetText("STRINGS_AMMO_QUANTITY", 8, "#" + iCrewPercent + "%");
		iCannonsPercent = 100;
		StringCollection_SetText("STRINGS_CANNONS_PERCENT", 1, "#" + iCannonsPercent + "%");
		CheckButton_SetState("RADIO_START_CHARGE", 1, true);		// start charge: balls
		for (i=0; i<Net_GetGoodsNum(); i++)	
		{ 
			iNetGoodsQuantity[i] = 0;
			StringCollection_SetTextValue("STRINGS_AMMO_MONEY", i + 1, Net_GetGoodCost(i, sti(NetGoods[i].Units))); 
			StringCollection_SetTextValue("STRINGS_AMMO_QUANTITY", i + 1, iNetGoodsQuantity[i]); 
		}

		Buy_SeaPerksReset();		// COOP s28
		Buy_SetShip(1);

		string sIslandMap = "OpenSea";
		if (NCServer.IslandID != "") { sIslandMap = NCServer.IslandID; }
		// COOP: a land duel: the picture is the loading screen of its location, the name is the location
		bool bBuyLand = NetLand_IsAdvert(sIslandMap);
		if (bBuyLand)
		{
			string sBuyLandLoc = NetLand_AdvertLoc(sIslandMap);
			Picture_SetPicture("SERVER_MAP_PICTURE", NetLand_LoadingImage(sBuyLandLoc));
			SetFormatedText("CAPTION_MAP_NAME", NetLand_LocName(sBuyLandLoc));
		}
		else
		{
			Picture_SetPicture("SERVER_MAP_PICTURE", "Interfaces\Net\Islands\" + sIslandMap + ".tga");
			SetFormatedText("CAPTION_MAP_NAME", XI_ConvertString(sIslandMap));
		}

		Buy_FillPlayersTable("TABLE_PLAYERS");
		Buy_UpdatePlayersQuantity();

		if (bBuyLand) { StringCollection_SetText("STRINGS_MAP_INFO_VALUE", 1, "#Сухопутная дуэль"); }
		else { StringCollection_SetText("STRINGS_MAP_INFO_VALUE", 1, "txtCS_" + NetModes[sti(NCServer.GameType)].Name); }
		StringCollection_SetText("STRINGS_MAP_INFO_VALUE", 4, "#" + NCServer.MaxShipClass);

		Buy_AddGood(1, 2);
		Buy_AddGood(7, 2);

		// COOP: the loadout of the land duel can be chosen here; the ship of the profile comes back (if the money allows)
		SetNodeUsing("BTN_LOBBY_LOADOUT", bBuyLand || NetBoard_RealOnClient());		// COOP s37: the loadout of the boarding deck
		// COOP: the preview of the ship: the ship chosen in the shop of the profile (Buy_ProfPreview keeps it as "NetPreview")
		if (iNetPreview == 2) { sNetLandProfNick = "NetPreview"; }
		Buy_ProfRestore();
		if (iNetPreview == 2)
		{
			sNetLandProfNick = "";
			DeleteAttribute(&NCProfiles, "NetShip.NetPreview");
		}
		// COOP: the lobby of a preview starts the game at once
		if (iNetPreview > 0) { PostEvent("Buy_PreviewGo", 500); }
	}
}

// COOP: the lobby of a preview (net\Land.c: NetPreview_*): "Start" by itself
void Buy_PreviewGo()
{
	if (iNetPreview <= 0) { return; }
	if (!bServerInfoReady) { PostEvent("Buy_PreviewGo", 300); return; }
	Trace("COOP: preview " + iNetPreview + ": the lobby starts the game");
	OnTmpStart();
}

// COOP: "Предпросмотр" in the shop of the ship of the profile: the chosen ship alone in the open sea. The choice is not
// saved in the profile: it goes to the lobby of the preview as the profile "NetPreview" (deleted there)
void Buy_ProfPreview()
{
	if (!bNetBuyProfile) { return; }
	if (NetPreview_Busy())		// COOP s32 (net\Land.c)
	{
		SetFormatedText("TEXT_PROF_MONEY", "Предпросмотр недоступен: запущен сервер");
		return;
	}
	string sKeepProf = sNetLandProfNick;
	sNetLandProfNick = "NetPreview";
	Buy_ProfSave(true);
	sNetLandProfNick = sKeepProf;
	bNetBuyProfile = false;
	sNetLandProfNick = "";
	bNetLobbyChatActive = false;
	// COOP s30: the ship in the preview has the name, the sails and the flag of the window "Герб/Паруса/Флаг"
	string sPvShip = GameInterface.EDITBOX_PROF_SHIPNAME.str;
	if (sPvShip != "") { NetClient.ShipName = sPvShip; }
	NetClient.SailColor = sti(SailsColors[Buy_ProfColorIdx()].color);
	NetClient.FlagImage = Buy_ProfListName("FLAGSLIST", "_emptyflag_");
	NetClient.SailImage = Buy_ProfListName("SAILSLIST", "_emptysail_");
	DeleteAttribute(&NetPSEdit, "Ret");
	Trace("COOP: preview 2: flag <" + NetClient.FlagImage + ">, gerald <" + NetClient.SailImage + ">, sails color " + NetClient.SailColor);
	NetPS_SceneDelete();		// net\MenuScene.c
	NetPreview_Begin(2);
	IDoExit(RC_INTERFACE_NET_FINDGAME);
}

// ========================================
// COOP: the loadout of the land duel in the lobby
// ========================================
void Buy_LoOpen()
{
	XI_WindowDisable("WND_NETBUY", true);
	XI_WindowShow("WND_LOADOUT", true);
	XI_WindowDisable("WND_LOADOUT", false);
	NetLand_UiLoOpen();
}

void Buy_LoBack()
{
	bNetLandUiLo = false;
	XI_WindowShow("WND_LOADOUT", false);
	XI_WindowDisable("WND_LOADOUT", true);
	XI_WindowDisable("WND_NETBUY", false);
	SetCurrentNode("BTN_LOBBY_LOADOUT");
}

void Buy_LoApply()
{
	aref lo;
	makearef(lo, oNetLandLo);
	if (NetLand_LoCost(lo) > NetLand_LoBudget()) { Log_SetStringToLog("Не хватает денег на это снаряжение"); return; }
	NetLand_ProfSaveLo();
	Buy_LoBack();
}

// ========================================
// COOP: the ship of the profile (Profiles.nsv): saved when the game starts, set again in the next lobby. What the money of
// this server does not allow stays as it was; the profile keeps the choice (it is saved again only on a server with as much
// money or more)
// ========================================
void Buy_ProfSave(bool bForce)
{
	string sN = NetLand_ProfNick();
	if (!bForce && CheckAttribute(&NCProfiles, "NetShip." + sN + ".Credit"))
	{
		if (iMaxCredit < sti(NCProfiles.NetShip.(sN).Credit)) { Trace("COOP: the ship of the profile is kept (a poorer server)"); return; }
	}
	NCProfiles.NetShip.(sN).Credit = iMaxCredit;
	NCProfiles.NetShip.(sN).Ship = iCurrentShipType;
	NCProfiles.NetShip.(sN).Hull = iCurrentShipUpgradeHull;
	NCProfiles.NetShip.(sN).Rig = iCurrentShipUpgradeRigging;
	NCProfiles.NetShip.(sN).Cannon = iCurrentCannonType;
	NCProfiles.NetShip.(sN).CannonUpg = iCurrentCannonUpgrade;
	NCProfiles.NetShip.(sN).CannonPct = iCannonsPercent;
	NCProfiles.NetShip.(sN).Crew = iCrewPercent;
	NCProfiles.NetShip.(sN).Charge = Buy_GetStartCharge();
	int i;
	for (i=0; i<Net_GetSkillsNum(); i++) { string sK = "k" + i; NCProfiles.NetShip.(sN).Sk.(sK) = iSkillsQuantity[i]; }
	for (i=0; i<Net_GetGoodsNum(); i++) { string sG = "g" + i; NCProfiles.NetShip.(sN).Goods.(sG) = iNetGoodsQuantity[i]; }
	NCProfiles.NetShip.(sN).SeaPerks = Buy_SeaPerksMask();		// COOP s28
	NetLand_ProfSave();
	Trace("COOP: the ship is saved in the profile " + sN);
}

void Buy_ProfRestore()
{
	string sN = NetLand_ProfNick();
	if (!CheckAttribute(&NCProfiles, "NetShip." + sN + ".Ship")) { return; }
	aref ap;
	makearef(ap, NCProfiles.NetShip.(sN));
	int i;
	int iGuard;
	int iBefore;
	// the ship
	int iShip = sti(ap.Ship);
	int iDefShip = iCurrentShipType;
	iGuard = 0;
	while (iCurrentShipType < iShip && iGuard < 200)
	{
		iBefore = iCurrentShipType;
		Buy_SetShip(1);
		if (iCurrentShipType == iBefore) { break; }
		iGuard++;
	}
	if (iCurrentShipType != iShip)
	{
		Trace("COOP: the ship of the profile is not allowed here: the first ship");
		iGuard = 0;
		while (iCurrentShipType > iDefShip && iGuard < 200)
		{
			iBefore = iCurrentShipType;
			Buy_SetShip(-1);
			if (iCurrentShipType == iBefore) { break; }
			iGuard++;
		}
		return;
	}
	Buy_RestoreKit(sti(ap.Hull), sti(ap.Rig), sti(ap.Cannon), sti(ap.CannonUpg));
	// the percent of the cannons and of the crew
	iGuard = 0;
	while (iCannonsPercent > sti(ap.CannonPct) && iGuard < 20) { iBefore = iCannonsPercent; Buy_SetCannonsPercent(-1); if (iCannonsPercent == iBefore) { break; } iGuard++; }
	iGuard = 0;
	while (iCrewPercent > sti(ap.Crew) && iGuard < 40) { iBefore = iCrewPercent; Buy_SetCrewPercent(-1); if (iCrewPercent == iBefore) { break; } iGuard++; }
	// the skills
	for (i=0; i<Net_GetSkillsNum(); i++)
	{
		string sK = "k" + i;
		if (!CheckAttribute(ap, "Sk." + sK)) { continue; }
		int iWant = sti(ap.Sk.(sK));
		iGuard = 0;
		while (iSkillsQuantity[i] < iWant && iGuard < 20) { iBefore = iSkillsQuantity[i]; Buy_AddSkills(i + 1, 1); if (iSkillsQuantity[i] == iBefore) { break; } iGuard++; }
	}
	// the goods (one step of each kind: its units)
	for (i=0; i<Net_GetGoodsNum(); i++)
	{
		string sG = "g" + i;
		if (!CheckAttribute(ap, "Goods." + sG)) { continue; }
		int iWantG = sti(ap.Goods.(sG));
		iGuard = 0;
		while (iNetGoodsQuantity[i] > iWantG && iGuard < 400) { iBefore = iNetGoodsQuantity[i]; Buy_AddGood(i + 1, -1); if (iNetGoodsQuantity[i] == iBefore) { break; } iGuard++; }
		iGuard = 0;
		while (iNetGoodsQuantity[i] < iWantG && iGuard < 400) { iBefore = iNetGoodsQuantity[i]; Buy_AddGood(i + 1, 1); if (iNetGoodsQuantity[i] == iBefore) { break; } iGuard++; }
	}
	// the start charge (balls, grapes, knippels, bombs)
	if (CheckAttribute(ap, "Charge"))
	{
		int iCh = sti(ap.Charge);
		if (iCh >= 0 && iCh < 4) { CheckButton_SetState("RADIO_START_CHARGE", iCh + 1, true); }
	}
	// COOP s28: the sea perks (those the money allows)
	if (CheckAttribute(ap, "SeaPerks"))
	{
		int iPkMask = sti(ap.SeaPerks);
		for (i=0; i<NET_SPERK_NUM; i++)
		{
			if (Net_SeaPerkBit(iPkMask, i)) { Buy_SetSeaPerk(i, true); }
		}
		Buy_SeaPerksFill();
	}
	Buy_UpdateCredit();
	Trace("COOP: the ship of the profile " + sN + " is set");
}

// COOP: after a change of the ship: its upgrades, cannons (and the goods) stay if the new ship takes them; otherwise the
// defaults of the new ship (the old way)
void Buy_RestoreKit(int iHull, int iRig, int iCannon, int iCannonUpg)
{
	int iGuard = 0;
	int iBefore;
	while (iCurrentShipUpgradeHull < iHull && iGuard < 10) { iBefore = iCurrentShipUpgradeHull; Buy_SetShipUpgradeHull(1); if (iCurrentShipUpgradeHull == iBefore) { break; } iGuard++; }
	iGuard = 0;
	while (iCurrentShipUpgradeRigging < iRig && iGuard < 10) { iBefore = iCurrentShipUpgradeRigging; Buy_SetShipUpgradeRigging(1); if (iCurrentShipUpgradeRigging == iBefore) { break; } iGuard++; }
	if (iCannon < 0) { return; }
	int iDefCannon = iCurrentCannonType;
	iGuard = 0;
	while (iCurrentCannonType < iCannon && iGuard < 64) { iBefore = iCurrentCannonType; Buy_SetCannon(1); if (iCurrentCannonType == iBefore) { break; } iGuard++; }
	if (iCurrentCannonType != iCannon)
	{
		// the cannons of the old ship do not fit: the first cannons of the new one
		iGuard = 0;
		while (iCurrentCannonType > iDefCannon && iGuard < 64) { iBefore = iCurrentCannonType; Buy_SetCannon(-1); if (iCurrentCannonType == iBefore) { break; } iGuard++; }
		return;
	}
	if (iCannonUpg > 0) { Buy_SetCannonUpgrade(iCannonUpg, true); }
}

void OnReset()
{
}

void Buy_SetShipUpgradeHull(int iDirection)
{
	if (iDirection == 0) { return; }
	iDirection = Clampi(iDirection, -1, 1);

	int iOldPrice = Net_GetShipUpgradeHullCost(iCurrentShipType, iCurrentShipUpgradeHull);
	int iNumUpgrades = 3;
	int iNextUpgrade = iCurrentShipUpgradeHull;
	for (int i=0; i<iNumUpgrades; i++)
	{
		iNextUpgrade = iNextUpgrade + iDirection;
		if (iNextUpgrade >= iNumUpgrades) { return; }//iNextUpgrade = 0;
		if (iNextUpgrade < 0) { return; }//iNextUpgrade = iNumUpgrades - 1;

		int iNewPrice = Net_GetShipUpgradeHullCost(iCurrentShipType, iNextUpgrade);

		if (iCurrentCredit - iOldPrice + iNewPrice < iMaxCredit)
		{
			iCurrentCredit = iCurrentCredit - iOldPrice + iNewPrice;
			iCurrentShipUpgradeHull = iNextUpgrade;

			string sCurrentUpgrade = "HULL_UPGRADE_" + (iCurrentShipUpgradeHull + 1);
			Picture_SetGroupPicture("PICTURE_SHIP_UPGRADE_HULL", sCurrentUpgrade, "hull");
			StringCollection_SetTextValue("STRINGS_SHIP_UPGRADE_MONEY", 1, iNewPrice);
			Buy_UpdateCredit();
			break;
		}
	}
}
void Buy_DecUpgradeHull() { Buy_SetShipUpgradeHull(-1); }
void Buy_AddUpgradeHull() { Buy_SetShipUpgradeHull(1); }

void Buy_SetShipUpgradeRigging(int iDirection)
{
	if (iDirection == 0) { return; }
	iDirection = Clampi(iDirection, -1, 1);

	int iOldPrice = Net_GetShipUpgradeRiggingCost(iCurrentShipType, iCurrentShipUpgradeRigging);
	int iNumUpgrades = 4;
	int iNextUpgrade = iCurrentShipUpgradeRigging;
	for (int i=0; i<iNumUpgrades; i++)
	{
		iNextUpgrade = iNextUpgrade + iDirection;
		if (iNextUpgrade >= iNumUpgrades) { return; }//iNextUpgrade = 0;
		if (iNextUpgrade < 0) { return; }//iNextUpgrade = iNumUpgrades - 1;

		int iNewPrice = Net_GetShipUpgradeRiggingCost(iCurrentShipType, iNextUpgrade);

		if (iCurrentCredit - iOldPrice + iNewPrice < iMaxCredit)
		{
			iCurrentCredit = iCurrentCredit - iOldPrice + iNewPrice;
			iCurrentShipUpgradeRigging = iNextUpgrade;

			string sCurrentUpgrade = "SAILS_UPGRADE_" + (iCurrentShipUpgradeRigging + 1);
			Picture_SetGroupPicture("PICTURE_SHIP_UPGRADE_RIGGING", sCurrentUpgrade, "sails");
			StringCollection_SetTextValue("STRINGS_SHIP_UPGRADE_MONEY", 2, iNewPrice);
			Buy_UpdateCredit();
			break;
		}
	}
}
void Buy_DecUpgradeRigging() { Buy_SetShipUpgradeRigging(-1); }
void Buy_AddUpgradeRigging() { Buy_SetShipUpgradeRigging(1); }

void Buy_SetShip(int iDirection)
{
	if (iDirection == 0) { return; }
	iDirection = Clampi(iDirection, -1, 1);

	int iOldCannonsNum = Buy_CannonsNum(iCurrentShipType);
	int iOldPrice = Net_GetShipCost(iCurrentShipType)
	// COOP: the kit of the old ship (kept if the new one takes it: Buy_RestoreKit)
	int iKeepHull = iCurrentShipUpgradeHull;
	int iKeepRig = iCurrentShipUpgradeRigging;
	int iKeepCannon = iCurrentCannonType;
	int iKeepCannonUpg = iCurrentCannonUpgrade;
	bool bKeepKit = (iCurrentShipType >= 0);

	int iMaxShipClass = sti(NCServer.MaxShipClass);

	// find next ship
	int iNumShips = Net_GetShipsNum();
	int iNextShipType = iCurrentShipType;
	for (int i=0; i<iNumShips; i++)
	{
		iNextShipType = iNextShipType + iDirection;
		if (iNextShipType >= iNumShips) { return; }//iNextShipType = 0;
		if (iNextShipType < 0) { return; }//iNextShipType = iNumShips - 1;
		
		ref rShip = Net_GetShipByIndex(iNextShipType);
		if (sti(rShip.TradeOff)) { continue; }
		if (sti(rShip.Class) < iMaxShipClass) { continue; }
		int iNewPrice = Net_GetShipCost(iNextShipType);
		// crew is hired for the new ship with the same percent
		int iCrewDelta = Net_GetCrewCost(iNextShipType, iCrewPercent) - Net_GetCrewCost(iCurrentShipType, iCrewPercent);

		if (iCurrentCredit - iOldPrice + iNewPrice + iCrewDelta < iMaxCredit)
		{
			if (iCurrentShipType >= 0)
			{
				// sell old cannons and cannons upgrade
				if (iCurrentCannonType >= 0)
				{
					iCurrentCredit = iCurrentCredit - iOldCannonsNum * Net_GetCannonCost(iCurrentCannonType);
					iCurrentCannonType = -1;

					// sell old cannon upgrade
					if (iCurrentCannonUpgrade >= 0)
					{
						iCurrentCredit = iCurrentCredit - iOldCannonsNum * Net_GetCannonUpgradeCost(iCurrentCannonType, iCurrentCannonUpgrade);
						iCurrentCannonUpgrade = -1;
					}
				}

				// sell old hull upgrades
				if (iCurrentShipUpgradeHull >= 0)
				{
					iCurrentCredit = iCurrentCredit - Net_GetShipUpgradeHullCost(iCurrentShipType, iCurrentShipUpgradeHull);
					iCurrentShipUpgradeHull = -1;
				}

				// sell old rigging upgrades
				if (iCurrentShipUpgradeRigging >= 0)
				{
					iCurrentCredit = iCurrentCredit - Net_GetShipUpgradeRiggingCost(iCurrentShipType, iCurrentShipUpgradeRigging);
					iCurrentShipUpgradeRigging = -1;
				}
		}

			// buy new ship
			iCurrentShipType = iNextShipType;
			iCurrentCredit = iCurrentCredit - iOldPrice + iNewPrice + iCrewDelta;
			Buy_UpdateCrewMoney();

			string sGroup = "SHIPS_" + rShip.Name;
			Picture_SetGroupPicture("PICTURE_SHIP", sGroup, "ship");
			
			// buy new cannons
			iCurrentCannonType = -1; Buy_SetCannon(1);
			// set price of cannons
			StringCollection_SetTextValue("STRINGS_SHIPCANNONS_MONEY", 1, iNewPrice);

			iCurrentShipUpgradeHull = -1;
			iCurrentShipUpgradeRigging = -1;
			Buy_SetShipUpgradeHull(1);
			Buy_SetShipUpgradeRigging(1);

			StringCollection_SetText("STRINGS_SHIPCANNONS_TYPE", 1, rShip.Name);
			if (bKeepKit) { Buy_RestoreKit(iKeepHull, iKeepRig, iKeepCannon, iKeepCannonUpg); }
			Buy_FitCargo();		// COOP: the goods of a bigger ship may not fit into the new hold
			Buy_UpdateCredit();
			Buy_Server_UpdateShipType();
			break;
		}
	}
}
// COOP s31: the right click on a part of the buy screen or of the loadout window: its description (net/NetInfo.c)
void Buy_InfoShow()
{
	string sNode = GetCurrentNode();
	if (NetInfo_Land(sNode)) { return; }
	int iNum = Buy_CannonsNum(iCurrentShipType);
	switch (sNode)
	{
		case "PICTURE_SHIP":				NetInfo_Ship(iCurrentShipType); break;
		case "SBAR_SHIP_UPGRADE_HULL":		NetInfo_HullUpg(iCurrentShipType, iCurrentShipUpgradeHull); break;
		case "SBAR_SHIP_UPGRADE_RIGGING":	NetInfo_RigUpg(iCurrentShipType, iCurrentShipUpgradeRigging); break;
		case "PICTURE_CANNON":				NetInfo_Cannon(iCurrentCannonType, iNum); break;
		case "RADIO_CANNONS_UPGRADELIST":	NetInfo_CannonUpg(iCurrentCannonType, iNum, iCurrentCannonUpgrade); break;
		case "SBAR_SKILL_ACCURACY":			NetInfo_SeaSkill(0, iSkillsQuantity[0]); break;
		case "SBAR_SKILL_CANNONS":			NetInfo_SeaSkill(1, iSkillsQuantity[1]); break;
		case "SBAR_SKILL_SAILING":			NetInfo_SeaSkill(2, iSkillsQuantity[2]); break;
		case "SBAR_SKILL_DEFENCE":			NetInfo_SeaSkill(3, iSkillsQuantity[3]); break;
		case "SBAR_SKILL_REPAIR":			NetInfo_SeaSkill(4, iSkillsQuantity[4]); break;
		case "SBAR_SKILL_GRAPPLING":		NetInfo_SeaSkill(5, iSkillsQuantity[5]); break;
		case "SBAR_GOOD_BALLS":				NetInfo_Good("Balls", iNetGoodsQuantity[0]); break;
		case "SBAR_GOOD_GRAPES":			NetInfo_Good("Grapes", iNetGoodsQuantity[1]); break;
		case "SBAR_GOOD_KNIPPELS":			NetInfo_Good("Knippels", iNetGoodsQuantity[2]); break;
		case "SBAR_GOOD_BOMBS":				NetInfo_Good("Bombs", iNetGoodsQuantity[3]); break;
		case "SBAR_GOOD_PLANKS":			NetInfo_Good("Planks", iNetGoodsQuantity[4]); break;
		case "SBAR_GOOD_SAILCLOTH":			NetInfo_Good("Sailcloth", iNetGoodsQuantity[5]); break;
		case "SBAR_GOOD_POWDER":			NetInfo_Good("Powder", iNetGoodsQuantity[6]); break;
		case "SBAR_GOOD_CREW":				NetInfo_Crew(iCurrentShipType, iCrewPercent); break;
		case "TABLE_SEA_PERKS":				NetInfo_SeaPerk(NetInfo_Row(sNode) - 1, "Двойной щелчок по строке покупает или продаёт перк."); break;
	}
}

void Buy_ShipLeft() { Buy_SetShip(-1); }
void Buy_ShipRight() { Buy_SetShip(1); }

void Buy_SetCannonUpgrade(int iNewCannonUpgrade, bool bSetRadioButton)
{
	int iNumShipCannons = Buy_CannonsNum(iCurrentShipType);
	
	int iOldPrice = iNumShipCannons * Net_GetCannonUpgradeCost(iCurrentCannonType, iCurrentCannonUpgrade);
	int iNewPrice = iNumShipCannons * Net_GetCannonUpgradeCost(iCurrentCannonType, iNewCannonUpgrade);

	if (iCurrentCredit - iOldPrice + iNewPrice < iMaxCredit)
	{
		if (iCurrentCannonUpgrade >= 0) { iCurrentCredit = iCurrentCredit - iOldPrice; }

		iCurrentCannonUpgrade = iNewCannonUpgrade;
		iCurrentCredit = iCurrentCredit + iNewPrice;

		if (bSetRadioButton)
		{
			CheckButton_SetState("RADIO_CANNONS_UPGRADELIST", iCurrentCannonUpgrade + 1, true);
		}

		Buy_UpdateCredit();
	}
}

void Buy_SetCannon(int iDirection)
{
	if (iDirection == 0) { return; }
	iDirection = Clampi(iDirection, -1, 1);	

	int iNumShipCannons = Buy_CannonsNum(iCurrentShipType);
	bool bAutoPick = (iCurrentCannonType < 0);		// COOP: the ship was changed: the goods are trimmed afterwards

	int iOldPrice = iNumShipCannons * Net_GetCannonCost(iCurrentCannonType);

	// find next cannon
	int iNumCannons = Net_GetCannonsNum();
	int iNextType = iCurrentCannonType;
	for (int i=0; i<iNumCannons; i++)
	{
		iNextType = iNextType + iDirection;
		if (iNextType >= iNumCannons) { return; }//iNextType = 0;
		if (iNextType < 0) { return; }//iNextType = iNumCannons - 1;

		ref rCannon = Net_GetCannonByIndex(iNextType);
		if (sti(rCannon.TradeOff)) { continue; }

		int iNewPrice = iNumShipCannons * Net_GetCannonCost(iNextType);

		// COOP: deadweight - a heavier cannon type must fit into the hold
		if (!bAutoPick && Buy_LoadWith(iCurrentShipType, iNextType, iNumShipCannons, iCrewPercent, Buy_GoodsWeight()) > Net_GetShipCapacity(iCurrentShipType)) { continue; }

		if (iCurrentCredit - iOldPrice + iNewPrice < iMaxCredit)
		{
			// sell old cannon upgrade
			if (iCurrentCannonUpgrade >= 0)
			{
				iCurrentCredit = iCurrentCredit - iNumShipCannons * Net_GetCannonUpgradeCost(iCurrentCannonType, iCurrentCannonUpgrade);
				iCurrentCannonUpgrade = -1;
			}

			iCurrentCannonType = iNextType;
			iCurrentCredit = iCurrentCredit - iOldPrice + iNewPrice;
			Picture_SetGroupPicture("PICTURE_CANNON", "CANNONS_UPGRADE_1", rCannon.picture);
			
			// set price of ship
			StringCollection_SetTextValue("STRINGS_SHIPCANNONS_MONEY", 2, iNewPrice);

			string sCannonTypeText = XI_ConvertString(rCannon.Name);
			string sCannonCaliberText = XI_ConvertString("caliber" + rCannon.caliber);
			StringCollection_SetText("STRINGS_SHIPCANNONS_TYPE", 2, "#" + sCannonTypeText + " " + sCannonCaliberText);

			StringCollection_SetText("STRINGS_CANNON_UPGRADE_MONEY", 1, "#" + iNumShipCannons * Net_GetCannonUpgradeCost(iCurrentCannonType, 0));
			StringCollection_SetText("STRINGS_CANNON_UPGRADE_MONEY", 2, "#" + iNumShipCannons * Net_GetCannonUpgradeCost(iCurrentCannonType, 1));
			StringCollection_SetText("STRINGS_CANNON_UPGRADE_MONEY", 3, "#" + iNumShipCannons * Net_GetCannonUpgradeCost(iCurrentCannonType, 2));
			
			// set new cannon upgrade = Copper
			Buy_SetCannonUpgrade(0, true);
			Picture_SetGroupPicture("PICTURE_CANNON_UPGRADE_COPPER", "CANNONS_UPGRADE_1", rCannon.picture);
			Picture_SetGroupPicture("PICTURE_CANNON_UPGRADE_BRONZE", "CANNONS_UPGRADE_2", rCannon.picture);
			Picture_SetGroupPicture("PICTURE_CANNON_UPGRADE_IRON", "CANNONS_UPGRADE_3", rCannon.picture);

			Buy_UpdateCredit();
			break;
		}
	}
}
void Buy_CannonLeft()	{ Buy_SetCannon(-1); }
void Buy_CannonRight()	{ Buy_SetCannon(1); }

void Buy_SetPerk(int iPerkIndex, bool bState)
{
	if (iPerks[iPerkIndex] == bState) return;

	int iPerkCost = Net_GetPerkCost(iPerkIndex);

	if (iPerks[iPerkIndex])
	{
		// sell old perk
		iCurrentCredit = iCurrentCredit - iPerkCost;
	}

	if (bState)
	{
		iCurrentCredit = iCurrentCredit + iPerkCost;
	}

	iPerks[iPerkIndex] = bState;
	Buy_UpdateCredit();
}

// ================================================================================================
// COOP s28: the sea perks are bought one by one (common\Perks.c). The table TABLE_SEA_PERKS of the buy screen: a double click
// or the button under it buys / sells the selected perk. The perks that the switches of the server give everybody are free
// and can not be sold.
// ================================================================================================
bool Buy_SeaPerkForced(int iPerk)
{
	if (bNetBuyProfile) { return false; }
	string sSw = "ActivePerks";
	if (iPerk < NET_SPERK_PASSIVE_NUM) { sSw = "PassivePerks"; }
	if (!CheckAttribute(&NCServer, sSw)) { return false; }
	return sti(NCServer.(sSw)) != 0;
}

string Buy_SeaPerkTitle(int iPerk)
{
	switch (iPerk)
	{
		case 0: return "Мастер паруса"; break;
		case 1: return "Быстр. зарядка"; break;
		case 2: return "Дальний выстрел"; break;
		case 3: return "Канонир-профи"; break;
		case 4: return "Точный выстрел"; break;
		case 5: return "Урон команде"; break;
		case 6: return "Урон парусам"; break;
		case 7: return "Защита корабля"; break;
		case 8: return "Врач"; break;
		case 9: return "Морской волк"; break;
		case 10: return "Плотник"; break;
		case 11: return "Строитель"; break;
		case 12: return "А: Аврал пушек"; break;
		case 13: return "А: Разворот"; break;
		case 14: return "А: Лёгк. ремонт"; break;
		case 15: return "А: Быстр. ремонт"; break;
		case 16: return "А: Брандер"; break;
	}
	return "";
}

int Buy_SeaPerksMask()
{
	int iMask = 0;
	for (int i=0; i<NET_SPERK_NUM; i++)
	{
		if (iBuySeaPerk[i] != 0) { iMask = iMask + Net_SeaPerkPow2(i); }
	}
	return iMask;
}

void Buy_SeaPerksReset()
{
	for (int i=0; i<NET_SPERK_NUM; i++) { iBuySeaPerk[i] = 0; }
	GameInterface.TABLE_SEA_PERKS.select = 1;
	Buy_SeaPerksFill();
}

// buy (bState true) / sell one perk; false = not enough money (or a free perk of the server)
bool Buy_SetSeaPerk(int iPerk, bool bState)
{
	if (iPerk < 0 || iPerk >= NET_SPERK_NUM) { return false; }
	if (Buy_SeaPerkForced(iPerk)) { return false; }
	int iHave = iBuySeaPerk[iPerk];
	int iWant = 0;
	if (bState) { iWant = 1; }
	if (iHave == iWant) { return true; }
	int iCost = Net_GetSeaPerkCost(iPerk);
	if (iWant == 1)
	{
		if (iCurrentCredit + iCost > iMaxCredit) { return false; }
		iCurrentCredit = iCurrentCredit + iCost;
	}
	else
	{
		iCurrentCredit = iCurrentCredit - iCost;
	}
	iBuySeaPerk[iPerk] = iWant;
	Buy_UpdateCredit();
	return true;
}

void Buy_SeaPerksFill()
{
	int iSelected = 1;
	if (CheckAttribute(&GameInterface, "TABLE_SEA_PERKS.select")) { iSelected = sti(GameInterface.TABLE_SEA_PERKS.select); }
	Table_Clear("TABLE_SEA_PERKS", false, true, false);
	for (int i=0; i<NET_SPERK_NUM; i++)
	{
		string sRow = "tr" + (i + 1);
		bool bForced = Buy_SeaPerkForced(i);
		bool bOn = bForced;
		if (iBuySeaPerk[i] != 0) { bOn = true; }
		string sGrp = "PERK_DISABLE";
		int iCol = argb(255,170,170,170);
		if (bOn) { sGrp = "PERK_ENABLE"; iCol = argb(255,255,255,255); }
		GameInterface.TABLE_SEA_PERKS.(sRow).td1.icon.group = sGrp;
		GameInterface.TABLE_SEA_PERKS.(sRow).td1.icon.image = sNetSeaPerkName[i];
		GameInterface.TABLE_SEA_PERKS.(sRow).td1.icon.width = 20;
		GameInterface.TABLE_SEA_PERKS.(sRow).td1.icon.height = 20;
		GameInterface.TABLE_SEA_PERKS.(sRow).td1.icon.offset = "1, 0";
		GameInterface.TABLE_SEA_PERKS.(sRow).td2.str = Buy_SeaPerkTitle(i);
		GameInterface.TABLE_SEA_PERKS.(sRow).td2.scale = 0.7;
		GameInterface.TABLE_SEA_PERKS.(sRow).td2.color = iCol;
		string sPrice = "" + Net_GetSeaPerkCost(i);
		if (bForced) { sPrice = "всем"; }
		GameInterface.TABLE_SEA_PERKS.(sRow).td3.str = sPrice;
		GameInterface.TABLE_SEA_PERKS.(sRow).td3.scale = 0.65;
		GameInterface.TABLE_SEA_PERKS.(sRow).td3.color = iCol;
	}
	if (iSelected < 1 || iSelected > NET_SPERK_NUM) { iSelected = 1; }
	GameInterface.TABLE_SEA_PERKS.select = iSelected;
	Table_UpdateWindow("TABLE_SEA_PERKS");
}

// the double click on a row / the button "Купить / продать"
void Buy_OnSeaPerk()
{
	int iSel = 0;
	if (CheckAttribute(&GameInterface, "TABLE_SEA_PERKS.select")) { iSel = sti(GameInterface.TABLE_SEA_PERKS.select); }
	int iPerk = iSel - 1;
	if (iPerk < 0 || iPerk >= NET_SPERK_NUM) { return; }
	if (Buy_SeaPerkForced(iPerk)) { Log_SetStringToLog("Этот перк сервер даёт всем бесплатно"); return; }
	bool bWant = (iBuySeaPerk[iPerk] == 0);
	if (!Buy_SetSeaPerk(iPerk, bWant)) { Log_SetStringToLog("Не хватает денег на перк (" + Net_GetSeaPerkCost(iPerk) + ")"); }
	Buy_SeaPerksFill();
}

void Buy_SortPlayers()
{
	int i, j;
	int iThisClientID = NetClient_GetThisClientID();
	int iMainTeam;

	iNumClients = 0;

	// create list
	for (i=0; i<NET_MAXCLIENTS; i++) 
	{
		if (!sti(NCClients[i].Use)) { continue; }
		if (iThisClientID == sti(NCClients[i].ID))
		{
			iMainTeam = sti(NCClients[i].Team);
		}
		BuyClients[iNumClients] = sti(NCClients[i].ID);
		iNumClients++;
	}

	// sort BuyClients
	for (i=0; i<iNumClients; i++) 
	{
		for (j=i + 1; j<iNumClients; j++) 
		{
			bool bSwap = false;

			ref rClient1 = NetClient_GetClient(BuyClients[i]);
			ref rClient2 = NetClient_GetClient(BuyClients[j]);

			if (sti(rClient2.ID) == iThisClientID) { bSwap = true; }

			int iTeam1 = sti(rClient1.Team);
			int iTeam2 = sti(rClient2.Team);

			if (iTeam1 != iMainTeam)
			{
				if (iTeam2 == iMainTeam || iTeam2 > iTeam1) { bSwap = true; }
			}

			if (bSwap)
			{
				int iTmp = BuyClients[i];
				BuyClients[i] = BuyClients[j];
				BuyClients[j] = iTmp;
			}
		}
	}
}

void Buy_FillPlayersTable(string sTable)
{
	Table_Clear(sTable, false, true, false);

	if (!bServerInfoReady) { return; }

	// sort players: our player first and his team first too
	Buy_SortPlayers();

	for (int i=0; i<iNumClients; i++)
	{
		ref rClient = NetClient_GetClient(BuyClients[i]);

		string sRow = "tr" + (i + 1);

		bool bHimself = (rClient.ID == wClientID);

		string sShipGroup = "SHIPS_empty";

		int iShipType = sti(rClient.Ship.Type);
		if (iShipType >= 0)
		{
			ref rShip = Net_GetShipByIndex(iShipType);
			sShipGroup = "SHIPS_" + rShip.Name;
		}

		int dwColor = 0; if (bHimself) { dwColor = ARGB(120, 248, 255, 214); }
		GameInterface.(sTable).(sRow).speccolor = dwColor;

		GameInterface.(sTable).(sRow).ClientID = rClient.ID;

		// face
		GameInterface.(sTable).(sRow).td1.icon.texturepointer = sti(rClient.UserData.Face.Texture);
		GameInterface.(sTable).(sRow).td1.icon.cellsize = true;

		// ship
		GameInterface.(sTable).(sRow).td2.icon.group = sShipGroup;
		GameInterface.(sTable).(sRow).td2.icon.image = "ship";
		GameInterface.(sTable).(sRow).td2.icon.cellsize = true;

		GameInterface.(sTable).(sRow).td3.str = rClient.NickName + "\n" + rClient.Ship.Name;

		if (sti(NCServer.GameType) == NETGAME_DEATHMATCH && !NetLand_TeamGame())		// COOP: a team land duel shows the teams
		{
			GameInterface.(sTable).(sRow).td5.icon.group = "NET_TEAMICONS";
			GameInterface.(sTable).(sRow).td5.icon.image = "no_team";
		}
		else
		{
			GameInterface.(sTable).(sRow).td5.icon.group = "NET_TEAMICONS";
			GameInterface.(sTable).(sRow).td5.icon.image = "team" + rClient.Team;
		}

		GameInterface.(sTable).(sRow).td5.icon.cellsize = true;

		GameInterface.(sTable).(sRow).td6.icon.group = "NET_BUYCHECKICONS";
		string sReady = "check_empty";
		if (bHimself) 
		{ 
			sReady = "check_no"; 
			if (sti(rClient.BuyReady)) { sReady = "check_yes"; }
		}
		else 
		{ 
			if (sti(rClient.BuyReady)) { sReady = "check_yes_no_choice"; } 
		}
		GameInterface.(sTable).(sRow).td6.icon.image = sReady;
		GameInterface.(sTable).(sRow).td6.icon.cellsize = true;

		GameInterface.(sTable).(sRow).td7.icon.group = "NET_BUYCHECKICONS";
		string sKickIcon = "check_no";
		if (sti(rClient.Kick)) { sKickIcon = "check_yes"; }
		if (bHimself) { sKickIcon = "check_empty"; }
		GameInterface.(sTable).(sRow).td7.icon.image = sKickIcon;
		GameInterface.(sTable).(sRow).td7.icon.cellsize = true;
	}

	Buy_LBotsRows(sTable, iNumClients);		// COOP s30: the bots of the lobby

	Table_UpdateWindow(sTable);
}

void Buy_TryChangeTeam()
{
	int iGameType = sti(NCServer.GameType);
	if (iGameType == NETGAME_DEATHMATCH) { return; }


}

void Buy_OnTableClick()
{
	if (bNetBuyProfile || bNetBuyRespawn) { return; }		// COOP: the ship editor of the profile: no server
	string sControl = GetEventData();
	int iRow = GetEventData();
	int iColumn = GetEventData();

	if (sControl != "TABLE_PLAYERS") { return; } 
	//if (iRow == 0) { return; } 

	string sRow = "tr" + (iRow + 1);
	if (!CheckAttribute(&GameInterface, "TABLE_PLAYERS." + sRow + ".ClientID")) { return; }
	int wNetClientID = sti(GameInterface.TABLE_PLAYERS.(sRow).ClientID);
	// COOP s30: a bot of the lobby: the admin removes it with the kick column
	if (wNetClientID >= BUY_LBOT_ROWID)
	{
		if (iColumn == 7 && bNetClientIsAdmin) { NetClient_AdminSend(NET_ADMIN_LBOT_DEL, DST_INVALID, makefloat(wNetClientID - BUY_LBOT_ROWID)); return; }
		if (iColumn <= 4) { Buy_PeekBot(wNetClientID - BUY_LBOT_ROWID); }		// COOP s30: its ship (read only)
		return;
	}
	bool bClickHimself = (wNetClientID == wClientID);
	// COOP s30: the face, the ship or the name of another player: his choice (read only)
	if (iColumn <= 4 && !bClickHimself) { Buy_PeekPlayer(wNetClientID); return; }

	ref rClient = NetClient_GetClient(wNetClientID);

	int iSMsg;
	switch (iColumn)
	{
		case 5:		// team click
			if (!bClickHimself) { return; }
			// try to change team and resort table
			if (sti(NCServer.NumTeams) > 1) NetBuy_OpenSelectTeam();
		break;
		case 6:		// ready click
			if (!bClickHimself) { return; }
			Buy_Server_UpdateReady();
		break;
		case 7:		// kick click
			if (bClickHimself) { return; }

			// COOP: an admin kicks the player out of the lobby at once (the server drops him and every table is updated)
			if (bNetClientIsAdmin)
			{
				NetClient_AdminSend(NET_ADMIN_KICK, wNetClientID, 0.0);
				return;
			}

			rClient.Kick = !sti(rClient.Kick);

			iSMsg = NMCreate();
			NMAddClientID(iSMsg, wClientID);
			NMAddByte(iSMsg, NC_COMMAND);
			NMAddByte(iSMsg, NSC_COMMAND_VOTE_KICK);
			NMAddClientID(iSMsg, wNetClientID);
			NetClient_SendMessage(iSMsg, true);
			NMDelete(iSMsg);
		break;
	}

	Buy_FillPlayersTable("TABLE_PLAYERS");
}

void Buy_CheckButtonChange()
{
	string sControl = GetEventData();
	int iNumber = GetEventData();
	int iState = GetEventData();

	switch (sControl)
	{
		case "RADIO_CANNONS_UPGRADELIST":
			Buy_SetCannonUpgrade(iNumber - 1, false);
		break;
		case "CHECKBUTTON_LOBBY_BOTRESPAWN":		// COOP s38
			if (bBuyBotRespSync) { break; }
			if (!bNetClientIsAdmin) { Buy_BotRespShow(); break; }
			NetClient_AdminSend(NET_ADMIN_BOT_RESPAWN, DST_INVALID, makefloat(iState));
		break;
		case "CHECKBUTTONS_PERKS":
			Buy_SetPerk(iNumber - 1, iState);
		break;
	}
	NetLand_UiLoCheck(sControl, iState);		// COOP: the poison switch of the loadout
}

void IDoExit(int exitCode)
{
	DelEventHandler("OnGameSettings", "Buy_OnGameSettings");
	DelEventHandler("OnLobbyBots", "Buy_LBotOpen");
	DelEventHandler("OnPeekClose", "Buy_PeekClose");
	DelEventHandler("NCEvent_LobbyBots", "Buy_OnLobbyBots");
	DelEventHandler("OnBotShipPrev", "Buy_LBotShipPrev");
	DelEventHandler("OnBotShipNext", "Buy_LBotShipNext");
	DelEventHandler("OnBotCannonPrev", "Buy_LBotCannonPrev");
	DelEventHandler("OnBotCannonNext", "Buy_LBotCannonNext");
	DelEventHandler("OnBotCrewDec", "Buy_LBotCrewDec");
	DelEventHandler("OnBotCrewInc", "Buy_LBotCrewInc");
	DelEventHandler("OnBotHullDec", "Buy_LBotHullDec");
	DelEventHandler("OnBotHullInc", "Buy_LBotHullInc");
	DelEventHandler("OnBotSailsDec", "Buy_LBotSailsDec");
	DelEventHandler("OnBotSailsInc", "Buy_LBotSailsInc");
	DelEventHandler("OnBotGunPrev", "Buy_LBotGunPrev");
	DelEventHandler("OnBotGunNext", "Buy_LBotGunNext");
	DelEventHandler("OnBotRelPrev", "Buy_LBotRelPrev");
	DelEventHandler("OnBotRelNext", "Buy_LBotRelNext");
	DelEventHandler("OnBotDoAdd", "Buy_LBotDoAdd");
	DelEventHandler("OnBotBack", "Buy_LBotClose");
	DelEventHandler("NCEvent_ServerChanged", "Buy_OnServerChanged");
	DelEventHandler("CheckButtonChange", "Buy_CheckButtonChange");
	DelEventHandler("OnLobbyLoadout", "Buy_LoOpen");
	DelEventHandler("OnSeaPerk", "Buy_OnSeaPerk");
	DelEventHandler("OnProfOk", "Buy_ProfOk");
	DelEventHandler("OnProfBack", "Buy_ProfLeave");
	DelEventHandler("OnProfPreview", "Buy_ProfPreview");
	DelEventHandler("OnProfRotL", "Buy_ProfRotL");
	DelEventHandler("OnProfRotR", "Buy_ProfRotR");
	DelEventHandler("OnProfZoomIn", "Buy_ProfZoomIn");
	DelEventHandler("OnProfZoomOut", "Buy_ProfZoomOut");
	DelEventHandler("OnProfFlags", "Buy_ProfFlagsOpen");
	DelEventHandler("OnPflClose", "Buy_ProfFlagsClose");
	if (bBuyProfLists)
	{
		bBuyProfLists = false;
		DelEventHandler("GetInterfaceTexture", "Buy_ScrollGetTexture");
		NFDelGroupTextures("SAILSLIST");
		NFDelGroupTextures("FLAGSLIST");
	}
	DelEventHandler("Buy_PreviewGo", "Buy_PreviewGo");
	DelEventHandler("OnLoApply", "Buy_LoApply");
	DelEventHandler("OnLoBack", "Buy_LoBack");
	NetLand_UiLoRegister(false);
	bNetLandUiLo = false;

	DelEventHandler("OnTmpStart", "OnTmpStart");
	DelEventHandler("OnReset", "OnReset");
	DelEventHandler("OnDisconnect", "OnDisconnect");

	DelEventHandler("ShipLeft", "Buy_ShipLeft");		DelEventHandler("ShipRight", "Buy_ShipRight");

	DelEventHandler("AddUpgradeHull", "Buy_AddUpgradeHull");		DelEventHandler("DecUpgradeHull", "Buy_DecUpgradeHull");
	DelEventHandler("AddUpgradeRigging", "Buy_AddUpgradeRigging");	DelEventHandler("DecUpgradeRigging", "Buy_DecUpgradeRigging");

	DelEventHandler("CannonLeft", "Buy_CannonLeft");	DelEventHandler("CannonRight", "Buy_CannonRight");

	DelEventHandler("AddAccuracy", "Buy_AddAccuracy");	DelEventHandler("DecAccuracy", "Buy_DecAccuracy");
	DelEventHandler("NetInfoShow", "Buy_InfoShow");
	DelEventHandler("MouseRClickUp", "NetInfo_Hide");
	DelEventHandler("AddCannons", "Buy_AddCannons");	DelEventHandler("DecCannons", "Buy_DecCannons");
	DelEventHandler("AddSailing", "Buy_AddSailing");	DelEventHandler("DecSailing", "Buy_DecSailing");
	DelEventHandler("AddDefence", "Buy_AddDefence");	DelEventHandler("DecDefence", "Buy_DecDefence");
	DelEventHandler("AddRepair", "Buy_AddRepair");		DelEventHandler("DecRepair", "Buy_DecRepair");
	DelEventHandler("AddGrappling", "Buy_AddGrappling");	DelEventHandler("DecGrappling", "Buy_DecGrappling");

	DelEventHandler("AddBalls", "Buy_AddBalls");			DelEventHandler("DecBalls", "Buy_DecBalls");
	DelEventHandler("AddGrapes", "Buy_AddGrapes");			DelEventHandler("DecGrapes", "Buy_DecGrapes");
	DelEventHandler("AddKnippels", "Buy_AddKnippels");		DelEventHandler("DecKnippels", "Buy_DecKnippels");
	DelEventHandler("AddBombs", "Buy_AddBombs");			DelEventHandler("DecBombs", "Buy_DecBombs");
	DelEventHandler("AddPlanks", "Buy_AddPlanks");			DelEventHandler("DecPlanks", "Buy_DecPlanks");
	DelEventHandler("AddSailcloth", "Buy_AddSailcloth");	DelEventHandler("DecSailcloth", "Buy_DecSailcloth");
	DelEventHandler("AddPowder", "Buy_AddPowder");	DelEventHandler("DecPowder", "Buy_DecPowder");
	DelEventHandler("AddCrew", "Buy_AddCrew");	DelEventHandler("DecCrew", "Buy_DecCrew");
	DelEventHandler("AddCannonsPercent", "Buy_AddCannonsPercent");	DelEventHandler("DecCannonsPercent", "Buy_DecCannonsPercent");

	DelEventHandler("OnTableClick", "Buy_OnTableClick");
	DelEventHandler("NCEvent_PlayerInfo_ShipType", "Buy_PlayerInfo_ShipType");
	DelEventHandler("NCEvent_PlayerInfo_BuyReady", "Buy_PlayerInfo_BuyReady");
	DelEventHandler("NCEvent_PlayerInfo_Team", "Buy_PlayerInfo_Team");
	DelEventHandler("NCEvent_Client_Connect", "Buy_NewClientConnect");

	DelEventHandler("NCEvent_StartGame", "Buy_StartGame");
	DelEventHandler("NCEvent_StartGame_GetBuyInfo", "Buy_GetBuyInfo");
	DelEventHandler("NCEvent_PlayerInfo_StartCountdown", "Buy_StartCountdown");
	DelEventHandler("NCEvent_PlayerInfo_StopCountdown",  "Buy_StopCountdown");
	DelEventHandler("NCEvent_Disconnect", "Buy_Disconnect");

	DelEventHandler("NCEvent_ClientDrop", "Buy_ClientDrop");

	DelEventHandler("NCEvent_LobbyChat", "Buy_OnLobbyChat");
	DelEventHandler("OnChatOpen", "Buy_ChatOpen");
	DelEventHandler("OnChatClose", "Buy_ChatClose");
	DelEventHandler("OnChatSend", "Buy_ChatSend");
	DelEventHandler("EditExit", "Buy_ChatEditExit");
	DelEventHandler("OnAdminOpen", "Buy_AdminOpen");
	DelEventHandler("OnAdminClose", "Buy_AdminClose");
	DelEventHandler("OnAdminSend", "Buy_AdminSend");
	DelEventHandler("NCEvent_AdminStatus", "Buy_OnAdminStatus");
	DelEventHandler("OnDetails", "Buy_DetailsOpen");
	DelEventHandler("OnDetailsClose", "Buy_DetailsClose");
	DelEventHandler("NCEvent_SrvDetails", "Buy_OnSrvDetails");
	bNetLobbyChatActive = false;

	DelEventHandler("frame", "NetBuy_OnFrame");

	DelEventHandler("OnClickTeam", "NetBuy_OnClickTeam");

	interfaceResultCommand = exitCode;
	EndCancelInterface(true);
}

// ========================================
// COOP s30: the bots of the lobby (server\Bots.c: NetServer_LBot*). The admin adds them with the "Бот+" window (a copy of the
// window of the game, the sea or the land one); they are rows of the players table and come into the game with the players
// ========================================
#define BUY_LBOT_ROWID		1000		// ClientID of the rows of the bots: BUY_LBOT_ROWID + the number of the bot

bool bBuyLBotOpen = false;
int iBuyLBotsCapShown = -2;
int iLBShip = -1;
int iLBCannon = -1;
int iLBCrew = 100;
int iLBHull = 100;
int iLBSails = 100;
int iLBRel = 0;
int iLBFace = -1;
string sLBBlade = "";
string sLBGun = "";
int iLBLevel = 10;

bool Buy_LBotLand()
{
	if (!CheckAttribute(&NCServer, "IslandID")) { return false; }
	return NetLand_IsAdvert(NCServer.IslandID);
}

// the teams a bot can join (0 = no team game)
int Buy_LBotTeams()
{
	if (Buy_LBotLand())
	{
		if (NetLand_TeamGame()) { return 2; }
		return 0;
	}
	if (!CheckAttribute(&NCServer, "GameType")) { return 0; }
	if (sti(NCServer.GameType) == NETGAME_DEATHMATCH) { return 0; }
	int iN = 2;
	if (CheckAttribute(&NCServer, "NumTeams")) { iN = sti(NCServer.NumTeams); }
	if (iN < 2) { iN = 2; }
	if (iN > 4) { iN = 4; }
	return iN;
}

bool Buy_LBotRelOk(int iRel)
{
	if (iRel >= NET_BOTREL_ALLY && iRel <= NET_BOTREL_NEUTRAL) { return true; }
	if (iRel < NET_BOTREL_TEAM0) { return false; }
	return (iRel - NET_BOTREL_TEAM0) < Buy_LBotTeams();
}

string Buy_LBotRelName(int iRel)
{
	if (iRel >= NET_BOTREL_TEAM0 && iRel < NET_BOTREL_TEAM0 + 4) { return "Команда: " + XI_ConvertString("nteam_" + NetTeamName[iRel - NET_BOTREL_TEAM0]); }
	if (iRel == NET_BOTREL_ALLY) { return XI_ConvertString("capNetAdmin_RelAlly"); }
	if (iRel == NET_BOTREL_NEUTRAL) { return XI_ConvertString("capNetAdmin_RelNeutral"); }
	return XI_ConvertString("capNetAdmin_RelEnemy");
}

// "Боты: 1-2-3" (the bots of every team, red first) or "Боты: N" (no teams); the bots out of the teams as "+N"
void Buy_LBotsCaption()
{
	int n = NetClient_LobbyBotsNum();
	int iT = Buy_LBotTeams();
	int iStamp = n * 16 + iT;
	if (iStamp == iBuyLBotsCapShown) { return; }
	iBuyLBotsCapShown = iStamp;
	string sCap = "Боты: " + n;
	if (iT > 0)
	{
		int iCnt[4];
		int iOther = 0;
		int k;
		for (k=0; k<4; k++) { iCnt[k] = 0; }
		for (k=0; k<n; k++)
		{
			string sB = "b" + k;
			int iR = sti(NCLobbyBots.(sB).rel) - NET_BOTREL_TEAM0;
			if (iR >= 0 && iR < iT) { iCnt[iR] = iCnt[iR] + 1; }
			else { iOther++; }
		}
		sCap = "Боты: " + iCnt[0];
		for (k=1; k<iT; k++) { sCap = sCap + "-" + iCnt[k]; }
		if (iOther > 0) { sCap = sCap + " +" + iOther; }
	}
	Button_SetText("BTN_LOBBY_BOTS", "#" + sCap);
}

void Buy_OnLobbyBots()
{
	iBuyLBotsCapShown = -2;
	Buy_LBotsCaption();
	Buy_FillPlayersTable("TABLE_PLAYERS");
}

// the rows of the bots after the players
void Buy_LBotsRows(string sTable, int iFirst)
{
	if (sTable != "TABLE_PLAYERS") { return; }
	int n = NetClient_LobbyBotsNum();
	bool bLand = Buy_LBotLand();
	bool bTeams = (Buy_LBotTeams() > 0);
	for (int k=0; k<n; k++)
	{
		string sB = "b" + k;
		string sRow = "tr" + (iFirst + k + 1);
		int iRel = sti(NCLobbyBots.(sB).rel);
		int iShip = sti(NCLobbyBots.(sB).ship);
		GameInterface.(sTable).(sRow).speccolor = ARGB(60, 160, 160, 255);
		GameInterface.(sTable).(sRow).ClientID = BUY_LBOT_ROWID + k;
		string sInfo = "";
		if (bLand)
		{
			sInfo = NetLand_FaceToModel(iShip - (iShip / 512) * 512);
		}
		else
		{
			if (iShip >= 0 && iShip < Net_GetShipsNum())
			{
				ref rShip = Net_GetShipByIndex(iShip);
				string sShipName = rShip.Name;
				if (sShipName != "FORT")
				{
					GameInterface.(sTable).(sRow).td2.icon.group = "SHIPS_" + sShipName;
					GameInterface.(sTable).(sRow).td2.icon.image = "ship";
					GameInterface.(sTable).(sRow).td2.icon.cellsize = true;
				}
				sInfo = XI_ConvertString(sShipName);
			}
		}
		GameInterface.(sTable).(sRow).td3.str = "Бот " + (k + 1) + "\n" + sInfo;
		GameInterface.(sTable).(sRow).td5.icon.group = "NET_TEAMICONS";
		if (bTeams && iRel >= NET_BOTREL_TEAM0) { GameInterface.(sTable).(sRow).td5.icon.image = "team" + (iRel - NET_BOTREL_TEAM0); }
		else { GameInterface.(sTable).(sRow).td5.icon.image = "no_team"; }
		GameInterface.(sTable).(sRow).td5.icon.cellsize = true;
		GameInterface.(sTable).(sRow).td6.icon.group = "NET_BUYCHECKICONS";
		GameInterface.(sTable).(sRow).td6.icon.image = "check_yes_no_choice";
		GameInterface.(sTable).(sRow).td6.icon.cellsize = true;
		GameInterface.(sTable).(sRow).td7.icon.group = "NET_BUYCHECKICONS";
		if (bNetClientIsAdmin) { GameInterface.(sTable).(sRow).td7.icon.image = "check_no"; }
		else { GameInterface.(sTable).(sRow).td7.icon.image = "check_empty"; }
		GameInterface.(sTable).(sRow).td7.icon.cellsize = true;
	}
}

void Buy_LBotOpen()
{
	if (!bNetClientIsAdmin) { Log_SetStringToLog("Ботов добавляет админ (кнопка Админ)"); return; }
	if (!bServerInfoReady) { return; }
	if (sti(NCServer.GameRunning)) { return; }
	Buy_ChatClose();
	Buy_DetailsClose();
	if (iLBShip < 0) { iLBShip = Buy_LBotNextShip(-1, 1); }
	if (iLBCannon < 0) { iLBCannon = Buy_LBotNextCannon(-1, 1); }
	if (!Buy_LBotRelOk(iLBRel))
	{
		iLBRel = NET_BOTREL_ENEMY;
		if (Buy_LBotTeams() > 0) { iLBRel = NET_BOTREL_TEAM0; }
	}
	bBuyLBotOpen = true;
	XI_WindowDisable("WND_NETBUY", true);
	XI_WindowShow("WND_BOTADD", true);
	XI_WindowDisable("WND_BOTADD", false);
	Buy_LBotUpdate();
	SetCurrentNode("BTN_BOT_DOADD");
}

void Buy_LBotClose()
{
	if (!bBuyLBotOpen) { return; }
	bBuyLBotOpen = false;
	XI_WindowShow("WND_BOTADD", false);
	XI_WindowDisable("WND_BOTADD", true);
	XI_WindowDisable("WND_NETBUY", false);
}

void Buy_LBotNodes(bool bLand)
{
	SetNodeUsing("BTN_BOT_GUN_PREV", bLand);
	SetNodeUsing("PICTURE_BOT_GUN", bLand);
	SetNodeUsing("TEXT_BOT_GUN", bLand);
	SetNodeUsing("BTN_BOT_GUN_NEXT", bLand);
	SetNodeUsing("TEXT_BOT_HULL", !bLand);
	SetNodeUsing("BTN_BOT_HULL_DEC", !bLand);
	SetNodeUsing("BTN_BOT_HULL_INC", !bLand);
	SetNodeUsing("TEXT_BOT_SAILS", !bLand);
	SetNodeUsing("BTN_BOT_SAILS_DEC", !bLand);
	SetNodeUsing("BTN_BOT_SAILS_INC", !bLand);
}

void Buy_LBotUpdate()
{
	if (!bBuyLBotOpen) { return; }
	if (Buy_LBotLand()) { Buy_LBotUpdateLand(); return; }
	Buy_LBotNodes(false);
	ref rShip = Net_GetShipByIndex(iLBShip);
	string sShipName = rShip.Name;
	if (sShipName == "FORT")
	{
		Picture_SetGroupPicture("PICTURE_BOT_SHIP", "ICONS_SPEC", "ICON_Fort");
		SetFormatedText("TEXT_BOT_SHIP", XI_ConvertString("Fort"));
		SetFormatedText("TEXT_BOT_SHIPINFO", XI_ConvertString("capNetAdmin_BotFortInfo"));
	}
	else
	{
		Picture_SetGroupPicture("PICTURE_BOT_SHIP", "SHIPS_" + sShipName, "ship");
		SetFormatedText("TEXT_BOT_SHIP", XI_ConvertString(sShipName) + ", " + XI_ConvertString("capNetAdmin_BotClass") + " " + rShip.Class);
		int iCannons = Net_GetShipCannonsNum(iLBShip);
		int iCrewNum = Net_GetCrewCount(iLBShip, iLBCrew);
		SetFormatedText("TEXT_BOT_SHIPINFO", XI_ConvertString("capNetAdmin_BotCannons") + ": " + iCannons + ", " + XI_ConvertString("capNetAdmin_BotCrewNum") + ": " + iCrewNum);
	}
	ref rCannon = Net_GetCannonByIndex(iLBCannon);
	Picture_SetGroupPicture("PICTURE_BOT_CANNON", "CANNONS_UPGRADE_1", rCannon.picture);
	SetFormatedText("TEXT_BOT_CANNON", XI_ConvertString(rCannon.Name) + " " + XI_ConvertString("caliber" + rCannon.caliber));
	SetFormatedText("TEXT_BOT_CREW", XI_ConvertString("capNetAdmin_BotCrew") + ": " + iLBCrew + "%");
	SetFormatedText("TEXT_BOT_HULL", XI_ConvertString("capNetAdmin_BotHull") + ": " + iLBHull + "%");
	SetFormatedText("TEXT_BOT_SAILS", XI_ConvertString("capNetAdmin_BotSails") + ": " + iLBSails + "%");
	SetFormatedText("TEXT_BOT_REL", XI_ConvertString("capNetAdmin_BotRel") + ": " + Buy_LBotRelName(iLBRel));
}

void Buy_LBotUpdateLand()
{
	if (iLBFace < 0) { iLBFace = NetLand_BotNextFace(-1, 1); }
	if (sLBBlade == "") { sLBBlade = NetLand_NextItem("", BLADE_ITEM_TYPE, 1, false); }
	SetNewPicture("PICTURE_BOT_SHIP", "interfaces\portraits\256\face_" + iLBFace + ".tga");
	SetFormatedText("TEXT_BOT_SHIP", "Скин: " + NetLand_FaceToModel(iLBFace));
	SetFormatedText("TEXT_BOT_SHIPINFO", "Стрелками выберите лицо и клинок бота");
	string sBladeName = sLBBlade;
	int iBl = GetItemIndex(sLBBlade);
	if (iBl >= 0)
	{
		SetNewGroupPicture("PICTURE_BOT_CANNON", Items[iBl].picTexture, "itm" + Items[iBl].picIndex);
		sBladeName = NetLand_ItemTitle(sLBBlade);
	}
	SetFormatedText("TEXT_BOT_CANNON", "Клинок: " + sBladeName);
	SetFormatedText("TEXT_BOT_CREW", "Уровень: " + iLBLevel);
	string sGunName = "нет";
	if (sLBGun != "") { sGunName = NetLand_ItemTitle(sLBGun); }
	SetFormatedText("TEXT_BOT_GUN", "Пистолет: " + sGunName);
	Buy_LBotNodes(true);
	int iGn = -1;
	if (sLBGun != "") { iGn = GetItemIndex(sLBGun); }
	if (iGn >= 0) { SetNodeUsing("PICTURE_BOT_GUN", true); SetNewGroupPicture("PICTURE_BOT_GUN", Items[iGn].picTexture, "itm" + Items[iGn].picIndex); }
	else { SetNodeUsing("PICTURE_BOT_GUN", false); }
	SetFormatedText("TEXT_BOT_REL", XI_ConvertString("capNetAdmin_BotRel") + ": " + Buy_LBotRelName(iLBRel));
}

int Buy_LBotPercent(int iValue, int iDelta)
{
	iValue = iValue + iDelta;
	if (iValue < 10) { iValue = 10; }
	if (iValue > 100) { iValue = 100; }
	return iValue;
}

int Buy_LBotNextShip(int iFrom, int iDir)
{
	int iNum = Net_GetShipsNum();
	int iCur = iFrom;
	for (int k=0; k<iNum; k++)
	{
		iCur = iCur + iDir;
		if (iCur >= iNum) { iCur = 0; }
		if (iCur < 0) { iCur = iNum - 1; }
		ref rShip = Net_GetShipByIndex(iCur);
		if (rShip.Name == "FORT") { return iCur; }
		if (CheckAttribute(rShip, "TradeOff") && sti(rShip.TradeOff)) { continue; }
		return iCur;
	}
	if (iFrom < 0) { return 0; }
	return iFrom;
}

int Buy_LBotNextCannon(int iFrom, int iDir)
{
	int iNum = Net_GetCannonsNum();
	int iCur = iFrom;
	for (int k=0; k<iNum; k++)
	{
		iCur = iCur + iDir;
		if (iCur >= iNum) { iCur = 0; }
		if (iCur < 0) { iCur = iNum - 1; }
		ref rCannon = Net_GetCannonByIndex(iCur);
		if (CheckAttribute(rCannon, "TradeOff") && sti(rCannon.TradeOff)) { continue; }
		return iCur;
	}
	if (iFrom < 0) { return 0; }
	return iFrom;
}

void Buy_LBotShipPrev()
{
	if (Buy_LBotLand()) { iLBFace = NetLand_BotNextFace(iLBFace, -1); Buy_LBotUpdate(); return; }
	iLBShip = Buy_LBotNextShip(iLBShip, -1); Buy_LBotUpdate();
}
void Buy_LBotShipNext()
{
	if (Buy_LBotLand()) { iLBFace = NetLand_BotNextFace(iLBFace, 1); Buy_LBotUpdate(); return; }
	iLBShip = Buy_LBotNextShip(iLBShip, 1); Buy_LBotUpdate();
}
void Buy_LBotCannonPrev()
{
	if (Buy_LBotLand()) { sLBBlade = NetLand_NextItem(sLBBlade, BLADE_ITEM_TYPE, -1, false); Buy_LBotUpdate(); return; }
	iLBCannon = Buy_LBotNextCannon(iLBCannon, -1); Buy_LBotUpdate();
}
void Buy_LBotCannonNext()
{
	if (Buy_LBotLand()) { sLBBlade = NetLand_NextItem(sLBBlade, BLADE_ITEM_TYPE, 1, false); Buy_LBotUpdate(); return; }
	iLBCannon = Buy_LBotNextCannon(iLBCannon, 1); Buy_LBotUpdate();
}
void Buy_LBotCrewDec()
{
	if (Buy_LBotLand()) { iLBLevel = iLBLevel - 1; if (iLBLevel < 1) { iLBLevel = 50; } Buy_LBotUpdate(); return; }
	iLBCrew = Buy_LBotPercent(iLBCrew, -10); Buy_LBotUpdate();
}
void Buy_LBotCrewInc()
{
	if (Buy_LBotLand()) { iLBLevel = iLBLevel + 1; if (iLBLevel > 50) { iLBLevel = 1; } Buy_LBotUpdate(); return; }
	iLBCrew = Buy_LBotPercent(iLBCrew, 10); Buy_LBotUpdate();
}
void Buy_LBotHullDec()
{
	if (Buy_LBotLand()) { sLBGun = NetLand_NextItem(sLBGun, GUN_ITEM_TYPE, -1, true); Buy_LBotUpdate(); return; }
	iLBHull = Buy_LBotPercent(iLBHull, -10); Buy_LBotUpdate();
}
void Buy_LBotHullInc()
{
	if (Buy_LBotLand()) { sLBGun = NetLand_NextItem(sLBGun, GUN_ITEM_TYPE, 1, true); Buy_LBotUpdate(); return; }
	iLBHull = Buy_LBotPercent(iLBHull, 10); Buy_LBotUpdate();
}
void Buy_LBotGunPrev() { Buy_LBotHullDec(); }
void Buy_LBotGunNext() { Buy_LBotHullInc(); }
void Buy_LBotSailsDec() { iLBSails = Buy_LBotPercent(iLBSails, -10); Buy_LBotUpdate(); }
void Buy_LBotSailsInc() { iLBSails = Buy_LBotPercent(iLBSails, 10); Buy_LBotUpdate(); }

void Buy_LBotRelPrev()
{
	int iLast = NET_BOTREL_NEUTRAL;
	if (Buy_LBotTeams() > 0) { iLast = NET_BOTREL_TEAM0 + Buy_LBotTeams() - 1; }
	iLBRel = iLBRel - 1;
	if (iLBRel < NET_BOTREL_ALLY) { iLBRel = iLast; }
	if (!Buy_LBotRelOk(iLBRel)) { iLBRel = NET_BOTREL_NEUTRAL; }
	Buy_LBotUpdate();
}

void Buy_LBotRelNext()
{
	iLBRel = iLBRel + 1;
	if (!Buy_LBotRelOk(iLBRel)) { iLBRel = NET_BOTREL_ALLY; }
	Buy_LBotUpdate();
}

// the bot goes into the list of the lobby (the server answers with the new list: the table and the button)
void Buy_LBotDoAdd()
{
	if (!bBuyLBotOpen) { return; }
	if (Buy_LBotLand())
	{
		int iBlIdx = GetItemIndex(sLBBlade);
		if (iBlIdx < 0) { iBlIdx = 0; }
		int iGunN = 0;
		if (sLBGun != "") { iGunN = GetItemIndex(sLBGun) + 1; }
		if (iGunN < 0) { iGunN = 0; }
		int iShipW = iLBFace + 512 * (iGunN / 256);
		NetClient_AdminBotAdd(iShipW, iLBLevel, iGunN - (iGunN / 256) * 256, iBlIdx - (iBlIdx / 256) * 256, iBlIdx / 256, iLBRel);
		return;
	}
	NetClient_AdminBotAdd(iLBShip, iLBCannon, iLBCrew, iLBHull, iLBSails, iLBRel);
}

// ========================================
// COOP s30: the choice of the other players. Every lobby sends its choice (the ship, its kit, the skills, the goods, the
// sea perks, the land loadout) as a string when it changes (server\Buy.c relays it: NCClients[i].LobbyChoice). A click on the
// row of another player (or of a bot of the lobby) shows it in the window WND_PEEK: nothing of our own choice changes
// "ship|hull|rig|cannon|cannon upgrade|cannons %|crew %|charge|sea perks|skills a,b,..|goods a,b,..|blade,gun,armor,level"
// ========================================
bool bBuyPeekOpen = false;
string sBuyChoiceSent = "";
int iBuyChoiceTimer = 0;

string Buy_ChoiceCode()
{
	int i;
	string s = "" + iCurrentShipType + "|" + iCurrentShipUpgradeHull + "|" + iCurrentShipUpgradeRigging + "|" + iCurrentCannonType;
	s = s + "|" + iCurrentCannonUpgrade + "|" + iCannonsPercent + "|" + iCrewPercent + "|" + Buy_GetStartCharge() + "|" + Buy_SeaPerksMask() + "|";
	for (i=0; i<Net_GetSkillsNum(); i++)
	{
		if (i > 0) { s = s + ","; }
		s = s + iSkillsQuantity[i];
	}
	s = s + "|";
	for (i=0; i<Net_GetGoodsNum(); i++)
	{
		if (i > 0) { s = s + ","; }
		s = s + iNetGoodsQuantity[i];
	}
	s = s + "|";
	string sBl = "";
	string sGn = "";
	string sAr = "";
	string sLv = "1";
	if (CheckAttribute(&oNetLandLo, "Blade")) { sBl = oNetLandLo.Blade; }
	if (CheckAttribute(&oNetLandLo, "Gun")) { sGn = oNetLandLo.Gun; }
	if (CheckAttribute(&oNetLandLo, "Armor")) { sAr = oNetLandLo.Armor; }
	if (CheckAttribute(&oNetLandLo, "Level")) { sLv = oNetLandLo.Level; }
	s = s + sBl + "," + sGn + "," + sAr + "," + sLv;
	return s;
}

// a changed choice goes to the server (not more often than every 0.7 s)
void Buy_ChoiceTick()
{
	if (!bServerInfoReady) { return; }
	iBuyChoiceTimer = iBuyChoiceTimer + iRealDeltaTime;
	if (iBuyChoiceTimer < 700) { return; }
	iBuyChoiceTimer = 0;
	string sCode = Buy_ChoiceCode();
	if (sCode == sBuyChoiceSent) { return; }
	sBuyChoiceSent = sCode;
	int iSMsg = NMCreate();
	NMAddClientID(iSMsg, wClientID);
	NMAddByte(iSMsg, NC_BUY);
	NMAddByte(iSMsg, NSC_BUY_CHOICE);
	NMAddString(iSMsg, sCode, 250);
	NetClient_SendMessage(iSMsg, true);
	NMDelete(iSMsg);
}

void Buy_PeekOpenWnd(string sTitle)
{
	Buy_ChatClose();
	Buy_DetailsClose();
	bBuyPeekOpen = true;
	XI_WindowDisable("WND_NETBUY", true);
	XI_WindowShow("WND_PEEK", true);
	XI_WindowDisable("WND_PEEK", false);
	SetFormatedText("TEXT_PEEK_TITLE", sTitle);
	SetCurrentNode("BTN_PEEK_CLOSE");
}

void Buy_PeekClose()
{
	if (!bBuyPeekOpen) { return; }
	bBuyPeekOpen = false;
	XI_WindowShow("WND_PEEK", false);
	XI_WindowDisable("WND_PEEK", true);
	XI_WindowDisable("WND_NETBUY", false);
}

void Buy_PeekShip(int iShip, int iCannon)
{
	SetNodeUsing("PICTURE_PEEK_SHIP", false);
	SetNodeUsing("PICTURE_PEEK_CANNON", false);
	if (iShip >= 0 && iShip < Net_GetShipsNum())
	{
		ref rShip = Net_GetShipByIndex(iShip);
		string sShipName = rShip.Name;
		if (sShipName != "FORT") { SetNodeUsing("PICTURE_PEEK_SHIP", true); Picture_SetGroupPicture("PICTURE_PEEK_SHIP", "SHIPS_" + sShipName, "ship"); }
	}
	if (iCannon >= 0 && iCannon < Net_GetCannonsNum())
	{
		ref rCannon = Net_GetCannonByIndex(iCannon);
		SetNodeUsing("PICTURE_PEEK_CANNON", true);
		Picture_SetGroupPicture("PICTURE_PEEK_CANNON", "CANNONS_UPGRADE_1", rCannon.picture);
	}
}

string Buy_PeekShipLine(int iShip)
{
	if (iShip < 0 || iShip >= Net_GetShipsNum()) { return "Корабль: не выбран"; }
	ref rShip = Net_GetShipByIndex(iShip);
	if (rShip.Name == "FORT") { return "Форт"; }
	return "Корабль: " + XI_ConvertString(rShip.Name) + ", класс " + rShip.Class;
}

string Buy_PeekCannonLine(int iCannon)
{
	if (iCannon < 0 || iCannon >= Net_GetCannonsNum()) { return "Пушки: нет"; }
	ref rCannon = Net_GetCannonByIndex(iCannon);
	return "Пушки: " + XI_ConvertString(rCannon.Name) + " " + XI_ConvertString("caliber" + rCannon.caliber);
}

// field iIdx of a list "a,b,c"
string Buy_PeekItem(string sList, int iIdx)
{
	int iPos = 0;
	int iLen = strlen(sList);
	for (int i=0; i<iIdx; i++)
	{
		int iSep = findSubStr(sList, ",", iPos);
		if (iSep < 0) { return ""; }
		iPos = iSep + 1;
	}
	if (iPos >= iLen) { return ""; }
	int iEnd = findSubStr(sList, ",", iPos);
	if (iEnd < 0) { iEnd = iLen; }
	if (iEnd <= iPos) { return ""; }
	return strcut(sList, iPos, iEnd - 1);
}

string Buy_PeekItemName(string sId)
{
	if (sId == "") { return "нет"; }
	return NetLand_ItemTitle(sId);
}

void Buy_PeekPlayer(int wID)
{
	ref rC = NetClient_GetClient(wID);
	string sNick = rC.NickName;
	if (!CheckAttribute(rC, "LobbyChoice")) { Log_SetStringToLog(sNick + ": выбор ещё не пришёл"); return; }
	string s = rC.LobbyChoice;
	Buy_PeekOpenWnd("Выбор игрока " + sNick);
	int i;
	string sLo = Net_DetField(s, 11);
	if (Buy_LBotLand())
	{
		SetNodeUsing("PICTURE_PEEK_SHIP", false);
		SetNodeUsing("PICTURE_PEEK_CANNON", false);
		Net_DetLine("TEXT_PEEK", "Снаряжение:", true);
		Net_DetLine("TEXT_PEEK", "Клинок: " + Buy_PeekItemName(Buy_PeekItem(sLo, 0)), false);
		Net_DetLine("TEXT_PEEK", "Пистолет: " + Buy_PeekItemName(Buy_PeekItem(sLo, 1)), false);
		Net_DetLine("TEXT_PEEK", "Доспех: " + Buy_PeekItemName(Buy_PeekItem(sLo, 2)), false);
		Net_DetLine("TEXT_PEEK", "Уровень: " + Buy_PeekItem(sLo, 3), false);
		return;
	}
	int iShip = sti(Net_DetField(s, 0));
	int iCannon = sti(Net_DetField(s, 3));
	Buy_PeekShip(iShip, iCannon);
	Net_DetLine("TEXT_PEEK", Buy_PeekShipLine(iShip), true);
	Net_DetLine("TEXT_PEEK", "Улучшения: корпус " + (sti(Net_DetField(s, 1)) + 1) + ", такелаж " + (sti(Net_DetField(s, 2)) + 1), false);
	Net_DetLine("TEXT_PEEK", Buy_PeekCannonLine(iCannon) + ", улучшение " + (sti(Net_DetField(s, 4)) + 1) + ", " + Net_DetField(s, 5) + "% пушек", false);
	Net_DetLine("TEXT_PEEK", "Команда: " + Net_DetField(s, 6) + "%", false);
	string sSk = Net_DetField(s, 9);
	string sLine = "Навыки:";
	for (i=0; i<Net_GetSkillsNum(); i++)
	{
		ref rSk = Net_GetSkillByIndex(i);
		sLine = sLine + " " + XI_ConvertString(rSk.Name) + " " + Buy_PeekItem(sSk, i) + ";";
	}
	Net_DetLine("TEXT_PEEK", sLine, false);
	string sGd = Net_DetField(s, 10);
	sLine = "Товары:";
	for (i=0; i<Net_GetGoodsNum(); i++)
	{
		int iQ = sti(Buy_PeekItem(sGd, i));
		if (iQ <= 0) { continue; }
		sLine = sLine + " " + XI_ConvertString(NetGoods[i].Name) + " " + iQ + ";";
	}
	Net_DetLine("TEXT_PEEK", sLine, false);
	int iMask = sti(Net_DetField(s, 8));
	sLine = "Перки:";
	bool bAny = false;
	for (i=0; i<NET_SPERK_NUM; i++)
	{
		if (!Net_SeaPerkBit(iMask, i)) { continue; }
		sLine = sLine + " " + XI_ConvertString(sNetSeaPerkName[i]) + ";";
		bAny = true;
	}
	if (!bAny) { sLine = "Перки: нет"; }
	Net_DetLine("TEXT_PEEK", sLine, false);
}

void Buy_PeekBot(int k)
{
	if (k < 0 || k >= NetClient_LobbyBotsNum()) { return; }
	string sB = "b" + k;
	int iShip = sti(NCLobbyBots.(sB).ship);
	int iRel = sti(NCLobbyBots.(sB).rel);
	Buy_PeekOpenWnd("Бот " + (k + 1));
	if (Buy_LBotLand())
	{
		SetNodeUsing("PICTURE_PEEK_SHIP", false);
		SetNodeUsing("PICTURE_PEEK_CANNON", false);
		int iBlIdx = sti(NCLobbyBots.(sB).hull) + sti(NCLobbyBots.(sB).sails) * 256;
		int iGunIdx = (iShip / 512) * 256 + sti(NCLobbyBots.(sB).crew) - 1;
		string sBl = "нет";
		string sGn = "нет";
		if (iBlIdx >= 0 && iBlIdx < ITEMS_QUANTITY) { sBl = NetLand_ItemTitle(Items[iBlIdx].id); }
		if (iGunIdx >= 0 && iGunIdx < ITEMS_QUANTITY) { sGn = NetLand_ItemTitle(Items[iGunIdx].id); }
		Net_DetLine("TEXT_PEEK", "Скин: " + NetLand_FaceToModel(iShip - (iShip / 512) * 512), true);
		Net_DetLine("TEXT_PEEK", "Клинок: " + sBl, false);
		Net_DetLine("TEXT_PEEK", "Пистолет: " + sGn, false);
		Net_DetLine("TEXT_PEEK", "Уровень: " + sti(NCLobbyBots.(sB).cannon), false);
		Net_DetLine("TEXT_PEEK", Buy_LBotRelName(iRel), false);
		return;
	}
	int iCannon = sti(NCLobbyBots.(sB).cannon);
	Buy_PeekShip(iShip, iCannon);
	Net_DetLine("TEXT_PEEK", Buy_PeekShipLine(iShip), true);
	Net_DetLine("TEXT_PEEK", Buy_PeekCannonLine(iCannon), false);
	Net_DetLine("TEXT_PEEK", "Команда: " + NCLobbyBots.(sB).crew + "%, корпус: " + NCLobbyBots.(sB).hull + "%, паруса: " + NCLobbyBots.(sB).sails + "%", false);
	Net_DetLine("TEXT_PEEK", Buy_LBotRelName(iRel), false);
}

// COOP s30: "Game settings" (top right, over the map): the host changes the game of his server in the lobby (map, mode,
// teams, money, switches) on the Create game screen; nobody is disconnected (NetServer.c: NetServer_ApplySettings)
int iBuyGsShown = -1;

bool Buy_GameSettingsOk()
{
	if (bNetBuyProfile || bNetBuyRespawn) { return false; }
	if (!NetServer_IsStarted()) { return false; }
	if (!bServerInfoReady) { return false; }
	if (sti(NCServer.GameRunning)) { return false; }
	if (bStartCountDown) { return false; }
	return true;
}

void Buy_GameSettingsShow()
{
	int iShow = 0;
	if (Buy_GameSettingsOk()) { iShow = 1; }
	if (iShow == iBuyGsShown) { return; }
	iBuyGsShown = iShow;
	SetNodeUsing("BTN_GAMESETTINGS", iShow != 0);
}

void Buy_OnGameSettings()
{
	if (!Buy_GameSettingsOk()) { return; }
	Buy_ChatClose();
	Buy_AdminClose();
	Buy_DetailsClose();
	bNetCSEdit = true;
	IDoExit(RC_INTERFACE_NET_CREATEGAME);
}

// the server has a new game: the lobby reads all of its info again (the map, the ships, the money)
void Buy_OnServerChanged()
{
	if (bNetBuyProfile || bNetBuyRespawn) { return; }
	DelEventHandler("NetClient_OnServerInfo_Event", "Buy_OnServerInfo");
	Buy_GetFullServerInfo();
}

void Buy_Disconnect()
{
	//IDoExit(RC_INTERFACE_NET_FINDGAME);
}

void Buy_StopCountdown()
{
	iClientStartTimer = 10000;
	bStartCountDown = false;
}

void Buy_StartCountdown()
{
	bStartCountDown = true;
	iClientStartTimer = GetEventData() * 1000;

	// drop team select interface
	if (bSelectNewTeamWindow) { NetBuy_SelectTeamCancel(); }
}

void Buy_GetBuyInfo()
{
	int i;

	// COOP: the game is starting, close the chat and admin windows
	Buy_ChatClose();
	Buy_AdminClose();
	Buy_DetailsClose();

	int iSMsg = NMCreate();
	NMAddClientID(iSMsg, wClientID);
	NMAddByte(iSMsg, NC_BUY);
	NMAddByte(iSMsg, NSC_BUY_FULLINFO);
	Buy_AddPayload(iSMsg);
	NetClient_SendMessage(iSMsg, true);
	NMDelete(iSMsg);

	XI_WindowDisable("WND_NETBUY", true);
	if (iNetPreview <= 0) { Buy_ProfSave(false); }		// COOP: the ship of the profile (not in a preview)

	XI_WindowShow("WND_STARTGAME_WAIT", true);
	XI_WindowDisable("WND_STARTGAME_WAIT", false);
}

// COOP s38: the switch of the bot respawn of the lobby (the admin menu of the game has the same one)

void Buy_BotRespShow()
{
	bBuyBotRespSync = true;
	int iOn = 0;
	if (bNetCliBotRespawn) { iOn = 1; }
	CheckButton_SetState("CHECKBUTTON_LOBBY_BOTRESPAWN", 1, iOn);
	bBuyBotRespSync = false;
	iBuyBotRespShown = iOn;
}

void Buy_BotRespFrame()
{
	int iOn = 0;
	if (bNetCliBotRespawn) { iOn = 1; }
	if (iOn != iBuyBotRespShown) { Buy_BotRespShow(); }
	int iAdm = 0;
	if (bNetClientIsAdmin) { iAdm = 1; }
	if (iAdm != iBuyBotRespAdm)
	{
		iBuyBotRespAdm = iAdm;
		SetSelectable("CHECKBUTTON_LOBBY_BOTRESPAWN", bNetClientIsAdmin);
	}
}

void Buy_StartGame()
{
	IDoExit(RC_INTERFACE_DO_NOTHING);
}

void OnDisconnect()
{
	if (bNetBuyRespawn) { Buy_RespLeave(); return; }		// COOP s46: Esc / "Назад": back to the Esc menu of the game
	if (bNetBuyProfile) { Buy_ProfLeave(); return; }
	NetClient_ClearPacketsQueue();

	int iSMsg = NMCreate();
	NMAddClientID(iSMsg, wClientID);
	NMAddByte(iSMsg, NC_DISCONNECT);
	NetClient_SendMessage(iSMsg, false);
	NMDelete(iSMsg);

	// COOP s43: the host leaves the lobby of a running game: the server goes first. Its ships live in the same engine: the
	// client deleted the sea, the island and the rest under them and the game crashed (c0000096) on the Find game screen.
	// The whole net stops (Net_Stop: the server, the client and the entity Net): net.dll keeps a pointer of the server that its
	// destructor does not clear, the Net entity of the Find game screen ran the deleted server (AV net.dll +A519). The Find
	// game screen starts the net again (seadogs.c: PrepareNetEnvironment -> Net_Start).
	if (NetServer_IsStarted() && bServerGameStarted)
	{
		Trace("COOP: the host disconnects from a running game: the whole net stops");
		Net_Stop();
		IDoExit(RC_INTERFACE_NET_FINDGAME);
		return;
	}
	Net_DeleteClient();
	IDoExit(RC_INTERFACE_NET_FINDGAME);
}

void OnTmpStart()
{
	// COOP: the game is running: every player joins it with this button (the server asks for the buy info)
	if (sti(NCServer.GameRunning))
	{
		if (bNetHostParked) { NetClient_HostUnpark(); return; }		// COOP s38: the sea of the host waits hidden (net\Board.c)
		if (!bServerInfoReady) { return; }
		if (bBuyHostStartSent) { return; }
		bBuyHostStartSent = true;
		iBuyHostStartTimer = 5000;
		Net_StartServer();
		return;
	}

	// COOP: only the host (the machine that runs the server) can start the game from the lobby
	if (!NetServer_IsStarted() && !bNetClientIsAdmin) { return; }
	if (bBuyHostStartSent) { return; }

	bBuyHostStartSent = true;
	iBuyHostStartTimer = 3000;

	Net_StartServer();
}

void Buy_UpdatePlayersQuantity()
{
	//int iNumClients = 0;
	//for (int i=0; i<NET_MAXCLIENTS; i++) { if (sti(NCClients[i].Use)) { iNumClients++; } }

	string sValue = "#" + iNumClients + " / " + NCServer.MaxClients;
	StringCollection_SetText("STRINGS_MAP_INFO_VALUE", 3, sValue);
}

void Buy_UpdateCredit()
{
	string sValue = "#" + iCurrentCredit + " / " + iMaxCredit;
	StringCollection_SetText("STRINGS_MAP_INFO_VALUE", 2, sValue);
	if (bNetBuyProfile || bNetBuyRespawn) { SetFormatedText("TEXT_PROF_MONEY", "Кол-во денег: " + iCurrentCredit + " / " + iMaxCredit); }
	Buy_UpdateDeadweight();

	int i, bState;
	bool bDisable;

	// enable/disable cannons upgrade
	for (i=0; i<3; i++)
	{
		bDisable = false;
		bState = CheckButton_GetState("RADIO_CANNONS_UPGRADELIST", i + 1);
		if (!bState && (iCurrentCredit + Net_GetCannonUpgradeCost(iCurrentCannonType, i) > iMaxCredit)) { bDisable = true; }
		CheckButton_SetDisable("RADIO_CANNONS_UPGRADELIST", i + 1, bDisable);
	}

	// enable/disable perks
	/*for (i=0; i<Net_GetPerksNum(); i++)
	{
		bDisable = false;
		bState = CheckButton_GetState("CHECKBUTTONS_PERKS", i + 1);
		if (!bState && (iCurrentCredit + Net_GetPerkCost(i) > iMaxCredit)) { bDisable = true; }
		CheckButton_SetDisable("CHECKBUTTONS_PERKS", i + 1, bDisable);
	}*/
}

// ========================================
//				SKILLS SECTION
// ========================================

int Clampi(int iValue, int iMin, int iMax)
{
	if (iValue < iMin) { iValue = iMin; }
	if (iValue > iMax) { iValue = iMax; }
	return iValue;
}

void Buy_AddSkills(int iSkillIndex, int iSkillAdd)
{
	if (XI_IsKeyPressed("shift")) { iSkillAdd = iSkillAdd * 10; }

	int i = iSkillIndex - 1;
	ref rSkill = Net_GetSkillByIndex(i);
	int iMoney = sti(rSkill.Cost);

	int iOldSkill = iSkillsQuantity[i];
	int iNewSkill = Clampi(iOldSkill + iSkillAdd, 1, 10);

	int iOldPrice = (iOldSkill - 1) * iMoney;
	int iNewPrice = (iNewSkill - 1) * iMoney;

	if (iCurrentCredit - iOldPrice + iNewPrice > iMaxCredit) { return; }

	iCurrentCredit = iCurrentCredit - iOldPrice + iNewPrice;
	iSkillsQuantity[i] = iNewSkill;

	StringCollection_SetTextValue("STRINGS_SKILLS_QUANTITY", iSkillIndex, iSkillsQuantity[iSkillIndex - 1]);

	Buy_UpdateCredit();
}

void Buy_AddAccuracy() { Buy_AddSkills(1, 1); }
void Buy_DecAccuracy() { Buy_AddSkills(1, -1); }

void Buy_AddCannons() { Buy_AddSkills(2, 1); }
void Buy_DecCannons() { Buy_AddSkills(2, -1); }

void Buy_AddSailing() { Buy_AddSkills(3, 1); }
void Buy_DecSailing() { Buy_AddSkills(3, -1); }

void Buy_AddDefence() { Buy_AddSkills(4, 1); }
void Buy_DecDefence() { Buy_AddSkills(4, -1); }

void Buy_AddRepair() { Buy_AddSkills(5, 1); }
void Buy_DecRepair() { Buy_AddSkills(5, -1); }

void Buy_AddGrappling() { Buy_AddSkills(6, 1); }
void Buy_DecGrappling() { Buy_AddSkills(6, -1); }

// ========================================
//				GOODS SECTION
// ========================================

void Buy_AddGood(int iGoodIndex, int iGoodAddQuantity)
{
	if (XI_IsKeyPressed("shift")) { iGoodAddQuantity = iGoodAddQuantity * 5; }

	int i = iGoodIndex - 1;
	ref rGood = Net_GetGoodByIndex(i);
	int iMoney = sti(rGood.Cost);
	int iUnits = sti(rGood.Units);

	int iOldQuantity = iNetGoodsQuantity[i];
	int iNewQuantity = iOldQuantity + iGoodAddQuantity * iUnits;
	if (iNewQuantity < 0) { iNewQuantity = 0; }
	if (iNewQuantity > 9999) { iNewQuantity = 9999; }

	// COOP: deadweight - no more than the hold takes (the step is made smaller until it fits)
	if (iNewQuantity > iOldQuantity)
	{
		int iCap = Net_GetShipCapacity(iCurrentShipType);
		int iOtherWeight = Buy_GoodsWeight() - Net_GetGoodWeight(i, iOldQuantity);
		while (iNewQuantity > iOldQuantity && Buy_LoadWith(iCurrentShipType, iCurrentCannonType, Buy_CannonsNum(iCurrentShipType), iCrewPercent, iOtherWeight + Net_GetGoodWeight(i, iNewQuantity)) > iCap)
		{
			iNewQuantity = iNewQuantity - iUnits;
		}
		if (iNewQuantity <= iOldQuantity) { return; }
	}

	int iOldPrice = (iOldQuantity * iMoney) / iUnits;
	int iNewPrice = (iNewQuantity * iMoney) / iUnits;

	if (iCurrentCredit - iOldPrice + iNewPrice > iMaxCredit) { return; }

	iCurrentCredit = iCurrentCredit - iOldPrice + iNewPrice;
	iNetGoodsQuantity[i] = iNewQuantity;

	StringCollection_SetTextValue("STRINGS_AMMO_QUANTITY", iGoodIndex, iNewQuantity);

	Buy_UpdateCredit();
}

void Buy_AddBalls() { Buy_AddGood(1, 1); }
void Buy_DecBalls() { Buy_AddGood(1, -1); }

void Buy_AddGrapes() { Buy_AddGood(2, 1); }
void Buy_DecGrapes() { Buy_AddGood(2, -1); }

void Buy_AddKnippels() { Buy_AddGood(3, 1); }
void Buy_DecKnippels() { Buy_AddGood(3, -1); }

void Buy_AddBombs() { Buy_AddGood(4, 1); }
void Buy_DecBombs() { Buy_AddGood(4, -1); }

void Buy_AddPlanks() { Buy_AddGood(5, 1); }
void Buy_DecPlanks() { Buy_AddGood(5, -1); }

void Buy_AddSailcloth() { Buy_AddGood(6, 1); }
void Buy_DecSailcloth() { Buy_AddGood(6, -1); }

void Buy_AddPowder() { Buy_AddGood(7, 1); }
void Buy_DecPowder() { Buy_AddGood(7, -1); }

// percent of the crew on the own ship (10..100), changes the ship weight and the boarding strength
void Buy_SetCrewPercent(int iDelta)
{
	if (XI_IsKeyPressed("shift")) { iDelta = iDelta * 2; }
	int iNewPercent = Clampi(iCrewPercent + iDelta * 5, 10, 100);
	if (iNewPercent == iCrewPercent) { return; }
	// COOP: deadweight - 1 cwt per sailor
	if (iNewPercent > iCrewPercent && Buy_LoadWith(iCurrentShipType, iCurrentCannonType, Buy_CannonsNum(iCurrentShipType), iNewPercent, Buy_GoodsWeight()) > Net_GetShipCapacity(iCurrentShipType)) { return; }

	// hiring the crew is paid
	int iOldPrice = Net_GetCrewCost(iCurrentShipType, iCrewPercent);
	int iNewPrice = Net_GetCrewCost(iCurrentShipType, iNewPercent);
	if (iCurrentCredit - iOldPrice + iNewPrice > iMaxCredit) { return; }

	iCurrentCredit = iCurrentCredit - iOldPrice + iNewPrice;
	iCrewPercent = iNewPercent;
	StringCollection_SetText("STRINGS_AMMO_QUANTITY", 8, "#" + iCrewPercent + "%");
	Buy_UpdateCrewMoney();
	Buy_UpdateCredit();
}

// price of the current crew in the money column
void Buy_UpdateCrewMoney()
{
	StringCollection_SetTextValue("STRINGS_AMMO_MONEY", 8, Net_GetCrewCost(iCurrentShipType, iCrewPercent));
}
// =============================================================================================
// COOP: cannons percent, deadweight
// =============================================================================================
int Buy_CannonsNum(int iShipType)
{
	return Net_GetCannonsCountByPercent(iShipType, iCannonsPercent);
}

int Buy_GoodsWeight()
{
	int iW = 0;
	for (int i=0; i<Net_GetGoodsNum(); i++) { iW = iW + Net_GetGoodWeight(i, iNetGoodsQuantity[i]); }
	return iW;
}

// load of the ship (cwt): cannons + crew + goods, as on the server (server\OnLoad.c: NetServer_ShipLoad)
int Buy_LoadWith(int iShipType, int iCannonType, int iCannonsNum, int iCrewPct, int iGoodsWeight)
{
	if (iShipType < 0) { return 0; }
	return Net_GetShipLoadWeight(iCannonType, iCannonsNum, Net_GetCrewCount(iShipType, iCrewPct), iGoodsWeight);
}

int Buy_CurrentLoad()
{
	return Buy_LoadWith(iCurrentShipType, iCurrentCannonType, Buy_CannonsNum(iCurrentShipType), iCrewPercent, Buy_GoodsWeight());
}

// the caption of the goods block: "Goods. Deadweight 120/600"
void Buy_UpdateDeadweight()
{
	if (iCurrentShipType < 0) { return; }
	string sCap = XI_ConvertString("capNetBuy_AmmoList") + ", Дедвейт " + Buy_CurrentLoad() + "/" + Net_GetShipCapacity(iCurrentShipType);
	StringCollection_SetText("CAPTIONS", 6, "#" + sCap);
}

// the hold is overloaded (another ship / cannons): goods are sold back from the last one until everything fits
void Buy_FitCargo()
{
	if (iCurrentShipType < 0) { return; }
	int iCap = Net_GetShipCapacity(iCurrentShipType);
	int i = Net_GetGoodsNum() - 1;
	while (i >= 0 && Buy_CurrentLoad() > iCap)
	{
		if (iNetGoodsQuantity[i] <= 0) { i = i - 1; continue; }
		ref rGood = Net_GetGoodByIndex(i);
		int iUnits = sti(rGood.Units);
		int iMoney = sti(rGood.Cost);
		int iOldQ = iNetGoodsQuantity[i];
		int iNewQ = iOldQ - iUnits;
		if (iNewQ < 0) { iNewQ = 0; }
		iCurrentCredit = iCurrentCredit - (iOldQ * iMoney) / iUnits + (iNewQ * iMoney) / iUnits;
		iNetGoodsQuantity[i] = iNewQ;
		StringCollection_SetTextValue("STRINGS_AMMO_QUANTITY", i + 1, iNewQ);
	}
}

// percent of the cannons (step 10%, shift - 50%): the price of the cannons and of their upgrade follows the number
void Buy_SetCannonsPercent(int iDelta)
{
	int iStep = 10;
	if (XI_IsKeyPressed("shift")) { iStep = 50; }
	int iNewPercent = Clampi(iCannonsPercent + iDelta * iStep, 0, 100);
	if (iNewPercent == iCannonsPercent) { return; }

	int iOldNum = Buy_CannonsNum(iCurrentShipType);
	int iNewNum = Net_GetCannonsCountByPercent(iCurrentShipType, iNewPercent);
	int iOneCannon = Net_GetCannonCost(iCurrentCannonType) + Net_GetCannonUpgradeCost(iCurrentCannonType, iCurrentCannonUpgrade);
	int iPriceDelta = (iNewNum - iOldNum) * iOneCannon;

	if (iPriceDelta > 0 && iCurrentCredit + iPriceDelta > iMaxCredit) { return; }
	if (iNewNum > iOldNum && Buy_LoadWith(iCurrentShipType, iCurrentCannonType, iNewNum, iCrewPercent, Buy_GoodsWeight()) > Net_GetShipCapacity(iCurrentShipType)) { return; }

	iCannonsPercent = iNewPercent;
	iCurrentCredit = iCurrentCredit + iPriceDelta;
	Buy_UpdateCannonsMoney();
	Buy_UpdateCredit();
}
void Buy_AddCannonsPercent() { Buy_SetCannonsPercent(1); }
void Buy_DecCannonsPercent() { Buy_SetCannonsPercent(-1); }

void Buy_UpdateCannonsMoney()
{
	int iNum = Buy_CannonsNum(iCurrentShipType);
	StringCollection_SetText("STRINGS_CANNONS_PERCENT", 1, "#" + iCannonsPercent + "%");
	StringCollection_SetTextValue("STRINGS_SHIPCANNONS_MONEY", 2, iNum * Net_GetCannonCost(iCurrentCannonType));
	StringCollection_SetText("STRINGS_CANNON_UPGRADE_MONEY", 1, "#" + iNum * Net_GetCannonUpgradeCost(iCurrentCannonType, 0));
	StringCollection_SetText("STRINGS_CANNON_UPGRADE_MONEY", 2, "#" + iNum * Net_GetCannonUpgradeCost(iCurrentCannonType, 1));
	StringCollection_SetText("STRINGS_CANNON_UPGRADE_MONEY", 3, "#" + iNum * Net_GetCannonUpgradeCost(iCurrentCannonType, 2));
}

// start charge of the radio buttons (0 balls, 1 grapes, 2 knippels, 3 bombs)
int Buy_GetStartCharge()
{
	for (int i=0; i<4; i++)
	{
		if (CheckButton_GetState("RADIO_START_CHARGE", i + 1)) { return i; }
	}
	return 0;
}

void Buy_AddCrew() { Buy_SetCrewPercent(1); }
void Buy_DecCrew() { Buy_SetCrewPercent(-1); }

void Buy_Server_UpdateShipType()
{
	if (bNetBuyProfile) { return; }		// COOP: the ship editor of the profile: no server
	if (bNetClientInGame) { return; }	// COOP s30: our ship is in the game already (a reconnect): the lobby does not change it
	int iSMsg = NMCreate();
	NMAddClientID(iSMsg, wClientID);
	NMAddByte(iSMsg, NC_BUY);
	NMAddByte(iSMsg, NSC_BUY_SHIPCHANGE);
	NMAddWord(iSMsg, iCurrentShipType);
	NetClient_SendMessage(iSMsg, true);
	NMDelete(iSMsg);
}

void Buy_Server_UpdateReady()
{
	if (bNetBuyProfile || bNetBuyRespawn) { return; }		// COOP: the ship editor of the profile: no server
	int iSMsg = NMCreate();
	NMAddClientID(iSMsg, wClientID);
	NMAddByte(iSMsg, NC_BUY);
	NMAddByte(iSMsg, NSC_BUY_READY);
	NetClient_SendMessage(iSMsg, true);
	NMDelete(iSMsg);
}

// ========================================
// COOP: the ship editor of the profile (PlayerSettings.c, "Корабль"): this screen without a server, with one million.
// "Принять" saves the ship in the profile, "Назад" leaves it as it was
// ========================================
void Buy_ProfModeInit()
{
	int i;
	sNetLandProfNick = sNetBuyProfNick;
	bServerInfoReady = true;
	if (!CheckAttribute(&NCServer, "MaxShipClass")) { NCServer.MaxShipClass = 1; }
	if (sti(NCServer.MaxShipClass) < 1) { NCServer.MaxShipClass = 1; }
	NCServer.MaxShipClass = 1;		// every ship
	iMaxCredit = 1000000;
	iCurrentCredit = 0;
	iCurrentShipType = -1;
	iCurrentShipUpgradeHull = -1;
	iCurrentShipUpgradeRigging = -1;
	iCurrentCannonType = -1;
	iCurrentCannonUpgrade = -1;

	XI_WindowShow("WND_NETBUY_WAITINFO", false);
	XI_WindowDisable("WND_NETBUY_WAITINFO", true);
	XI_WindowShow("WND_STARTGAME_WAIT", false);
	XI_WindowDisable("WND_STARTGAME_WAIT", true);
	XI_WindowDisable("WND_NETBUY", false);

	// only the shop: no players, no chat, no start
	SetNodeUsing("TABLE_PLAYERS", false);
	SetNodeUsing("SCROLL_PLAYERS", false);
	SetNodeUsing("BTN_CHAT", false);
	SetNodeUsing("BTN_ADMIN", false);
	SetNodeUsing("BTN_START", false);
	SetNodeUsing("BTN_RESET", false);
	SetNodeUsing("BTN_DISCONNECT", false);
	SetNodeUsing("BTN_LOBBY_LOADOUT", false);
	SetNodeUsing("SERVER_MAP_PICTURE", false);
	SetFormatedText("CAPTION_MAP_NAME", "Корабль профиля " + sNetBuyProfNick);

	for (i=0; i<Net_GetSkillsNum(); i++)
	{
		iSkillsQuantity[i] = 1;
		StringCollection_SetTextValue("STRINGS_SKILLS_QUANTITY", i + 1, iSkillsQuantity[i]);
		StringCollection_SetTextValue("STRINGS_SKILLS_MONEY", i + 1, Net_GetSkillCost(i, 1));
	}
	for (i=0; i<6; i++)
	{
		iPerks[i] = 0;
		StringCollection_SetTextValue("STRINGS_PERKS_MONEY", i + 1, Net_GetPerkCost(i));
	}
	Buy_SeaPerksReset();		// COOP s28
	iCrewPercent = 100;
	StringCollection_SetText("STRINGS_AMMO_QUANTITY", 8, "#" + iCrewPercent + "%");
	iCannonsPercent = 100;
	StringCollection_SetText("STRINGS_CANNONS_PERCENT", 1, "#" + iCannonsPercent + "%");
	CheckButton_SetState("RADIO_START_CHARGE", 1, true);
	for (i=0; i<Net_GetGoodsNum(); i++)
	{
		iNetGoodsQuantity[i] = 0;
		StringCollection_SetTextValue("STRINGS_AMMO_MONEY", i + 1, Net_GetGoodCost(i, sti(NetGoods[i].Units)));
		StringCollection_SetTextValue("STRINGS_AMMO_QUANTITY", i + 1, iNetGoodsQuantity[i]);
	}
	Buy_SetShip(1);
	StringCollection_SetText("STRINGS_MAP_INFO_VALUE", 1, "#Профиль");
	// no players and no class of the ships here: the lines are empty ("#" alone shows "(null)")
	StringCollection_SetText("STRINGS_MAP_INFO", 3, "# ");
	StringCollection_SetText("STRINGS_MAP_INFO", 4, "# ");
	StringCollection_SetText("STRINGS_MAP_INFO_VALUE", 3, "# ");
	StringCollection_SetText("STRINGS_MAP_INFO_VALUE", 4, "# ");
	Buy_AddGood(1, 2);
	Buy_AddGood(7, 2);
	Buy_ProfRestore();
	Buy_UpdateCredit();
	// COOP s30: the 3D scene of the main menu above the panels (net\MenuScene.c): the camera flies from the hero to the side of
	// the ship of the profile. No background, no title, no map: the whole upper part of the screen is the ship
	NetPS_SceneEnsure();
	NetPS_SetView(1);
	SetNodeUsing("MAIN_BACK", false);
	SetNodeUsing("TITLE", false);
	SetNodeUsing("FRAME_MAP", false);
	SetNodeUsing("FRAME_MAP2", false);
	SetNodeUsing("CAPTION_MAP_NAME", false);
	SetNodeUsing("STRINGS_MAP_INFO", false);
	SetNodeUsing("STRINGS_MAP_INFO_VALUE", false);
	string sShipNm = sNetBuyProfNick;
	if (CheckAttribute(&NetPSEdit, "ShipName"))
	{
		if (NetPSEdit.ShipName != "") { sShipNm = NetPSEdit.ShipName; }
	}
	GameInterface.EDITBOX_PROF_SHIPNAME.str = sShipNm;
	int iClr = 0;
	if (CheckAttribute(&NetPSEdit, "ColorSail")) { iClr = sti(NetPSEdit.ColorSail); }
	ScrollImage_SetPosition("COLORSLIST", iClr);
	if (CheckAttribute(&NetPSEdit, "ImageFlag")) { ScrollImage_SetPosition("FLAGSLIST", Buy_ProfFindIndex("FLAGSLIST", NetPSEdit.ImageFlag)); }
	if (CheckAttribute(&NetPSEdit, "ImageSail")) { ScrollImage_SetPosition("SAILSLIST", Buy_ProfFindIndex("SAILSLIST", NetPSEdit.ImageSail)); }
	SetCurrentNode("BTN_PROF_OK");
	Trace("COOP: the ship editor of the profile " + sNetBuyProfNick);
}

// ========================================
// COOP s30: the ship screen of the profile: the camera of the 3D scene, the window "Герб/Паруса/Флаг"
// ========================================
void Buy_ProfRotL() { NetPS_Turn(-1); }
void Buy_ProfRotR() { NetPS_Turn(1); }
void Buy_ProfZoomIn() { NetPS_Zoom(1); }
void Buy_ProfZoomOut() { NetPS_Zoom(-1); }

int Buy_ScrollGetTexture()
{
	string sName = GetEventData();
	int iPicIndex = GetEventData();
	string sScrollID = GetEventData();
	return NFGetTexture(sScrollID, sName);
}

int Buy_ProfFindIndex(string sControl, string sName)
{
	int iNum = sti(GameInterface.(sControl).ListSize);
	for (int i=0; i<iNum; i++)
	{
		string sRow = "pic" + (i + 1);
		if (GameInterface.(sControl).(sRow).FileName.Name == sName) { return i; }
	}
	return 0;
}

int Buy_ProfColorIdx()
{
	int iC = 0;
	if (CheckAttribute(&GameInterface, "COLORSLIST.current")) { iC = sti(GameInterface.COLORSLIST.current); }
	if (iC < 0 || iC >= SAILS_COLOR_QUANTITY) { iC = 0; }
	return iC;
}

// the file name of the chosen picture of a list (FLAGSLIST, SAILSLIST)
string Buy_ProfListName(string sControl, string sDef)
{
	if (!CheckAttribute(&GameInterface, sControl + ".current")) { return sDef; }
	string sPos = "pic" + (sti(GameInterface.(sControl).current) + 1);
	if (!CheckAttribute(&GameInterface, sControl + "." + sPos + ".FileName.Name")) { return sDef; }
	return GameInterface.(sControl).(sPos).FileName.Name;
}

void Buy_ProfFrame()
{
	NetPS_WantShip(iCurrentShipType, sti(SailsColors[Buy_ProfColorIdx()].color), Buy_ProfListName("SAILSLIST", ""), iCurrentShipUpgradeRigging + 1, Buy_ProfListName("FLAGSLIST", ""));
}

// "Принять": the ship name, the sails and the flag go to the profile screen (NetPSEdit) and to the saved profile
void Buy_ProfApplyLook()
{
	string sN = sNetBuyProfNick;
	string sShipNm = GameInterface.EDITBOX_PROF_SHIPNAME.str;
	if (sShipNm == "") { sShipNm = sN; }
	NetPSEdit.ShipName = sShipNm;
	NetPSEdit.ColorSail = Buy_ProfColorIdx();
	NetPSEdit.ImageFlag = Buy_ProfListName("FLAGSLIST", "_emptyflag_");
	NetPSEdit.ImageSail = Buy_ProfListName("SAILSLIST", "_emptysail_");
	if (!CheckAttribute(&NCProfiles, "Profiles." + sN)) { return; }
	NCProfiles.Profiles.(sN).ShipName = NetPSEdit.ShipName;
	NCProfiles.Profiles.(sN).ColorSail = NetPSEdit.ColorSail;
	NCProfiles.Profiles.(sN).ImageFlag = NetPSEdit.ImageFlag;
	NCProfiles.Profiles.(sN).ImageSail = NetPSEdit.ImageSail;
}

// COOP s30: the last profile (NetClient at the start of the game: NetClient.c Net_CreateClient) and the client now get the look
// too, when it is their profile: the sails, the gerald and the flag were lost after a restart
void Buy_ProfApplyLast()
{
	string sN = sNetBuyProfNick;
	int iSc = sti(SailsColors[sti(NetPSEdit.ColorSail)].color);
	if (CheckAttribute(&NCProfiles, "LastProfile.NickName"))
	{
		if (NCProfiles.LastProfile.NickName == sN)
		{
			NCProfiles.LastProfile.ShipName = NetPSEdit.ShipName;
			NCProfiles.LastProfile.SailColor = iSc;
			NCProfiles.LastProfile.FlagImage = NetPSEdit.ImageFlag;
			NCProfiles.LastProfile.SailImage = NetPSEdit.ImageSail;
		}
	}
	if (CheckAttribute(&NetClient, "NickName"))
	{
		if (NetClient.NickName == sN)
		{
			NetClient.ShipName = NetPSEdit.ShipName;
			NetClient.SailColor = iSc;
			NetClient.FlagImage = NetPSEdit.ImageFlag;
			NetClient.SailImage = NetPSEdit.ImageSail;
		}
	}
}

void Buy_ProfFlagsEnable(bool bOn)
{
	XI_WindowDisable("WND_NETBUY", !bOn);
	Button_SetEnable("BTN_PROF_OK", bOn);
	Button_SetEnable("BTN_PROF_BACK", bOn);
	Button_SetEnable("BTN_PROF_PREVIEW", bOn);
	Button_SetEnable("BTN_PROF_FLAGS", bOn);
	Button_SetEnable("BTN_PROF_ZOOMIN", bOn);
	Button_SetEnable("BTN_PROF_ZOOMOUT", bOn);
}

void Buy_ProfFlagsOpen()
{
	if (!bNetBuyProfile) { return; }
	Buy_ProfFlagsEnable(false);
	XI_WindowShow("WND_PROF_FLAGS", true);
	XI_WindowDisable("WND_PROF_FLAGS", false);
	SetCurrentNode("COLORSLIST");
}

void Buy_ProfFlagsClose()
{
	XI_WindowShow("WND_PROF_FLAGS", false);
	XI_WindowDisable("WND_PROF_FLAGS", true);
	Buy_ProfFlagsEnable(true);
	SetCurrentNode("BTN_PROF_FLAGS");
}

void Buy_ProfOk()
{
	if (bNetBuyRespawn) { Buy_RespOk(); return; }
	if (!bNetBuyProfile) { return; }
	Buy_ProfApplyLook();		// COOP s30: the ship name, the sails and the flag (before the save: it writes Profiles.nsv)
	Buy_ProfApplyLast();
	Buy_ProfSave(true);
	Log_SetStringToLog("Корабль сохранён в профиле " + sNetBuyProfNick);
	Buy_ProfLeave();
}

void Buy_ProfLeave()
{
	if (bNetBuyRespawn) { Buy_RespLeave(); return; }
	if (!bNetBuyProfile) { return; }
	bNetBuyProfile = false;
	sNetLandProfNick = "";
	bNetLobbyChatActive = false;
	IDoExit(RC_INTERFACE_NET_PLAYERSETTINGS);
}

// COOP s46: the loadout of NSC_BUY_FULLINFO / NSC_BUY_RESPAWN (server\Buy.c: NetServer_BuyRead)
void Buy_AddPayload(int iSMsg)
{
	int i;
	NMAddWord(iSMsg, iCurrentShipType);
	NMAddByte(iSMsg, iCurrentShipUpgradeHull + 1);
	NMAddByte(iSMsg, iCurrentShipUpgradeRigging + 1);
	NMAddByte(iSMsg, iCurrentCannonType);
	NMAddByte(iSMsg, iCurrentCannonUpgrade + 1);
	for (i=0; i<Net_GetSkillsNum(); i++) { NMAddByte(iSMsg, iSkillsQuantity[i]); }
	for (i=0; i<6; i++) { NMAddByte(iSMsg, iPerks[i]); }
	for (i=0; i<Net_GetGoodsNum(); i++) { NMAddWord(iSMsg, iNetGoodsQuantity[i]); }
	NMAddByte(iSMsg, iCrewPercent);
	NMAddByte(iSMsg, Buy_GetStartCharge());		// COOP: the server reads the start charge after the crew
	NMAddByte(iSMsg, iCannonsPercent + 1);		// COOP: percent of the cannons (+ 1: 0 means an old client)
	NMAddDword(iSMsg, Buy_SeaPerksMask());		// COOP s28: the sea perks bought here (server\Buy.c)
}

// ========================================
// COOP s46: "Корабль" of the Esc menu of a sea game with respawn (NetMenu.c): this screen over the running game, without
// the lobby. The money and the ships of the server are known already (no server info request: it would reset the players).
// "Принять" sends the loadout, the server gives it at the next respawn (the ship that sails now does not change)
// ========================================
void Buy_RespModeInit()
{
	int i;
	bServerInfoReady = true;
	bNetLobbyChatActive = false;		// the chat of the game stays the chat of the game
	iMaxCredit = sti(NCServer.Credit);
	iCurrentCredit = 0;
	iCurrentShipType = -1;
	iCurrentShipUpgradeHull = -1;
	iCurrentShipUpgradeRigging = -1;
	iCurrentCannonType = -1;
	iCurrentCannonUpgrade = -1;

	XI_WindowShow("WND_NETBUY_WAITINFO", false);
	XI_WindowDisable("WND_NETBUY_WAITINFO", true);
	XI_WindowShow("WND_STARTGAME_WAIT", false);
	XI_WindowDisable("WND_STARTGAME_WAIT", true);
	XI_WindowDisable("WND_NETBUY", false);

	// only the shop: no players, no chat, no start
	SetNodeUsing("TABLE_PLAYERS", false);
	SetNodeUsing("SCROLL_PLAYERS", false);
	SetNodeUsing("BTN_CHAT", false);
	SetNodeUsing("BTN_ADMIN", false);
	SetNodeUsing("BTN_START", false);
	SetNodeUsing("BTN_RESET", false);
	SetNodeUsing("BTN_DISCONNECT", false);
	SetNodeUsing("BTN_LOBBY_LOADOUT", false);
	SetNodeUsing("BTN_DETAILS", false);
	SetNodeUsing("BTN_GAMESETTINGS", false);
	SetNodeUsing("BTN_LOBBY_BOTS", false);
	SetNodeUsing("CHECKBUTTON_LOBBY_BOTRESPAWN", false);
	SetNodeUsing("TEXT_PROF_MONEY", true);

	for (i=0; i<Net_GetSkillsNum(); i++)
	{
		iSkillsQuantity[i] = 1;
		StringCollection_SetTextValue("STRINGS_SKILLS_QUANTITY", i + 1, iSkillsQuantity[i]);
		StringCollection_SetTextValue("STRINGS_SKILLS_MONEY", i + 1, Net_GetSkillCost(i, 1));
	}
	for (i=0; i<6; i++)
	{
		iPerks[i] = 0;
		StringCollection_SetTextValue("STRINGS_PERKS_MONEY", i + 1, Net_GetPerkCost(i));
	}
	Buy_SeaPerksReset();
	iCrewPercent = 100;
	StringCollection_SetText("STRINGS_AMMO_QUANTITY", 8, "#" + iCrewPercent + "%");
	iCannonsPercent = 100;
	StringCollection_SetText("STRINGS_CANNONS_PERCENT", 1, "#" + iCannonsPercent + "%");
	CheckButton_SetState("RADIO_START_CHARGE", 1, true);
	for (i=0; i<Net_GetGoodsNum(); i++)
	{
		iNetGoodsQuantity[i] = 0;
		StringCollection_SetTextValue("STRINGS_AMMO_MONEY", i + 1, Net_GetGoodCost(i, sti(NetGoods[i].Units)));
		StringCollection_SetTextValue("STRINGS_AMMO_QUANTITY", i + 1, iNetGoodsQuantity[i]);
	}
	Buy_SetShip(1);

	string sIslandMap = "OpenSea";
	if (CheckAttribute(&NCServer, "IslandID")) { if (NCServer.IslandID != "") { sIslandMap = NCServer.IslandID; } }
	Picture_SetPicture("SERVER_MAP_PICTURE", "Interfaces\Net\Islands\" + sIslandMap + ".tga");
	SetFormatedText("CAPTION_MAP_NAME", "Корабль после гибели текущего");
	StringCollection_SetText("STRINGS_MAP_INFO_VALUE", 1, "txtCS_" + NetModes[sti(NCServer.GameType)].Name);
	StringCollection_SetText("STRINGS_MAP_INFO_VALUE", 4, "#" + NCServer.MaxShipClass);
	Buy_UpdatePlayersQuantity();

	Buy_AddGood(1, 2);
	Buy_AddGood(7, 2);
	Buy_ProfRestore();		// the last loadout of this profile (the lobby saved it at the start, "Принять" here too)
	Buy_UpdateCredit();
	SetCurrentNode("BTN_PROF_OK");
	Trace("COOP: the ship of the next respawn: the buy screen over the game");
}

void Buy_RespOk()
{
	if (!bNetBuyRespawn) { return; }
	if (iCurrentCredit > iMaxCredit) { SetFormatedText("TEXT_PROF_MONEY", "Не хватает денег"); return; }
	int iSMsg = NMCreate();
	NMAddClientID(iSMsg, wClientID);
	NMAddByte(iSMsg, NC_BUY);
	NMAddByte(iSMsg, NSC_BUY_RESPAWN);
	Buy_AddPayload(iSMsg);
	NetClient_SendMessage(iSMsg, true);
	NMDelete(iSMsg);
	Buy_ProfSave(false);
	Trace("COOP: the loadout of the next respawn is sent, ship type " + iCurrentShipType);
	Buy_RespLeave();
}

void Buy_RespLeave()
{
	if (!bNetBuyRespawn) { return; }
	bNetBuyRespawn = false;
	IDoExit(RC_INTERFACE_NET_OPTIONS_EXIT);		// back to the Esc menu of the game (seadogs.c)
}
