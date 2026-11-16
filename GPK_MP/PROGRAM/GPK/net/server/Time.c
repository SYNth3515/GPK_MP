int iClientDeltaTimeFraction = 0;
int iCoopCliWatchTicks = 0;

// COOP: number of the current game on this client. Ships of a previous game stay in NCClients (Use/Ship.Type are not reset),
// so every ship loaded in this game is stamped with this number and the watch ignores the stale ones.
int iCoopGameNo = 0;

// COOP: true when the client slot was loaded in the CURRENT game (CoopGame is set in NetClient_Load)
bool COOP_CliSlotLive(int i)
{
	if (!CheckAttribute(&NCClients[i], "CoopGame")) { return false; }
	return (sti(NCClients[i].CoopGame) == iCoopGameNo);
}

// COOP: true if the ship height is NaN / INF (string form contains N or #)
bool COOP_CliYBad(ref rC)
{
	if (!CheckAttribute(rC, "Ship.Pos.y")) { return false; }
	string sY = "" + rC.Ship.Pos.y;
	if (findSubStr(&sY, "N", 0) >= 0) { return true; }
	if (findSubStr(&sY, "#", 0) >= 0) { return true; }
	return false;
}

// COOP switch: 1 = if a ship height stays NaN on a client machine, leave to the main menu (the game is unplayable with a NaN camera);
// 0 = just keep logging. The host never leaves: it would close the game for everybody.
int iCoopCliNaNLeave = 1;
bool bCoopCliLeaving = false;

// COOP: the NetShip entity must NOT be deleted and recreated on a client (an attempt crashed the game right after the
// "recreating" line): the original code never deletes it either (see NetClient_Drop) because the rigging, the foam,
// the battle interface and the ship pointer keep references to it. Writing y into the attributes does not help either
// (the engine keeps its own value), so this only logs, and leaves when OUR OWN ship stays NaN. A NaN height of the other
// player's ship is only logged: our camera and controls are fine then.
void COOP_CliShipWatch()
{
	int i;
	bool bMeBad = false;
	iCoopCliWatchTicks = iCoopCliWatchTicks + 1;
	for (i=0; i<NET_TOTALCLIENTS; i++)
	{
		if (!COOP_CliSlotLive(i)) { continue; }
		if (!sti(NCClients[i].Use)) { continue; }
		if (sti(NCClients[i].Ship.Type) < 0) { continue; }
		ref rC = NetClient_GetClient(i);
		if (COOP_CliYBad(rC))
		{
			int iOwn = 0;
			if (i == wClientID) { iOwn = 1; bMeBad = true; }
			Trace("COOP: cli ship " + i + " height is NaN (tick " + iCoopCliWatchTicks + ", own=" + iOwn + ")");
		}
	}

	if (bMeBad && iCoopCliWatchTicks >= 6 && iCoopCliNaNLeave && !bCoopCliLeaving && !NetServer_IsStarted())
	{
		bCoopCliLeaving = true;
		Trace("COOP: cli own ship height is still NaN, leaving to the main menu");
		SetEventHandler("COOP_CliLeaveNaN", "COOP_CliLeaveNaN", 0);
		PostEvent("COOP_CliLeaveNaN", 1);
		return;
	}

	if (iCoopCliWatchTicks < 14) { PostEvent("COOP_CliShipWatch", 700); }
}

void COOP_CliLeaveNaN()
{
	DelEventHandler("COOP_CliLeaveNaN", "COOP_CliLeaveNaN");
	Net_Stop();
	ReturnToMainMenu();
}

void NetClient_DeleteTimeEnvironment()
{
	DelEventHandler("COOP_CliShipWatch", "COOP_CliShipWatch");
	DelEventHandler("NetClient_ShipCreateLater", "NetClient_ShipCreateLater");
	DelEventHandler("COOP_CliLeaveNaN", "COOP_CliLeaveNaN");
	DelEventHandler("frame", "NetClient_TimeFrame");
	DelEventHandler("NetClient_MatchTimerTick", "NetClient_MatchTimerTick");
	if (iNetMatchEnd > 0) { ClearAllLogStrings(); }
	iNetMatchEnd = 0;
}

