// COOP: Esc menu of the running game: return / admin menu / leave the game.
// The admin menu: login window (admin password) and the admin panel.

bool bNetMenuLogin = false;
bool bNetMenuAdmin = false;
// COOP: bot windows of the admin (server\Bots.c)
bool bNetMenuBotAdd = false;
bool bNetMenuBots = false;
int iNMBotShip = -1;
int iNMBotCannon = -1;
int iNMBotCrew = 100;
int iNMBotHull = 100;
int iNMBotSails = 100;
int iNMBotRel = NET_BOTREL_ENEMY;
int iNMBotLevel = 10;			// COOP: land duel: the level of the new bot
string sNMBotGun = "";			// and its gun ("": none)
int iNMBotFace = -1;			// COOP: land duel: the skin (portrait) of the new bot
string sNMBotBlade = "";		// and its blade
int iNMBotsTimerGen = 0;
bool bNetMenuLoadout = false;		// COOP: land duel loadout window (net\Land.c)

void InitInterface(string iniName)
{
	GameInterface.title = "";

	SendMessage(&GameInterface, "ls", MSG_INTERFACE_INIT, iniName);

	SetEventHandler("OnNetMenuDisconnect", "ICancelProcedure", 0);
	SetEventHandler("OnNetMenuReturn", "IReturnToGame", 0);
	SetEventHandler("OnNetMenuOptions", "NetMenu_OpenOptions", 0);	// COOP: button "Options"
	SetEventHandler("exitCancel", "NetMenu_Cancel", 0);
	SetEventHandler("NetInfoShow", "NetMenu_InfoShow", 0);		// COOP s31: the right click in the loadout: a description (net/NetInfo.c)
	SetEventHandler("MouseRClickUp", "NetInfo_Hide", 0);

	SetEventHandler("OnNetMenuAdmin", "NetMenu_OpenAdmin", 0);
	SetEventHandler("OnAdminBack", "NetMenu_Back", 0);
	SetEventHandler("OnAdminLogin", "NetMenu_LoginSend", 0);
	SetEventHandler("EditExit", "NetMenu_EditExit", 0);

	SetEventHandler("NCEvent_AdminStatus", "NetMenu_OnStatus", 0);
	SetEventHandler("NCEvent_AdminFlags", "NetMenu_FillTable", 0);
	SetEventHandler("NCEvent_ClientDrop", "NetMenu_FillTable", 0);
	SetEventHandler("NCEvent_AdminEnv", "NetMenu_UpdateEnv", 0);
    SetEventHandler("OnAdmKick", "NetMenu_OnKick", 0);
    SetEventHandler("OnAdmMute", "NetMenu_OnMute", 0);
    SetEventHandler("OnAdmTpTo", "NetMenu_OnTpTo", 0);
    SetEventHandler("OnAdmBring", "NetMenu_OnBring", 0);
    SetEventHandler("OnAdmMake", "NetMenu_OnMake", 0);
    SetEventHandler("OnAdmHeal", "NetMenu_OnHeal", 0);
    SetEventHandler("OnAdmDestroy", "NetMenu_OnDestroy", 0);
    SetEventHandler("OnAdmHealAll", "NetMenu_OnHealAll", 0);
    SetEventHandler("OnAdmReload", "NetMenu_OnReload", 0);
    SetEventHandler("OnAdmWeatherDec", "NetMenu_OnWeatherDec", 0);
    SetEventHandler("OnAdmWeatherInc", "NetMenu_OnWeatherInc", 0);
    SetEventHandler("OnAdmTimeDec", "NetMenu_OnTimeDec", 0);
    SetEventHandler("OnAdmTimeInc", "NetMenu_OnTimeInc", 0);
    SetEventHandler("OnAdmSpeedDec", "NetMenu_OnSpeedDec", 0);
    SetEventHandler("OnAdmSpeedInc", "NetMenu_OnSpeedInc", 0);
    SetEventHandler("OnAdmWindAngDec", "NetMenu_OnWindAngDec", 0);
    SetEventHandler("OnAdmWindAngInc", "NetMenu_OnWindAngInc", 0);
    SetEventHandler("OnAdmWindSpdDec", "NetMenu_OnWindSpdDec", 0);
    SetEventHandler("OnAdmWindSpdInc", "NetMenu_OnWindSpdInc", 0);
    // COOP: land duel loadout
    SetEventHandler("OnNetMenuLoadout", "NetMenu_OpenLoadout", 0);
    SetEventHandler("OnLoBack", "NetMenu_LoBack", 0);
    SetEventHandler("OnLoApply", "NetMenu_LoApply", 0);
    SetEventHandler("OnLoPerk", "NetMenu_LoPerk", 0);
    SetEventHandler("OnLo0Dec", "NetMenu_Lo0Dec", 0);
    SetEventHandler("OnLo0Inc", "NetMenu_Lo0Inc", 0);
    SetEventHandler("OnLo1Dec", "NetMenu_Lo1Dec", 0);
    SetEventHandler("OnLo1Inc", "NetMenu_Lo1Inc", 0);
    SetEventHandler("OnLo2Dec", "NetMenu_Lo2Dec", 0);
    SetEventHandler("OnLo2Inc", "NetMenu_Lo2Inc", 0);
    SetEventHandler("OnLo3Dec", "NetMenu_Lo3Dec", 0);
    SetEventHandler("OnLo3Inc", "NetMenu_Lo3Inc", 0);
    SetEventHandler("OnLo4Dec", "NetMenu_Lo4Dec", 0);
    SetEventHandler("OnLo4Inc", "NetMenu_Lo4Inc", 0);
    SetEventHandler("OnLo5Dec", "NetMenu_Lo5Dec", 0);
    SetEventHandler("OnLo5Inc", "NetMenu_Lo5Inc", 0);
    SetEventHandler("OnLo6Dec", "NetMenu_Lo6Dec", 0);
    SetEventHandler("OnLo6Inc", "NetMenu_Lo6Inc", 0);
    SetEventHandler("OnLo7Dec", "NetMenu_Lo7Dec", 0);
    SetEventHandler("OnLo7Inc", "NetMenu_Lo7Inc", 0);
    SetEventHandler("OnLo8Dec", "NetMenu_Lo8Dec", 0);
    SetEventHandler("OnLo8Inc", "NetMenu_Lo8Inc", 0);
    SetEventHandler("OnLo9Dec", "NetMenu_Lo9Dec", 0);
    SetEventHandler("OnLo9Inc", "NetMenu_Lo9Inc", 0);
    SetEventHandler("OnLo10Dec", "NetMenu_Lo10Dec", 0);
    SetEventHandler("OnLo10Inc", "NetMenu_Lo10Inc", 0);
    SetEventHandler("OnLo11Dec", "NetMenu_Lo11Dec", 0);
    SetEventHandler("OnLo11Inc", "NetMenu_Lo11Inc", 0);
    SetEventHandler("OnLo12Dec", "NetMenu_Lo12Dec", 0);
    SetEventHandler("OnLo12Inc", "NetMenu_Lo12Inc", 0);
    SetEventHandler("OnLo13Dec", "NetMenu_Lo13Dec", 0);
    SetEventHandler("OnLo13Inc", "NetMenu_Lo13Inc", 0);
    SetEventHandler("OnLo14Dec", "NetMenu_Lo14Dec", 0);
    SetEventHandler("OnLo14Inc", "NetMenu_Lo14Inc", 0);
    SetEventHandler("OnLo15Dec", "NetMenu_Lo15Dec", 0);
    SetEventHandler("OnLo15Inc", "NetMenu_Lo15Inc", 0);
    SetEventHandler("OnLo16Dec", "NetMenu_Lo16Dec", 0);
    SetEventHandler("OnLo16Inc", "NetMenu_Lo16Inc", 0);
    SetEventHandler("OnLo17Dec", "NetMenu_Lo17Dec", 0);
    SetEventHandler("OnLo17Inc", "NetMenu_Lo17Inc", 0);
    SetEventHandler("OnLo18Dec", "NetMenu_Lo18Dec", 0);
    SetEventHandler("OnLo18Inc", "NetMenu_Lo18Inc", 0);
    SetEventHandler("OnLo19Dec", "NetMenu_Lo19Dec", 0);
    SetEventHandler("OnLo19Inc", "NetMenu_Lo19Inc", 0);
    SetEventHandler("OnLo20Dec", "NetMenu_Lo20Dec", 0);
    SetEventHandler("OnLo20Inc", "NetMenu_Lo20Inc", 0);
    SetEventHandler("OnLo21Dec", "NetMenu_Lo21Dec", 0);
    SetEventHandler("OnLo21Inc", "NetMenu_Lo21Inc", 0);
    SetEventHandler("OnLo22Dec", "NetMenu_Lo22Dec", 0);
    SetEventHandler("OnLo22Inc", "NetMenu_Lo22Inc", 0);
    SetEventHandler("OnLo23Dec", "NetMenu_Lo23Dec", 0);		// COOP: the armor
    SetEventHandler("OnLo23Inc", "NetMenu_Lo23Inc", 0);
    // COOP: bots
    SetEventHandler("OnAdmBotAdd", "NetMenu_OpenBotAdd", 0);
    SetEventHandler("OnAdmBots", "NetMenu_OpenBots", 0);
    SetEventHandler("OnAdmNoclip", "NetMenu_OnAdmNoclip", 0);		// COOP s27: noclip of the admin (client\Admin.c)
    SetEventHandler("OnAdmFreecam", "NetMenu_OnAdmFreecam", 0);		// COOP s27: free camera of the admin (client\Admin.c)
    SetEventHandler("OnAdmGod", "NetMenu_OnAdmGod", 0);
    SetEventHandler("OnAdmLobby", "NetMenu_OnAdmLobby", 0);		// COOP s30: everybody back to the lobby		// COOP s27: immortality of the selected player
    SetEventHandler("OnBotBack", "NetMenu_BotBack", 0);
    SetEventHandler("OnBotShipPrev", "NetMenu_OnBotShipPrev", 0);
    SetEventHandler("OnBotShipNext", "NetMenu_OnBotShipNext", 0);
    SetEventHandler("OnBotCannonPrev", "NetMenu_OnBotCannonPrev", 0);
    SetEventHandler("OnBotCannonNext", "NetMenu_OnBotCannonNext", 0);
    SetEventHandler("OnBotCrewDec", "NetMenu_OnBotCrewDec", 0);
    SetEventHandler("OnBotCrewInc", "NetMenu_OnBotCrewInc", 0);
    SetEventHandler("OnBotHullDec", "NetMenu_OnBotHullDec", 0);
    SetEventHandler("OnBotHullInc", "NetMenu_OnBotHullInc", 0);
    SetEventHandler("OnBotSailsDec", "NetMenu_OnBotSailsDec", 0);
    SetEventHandler("OnBotSailsInc", "NetMenu_OnBotSailsInc", 0);
    SetEventHandler("OnBotRelPrev", "NetMenu_OnBotRelPrev", 0);
    SetEventHandler("OnBotRelNext", "NetMenu_OnBotRelNext", 0);
    SetEventHandler("OnBotDoAdd", "NetMenu_OnBotDoAdd", 0);
    SetEventHandler("OnBotsAlly", "NetMenu_OnBotsAlly", 0);
    SetEventHandler("OnBotsEnemy", "NetMenu_OnBotsEnemy", 0);
    SetEventHandler("OnBotsNeutral", "NetMenu_OnBotsNeutral", 0);
    SetEventHandler("OnBotsTeam", "NetMenu_OnBotsTeam", 0);		// COOP s28: the bot goes to a team
    SetEventHandler("OnBotsBring", "NetMenu_OnBotsBring", 0);
    SetEventHandler("OnBotsGoTo", "NetMenu_OnBotsGoTo", 0);
    SetEventHandler("OnBotsHeal", "NetMenu_OnBotsHeal", 0);
    SetEventHandler("OnBotsDestroy", "NetMenu_OnBotsDestroy", 0);
    SetEventHandler("OnBotsDelete", "NetMenu_OnBotsDelete", 0);
    SetEventHandler("OnBotsSkillDec", "NetMenu_OnBotsSkillDec", 0);
    SetEventHandler("OnBotsSkillInc", "NetMenu_OnBotsSkillInc", 0);
    SetEventHandler("NCEvent_BotInfo", "NetMenu_FillBots", 0);
    SetEventHandler("NCEvent_ShipDead", "NetMenu_FillBots", 0);
    SetEventHandler("CheckButtonChange", "NetMenu_CheckButtonChange", 0);		// COOP: switch "Bot respawn"
    SetEventHandler("NCEvent_BotOpts", "NetMenu_BotOptsUpdate", 0);
    SetEventHandler("NetMenu_BotsTimer", "NetMenu_BotsTimer", 0);
	FreezeGroupControls("BattleInterfaceControls", true);

	NetMenu_Show("main");

	// COOP: land duel: the loadout window opens by itself once, at the start
	if (NetLand_TakeOpenLoadout()) { NetMenu_OpenLoadout(); }
}

