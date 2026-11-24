// ================================================================================================
// COOP s48: THE DEDICATED SERVER. A file "dedicated.ini" in the folder of the game with [Server] Enabled = 1 turns the game
// into a server without a player: no intro, no main menu, the server is made at once from the file (Dedicated_Start, called
// by seadogs.c Main_Start) and the game runs itself:
//   - the lobby: when MinPlayers are there, the start in StartDelay s (all of them "ready": in 5 s); an admin can start at once
//   - the round is over (a winner, the time is up): the players are sent back to the lobby after RoundEndDelay s, the next
//     round (another map if the map is "random" and RotateMap = 1)
//   - the last player has left: the game closes itself (QuitWhenEmpty = 1); the launcher (dedicated\*.ps1, systemd) starts it
//     again, the same after a crash
// Nobody plays on this machine: slot 0 is a slot of the players, the NPC of a land duel are kept by every player himself
// (LandNpc.c), the realize layers are frozen (Render = 0): a VPS without a video card draws nothing.
// The template of the file: dedicated\dedicated.ini. Values that are not in the file: the defaults below.
// ================================================================================================

#define DED_INI				"dedicated.ini"
#define DED_SEC				"Server"
#define DED_TICK_MS			1000

int iDedOn = -1;				// -1 not read yet, 0 a normal game, 1 the dedicated server
bool bDedHadPlayers = false;	// somebody has come since the start
int iDedEmpty = 0;				// s without players
int iDedCountdown = -1;			// s to the start of the round, -1 none
int iDedOverLeft = -1;			// s to the lobby after the end of the round, -1 none
int iDedIdle = 0;				// s of a running game without a ship of a player in it
int iDedRound = 0;
object oDedCfg;					// the values of the file (strings)

bool Dedicated_IsOn()
{
	if (iDedOn >= 0) { return iDedOn == 1; }
	iDedOn = 0;
	if (NFExist(DED_INI) == 0) { return false; }
	if (NetIni_GetLong(DED_INI, DED_SEC, "Enabled") == 1) { iDedOn = 1; }
	return iDedOn == 1;
}

// a value of the file: "" / "default" gives sDef. A MISSING key crashes NetIni_GetString: the launchers add every key.
string Ded_Str(string sKey, string sDef)
{
	string s = NetIni_GetString(DED_INI, DED_SEC, sKey);
	if (s == "" || s == "(null)" || s == "default" || s == "Default") { return sDef; }
	return s;
}

int Ded_Int(string sKey, int iDef)
{
	string s = Ded_Str(sKey, "");
	if (s == "") { return iDef; }
	return sti(s);
}

string Ded_Lower(string s)
{
	string sUp = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	string sLo = "abcdefghijklmnopqrstuvwxyz";
	string sOut = "";
	int n = strlen(s);
	for (int i=0; i<n; i++)
	{
		string c = GetSymbol(s, i);
		int k = findSubStr(sUp, c, 0);
		if (k >= 0) { c = GetSymbol(sLo, k); }
		sOut = sOut + c;
	}
	return sOut;
}

void Ded_ReadConfig()
{
	DeleteAttribute(&oDedCfg, "");
	string sMode = Ded_Lower(Ded_Str("Mode", "deathmatch"));
	int iType = NETGAME_DEATHMATCH;
	bool bLand = false;
	if (sMode == "tdm" || sMode == "teamdeathmatch" || sMode == "naval") { iType = NETGAME_NAVAL; }
	if (sMode == "convoy") { iType = NETGAME_CONVOY; }
	if (sMode == "fort") { iType = NETGAME_FORT; }
	if (sMode == "land" || sMode == "landduel" || sMode == "duel") { bLand = true; }
	oDedCfg.GameType = iType;
	oDedCfg.Land = bLand;
	oDedCfg.Mode = sMode;

	int iTeams = 1;
	if (iType == NETGAME_NAVAL) { iTeams = Ded_Int("Teams", 2); }
	if (iType == NETGAME_CONVOY || iType == NETGAME_FORT) { iTeams = 2; }
	if (bLand) { iTeams = Ded_Int("Teams", 1); }
	if (iTeams < 1) { iTeams = 1; }
	if (iTeams > 4) { iTeams = 4; }
	if (bLand && iTeams > 2) { iTeams = 2; }
	if (iType == NETGAME_NAVAL && iTeams < 2) { iTeams = 2; }
	oDedCfg.NumTeams = iTeams;

	int iMinDef = 1;
	if (iType != NETGAME_DEATHMATCH) { iMinDef = 2; }
	if (bLand && iTeams > 1) { iMinDef = 2; }
	oDedCfg.MinPlayers = Ded_Int("MinPlayers", iMinDef);
	if (sti(oDedCfg.MinPlayers) < 1) { oDedCfg.MinPlayers = 1; }
	oDedCfg.StartDelay = Ded_Int("StartDelay", 30);
	if (sti(oDedCfg.StartDelay) < 5) { oDedCfg.StartDelay = 5; }
	oDedCfg.RoundEndDelay = Ded_Int("RoundEndDelay", 20);
	if (sti(oDedCfg.RoundEndDelay) < 5) { oDedCfg.RoundEndDelay = 5; }
	oDedCfg.QuitWhenEmpty = Ded_Int("QuitWhenEmpty", 1);
	oDedCfg.RotateMap = Ded_Int("RotateMap", 1);
	oDedCfg.Render = Ded_Int("Render", 0);
	oDedCfg.Island = Ded_Str("Island", "random");
	oDedCfg.LandLoc = Ded_Str("Land", "random");
	oDedCfg.Weather = Ded_Str("Weather", "random");
}

