bool NetServer_CheckGameOver_Fort()
{
	int i;

	// check if fort dead... (COOP: server\Fort.c - also a fort that the engine took away and one without a cannon left)
	if (NetServer_FortGameDead())
	{
		NetServer.iWinnerTeam = 1;
		Trace("COOP: the fort of the game mode is destroyed, the attackers won");
		return true;
	}

	// calculate survived ppl
	int iSurvivedTeams[2];
	for (i=0; i<2; i++) { iSurvivedTeams[i] = 0; }
	for (i=0; i<NET_MAXCLIENTS; i++)
	{
		if (!NetServer_TeamSideSlot(i)) { continue; }		// COOP s28: the players and the bots of the teams
		if (stf(NSClients[i].Ship.hp) <= 0.0) { continue; }

		int iTeam = sti(NSClients[i].Team);
		if (iTeam < 0 || iTeam > 1) { continue; }
iSurvivedTeams[iTeam] = iSurvivedTeams[iTeam] + 1;
	}

	// check if fort command dead... [0]
	if (iSurvivedTeams[0] == 0 && iSurvivedTeams[1] > 0)
	{
		// playlogic want it to disable
		//NetServer.iWinnerTeam = 1;
		//return true;
	}

	// check if fort attackers dead... [1]
	if (iSurvivedTeams[0] > 0 && iSurvivedTeams[1] == 0)
	{
		NetServer.iWinnerTeam = 0;
		return true;
	}

	// if both teams dead
	if (iSurvivedTeams[0] == 0 && iSurvivedTeams[1] == 0)
	{
		NetServer.iWinnerTeam = 0;
		return true;
	}

	return false;
}