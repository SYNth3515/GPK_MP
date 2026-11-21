void NetClient_OnPlayerInfo(int iMsg)
{
	int iSubCode = NMGetByte(iMsg);

	ref rClient;
	int wNetClientID;

	switch (iSubCode)
	{
		case NSC_PLAYERINFO_SHIPCHANGE:
			wNetClientID = NMGetClientID(iMsg);
			rClient = NetClient_GetClient(wNetClientID);
			int iNewShipType = NMGetWord(iMsg);
			// COOP s30: the ship of the slot sails in this game: its type never changes (an old server sent the lobby ship of
			// a player who reconnects)
			if (bNetClientInGame && IsEntity(rClient))
			{
				Trace("COOP: cli ship change of " + wNetClientID + " ignored, the ship is in the game");
			}
			else
			{
				rClient.Ship.Type = iNewShipType;
				Event("NCEvent_PlayerInfo_ShipType", "ll", wNetClientID, sti(rClient.Ship.Type));
			}
		break;
		case NSC_PLAYERINFO_BUYREADY:
			wNetClientID = NMGetClientID(iMsg);
			rClient = NetClient_GetClient(wNetClientID);
			rClient.BuyReady = NMGetByte(iMsg);
			Event("NCEvent_PlayerInfo_BuyReady", "l", wNetClientID);
		break;
		case NSC_PLAYERINFO_STARTREADY:
			wNetClientID = NMGetClientID(iMsg);
			rClient = NetClient_GetClient(wNetClientID);
			rClient.StartReady = NMGetByte(iMsg);
			Event("NCEvent_PlayerInfo_StartReady", "l", wNetClientID);
		break;
		case NSC_PLAYERINFO_TEAMCHANGE:
			wNetClientID = NMGetClientID(iMsg);
			rClient = NetClient_GetClient(wNetClientID);
			rClient.Team = NMGetByte(iMsg);
			Event("NCEvent_PlayerInfo_Team", "ll", wNetClientID, sti(rClient.Team));
		break;
		case NSC_PLAYERINFO_STARTCOUNTDOWN:
			int iClientStartTimer = NMGetDword(iMsg);
			Event("NCEvent_PlayerInfo_StartCountdown", "l", iClientStartTimer);
		break;
		case NSC_PLAYERINFO_STOPCOUNTDOWN:
			Event("NCEvent_PlayerInfo_StopCountdown");
		break;
	}
}
