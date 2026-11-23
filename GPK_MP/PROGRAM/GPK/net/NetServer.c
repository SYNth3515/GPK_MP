#include "GPK\Net\Server\OnCamera.c"
#include "GPK\Net\Server\OnBall.c"
#include "GPK\Net\Server\OnCommand.c"
#include "GPK\Net\Server\Admin.c"
#include "GPK\Net\Server\Bots.c"
#include "GPK\Net\Server\OnConnect.c"
#include "GPK\Net\Server\OnNetMessage.c"
#include "GPK\Net\Server\OnSay.c"
#include "GPK\Net\Server\OnShip.c"
#include "GPK\Net\Server\OnLoad.c"
#include "GPK\Net\Server\OnPing.c"
#include "GPK\Net\Server\OnStart.c"
#include "GPK\Net\Server\Utils.c"
#include "GPK\Net\Server\Rigging.c"
#include "GPK\Net\Server\Island.c"
#include "GPK\Net\Server\Touch.c"
#include "GPK\Net\Server\Time.c"
#include "GPK\Net\Server\File.c"
#include "GPK\Net\Server\Buy.c"
#include "GPK\Net\Server\Fort.c"
#include "GPK\Net\Server\ServerInfo.c"
#include "GPK\Net\Server\LocalServer.c"
#include "GPK\Net\Server\DedicatedServer.c"
#include "GPK\Net\Server\Weather\Weather.c"
#include "GPK\Net\Server\Games\Convoy.c"
#include "GPK\Net\Server\Games\DeathMatch.c"
#include "GPK\Net\Server\Games\Fort.c"
#include "GPK\Net\Server\Games\Naval.c"

#define NET_TOTALCLIENTS			255		// total massive
#define NET_MAXPLAYERS				32		// COOP: slots of the players (0..31; s46: 16 before)
#define NET_MAXCLIENTS				82		// absolute maximum is 254. COOP: 32 players + 50 bots
#define NET_BOT_FIRST				32		// COOP: the bots of the admin live in the slots 32..81 (server\Bots.c)
#define NET_BOT_MAX					50
#define NET_FORTCLIENT				253

#define MAX_TOPPLAYERS				100

#define MASTERSERVER_UPDATETIME		15000	// P2P: short interval keeps the NAT mapping to the master server alive

int		NSSortedPlayers[MAX_TOPPLAYERS];
object	NSClients[NET_TOTALCLIENTS];	// current game clients
object	NSBanList;						// server ban list

object NSPlayers;						// All players in server for last month

int iServerTime;
int iPingTime;
bool bNetServerIsStarted = false;
bool bNetCSEdit = false;		// COOP s30: the Create game screen edits the running server ("Game settings" of the lobby)
float fServerStartGameSpeed = 1.0;	// COOP: the game speed of the running game (the choice of the Create game screen)

void Net_DeleteServer()
{
	DelEventHandler("NetServer_UpdateMasterServerInfo", "NetServer_UpdateMasterServerInfo");

	// COOP s46: the server entity goes LAST. net.dll keeps its pointer in a global that its destructor does not clear, and the
	// natives NetServer_SendMessage / NetServer_ClearPacketsQueue / NetServer_SendMasterServerMessage use it without a check:
	// NetServer_EndGame below wrote into the freed server and the heap broke (crash in ntdll after "end: client done" when the
	// host disconnected from the lobby of a running game).
	if (!NetServer_IsStarted()) { DeleteClass(&NetServer); return; }

	if (sti(NetServer.InetServer))
	{
		int iSMsg = NMCreate();
		NMAddByte(iSMsg, NC_MASTERSERVER);
		NMAddByte(iSMsg, NSC_MASTERSERVER_DELSERVER);
		NetServer_SendMasterServerMessage(sMasterServerAddress, iMasterServerPort, iSMsg);
		NMDelete(iSMsg);
	}

	NetServer_EndGame(false);

	// save global server massives
	Net_SaveFile(true, &NSBanList, "BanList.nsv");
	Net_SaveFile(true, &NSPlayers, "Players.nsv");

	// clear all server massives
	DeleteAttribute(&NSBanList, "");
	DeleteAttribute(&NSClients, "");
	DeleteAttribute(&NSPlayers, "");

	DelEventHandler("ExitApplication", "NetServer_ExitApplication");
	DelEventHandler("NetServer_OnNetMessage", "NetServer_OnNetMessage");
	DelEventHandler("frame", "NetServer_OneSecondTick");

	Trace("COOP: server deleted");
	DeleteClass(&NetServer);

	iSecondsToStartGame = -1;
	bServerStartRequested = false;
	bNetServerIsStarted = false;
	NetServer.StopServer = "";
}

