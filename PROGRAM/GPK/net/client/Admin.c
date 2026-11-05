// ================================================================================================
// COOP: admin system (client side). The server decides everything, see server/Admin.c
// ================================================================================================

bool bNetClientIsAdmin = false;

// Reload ships: the server always recreates the ship entities. If this is 1, every client also deletes and creates its own
// copies of the ships (the real "delete and create again on all machines"). It is OFF because a delete of a ship on a client
// crashed the game in tests (see the note in client/Time.c), the server part works without it.
int iNetAdminClientReload = 0;

// last environment sent by the server (admin menu)
int iNetCliAdminSpeed = 1;
float fNetCliAdminWindAngle = 0.0;
float fNetCliAdminWindSpeed = 0.0;
string sNetCliAdminWeather = "";
int iNetCliAdminTime = 0;
int iNetCliAdminWeatherType = 0;

void NetClient_AdminReset()
{
	bNetClientIsAdmin = false;
}

void NetClient_AdminLogin(string sPassword)
{
	int iSMsg = NMCreate();
	NMAddClientID(iSMsg, wClientID);
	NMAddByte(iSMsg, NC_COMMAND);
	NMAddByte(iSMsg, NSC_COMMAND_ADMIN_LOGIN);
	NMAddString(iSMsg, sPassword, 24);
	NetClient_SendMessage(iSMsg, true);
	NMDelete(iSMsg);
}

// the machine that created the server knows its admin password: it is an admin without typing it
// (empty password = nobody is an admin, the host too)
void NetClient_AdminAutoLogin()
{
	if (!NetServer_IsStarted()) { return; }

	string sPassword = NetServer.AdminPassword;
	if (sPassword == "") { return; }

	NetClient_AdminLogin(sPassword);
}

void NetClient_AdminSend(int iAction, int wTarget, float fValue)
{
	int iSMsg = NMCreate();
	NMAddClientID(iSMsg, wClientID);
	NMAddByte(iSMsg, NC_COMMAND);
	NMAddByte(iSMsg, NSC_COMMAND_ADMIN_CMD);
	NMAddByte(iSMsg, iAction);
	NMAddClientID(iSMsg, wTarget);
	NMAddFloat(iSMsg, fValue);
	NetClient_SendMessage(iSMsg, true);
	NMDelete(iSMsg);
}

// ------------------------------------------------------------------------------------------------
void NetClient_OnCommand(int iMsg)
{
	int iSubCode = NMGetByte(iMsg);

	switch (iSubCode)
	{
		case NSC_COMMAND_ADMIN_STATUS:	NetClient_OnAdminStatus(iMsg);	break;
		case NSC_COMMAND_ADMIN_FLAGS:	NetClient_OnAdminFlags(iMsg);	break;
		case NSC_COMMAND_ADMIN_ENV:		NetClient_OnAdminEnv(iMsg);		break;
		case NSC_COMMAND_SET_WEATHER:	NetClient_OnSetWeather(iMsg);	break;
		case NSC_COMMAND_SET_WIND:		NetClient_OnSetWind(iMsg);		break;
		case NSC_COMMAND_RELOAD_SHIPS:	NetClient_AdminReloadShips();	break;
	}
}

void NetClient_OnAdminStatus(int iMsg)
{
	int iAdmin = NMGetByte(iMsg);

	bNetClientIsAdmin = (iAdmin != 0);

	Event("NCEvent_AdminStatus", "l", iAdmin);
}

void NetClient_OnAdminFlags(int iMsg)
{
	int wNetClientID = NMGetClientID(iMsg);
	int iAdmin = NMGetByte(iMsg);
	int iMuted = NMGetByte(iMsg);

	ref rClient = NetClient_GetClient(wNetClientID);
	rClient.Admin = iAdmin;
	rClient.Muted = iMuted;

	if (wNetClientID == wClientID) { bNetClientIsAdmin = (iAdmin != 0); }

	Event("NCEvent_AdminFlags");
}

