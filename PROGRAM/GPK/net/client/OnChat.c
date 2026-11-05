int rgbGameMessage = 14610157;

// COOP: lobby chat. The native NetSay entity is only meant for the battle, so while the lobby (Buy interface) is open
// the messages are stored here and shown by the lobby itself. NCEvent_LobbyChat tells the lobby that a line was added.
object NCLobbyChat;
bool bNetLobbyChatActive = false;

// COOP: full chat history for the Y window. NCSay forgets a line after NCSay.Time seconds, so every line
// (players, system lines, game messages) is also stored here. Cleared on connect (NetClient_CreateChatEnvironment).
object NCChatHist;
int iNetChatHistMax = 300;	// how many lines are kept, the oldest ones are dropped
int iNetChatHistPage = 16;	// how many lines the Y window shows at once

void NetClient_ChatHistoryClear()
{
	DeleteAttribute(&NCChatHist, "");
	NCChatHist.Num = 0;
	NCChatHist.First = 0;
}

void NetClient_ChatHistoryAdd(string sLine)
{
	if (sLine == "") { return; }
	if (!CheckAttribute(&NCChatHist, "Num")) { NetClient_ChatHistoryClear(); }

	int iNum = sti(NCChatHist.Num);
	string sAttr = "l" + iNum;
	NCChatHist.Lines.(sAttr) = sLine;
	NCChatHist.Num = iNum + 1;

	while (sti(NCChatHist.Num) - sti(NCChatHist.First) > iNetChatHistMax)
	{
		string sOld = "l" + sti(NCChatHist.First);
		DeleteAttribute(&NCChatHist, "Lines." + sOld);
		NCChatHist.First = sti(NCChatHist.First) + 1;
	}

	Event("NCEvent_ChatHistory");
}

void NetClient_LobbyChatClear()
{
	DeleteAttribute(&NCLobbyChat, "");
	NCLobbyChat.Num = 0;
}

void NetClient_LobbyChatAdd(string sLine)
{
	if (!CheckAttribute(&NCLobbyChat, "Num")) { NetClient_LobbyChatClear(); }

	int iNum = sti(NCLobbyChat.Num);
	string sAttr = "l" + iNum;
	NCLobbyChat.Lines.(sAttr) = sLine;
	NCLobbyChat.Num = iNum + 1;

	Event("NCEvent_LobbyChat");
}

void NetClient_DeleteChatEnvironment()
{
	bNetLobbyChatActive = false;
	DeleteClass(&NCSay);

	DelEventHandler("Control Activation", "NetClient_ChatControlActivation");
}

void NetClient_CreateChatEnvironment()
{
	bNetLobbyChatActive = false;
	NetClient_LobbyChatClear();
	NetClient_ChatHistoryClear();

	CreateEntity(&NCSay, "NetSay");
	LayerAddObject("net_execute", &NCSay, -1);
	LayerAddObject("net_realize", &NCSay, 65535);

	NCSay.FontScale = 1.0;
	NCSay.StartY = 0.93;
	NCSay.ChatLines = 11;

	SetEventHandler("Control Activation", "NetClient_ChatControlActivation", 0);
}