void IDoExit(int exitCode)
{
	DelEventHandler("OnNetMenuDisconnect", "ICancelProcedure");
	DelEventHandler("OnNetMenuReturn", "IReturnToGame");
	DelEventHandler("OnNetMenuOptions", "NetMenu_OpenOptions");
	DelEventHandler("exitCancel", "NetMenu_Cancel");
	DelEventHandler("NetInfoShow", "NetMenu_InfoShow");
	DelEventHandler("MouseRClickUp", "NetInfo_Hide");

	DelEventHandler("OnNetMenuAdmin", "NetMenu_OpenAdmin");
	DelEventHandler("OnAdminBack", "NetMenu_Back");
	DelEventHandler("OnAdminLogin", "NetMenu_LoginSend");
	DelEventHandler("EditExit", "NetMenu_EditExit");

	DelEventHandler("NCEvent_AdminStatus", "NetMenu_OnStatus");
	DelEventHandler("NCEvent_AdminFlags", "NetMenu_FillTable");
	DelEventHandler("NCEvent_ClientDrop", "NetMenu_FillTable");
	DelEventHandler("NCEvent_AdminEnv", "NetMenu_UpdateEnv");
    DelEventHandler("OnAdmKick", "NetMenu_OnKick");
    DelEventHandler("OnAdmMute", "NetMenu_OnMute");
    DelEventHandler("OnAdmTpTo", "NetMenu_OnTpTo");
    DelEventHandler("OnAdmBring", "NetMenu_OnBring");
    DelEventHandler("OnAdmMake", "NetMenu_OnMake");
    DelEventHandler("OnAdmHeal", "NetMenu_OnHeal");
    DelEventHandler("OnAdmDestroy", "NetMenu_OnDestroy");
    DelEventHandler("OnAdmHealAll", "NetMenu_OnHealAll");
    DelEventHandler("OnAdmReload", "NetMenu_OnReload");
    DelEventHandler("OnAdmWeatherDec", "NetMenu_OnWeatherDec");
    DelEventHandler("OnAdmWeatherInc", "NetMenu_OnWeatherInc");
    DelEventHandler("OnAdmTimeDec", "NetMenu_OnTimeDec");
    DelEventHandler("OnAdmTimeInc", "NetMenu_OnTimeInc");
    DelEventHandler("OnAdmSpeedDec", "NetMenu_OnSpeedDec");
    DelEventHandler("OnAdmSpeedInc", "NetMenu_OnSpeedInc");
    DelEventHandler("OnAdmWindAngDec", "NetMenu_OnWindAngDec");
    DelEventHandler("OnAdmWindAngInc", "NetMenu_OnWindAngInc");
    DelEventHandler("OnAdmWindSpdDec", "NetMenu_OnWindSpdDec");
    DelEventHandler("OnAdmWindSpdInc", "NetMenu_OnWindSpdInc");
    // COOP: land duel loadout
    DelEventHandler("OnNetMenuLoadout", "NetMenu_OpenLoadout");
    DelEventHandler("OnLoBack", "NetMenu_LoBack");
    DelEventHandler("OnLoApply", "NetMenu_LoApply");
    DelEventHandler("OnLoPerk", "NetMenu_LoPerk");
    DelEventHandler("OnLo0Dec", "NetMenu_Lo0Dec");
    DelEventHandler("OnLo0Inc", "NetMenu_Lo0Inc");
    DelEventHandler("OnLo1Dec", "NetMenu_Lo1Dec");
    DelEventHandler("OnLo1Inc", "NetMenu_Lo1Inc");
    DelEventHandler("OnLo2Dec", "NetMenu_Lo2Dec");
    DelEventHandler("OnLo2Inc", "NetMenu_Lo2Inc");
    DelEventHandler("OnLo3Dec", "NetMenu_Lo3Dec");
    DelEventHandler("OnLo3Inc", "NetMenu_Lo3Inc");
    DelEventHandler("OnLo4Dec", "NetMenu_Lo4Dec");
    DelEventHandler("OnLo4Inc", "NetMenu_Lo4Inc");
    DelEventHandler("OnLo5Dec", "NetMenu_Lo5Dec");
    DelEventHandler("OnLo5Inc", "NetMenu_Lo5Inc");
    DelEventHandler("OnLo6Dec", "NetMenu_Lo6Dec");
    DelEventHandler("OnLo6Inc", "NetMenu_Lo6Inc");
    DelEventHandler("OnLo7Dec", "NetMenu_Lo7Dec");
    DelEventHandler("OnLo7Inc", "NetMenu_Lo7Inc");
    DelEventHandler("OnLo8Dec", "NetMenu_Lo8Dec");
    DelEventHandler("OnLo8Inc", "NetMenu_Lo8Inc");
    DelEventHandler("OnLo9Dec", "NetMenu_Lo9Dec");
    DelEventHandler("OnLo9Inc", "NetMenu_Lo9Inc");
    DelEventHandler("OnLo10Dec", "NetMenu_Lo10Dec");
    DelEventHandler("OnLo10Inc", "NetMenu_Lo10Inc");
    DelEventHandler("OnLo11Dec", "NetMenu_Lo11Dec");
    DelEventHandler("OnLo11Inc", "NetMenu_Lo11Inc");
    DelEventHandler("OnLo12Dec", "NetMenu_Lo12Dec");
    DelEventHandler("OnLo12Inc", "NetMenu_Lo12Inc");
    DelEventHandler("OnLo13Dec", "NetMenu_Lo13Dec");
    DelEventHandler("OnLo13Inc", "NetMenu_Lo13Inc");
    DelEventHandler("OnLo14Dec", "NetMenu_Lo14Dec");
    DelEventHandler("OnLo14Inc", "NetMenu_Lo14Inc");
    DelEventHandler("OnLo15Dec", "NetMenu_Lo15Dec");
    DelEventHandler("OnLo15Inc", "NetMenu_Lo15Inc");
    DelEventHandler("OnLo16Dec", "NetMenu_Lo16Dec");
    DelEventHandler("OnLo16Inc", "NetMenu_Lo16Inc");
    DelEventHandler("OnLo17Dec", "NetMenu_Lo17Dec");
    DelEventHandler("OnLo17Inc", "NetMenu_Lo17Inc");
    DelEventHandler("OnLo18Dec", "NetMenu_Lo18Dec");
    DelEventHandler("OnLo18Inc", "NetMenu_Lo18Inc");
    DelEventHandler("OnLo19Dec", "NetMenu_Lo19Dec");
    DelEventHandler("OnLo19Inc", "NetMenu_Lo19Inc");
    DelEventHandler("OnLo20Dec", "NetMenu_Lo20Dec");
    DelEventHandler("OnLo20Inc", "NetMenu_Lo20Inc");
    DelEventHandler("OnLo21Dec", "NetMenu_Lo21Dec");
    DelEventHandler("OnLo21Inc", "NetMenu_Lo21Inc");
    DelEventHandler("OnLo22Dec", "NetMenu_Lo22Dec");
    DelEventHandler("OnLo22Inc", "NetMenu_Lo22Inc");
    DelEventHandler("OnLo23Dec", "NetMenu_Lo23Dec");
    DelEventHandler("OnLo23Inc", "NetMenu_Lo23Inc");
    // COOP: bots
    DelEventHandler("OnAdmBotAdd", "NetMenu_OpenBotAdd");
    DelEventHandler("OnAdmBots", "NetMenu_OpenBots");
    DelEventHandler("OnAdmNoclip", "NetMenu_OnAdmNoclip");
    DelEventHandler("OnAdmFreecam", "NetMenu_OnAdmFreecam");
    DelEventHandler("OnAdmGod", "NetMenu_OnAdmGod");
    DelEventHandler("OnAdmLobby", "NetMenu_OnAdmLobby");
    DelEventHandler("OnBotBack", "NetMenu_BotBack");
    DelEventHandler("OnBotShipPrev", "NetMenu_OnBotShipPrev");
    DelEventHandler("OnBotShipNext", "NetMenu_OnBotShipNext");
    DelEventHandler("OnBotCannonPrev", "NetMenu_OnBotCannonPrev");
    DelEventHandler("OnBotCannonNext", "NetMenu_OnBotCannonNext");
    DelEventHandler("OnBotCrewDec", "NetMenu_OnBotCrewDec");
    DelEventHandler("OnBotCrewInc", "NetMenu_OnBotCrewInc");
    DelEventHandler("OnBotHullDec", "NetMenu_OnBotHullDec");
    DelEventHandler("OnBotHullInc", "NetMenu_OnBotHullInc");
    DelEventHandler("OnBotSailsDec", "NetMenu_OnBotSailsDec");
    DelEventHandler("OnBotSailsInc", "NetMenu_OnBotSailsInc");
    DelEventHandler("OnBotRelPrev", "NetMenu_OnBotRelPrev");
    DelEventHandler("OnBotRelNext", "NetMenu_OnBotRelNext");
    DelEventHandler("OnBotDoAdd", "NetMenu_OnBotDoAdd");
    DelEventHandler("OnBotsAlly", "NetMenu_OnBotsAlly");
    DelEventHandler("OnBotsEnemy", "NetMenu_OnBotsEnemy");
    DelEventHandler("OnBotsNeutral", "NetMenu_OnBotsNeutral");
    DelEventHandler("OnBotsTeam", "NetMenu_OnBotsTeam");
    DelEventHandler("OnBotsBring", "NetMenu_OnBotsBring");
    DelEventHandler("OnBotsGoTo", "NetMenu_OnBotsGoTo");
    DelEventHandler("OnBotsHeal", "NetMenu_OnBotsHeal");
    DelEventHandler("OnBotsDestroy", "NetMenu_OnBotsDestroy");
    DelEventHandler("OnBotsDelete", "NetMenu_OnBotsDelete");
    DelEventHandler("OnBotsSkillDec", "NetMenu_OnBotsSkillDec");
    DelEventHandler("OnBotsSkillInc", "NetMenu_OnBotsSkillInc");
    DelEventHandler("NCEvent_BotInfo", "NetMenu_FillBots");
    DelEventHandler("NCEvent_ShipDead", "NetMenu_FillBots");
    DelEventHandler("CheckButtonChange", "NetMenu_CheckButtonChange");
    DelEventHandler("NCEvent_BotOpts", "NetMenu_BotOptsUpdate");
    DelEventHandler("NetMenu_BotsTimer", "NetMenu_BotsTimer");
	interfaceResultCommand = exitCode;
	EndCancelInterface(true);
}

