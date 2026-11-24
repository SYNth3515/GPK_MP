void NetClient_OnServerInfo(int iMsg)
{
	int iSubCode = NMGetByte(iMsg);

	switch (iSubCode)
	{
		case NSC_SERVERINFO_COMMON:				NetClient_OnServerInfo_Common(iMsg);			break;
		case NSC_SERVERINFO_PLAYER:				NetClient_OnServerInfo_Player(iMsg);			break;
		case NSC_SERVERINFO_SHIP:				NetClient_OnServerInfo_Ship(iMsg);				break;
		case NSC_SERVERINFO_CANNONS:			NetClient_OnServerInfo_Cannons(iMsg);			break;
		case NSC_SERVERINFO_SKILLS:				NetClient_OnServerInfo_Skills(iMsg);			break;
		case NSC_SERVERINFO_PERKS:				NetClient_OnServerInfo_Perks(iMsg);				break;
		case NSC_SERVERINFO_GOODS:				NetClient_OnServerInfo_Goods(iMsg);				break;
		case NSC_SERVERINFO_TOPLIST_PLAYER:		NetClient_OnServerInfo_TopListPlayer(iMsg);		break;
		case NSC_SERVERINFO_DETAILS:			NetClient_OnServerInfo_Details(iMsg);			return; break;		// COOP s27
		case NSC_SERVERINFO_CHANGED:			Event("NCEvent_ServerChanged");					return; break;		// COOP s30
	}

	Event("NetClient_OnServerInfo_Event", "l", iSubCode);
}

void NetClient_OnServerInfo_Common(int iMsg)
{
	NCServer.NumClients = NMGetByte(iMsg);
	NCServer.MaxClients = NMGetByte(iMsg);
	NCServer.GameType = NMGetByte(iMsg);
	NCServer.NumTeams = NMGetByte(iMsg);
	NCServer.MaxShipClass = NMGetByte(iMsg);
	NCServer.Credit = NMGetDword(iMsg);
	NCServer.GameSpeed = NMGetByte(iMsg);
	NCServer.WeatherID = NMGetString(iMsg);
	NCServer.IslandID = NMGetString(iMsg);
	NetDl_CliMapChanged(NCServer.IslandID);		// COOP s47: the host chose a downloaded map in the lobby: it is downloaded now
	// COOP: game state and switches of the server
	NCServer.GameRunning = NMGetByte(iMsg);
	NCServer.Respawn = NMGetByte(iMsg);
	NCServer.NoBoarding = NMGetByte(iMsg);
	NCServer.PassivePerks = NMGetByte(iMsg);
	NCServer.ActivePerks = NMGetByte(iMsg);
	NCServer.FastBoarding = NMGetByte(iMsg);		// COOP s37
}

void NetClient_OnServerInfo_Player(int iMsg)
{
	int wSrvClientID = NMGetClientID(iMsg);
	ref rClient = NetClient_GetClient(wSrvClientID);
	rClient.NickName = NMGetString(iMsg);
	rClient.Ship.Name = NMGetString(iMsg);
	rClient.UserData.Face = NMGetString(iMsg);
	rClient.UserData.Flag = NMGetString(iMsg);
	rClient.UserData.Sail = NMGetString(iMsg);
	rClient.Ship.Type = NMGetDword(iMsg);
	rClient.Team = NMGetByte(iMsg);
}

void NetClient_OnServerInfo_Ship(int iMsg)
{
	int i = NMGetWord(iMsg);
	
	ref rShip = Net_GetShipByIndex(i);

	rShip.Cost = NMGetDword(iMsg);
	rShip.Upgrades.Hull1 = NMGetDword(iMsg);
	rShip.Upgrades.Hull2 = NMGetDword(iMsg);
	rShip.Upgrades.Hull3 = NMGetDword(iMsg);
	rShip.Upgrades.Rigging1 = NMGetDword(iMsg);
	rShip.Upgrades.Rigging2 = NMGetDword(iMsg);
	rShip.Upgrades.Rigging3 = NMGetDword(iMsg);

	rShip.MaxCrew = NMGetWord(iMsg);
	rShip.MinCrew = NMGetWord(iMsg);
	rShip.SpeedRate = NMGetFloat(iMsg);
	rShip.TurnRate = NMGetFloat(iMsg);
	rShip.HP = NMGetFloat(iMsg);
	rShip.SP = NMGetFloat(iMsg);
}