void NetClient_OnSay(int iMsg)
{
	int iSubCode = NMGetByte(iMsg);
	int wNetClientID = NMGetClientID(iMsg);
	string sMessage = NMGetString(iMsg);

	ref rClient = NetClient_GetClient(wNetClientID);

	// COOP: sound of an incoming chat message (lobby and battle)
	PlayStereoSound(NET_SOUND_CHAT);

	// COOP: remember the line for the Y history window
	string sHistPrefix = "";
	if (iSubCode == NSC_SAY_TEAM) { sHistPrefix = "[команда] "; }
	if (iSubCode == NSC_SAY_PERSON) { sHistPrefix = "[лично] "; }
	string sHistNick = rClient.NickName;
	NetClient_ChatHistoryAdd(sHistPrefix + sHistNick + ": " + sMessage);

	// COOP: in the lobby the message goes to the lobby chat window, not to the battle chat entity
	if (bNetLobbyChatActive)
	{
		string sLobbyNick = rClient.NickName;
		NetClient_LobbyChatAdd(sLobbyNick + ": " + sMessage);
		return;
	}

	NCSay.SenderName = rClient.NickName;
	NCSay.Time = 30.0;

	switch (iSubCode)
	{
		case NSC_SAY_CHAT:
			NCSay.Color = ARGB(0, 251, 251, 251);
			NCSay.ChatMessage = sMessage;
		break;
		case NSC_SAY_PERSON:
			NCSay.Color = ARGB(0, 234, 255, 0);
			NCSay.PersonMessage = sMessage;
		break;
		case NSC_SAY_TEAM:
			NCSay.Color = ARGB(0, 121, 122, 255);
			NCSay.TeamMessage = sMessage;
		break;
	}
}

void NetClient_ChatAddGameMessage(string sMessage, int dwColor, int iMode, int wID1, int wID2)
{
	object oRes;
	string sParseString = LanguageConvertString(iLangNetClient, sMessage);

	oRes.Str = "";

	ref rClient1 = NetClient_GetClient(wID1);
	string sNick1 = rClient1.NickName;
	string sShip1 = rClient1.Ship.Name;

	if (iMode == 1)
	{
		Event(PARSE_STRING, "aslss", &oRes, sParseString, 2, sNick1, sShip1);
	}
	else
	{
		ref rClient2 = NetClient_GetClient(wID2);
		string sNick2 = rClient2.NickName;
		string sShip2 = rClient2.Ship.Name;

		Event(PARSE_STRING, "aslssss", &oRes, sParseString, 4, sNick1, sShip1, sNick2, sShip2);
	}

	NCSay.Color = dwColor;
	NCSay.Time = 30.0;
	NCSay.GameMessage = oRes.Str;
	NetClient_ChatHistoryAdd(oRes.Str);
}

// Same as NetClient_ChatAddGameMessage, but with two numbers. Text parameters:
// %1 nick and %2 ship of wID1, %3 nick and %4 ship of wID2, %5 = iNum1, %6 = iNum2
void NetClient_ChatAddGameMessageNum(string sMessage, int dwColor, int wID1, int wID2, int iNum1, int iNum2)
{
	object oRes;
	string sParseString = LanguageConvertString(iLangNetClient, sMessage);

	oRes.Str = "";

	ref rClient1 = NetClient_GetClient(wID1);
	ref rClient2 = NetClient_GetClient(wID2);
	string sNick1 = rClient1.NickName;
	string sShip1 = rClient1.Ship.Name;
	string sNick2 = rClient2.NickName;
	string sShip2 = rClient2.Ship.Name;
	string sNum1 = "" + iNum1;
	string sNum2 = "" + iNum2;

	Event(PARSE_STRING, "aslssssss", &oRes, sParseString, 6, sNick1, sShip1, sNick2, sShip2, sNum1, sNum2);

	NCSay.Color = dwColor;
	NCSay.Time = 30.0;
	NCSay.GameMessage = oRes.Str;
	NetClient_ChatHistoryAdd(oRes.Str);
}

