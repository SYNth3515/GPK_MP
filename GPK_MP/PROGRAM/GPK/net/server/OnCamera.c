void NetServer_OnCamera(int wNetClientID, int iMsg)
{
	int iSubCode = NMGetByte(iMsg);

	switch (iSubCode)
	{
		case NSC_DECK_CAMERA_UPDATE:
			EntityUpdate(0);
			ref rClient = NetServer_GetClient(wNetClientID);
			rClient.Camera.Dir.x = NMGetFloat(iMsg);
			rClient.Camera.Dir.y = NMGetFloat(iMsg);
			rClient.Camera.Dir.z = NMGetFloat(iMsg);
			EntityUpdate(1);

			rClient.IsCanFire = "";	// IsCanFire and CurrentTargetRelation updated here

			int iOurTeam = sti(rClient.Team);
			int iCurrentTargetTeam = sti(rClient.CurrentTargetTeam);

			int iSMsg = NMCreate();
			NMAddByte(iSMsg, NC_CAMERA);
			NMAddByte(iSMsg, NSC_DECK_CAMERA_UPDATE);
			NMAddByte(iSMsg, sti(rClient.IsCanFire));
			int iCamRelS = NetServer_GetTeamRelation(iOurTeam, iCurrentTargetTeam);
			// COOP s38: an ally under the crosshair was green for a moment only (the engine finds a target it may fire at):
			// a ship of our side along the camera keeps it green
			if (iCamRelS == 0)
			{
				int wCamAlly = NetServer_CamShipAlong(wNetClientID, rClient);
				if (wCamAlly != DST_INVALID)
				{
					ref rCamT = NetServer_GetClient(wCamAlly);
					if (NetServer_GetTeamRelation(iOurTeam, sti(rCamT.Team)) == 1) { iCamRelS = 1; }
				}
			}
			NMAddByte(iSMsg, iCamRelS);		// COOP s34: 3 = neutral too
			NetServer_SendMessage(wNetClientID, iSMsg, false);
			NMDelete(iSMsg);
		break;
	}
}

// the colour of the crosshair of the deck camera (net.dll: 0 Colors.Default, 1 Colors.Friend, 2 Colors.Enemy; COOP s34: 3 = neutral,
// the client shows it with Colors.Default = grey, client\Cameras.c). COOP s34: the bots: before, an allied bot (Team 5, not the
// team of the player) was an enemy, a neutral one too
float NetServer_CamFAbs(float f)
{
	if (f < 0.0) { return 0.0 - f; }
	return f;
}

// COOP s38: the nearest ship along the camera of the player (in 1500 m, 14 m to the side at most); DST_INVALID = none
int NetServer_CamShipAlong(int w, ref rC)
{
	if (!CheckAttribute(rC, "Ship.Pos.x") || !CheckAttribute(rC, "Camera.Dir.x")) { return DST_INVALID; }
	float fDX = stf(rC.Camera.Dir.x);
	float fDZ = stf(rC.Camera.Dir.z);
	float fDL = sqrt(fDX * fDX + fDZ * fDZ);
	if (fDL < 0.01) { return DST_INVALID; }
	fDX = fDX / fDL;
	fDZ = fDZ / fDL;
	float fPX = stf(rC.Ship.Pos.x);
	float fPZ = stf(rC.Ship.Pos.z);
	int wBest = DST_INVALID;
	float fBest = 1500.0;
	for (int i=0; i<NET_MAXCLIENTS; i++)
	{
		if (i == w) { continue; }
		if (!NetServer_HasShip(i)) { continue; }
		ref rT = NetServer_GetClient(i);
		if (CheckAttribute(rT, "Dead")) { if (sti(rT.Dead)) { continue; } }
		if (!CheckAttribute(rT, "Ship.Pos.x") || !CheckAttribute(rT, "Team")) { continue; }
		float fTX = stf(rT.Ship.Pos.x) - fPX;
		float fTZ = stf(rT.Ship.Pos.z) - fPZ;
		float fAlong = fTX * fDX + fTZ * fDZ;
		if (fAlong < 5.0 || fAlong > fBest) { continue; }
		float fSide = NetServer_CamFAbs(fTX * fDZ - fTZ * fDX);
		if (fSide > 14.0) { continue; }
		wBest = i;
		fBest = fAlong;
	}
	return wBest;
}

int NetServer_GetTeamRelation(int iOurTeam, int iCurrentTargetTeam)
{
	if (iCurrentTargetTeam == -1) return 0;							// nothing selected
	if (iCurrentTargetTeam == NetServer_BotTeamOf(NET_BOTREL_ALLY)) return 1;		// an allied bot
	if (iCurrentTargetTeam == NetServer_BotTeamOf(NET_BOTREL_ENEMY)) return 2;		// an enemy bot
	if (iCurrentTargetTeam == NetServer_BotTeamOf(NET_BOTREL_NEUTRAL)) return 3;	// a neutral bot
	if (sti(NetServer.GameType) == NETGAME_DEATHMATCH) return 2;	// enemy
	if (iOurTeam != iCurrentTargetTeam) return 2;					// enemy
	return 1;														// friend
}