void ICancelProcedure()
{
	// COOP s31: the host leaves the game: only he goes to the lobby, the game goes on and he can join it again
	// (client\OnConnect.c NetClient_LeaveToLobby); all players go to the lobby by "В лобби" of the admin menu only
	// (not a preview, net\Land.c: Esc closes its server, the lobby would start it again at once)
	if (bNetHostExitToLobby && NetServer_IsStarted() && iNetPreview <= 0)
	{
		IDoExit(RC_INTERFACE_DO_NOTHING);
		SetEventHandler("NetClient_LeaveToLobby", "NetClient_LeaveToLobby", 0);
		PostEvent("NetClient_LeaveToLobby", 1);
		return;
	}
	// COOP: leaving the running game from the in-game net menu also goes to the main menu (see EndGame.c)
	Net_Stop();

	IDoExit( RC_INTERFACE_NET_EXIT );

	ReturnToMainMenu();
}

void NetMenu_InfoShow()
{
	NetInfo_Land(GetCurrentNode());
}

void IReturnToGame()
{
	IDoExit( INTERFACE_NET_NETMENU );
}

// COOP: the usual options screen of the single player (sound, graphics, controls). It closes back to this menu
// (interface\option_screen.c: ProcessExit, seadogs.c: RC_INTERFACE_NET_OPTIONS_EXIT)
void NetMenu_OpenOptions()
{
	InterfaceStates.NetOptionsReturn = true;
	IDoExit( RC_INTERFACE_DO_OPTIONS );
}

