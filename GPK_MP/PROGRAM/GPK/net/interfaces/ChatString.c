// COOP: Y window = input line + the full chat history (NCChatHist, see client/OnChat.c)
int iChatHistOffset = 0;	// how many lines the view is scrolled up from the newest one

void InitInterface(string iniName)
{
	GameInterface.title = "";

	SendMessage(&GameInterface, "ls", MSG_INTERFACE_INIT, iniName);
	SetEventHandler("OnCancel", "ICancelProcedure", 0);
	SetEventHandler("EditExit", "ChatS_Enter", 0);
	SetEventHandler("OnSend", "ChatS_OnSend", 0);
	SetEventHandler("OnHistUp", "ChatS_HistUp", 0);
	SetEventHandler("OnHistDown", "ChatS_HistDown", 0);
	SetEventHandler("NCEvent_ChatHistory", "ChatS_HistNew", 0);

	iChatHistOffset = 0;
	ChatS_HistoryUpdate();

	// set string say mode
	SetFormatedText("CAPTION_STRING", XI_ConvertString("NetSay_Mode_" + NCSay.Mode));

	FreezeGroupControls("BattleInterfaceControls", true);
}

void IDoExit(int exitCode)
{
	DelEventHandler("OnCancel", "ICancelProcedure");
	DelEventHandler("EditExit", "ChatS_Enter");
	DelEventHandler("OnSend", "ChatS_OnSend");
	DelEventHandler("OnHistUp", "ChatS_HistUp");
	DelEventHandler("OnHistDown", "ChatS_HistDown");
	DelEventHandler("NCEvent_ChatHistory", "ChatS_HistNew");

	interfaceResultCommand = exitCode;
	EndCancelInterface(true);
}

void ICancelProcedure()
{
	IDoExit( RC_INTERFACE_NET_CHATSTRING );
}

void ChatS_OnSend()
{
	string sMessage = GameInterface.EDITBOX_STRING.str;
	if (sMessage == "") 
	{
		ICancelProcedure();
		return;
	}

	int iSMsg = NMCreate();
	NMAddClientID(iSMsg, wClientID);
	NMAddByte(iSMsg, NC_SAY);
	switch (NCSay.Mode)
	{
		case "Say":			NMAddByte(iSMsg, NSC_SAY_CHAT); break;
		case "Team_Say":	NMAddByte(iSMsg, NSC_SAY_TEAM); break;
		case "Person_Say":	NMAddByte(iSMsg, NSC_SAY_PERSON); break;
	}
	NMAddString(iSMsg, sMessage, 128);
	NetClient_SendMessage(iSMsg, false);
	NMDelete(iSMsg);

	ICancelProcedure();
}

void ChatS_Enter()
{
	string sEditControl = GetEventData();
	if (sEditControl != "EDITBOX_STRING") return;

	ChatS_OnSend();
}

void ChatS_HistoryUpdate()
{
	int i;

	if (!CheckAttribute(&NCChatHist, "Num"))
	{
		SetFormatedText("TEXT_HISTORY", "");
		return;
	}

	int iNum = sti(NCChatHist.Num);
	int iBase = sti(NCChatHist.First);

	int iMaxOffset = iNum - iBase - iNetChatHistPage;
	if (iMaxOffset < 0) { iMaxOffset = 0; }
	if (iChatHistOffset > iMaxOffset) { iChatHistOffset = iMaxOffset; }
	if (iChatHistOffset < 0) { iChatHistOffset = 0; }

	int iLast = iNum - iChatHistOffset;
	int iFirst = iLast - iNetChatHistPage;
	if (iFirst < iBase) { iFirst = iBase; }

	string sText = "";
	for (i=iFirst; i<iLast; i++)
	{
		string sAttr = "l" + i;
		string sLine = NCChatHist.Lines.(sAttr);
		if (sText != "") { sText = sText + "\n"; }
		sText = sText + sLine;
	}

	SetFormatedText("TEXT_HISTORY", sText);
}

// scroll to older / newer lines (buttons and mouse wheel / arrow keys on the input line)
void ChatS_HistUp()
{
	iChatHistOffset = iChatHistOffset + 4;
	ChatS_HistoryUpdate();
}

void ChatS_HistDown()
{
	iChatHistOffset = iChatHistOffset - 4;
	ChatS_HistoryUpdate();
}

// a new line came while the window is open: keep the scrolled position, the newest lines stay at the bottom
void ChatS_HistNew()
{
	if (iChatHistOffset > 0) { iChatHistOffset++; }
	ChatS_HistoryUpdate();
}
