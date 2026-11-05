int iServerDeltaTimeFraction = 0;
int iCoopSrvWatchTicks = 0;

// COOP: true if the ship height is NaN / INF (string form contains N or #)
bool COOP_SrvYBad(ref rC)
{
	string sY = "" + rC.Ship.Pos.y;
	if (findSubStr(&sY, "N", 0) >= 0) { return true; }
	if (findSubStr(&sY, "#", 0) >= 0) { return true; }
	return false;
}

void COOP_SrvRecreateShip(int i)
{
	ref rC = NetServer_GetClient(i);
	ref rShip = Net_GetShipByIndex(sti(rC.Ship.Type));
	rC.CoopFix = sti(rC.CoopFix) + 1;
	Trace("COOP: srv ship " + i + " height is NaN, recreating, attempt " + rC.CoopFix);
	DeleteClass(&NSClients[i]);
	NSClients[i].Ship.Pos.y = 0.0;
	NSClients[i].Ship.Speed.x = 0.0;
	NSClients[i].Ship.Speed.y = 0.0;
	NSClients[i].Ship.Speed.z = 0.0;
	CreateEntity(&NSClients[i], "NetShip");
	SendMessage(&NSClients[i], "la", MSG_SHIP_CREATE, rShip);
	LayerAddObject("net_execute", &NSClients[i], 31);
	LayerAddObject("ship_cannon_trace", &NSClients[i], 31);
}

void COOP_SrvShipWatch()
{
	int i;
	iCoopSrvWatchTicks = iCoopSrvWatchTicks + 1;
	for (i=0; i<NET_MAXCLIENTS; i++)
	{
		if (!NetServer_HasShip(i)) { continue; }		// COOP: lobby players of the running game have no ship, parked ships do
		ref rC = NetServer_GetClient(i);
		if (COOP_SrvYBad(rC) && sti(rC.CoopFix) < 6) { COOP_SrvRecreateShip(i); }
	}
	if (iCoopSrvWatchTicks < 14) { PostEvent("COOP_SrvShipWatch", 700); }
}

void NetServer_DeleteTimeEnvironment()
{
	DelEventHandler("COOP_SrvShipWatch", "COOP_SrvShipWatch");
	DelEventHandler("frame", "NetServer_TimeFrame");
}

void NetServer_CreateTimeEnvironment(int iServerEnvSMsg)
{
	iServerTime = 300;
	SetEventHandler("frame", "NetServer_TimeFrame", 0);

	// COOP: watch ship height for the first seconds (NetShip may start with NaN height and then never moves)
	iCoopSrvWatchTicks = 0;
	SetEventHandler("COOP_SrvShipWatch", "COOP_SrvShipWatch", 0);
	PostEvent("COOP_SrvShipWatch", 2500);

	iServerDeltaTimeFraction = 0;
	NMAddDword(iServerEnvSMsg, iServerTime);
}

void NetServer_TimeFrame()
{
	if (fHighPrecisionDeltaTime > 0.2) { Trace("COOP: dt spike srv t=" + iServerTime + " dt=" + fHighPrecisionDeltaTime); }
	int iFullDeltaTime = MakeInt(fHighPrecisionDeltaTime * 1000000.0);
	int iDeltaTime = iFullDeltaTime / 1000;
	iServerDeltaTimeFraction = iServerDeltaTimeFraction + (iFullDeltaTime - iDeltaTime * 1000);

	iServerTime = iServerTime + iDeltaTime;

	if (iServerDeltaTimeFraction > 1000)
	{
		int iMilliseconds = iServerDeltaTimeFraction / 1000;
		iServerTime = iServerTime + iMilliseconds;
		iServerDeltaTimeFraction = iServerDeltaTimeFraction - (iMilliseconds * 1000);
	}
}