// Esc: from the admin windows back to the menu, from the menu back to the game
void NetMenu_Cancel()
{
	if (bNetMenuLoadout) { NetMenu_LoBack(); return; }
	if (bNetMenuBotAdd || bNetMenuBots) { NetMenu_BotBack(); return; }
	if (bNetMenuLogin || bNetMenuAdmin) { NetMenu_Back(); return; }

	IReturnToGame();
}

// sWindow: "main", "login" or "admin"
void NetMenu_Show(string sWindow)
{
	bNetMenuLogin = (sWindow == "login");
	bNetMenuAdmin = (sWindow == "admin");
	bNetMenuBotAdd = (sWindow == "botadd");		// COOP: bots
	bNetMenuBots = (sWindow == "bots");
	bNetMenuLoadout = (sWindow == "loadout");
	bNetLandUiLo = bNetMenuLoadout;

	XI_WindowShow("WND_NETMENU", sWindow == "main");
	XI_WindowDisable("WND_NETMENU", sWindow != "main");

	XI_WindowShow("WND_ADM_LOGIN", bNetMenuLogin);
	XI_WindowDisable("WND_ADM_LOGIN", !bNetMenuLogin);

	XI_WindowShow("WND_ADM", bNetMenuAdmin);
	XI_WindowDisable("WND_ADM", !bNetMenuAdmin);

	XI_WindowShow("WND_BOTADD", bNetMenuBotAdd);
	XI_WindowDisable("WND_BOTADD", !bNetMenuBotAdd);

	XI_WindowShow("WND_BOTS", bNetMenuBots);
	XI_WindowDisable("WND_BOTS", !bNetMenuBots);
	if (bNetMenuBots) { SetNodeUsing("BTN_BOTS_TEAM", NetMenu_BotTeamsNum() > 0); }		// COOP s28: only in the team games

	XI_WindowShow("WND_LOADOUT", bNetMenuLoadout);
	XI_WindowDisable("WND_LOADOUT", !bNetMenuLoadout);

	// COOP: the loadout button only in a land duel
	SetNodeUsing("BTN_NETMENU_LOADOUT", NetLand_IsActive());

	if (sWindow == "main") { SetCurrentNode("BTN_ENDGAME_RETURN"); }
}

void NetMenu_Back()
{
	if (bNetMenuBotAdd || bNetMenuBots) { NetMenu_BotBack(); return; }
	if (!bNetMenuLogin && !bNetMenuAdmin) { IReturnToGame(); return; }

	NetMenu_Show("main");
}

// the "Admin menu" button
void NetMenu_OpenAdmin()
{
	if (bNetClientIsAdmin)
	{
		NetMenu_Show("admin");
		NetClient_AdminSend(NET_ADMIN_SYNC, DST_INVALID, 0.0);
		NetMenu_FillTable();
		NetMenu_UpdateEnv();
		SetCurrentNode("TABLE_ADMIN");
		return;
	}

	SetFormatedText("TEXT_ADM_LOGIN_RESULT", "");
	GameInterface.EDITBOX_ADM_LOGIN.str = "";

	NetMenu_Show("login");
	SetCurrentNode("EDITBOX_ADM_LOGIN");
}

void NetMenu_LoginSend()
{
	string sPassword = GameInterface.EDITBOX_ADM_LOGIN.str;
	if (sPassword == "") { return; }

	NetClient_AdminLogin(sPassword);

	GameInterface.EDITBOX_ADM_LOGIN.str = "";
	SetCurrentNode("EDITBOX_ADM_LOGIN");
}

void NetMenu_EditExit()
{
	string sEditControl = GetEventData();

	if (sEditControl == "EDITBOX_ADM_LOGIN") { NetMenu_LoginSend(); }
}

// the answer of the server to the password
void NetMenu_OnStatus()
{
	int iAdmin = GetEventData();

	if (!bNetMenuLogin) { return; }

	if (iAdmin)
	{
		NetMenu_OpenAdmin();
	}
	else
	{
		SetFormatedText("TEXT_ADM_LOGIN_RESULT", XI_ConvertString("capNetAdmin_WrongPassword"));
	}
}

// ================= admin panel =================

// the flags are only set when the server tells about them: no attribute = 0 (no "missed attribute" errors)
int NetMenu_Flag(int i, string sAttr)
{
	if (!CheckAttribute(&NCClients[i], sAttr)) { return 0; }
	return sti(NCClients[i].(sAttr));
}

void NetMenu_FillTable()
{
	if (!bNetMenuAdmin) { return; }

	int i;
	int iSelected = sti(GameInterface.TABLE_ADMIN.select);

	Table_Clear("TABLE_ADMIN", false, true, false);

	GameInterface.TABLE_ADMIN.hr.td1.str = XI_ConvertString("capNetAdmin_ColPlayer");
	GameInterface.TABLE_ADMIN.hr.td1.scale = 0.85;
	GameInterface.TABLE_ADMIN.hr.td2.str = XI_ConvertString("capNetAdmin_ColAdmin");
	GameInterface.TABLE_ADMIN.hr.td2.scale = 0.85;
	GameInterface.TABLE_ADMIN.hr.td3.str = XI_ConvertString("capNetAdmin_ColMute");
	GameInterface.TABLE_ADMIN.hr.td3.scale = 0.85;
	GameInterface.TABLE_ADMIN.hr.td4.str = XI_ConvertString("capNetAdmin_ColGod");		// COOP s27
	GameInterface.TABLE_ADMIN.hr.td4.scale = 0.85;

	int iRow = 0;
	for (i=0; i<NET_MAXCLIENTS; i++)
	{
		if (!sti(NCClients[i].Use)) { continue; }

		iRow++;
		string sRow = "tr" + iRow;

		string sName = NCClients[i].NickName;
		if (i == wClientID) { sName = sName + " *"; }

		string sAdmin = "";
		if (NetMenu_Flag(i, "Admin")) { sAdmin = "+"; }

		string sMute = "";
		if (NetMenu_Flag(i, "Muted")) { sMute = "x"; }

		GameInterface.TABLE_ADMIN.(sRow).ClientID = i;
		GameInterface.TABLE_ADMIN.(sRow).td1.str = sName;
		GameInterface.TABLE_ADMIN.(sRow).td2.str = sAdmin;
		GameInterface.TABLE_ADMIN.(sRow).td3.str = sMute;
		string sGod = "";
		if (NetMenu_Flag(i, "God")) { sGod = "+"; }
		GameInterface.TABLE_ADMIN.(sRow).td4.str = sGod;
	}

	if (iRow > 0)
	{
		if (iSelected < 1 || iSelected > iRow) { iSelected = 1; }
		GameInterface.TABLE_ADMIN.select = iSelected;
	}

	Table_UpdateWindow("TABLE_ADMIN");
}

// client id of the selected row or DST_INVALID
int NetMenu_GetSelected()
{
	int iSelected = sti(GameInterface.TABLE_ADMIN.select);
	if (iSelected < 1) { return DST_INVALID; }

	string sRow = "tr" + iSelected;
	if (!CheckAttribute(&GameInterface, "TABLE_ADMIN." + sRow + ".ClientID")) { return DST_INVALID; }

	return sti(GameInterface.TABLE_ADMIN.(sRow).ClientID);
}

void NetMenu_Act(int iAction, bool bNeedTarget, float fValue)
{
	int wTarget = DST_INVALID;

	if (bNeedTarget)
	{
		wTarget = NetMenu_GetSelected();
		if (wTarget == DST_INVALID) { return; }
	}

	NetClient_AdminSend(iAction, wTarget, fValue);
}

string NetMenu_WeatherKey(int iType)
{
	switch (iType)
	{
		case 0: return "txtCS_Calm"; break;
		case 1: return "txtCS_Windy"; break;
		case 2: return "txtCS_Heavy"; break;
		case 3: return "txtCS_Storm"; break;
		case 4: return "txtCS_Tornado"; break;
	}
	return "txtCS_Calm";
}

string NetMenu_TimeKey(int iTime)
{
	switch (iTime)
	{
		case 0: return "txtCS_Morning"; break;
		case 1: return "txtCS_Day"; break;
		case 2: return "txtCS_Evening"; break;
		case 3: return "txtCS_Night"; break;
	}
	return "txtCS_Day";
}

