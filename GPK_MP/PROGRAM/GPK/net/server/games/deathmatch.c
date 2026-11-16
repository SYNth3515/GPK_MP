bool NetServer_CheckGameOver_DeathMatch()
{
	int i;
	int iNumAlive = 0;
	// if only one client alive - game over
	for (i=0; i<NET_MAXCLIENTS; i++) 
	{ 
		if (!sti(NSClients[i].Use)) { continue; } 
		if (stf(NSClients[i].Ship.hp) <= 0.0) { continue; } 
		iNumAlive++;
		NetServer.wWinnerClientID = NSClients[i].id;
	}

	// COOP: ships alive on the previous check: 2 -> 0 in one step means the last two ships sank together (mine, brander...)
	int iPrevAlive = 0;
	if (CheckAttribute(&NetServer, "PrevAlive")) { iPrevAlive = sti(NetServer.PrevAlive); }
	NetServer.PrevAlive = iNumAlive;

	if (iNumAlive == 1) { return true; }

	if (iNumAlive == 0) // all dead
	{
		// COOP: only two players were left and both are dead now: a draw, nobody wins
		if (iPrevAlive == 2)
		{
			NetServer.Draw = 1;
			NetServer.wWinnerClientID = DST_INVALID;
			return true;
		}

		// more than two players were left - check one client who did most damage
		int iMaxDamage = -1;
		for (i=0; i<NET_MAXCLIENTS; i++) 
		{ 
			if (!sti(NSClients[i].Use)) { continue; } 
			int iDamage = sti(NSClients[i].Stat.DamageInflicted);
			if (iDamage < iMaxDamage) { continue; }

			iMaxDamage = iDamage;
			NetServer.wWinnerClientID = NSClients[i].id;
		}
		return true;		
	}

	return false;
}