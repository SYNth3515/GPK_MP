// COOP: the ship of the convoy (and nobody else) is inside the point of the mode: the convoy team has won.
// Works in a solo game too: the sides do not have to meet for the objective of the mode.
bool NetServer_ConvoyReached()
{
	if (sti(NetServer.GameType) != NETGAME_CONVOY) { return false; }	// COOP: the check is safe for every mode
	if (sti(NetServer.ConvoyShipID) == DST_INVALID) { return false; }

	ref rClient = NetServer_GetClient(sti(NetServer.ConvoyShipID));
	if (!sti(rClient.Use)) { return false; }		// COOP: the convoy captain left the game
	if (stf(rClient.Ship.hp) <= 0.0) { return false; }
	if (sti(rClient.Team) != 0) { return false; }		// COOP: the captain changed to the defending team

	float x = stf(rClient.Ship.Pos.x);
	float z = stf(rClient.Ship.Pos.z);
	float fDistance = GetDistance2D(x, z, stf(NetServer.ConvoyPoint.x), stf(NetServer.ConvoyPoint.z));
	if (fDistance > CONVOY_POINT_RADIUS) { return false; }

	return true;
}

bool NetServer_CheckGameOver_Convoy()
{
	int i;
	bool bFlagCarrierAlive = false;

	// calculate survived ppl
	int iSurvivedTeams[2];
	for (i=0; i<2; i++) { iSurvivedTeams[i] = 0; }
	for (i=0; i<NET_MAXCLIENTS; i++) 
	{ 
		if (!sti(NSClients[i].Use)) { continue; } 
		if (stf(NSClients[i].Ship.hp) <= 0.0) { continue; } 

		int iTeam = sti(NSClients[i].Team);
		iSurvivedTeams[iTeam] = iSurvivedTeams[iTeam] + 1;
	}

	if (sti(NetServer.ConvoyShipID) != DST_INVALID)
	{
		ref rConvoyClient = NetServer_GetClient(sti(NetServer.ConvoyShipID));
		if (stf(rConvoyClient.Ship.hp) > 0.0) { bFlagCarrierAlive = true;	} 
	}
	
	// check if flag carrier dead
	if (!bFlagCarrierAlive)
	{
		NetServer.iWinnerTeam = 1;
		return true;
	}

	// check if first group(alive flag carrier) reached safe point (30 meters diameter)
	float fDistance = CONVOY_POINT_RADIUS + 1.0;
	if (sti(NetServer.ConvoyShipID) != DST_INVALID)
	{
		ref rClient = NetServer_GetClient(sti(NetServer.ConvoyShipID));
		
		float x = stf(rClient.Ship.Pos.x);
		float z = stf(rClient.Ship.Pos.z);

		fDistance = GetDistance2D(x, z, stf(NetServer.ConvoyPoint.x), stf(NetServer.ConvoyPoint.z));
	}
	if (fDistance <= CONVOY_POINT_RADIUS)
	{
		NetServer.iWinnerTeam = 0;
		return true;
	}

	// check if whole team 1 dead
	if (iSurvivedTeams[1] == 0)
	{
		NetServer.iWinnerTeam = 0;
		return true;
	}

	return false;
}