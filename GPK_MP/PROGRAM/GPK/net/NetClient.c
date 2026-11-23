#include "GPK\Net\Client\Cameras.c"
#include "GPK\Net\Client\ShipLights.c"
#include "GPK\Net\Client\Rigging.c"
#include "GPK\Net\Client\Island.c"
#include "GPK\Net\Client\Time.c"
#include "GPK\Net\Client\Fort.c"

#include "GPK\Net\Client\Utils.c"
#include "GPK\Net\Client\File.c"

#include "GPK\Net\Client\OnNetMessage.c"
#include "GPK\Net\Client\OnConnect.c"
#include "GPK\Net\Client\OnBall.c"
#include "GPK\Net\Client\OnChat.c"
#include "GPK\Net\Client\OnShip.c"
#include "GPK\Net\Client\Sailors.c"
#include "GPK\Net\Client\OnLoad.c"
#include "GPK\Net\Client\OnPing.c"
#include "GPK\Net\Client\OnStart.c"
#include "GPK\Net\Client\OnWeather.c"
#include "GPK\Net\Client\OnClient.c"
#include "GPK\Net\Client\Admin.c"
#include "GPK\Net\Land.c"
#include "GPK\Net\LandNpc.c"		// COOP s35: the NPC of the location are the ones of the host
#include "GPK\Net\Board.c"		// COOP s37: the real boarding: the captains fight on the deck
#include "GPK\Net\MenuScene.c"		// COOP s30: the 3D scene of the menu behind the profile screens
#include "GPK\Net\LandTdm.c"		// COOP s27: the locations of TDM for the land duel
#include "GPK\Net\NetInfo.c"		// COOP s31: the right click shows a description (loadout, buy screen)
#include "GPK\Net\Client\ServerInfo.c"
#include "GPK\Net\Client\PlayerInfo.c"

#include "GPK\Net\Client\Weather\Weather.c"

#include "GPK\Net\Interfaces\Interface.c"
#include "GPK\Net\Interfaces\BattleInterface\BattleInterface.c"

int wClientID;
bool bNetClientConnected = false;
bool bNetClientInGame = false;		// COOP: the environment of the game is created (false in the lobby, also while a game runs)

object NCBalls;
object NCConsole;
object NCIsland, NCCoastFoam;
object NCSail, NCVant, NCRope, NCFlag;
object NCSay;
object NCShipTracks;
object NCLightPillar;

object NCServer;
object NCPerks;		// COOP: state of our active perks (p<N>.Until / p<N>.Ready, server time)

object NCClients[NET_TOTALCLIENTS];
object NCProfiles;
object NCFavorites;
object NCInetServers;

int iLangNetClient;

string sClientNetCamera = "DeckCamera";
int iClientTime, iClientServerTime, iClientDeltaTime;
int iTestDeltaTime;

void Net_ClientFirstInit()
{
	NetClient.NickName = "default";
	NetClient.Password = "";
	NetClient.ShipName = "noname";
	NetClient.FaceImage = "_emptyface_";
	NetClient.SailImage = "_emptysail_";
	NetClient.FlagImage = "_emptyflag_";
	NetClient.SailColor = argb(255, 255, 255, 255);

	NetClient.ClientLastPacketIndex = 0;
}

// COOP s32: the chosen profile (the button "Выбрать") into NetClient. Also after a preview: it put the unsaved face of the
// profile screen into NetClient, FindGame and the menu showed it after that
void NetClient_LoadLastProfile()
{
	if (!CheckAttribute(&NCProfiles, "LastProfile")) { return; }
	NetClient.SailColor = NCProfiles.LastProfile.SailColor;
	NetClient.NickName = NCProfiles.LastProfile.NickName;
	NetClient.ShipName = NCProfiles.LastProfile.ShipName;
	NetClient.Password = NCProfiles.LastProfile.Password;
	NetClient.FaceImage = NCProfiles.LastProfile.FaceImage;
	NetClient.FlagImage = NCProfiles.LastProfile.FlagImage;
	NetClient.SailImage = NCProfiles.LastProfile.SailImage;
	DeleteAttribute(&NCClients[wClientID], "LandModel");		// net\Land.c: the skin is taken from the face again
}