// ---------------------------------------------------------------------------------------- the maps
bool Ded_IslandFits(ref rIsland, int iType)
{
	if (iType == NETGAME_FORT && !Net_IsIslandHaveForts(rIsland)) { return false; }
	// the City of the lost ships has no forts and no points of the convoy (interfaces\CreateGame.c CS_IsIslandAllowedForMode)
	if (rIsland.ID == "LostShipsCity" && iType != NETGAME_DEATHMATCH && iType != NETGAME_NAVAL) { return false; }
	return true;
}

string Ded_PickIsland()
{
	int iType = sti(oDedCfg.GameType);
	string sIsl = oDedCfg.Island;
	if (Ded_Lower(sIsl) == "opensea") { return ""; }
	if (Ded_Lower(sIsl) != "random") { return sIsl; }
	int iN = 0;
	int i;
	for (i=0; i<Net_GetIslandsNum(); i++)
	{
		ref rI = Net_GetIslandByIndex(i);
		if (Ded_IslandFits(rI, iType)) { iN++; }
	}
	if (iN <= 0) { return ""; }
	int iPick = rand(iN - 1);
	for (i=0; i<Net_GetIslandsNum(); i++)
	{
		ref rJ = Net_GetIslandByIndex(i);
		if (!Ded_IslandFits(rJ, iType)) { continue; }
		if (iPick == 0) { return rJ.ID; }
		iPick--;
	}
	return "";
}

// the locations of a duel that every build has (interfaces\CreateGame.c CS_AddLandRows)
string Ded_PickLand()
{
	string sLoc = oDedCfg.LandLoc;
	if (Ded_Lower(sLoc) != "random") { return sLoc; }
	switch (rand(5))
	{
		case 0: return "BOARDING_SMALL_DECK"; break;
		case 1: return "BOARDING_MEDIUM_DECK"; break;
		case 2: return "BOARDING_BIG_DECK"; break;
		case 3: return "Cabin_Huge"; break;
		case 4: return "Ship_deck_Big"; break;
	}
	return "Deck_Near_Ship_Big";
}

// the weathers of the Create game screen (CS_GetCurrentWeather) without the storms
string Ded_PickWeather()
{
	string sW = oDedCfg.Weather;
	if (Ded_Lower(sW) != "random") { return sW; }
	switch (rand(11))
	{
		case 0: return "6 Hours"; break;
		case 1: return "8 Hour"; break;
		case 2: return "10 Hours"; break;
		case 3: return "11 Hour"; break;
		case 4: return "14 Hour"; break;
		case 5: return "16 Nour"; break;
		case 6: return "19 Hour"; break;
		case 7: return "22 Hour"; break;
		case 8: return "23 Hour"; break;
		case 9: return "0 Hours"; break;
		case 10: return "1 hour"; break;
	}
	return "3 Hours";
}