void NetClient_OnServerInfo_Cannons(int iMsg)
{
	int iNumCannons = NMGetWord(iMsg);

	for (int i=0; i<iNumCannons; i++)
	{
		ref rCannon = Net_GetCannonByIndex(i);
		rCannon.Cost = NMGetWord(iMsg);
		rCannon.Upgrade1 = NMGetWord(iMsg);
		rCannon.Upgrade2 = NMGetWord(iMsg);
		rCannon.Upgrade3 = NMGetWord(iMsg);
	}
}

void NetClient_OnServerInfo_Skills(int iMsg)
{
	int iNumSkills = NMGetWord(iMsg);

	for (int i=0; i<iNumSkills; i++)
	{
		ref rSkill = Net_GetSkillByIndex(i);
		rSkill.Cost = NMGetDword(iMsg);
	}
}

void NetClient_OnServerInfo_Perks(int iMsg)
{
	int iNumPerks = NMGetWord(iMsg);

	for (int i=0; i<iNumPerks; i++)
	{
		ref rPerk = Net_GetPerkByIndex(i);
		rPerk.Cost = NMGetDword(iMsg);
	}
}

void NetClient_OnServerInfo_Goods(int iMsg)
{
	int iNumGoods = NMGetWord(iMsg);

	for (int i=0; i<iNumGoods; i++)
	{
		ref rGood = Net_GetGoodByIndex(i);
		rGood.Unit = NMGetWord(iMsg);
		rGood.Cost = NMGetDword(iMsg);
	}
}

void NetClient_OnServerInfo_TopListPlayer(int iMsg)
{
	int iNumPlayers = NMGetWord(iMsg);
	
	if (!iNumPlayers)
	{
		Event("NetClient_OnServerInfo_TopListEnd_Event");
		return;
	}

	Event("NetClient_OnServerInfo_TopListPlayer_Event");
}

void NetClient_OnServerInfo_Details(int iMsg)
{
	string sDetails = NMGetString(iMsg);
	Event("NCEvent_SrvDetails", "s", sDetails);
}

// ---------- COOP s27: the window "Подробно" (the list of the servers, the lobby): the parameters of the server ----------
// field iIdx (0 = "GPKI") of the line of server\ServerInfo.c: Net_DetailsCode
string Net_DetField(string sCode, int iIdx)
{
	int iPos = 0;
	int iLen = strlen(sCode);
	for (int i=0; i<iIdx; i++)
	{
		int iBar = findSubStr(sCode, "|", iPos);
		if (iBar < 0) { return ""; }
		iPos = iBar + 1;
	}
	if (iPos >= iLen) { return ""; }
	int iEnd = findSubStr(sCode, "|", iPos);
	if (iEnd < 0) { iEnd = iLen; }
	if (iEnd <= iPos) { return ""; }
	return strcut(sCode, iPos, iEnd - 1);
}

int Net_DetInt(string sCode, int iIdx)
{
	string sF = Net_DetField(sCode, iIdx);
	if (sF == "") { return 0; }
	return sti(sF);
}

// the time of day and the weather of the id of the weather (the table of the Create game screen)
string Net_DetWeather(string sId, int iTornado)
{
	for (int iT=0; iT<4; iT++)
	{
		for (int iW=0; iW<4; iW++)
		{
			if (NetServer_AdminWeatherID(iT, iW) == sId)
			{
				string sTime = "txtCS_Morning";
				if (iT == 1) { sTime = "txtCS_Day"; }
				if (iT == 2) { sTime = "txtCS_Evening"; }
				if (iT == 3) { sTime = "txtCS_Night"; }
				string sWeather = "txtCS_Calm";
				if (iW == 1) { sWeather = "txtCS_Windy"; }
				if (iW == 2) { sWeather = "txtCS_Heavy"; }
				if (iW == 3) { sWeather = "txtCS_Storm"; }
				if (iW == 3 && iTornado != 0) { sWeather = "txtCS_Tornado"; }
				return XI_ConvertString(sTime) + ", " + XI_ConvertString(sWeather);
			}
		}
	}
	return sId;
}

// the text of the window: the first line is set, the others are added
void Net_DetLine(string sNode, string sLine, bool bFirst)
{
	if (bFirst) { SetFormatedText(sNode, sLine); }
	else { AddLineToFormatedText(sNode, sLine); }
}

