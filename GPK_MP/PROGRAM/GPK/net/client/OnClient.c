// COOP switch: 0 = create the client ship at once inside the load message (as in the original code),
//              1 = create it 50..3000 ms later, when the client clock has reached 150 (NetClient_ShipCreateLater).
// Two hard freezes (one on the host, one on a client) happened right after the "ship create postponed" lines, so the
// default is 0.
int iCoopDelayShip = 0;

// COOP: respawn of the bots (the switch of the admin, comes from the server: NET_BOTOP_OPTS)
bool bNetCliBotRespawn = false;
bool bNetCliLateJoin = true;		// COOP s30: "Join during the game" of the server (the admin menu)


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
		case NSC_CLIENT_TOLOBBY:		// COOP s31: a player (the host) went to the lobby, the game goes on (server\OnStart.c)
			int wTlID = NMGetClientID(iMsg);
			if (wTlID >= 0 && wTlID < NET_MAXPLAYERS && wTlID != wClientID)
			{
				NetLand_PlayerLeft(wTlID);
				string sTlNick = NetClient_PlayerNick(wTlID);
				if (sTlNick != "") { NetClient_ChatAddSystemLineEx(sTlNick + " גûרוכ ג כמבבט (במי ןנמהמכזאועסÿ)", false); }
			}
		break;
		case NSC_CLIENT_CHOICE:		// COOP s30: the choice of the lobby of a player (interfaces\Buy.c: Buy_PeekPlayer)
			int wChID = NMGetClientID(iMsg);
			string sChoice = NMGetString(iMsg);
			if (wChID >= 0 && wChID < NET_TOTALCLIENTS) { NCClients[wChID].LobbyChoice = sChoice; }
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
	string sNpcTex = NetLand_NpcFaceTex(sFaceImage);		// COOP: an NPC skin of the profile (net\Land.c)
	if (sNpcTex != "") { sFaceTex = sNpcTex; }
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

	Trace("COOP: client " + wNetClientID + " (" + sNickName + "): flag <" + sFlagImage + "> tex " + rClient.UserData.Flag.Texture + ", gerald <" + sSailImage + "> tex " + rClient.UserData.Sail.Texture + ", face <" + sFaceImage + ">");
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
	rClient.SeaPerks = NMGetDword(iMsg);		// COOP s28: the sea perks bought in the lobby (common\Perks.c)

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

	// COOP s27: a second load message of a slot whose ship exists (a reconnect, a repeated load): the old NetShip is deleted the
	// safe way first. CreateEntity over a live ship left the old one in the foam, the crew and the battle interface.
	if (IsEntity(rClient))
	{
		Trace("COOP: cli ship create id=" + wNetClientID + ": the old ship entity still exists, it is deleted first");
		NetClient_SailorsDeleteShip(wNetClientID);
		SendMessage(&NetBInterface, "ll", BI_IN_DELETE_SHIP, wNetClientID);
		NetClient_ShipReleaseSounds(rClient);
		rClient.CoopReady = 0;
		DeleteClass(rClient);
		NetClient_BotDelRebuildEngine(wNetClientID);
	}

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
	// COOP s27: one loop of the random sounds per slot (the ship may be created again: respawn, repair of the masts)
	if (!CheckAttribute(rClient, "SndLoop") || !sti(rClient.SndLoop))
	{
		rClient.SndLoop = 1;
		PostEvent("NetClient_ShipUpdateSounds", 1000, "l", wNetClientID);
	}

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
		case NET_BOTOP_LOBBY:	NetClient_OnLobbyBots(iMsg);	break;		// COOP s30
		case NET_BOTOP_OPTS:
			bNetCliBotRespawn = (NMGetByte(iMsg) != 0);
			NetLand_SetBotSkill(NMGetByte(iMsg));		// COOP: the difficulty of the bots (the admin menu can change it)
			bNetCliLateJoin = (NMGetByte(iMsg) != 0);		// COOP s30
			Event("NCEvent_BotOpts");
		break;
	}
}

// COOP s30: the bots of the lobby (server\Bots.c: NetServer_LBotsSend): NCLobbyBots.n, b0..: rel, ship, cannon, crew, hull, sails
object NCLobbyBots;

