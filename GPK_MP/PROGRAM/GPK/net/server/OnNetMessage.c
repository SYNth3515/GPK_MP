
bool NetServer_CheckForBan(int dwIP, int wPort, int iMsg)
{
	string sBan = "b" + inet_ntoax(dwIP);

	if (CheckAttribute(NSBanList, sBan))	// if ban exist
	{
		// check for time, maybe unBan?

		// check for code CONNECT
		if (NMGetByte(iMsg) == NC_CONNECT) 
		{
			int iSMsg = NMCreate();
			NMAddByte(iSMsg, NC_CONNECT);
			NMAddByte(iSMsg, NSC_CONNECT_BANNED);
			NMAddString(iSMsg, NSBanList.(sBan).Reason, 256);
			NetServer_SendMessageDirect(dwIP, wPort, iSMsg);
			NMDelete(iSMsg);
		}
		return true;
	}
	return false;
}

void NetServer_OnNetMessage()
{
	ref rClient;

	int dwIP = GetEventData();
	int wPort = GetEventData();
	int iMsg = GetEventData();
	int iPacketIndex = GetEventData();

	if (NetServer_CheckForBan(dwIP, wPort, iMsg)) { return; }	// skip message if dwIP banned

	int wNetClientID = NMGetClientID(iMsg);

	// callback garanted message
	if (iPacketIndex)
	{
		int iSMsg = NMCreate();
		NMAddByte(iSMsg, NC_GARANTED);
		NMAddWord(iSMsg, iPacketIndex);
		NetServer_SendMessageDirect(dwIP, wPort, iSMsg);
		NMDelete(iSMsg);
	
		if (wNetClientID != DST_INVALID)
		{
			rClient = NetServer_GetClient(wNetClientID);
			if (iPacketIndex <= sti(rClient.LastPacketIndex)) { return; }	// check for 32767 << 1
			rClient.LastPacketIndex = iPacketIndex;
		}
	}

	int iCode = NMGetByte(iMsg);

	// COOP: a message from a slot that was dropped (ping timeout, kick): this client does not know it, tell it to leave
	// (it goes to the main menu and can connect again: in a running game it gets its parked ship back)
	if (wNetClientID < NET_MAXCLIENTS && !sti(NSClients[wNetClientID].Use))
	{
		if (iCode != NC_DISCONNECT && iCode != NC_GARANTED && iCode != NC_CONNECT && iCode != NC_BROADCAST)
		{
			int iByeSMsg = NMCreate();
			NMAddByte(iByeSMsg, NC_CONNECT);
			NMAddByte(iByeSMsg, NSC_CONNECT_DISCONNECT);
			NetServer_SendMessageDirect(dwIP, wPort, iByeSMsg);
			NMDelete(iByeSMsg);
		}
		if (iCode != NC_CONNECT && iCode != NC_BROADCAST) { return; }
	}

	if (wNetClientID < NET_MAXCLIENTS)	// update port for client, router can change this value on the fly
	{
		rClient = NetServer_GetClient(wNetClientID);
		//rClient.dwIP = dwIP;
		rClient.wPort = wPort;
	}

	switch (iCode)
	{
		case NC_GARANTED:		NetServer_OnGaranted(wNetClientID, iMsg);		break;
		case NC_PING:			NetServer_OnPing(wNetClientID, iMsg);			break;
		case NC_SAY:			NetServer_OnSay(wNetClientID, iMsg);			break;
		case NC_CONNECT:		NetServer_OnConnect(dwIP, wPort, iMsg);			break;
		case NC_DISCONNECT:		NetServer_OnDisconnect(wNetClientID, iMsg);		break;
		case NC_COMMAND:		NetServer_OnCommand(wNetClientID, iMsg);		break;
		case NC_SHIP:			NetServer_OnShip(wNetClientID, iMsg);			break;
		case NC_CAMERA:			NetServer_OnCamera(wNetClientID, iMsg);			break;
		case NC_BROADCAST:		NetServer_OnBroadcast(dwIP, wPort, iMsg);		break;
		case NC_MASTERSERVER:	NetServer_OnMasterServer(dwIP, wPort, iMsg);	break;
		case NC_FILE:			NetServer_OnFile(wNetClientID, iMsg);			break;
		case NC_BUY:			NetServer_OnBuy(wNetClientID, iMsg);			break;
		case NC_SERVERINFO:		NetServer_OnServerInfo(wNetClientID, iMsg);		break;
		case NC_STATISTICS:		NetServer_OnStatistics(wNetClientID, iMsg);		break;
		case NC_LAND:			NetServer_OnLand(wNetClientID, iMsg);			break;	// COOP: land duel (Land.c)
	}
}