void Net_DeleteClient()
{
	// COOP: tell the server that we leave (the running game parks our ship for a reconnect at once, without waiting for
	// the ping timeout). Sent directly and twice: the queue of the client is deleted right after this.
	if (bNetClientConnected && IsEntity(&NetClient))
	{
		for (int iBye=0; iBye<2; iBye++)
		{
			int iByeMsg = NMCreate();
			NMAddClientID(iByeMsg, wClientID);
			NMAddByte(iByeMsg, NC_DISCONNECT);
			NetClient_SendMessageDirect(iByeMsg);
			NMDelete(iByeMsg);
		}
	}

	bNetClientConnected = false;
	bNetClientInGame = false;
	DelEventHandler("ExitApplication", "NetClient_ExitApplication");
	DelEventHandler("NetClient_OnNetMessage", "NetClient_OnNetMessage");
	DelEventHandler("NetClient_Ping", "NetClient_Ping");

	NetClient_OnEndGame();

	DeleteClass(&NetClient);
	DeleteClass(&NCLightPillar);

	// COOP s46: the faces, the sails and the flags of the players (OnClient.c: NFGetTexture "UserDataImages") were never
	// released: every game leaked them. The client is gone, nothing shows them any more.
	NFDelGroupTextures("UserDataImages");
	for (int iUd=0; iUd<NET_MAXCLIENTS; iUd++)
	{
		DeleteAttribute(&NCClients[iUd], "UserData");
		DeleteAttribute(&NCClients[iUd], "Stat.Kills");
	}
	Trace("COOP: user textures released");

	LanguageCloseFile(iLangNetClient); iLangNetClient  = -1;

	Net_SaveFile(false, &NCInetServers, "InetServers.nsv");
}

void Net_CreateClient()
{
	DeleteAttribute(&NCLobbyBots, "");		// COOP s30: the bots of the lobby of the last server
	for (int i=0; i<NET_TOTALCLIENTS; i++)
	{
		DeleteAttribute(&NCClients[i], "");
		NCClients[i].index = i;
		NCClients[i].ID = i;
		NCClients[i].Use = false;
		NCClients[i].Server = false;			// netclient clients
		
		NCClients[i].BuyReady = false;
		NCClients[i].StartReady = false;
		NCClients[i].Kick = false;

		NCClients[i].Team = 0;

		NCClients[i].Dead = false;

		NCClients[i].Stat.DamageInflicted = 0;
		NCClients[i].Stat.Accuracy = 0;

		NCClients[i].Stat.Kills = "";
		NCClients[i].Stat.Frags = 0;		// COOP: ships sunk (server value, F1 table)
		NCClients[i].Stat.Deaths = 0;		// COOP: own ship lost
		NCClients[i].Wrecked = false;		// COOP: respawn mode, the ship waits for the respawn
		NCClients[i].ShipState = 0;		// COOP: 0 afloat, 1 wrecked, 2 sunk

		NCClients[i].Skills.Repair.Active = 0;
		NCClients[i].Skills.Repair.TotalCooldownTime = 0;
		NCClients[i].Skills.Repair.CurrentCooldownTime = 0;
	}

	iClientTime = 0;
	iClientServerTime = 0;
	iClientDeltaTime = 0;
	iTestDeltaTime = 0;

	CreateEntity(&NetClient, "NetClient");

	NetClient.ServerPassword = "";
	NetClient.ServerAddr = "";
	NetClient.ServerPort = NET_DEFAULT_SERVER_PORT;
	NetClient.Start = "";
	NetClient.AllowDownload = true;
	NetClient.AllowUpload = true;

	NetClient.ConvoyShipID = DST_INVALID;
	NetClient.ConvoyPoint.x = 0.0;
	NetClient.ConvoyPoint.z = 0.0;

	NetClient_PingInit();

	SetEventHandler("ExitApplication", "NetClient_ExitApplication", 0);
	SetEventHandler("NetClient_OnNetMessage", "NetClient_OnNetMessage", 0);
	SetEventHandler("NetClient_Ping", "NetClient_Ping", 0);

	// init NCServer object with fake values
	NCServer.NumClients = 0;
	NCServer.MaxClients = NET_MAXCLIENTS;
	NCServer.NumTeams = 4;
	NCServer.IslandID = "";
	NCServer.WeatherID = "";
	NCServer.GameRunning = false;		// COOP: values of the common server info
	NCServer.Respawn = false;
	NCServer.NoBoarding = false;
	NCServer.FastBoarding = true;		// COOP s37
	NCServer.PassivePerks = false;
	NCServer.ActivePerks = false;
	DeleteAttribute(&NCPerks, "");
	bNetClientInGame = false;

	NetClient.ClientLastPacketIndex = 0;

	iLangNetClient = LanguageOpenFile("NetSection.txt");

	// load client profiles
	DeleteAttribute(&NCProfiles, "");
	Net_LoadFile(false, &NCProfiles, "Profiles.nsv");

	NetClient_LoadLastProfile();

	// load client inet servers list
	DeleteAttribute(&NCInetServers, "");
	Net_LoadFile(false, &NCInetServers, "InetServers.nsv");
}