void NetClient_ChatControlActivation()
{
	string sControl = GetEventData();

	int iSMsg;
	switch (sControl)
	{
		/*case "Net_RestartGame":
			iSMsg = NMCreate();
			NMAddClientID(iSMsg, wClientID);
			NMAddByte(iSMsg, NC_COMMAND);
			NMAddByte(iSMsg, NSC_COMMAND_RESTARTGAME);
			NetClient_SendMessage(iSMsg, true);
			NMDelete(iSMsg);
		break;
		case "Net_StartGame":
			iSMsg = NMCreate();
			NMAddClientID(iSMsg, wClientID);
			NMAddByte(iSMsg, NC_COMMAND);
			NMAddByte(iSMsg, NSC_COMMAND_STARTGAME);
			NetClient_SendMessage(iSMsg, true);
			NMDelete(iSMsg);
		break;*/
		case "Say":
			NCSay.Mode = "Say";
			LaunchNetChatString();
		break;
		case "Team_Say":
			if (sti(NCServer.GameType) != NETGAME_DEATHMATCH)
			{
				NCSay.Mode = "Team_Say";
				LaunchNetChatString();
			}
		break;
		/*case "Person_Say":
			NCSay.Mode = sControl;
			iSMsg = NMCreate();
			NMAddClientID(iSMsg, wClientID);
			NMAddByte(iSMsg, NC_SAY);
			NMAddByte(iSMsg, NSC_SAY_PERSON);
			NMAddString(iSMsg, "Person Say", 128);
			NMAddByte(iSMsg, wClientID);
			NetClient_SendMessage(iSMsg, false);
			NMDelete(iSMsg);
		break;*/
	}
}

// ---------------------------------------------------------------------------------------------
// COOP: system lines of the chat (player joined / left, kill feed of mines, brander and fort)
// The texts are written here in Russian; to localize them move them to the net ini and use LanguageConvertString
// ---------------------------------------------------------------------------------------------
string NetClient_PlayerNick(int wID)
{
	if (wID == DST_INVALID) { return ""; }
	if (wID < 0 || wID >= NET_MAXCLIENTS) { return ""; }
	ref rPlayer = NetClient_GetClient(wID);
	string sPlayerNick = rPlayer.NickName;
	return sPlayerNick;
}

// one line to the chat: the lobby chat window in the lobby, the battle chat (game message) in the battle
void NetClient_ChatAddSystemLine(string sText)
{
	NetClient_ChatHistoryAdd(sText);

	if (bNetLobbyChatActive)
	{
		NetClient_LobbyChatAdd(sText);
		return;
	}

	NCSay.Color = rgbGameMessage;
	NCSay.Time = 30.0;
	NCSay.GameMessage = sText;
}

// bJoin = true: "player connected" + new_level.wav (new player and reconnect); false: "player disconnected" + box_locked.wav
void NetClient_AnnouncePlayer(int wID, bool bJoin)
{
	string sNick = NetClient_PlayerNick(wID);
	if (sNick == "") { return; }

	if (bJoin)
	{
		PlayStereoSound(NET_SOUND_PLAYER_JOIN);
		NetClient_ChatAddSystemLine("Игрок " + sNick + " подключился");
	}
	else
	{
		PlayStereoSound(NET_SOUND_PLAYER_LEFT);
		NetClient_ChatAddSystemLine("Игрок " + sNick + " отключился");
	}
}

// kill feed lines for the new kill statuses
string NetClient_KillText(int iKillStatus, int wKillerID, int wDeadID)
{
	string sDead = NetClient_PlayerNick(wDeadID);
	string sKiller = NetClient_PlayerNick(wKillerID);
	string sText = "";

	switch (iKillStatus)
	{
		case NETKILL_BY_OWN_MINE:
			sText = sDead + " подорвался на своей мине";
		break;
		case NETKILL_BY_MINE:
			if (sKiller != "") { sText = sDead + " подорвался на мине игрока " + sKiller; }
			else { sText = sDead + " подорвался на мине"; }
		break;
		case NETKILL_BY_BRANDER:
			if (sKiller != "") { sText = sKiller + " уничтожил брандером " + sDead; }
			else { sText = sDead + " уничтожен брандером"; }
		break;
		case NETKILL_BY_OWN_BRANDER:
			sText = sDead + " взорвал свой брандер";
		break;
		case NETKILL_BY_FORT:
			sText = "Форт пустил ко дну " + sDead;
		break;
	}
	return sText;
}
