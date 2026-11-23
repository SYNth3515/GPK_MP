
// COOP: a slot whose ship was created in the running game is "burned" until the game is over: the ship entity of that slot
// stays alive on every client (a ship of a client is never deleted, see client\Time.c), so a NEW player must get another slot.
// The same player can come back to his own slot (NetServer_FindParkedSlot).
bool NetServer_SlotBurned(int i)
{
	if (CheckAttribute(&NSClients[i], "Parked") && sti(NSClients[i].Parked)) { return true; }
	if (bServerGameStarted && CheckAttribute(&NSClients[i], "Spawned") && sti(NSClients[i].Spawned)) { return true; }
	return false;
}

int NetServer_FindFreeClientID()
{
	for (int i=0; i<NET_MAXPLAYERS; i++)		// COOP: the slots after them are for the bots (server\Bots.c)
	{
		if (sti(NSClients[i].Use)) { continue; }
		if (NetServer_SlotBurned(i)) { continue; }
		return i;
	}

	return DST_INVALID;
}

// COOP: reconnect. The parked slot of the same nick (and password) whose ship can be given back:
// the ship is afloat, or respawn is on (a wrecked ship comes back at once).
int NetServer_FindParkedSlot(string sNickName, string sNickPassword)
{
	if (!bServerGameStarted) { return DST_INVALID; }
	for (int i=0; i<NET_MAXCLIENTS; i++)
	{
		if (sti(NSClients[i].Use)) { continue; }
		if (!CheckAttribute(&NSClients[i], "Parked") || !sti(NSClients[i].Parked)) { continue; }
		if (NSClients[i].NickName != sNickName) { continue; }
		if (NSClients[i].NickPassword != sNickPassword) { continue; }
		if (sti(NSClients[i].Dead) && !sti(NetServer.Respawn)) { continue; }		// sunk without respawn: a new ship in a new slot
		return i;
	}
	return DST_INVALID;
}

void NetServer_DropClient(int wNetClientID)
{
	if (wNetClientID < 0 || wNetClientID >= NET_MAXCLIENTS) { return; }
	ref rClient = NetServer_GetClient(wNetClientID);
	if (!sti(rClient.Use)) { return; }		// COOP: already dropped (the disconnect message can come twice)
	rClient.Use = false;
	rClient.InGame = false;
	rClient.JoinRequested = false;
	NetServer_BDLeft(wNetClientID);		// COOP s37: a captain on the deck has left: the other one wins
	NetServer_PrizeCaptorGone(wNetClientID);		// COOP s41: his prize goes down

	// COOP: the player left the running game: his ship stays (it can not be deleted on the clients) and waits for a reconnect.
	// It drops the sails and stops; it can still be shot and sunk.
	if (bServerGameStarted && CheckAttribute(rClient, "Spawned") && sti(rClient.Spawned))
	{
		rClient.Parked = true;
		rClient.Ship.SailState = 0.0;
		rClient.Ship.TurnLeft = 0;
		rClient.Ship.TurnRight = 0;
		rClient.Ship.TurnState = 0.0;
		Trace("COOP: client " + wNetClientID + " (" + rClient.NickName + ") left the game, ship parked for a reconnect");
	}

	/*if (bServerGameStarted)
	{
		NetServer_UpdateGlobalPlayerStatistics(rClient);
		DeleteClass(&NSClients[sti(rClient.index)]);
	}*/

	NetServer_DelClient(wNetClientID);

	// send to all
	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_CLIENT);
	NMAddByte(iSMsg, NSC_CLIENT_DROP);
	NMAddClientID(iSMsg, wNetClientID);
	NetServer_SendMessage(DST_ALL, iSMsg, true);
	NMDelete(iSMsg);
}

void NetServer_OnDisconnect(int wNetClientID, int iMsg)
{
	if (wNetClientID < 0 || wNetClientID >= NET_MAXCLIENTS) { return; }
	NetServer_DropClient(wNetClientID);
}

bool NetServer_IsLocalIP(int dwIP)
{
	/*int dw1 = and(dwIP, 255);
	int dw2 = and(shr(dwIP, 8), 255);
	int dw3 = and(shr(dwIP, 16), 255);
	int dw4 = shr(dwIP, 24);

	if (dw1 == 192 && dw2 == 168) { return true; }
	if (dw1 == 10) { return true; }
	if (dw1 == 172 && dw2 >= 16 && dw2 <= 31) { return true; }
	if (dw1 == 169 && dw2 == 254) { return true; }
	if (dw1 == 127 && dw2 == 0 && dw2 == 0 && dw2 == 1) { return true; }
	// maybe 169.254

	return false;*/
	return true;
}