void NetClient_ExitApplication()
{
}

void NetClient_FindLocalServers()
{
	int iSMsg = NMCreate();
	NMAddClientID(iSMsg, DST_INVALID);
	NMAddByte(iSMsg, NC_BROADCAST);
	NMAddByte(iSMsg, NSC_BROADCAST_LOCAL);
	NMAddDword(iSMsg, Net_GetTickCount());
	NetClient_SendBroadcastMessage(iSMsg);
	NMDelete(iSMsg);
}

void NetClient_TryConnect(string sServerIP, int wPort)
{
	NetClient.ServerAddr = sServerIP;
	NetClient.ServerPort = wPort;

	int iSMsg = NMCreate();
	NMAddClientID(iSMsg, DST_INVALID);
	NMAddByte(iSMsg, NC_CONNECT);
	NMAdd24(iSMsg, NET_TEST_DWORD);
	NMAddByte(iSMsg, NET_SCRIPT_VERSION);
	NMAddString(iSMsg, NetClient.NickName, 24);			// nick name
	NMAddString(iSMsg, NetClient.Password, 12);			// nick password
	NMAddString(iSMsg, NetClient.ServerPassword, 12);	// nick password
	NMAddString(iSMsg, NetClient.ShipName, 24);			// ship name
	NMAddString(iSMsg, NetClient.FaceImage, 12);		// client face
	NMAddString(iSMsg, NetClient.FlagImage, 12);		// client flag
	NMAddString(iSMsg, NetClient.SailImage, 12);		// client sail emblem
	NMAddDword(iSMsg, sti(NetClient.SailColor));		// client sail color
	NetClient_SendMessage(iSMsg, false);
	NMDelete(iSMsg);
}

void NetClient_SelectProfile(string sProfileName)
{
	NetClient.SailColor = sti(SailsColors[sti(NCProfiles.Profiles.(sProfileName).ColorSail)].Color);
	
	NetClient.NickName = NCProfiles.Profiles.(sProfileName).Name;
	NetClient.ShipName = NCProfiles.Profiles.(sProfileName).ShipName;
	NetClient.Password = NCProfiles.Profiles.(sProfileName).Password;

	NetClient.FaceImage = NCProfiles.Profiles.(sProfileName).ImageFace;
	NetClient.FlagImage = NCProfiles.Profiles.(sProfileName).ImageFlag;
	NetClient.SailImage = NCProfiles.Profiles.(sProfileName).ImageSail;
}