// COOP switch: 0 = create the client ship at once inside the load message (as in the original code),
//              1 = create it 50..3000 ms later, when the client clock has reached 150 (NetClient_ShipCreateLater).
// Two hard freezes (one on the host, one on a client) happened right after the "ship create postponed" lines, so the
// default is 0.
int iCoopDelayShip = 0;

// COOP: respawn of the bots (the switch of the admin, comes from the server: NET_BOTOP_OPTS)
bool bNetCliBotRespawn = false;


void NetClient_OnClient(int iMsg)
{
	int iSubCode = NMGetByte(iMsg);

	switch (iSubCode)
	{
		case NSC_CLIENT_CONNECTED: 
			NetClient_Connect(iMsg);
		break;
		case NSC_CLIENT_LOAD: 
			NetClient_Load(iMsg);
		break;
		case NSC_CLIENT_DROP:
			NetClient_Drop(iMsg);
		break;
		case NSC_CLIENT_BOT:
			NetClient_OnBot(iMsg);		// COOP: bots of the admin
		break;
	}
}

void NetClient_Drop(int iMsg)
{
	int wNetClientID = NMGetClientID(iMsg);	// ClientID
	ref rClient = NetClient_GetClient(wNetClientID);

	// COOP: chat line + sound "player disconnected" (lobby and battle; in the running game his ship stays parked, he can come back)
	if (sti(rClient.Use) && wNetClientID != wClientID)
	{
		NetClient_AnnouncePlayer(wNetClientID, false);
	}

	rClient.Use = false;
	NetLand_PlayerLeft(wNetClientID);		// COOP: land duel (Land.c)

	//DeleteClass(rClient);

	Event("NCEvent_ClientDrop", "l", wNetClientID);
	Event("NCEvent_ShipDelete", "l", wNetClientID);
}

void NetClient_Connect(int iMsg)
{
	int wNetClientID = NMGetClientID(iMsg);

	string sNickName = NMGetString(iMsg);
	string sShipName = NMGetString(iMsg);
	string sFaceImage = NMGetString(iMsg);
	string sFlagImage = NMGetString(iMsg);
	string sSailImage = NMGetString(iMsg);

	int iSailColor = NMGetDword(iMsg);
	int bBuyReady = NMGetByte(iMsg);
	int iKeepShip = NMGetByte(iMsg);		// COOP: 1 = the player reconnected to the running game, his ship exists here
	int iAnnounce = NMGetByte(iMsg);		// COOP: 1 = write "player connected" to the chat (new player or reconnect), 0 = he was here before us

	ref rClient = NetClient_GetClient(wNetClientID);

	// COOP: our copy of his ship is alive: keep its type, put it back into the battle interface
	bool bKeepShip = false;
	if (iKeepShip != 0 && wNetClientID != wClientID && COOP_CliSlotLive(wNetClientID) && CheckAttribute(rClient, "CoopReady") && sti(rClient.CoopReady)) { bKeepShip = true; }

	rClient.Use = true;
	rClient.BuyReady = bBuyReady;
	rClient.StartReady = false;
	rClient.Kick = false;
	rClient.NickName = sNickName;
	rClient.Ship.Name = sShipName;
	rClient.Ship.Sails.Color = iSailColor;

	// Single-player portraits (face_*.tga.tx) live outside the multiplayer user-face folder.
	// Keep compatibility with existing custom multiplayer avatars.
	string sFaceTex = sUserFacesPath + sFaceImage + ".tga";
	if (findSubStr(sFaceImage, "face_", 0) == 0)
	{
		sFaceTex = "interfaces\portraits\256\" + sFaceImage + ".tga";
	}
	string sFlagTex = sUserFlagsPath + sFlagImage + ".tga";
	string sSailTex = sUserSailsPath + sSailImage + ".tga";

	rClient.UserData.Face = sFaceImage;
	rClient.UserData.Face.Texture = NFGetTexture("UserDataImages", sFaceTex);

	rClient.UserData.Sail = sSailImage;
	rClient.UserData.Sail.Texture = NFGetTexture("UserDataImages", sSailTex);

	rClient.UserData.Flag = sFlagImage;
	rClient.UserData.Flag.Texture = NFGetTexture("UserDataImages", sFlagTex);

	if (!bKeepShip) { rClient.Ship.Type = -1; }

	if (!sti(rClient.UserData.Face.Texture))
	{
		rClient.UserData.Face.Texture = NFGetTexture("UserDataImages", sUserFacesPath + "_emptyface_.tga");
		//NetClient_DownloadServerFile("resource\textures\" + sUserFacesPath + sFaceImage + ".tga.tx", NETFILE_FACE, wNetClientID);
	}

	if (!sti(rClient.UserData.Sail.Texture))
	{
		rClient.UserData.Sail.Texture = NFGetTexture("UserDataImages", sUserSailsPath + "_emptysail_.tga");
		//NetClient_DownloadServerFile("resource\textures\" + sUserSailsPath + sSailImage + ".tga.tx", NETFILE_SAIL, wNetClientID);
	}

	if (!sti(rClient.UserData.Flag.Texture))
	{
		rClient.UserData.Flag.Texture = NFGetTexture("UserDataImages", sUserFlagsPath + "_emptyflag_.tga");
		//NetClient_DownloadServerFile("resource\textures\" + sUserFlagsPath + sFlagImage + ".tga.tx", NETFILE_FLAG, wNetClientID);
	}

	Event("NCEvent_Client_Connect", "l", wNetClientID);

	// COOP: chat line + sound "player connected" (new player and reconnect; never for ourselves)
	if (iAnnounce != 0 && wNetClientID != wClientID) { NetClient_AnnouncePlayer(wNetClientID, true); }

	if (bKeepShip)
	{
		Event("NCEvent_ShipCreate", "l", wNetClientID);
	}
}