void Net_ServerFirstInit()
{
	NetServer.ServerName = "Default Server Name";
	NetServer.ServerPort = NET_DEFAULT_SERVER_PORT;
	NetServer.MaxClients = NET_MAXPLAYERS;
	NetServer.ServerPassword = "";
	NetServer.AdminPassword = "";		// COOP: empty = nobody can become an admin
	NetServer.LanServer = true;
	NetServer.InetServer = false;
	NetServer.P2P = false;
	NetServer.UpdateMasterServer = true;
	NetServer.MaxFps = 100;
	NetServer.WeatherID = "Night_sky_minor_clouds";
	NetServer.IslandID = "";
	NetServer.LandLoc = "";		// COOP: land duel location (Land.c), empty = sea
	NetServer.Tornado = 0;		// COOP: 1 = the Tornado weather (storm + tornado, server\weather\Tornado.c)
	NetServer.GameType = NETGAME_DEATHMATCH;
	NetServer.AllowDownload = false;
	NetServer.AllowUpload = false;
	NetServer.GameSpeed = 0;
	NetServer.Respawn = false;			// COOP: CS-like respawn (Create game screen)
	NetServer.BotRespawn = false;		// COOP: respawn of the bots (admin menu, "Bots")
	NetServer.NoBoarding = false;		// COOP: boarding is switched off
	NetServer.FastBoarding = true;		// COOP s37: off = the captains fight on the deck (net\Board.c)
	NetServer.LateJoin = true;			// COOP s30: new players can join the running game (Create game screen, admin menu)
	NetServer.BotSkill = 2;			// COOP: difficulty of the bots 0..3 (server\Bots.c)
	NetServer.PassivePerks = false;		// COOP: all passive sea perks of single player for every captain
	NetServer.ActivePerks = false;		// COOP: active sea perks of single player (and the brander) for every captain
}