void NetServer_OnGaranted(int wNetClientID, int iMsg)
{
	int iGaranted = NMGetWord(iMsg);
	NetServer_GarantedDeliveryCallback(wNetClientID, iGaranted);
}

int iPunchTick = 0;
int iPunchCount = 0;

// P2P: the master server tells us the address of a client that wants to connect (message starts with ClientID 0xFE like any
// client message). We send one tiny packet to that address from the game socket: this opens our NAT for the client,
// so its connect request is not dropped by the router. Other master messages (the ack of ADD/UPDATE) need no action.
// COOP s47d: the master answers ADD / UPDATE with the public address it sees for this server (one per path: the real one and
// the exit of a VPN). The Internet tab of the host shows its own server with it (interfaces\FindGame.c, FG_SetOwnPublicAddr).
void NetServer_OnPublicAddress(int iMsg)
{
	int dwPubIP = NMGetDword(iMsg);
	int wPubPort = ntohs(NMGetWord(iMsg));
	if (wPubPort <= 0) { return; }
	string sPubIP = inet_ntoa(dwPubIP);
	if (!CheckAttribute(&NetServer, "Public")) { NetServer.Public = ""; }
	aref arPub;
	makearef(arPub, NetServer.Public);
	int iNum = GetAttributesNum(arPub);
	for (int i=0; i<iNum; i++)
	{
		aref arA = GetAttributeN(arPub, i);
		if (arA.IP == sPubIP && sti(arA.Port) == wPubPort) { return; }
	}
	if (iNum >= 8) { return; }
	string sKey = "a" + iNum;
	NetServer.Public.(sKey).IP = sPubIP;
	NetServer.Public.(sKey).Port = wPubPort;
	if (wPubPort == sti(NetServer.ServerPort))
	{
		Trace("Master: this server is seen on the Internet as " + sPubIP + ":" + wPubPort + " (the port is kept: the players connect there)");
	}
	else
	{
		Trace("Master: this server is seen on the Internet as " + sPubIP + ":" + wPubPort + " (another port: a NAT or a VPN, the players need P2P)");
	}
}

void NetServer_OnMasterServer(int dwIP, int wPort, int iMsg)
{
	int iSubCode = NMGetByte(iMsg);
	if (iSubCode == NSC_MASTERSERVER_ADDRESS) { NetServer_OnPublicAddress(iMsg); return; }	// COOP s47d
	if (iSubCode != NSC_MASTERSERVER_PUNCH) { return; }
	if (!sti(NetServer.InetServer) || !sti(NetServer.P2P)) { Trace("P2P: punch request ignored (Internet/P2P switch is off)"); return; }

	int dwClientIP = NMGetDword(iMsg);
	int wClientPort = ntohs(NMGetWord(iMsg));
	if (wClientPort <= 0) { return; }
	Trace("P2P: punch request from master, client " + inet_ntoa(dwClientIP) + ":" + wClientPort);

	// at most 30 punches per second
	int iNow = Net_GetTickCount();
	if (iNow - iPunchTick > 1000 || iNow < iPunchTick) { iPunchTick = iNow; iPunchCount = 0; }
	if (iPunchCount >= 30) { return; }
	iPunchCount++;

	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_MASTERSERVER);
	NMAddByte(iSMsg, NSC_MASTERSERVER_PUNCH);
	NetServer_SendMessageDirect(dwClientIP, wClientPort, iSMsg);
	NMDelete(iSMsg);
}