void NetServer_OnConnect(int dwIP, int wPort, int iMsg)
{
	int i, iSMsg;

	int iNetTestDword = NMGet24(iMsg);

	bool bLocal = NetServer_IsLocalIP(dwIP);
	bool bInet = !bLocal;
	bool bError = false;

	if (bLocal && !sti(NetServer.LanServer)) { bError = true; }
	if (bInet && !sti(NetServer.InetServer)) { bError = true; }
	
	if (bError)
	{
		iSMsg = NMCreate();
		NMAddByte(iSMsg, NC_CONNECT);
		NMAddByte(iSMsg, NSC_CONNECT_REJECTED);
		NetServer_SendMessageDirect(dwIP, wPort, iSMsg);
		NMDelete(iSMsg);
		return;
	}

	if (iNetTestDword != NET_TEST_DWORD)
	{
		iSMsg = NMCreate();
		NMAddByte(iSMsg, NC_CONNECT);
		NMAddByte(iSMsg, NSC_CONNECT_REJECTED);
		NetServer_SendMessageDirect(dwIP, wPort, iSMsg);
		NMDelete(iSMsg);
		return;
	}

	int iClientNetVersion = NMGetByte(iMsg);
	if (iClientNetVersion != NET_SCRIPT_VERSION)
	{
		iSMsg = NMCreate();
		NMAddByte(iSMsg, NC_CONNECT);
		NMAddByte(iSMsg, NSC_CONNECT_REJECTED_DIFFERENT_VERSION);
		NetServer_SendMessageDirect(dwIP, wPort, iSMsg);
		NMDelete(iSMsg);
		return;
	}

	string sNickName = NMGetString(iMsg);
	string sNickPassword = NMGetString(iMsg);
	string sServerPassword = NMGetString(iMsg);
	string sShipName = NMGetString(iMsg);
	string sFaceImage = NMGetString(iMsg);
	string sFlagImage = NMGetString(iMsg);
	string sSailImage = NMGetString(iMsg);
	int iSailColor = NMGetDword(iMsg);

	// check for already connect(maybe if big lag)

	// check for connecting to already played game
	// COOP: a running game can be joined (late join / reconnect); only the short start phase (countdown / buy info) is closed
	if (!bServerGameStarted && (iSecondsToStartGame >= 0))
	{
		iSMsg = NMCreate();
		NMAddByte(iSMsg, NC_CONNECT);
		NMAddByte(iSMsg, NSC_CONNECT_REJECTED);
		NetServer_SendMessageDirect(dwIP, wPort, iSMsg);
		NMDelete(iSMsg);
		return;
	}

	// check password
	if (NetServer.ServerPassword != "" && sServerPassword != NetServer.ServerPassword)
	{
		iSMsg = NMCreate();
		NMAddByte(iSMsg, NC_CONNECT);
		NMAddByte(iSMsg, NSC_CONNECT_NEED_PASSWORD);
		NetServer_SendMessageDirect(dwIP, wPort, iSMsg);
		NMDelete(iSMsg);
		return;
	}

	// COOP: the same player (nick, password, address) is still in the list: his game crashed and the timeout has not dropped
	// him yet. Drop the old connection now, its ship is parked and given back below.
	if (bServerGameStarted)
	{
		for (i=0; i<NET_MAXCLIENTS; i++)
		{
			if (!sti(NSClients[i].Use)) { continue; }
			if (NSClients[i].NickName != sNickName) { continue; }
			if (NSClients[i].NickPassword != sNickPassword) { continue; }
			if (sti(NSClients[i].dwIP) != dwIP) { continue; }
			Trace("COOP: " + sNickName + " connects again, the old connection of slot " + i + " is dropped");
			NetServer_DropClient(i);
		}
	}

	// COOP s38: "Join during the game" is off: nobody comes into the running game, not even into its lobby (a player whose
	// ship waits for him - a reconnect - does)
	if (bServerGameStarted && !NetServer_LateJoinOn())
	{
		if (NetServer_FindParkedSlot(sNickName, sNickPassword) == DST_INVALID)
		{
			Trace("COOP: " + sNickName + " is not let in: the game runs and the join during the game is off");
			iSMsg = NMCreate();
			NMAddByte(iSMsg, NC_CONNECT);
			NMAddByte(iSMsg, NSC_CONNECT_REJECTED);
			NetServer_SendMessageDirect(dwIP, wPort, iSMsg);
			NMDelete(iSMsg);
			return;
		}
	}

	// check for maxclients
	if (NetServer_GetClientsNum() >= sti(NetServer.MaxClients))
	{
		iSMsg = NMCreate();
		NMAddByte(iSMsg, NC_CONNECT);
		NMAddByte(iSMsg, NSC_CONNECT_REJECTED_SERVER_FULL);
		NetServer_SendMessageDirect(dwIP, wPort, iSMsg);
		NMDelete(iSMsg);
		return;
	}

	// find new client id
	// COOP: the same player comes back to the running game: his old slot and ship
	bool bReconnect = false;
	int wNetClientID = NetServer_FindParkedSlot(sNickName, sNickPassword);
	if (wNetClientID != DST_INVALID) { bReconnect = true; }
	else { wNetClientID = NetServer_FindFreeClientID(); }
	if (wNetClientID == DST_INVALID)
	{
		iSMsg = NMCreate();
		NMAddByte(iSMsg, NC_CONNECT);
		NMAddByte(iSMsg, NSC_CONNECT_REJECTED);
		NetServer_SendMessageDirect(dwIP, wPort, iSMsg);
		NMDelete(iSMsg);
		return;
	}
	
	// if find client in NSPlayers, check password
	if (CheckAttribute(NSPlayers, "Players." + sNickName))
	{
		if (NSPlayers.Players.(sNickName).NickPassword != sNickPassword)
		{
			iSMsg = NMCreate();
			NMAddByte(iSMsg, NC_CONNECT);
			NMAddByte(iSMsg, NSC_CONNECT_REJECTED_NEED_CLIENT_PASSWORD);
			NetServer_SendMessageDirect(dwIP, wPort, iSMsg);
			NMDelete(iSMsg);
			return;
		}
	}

	// add new client
	NetServer_AddClient(wNetClientID, dwIP, wPort);

	ref rClient = NetServer_GetCLient(wNetClientID);

	rClient.Use = true;
	rClient.dwIP = dwIP;
	rClient.wPort = wPort;

	rClient.NickName = sNickName;
	rClient.NickPassword = sNickPassword;

	if (bReconnect)
	{
		// COOP: keep the ship, the team and the statistics of the slot, only the connection is new
		rClient.LastPingTime = iPingTime;
		rClient.LastPacketIndex = 0;
		rClient.Admin = false;
		rClient.Parked = false;
		rClient.InGame = false;
		rClient.Reconnect = true;
		rClient.JoinRequested = false;
		rClient.UserData.Face = sFaceImage;
		rClient.UserData.Flag = sFlagImage;
		rClient.UserData.Sail = sSailImage;
		Trace("COOP: client " + wNetClientID + " (" + sNickName + ") reconnected to the running game");
	}
	else
	{
		NetServer_InitNewClientSlot(rClient, sShipName, sFaceImage, sFlagImage, sSailImage, iSailColor);
	}

	NetServer_SendConnectAccepted(wNetClientID, bReconnect);
}

