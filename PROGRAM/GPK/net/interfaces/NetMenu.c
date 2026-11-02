// COOP: Esc menu of the running game: return / admin menu / leave the game.
// The admin menu: login window (admin password) and the admin panel.

bool bNetMenuLogin = false;
bool bNetMenuAdmin = false;

void InitInterface(string iniName)
{
	GameInterface.title = "";

	SendMessage(&GameInterface, "ls", MSG_INTERFACE_INIT, iniName);

	SetEventHandler("OnNetMenuDisconnect", "ICancelProcedure", 0);
	SetEventHandler("OnNetMenuReturn", "IReturnToGame", 0);
	SetEventHandler("exitCancel", "NetMenu_Cancel", 0);

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
	FreezeGroupControls("BattleInterfaceControls", true);

	NetMenu_Show("main");
}

void IDoExit(int exitCode)
{
	DelEventHandler("OnNetMenuDisconnect", "ICancelProcedure");
	DelEventHandler("OnNetMenuReturn", "IReturnToGame");
	DelEventHandler("exitCancel", "NetMenu_Cancel");

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
	interfaceResultCommand = exitCode;
	EndCancelInterface(true);
}

void ICancelProcedure()
{
	// COOP: leaving the running game from the in-game net menu also goes to the main menu (see EndGame.c)
	Net_Stop();

	IDoExit( RC_INTERFACE_NET_EXIT );

	ReturnToMainMenu();
}

void IReturnToGame()
{
	IDoExit( INTERFACE_NET_NETMENU );
}

// Esc: from the admin windows back to the menu, from the menu back to the game
void NetMenu_Cancel()
{
	if (bNetMenuLogin || bNetMenuAdmin) { NetMenu_Back(); return; }

	IReturnToGame();
}

// sWindow: "main", "login" or "admin"
void NetMenu_Show(string sWindow)
{
	bNetMenuLogin = (sWindow == "login");
	bNetMenuAdmin = (sWindow == "admin");

	XI_WindowShow("WND_NETMENU", sWindow == "main");
	XI_WindowDisable("WND_NETMENU", sWindow != "main");

	XI_WindowShow("WND_ADM_LOGIN", bNetMenuLogin);
	XI_WindowDisable("WND_ADM_LOGIN", !bNetMenuLogin);

	XI_WindowShow("WND_ADM", bNetMenuAdmin);
	XI_WindowDisable("WND_ADM", !bNetMenuAdmin);

	if (sWindow == "main") { SetCurrentNode("BTN_ENDGAME_RETURN"); }
}

void NetMenu_Back()
{
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
	SetFormatedText("TEXT_ADM_SPEED", XI_ConvertString("capNetAdmin_Speed") + ": " + iNetCliAdminSpeed + "x");
	SetFormatedText("TEXT_ADM_WINDDIR", XI_ConvertString("capNetAdmin_WindDir") + ": " + iDegrees);
	SetFormatedText("TEXT_ADM_WINDPWR", XI_ConvertString("capNetAdmin_WindPower") + ": " + makeint(fNetCliAdminWindSpeed));
}

void NetMenu_OnKick() { NetMenu_Act(NET_ADMIN_KICK, true, 0.0); }
void NetMenu_OnMute() { NetMenu_Act(NET_ADMIN_MUTE, true, 0.0); }
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