// COOP s30: the game of the server (map, mode, teams, money, switches): the Create game screen, and its "Game settings"
// mode from the lobby (NetServer_ApplySettings) for the running server
void NetServer_SetGameParams(ref rSrvParams)
{
	NetServer.WeatherID = rSrvParams.WeatherID;
	NetServer.IslandID = rSrvParams.IslandID;
	NetServer.LandLoc = "";
	if (CheckAttribute(rSrvParams, "LandLoc")) { NetServer.LandLoc = rSrvParams.LandLoc; }
	NetServer.Tornado = 0;
	if (CheckAttribute(rSrvParams, "Tornado")) { NetServer.Tornado = sti(rSrvParams.Tornado); }
	NetServer.GameType = sti(rSrvParams.GameType);
	NetServer.NumTeams = sti(rSrvParams.NumTeams);
	NetServer.AllowDownload = sti(rSrvParams.AllowDownload);
	NetServer.AllowUpload = sti(rSrvParams.AllowUpload);
	NetServer.MaxShipClass = sti(rSrvParams.MaxShipClass);
	NetServer.Credit = sti(rSrvParams.Credit);
	NetServer.StartPosition = sti(rSrvParams.StartPosition);
	NetServer.TimeLimit = sti(rSrvParams.TimeLimit);
	// COOP: the game speed chosen on the Create game screen (0.5 / 0.75 / 1 / 2 ... 8). It is kept here, because
	// NetServer.GameSpeed is set again when the game starts (server/OnStart.c): a bad / missing value means 1x
	fServerStartGameSpeed = 1.0;
	if (CheckAttribute(rSrvParams, "GameSpeed")) { fServerStartGameSpeed = stf(rSrvParams.GameSpeed); }
	if (fServerStartGameSpeed < 0.5) { fServerStartGameSpeed = 1.0; }
	if (fServerStartGameSpeed > 8.0) { fServerStartGameSpeed = 8.0; }
	NetServer.GameSpeed = fServerStartGameSpeed;

	// COOP: switches of the Create game screen (missing attributes = old behaviour)
	NetServer.Respawn = false;
	NetServer.BotRespawn = false;
	NetServer.NoBoarding = false;
	NetServer.PassivePerks = false;
	NetServer.ActivePerks = false;
	if (CheckAttribute(rSrvParams, "Respawn")) { NetServer.Respawn = sti(rSrvParams.Respawn); }
	NetServer.RespawnTime = NET_RESPAWN_DELAY / 1000;
	if (CheckAttribute(rSrvParams, "RespawnTime")) { NetServer.RespawnTime = sti(rSrvParams.RespawnTime); }
	if (sti(NetServer.RespawnTime) < NET_RESPAWN_TIME_MIN) { NetServer.RespawnTime = NET_RESPAWN_TIME_MIN; }
	if (sti(NetServer.RespawnTime) > NET_RESPAWN_TIME_MAX) { NetServer.RespawnTime = NET_RESPAWN_TIME_MAX; }
	if (CheckAttribute(rSrvParams, "NoBoarding")) { NetServer.NoBoarding = sti(rSrvParams.NoBoarding); }
	NetServer.FastBoarding = true;		// COOP s37
	if (CheckAttribute(rSrvParams, "FastBoarding")) { NetServer.FastBoarding = sti(rSrvParams.FastBoarding); }
	NetServer.LateJoin = true;
	if (CheckAttribute(rSrvParams, "LateJoin")) { NetServer.LateJoin = sti(rSrvParams.LateJoin); }
	NetServer.PistolReload = false;		// COOP s31: a land duel: the pistols load in the fight (net/Land.c)
	if (CheckAttribute(rSrvParams, "PistolReload")) { NetServer.PistolReload = sti(rSrvParams.PistolReload); }
	NetServer.BotSkill = 2;
	if (CheckAttribute(rSrvParams, "BotSkill")) { NetServer.BotSkill = sti(rSrvParams.BotSkill); }
	if (CheckAttribute(rSrvParams, "PassivePerks")) { NetServer.PassivePerks = sti(rSrvParams.PassivePerks); }
	if (CheckAttribute(rSrvParams, "ActivePerks")) { NetServer.ActivePerks = sti(rSrvParams.ActivePerks); }
	Trace("COOP: server switches respawn=" + NetServer.Respawn + " (" + NetServer.RespawnTime + " s) noboarding=" + NetServer.NoBoarding + " fast boarding=" + NetServer.FastBoarding + " passive perks=" + NetServer.PassivePerks + " active perks=" + NetServer.ActivePerks);
}