// COOP: the old body of NetServer_OnConnect after "add new client": a fresh slot
void NetServer_InitNewClientSlot(ref rClient, string sShipName, string sFaceImage, string sFlagImage, string sSailImage, int iSailColor)
{
	int iShipType = 10;
	ref rShip = Net_GetShipByIndex(iShipType);

	float fRadius = 100.0 + frnd() * 200.0;
	float fAng = frnd() * PIm2;

	rClient.LastPingTime = iPingTime;
	rClient.LastPacketIndex = 0;
	rClient.StartReady = false;
	rClient.BuyReady = false;
	rClient.Dead = false;
	rClient.Admin = false;		// COOP: slot may be reused, never inherit admin rights / mute
	rClient.Muted = false;
	rClient.Reconnect = false;
	rClient.Parked = false;
	rClient.InGame = false;
	rClient.Spawned = false;
	rClient.JoinRequested = false;
	rClient.Team = 0;
	rClient.Stat.Sunk = 0;
	rClient.Stat.Wreck = 0;
	rClient.Stat.Misses = 0;
	rClient.Stat.Hits = 0;
	rClient.Stat.Won = 0;
	rClient.Stat.Lost = 0;
	rClient.Stat.DamageInflicted = 0;
	rClient.Stat.Deaths = 0;
	rClient.UserData.Face = sFaceImage;
	rClient.UserData.Flag = sFlagImage;
	rClient.UserData.Sail = sSailImage;
	rClient.Ship.Type = -1;
	rClient.Ship.Name = sShipName;
	rClient.Ship.Pos.x = -1400.0 + fRadius * sin(fAng);
	rClient.Ship.Pos.y = 0.0;
	rClient.Ship.Pos.z = -1900.0 + fRadius * cos(fAng);
	rClient.Ship.Ang.y = frnd() * PIm2;
	rClient.Ship.Speed.x = 0.0;
	rClient.Ship.Speed.y = 0.0;
	rClient.Ship.Speed.z = 0.0;
	rClient.Ship.Upgrades.Hull = 1;
	rClient.Ship.Upgrades.Sails = 1;
	rClient.Ship.Upgrades.Cannons = 1;

	rClient.Ship.Sails.Color = iSailColor;

	// setup goods
/*	for (int i=0; i<Net_GetGoodsNum(); i++)
	{
		ref rGood = Net_GetGoodByIndex(i);
		string sGood = rGood.Name;
		rClient.Ship.Goods.(sGood) = (i + 1) * 1350;
	}*/
}