// the values of the environment (weather, time of day, game speed, wind)
void NetMenu_UpdateEnv()
{
	if (!bNetMenuAdmin) { return; }

	int iDegrees = makeint(fNetCliAdminWindAngle * 57.29578);
	while (iDegrees >= 360) { iDegrees = iDegrees - 360; }
	while (iDegrees < 0) { iDegrees = iDegrees + 360; }

	SetFormatedText("TEXT_ADM_WEATHER", XI_ConvertString("capNetAdmin_Weather") + ": " + XI_ConvertString(NetMenu_WeatherKey(iNetCliAdminWeatherType)));
	SetFormatedText("TEXT_ADM_TIME", XI_ConvertString("capNetAdmin_Time") + ": " + XI_ConvertString(NetMenu_TimeKey(iNetCliAdminTime)));
	SetFormatedText("TEXT_ADM_SPEED", XI_ConvertString("capNetAdmin_Speed") + ": " + NetClient_AdminSpeedText(fNetCliAdminSpeed) + "x");
	SetFormatedText("TEXT_ADM_WINDDIR", XI_ConvertString("capNetAdmin_WindDir") + ": " + iDegrees);
	SetFormatedText("TEXT_ADM_WINDPWR", XI_ConvertString("capNetAdmin_WindPower") + ": " + makeint(fNetCliAdminWindSpeed));
}

void NetMenu_OnKick() { NetMenu_Act(NET_ADMIN_KICK, true, 0.0); }
void NetMenu_OnMute() { NetMenu_Act(NET_ADMIN_MUTE, true, 0.0); }
void NetMenu_OnAdmGod() { NetMenu_Act(NET_ADMIN_GOD, true, 0.0); }		// COOP s27

// COOP s30: the game ends for everybody, the lobby opens (server\OnStart.c: NetServer_ReturnToLobby)
void NetMenu_OnAdmLobby()
{
	if (!bNetClientIsAdmin) { return; }
	NetClient_AdminSend(NET_ADMIN_TOLOBBY, DST_INVALID, 0.0);
}
void NetMenu_OnTpTo() { NetMenu_Act(NET_ADMIN_TP_TO, true, 0.0); }
void NetMenu_OnBring() { NetMenu_Act(NET_ADMIN_TP_BRING, true, 0.0); }
void NetMenu_OnMake() { NetMenu_Act(NET_ADMIN_MAKE_ADMIN, true, 0.0); }
void NetMenu_OnHeal() { NetMenu_Act(NET_ADMIN_HEAL, true, 0.0); }
void NetMenu_OnDestroy() { NetMenu_Act(NET_ADMIN_DESTROY, true, 0.0); }
void NetMenu_OnHealAll() { NetMenu_Act(NET_ADMIN_ALL_HEAL, false, 0.0); }
void NetMenu_OnReload() { NetMenu_Act(NET_ADMIN_RELOAD_SHIPS, false, 0.0); }
void NetMenu_OnWeatherDec() { NetMenu_Act(NET_ADMIN_WEATHER, false, -1.0); }
void NetMenu_OnWeatherInc() { NetMenu_Act(NET_ADMIN_WEATHER, false, 1.0); }
void NetMenu_OnTimeDec() { NetMenu_Act(NET_ADMIN_TIME, false, -1.0); }
void NetMenu_OnTimeInc() { NetMenu_Act(NET_ADMIN_TIME, false, 1.0); }
void NetMenu_OnSpeedDec() { NetMenu_Act(NET_ADMIN_SPEED, false, -1.0); }
void NetMenu_OnSpeedInc() { NetMenu_Act(NET_ADMIN_SPEED, false, 1.0); }
void NetMenu_OnWindAngDec() { NetMenu_Act(NET_ADMIN_WIND_ANGLE, false, -0.2618); }
void NetMenu_OnWindAngInc() { NetMenu_Act(NET_ADMIN_WIND_ANGLE, false, 0.2618); }
void NetMenu_OnWindSpdDec() { NetMenu_Act(NET_ADMIN_WIND_SPEED, false, -1.0); }
void NetMenu_OnWindSpdInc() { NetMenu_Act(NET_ADMIN_WIND_SPEED, false, 1.0); }

// ================= land duel loadout (COOP: net\Land.c) =================

void NetMenu_OpenLoadout()
{
	if (!NetLand_IsActive()) { return; }
	NetLand_LoSheathe(false);		// the sword goes away while the loadout is chosen
	NetLand_LoInit();
	NetMenu_Show("loadout");
	SetNodeUsing("PIC_LO_FACE", true);
	NetMenu_LoUpdate();
	SetCurrentNode("BTN_LO_APPLY");
}

void NetMenu_LoBack()
{
	NetMenu_Show("main");
}

void NetMenu_LoUpdate() { NetLand_UiLoUpdate(); }
void NetMenu_LoStep(int iRow, int iDir) { NetLand_UiLoStep(iRow, iDir); }
void NetMenu_LoPerk() { NetLand_UiLoPerk(); }

void NetMenu_LoApply()
{
	if (!bNetMenuLoadout) { return; }
	if (!NetLand_LoChangeAllowed())
	{
		Log_SetStringToLog("Снаряжение меняется только при включённом респавне");
		return;
	}
	NetLand_LoApply();
	NetLand_ProfSaveLo();
	Log_SetStringToLog("Снаряжение применено");
	IReturnToGame();
}

void NetMenu_Lo0Dec() { NetMenu_LoStep(0, -1); }
void NetMenu_Lo0Inc() { NetMenu_LoStep(0, 1); }
void NetMenu_Lo1Dec() { NetMenu_LoStep(1, -1); }
void NetMenu_Lo1Inc() { NetMenu_LoStep(1, 1); }
void NetMenu_Lo2Dec() { NetMenu_LoStep(2, -1); }
void NetMenu_Lo2Inc() { NetMenu_LoStep(2, 1); }
void NetMenu_Lo3Dec() { NetMenu_LoStep(3, -1); }
void NetMenu_Lo3Inc() { NetMenu_LoStep(3, 1); }
void NetMenu_Lo4Dec() { NetMenu_LoStep(4, -1); }
void NetMenu_Lo4Inc() { NetMenu_LoStep(4, 1); }
void NetMenu_Lo5Dec() { NetMenu_LoStep(5, -1); }
void NetMenu_Lo5Inc() { NetMenu_LoStep(5, 1); }
void NetMenu_Lo6Dec() { NetMenu_LoStep(6, -1); }
void NetMenu_Lo6Inc() { NetMenu_LoStep(6, 1); }
void NetMenu_Lo7Dec() { NetMenu_LoStep(7, -1); }
void NetMenu_Lo7Inc() { NetMenu_LoStep(7, 1); }
void NetMenu_Lo8Dec() { NetMenu_LoStep(8, -1); }
void NetMenu_Lo8Inc() { NetMenu_LoStep(8, 1); }
void NetMenu_Lo9Dec() { NetMenu_LoStep(9, -1); }
void NetMenu_Lo9Inc() { NetMenu_LoStep(9, 1); }
void NetMenu_Lo10Dec() { NetMenu_LoStep(10, -1); }
void NetMenu_Lo10Inc() { NetMenu_LoStep(10, 1); }
void NetMenu_Lo11Dec() { NetMenu_LoStep(11, -1); }
void NetMenu_Lo11Inc() { NetMenu_LoStep(11, 1); }
void NetMenu_Lo12Dec() { NetMenu_LoStep(12, -1); }
void NetMenu_Lo12Inc() { NetMenu_LoStep(12, 1); }
void NetMenu_Lo13Dec() { NetMenu_LoStep(13, -1); }
void NetMenu_Lo13Inc() { NetMenu_LoStep(13, 1); }
void NetMenu_Lo14Dec() { NetMenu_LoStep(14, -1); }
void NetMenu_Lo14Inc() { NetMenu_LoStep(14, 1); }
void NetMenu_Lo15Dec() { NetMenu_LoStep(15, -1); }
void NetMenu_Lo15Inc() { NetMenu_LoStep(15, 1); }
void NetMenu_Lo16Dec() { NetMenu_LoStep(16, -1); }
void NetMenu_Lo16Inc() { NetMenu_LoStep(16, 1); }
void NetMenu_Lo17Dec() { NetMenu_LoStep(17, -1); }
void NetMenu_Lo17Inc() { NetMenu_LoStep(17, 1); }
void NetMenu_Lo18Dec() { NetMenu_LoStep(18, -1); }
void NetMenu_Lo18Inc() { NetMenu_LoStep(18, 1); }
void NetMenu_Lo19Dec() { NetMenu_LoStep(19, -1); }
void NetMenu_Lo19Inc() { NetMenu_LoStep(19, 1); }
void NetMenu_Lo20Dec() { NetMenu_LoStep(20, -1); }
void NetMenu_Lo20Inc() { NetMenu_LoStep(20, 1); }
void NetMenu_Lo21Dec() { NetMenu_LoStep(21, -1); }
void NetMenu_Lo21Inc() { NetMenu_LoStep(21, 1); }
void NetMenu_Lo22Dec() { NetMenu_LoStep(22, -1); }		// COOP: the antidote
void NetMenu_Lo22Inc() { NetMenu_LoStep(22, 1); }
void NetMenu_Lo23Dec() { NetMenu_LoStep(23, -1); }		// COOP: the armor
void NetMenu_Lo23Inc() { NetMenu_LoStep(23, 1); }