void NetClient_Load(int iMsg)
{
	int i;

	int wNetClientID = NMGetClientID(iMsg);	// ClientID

	// create battle interface
	if (wNetClientID == NetClient_GetThisClientID())
	{
		// create battle interface
		NetClient_CreateBattleInterface();
		NetClient_CreateInterfaceEnvironment();
	}

	ref rClient = NetClient_GetClient(wNetClientID);

	rClient.Use = true;

	rClient.NickName = NMGetString(iMsg);
	rClient.Dead = false;
	rClient.Wrecked = false;
	rClient.ShipState = 0;

	rClient.Ship.Type = NMGetWord(iMsg);
	rClient.Ship.HP = NMGetFloat(iMsg);
	rClient.Ship.MaxHP = rClient.Ship.HP;
	rClient.Ship.Upgrades.Hull = NMGetByte(iMsg);
	rClient.Ship.Upgrades.Sails = NMGetByte(iMsg);

	rClient.Ship.Cannons.Type = NMGetByte(iMsg);
	rClient.Ship.Upgrades.Cannons = NMGetByte(iMsg);
	rClient.Ship.Crew.Quantity = NMGetWord(iMsg);

	rClient.Skill.Accuracy = NMGetByte(iMsg);
	rClient.Skill.Cannons = NMGetByte(iMsg);
	rClient.Skill.Sailing = NMGetByte(iMsg);
	rClient.Skill.Defence = NMGetByte(iMsg);
	rClient.Skill.Repair = NMGetByte(iMsg);
	rClient.Skill.Grappling = NMGetByte(iMsg);	// COOP: for the spyglass (server\OnStart.c)

	rClient.Ship.Pos.x = NMGetFloat(iMsg);
	rClient.Ship.Pos.y = NMGetFloat(iMsg);
	rClient.Ship.Pos.z = NMGetFloat(iMsg);
	rClient.Ship.Ang.y = NMGetFloat(iMsg);

	// goods
	for (i=0; i<iNetGoodsNum; i++)
	{
		ref rGood = Net_GetGoodByIndex(i);
		string sGoodName = rGood.Name;

		rClient.Ship.Goods.(sGoodName) = NMGetWord(iMsg);
	}
	
	rClient.Ship.SailState = 0.0;

	rClient.Ship.Cannons.Charge.Type = NMGetByte(iMsg);

	rClient.Ship.MaxSpeedZ = 12.0;
	rClient.Ship.MaxSpeedY = 0.6;
	rClient.Ship.SP = 100.0;

	// init borts with default values
	rClient.Ship.Cannons.Borts = "";
	for (i=0; i<4; i++)
	{
		aref arBorts; makearef(arBorts, rClient.Ship.Cannons.Borts);
		string sCurBort = sNetBortNames[i];

		arBorts.(sCurBort).MaxFireDistance = 1.0;
		arBorts.(sCurBort).ChargeRatio = 1.0;
		arBorts.(sCurBort).DamageRatio = 0.0;
	}

	rClient.Ship.Flares = sti(NCSea.Lights);
	rClient.Ship.Lights = sti(NCSea.Lights);

	ref rShip = Net_GetShipByIndex(sti(rClient.Ship.Type));
	rClient.Model = rShip.Name + "1";

	NetClient_SetTrackSettings(rClient);

	// COOP: ships created in the very first frame (before the sea and the other entities have run a few frames) come out
	// with a garbage or NaN height: every ship created on the host (server time >= 107) was fine, ships created on the
	// client machine at server time 0 mostly were not, and a stale key at the start did not help. So the ship is created
	// a little later, when the client clock has run for a moment (see NetClient_ShipCreateLater).
	rClient.CoopGame = iCoopGameNo;
	rClient.CoopReady = 0;
	rClient.CoopPolls = 0;
	if (NetClient_IsBotID(wNetClientID)) { rClient.Use = false; }		// COOP: a bot is not a player (no lists, no chat)
	if (iCoopDelayShip == 0)
	{
		// original order: the ship is created right here, inside the load message
		NetClient_LoadCreateShip(wNetClientID);
		return;
	}
	Trace("COOP: cli ship create postponed id=" + wNetClientID + " (server time " + iClientServerTime + ")");
	PostEvent("NetClient_ShipCreateLater", 50, "l", wNetClientID);
}