void NetServer_SendConnectAccepted(int wNetClientID, bool bReconnect)
{
	int i, iSMsg;
	ref rClient = NetServer_GetClient(wNetClientID);

	string sFaceImage = rClient.UserData.Face;
	string sFlagImage = rClient.UserData.Flag;
	string sSailImage = rClient.UserData.Sail;
	int iSailColor = sti(rClient.Ship.Sails.Color);

	// a reconnecting player keeps his ship: the other clients must not forget its type (byte "keep ship")
	int iKeepShip = 0;
	if (bReconnect && sti(rClient.Spawned)) { iKeepShip = 1; }

	iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_CONNECT);
	NMAddByte(iSMsg, NSC_CONNECT_ACCEPTED);
	NMAddClientID(iSMsg, wNetClientID);
	NetServer_SendMessage(wNetClientID, iSMsg, true);
	Trace("Client with IP " + inet_ntoa(sti(rClient.dwIP)) + ":" + ntohs(sti(rClient.wPort)) + " accepted. wNetClientID = " + wNetClientID + " reconnect=" + rClient.Reconnect);
	NMDelete(iSMsg);

	// send to all
	iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_CLIENT);
	NMAddByte(iSMsg, NSC_CLIENT_CONNECTED);
	NMAddClientID(iSMsg, wNetClientID);
	NMAddString(iSMsg, rClient.NickName, 24);	// nick name
	NMAddString(iSMsg, rClient.Ship.Name, 24);	// nick name
	NMAddString(iSMsg, sFaceImage, 12);							// client face
	NMAddString(iSMsg, sFlagImage, 12);							// client flag
	NMAddString(iSMsg, sSailImage, 12);							// client sail emblem
	NMAddDword(iSMsg, iSailColor);								// client sail color
	NMAddByte(iSMsg, sti(rClient.BuyReady));					// Buy Ready
	NMAddByte(iSMsg, iKeepShip);								// COOP: 1 = reconnect, the ship of this slot exists
	NMAddByte(iSMsg, 1);												// COOP: 1 = announce this player in the chat (joined / reconnected)
	NetServer_SendMessage(DST_ALL, iSMsg, true);
	NMDelete(iSMsg);

	// send to new client all other clients
	for (i=0; i<NET_MAXCLIENTS; i++) 
	{
		ref rOldClient = &NSClients[i];
		if (!sti(rOldClient.Use)) { continue; }
		if (sti(rOldClient.ID) == wNetClientID) { continue; }

		iSMsg = NMCreate();
		NMAddByte(iSMsg, NC_CLIENT);
		NMAddByte(iSMsg, NSC_CLIENT_CONNECTED);
		NMAddClientID(iSMsg, sti(rOldClient.ID));
		NMAddString(iSMsg, rOldClient.NickName, 24);			// nick name
		NMAddString(iSMsg, rOldClient.Ship.Name, 24);			// nick name
		NMAddString(iSMsg, rOldClient.UserData.Face, 12);		// client face
		NMAddString(iSMsg, rOldClient.UserData.Flag, 12);		// client flag
		NMAddString(iSMsg, rOldClient.UserData.Sail, 12);		// client sail emblem
		NMAddDword(iSMsg, sti(rOldClient.Ship.Sails.Color));	// client sail color
		NMAddByte(iSMsg, sti(rOldClient.BuyReady));				// Buy Ready
		NMAddByte(iSMsg, 0);									// COOP: the new client has no ships yet
		NMAddByte(iSMsg, 0);											// COOP: 0 = the player was here before the newcomer: no chat line
		NetServer_SendMessage(wNetClientID, iSMsg, true);
		NMDelete(iSMsg);
	}

	//NetServer_DownloadClientFile(wNetClientID, "resource\textures\" + sUserFacesPath + sFaceImage + "tga.tx");
	//NetServer_DownloadClientFile(wNetClientID, "resource\textures\" + sUserSailsPath + sSailImage + "tga.tx");
	//NetServer_DownloadClientFile(wNetClientID, "resource\textures\" + sUserFlagsPath + sFlagImage + "tga.tx");
}