void Net_CreateServer(ref rSrvParams)
{
	Trace("COOP: Net_CreateServer enter");
	DeleteAttribute(&NSLobbyBots, "");		// COOP s30: the bots of the lobby belong to one server (server\Bots.c)
	iServerTime = 0;
	iPingTime = 0;
	bServerStartRequested = false;

	for (int i=0; i<NET_TOTALCLIENTS; i++)
	{
		DeleteAttribute(&NSClients[i], "");
		NSClients[i].index = i;
		NSClients[i].id = i;
		NSClients[i].Use = false;
		NSClients[i].BuyReady = false;
		NSClients[i].StartReady = false;
		NSClients[i].Server = true;			// netserver clients
		NSClients[i].Team = 0;
		NSClients[i].LastPacketIndex = 0;

		NSClients[i].LastPingTime = iPingTime;

		NSClients[i].Stat.Sunk = 0;
		NSClients[i].Stat.Wreck = 0;
		NSClients[i].Stat.Misses = 0;
		NSClients[i].Stat.Hits = 0;
		NSClients[i].Stat.Won = 0;
		NSClients[i].Stat.Lost = 0;
		NSClients[i].Stat.DamageInflicted = 0;
		NSClients[i].Stat.Deaths = 0;

		NSClients[i].Spawned = false;		// COOP: the ship of this slot was created in the current game (the slot is "burned" for new players)
		NSClients[i].InGame = false;		// COOP: the client got the environment of the running game (lobby clients do not)
		NSClients[i].Parked = false;		// COOP: the player left the running game, his ship waits for a reconnect
	}

	NetServer.ServerName = rSrvParams.ServerName;
	NetServer.ServerPort = sti(rSrvParams.ServerPort);
	NetServer.ServerPassword = rSrvParams.ServerPassword;
	NetServer.AdminPassword = rSrvParams.AdminPassword;	// COOP: set on the "Create game" screen, empty = no admins
	NetServer.Dedicated = sti(rSrvParams.Dedicated);
	NetServer.P2P = sti(rSrvParams.P2P);	// P2P: answer punch requests of the master server (no port forwarding needed)
	NetServer.MaxClients = sti(rSrvParams.MaxPlayers);
	NetServer.LanServer = sti(rSrvParams.LanServer);
	NetServer.InetServer = sti(rSrvParams.InetServer);
	Trace("COOP: InetServer = " + NetServer.InetServer);
	NetServer.UpdateMasterServer = sti(rSrvParams.UpdateMasterServer);
	NetServer.UpdateMasterServer = 0;	// COOP
	NetServer.MaxFps = sti(rSrvParams.MaxFps);
	NetServer_SetGameParams(rSrvParams);		// COOP s30: also "Game settings" of the lobby (NetServer_ApplySettings)

	NetServer.GameOver = false;
	NetServer.ContestStarted = false;	// COOP: at least two sides have fought in this game (a lonely captain does not "win")
	NetServer.Round = 0;
	
	NetServer.ConvoyShipID = DST_INVALID;
	NetServer.ConvoyPoint.x = 0.0;
	NetServer.ConvoyPoint.z = 0.0;

	Trace("COOP: before CreateEntity NetServer");
	CreateEntity(&NetServer, "NetServer");
	Trace("COOP: after CreateEntity NetServer");
	NetServer.StartServer = "";
	Trace("COOP: after StartServer attr");

	Net_LoadFile(true, &NSBanList, "BanList.nsv");
	Net_LoadFile(true, &NSPlayers, "Players.nsv");
	NetServer_LoadSpawnLines();		// COOP s30: the spawn lines of the host (server\OnStart.c)
	Trace("COOP: after Net_LoadFile x2");
	
	SetEventHandler("NetServer_OnNetMessage", "NetServer_OnNetMessage", 0);
	SetEventHandler("ExitApplication", "NetServer_ExitApplication", 0);
	SetEventHandler("frame", "NetServer_OneSecondTick", 0);

	bNetServerIsStarted = true;
	Trace("COOP: event handlers set");

	NetServer.wWinnerClientID = DST_INVALID;
	NetServer.iWinnerTeam = -1;

	// sort NSPlayers
	NetServer_SortPlayers();
	Trace("COOP: after SortPlayers");

	// send to master server message
	if (sti(NetServer.InetServer))
	{
		int iSMsg = NMCreate();
		NMAddByte(iSMsg, NC_MASTERSERVER);
		NMAddByte(iSMsg, NSC_MASTERSERVER_ADDSERVER);
		NMAddByte(iSMsg, NET_SCRIPT_VERSION);
		NetServer_SendMasterServerMessage(sMasterServerAddress, iMasterServerPort, iSMsg);
		NMDelete(iSMsg);

		SetEventHandler("NetServer_UpdateMasterServerInfo", "NetServer_UpdateMasterServerInfo", 0);
		PostEvent("NetServer_UpdateMasterServerInfo", MASTERSERVER_UPDATETIME);
	}
}

void NetServer_UpdateMasterServerInfo()
{
	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_MASTERSERVER);
	NMAddByte(iSMsg, NSC_MASTERSERVER_UPDATESERVER);
	NMAddByte(iSMsg, NET_SCRIPT_VERSION);
	NetServer_SendMasterServerMessage(sMasterServerAddress, iMasterServerPort, iSMsg);
	NMDelete(iSMsg);

	PostEvent("NetServer_UpdateMasterServerInfo", MASTERSERVER_UPDATETIME);
}