// ================= bots (COOP: server\Bots.c) =================

// COOP s28: the teams a bot can join in this game (0 = no team game). Like server\Bots.c: NetServer_BotTeamsNum
int NetMenu_BotTeamsNum()
{
	if (NetLand_IsActive())
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

bool NetMenu_BotRelOk(int iRel)
{
	if (iRel >= NET_BOTREL_ALLY && iRel <= NET_BOTREL_NEUTRAL) { return true; }
	if (iRel < NET_BOTREL_TEAM0) { return false; }
	return (iRel - NET_BOTREL_TEAM0) < NetMenu_BotTeamsNum();
}

// the relation as the windows show it: "Союзник" / "Враг" / "Нейтрал" / "Команда: Красная"
string NetMenu_RelName(int iRel)
{
	if (iRel >= NET_BOTREL_TEAM0 && iRel < NET_BOTREL_TEAM0 + 4)
	{
		return "Команда: " + XI_ConvertString("nteam_" + NetTeamName[iRel - NET_BOTREL_TEAM0]);
	}
	return XI_ConvertString(NetMenu_RelKey(iRel));
}

string NetMenu_RelKey(int iRel)
{
	switch (iRel)
	{
		case NET_BOTREL_ALLY: return "capNetAdmin_RelAlly"; break;
		case NET_BOTREL_NEUTRAL: return "capNetAdmin_RelNeutral"; break;
	}
	return "capNetAdmin_RelEnemy";
}

// next ship of the list (like the buy screen: the ships that can be bought, without the fort)
int NetMenu_BotNextShip(int iFrom, int iDir)
{
	int iNum = Net_GetShipsNum();
	int iCur = iFrom;
	for (int k=0; k<iNum; k++)
	{
		iCur = iCur + iDir;
		if (iCur >= iNum) { iCur = 0; }
		if (iCur < 0) { iCur = iNum - 1; }
		ref rShip = Net_GetShipByIndex(iCur);
		if (rShip.Name == "FORT") { return iCur; }		// COOP: the fort is the last item of the list
		if (CheckAttribute(rShip, "TradeOff") && sti(rShip.TradeOff)) { continue; }
		return iCur;
	}
	if (iFrom < 0) { return 0; }
	return iFrom;
}

int NetMenu_BotNextCannon(int iFrom, int iDir)
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

void NetMenu_OpenBotAdd()
{
	if (!bNetClientIsAdmin) { return; }
	if (iNMBotShip < 0) { iNMBotShip = NetMenu_BotNextShip(-1, 1); }
	if (iNMBotCannon < 0) { iNMBotCannon = NetMenu_BotNextCannon(-1, 1); }
	NetMenu_Show("botadd");
	NetMenu_BotAddUpdate();
	SetCurrentNode("BTN_BOT_DOADD");
}

void NetMenu_BotAddUpdate()
{
	if (!bNetMenuBotAdd) { return; }
	if (NetLand_IsActive()) { NetMenu_BotAddUpdateLand(); return; }
	NetMenu_BotLandNodes(false);

	ref rShip = Net_GetShipByIndex(iNMBotShip);
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
		int iCannons = Net_GetShipCannonsNum(iNMBotShip);
		int iCrewNum = Net_GetCrewCount(iNMBotShip, iNMBotCrew);
		SetFormatedText("TEXT_BOT_SHIPINFO", XI_ConvertString("capNetAdmin_BotCannons") + ": " + iCannons + ", " + XI_ConvertString("capNetAdmin_BotCrewNum") + ": " + iCrewNum);
	}

	ref rCannon = Net_GetCannonByIndex(iNMBotCannon);
	Picture_SetGroupPicture("PICTURE_BOT_CANNON", "CANNONS_UPGRADE_1", rCannon.picture);
	SetFormatedText("TEXT_BOT_CANNON", XI_ConvertString(rCannon.Name) + " " + XI_ConvertString("caliber" + rCannon.caliber));

	SetFormatedText("TEXT_BOT_CREW", XI_ConvertString("capNetAdmin_BotCrew") + ": " + iNMBotCrew + "%");
	SetFormatedText("TEXT_BOT_HULL", XI_ConvertString("capNetAdmin_BotHull") + ": " + iNMBotHull + "%");
	SetFormatedText("TEXT_BOT_SAILS", XI_ConvertString("capNetAdmin_BotSails") + ": " + iNMBotSails + "%");
	SetFormatedText("TEXT_BOT_REL", XI_ConvertString("capNetAdmin_BotRel") + ": " + NetMenu_RelName(iNMBotRel));
}

// COOP: a land duel: the "ship" is the skin of the bot (the portraits like the icon of a player), the "cannon" its blade
void NetMenu_BotLandNodes(bool bLand)
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

void NetMenu_BotAddUpdateLand()
{
	if (iNMBotFace < 0) { iNMBotFace = NetLand_BotNextFace(-1, 1); }
	if (sNMBotBlade == "") { sNMBotBlade = NetLand_NextItem("", BLADE_ITEM_TYPE, 1, false); }
	SetNewPicture("PICTURE_BOT_SHIP", "interfaces\portraits\256\face_" + iNMBotFace + ".tga");
	SetFormatedText("TEXT_BOT_SHIP", "Скин: " + NetLand_FaceToModel(iNMBotFace));
	SetFormatedText("TEXT_BOT_SHIPINFO", "Стрелками выберите лицо и клинок бота");
	string sBladeName = sNMBotBlade;
	int iBl = GetItemIndex(sNMBotBlade);
	if (iBl >= 0)
	{
		SetNewGroupPicture("PICTURE_BOT_CANNON", Items[iBl].picTexture, "itm" + Items[iBl].picIndex);
		sBladeName = NetLand_ItemTitle(sNMBotBlade);
	}
	SetFormatedText("TEXT_BOT_CANNON", "Клинок: " + sBladeName);
	SetFormatedText("TEXT_BOT_CREW", "Уровень: " + iNMBotLevel);
	string sGunName = "нет";
	if (sNMBotGun != "") { sGunName = NetLand_ItemTitle(sNMBotGun); }
	SetFormatedText("TEXT_BOT_GUN", "Пистолет: " + sGunName);
	// the gun like the blade: its icon and arrows; the hull and the sails mean nothing here
	NetMenu_BotLandNodes(true);
	int iGn = -1;
	if (sNMBotGun != "") { iGn = GetItemIndex(sNMBotGun); }
	if (iGn >= 0) { SetNodeUsing("PICTURE_BOT_GUN", true); SetNewGroupPicture("PICTURE_BOT_GUN", Items[iGn].picTexture, "itm" + Items[iGn].picIndex); }
	else { SetNodeUsing("PICTURE_BOT_GUN", false); }
	SetFormatedText("TEXT_BOT_REL", XI_ConvertString("capNetAdmin_BotRel") + ": " + NetMenu_RelName(iNMBotRel));
}

int NetMenu_BotPercent(int iValue, int iDelta)
{
	iValue = iValue + iDelta;
	if (iValue < 10) { iValue = 10; }
	if (iValue > 100) { iValue = 100; }
	return iValue;
}