void Net_DetailsShow(string sNode, string sCode)
{
	if (Net_DetField(sCode, 0) != "GPKI")
	{
		Net_DetLine(sNode, "Сервер старой версии: его параметры не узнать", true);
		return;
	}
	int iType = Net_DetInt(sCode, 1);
	string sLand = Net_DetField(sCode, 2);
	string sIsland = Net_DetField(sCode, 3);
	bool bLand = false;
	if (sLand != "") { bLand = true; }
	int iTeams = Net_DetInt(sCode, 4);

	string sType = XI_ConvertString("txtCS_LandDuel");
	if (!bLand) { sType = XI_ConvertString("txtCS_" + NetModes[iType].Name); }
	Net_DetLine(sNode, "Тип игры: " + sType, true);

	string sMap = XI_ConvertString("OpenSea");
	if (sIsland != "") { sMap = Net_GetIslandName(sIsland); }
	if (bLand) { sMap = NetLand_LocName(sLand); }
	Net_DetLine(sNode, "Карта: " + sMap, false);

	string sState = "ожидание в лобби";
	if (Net_DetInt(sCode, 20) != 0) { sState = "идёт игра"; }
	Net_DetLine(sNode, "Игроки: " + Net_DetInt(sCode, 18) + " / " + Net_DetInt(sCode, 19) + " (" + sState + ")", false);

	string sTeams = "все против всех";
	if (iTeams >= 2) { sTeams = "" + iTeams; }
	Net_DetLine(sNode, "Кол-во команд: " + sTeams, false);

	float fSpeed = makefloat(Net_DetInt(sCode, 5)) / 100.0;
	Net_DetLine(sNode, "Скорость игры: " + NetClient_AdminSpeedText(fSpeed) + "x", false);

	int iTime = Net_DetInt(sCode, 6);
	string sTime = "нет";
	if (iTime > 0) { sTime = "" + iTime + " мин."; }
	Net_DetLine(sNode, "Время раунда: " + sTime, false);

	if (Net_DetInt(sCode, 7) != 0) { Net_DetLine(sNode, "Респаун: " + Net_DetInt(sCode, 8) + " сек.", false); }

	Net_DetLine(sNode, "Время суток, погода: " + Net_DetWeather(Net_DetField(sCode, 9), Net_DetInt(sCode, 10)), false);

	if (Net_DetInt(sCode, 11) != 0)
	{
		if (bLand) { Net_DetLine(sNode, XI_ConvertString("capNetCS_MeleeOnly"), false); }
		else { Net_DetLine(sNode, XI_ConvertString("capNetCS_NoBoarding"), false); }
	}
	else
	{
		// COOP s37: field 21 (an older server has none: the fast boarding)
		if (!bLand)
		{
			if (Net_DetField(sCode, 21) == "0") { Net_DetLine(sNode, XI_ConvertString("capNetCS_FastBoarding") + ": выкл (абордаж на палубе)", false); }
			else { Net_DetLine(sNode, XI_ConvertString("capNetCS_FastBoarding") + ": вкл", false); }
		}
	}
	// COOP s38: fields 22..24 (an older server has none)
	if (Net_DetField(sCode, 22) != "")
	{
		string sOnOff = "выкл";
		if (Net_DetInt(sCode, 22) != 0) { sOnOff = "вкл"; }
		Net_DetLine(sNode, XI_ConvertString("capNetCS_PistolReload") + ": " + sOnOff, false);
		sOnOff = "запрещено";
		if (Net_DetInt(sCode, 23) != 0) { sOnOff = "разрешено"; }
		Net_DetLine(sNode, XI_ConvertString("capNetCS_LateJoin") + ": " + sOnOff, false);
		sOnOff = "выкл";
		if (Net_DetInt(sCode, 24) != 0) { sOnOff = "вкл"; }
		Net_DetLine(sNode, XI_ConvertString("capNetAdmin_BotRespawn") + ": " + sOnOff, false);
	}
	if (Net_DetInt(sCode, 12) != 0) { Net_DetLine(sNode, XI_ConvertString("capNetCS_PassivePerks"), false); }
	if (Net_DetInt(sCode, 13) != 0) { Net_DetLine(sNode, XI_ConvertString("capNetCS_ActivePerks"), false); }

	Net_DetLine(sNode, XI_ConvertString("capNetCS_BotSkill") + " " + XI_ConvertString("txtCS_BotSkill" + Net_DetInt(sCode, 14)), false);
	Net_DetLine(sNode, XI_ConvertString("capNetCS_StartPosition") + " " + XI_ConvertString("txtNetCS_StartPosition" + Net_DetInt(sCode, 15)), false);
	Net_DetLine(sNode, XI_ConvertString("capNetCS_Credit") + " " + Net_DetInt(sCode, 16), false);
	if (!bLand) { Net_DetLine(sNode, XI_ConvertString("capNetCS_MaxShipClass") + " " + Net_DetInt(sCode, 17), false); }
}