// COOP: waits until the client clock has run (like on the host) and creates the ship
void NetClient_ShipCreateLater()
{
	int wNetClientID = GetEventData();
	ref rClient = NetClient_GetClient(wNetClientID);

	if (!sti(rClient.Use) && !NetClient_IsBotID(wNetClientID)) { return; }
	if (sti(rClient.CoopReady)) { return; }

	rClient.CoopPolls = sti(rClient.CoopPolls) + 1;
	if (iClientServerTime < 150 && sti(rClient.CoopPolls) < 60)
	{
		PostEvent("NetClient_ShipCreateLater", 50, "l", wNetClientID);
		return;
	}
	Trace("COOP: cli ship create now id=" + wNetClientID + " polls=" + rClient.CoopPolls + " server time " + iClientServerTime);
	NetClient_LoadCreateShip(wNetClientID);
}

void NetClient_LoadCreateShip(int wNetClientID)
{
	ref rClient = NetClient_GetClient(wNetClientID);
	ref rShip = Net_GetShipByIndex(sti(rClient.Ship.Type));

	string sPath = "ships\" + rClient.Model + "\" + NetShipHullUpgrades[sti(rClient.Ship.Upgrades.Hull) - 1].str + "\";
	SetTexturePath(0, sPath);
	Trace("COOP: cli ship create id=" + wNetClientID + " island=[" + NCServer.IslandID + "] srvPos=" + rClient.Ship.Pos.x + "," + rClient.Ship.Pos.y + "," + rClient.Ship.Pos.z + " ay=" + rClient.Ship.Ang.y);
	// COOP: explicitly initialise the dynamic ship state; the client NetShip starts with garbage/NaN height otherwise
	rClient.Ship.NewPos.x = stf(rClient.Ship.Pos.x);
	rClient.Ship.NewPos.y = stf(rClient.Ship.Pos.y);
	rClient.Ship.NewPos.z = stf(rClient.Ship.Pos.z);
	rClient.Ship.Speed.x = 0.0;
	rClient.Ship.Speed.y = 0.0;
	rClient.Ship.Speed.z = 0.0;
	rClient.Ship.Ang.x = 0.0;
	rClient.Ship.Ang.z = 0.0;
	CreateEntity(rClient, "NetShip");
	Trace("COOP: cli ship entity created id=" + wNetClientID);
	SendMessage(rClient, "la", MSG_SHIP_CREATE, rShip);
	Trace("COOP: cli ship after MSG_SHIP_CREATE id=" + wNetClientID + " pos=" + rClient.Ship.Pos.x + "," + rClient.Ship.Pos.y + "," + rClient.Ship.Pos.z);
	// COOP: give the interpolation a valid first key at once (position and angle from the server, zero speed).
	// Ships that got this key were always fine (every ship on the host, t>=107), ships created while the client did not know
	// the server time yet (t=0, the key was skipped) came out with a garbage or NaN height on the client machine.
	// So the key is now always sent, with a time that is never below 100 (the range seen working).
	int iCoopSeedTime = iClientServerTime;
	if (iCoopSeedTime < 100) { iCoopSeedTime = 100; }
	SendMessage(rClient, "llffffff", MSG_SHIP_CURVES, iCoopSeedTime, stf(rClient.Ship.Pos.x), stf(rClient.Ship.Ang.y), stf(rClient.Ship.Pos.z), 0.0, 0.0, 0.0);
	Trace("COOP: cli seed curves sent id=" + wNetClientID + " t=" + iCoopSeedTime + " (server time " + iClientServerTime + ")");
	SetTexturePath(0, "");

	// create water sound
	float fX = stf(rClient.Ship.Pos.x);
	float fY = stf(rClient.Ship.Pos.y);
	float fZ = stf(rClient.Ship.Pos.z);
	rClient.Ship.Sounds.WaterID.x = 0.0;
	rClient.Ship.Sounds.WaterID.y = 0.0;
	rClient.Ship.Sounds.WaterID.z = 0.12;
	rClient.Ship.Sounds.WaterID = Play3DSoundComplex("ship_water_" + rShip.Soundtype, fX, fY, fZ, true, false);
	if (sti(rClient.Ship.Sounds.WaterID) > 0) { Sound_SetVolume(sti(rClient.Ship.Sounds.WaterID), 0.0); }
	rClient.Ship.Sounds.WaterSpeed = 30.0;

	// create sails sound
	rClient.Ship.Sounds.SailsID.x = 0.0;
	rClient.Ship.Sounds.SailsID.y = 0.5;
	rClient.Ship.Sounds.SailsID.z = 0.0;
	rClient.Ship.Sounds.SailsID = Play3DSoundComplex("sails_up_" + rShip.Soundtype, fX, fY, fZ, true, false);
	if (sti(rClient.Ship.Sounds.SailsID) > 0) { Sound_SetVolume(sti(rClient.Ship.Sounds.SailsID), 0.0); }
	rClient.Ship.Sounds.SailsMove = 0.0;

	// create turn sound
	rClient.Ship.Sounds.TurnID.x = 0.0;
	rClient.Ship.Sounds.TurnID.y = 0.0;
	rClient.Ship.Sounds.TurnID.z = 0.0;
	rClient.Ship.Sounds.TurnID = Play3DSoundComplex("ship_turn_" + rShip.Soundtype, fX, fY, fZ, true, false);
	if (sti(rClient.Ship.Sounds.TurnID) > 0) { Sound_SetVolume(sti(rClient.Ship.Sounds.TurnID), 0.0); }
	rClient.Ship.Sounds.TurnSpeed = 0.05;

	// 
	LayerAddObject("net_execute", rClient, iClientShipPriorityExecute);
	LayerAddObject("net_realize", rClient, iClientShipPriorityRealize);
	LayerAddObject("sea_reflection", rClient, 100);

	// COOP: crew on the deck (client\Sailors.c); the ship, its model and its layers are ready here
	NetClient_SailorsAddShipLater(wNetClientID);

	Event("NCEvent_ShipCreate", "l", wNetClientID);
	PostEvent("NetClient_ShipUpdateSounds", 1000, "l", wNetClientID);

	RefreshBattleInterface();

	// add ship to foam 
	SeaFoam.AddNetShip = wNetClientID;

	NetBI_SelectShipPointer(wNetClientID);

	rClient.CoopReady = 1;
}