void NetClient_CreateTimeEnvironment(int iMsg)
{
	SetEventHandler("frame", "NetClient_TimeFrame", 0);

	iCoopGameNo = iCoopGameNo + 1;
	Trace("COOP: cli game no " + iCoopGameNo);

	// COOP: watch client ship height for the first seconds
	iCoopCliWatchTicks = 0;
	bCoopCliLeaving = false;
	SetEventHandler("COOP_CliShipWatch", "COOP_CliShipWatch", 0);
	SetEventHandler("NetClient_ShipCreateLater", "NetClient_ShipCreateLater", 0);
	PostEvent("COOP_CliShipWatch", 2500);

	iClientTime = NMGetDword(iMsg);
	// COOP: the start message carries the server time. Without it iClientServerTime stays 0 until the first pong,
	// and a ship created at that moment gets garbage/NaN height (seen on 6 of 6 client starts with t=0).
	iClientServerTime = iClientTime;
	iClientDeltaTimeFraction = 0;

	// COOP: time left until the end of the match (0 = no limit); shown on the screen by NetClient_MatchTimerTick
	int iMatchLeft = NMGetDword(iMsg);
	ClearAllLogStrings();
	iNetMatchEnd = 0;
	bNetMatchTimeUpShown = false;
	SetEventHandler("NetClient_MatchTimerTick", "NetClient_MatchTimerTick", 0);
	if (iMatchLeft > 0)
	{
		iNetMatchEnd = iClientServerTime + iMatchLeft;
		PostEvent("NetClient_MatchTimerTick", 3000);
	}
}

int iNetMatchEnd = 0;
bool bNetMatchTimeUpShown = false;

// COOP: remaining time of the match as a permanent line on the screen (the same line the single player timers use)
void NetClient_MatchTimerTick()
{
	if (!bNetClientInGame) { return; }
	if (iNetMatchEnd <= 0) { return; }

	int iLeft = iNetMatchEnd - iClientServerTime;
	if (iLeft <= 0)
	{
		ClearAllLogStrings();
		if (!bNetMatchTimeUpShown)
		{
			bNetMatchTimeUpShown = true;
			NetClient_ChatAddSystemLine("Время матча вышло");
		}
		return;
	}

	int iSec = (iLeft + 999) / 1000;
	int iMin = iSec / 60;
	iSec = iSec - iMin * 60;
	string sSec = "" + iSec;
	if (iSec < 10) { sSec = "0" + iSec; }
	Log_SetEternalString("До конца матча: " + iMin + ":" + sSec);

	PostEvent("NetClient_MatchTimerTick", 500);
}

void NetClient_TimeFrame()
{
	if (fHighPrecisionDeltaTime > 0.2) { Trace("COOP: dt spike cli t=" + iClientServerTime + " dt=" + fHighPrecisionDeltaTime); }
	int iPrevClientTime = iClientTime;

	int iFullDeltaTime = MakeInt(fHighPrecisionDeltaTime * 1000000.0);
	int iDeltaTime = iFullDeltaTime / 1000;
	iClientDeltaTimeFraction = iClientDeltaTimeFraction + (iFullDeltaTime - iDeltaTime * 1000);
	//Trace("dt = " + GetDeltaTime() + ", iD2 = " + iD2 + ", iD3 = " + iD3);

	iClientTime = iClientTime + iDeltaTime;
	iClientServerTime = iClientServerTime + iDeltaTime;

	if (iClientDeltaTimeFraction > 1000)
	{
		int iMilliseconds = iClientDeltaTimeFraction / 1000;
		
		iClientTime = iClientTime + iMilliseconds;
		iClientServerTime = iClientServerTime + iMilliseconds;

		iClientDeltaTimeFraction = iClientDeltaTimeFraction - (iMilliseconds * 1000);
	}

	iTestDeltaTime = iTestDeltaTime + iDeltaTime;
	if (iTestDeltaTime >= 100)
	{
		iTestDeltaTime = iTestDeltaTime - 100;

		int iDelta = iClientTime - iClientServerTime;
		if (iDelta)
		{
			//if (abs(iDelta) <= 2) iClientTime = iClientTime - iDelta;
			if (iDelta > 2) { iClientTime = iClientTime - 2; }
			if (iDelta < -2) { iClientTime = iClientTime + 2; }
		}
	}

	//Trace("ClientTime = " + iClientTime + ", ClientServerTime = " + iClientServerTime);

	iClientDeltaTime = iClientTime - iPrevClientTime;
	if (iClientDeltaTime < 0) { iClientDeltaTime = 0; }
}