void NetServer_OnBroadcast(int dwIP, int wPort, int iMsg)
{
	int byCallbackCode = NMGetByte(iMsg);
	int iBroadcastTime = NMGetDword(iMsg);

	if (byCallbackCode == NSC_BROADCAST_LOCAL && sti(NetServer.LanServer) != true) { return; }
	if (byCallbackCode == NSC_BROADCAST_INET && sti(NetServer.InetServer) != true) { return; }
	if (byCallbackCode == NSC_BROADCAST_INET) { Trace("Srv: Internet ping from " + inet_ntoa(dwIP) + ":" + ntohs(wPort) + ", answered"); }	// COOP s47c

	int iSMsg = NMCreate();
	// COOP s27: the button "Подробно" of the list of the servers: all the parameters of this server
	if (byCallbackCode == NSC_BROADCAST_DETAILS)
	{
		NMAddByte(iSMsg, NC_BROADCAST);
		NMAddByte(iSMsg, byCallbackCode);
		NMAddDword(iSMsg, iBroadcastTime);
		NMAddString(iSMsg, Net_DetailsCode(), 256);
		NetServer_SendMessageDirect(dwIP, wPort, iSMsg);
		NMDelete(iSMsg);
		return;
	}
	NMAddByte(iSMsg, NC_BROADCAST);
	NMAddByte(iSMsg, byCallbackCode);
	NMAddDword(iSMsg, iBroadcastTime);
	NMAddString(iSMsg, NetServer.ServerName, 32);
	NMAddByte(iSMsg, NetServer.ServerPassword != "");
	NMAddByte(iSMsg, sti(NetServer.GameType));
	NMAddString(iSMsg, NetServer_AdvertisedIsland(), 32);	// COOP: a land duel shows its location
	NMAddString(iSMsg, NetServer.WeatherID, 32);
	NMAddWord(iSMsg, NetServer_GetClientsNum());
	NMAddWord(iSMsg, sti(NetServer.MaxClients));
	NMAddDword(iSMsg, sti(NetServer.Credit));
	NMAddByte(iSMsg, sti(NetServer.MaxShipClass));
	NetServer_SendMessageDirect(dwIP, wPort, iSMsg);
	NMDelete(iSMsg);
}

void NetServer_OnStatistics(int wNetClientID, int iMsg)
{
	int iCode = NMGetByte(iMsg);
	switch (iCode)
	{
		case NSC_STATISTICS_TOTAL_PLAYER:
			NetServer_GetTotalPlayerStatistic(wNetClientID, iMsg);
		break;
	}
}

void NetServer_GetTotalPlayerStatistic(int wNetClientID, int iMsg)
{
	aref arPlayers; makearef(arPlayers, NSPlayers.Players);
	aref arPlayer;

	int iPlayersNum = GetAttributesNum(arPlayers);
	if (iPlayersNum > MAX_TOPPLAYERS) { iPlayersNum = MAX_TOPPLAYERS; }

	int iPIndex = NMGetWord(iMsg);

	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_STATISTICS);
	NMAddByte(iSMsg, NSC_STATISTICS_TOTAL_PLAYER);
	if (iPIndex >= iPlayersNum)
	{
		NMAddDword(iSMsg, -1);
		NetServer_SendMessage(wNetClientID, iSMsg, true);
		NMDelete(iSMsg);
		return;
	}

	//int iRealPIndex = NSSortedPlayers[iPIndex];
	int iRealPIndex = NSSortedPlayers[iPIndex];

	arPlayer = GetAttributeN(arPlayers, iRealPIndex);

	NMAddDword(iSMsg, iPIndex);
	NMAddString(iSMsg, arPlayer.NickName, 24);
	NMAddString(iSMsg, arPlayer.FaceImage, 20);
	NMAddByte(iSMsg, sti(arPlayer.Rank));
	NMAddByte(iSMsg, sti(arPlayer.Rewards.Accuracy));
	NMAddByte(iSMsg, sti(arPlayer.Rewards.Vitality));
	NMAddByte(iSMsg, sti(arPlayer.Rewards.Victorious));
	NMAddWord(iSMsg, sti(arPlayer.Won));
	NMAddWord(iSMsg, sti(arPlayer.Lost));
	NMAddWord(iSMsg, sti(arPlayer.Shipwrecks));
	NMAddWord(iSMsg, sti(arPlayer.ShipsSunk));
	NMAddFloat(iSMsg, stf(arPlayer.Accuracy));
	NMAddDword(iSMsg, sti(arPlayer.TotalDamage));
	NetServer_SendMessage(wNetClientID, iSMsg, true);
	NMDelete(iSMsg);
}