// ================================================================================================
// COOP: bots of the admin (server\Bots.c). A bot is a slot without a player (Use = 0, like a parked ship) with Bot = 1.
// ================================================================================================
bool NetClient_IsBotID(int wID)
{
	if (wID < NET_BOT_FIRST || wID >= NET_MAXCLIENTS) { return false; }
	if (!CheckAttribute(&NCClients[wID], "Bot")) { return false; }
	return sti(NCClients[wID].Bot) != 0;
}

bool NetClient_BotRemoved(int wID)
{
	if (!CheckAttribute(&NCClients[wID], "BotRemoved")) { return false; }
	return sti(NCClients[wID].BotRemoved) != 0;
}

void NetClient_OnBot(int iMsg)
{
	int iOp = NMGetByte(iMsg);
	switch (iOp)
	{
		case NET_BOTOP_INFO:	NetClient_OnBotInfo(iMsg);		break;
		case NET_BOTOP_REMOVED:	NetClient_OnBotRemoved(iMsg);	break;
		case NET_BOTOP_NOTE:	NetClient_OnBotNote(iMsg);		break;
		case NET_BOTOP_OPTS:
			bNetCliBotRespawn = (NMGetByte(iMsg) != 0);
			Event("NCEvent_BotOpts");
		break;
	}
}