void NetServer_OneSecondTick()
{
	int iFullDeltaTime = MakeInt(fHighPrecisionDeltaTime * 1000000.0);
	int iDeltaTime = iFullDeltaTime / 1000;
	// COOP: a long frame (loading of the sea / island on this machine) must not look like silence of the clients
	if (iDeltaTime > 1000) { iDeltaTime = 1000; }

	int iNewPingTime = iPingTime + iDeltaTime;

	// COOP: the running game always has a real game speed: 0 is "not set", it must not freeze the ships
	if (bServerGameStarted && stf(NetServer.GameSpeed) < 0.5) { NetServer.GameSpeed = fServerStartGameSpeed; }

	// COOP: the ping timeout works in the running game too (with a longer limit: loading the sea stops the pings for a while).
	// A client that crashed or lost the connection is dropped, his ship is parked and he can reconnect (server\OnConnect.c).
	int iPingLimit = 15 * 1000;
	if (bServerGameStarted) { iPingLimit = NET_INGAME_PING_TIMEOUT; }
	if (iNewPingTime / 1000 != iPingTime / 1000)
	{
		for (int i=0; i<NET_MAXCLIENTS; i++)
		{
			if (!sti(NSClients[i].Use)) { continue; }
			
			if (iPingTime - sti(NSClients[i].LastPingTime) >= iPingLimit)
			{
				Trace("COOP: client " + i + " ping timeout, dropped");
				NetServer_DropClient(sti(NSClients[i].ID));
			}
		}
	}

	iPingTime = iNewPingTime;
}

void NetServer_ExitApplication()
{
	Net_DeleteServer();
}

bool NetServer_IsStarted()
{
	return bNetServerIsStarted;
}

int NetServer_GetClientsNum()
{
	int iNumClients = 0;
	for (int i=0; i<NET_MAXCLIENTS; i++) { if (sti(NSClients[i].Use)) { iNumClients++; } }
	return iNumClients;
}

void NetServer_SortPlayers()
{
	int i;
	Trace("COOP: SortPlayers enter");
	// COOP: on a clean install Players.nsv does not exist, so NSPlayers.Players is missing -> null aref crash
	if (!CheckAttribute(NSPlayers, "Players")) { NSPlayers.Players = ""; Trace("COOP: NSPlayers.Players created"); }
	aref arPlayers; makearef(arPlayers, NSPlayers.Players);
	int iNumPlayers = GetAttributesNum(arPlayers);
	Trace("COOP: SortPlayers: got players num");
	
	int iRealDays = iRealYear * (iRealMonth * 31 + iRealDay);

	// kill players who offline > 30 days, fake counting ofcourse, but who care?
	for (i=0; i<iNumPlayers; i++)
	{
		aref arPlayer = GetAttributeN(arPlayers, i);

		int iYear = sti(arPlayer.LastLogin.Year);
		int iMonth = sti(arPlayer.LastLogin.Month);
		int iDay = sti(arPlayer.LastLogin.Day);
		
		if (iYear * (iMonth * 31 + iDay) - iRealDays > 31)
		{
			DeleteAttribute(arPlayers, GetAttributeName(arPlayer));
			i = i - 1;
			continue;
		}
	}

	iNumPlayers = GetAttributesNum(arPlayers);
	for (i=0; i<iNumPlayers; i++) 
	{ 
		if (i >= MAX_TOPPLAYERS) { break; }
		NSSortedPlayers[i] = i; 
	}

	// sort players using rank and (second condition) Ship Sunks
	if (iNumPlayers > 0) { Net_Sort2i(arPlayers, &NSSortedPlayers, 1, 100, "Rank", "ShipsSunk"); }	// COOP: skip native sort on empty list
	Trace("COOP: SortPlayers done");
}