void NetMenu_OnBotShipPrev()
{
	if (NetLand_IsActive()) { iNMBotFace = NetLand_BotNextFace(iNMBotFace, -1); NetMenu_BotAddUpdate(); return; }
	iNMBotShip = NetMenu_BotNextShip(iNMBotShip, -1); NetMenu_BotAddUpdate();
}
void NetMenu_OnBotShipNext()
{
	if (NetLand_IsActive()) { iNMBotFace = NetLand_BotNextFace(iNMBotFace, 1); NetMenu_BotAddUpdate(); return; }
	iNMBotShip = NetMenu_BotNextShip(iNMBotShip, 1); NetMenu_BotAddUpdate();
}
void NetMenu_OnBotCannonPrev()
{
	if (NetLand_IsActive()) { sNMBotBlade = NetLand_NextItem(sNMBotBlade, BLADE_ITEM_TYPE, -1, false); NetMenu_BotAddUpdate(); return; }
	iNMBotCannon = NetMenu_BotNextCannon(iNMBotCannon, -1); NetMenu_BotAddUpdate();
}
void NetMenu_OnBotCannonNext()
{
	if (NetLand_IsActive()) { sNMBotBlade = NetLand_NextItem(sNMBotBlade, BLADE_ITEM_TYPE, 1, false); NetMenu_BotAddUpdate(); return; }
	iNMBotCannon = NetMenu_BotNextCannon(iNMBotCannon, 1); NetMenu_BotAddUpdate();
}
void NetMenu_OnBotCrewDec()
{
	if (NetLand_IsActive()) { iNMBotLevel = iNMBotLevel - 1; if (iNMBotLevel < 1) { iNMBotLevel = 50; } NetMenu_BotAddUpdate(); return; }
	iNMBotCrew = NetMenu_BotPercent(iNMBotCrew, -10); NetMenu_BotAddUpdate();
}
void NetMenu_OnBotCrewInc()
{
	if (NetLand_IsActive()) { iNMBotLevel = iNMBotLevel + 1; if (iNMBotLevel > 50) { iNMBotLevel = 1; } NetMenu_BotAddUpdate(); return; }
	iNMBotCrew = NetMenu_BotPercent(iNMBotCrew, 10); NetMenu_BotAddUpdate();
}
void NetMenu_OnBotHullDec()
{
	if (NetLand_IsActive()) { sNMBotGun = NetLand_NextItem(sNMBotGun, GUN_ITEM_TYPE, -1, true); NetMenu_BotAddUpdate(); return; }
	iNMBotHull = NetMenu_BotPercent(iNMBotHull, -10); NetMenu_BotAddUpdate();
}
void NetMenu_OnBotHullInc()
{
	if (NetLand_IsActive()) { sNMBotGun = NetLand_NextItem(sNMBotGun, GUN_ITEM_TYPE, 1, true); NetMenu_BotAddUpdate(); return; }
	iNMBotHull = NetMenu_BotPercent(iNMBotHull, 10); NetMenu_BotAddUpdate();
}
void NetMenu_OnBotSailsDec() { iNMBotSails = NetMenu_BotPercent(iNMBotSails, -10); NetMenu_BotAddUpdate(); }
void NetMenu_OnBotSailsInc() { iNMBotSails = NetMenu_BotPercent(iNMBotSails, 10); NetMenu_BotAddUpdate(); }

// COOP s28: ally, enemy, neutral and (in a team game) the teams of the game
void NetMenu_OnBotRelPrev()
{
	int iLast = NET_BOTREL_NEUTRAL;
	if (NetMenu_BotTeamsNum() > 0) { iLast = NET_BOTREL_TEAM0 + NetMenu_BotTeamsNum() - 1; }
	iNMBotRel = iNMBotRel - 1;
	if (iNMBotRel < NET_BOTREL_ALLY) { iNMBotRel = iLast; }
	if (!NetMenu_BotRelOk(iNMBotRel)) { iNMBotRel = NET_BOTREL_NEUTRAL; }
	NetMenu_BotAddUpdate();
}

void NetMenu_OnBotRelNext()
{
	iNMBotRel = iNMBotRel + 1;
	if (!NetMenu_BotRelOk(iNMBotRel)) { iNMBotRel = NET_BOTREL_ALLY; }
	NetMenu_BotAddUpdate();
}

// the bot appears 400 in front of our bow; "Бот N добавлен" comes from the server
void NetMenu_OnBotDoAdd()
{
	if (!bNetMenuBotAdd) { return; }
	if (NetLand_IsActive())
	{
		// the face goes as the ship, the blade (its index) as the hull and the sails
		int iBlIdx = GetItemIndex(sNMBotBlade);
		if (iBlIdx < 0) { iBlIdx = 0; }
		// the gun: its index + 1 (0: none), the low byte as the crew, the high part in the ship word above the face
		int iGunN = 0;
		if (sNMBotGun != "") { iGunN = GetItemIndex(sNMBotGun) + 1; }
		if (iGunN < 0) { iGunN = 0; }
		int iShipW = iNMBotFace + 512 * (iGunN / 256);
		NetClient_AdminBotAdd(iShipW, iNMBotLevel, iGunN - (iGunN / 256) * 256, iBlIdx - (iBlIdx / 256) * 256, iBlIdx / 256, iNMBotRel);
		return;
	}
	NetClient_AdminBotAdd(iNMBotShip, iNMBotCannon, iNMBotCrew, iNMBotHull, iNMBotSails, iNMBotRel);
}

// Esc / "Back" in the bot windows: back to the admin panel
void NetMenu_BotBack()
{
	NetMenu_OpenAdmin();
}

// the switch "Bot respawn": the server owns the state, the checkbox only shows it and sends the click
bool bNMBotOptSync = false;

void NetMenu_BotOptsUpdate()
{
	bNMBotOptSync = true;
	CheckButton_SetState("CHECKBUTTON_BOTRESPAWN", 1, bNetCliBotRespawn);
	CheckButton_SetState("CHECKBUTTON_ADM_LATEJOIN", 1, bNetCliLateJoin);		// COOP s30
	SetFormatedText("TEXT_BOTS_SKILL", "Боты: " + NetMenu_BotSkillName(NetLand_GetBotSkill()));
	bNMBotOptSync = false;
}

void NetMenu_CheckButtonChange()
{
	string sNode = GetEventData();
	int iButtonNumber = GetEventData();
	int bChecked = GetEventData();

	if (sNode == "CHECKBUTTON_BOTRESPAWN" && !bNMBotOptSync)
	{
		NetClient_AdminSend(NET_ADMIN_BOT_RESPAWN, DST_INVALID, makefloat(bChecked));
	}
	if (sNode == "CHECKBUTTON_ADM_LATEJOIN" && !bNMBotOptSync)		// COOP s30
	{
		NetClient_AdminSend(NET_ADMIN_LATEJOIN, DST_INVALID, makefloat(bChecked));
	}
	// COOP: the poison on the blade (the loadout)
	NetLand_UiLoCheck(sNode, bChecked);		// COOP: the poison on the blade (the loadout)
}

// COOP s27: the noclip of the admin (client\Admin.c): on and off
void NetMenu_OnAdmNoclip()
{
	if (!bNetClientIsAdmin) { return; }
	NetNoclip_Toggle();
}

// COOP s27: the free camera of the admin (client\Admin.c): on and off
void NetMenu_OnAdmFreecam()
{
	if (!bNetClientIsAdmin) { return; }
	NetFreecam_Toggle();
}

void NetMenu_OpenBots()
{
	if (!bNetClientIsAdmin) { return; }
	NetMenu_Show("bots");
	NetMenu_FillBots();
	NetMenu_BotOptsUpdate();
	SetCurrentNode("TABLE_BOTS");

	iNMBotsTimerGen++;
	PostEvent("NetMenu_BotsTimer", 1500, "l", iNMBotsTimerGen);
}

// the hull of the bots changes all the time: the list is filled again while it is open
void NetMenu_BotsTimer()
{
	int iGen = GetEventData();
	if (iGen != iNMBotsTimerGen) { return; }
	if (!bNetMenuBots) { return; }
	NetMenu_FillBots();
	PostEvent("NetMenu_BotsTimer", 1500, "l", iGen);
}