// names and relation of a bot; comes before its load message, and again when the relation changes
void NetClient_OnBotInfo(int iMsg)
{
	int wID = NMGetClientID(iMsg);
	int iNum = NMGetByte(iMsg);
	int iRel = NMGetByte(iMsg);
	int iType = NMGetWord(iMsg);
	string sNick = NMGetString(iMsg);
	int iAnnounce = NMGetByte(iMsg);

	if (wID < NET_BOT_FIRST || wID >= NET_MAXCLIENTS) { return; }

	ref rC = NetClient_GetClient(wID);
	rC.Bot = true;
	rC.BotNum = iNum;
	rC.BotRel = iRel;
	rC.BotRemoved = false;
	rC.NickName = sNick;
	rC.Use = false;
	rC.Team = 0;
	rC.Kick = false;
	rC.Ship.Name = sNick;
	if (iType >= 0 && iType < Net_GetShipsNum())
	{
		ref rShip = Net_GetShipByIndex(iType);
		rC.Ship.Name = XI_ConvertString(rShip.Name);
	}
	rC.Ship.Sails.Color = argb(255, 255, 255, 255);

	if (!CheckAttribute(rC, "BotTex"))
	{
		rC.BotTex = true;
		rC.UserData.Face = "_emptyface_";
		rC.UserData.Face.Texture = NFGetTexture("UserDataImages", sUserFacesPath + "_emptyface_.tga");
		rC.UserData.Sail = "_emptysail_";
		rC.UserData.Sail.Texture = NFGetTexture("UserDataImages", sUserSailsPath + "_emptysail_.tga");
		rC.UserData.Flag = "_emptyflag_";
		rC.UserData.Flag.Texture = NFGetTexture("UserDataImages", sUserFlagsPath + "_emptyflag_.tga");
	}

	// a new relation of a ship that exists here: the battle interface takes the new colour
	if (bNetClientInGame && COOP_CliSlotLive(wID) && CheckAttribute(rC, "CoopReady") && sti(rC.CoopReady))
	{
		NetBI_CreateShipPrivate(wID);		// the sign and the bars over the ship
		RefreshBattleInterface();
	}

	if (iAnnounce != 0) { NetClient_LogMessageNum("Net_BotAdded", iNum, 0); }
	Event("NCEvent_BotInfo", "l", wID);
}

// TEST (server\Bots.c: bNetBotRealDelete): the ship of the bot is taken out of the foam, the crew on the deck, the sinking
// effect and the battle interface (the messages that single player sends when a ship dies: sea_ai\ShipDead.c), and then the
// NetShip entity is deleted. The slot is not "live" any more, so the loops of the client skip it.
// How far the deletion goes (for finding what crashes the game: the log shows the last step done, "COOP: cli bot del ...").
//   0 = nothing is deleted: the ship sinks and stays (the old "Delete")
//   1 = the crew of the ship is taken off the deck (Sailors is rebuilt without it), the ship sinks and stays
//   2 = + the foam of Seafoam forgets the ship (the message of single player), the ship sinks and stays
//   3 = + the ship leaves the battle interface, the ship sinks and stays
//   4 = + the NetShip entity is deleted (full deletion)
int iNetBotDelStage = 4;

// The message of single player to the foam (MSG_SHIP_DELETE of Seafoam): the first ship was deleted with it, the game crashed in it
// at the second one (a manowar; the log ends after "step 1 done"). false = the foam is not told, the NetShip entity is only deleted.
bool bNetBotDelFoamMsg = false;

// EXPERIMENT: after two deletions the game crashed when the third ship was created (the log ends before "cli ship create"), so
// something that keeps its own list of ships still holds the deleted ones. Like Sailors, the engine objects with their own ship
// lists are made again right after the deletion (the way they are made at the start of a game). Each one can be switched off
// to find which of them it is. (If a rebuilt object does not know the ships that stay, those lose a detail: foam / wake / lights.)
bool bNetBotDelRebuildFoam = true;
bool bNetBotDelRebuildTracks = true;
bool bNetBotDelRebuildLights = true;