void NetClient_OnLobbyBots(int iMsg)
{
	DeleteAttribute(&NCLobbyBots, "");
	int n = NMGetByte(iMsg);
	NCLobbyBots.n = n;
	for (int k=0; k<n; k++)
	{
		string sB = "b" + k;
		NCLobbyBots.(sB).rel = NMGetByte(iMsg);
		NCLobbyBots.(sB).ship = NMGetWord(iMsg);
		NCLobbyBots.(sB).cannon = NMGetByte(iMsg);
		NCLobbyBots.(sB).crew = NMGetByte(iMsg);
		NCLobbyBots.(sB).hull = NMGetByte(iMsg);
		NCLobbyBots.(sB).sails = NMGetByte(iMsg);
	}
	Event("NCEvent_LobbyBots");
}

int NetClient_LobbyBotsNum()
{
	if (!CheckAttribute(&NCLobbyBots, "n")) { return 0; }
	return sti(NCLobbyBots.n);
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
	int iBotFaceW = NMGetWord(iMsg);		// COOP s41: the portrait of its captain + 1 (0 = none)

	if (wID < NET_BOT_FIRST || wID >= NET_MAXCLIENTS) { return; }

	ref rC = NetClient_GetClient(wID);
	rC.Bot = true;
	rC.BotNum = iNum;
	rC.BotRel = iRel;
	rC.BotRemoved = false;
	rC.NickName = sNick;
	rC.Use = false;
	rC.Team = 0;
	if (iRel >= NET_BOTREL_TEAM0) { rC.Team = iRel - NET_BOTREL_TEAM0; }		// COOP s28: a bot of a team
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
	// COOP s41: the portrait of the captain of the bot (the skin of its captain on the boarding deck is the same face)
	if (iBotFaceW > 0)
	{
		string sBotFace = NetLand_FaceNumName(iBotFaceW - 1);		// COOP s45: face_N or npc_N (net\Land.c)
		if (!CheckAttribute(rC, "BotFace") || rC.UserData.Face != sBotFace)
		{
			rC.BotFace = iBotFaceW - 1;
			rC.UserData.Face = sBotFace;
			string sBotTex = "interfaces\portraits\256\" + sBotFace + ".tga";
			string sBotNpcTex = NetLand_NpcFaceTex(sBotFace);
			if (sBotNpcTex != "") { sBotTex = sBotNpcTex; }
			rC.UserData.Face.Texture = NFGetTexture("UserDataImages", sBotTex);
			if (!sti(rC.UserData.Face.Texture)) { rC.UserData.Face.Texture = NFGetTexture("UserDataImages", sUserFacesPath + "_emptyface_.tga"); }
		}
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

// The deletion of a NetShip entity on a client WORKS (tested with "Delete" of the admin menu, s27): the crew of the deck is taken
// off, the ship leaves the battle interface, its sounds are released, the entity is deleted and then the foam is made again.
// The only thing that crashed was the message of single player to the foam (MSG_SHIP_DELETE of Seafoam, bNetBotDelFoamMsg):
// Seafoam keeps its own list of the ships, so it is deleted and created again instead (it learns the ships that stay again).
// All the deletions of ships go this way now: NetClient_ShipEntityDelete / NetClient_ShipEntityRecreate below.
//   iNetBotDelStage: how far "Delete" of a bot goes (the log shows the last step done, "COOP: cli bot del ...").
//   0 = nothing is deleted: the ship sinks and stays; 1 = the crew; 2 = + the foam; 3 = + the battle interface; 4 = full deletion
int iNetBotDelStage = 4;

// The message of single player to the foam (MSG_SHIP_DELETE of Seafoam) crashed the game at the second deletion: keep it false,
// the foam is made again instead (bNetBotDelRebuildFoam).
bool bNetBotDelFoamMsg = false;

// The engine objects with their own lists of ships that are made again after a deletion. The foam must be (see above); with the
// tracks and the lights rebuilt too the game crashed in the first frame after the deletion, so they stay (they forget a deleted
// ship themselves).
bool bNetBotDelRebuildFoam = true;
bool bNetBotDelRebuildTracks = false;
bool bNetBotDelRebuildLights = false;

void NetClient_BotDelRebuildEngine(int wID)
{
	if (bNetBotDelRebuildFoam && IsEntity(&Seafoam))
	{
		Trace("COOP: cli bot del " + wID + " rebuild foam");
		NetClient_DeleteShipFoamEnvironment();
		NetClient_CreateShipFoamEnvironment(0);
		// COOP s27: the new foam does not know the ships that stay: they are added again (the way NetClient_LoadCreateShip adds one)
		for (int iFoam=0; iFoam<NET_MAXCLIENTS; iFoam++)
		{
			if (iFoam == wID) { continue; }
			if (!COOP_CliSlotLive(iFoam)) { continue; }
			if (!CheckAttribute(&NCClients[iFoam], "CoopReady")) { continue; }
			if (!sti(NCClients[iFoam].CoopReady)) { continue; }
			if (!IsEntity(&NCClients[iFoam])) { continue; }
			if (CheckAttribute(&NCClients[iFoam], "Dead"))
			{
				if (sti(NCClients[iFoam].Dead)) { continue; }
			}
			SeaFoam.AddNetShip = iFoam;
		}
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

// COOP s27: the looped sounds of a ship (water, sails, turn) are released before its entity is deleted (they played on otherwise)
void NetClient_ShipReleaseSounds(ref rC)
{
	if (CheckAttribute(rC, "Ship.Sounds.WaterID"))
	{
		if (sti(rC.Ship.Sounds.WaterID) > 0) { ReleaseSound(sti(rC.Ship.Sounds.WaterID)); }
	}
	if (CheckAttribute(rC, "Ship.Sounds.SailsID"))
	{
		if (sti(rC.Ship.Sounds.SailsID) > 0) { ReleaseSound(sti(rC.Ship.Sounds.SailsID)); }
	}
	if (CheckAttribute(rC, "Ship.Sounds.TurnID"))
	{
		if (sti(rC.Ship.Sounds.TurnID) > 0) { ReleaseSound(sti(rC.Ship.Sounds.TurnID)); }
	}
	rC.Ship.Sounds.WaterID = 0;
	rC.Ship.Sounds.SailsID = 0;
	rC.Ship.Sounds.TurnID = 0;
}

// COOP s27: a camera of this machine looks at the ship (the cameras of net.dll assert when their ship does not exist)
bool NetClient_ShipCameraOn(int wID)
{
	if (wID == wClientID) { return true; }
	if (CheckAttribute(&NCDeckCamera, "NetID"))
	{
		if (sti(NCDeckCamera.NetID) == wID) { return true; }
	}
	if (CheckAttribute(&NCShipCamera, "NetID"))
	{
		if (sti(NCShipCamera.NetID) == wID) { return true; }
	}
	return false;
}

// COOP s27: the safe deletion of the NetShip entity of a ship on this machine, the steps of "Delete" of the admin menu.
// bKeepSlot = true: the ship is created again at once (NetClient_ShipEntityRecreate); false: the ship is gone (sunk long ago)
bool NetClient_ShipEntityDelete(int wID, string sWhy, bool bKeepSlot)
{
	ref rC = NetClient_GetClient(wID);
	if (!bNetClientInGame) { return false; }
	if (!COOP_CliSlotLive(wID)) { return false; }
	if (!IsEntity(rC)) { return false; }
	if (!bKeepSlot)
	{
		// the ship camera looks at it (after our death: the killer): it goes back to our own ship, which is never deleted alone
		if (CheckAttribute(&NCShipCamera, "NetID"))
		{
			if (sti(NCShipCamera.NetID) == wID) { NCShipCamera.NetID = wClientID; }
		}
		if (NetClient_ShipCameraOn(wID))
		{
			Trace("COOP: cli ship del " + wID + " (" + sWhy + ") skipped: a camera of this machine is on it");
			return false;
		}
	}
	Trace("COOP: cli ship del " + wID + " (" + sWhy + ") start");
	NetClient_SailorsDeleteShip(wID);
	SendMessage(&NetBInterface, "ll", BI_IN_DELETE_SHIP, wID);
	NetClient_ShipReleaseSounds(rC);
	rC.CoopReady = 0;
	if (!bKeepSlot)
	{
		rC.CoopGame = -1;		// the loops of the client skip the slot
		rC.ShipGone = true;
	}
	DeleteClass(rC);
	Trace("COOP: cli ship del " + wID + " entity deleted");
	NetClient_BotDelRebuildEngine(wID);
	if (!bKeepSlot)
	{
		RefreshBattleInterface();
		NetBI_MainShipAgain();		// COOP s27: the interface keeps our ship as its main one (the ring of the cannons)
	}
	Trace("COOP: cli ship del " + wID + " done");
	return true;
}

// COOP s27: delete the ship and create it again at x, z, ay (whole masts, sails and hull model): the respawn, the repair of the
// masts. A ship that is gone (NetClient_ShipEntityDelete with bKeepSlot = false) is only created. false = nothing was done.
bool NetClient_ShipEntityRecreate(int wID, float x, float z, float ay, string sWhy)
{
	if (!bNetClientInGame) { return false; }
	if (wID < 0 || wID >= NET_MAXCLIENTS) { return false; }
	ref rC = NetClient_GetClient(wID);
	if (!CheckAttribute(rC, "Ship.Type")) { return false; }
	if (sti(rC.Ship.Type) < 0) { return false; }
	bool bGone = false;
	if (CheckAttribute(rC, "ShipGone")) { bGone = sti(rC.ShipGone); }
	if (!bGone)
	{
		if (!COOP_CliSlotLive(wID)) { return false; }
		if (!CheckAttribute(rC, "CoopReady")) { return false; }
		if (!sti(rC.CoopReady)) { return false; }		// not created yet: the normal creation comes
		if (!NetClient_ShipEntityDelete(wID, sWhy, true)) { return false; }
	}
	rC.CoopGame = iCoopGameNo;
	rC.ShipGone = false;
	DeleteAttribute(rC, "MastFall");
	DeleteAttribute(rC, "Ship.Masts");
	DeleteAttribute(rC, "Ship.Sink");
	rC.Ship.Pos.x = x;
	rC.Ship.Pos.y = 0.0;
	rC.Ship.Pos.z = z;
	rC.Ship.Ang.y = ay;
	NetClient_LoadCreateShip(wID);
	// the cameras find their ship again (the same NetID, a new entity)
	if (CheckAttribute(&NCDeckCamera, "NetID")) { NCDeckCamera.NetID = sti(NCDeckCamera.NetID); }
	if (CheckAttribute(&NCShipCamera, "NetID")) { NCShipCamera.NetID = sti(NCShipCamera.NetID); }
	Trace("COOP: cli ship " + wID + " created again (" + sWhy + ") at x=" + x + " z=" + z);
	return true;
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
		if (CheckAttribute(&NCShipCamera, "NetID"))		// COOP s27: our camera of the ship does not stay on a deleted ship
		{
			if (sti(NCShipCamera.NetID) == wID) { NCShipCamera.NetID = wClientID; }
		}
		NetClient_ShipReleaseSounds(rC);		// COOP s27: the water / sails sounds played on
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
		case NET_BOTNOTE_LANDFULL:	NetClient_LogMessageNum("Net_BotLandFull", iValue, 0);	break;		// COOP s46
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

// COOP s43: net.dll sends Net_ShipStartLoad / Net_ShipEndLoad (the attributes of the NCClients / NSClients slot) around the load
// of the model of every NetShip. Nobody handled them: the ships of the server (the host) and the ships made again loaded the
// hull textures with an empty texture path ("Can't load texture resource\textures\XEBECVML.TGA"). Now the path of the hull is set
// like in NetClient_ShipCreate (client\OnClient.c).
void Net_ShipStartLoad()
{
	aref rC = GetEventData();
	if (!CheckAttribute(rC, "Model")) { return; }
	int iHull = 1;
	if (CheckAttribute(rC, "Ship.Upgrades.Hull")) { iHull = sti(rC.Ship.Upgrades.Hull); }
	if (iHull < 1 || iHull > 3) { iHull = 1; }
	string sHullStr = NetShipHullUpgrades[iHull - 1].str;
	string sModel = rC.Model;
	string sPath = "ships\" + sModel + "\" + sHullStr + "\";
	SetTexturePath(0, sPath);
}

void Net_ShipEndLoad()
{
	SetTexturePath(0, "");
}