// ---------------------------------------------------------------------------------------- the parameters of the server
void Ded_FillParams(ref rP)
{
	rP.ServerName = Ded_Str("Name", "GPK_MP_Dedicated");
	rP.ServerPort = Ded_Int("Port", NET_DEFAULT_SERVER_PORT);
	rP.ServerPassword = Ded_Str("Password", "");
	rP.AdminPassword = Ded_Str("AdminPassword", "");
	rP.Dedicated = 1;
	rP.LanServer = Ded_Int("Lan", 1);
	rP.InetServer = Ded_Int("Internet", 1);
	rP.P2P = 0;
	if (sti(rP.InetServer)) { rP.P2P = Ded_Int("P2P", 0); }
	rP.UpdateMasterServer = true;
	rP.MaxFps = Ded_Int("MaxFps", 60);
	rP.GameType = sti(oDedCfg.GameType);
	rP.NumTeams = sti(oDedCfg.NumTeams);
	int iMax = Ded_Int("MaxPlayers", 16);
	if (iMax > NET_MAXPLAYERS) { iMax = NET_MAXPLAYERS; }
	if (iMax < 2) { iMax = 2; }
	rP.MaxPlayers = iMax;
	float fSpeed = stf(Ded_Str("GameSpeed", "1"));
	if (fSpeed < 0.5) { fSpeed = 1.0; }
	if (fSpeed > 8.0) { fSpeed = 8.0; }
	rP.GameSpeed = fSpeed;
	rP.TimeLimit = Ded_Int("TimeLimit", 20);
	rP.StartPosition = Ded_Int("StartPosition", 0);
	rP.Credit = Ded_Int("Credit", 1000000);
	rP.MaxShipClass = Ded_Int("MaxShipClass", 1);
	rP.BotSkill = Ded_Int("BotSkill", 3);
	rP.IslandID = "";
	rP.LandLoc = "";
	if (sti(oDedCfg.Land)) { rP.LandLoc = Ded_PickLand(); }
	else { rP.IslandID = Ded_PickIsland(); }
	rP.WeatherID = Ded_PickWeather();
	rP.Tornado = 0;
	rP.AllowUpload = true;
	rP.AllowDownload = true;
	rP.Respawn = Ded_Int("Respawn", 0);
	rP.RespawnTime = Ded_Int("RespawnTime", NET_RESPAWN_DELAY / 1000);
	rP.NoBoarding = Ded_Int("NoBoarding", 0);
	rP.PassivePerks = Ded_Int("PassivePerks", 0);
	rP.ActivePerks = Ded_Int("ActivePerks", 0);
	rP.LateJoin = Ded_Int("LateJoin", 0);
	rP.PistolReload = Ded_Int("PistolReload", 0);
	rP.FastBoarding = Ded_Int("FastBoarding", 0);
	if (sti(oDedCfg.Land)) { rP.FastBoarding = true; }		// a land duel has no boarding (CS_CreateServer)
}

// ---------------------------------------------------------------------------------------- the start
void Dedicated_Start()
{
	Ded_ReadConfig();
	Trace("COOP: DEDICATED server: mode " + oDedCfg.Mode + ", teams " + oDedCfg.NumTeams + ", min players " + oDedCfg.MinPlayers);
	Net_Start();
	// the layers that the sea of a player creates on a host: the cannons trace the ships / the fort through them, and
	// LayerDelObject on a missing layer crashes engine.exe (NetServer_ShipNoCannonTarget after a prize capture)
	LayerCreate("ship_cannon_trace", 1);
	LayerCreate("fort_cannon_trace", 1);
	// net.dll NetShip leaves sea_reflection when it dies (the attribute Dead): the core crashes on a missing layer
	LayerCreate("sea_reflection", 1);
	object oP;
	Ded_FillParams(&oP);
	Trace("COOP: DEDICATED " + oP.ServerName + " port " + oP.ServerPort + " island <" + oP.IslandID + "> land <" + oP.LandLoc + "> weather <" + oP.WeatherID + "> respawn " + oP.Respawn + " time " + oP.TimeLimit + " late join " + oP.LateJoin + " fast boarding " + oP.FastBoarding + " bots " + oP.BotSkill);
	Net_CreateServer(&oP);
	Ded_RenderOff();
	SetEventHandler("Dedicated_Tick", "Dedicated_Tick", 0);
	PostEvent("Dedicated_Tick", DED_TICK_MS);
}

// nothing is drawn here (no video card on a VPS); Render = 1 in the file keeps the drawing (for a check on a PC)
void Ded_RenderOff()
{
	if (sti(oDedCfg.Render)) { return; }
	LayerFreeze("realize", true);
	LayerFreeze("sea_realize", true);
	LayerFreeze("net_realize", true);
	LayerFreeze("iRealize", true);
	LayerFreeze("inf_realize", true);
	Trace("COOP: DEDICATED the realize layers are frozen");
}

void Ded_Say(string s)
{
	NetServer_SayTo(DST_ALL, s);
	Trace("COOP: DEDICATED say: " + s);
}

int Ded_Players()
{
	int n = 0;
	for (int i=0; i<NET_MAXPLAYERS; i++) { if (sti(NSClients[i].Use)) { n++; } }
	return n;
}