void NetMenu_FillBots()
{
	if (!bNetMenuBots) { return; }

	int i;
	int iPct = 0;
	int iSelected = 0;
	if (CheckAttribute(&GameInterface, "TABLE_BOTS.select")) { iSelected = sti(GameInterface.TABLE_BOTS.select); }

	Table_Clear("TABLE_BOTS", false, true, false);

	GameInterface.TABLE_BOTS.hr.td1.str = XI_ConvertString("capNetAdmin_ColBot");
	GameInterface.TABLE_BOTS.hr.td1.scale = 0.85;
	GameInterface.TABLE_BOTS.hr.td2.str = XI_ConvertString("capNetAdmin_ColRel");
	GameInterface.TABLE_BOTS.hr.td2.scale = 0.85;
	GameInterface.TABLE_BOTS.hr.td3.str = XI_ConvertString("capNetAdmin_ColHull");
	GameInterface.TABLE_BOTS.hr.td3.scale = 0.85;

	int iRow = 0;
	for (i=NET_BOT_FIRST; i<NET_MAXCLIENTS; i++)
	{
		if (!NetClient_IsBotID(i)) { continue; }
		if (NetClient_BotRemoved(i)) { continue; }

		ref rC = NetClient_GetClient(i);
		iRow++;
		string sRow = "tr" + iRow;

		string sName = rC.NickName;
		if (CheckAttribute(rC, "Ship.Name") && !CheckAttribute(rC, "LandBot")) { sName = sName + " - " + rC.Ship.Name; }

		string sHull = "";
		if (CheckAttribute(rC, "LandBot"))
		{
			// a land bot: its health (the owner sends it with its state)
			if (CheckAttribute(rC, "LandBotDead") && sti(rC.LandBotDead)) { sHull = "мёртв"; }
			else
			{
				if (CheckAttribute(rC, "LandBotHP")) { sHull = rC.LandBotHP + " HP"; }
			}
			sName = sName + " (" + NetLand_BotModel(i) + ")";
		}
		else
		{
		if (CheckAttribute(rC, "Dead") && sti(rC.Dead))
		{
			sHull = XI_ConvertString("capNetAdmin_BotSunk");
		}
		else
		{
			if (CheckAttribute(rC, "Ship.HP") && CheckAttribute(rC, "Ship.MaxHP") && stf(rC.Ship.MaxHP) > 0.0)
			{
				iPct = makeint(100.0 * stf(rC.Ship.HP) / stf(rC.Ship.MaxHP) + 0.5);
				sHull = "" + iPct + "%";
			}
		}
		}

		int iRel = NET_BOTREL_ENEMY;
		if (CheckAttribute(rC, "BotRel")) { iRel = sti(rC.BotRel); }

		GameInterface.TABLE_BOTS.(sRow).ClientID = i;
		GameInterface.TABLE_BOTS.(sRow).td1.str = sName;
		GameInterface.TABLE_BOTS.(sRow).td2.str = NetMenu_RelName(iRel);
		GameInterface.TABLE_BOTS.(sRow).td3.str = sHull;
	}

	// COOP: the fort of Bot+ is the last row, its relation can be changed like a bot's
	ref rFortRow = NetClient_GetClient(NET_FORTCLIENT);
	if (IsEntity(rFortRow) && CheckAttribute(rFortRow, "CoopRel") && sti(rFortRow.CoopRel) > 0)
	{
		iRow++;
		string sFRow = "tr" + iRow;
		string sFHull = "";
		if (CheckAttribute(rFortRow, "SpyGlass.CannonsTotal") && sti(rFortRow.SpyGlass.CannonsTotal) > 0)
		{
			int iFLeft = sti(rFortRow.SpyGlass.CannonsTotal) - sti(rFortRow.SpyGlass.CannonsDamaged);
			sFHull = "" + makeint(100.0 * iFLeft / sti(rFortRow.SpyGlass.CannonsTotal) + 0.5) + "%";
		}
		GameInterface.TABLE_BOTS.(sFRow).ClientID = NET_FORTCLIENT;
		GameInterface.TABLE_BOTS.(sFRow).td1.str = XI_ConvertString("Fort");
		GameInterface.TABLE_BOTS.(sFRow).td2.str = NetMenu_RelName(sti(rFortRow.CoopRel));
		GameInterface.TABLE_BOTS.(sFRow).td3.str = sFHull;
	}

	if (iRow > 0)
	{
		if (iSelected < 1 || iSelected > iRow) { iSelected = 1; }
		GameInterface.TABLE_BOTS.select = iSelected;
	}

	Table_UpdateWindow("TABLE_BOTS");
}

// client id of the bot in the selected row or DST_INVALID
int NetMenu_GetSelectedBot()
{
	if (!CheckAttribute(&GameInterface, "TABLE_BOTS.select")) { return DST_INVALID; }
	int iSelected = sti(GameInterface.TABLE_BOTS.select);
	if (iSelected < 1) { return DST_INVALID; }

	string sRow = "tr" + iSelected;
	if (!CheckAttribute(&GameInterface, "TABLE_BOTS." + sRow + ".ClientID")) { return DST_INVALID; }

	return sti(GameInterface.TABLE_BOTS.(sRow).ClientID);
}

void NetMenu_BotAct(int iAction, float fValue)
{
	int wBot = NetMenu_GetSelectedBot();
	if (wBot == DST_INVALID) { return; }
	NetClient_AdminSend(iAction, wBot, fValue);
}

void NetMenu_OnBotsAlly() { NetMenu_BotAct(NET_ADMIN_BOT_REL, makefloat(NET_BOTREL_ALLY)); }
void NetMenu_OnBotsEnemy() { NetMenu_BotAct(NET_ADMIN_BOT_REL, makefloat(NET_BOTREL_ENEMY)); }
void NetMenu_OnBotsNeutral() { NetMenu_BotAct(NET_ADMIN_BOT_REL, makefloat(NET_BOTREL_NEUTRAL)); }
// COOP s28: the selected bot goes to the next team of the game (a bot without a team: to the first one)
void NetMenu_OnBotsTeam()
{
	int iTeams = NetMenu_BotTeamsNum();
	if (iTeams <= 0) { return; }
	int wBot = NetMenu_GetSelectedBot();
	if (wBot == DST_INVALID) { return; }
	int iRelNow = NET_BOTREL_ENEMY;
	if (wBot == NET_FORTCLIENT)
	{
		if (CheckAttribute(&NCClients[NET_FORTCLIENT], "CoopRel")) { iRelNow = sti(NCClients[NET_FORTCLIENT].CoopRel); }
	}
	else
	{
		if (CheckAttribute(&NCClients[wBot], "BotRel")) { iRelNow = sti(NCClients[wBot].BotRel); }
	}
	int iTeamNext = 0;
	if (iRelNow >= NET_BOTREL_TEAM0) { iTeamNext = iRelNow - NET_BOTREL_TEAM0 + 1; }
	if (iTeamNext >= iTeams) { iTeamNext = 0; }
	NetClient_AdminSend(NET_ADMIN_BOT_REL, wBot, makefloat(NET_BOTREL_TEAM0 + iTeamNext));
}
void NetMenu_OnBotsBring() { NetMenu_BotAct(NET_ADMIN_BOT_BRING, 0.0); }
void NetMenu_OnBotsGoTo() { NetMenu_BotAct(NET_ADMIN_BOT_GOTO, 0.0); }
void NetMenu_OnBotsHeal() { NetMenu_BotAct(NET_ADMIN_BOT_HEAL, 0.0); }
void NetMenu_OnBotsDestroy() { NetMenu_BotAct(NET_ADMIN_BOT_DESTROY, 0.0); }
void NetMenu_OnBotsDelete() { NetMenu_BotAct(NET_ADMIN_BOT_DELETE, 0.0); }
// COOP: the difficulty of all the bots (the server applies it and sends it back: NetMenu_BotOptsUpdate)
void NetMenu_OnBotsSkillDec()
{
	if (NetLand_GetBotSkill() <= 0) { return; }
	NetClient_AdminSend(NET_ADMIN_BOT_SKILL, DST_INVALID, makefloat(NetLand_GetBotSkill() - 1));
}
void NetMenu_OnBotsSkillInc()
{
	if (NetLand_GetBotSkill() >= 3) { return; }
	NetClient_AdminSend(NET_ADMIN_BOT_SKILL, DST_INVALID, makefloat(NetLand_GetBotSkill() + 1));
}
string NetMenu_BotSkillName(int iS)
{
	switch (iS)
	{
		case 0: return "безобидные"; break;
		case 1: return "лёгкие"; break;
		case 2: return "средние"; break;
		case 3: return "тяжёлые"; break;
	}
	return "средние";
}