void NetClient_BotDelRebuildEngine(int wID)
{
	if (bNetBotDelRebuildFoam && IsEntity(&Seafoam))
	{
		Trace("COOP: cli bot del " + wID + " rebuild foam");
		NetClient_DeleteShipFoamEnvironment();
		NetClient_CreateShipFoamEnvironment(0);
	}
	if (bNetBotDelRebuildTracks && IsEntity(&NCShipTracks))
	{
		Trace("COOP: cli bot del " + wID + " rebuild tracks");
		DeleteClass(&NCShipTracks);
		NCShipTracks.Server = false;
		CreateEntity(&NCShipTracks, "NetShipTracks");
		LayerAddObject("net_execute", &NCShipTracks, 65531);
		LayerAddObject("net_realize", &NCShipTracks, 65531);
	}
	if (bNetBotDelRebuildLights && IsEntity(&NCShipLights))
	{
		Trace("COOP: cli bot del " + wID + " rebuild lights");
		NetClient_DeleteShipLightsEnvironment();
		NetClient_CreateShipLightsEnvironment(0);
	}
	Trace("COOP: cli bot del " + wID + " rebuild done");
}

void NetClient_BotShipDelete(int wID)
{
	ref rC = NetClient_GetClient(wID);
	rC.BotRemoved = true;
	// the ship asserts (ship.cpp: pASinkSpeed) when Dead is set before Ship.Sink.Speed exists
	rC.Ship.Sink.Speed.x = 0.0;
	rC.Ship.Sink.Speed.y = 0.9;
	rC.Ship.Sink.Speed.z = 0.0;
	rC.Dead = true;
	rC.ShipState = 2;
	if (!bNetClientInGame || !COOP_CliSlotLive(wID) || !IsEntity(rC)) { return; }

	Trace("COOP: cli bot del " + wID + " stage=" + iNetBotDelStage + " start");
	if (iNetBotDelStage >= 1)
	{
		NetClient_SailorsDeleteShip(wID);
		Trace("COOP: cli bot del " + wID + " step 1 done (crew)");
	}
	if (iNetBotDelStage >= 2)
	{
		Trace("COOP: cli bot del " + wID + " step 2: foam message=" + bNetBotDelFoamMsg);
		if (bNetBotDelFoamMsg)
		{
			if (IsEntity(&Seafoam)) { SendMessage(&Seafoam, "la", MSG_SHIP_DELETE, rC); }
			if (IsEntity(&SinkEffect)) { SendMessage(&SinkEffect, "la", MSG_SHIP_DELETE, rC); }
		}
		Trace("COOP: cli bot del " + wID + " step 2 done (foam)");
	}
	if (iNetBotDelStage >= 3)
	{
		SendMessage(&NetBInterface, "ll", BI_IN_DELETE_SHIP, wID);
		Trace("COOP: cli bot del " + wID + " step 3 done (interface)");
	}
	if (iNetBotDelStage >= 4)
	{
		rC.CoopReady = 0;
		rC.CoopGame = -1;
		Trace("COOP: cli bot del " + wID + " step 4: DeleteClass of the NetShip");
		DeleteClass(rC);
		Trace("COOP: cli bot del " + wID + " step 4 done (entity deleted)");
		NetClient_BotDelRebuildEngine(wID);
	}
	else
	{
		// the ship stays: it sinks like after the old "Delete" (the sink speed is set above)
		rC.Ship.Crew.Quantity = 0;
	}
	Event("NCEvent_ShipDead");
	Trace("COOP: cli bot del " + wID + " finished");
}

// "Delete" of the admin: the bot sinks without a kill line (or the ship is deleted, see above)
void NetClient_OnBotRemoved(int iMsg)
{
	int wID = NMGetClientID(iMsg);
	float fSX = NMGetFloat(iMsg);
	float fSY = NMGetFloat(iMsg);
	float fSZ = NMGetFloat(iMsg);
	int iGone = NMGetByte(iMsg);

	if (wID < NET_BOT_FIRST || wID >= NET_MAXCLIENTS) { return; }
	ref rC = NetClient_GetClient(wID);
	rC.BotRemoved = true;
	if (iGone != 0)
	{
		if (CheckAttribute(rC, "BotNum")) { NetClient_LogMessageNum("Net_BotRemoved", sti(rC.BotNum), 0); }
		NetClient_BotShipDelete(wID);
		Event("NCEvent_BotInfo", "l", wID);
		return;
	}
	if (CheckAttribute(rC, "BotNum")) { NetClient_LogMessageNum("Net_BotRemoved", sti(rC.BotNum), 0); }

	bool bWasDead = false;
	if (CheckAttribute(rC, "Dead") && sti(rC.Dead)) { bWasDead = true; }
	if (!bWasDead && bNetClientInGame && COOP_CliSlotLive(wID))
	{
		rC.Ship.Sink.Speed.x = fSX;
		rC.Ship.Sink.Speed.y = fSY;
		rC.Ship.Sink.Speed.z = fSZ;
		rC.Ship.Crew.Quantity = 0;
		rC.Dead = true;
		rC.ShipState = 2;
		Event("NCEvent_ShipDead");
	}
	else
	{
		rC.Dead = true;
		rC.ShipState = 2;
	}
	Event("NCEvent_BotInfo", "l", wID);
}