int Ded_ReadyPlayers()
{
	int n = 0;
	for (int i=0; i<NET_MAXPLAYERS; i++)
	{
		if (!sti(NSClients[i].Use)) { continue; }
		if (sti(NSClients[i].BuyReady)) { n++; }
	}
	return n;
}

int Ded_InGamePlayers()
{
	int n = 0;
	for (int i=0; i<NET_MAXPLAYERS; i++)
	{
		if (!sti(NSClients[i].Use)) { continue; }
		if (NetServer_IsInGame(i)) { n++; }
	}
	return n;
}

// ---------------------------------------------------------------------------------------- the clock (1 s)
void Dedicated_Tick()
{
	if (!NetServer_IsStarted()) { return; }
	PostEvent("Dedicated_Tick", DED_TICK_MS);

	int nPlayers = Ded_Players();
	if (nPlayers > 0) { bDedHadPlayers = true; }

	// the last player has gone: the game closes, the launcher starts a new one
	if (bDedHadPlayers && nPlayers == 0)
	{
		iDedEmpty++;
		if (iDedEmpty >= 5 && sti(oDedCfg.QuitWhenEmpty)) { Dedicated_Quit("all players have left"); return; }
	}
	else { iDedEmpty = 0; }

	if (bServerGameStarted) { Ded_TickGame(nPlayers); return; }
	iDedOverLeft = -1;
	iDedIdle = 0;
	if (bServerStartRequested) { return; }		// the start goes on (the buy info of the players)
	Ded_TickLobby(nPlayers);
}

void Ded_TickLobby(int nPlayers)
{
	if (nPlayers < sti(oDedCfg.MinPlayers))
	{
		if (iDedCountdown >= 0) { Ded_Say("Старт отменён: мало игроков (нужно " + oDedCfg.MinPlayers + ")"); }
		iDedCountdown = -1;
		return;
	}
	// the countdown starts when somebody is ready (the others still buy their ships)
	int nReady = Ded_ReadyPlayers();
	if (nReady <= 0)
	{
		if (iDedCountdown >= 0) { Ded_Say("Старт отменён: никто не готов"); }
		iDedCountdown = -1;
		return;
	}
	if (iDedCountdown < 0)
	{
		iDedCountdown = sti(oDedCfg.StartDelay);
		Ded_Say("Игра начнётся через " + iDedCountdown + " с (если «Готов» нажмут все - через 5 с)");
	}
	if (nReady >= nPlayers && iDedCountdown > 5) { iDedCountdown = 5; }
	iDedCountdown--;
	bool bDedSay = false;
	if (iDedCountdown == 20 || iDedCountdown == 10) { bDedSay = true; }
	if (iDedCountdown > 0 && iDedCountdown <= 3) { bDedSay = true; }
	if (bDedSay) { Ded_Say("Старт через " + iDedCountdown + " с"); }
	if (iDedCountdown > 0) { return; }
	iDedCountdown = -1;
	iDedRound++;
	Trace("COOP: DEDICATED round " + iDedRound + " starts, players " + nPlayers);
	NetServer_OnCommandStartGame(DST_INVALID);
}

void Ded_TickGame(int nPlayers)
{
	iDedCountdown = -1;
	// the round is over (a winner, a draw, the time is up): everybody goes to the lobby a bit later
	if (sti(NetServer.GameOver))
	{
		if (iDedOverLeft < 0)
		{
			iDedOverLeft = sti(oDedCfg.RoundEndDelay);
			Ded_Say("Раунд окончен. Возврат в лобби через " + iDedOverLeft + " с");
		}
		iDedOverLeft--;
		if (iDedOverLeft <= 0) { Ded_ToLobby("the round is over"); }
		return;
	}
	iDedOverLeft = -1;
	// the players are in the lobby, nobody sails / fights: no game for nothing
	if (Ded_InGamePlayers() == 0)
	{
		iDedIdle++;
		if (iDedIdle >= 60) { Ded_ToLobby("nobody is in the game"); }
		return;
	}
	iDedIdle = 0;
}

void Ded_ToLobby(string sWhy)
{
	Trace("COOP: DEDICATED back to the lobby: " + sWhy);
	iDedOverLeft = -1;
	iDedIdle = 0;
	NetServer_ReturnToLobby();
	// the next map ("random" in the file)
	if (sti(oDedCfg.RotateMap))
	{
		object oP;
		Ded_FillParams(&oP);
		NetServer_ApplySettings(&oP);
	}
}

void Dedicated_Quit(string sWhy)
{
	Trace("COOP: DEDICATED the game closes: " + sWhy);
	DelEventHandler("Dedicated_Tick", "Dedicated_Tick");
	Net_DeleteServer();
	ExitProgram();
}