void NetClient_OnAdminEnv(int iMsg)
{
	iNetCliAdminSpeed = NMGetByte(iMsg);
	fNetCliAdminWindAngle = NMGetFloat(iMsg);
	fNetCliAdminWindSpeed = NMGetFloat(iMsg);
	sNetCliAdminWeather = NMGetString(iMsg);
	iNetCliAdminTime = NMGetByte(iMsg);
	iNetCliAdminWeatherType = NMGetByte(iMsg);

	Event("NCEvent_AdminEnv");
}

// The admin changed the weather / time of day. Like Whr_UpdateWeather of the single player game: the sea (NCSea) and the
// weather entity (NCWeather) are NOT deleted, ships, foam and rigging use them; their data is replaced by
// NetClient_CreateWeatherEnvironment (it does not create an entity that is already alive). Sky, sun, rain, lightning and
// stars do not depend on ships and are created again.
void NetClient_OnSetWeather(int iMsg)
{
	// COOP: no weather here (lobby of a running game): read the data out of the message, the next message of the packet follows it
	if (!bWeatherLoaded)
	{
		string sSkipWeather = NMGetString(iMsg);
		float fSkipAngle = NMGetFloat(iMsg);
		float fSkipSpeed = NMGetFloat(iMsg);
		return;
	}

	NetClient_WhrDeleteRainEnvironment();
	NetClient_WhrDeleteSkyEnvironment();
	NetClient_WhrDeleteSunGlowEnvironment();
	NetClient_WhrDeleteLightningEnvironment();
	NetClient_WhrDeleteAstronomyEnvironment();

	DelEventHandler("NetClient_WhrOnCalcFogColor", "NetClient_WhrOnCalcFogColor");
	DelEventHandler("WhrCalcFogColor", "NetClient_WhrOnCalcFogColor");
	DelEventHandler("frame", "NetClient_WhrOnWindChange");
	DelEventHandler("frame", "NetClient_UpdateSky");

	NetClient_CreateWeatherEnvironment(iMsg);
}

void NetClient_AdminApplyWind(float fAngle, float fSpeed)
{
	if (COOP_WindBad(fAngle) || COOP_WindBad(fSpeed)) { return; }

	fClientWeatherAngle = fAngle;
	fClientWeatherSpeed = fSpeed;
	fClientWeatherDelta = 0.0;

	if (!bWeatherLoaded) { return; }

	// the same values as in NetClient_WhrOnWindChange
	NCWeather.WindAngle = fAngle;
	NCWeather.WindSpeed = fSpeed;
	Weather.Wind.Angle = fAngle;
	Weather.Wind.Speed = fSpeed;

	Particles.windpower = 0.05 * Clampf(5.0 * fSpeed / WIND_NORMAL_POWER);
	Particles.winddirection.x = sin(fAngle);
	Particles.winddirection.z = cos(fAngle);
}

void NetClient_OnSetWind(int iMsg)
{
	float fAngle = NMGetFloat(iMsg);
	float fSpeed = NMGetFloat(iMsg);

	NetClient_AdminApplyWind(fAngle, fSpeed);
}

// debug: delete and create the ships of this client again (only if iNetAdminClientReload is 1)
void NetClient_AdminReloadShips()
{
	if (iNetAdminClientReload == 0) { return; }

	for (int i=0; i<NET_MAXCLIENTS; i++)
	{
		if (!COOP_CliSlotLive(i)) { continue; }
		if (!sti(NCClients[i].Use)) { continue; }
		if (sti(NCClients[i].Ship.Type) < 0) { continue; }
		if (sti(NCClients[i].Dead)) { continue; }

		ref rC = NetClient_GetClient(i);

		// release the sounds of the old ship, NetClient_LoadCreateShip creates new ones
		if (CheckAttribute(rC, "Ship.Sounds.WaterID")) { ReleaseSound(sti(rC.Ship.Sounds.WaterID)); }
		if (CheckAttribute(rC, "Ship.Sounds.SailsID")) { ReleaseSound(sti(rC.Ship.Sounds.SailsID)); }
		if (CheckAttribute(rC, "Ship.Sounds.TurnID")) { ReleaseSound(sti(rC.Ship.Sounds.TurnID)); }

		DeleteClass(rC);
		NetClient_LoadCreateShip(i);
	}
}