// answers of the server to the admin
void NetClient_OnBotNote(int iMsg)
{
	int iNote = NMGetByte(iMsg);
	int iValue = NMGetByte(iMsg);
	switch (iNote)
	{
		case NET_BOTNOTE_NOSHIP:	NetClient_LogMessageNum("Net_BotNoShip", iValue, 0);	break;
		case NET_BOTNOTE_LIMIT:		NetClient_LogMessageNum("Net_BotLimit", iValue, 0);	break;
		case NET_BOTNOTE_NOGAME:	NetClient_LogMessageNum("Net_BotNoGame", iValue, 0);	break;
		case NET_BOTNOTE_NOFORT:	NetClient_LogMessageNum("Net_BotNoFort", iValue, 0);	break;
		case NET_BOTNOTE_FORTEXISTS:	NetClient_LogMessageNum("Net_BotFortExists", iValue, 0);	break;
		case NET_BOTNOTE_FORTADDED:	NetClient_LogMessageNum("Net_BotFortAdded", iValue, 0);	break;
		case NET_BOTNOTE_FORTFIXED:	NetClient_LogMessageNum("Net_BotFortFixed", iValue, 0);	break;
		case NET_BOTNOTE_FORTMOVEOFF:	NetClient_LogMessageNum("Net_BotFortMoveOff", iValue, 0);	break;
		case NET_BOTNOTE_FORTGONE:	NetClient_LogMessageNum("Net_BotFortGone", iValue, 0);	break;
	}
}

void NetClient_SetTrackSettings(ref rClient)
{
	ref rShip = Net_GetShipByIndex(sti(rClient.Ship.Type));

	rClient.Ship.Track.TrackDistance = 1.0;

	rClient.Ship.Track.WaveHeight1 = 0.2;
	rClient.Ship.Track.WaveHeight2 = 0.9;

	rClient.Ship.Track1.Texture = "ships\trailShip.tga";
	rClient.Ship.Track1.TrackWidthSteps = 12.0;

	rClient.Ship.Track1.ZStart = rShip.Track1.ZStart;
	rClient.Ship.Track1.LifeTime = rShip.Track1.LifeTime;
	rClient.Ship.Track1.Width = rShip.Track1.Width;
	rClient.Ship.Track1.Speed = rShip.Track1.Speed;

	rClient.Ship.Track2.Texture = "ships\trailKeel.tga";
	rClient.Ship.Track2.TrackWidthSteps = 4.0;

	rClient.Ship.Track2.ZStart = rShip.Track2.ZStart;
	rClient.Ship.Track2.LifeTime = rShip.Track2.LifeTime;
	rClient.Ship.Track2.Width = rShip.Track2.Width;
	rClient.Ship.Track2.Speed = rShip.Track2.Speed;
}

void NetClient_DownloadServerFile(string sFileName, int iEventID, int iEventSubID)
{
	if (!sti(NetClient.AllowDownload)) { return; }

	int iFileID1 = NFRCreateFile(-1, sFileName, iEventID, iEventSubID);

	Trace("NetClient trying download server file: " + sFileName);

	int iSMsg = NMCreate();
	NMAddClientID(iSMsg, wClientID);
	NMAddByte(iSMsg, NC_FILE);
	NMAddByte(iSMsg, NSC_FILE_GET_FILE);
	NMAddDword(iSMsg, 0);
	NMAddDword(iSMsg, iFileID1);
	NMAddString(iSMsg, sFileName, 128);
	NetClient_SendMessage(iSMsg, true);
	NMDelete(iSMsg);
}