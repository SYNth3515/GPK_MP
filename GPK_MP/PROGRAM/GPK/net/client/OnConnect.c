
void NetClient_OnConnect(int iMsg)
{
	int iSubCode = NMGetByte(iMsg);
	
	switch (iSubCode)
	{
		case NSC_CONNECT_ACCEPTED:
			NetClient_OnConnectAccepted(iMsg);
		break;
		case NSC_CONNECT_REJECTED:
			NetClient_OnConnectRejected(iMsg);
		break;
		case NSC_CONNECT_REJECTED_DIFFERENT_VERSION:
			NetClient_OnConnectRejected(iMsg);
		break;
		case NSC_CONNECT_REJECTED_SERVER_FULL:
			NetClient_OnConnectRejectedServerFull(iMsg);
		break;
		case NSC_CONNECT_REJECTED_NEED_CLIENT_PASSWORD:
			NetClient_OnConnectRejectedClientPassword(iMsg);
		break;
		case NSC_CONNECT_BANNED:
			NetClient_OnConnectBanned(iMsg);
		break;
		case NSC_CONNECT_NEED_PASSWORD:
			NetClient_OnConnectNeedPassword(iMsg);
		break;
		case NSC_CONNECT_RECONNECT:
			NetClient_OnConnectReconnect();
		break;
		case NSC_CONNECT_DISCONNECT:
			Event("NCEvent_Disconnect");
		break;
		case NSC_CONNECT_TOLOBBY:
			NetClient_OnBackToLobby();
		break;
	}
}

// COOP s30: the server ended the game and keeps the connection: the sea / land of this machine goes (the same cleaning as
// the exit to the main menu: client\OnStart.c NetClient_OnEndGame), the lobby (Buy screen) opens again
void NetClient_OnBackToLobby()
{
	int i;
	Trace("COOP: cli back to the lobby (in game " + bNetClientInGame + ")");
	NetClient_OnEndGame();
	NCServer.GameRunning = false;

	for (i=0; i<NET_TOTALCLIENTS; i++)
	{
		DeleteAttribute(&NCClients[i], "ShipGone");
		DeleteAttribute(&NCClients[i], "CoopReady");
		DeleteAttribute(&NCClients[i], "CoopGame");
		DeleteAttribute(&NCClients[i], "MastFall");
		DeleteAttribute(&NCClients[i], "Ship.Masts");
		DeleteAttribute(&NCClients[i], "Ship.Sink");
		DeleteAttribute(&NCClients[i], "APerk");
		NCClients[i].BuyReady = false;
		NCClients[i].StartReady = false;
		NCClients[i].Dead = false;
		NCClients[i].Wrecked = false;
		NCClients[i].ShipState = 0;
		NCClients[i].Stat.DamageInflicted = 0;
		NCClients[i].Stat.Accuracy = 0;
		NCClients[i].Stat.Kills = "";
		NCClients[i].Stat.Frags = 0;
		NCClients[i].Stat.Deaths = 0;
	}

	// NetClient_OnEndGame took the chat and the disconnect handler: the lobby needs them
	NetClient_CreateChatEnvironment();
	SetEventHandler("NCEvent_Disconnect", "NetClient_OnDisconnect", 0);

	PauseAllSounds();
	ResetSoundScheme();
	SetMusic("music_MainMenu");

	// a player who sat in the lobby (did not join the game) keeps it: it reads GameRunning every frame
	if (sti(InterfaceStates.Launched) && CurrentInterface == INTERFACE_NET_BUY) { return; }

	// the Esc menu, the statistics or any other window of the game closes; the lobby opens on the next frame
	if (sti(InterfaceStates.Launched))
	{
		interfaceResultCommand = RC_INTERFACE_DO_NOTHING;
		EndCancelInterface(true);
	}
	SetEventHandler("NetClient_LobbyOpen", "NetClient_LobbyOpen", 0);
	PostEvent("NetClient_LobbyOpen", 1);
}

// COOP s31: the host leaves the running game (interfaces\NetMenu.c): only he goes to the lobby, the server and the game go on;
// "Join the battle" of the lobby brings him back (his ship waited like the ship of a reconnect)
void NetClient_LeaveToLobby()
{
	DelEventHandler("NetClient_LeaveToLobby", "NetClient_LeaveToLobby");
	if (!bNetClientConnected) { return; }
	NetServer_ClientToLobby(wClientID);
	NetClient_OnBackToLobby();
	NCServer.GameRunning = true;
}

void NetClient_LobbyOpen()
{
	DelEventHandler("NetClient_LobbyOpen", "NetClient_LobbyOpen");
	if (!bNetClientConnected) { return; }
	if (sti(InterfaceStates.Launched)) { return; }
	LaunchNetBuyScreen();
}

void NetClient_OnConnectReconnect()
{
	Net_DeleteClient();
	//NetClient_TryConnect();
}

void NetClient_OnConnectAccepted(int iMsg)
{
	wClientID = NMGetClientID(iMsg);
	bNetClientConnected = true;

	// create chat environment
	NetClient_CreateChatEnvironment();

	// start ping
	NetClient_Ping();

	Event("NCEvent_Client_Accepted");

	// COOP: admin rights are reset on every connect; the machine that runs the server logs in with its own admin password
	NetClient_AdminReset();
	NetClient_AdminAutoLogin();
	//LaunchNetBuyScreen();

	SetEventHandler("NCEvent_Disconnect", "NetClient_OnDisconnect", 0);
}

void NetClient_OnConnectRejected(int iMsg)
{
	Event("NCEvent_Client_Rejected", "s", "");	// FIX-ME
}

void NetClient_OnConnectRejectedServerFull(int iMsg)
{
	Event("NCEvent_Client_RejectedServerFull");
}

void NetClient_OnConnectRejectedClientPassword(int iMsg)
{
	Event("NCEvent_Client_RejectedClientPassword");
}

void NetClient_OnConnectBanned(int iMsg)
{
	string sReason = NMGetString(iMsg);
	Event("NCEvent_Client_Banned", "s", sReason);
}

void NetClient_OnConnectNeedPassword(int iMsg)
{
	Event("NCEvent_Client_RejectedServerPassword");
}