// update global player statistics
void NetServer_UpdateGlobalPlayerStatistics(ref rPlayer)
{
	aref arPlayers; makearef(arPlayers, NSPlayers.Players);
	aref arPlayer;

	string sNickName = rPlayer.NickName;
	string sNickPassword = rPlayer.NickPassword;
	
	// add new player if not exist
	if (!CheckAttribute(arPlayers, sNickName)) 
	{ 
		arPlayers.(sNickName) = ""; 

		arPlayers.(sNickName).Nickname = sNickName;
		arPlayers.(sNickName).NickPassword = sNickPassword;
		arPlayers.(sNickName).FaceImage = rPlayer.UserData.Face;
		arPlayers.(sNickName).Hits = 0;
		arPlayers.(sNickName).Misses = 0;
		arPlayers.(sNickName).Won = 0;
		arPlayers.(sNickName).Lost = 0;
		arPlayers.(sNickName).ShipsSunk = 0;
		arPlayers.(sNickName).Shipwrecks = 0;
	}

	makearef(arPlayer, arPlayers.(sNickName));
	// Update player's avatar on every statistics save
	arPlayer.FaceImage = rPlayer.UserData.Face;
	int iHits = sti(arPlayer.Hits) + sti(rPlayer.Stat.Hits);
	int iMisses = sti(arPlayer.Misses) + sti(rPlayer.Stat.Misses);
	int iWon = sti(arPlayer.Won) + sti(rPlayer.Stat.Won);
	int iLost = sti(arPlayer.Lost) + sti(rPlayer.Stat.Lost);
	int iSunks = sti(arPlayer.ShipsSunk) + sti(rPlayer.Stat.Sunk);
	int iWrecks = sti(arPlayer.Shipwrecks) + sti(rPlayer.Stat.Wreck)

	arPlayer.Hits = iHits;
	arPlayer.Misses = iMisses;
	arPlayer.Won = iWon;
	arPlayer.Lost = iLost;
	arPlayer.Shipwrecks = iWrecks;
	arPlayer.ShipsSunk = iSunks;
	arPlayer.TotalDamage = sti(arPlayer.TotalDamage) + sti(rPlayer.Stat.DamageInflicted); 

	arPlayer.LastLogin.Year = iRealYear;
	arPlayer.LastLogin.Month = iRealMonth;
	arPlayer.LastLogin.Day = iRealDay;

	if (iHits + iMisses > 0) 
		{ arPlayer.Accuracy = makeint((iHits * 100) / (iHits + iMisses)); }
	else 
		{ arPlayer.Accuracy = 0; }

	arPlayer.Rank = Net_GetRankNum(iSunks, iWon);

	arPlayer.Rewards.Accuracy = Net_GetRewardAccuracy(iHits, iMisses);
	arPlayer.Rewards.Vitality = Net_GetRewardVitality(iSunks, iWrecks);
	arPlayer.Rewards.Victorious = Net_GetRewardVictorious(iWon, iLost);
}
// COOP s30: "Game settings" of the lobby: the new game of the running server (no game is running). The players stay
// connected; every lobby reads the server info again (NSC_SERVERINFO_CHANGED: interfaces\Buy.c)
void NetServer_ApplySettings(ref rSrvParams)
{
	if (!NetServer_IsStarted()) { return; }
	if (bServerGameStarted) { return; }
	NetServer.ServerName = rSrvParams.ServerName;
	NetServer.ServerPassword = rSrvParams.ServerPassword;
	NetServer.AdminPassword = rSrvParams.AdminPassword;
	if (sti(rSrvParams.MaxPlayers) >= NetServer_GetClientsNum()) { NetServer.MaxClients = sti(rSrvParams.MaxPlayers); }
	NetServer_SetGameParams(rSrvParams);
	iSecondsToStartGame = -1;
	bServerStartRequested = false;
	int i;
	for (i=0; i<NET_MAXCLIENTS; i++)
	{
		if (!sti(NSClients[i].Use)) { continue; }
		// a team that is not in the new game: the first one
		if (sti(NSClients[i].Team) >= sti(NetServer.NumTeams)) { NSClients[i].Team = 0; }
		NSClients[i].BuyReady = false;
		NSClients[i].StartReady = false;
	}
	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_SERVERINFO);
	NMAddByte(iSMsg, NSC_SERVERINFO_CHANGED);
	NetServer_SendMessage(DST_ALL, iSMsg, true);
	NMDelete(iSMsg);
	Trace("COOP: srv game settings changed: type " + NetServer.GameType + " island " + NetServer.IslandID + " land " + NetServer.LandLoc + " teams " + NetServer.NumTeams);
}
