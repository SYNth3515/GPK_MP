int iClientShipPriorityExecute = 2;
int iClientShipPriorityRealize = 31;
int NetClient_BoardTarget = -1;
bool NetClient_BoardWasAvailable = false;

// ---------------------------------------------------------------------------------------------
// COOP: sea battle voices / music like in single player
// ---------------------------------------------------------------------------------------------
#define NET_LOWHP_PERCENT		15.0		// below this % of max HP: "water in the hold" / "ship is falling apart" (single: 10%)

bool NetClient_LowHPWarned = false;		// low HP warning already played (re-armed when HP goes back up)
bool NetClient_BortArmL = false;		// left bort was seen not loaded -> "ready" voice is allowed
bool NetClient_BortArmR = false;
bool NetClient_BortReadyL = false;		// left/right bort became ready in the current 300 ms window
bool NetClient_BortReadyR = false;
bool NetClient_BortEventPosted = false;
float NetClient_PrevCrew = -1.0;		// previous crew of this client (-1 = unknown)

void NetClient_DeleteShipEnvironment()
{
	DelEventHandler("Control Activation", "NetClient_ShipControlActivation");
	DelEventHandler("Control Deactivation", "NetClient_ShipControlDeactivation");
	DelEventHandler("NetClient_ShipActivateFirePlace", "NetClient_ShipActivateFirePlace");
	DelEventHandler("NetClient_ShipSeriousBoom", "NetClient_ShipSeriousBoomEvent");
	DelEventHandler("NetClient_ShipDetonateSmall", "NetClient_ShipDetonateSmall");
	DelEventHandler("NetClient_ShipUpdateSounds", "NetClient_ShipUpdateSounds");
	DelEventHandler("NetClient_ShipGetCurrentTargetRelation", "NetClient_ShipGetCurrentTargetRelation");
	DelEventHandler("Interface_Started", "NetClient_DropCurrentControls");
	DelEventHandler("NetClient_BortReloadEvent", "NetClient_BortReloadEvent");
	NetClient_BoardTarget = -1;
	NetClient_BoardWasAvailable = false;
	NetClient_ResetSeaVoices();

	DeleteClass(&NCShipTracks);
}

void NetClient_CreateShipEnvironment(int iMsg)
{
	SetEventHandler("Control Activation", "NetClient_ShipControlActivation", 0);
	SetEventHandler("Control Deactivation", "NetClient_ShipControlDeactivation", 0);
	SetEventHandler("NetClient_ShipActivateFirePlace", "NetClient_ShipActivateFirePlace", 0);
	SetEventHandler("NetClient_ShipSeriousBoom", "NetClient_ShipSeriousBoomEvent", 0);
	SetEventHandler("NetClient_ShipDetonateSmall", "NetClient_ShipDetonateSmall", 0);
	SetEventHandler("NetClient_ShipUpdateSounds", "NetClient_ShipUpdateSounds", 0);
	SetEventHandler("NetClient_ShipGetCurrentTargetRelation", "NetClient_ShipGetCurrentTargetRelation", 0);
	SetEventHandler("Interface_Started", "NetClient_DropCurrentControls", 0);
	SetEventHandler("NetClient_BortReloadEvent", "NetClient_BortReloadEvent", 0);
	NetClient_BoardTarget = -1;
	NetClient_BoardWasAvailable = false;
	NetClient_ResetSeaVoices();

	NCShipTracks.Server = false;
	CreateEntity(&NCShipTracks, "NetShipTracks");		
	LayerAddObject("net_execute", &NCShipTracks, 65531);
	LayerAddObject("net_realize", &NCShipTracks, 65531);

	aref arCurWeather = NetClient_GetCurrentWeather();
	sSeaSoundPostfix = arCurWeather.Sounds.Sea.Postfix;
}

int NetClient_ShipGetCurrentTargetRelation()
{
	ref rClient = NetClient_GetThisClient();

	return sti(rClient.CurrentTargetRelation);
}

void NetClient_ShipSetSailState(float fNewSailState)
{
	int iSMsg = NMCreate();
	NMAddClientID(iSMsg, wClientID);
	NMAddByte(iSMsg, NC_SHIP);
	NMAddByte(iSMsg, NSC_SHIP_SETSAILSTATE);
	NMAddByte(iSMsg, makeint(fNewSailState * 2.0));
	NetClient_SendMessage(iSMsg, false);
	NMDelete(iSMsg);
}

void NetClient_ShipStartRepair()
{
	int iSMsg = NMCreate();
	NMAddClientID(iSMsg, wClientID);
	NMAddByte(iSMsg, NC_SHIP);
	NMAddByte(iSMsg, NSC_SHIP_START_REPAIR);
	NetClient_SendMessage(iSMsg, false);
	NMDelete(iSMsg);
}

// COOP: the client camera can be NaN (uninitialized ship height); a NaN fire request freezes the host
bool COOP_CliFloatOk(float f)
{
	return (f > -100000.0) && (f < 100000.0);
}

bool COOP_CliCamOk()
{
	bool bOk = COOP_CliFloatOk(stf(NCDeckCamera.Pos.x)) && COOP_CliFloatOk(stf(NCDeckCamera.Pos.y)) && COOP_CliFloatOk(stf(NCDeckCamera.Pos.z));
	bOk = bOk && COOP_CliFloatOk(stf(NCDeckCamera.Dir.x)) && COOP_CliFloatOk(stf(NCDeckCamera.Dir.y)) && COOP_CliFloatOk(stf(NCDeckCamera.Dir.z));
	if (!bOk) { Trace("COOP: cli fire blocked, deck camera is not finite"); }
	return bOk;
}

void NetClient_ShipControlActivation()
{
	string sControl = GetEventData();
	Trace("COOP: ctl " + sControl + " deckOn=" + NCDeckCamera.On + " shipOn=" + NCShipCamera.On + " canFire=" + NCDeckCamera.IsCanFire + " camPos=" + NCDeckCamera.Pos.x + "," + NCDeckCamera.Pos.y + "," + NCDeckCamera.Pos.z + " camDir=" + NCDeckCamera.Dir.x + "," + NCDeckCamera.Dir.y + "," + NCDeckCamera.Dir.z);

	bool bSend = false;

	int iSMsg = NMCreate();
	NMAddClientID(iSMsg, wClientID);
	NMAddByte(iSMsg, NC_SHIP);
	switch (sControl)
	{
		case "Ship_TurnLeft":
			NMAddByte(iSMsg, NSC_SHIP_TURNLEFT_ON);
			bSend = true;
		break;
		case "Ship_TurnRight":
			NMAddByte(iSMsg, NSC_SHIP_TURNRIGHT_ON);
			bSend = true;
		break;
		case "Ship_StartRepair":
			NetClient_ShipStartRepair();
			bSend = false;
		break;
		case "Ship_Board":
			NetClient_RequestBoarding();
			bSend = false;
		break;
		case "Ship_SailUp":
			NMAddByte(iSMsg, NSC_SHIP_SAILUP);
			bSend = true;
		break;
		case "Ship_SailDown":
			NMAddByte(iSMsg, NSC_SHIP_SAILDOWN);
			bSend = true;
		break;
		case "Ship_Fire":
			if (sti(NCDeckCamera.On))
			{
				if (sti(NCDeckCamera.IsCanFire) && COOP_CliCamOk())
				{
					NMAddByte(iSMsg, NSC_SHIP_FIRE);
					NMAddByte(iSMsg, 1);
					NMAddFloat(iSMsg, stf(NCDeckCamera.Pos.x));
					NMAddFloat(iSMsg, stf(NCDeckCamera.Pos.y));
					NMAddFloat(iSMsg, stf(NCDeckCamera.Pos.z));
					NMAddFloat(iSMsg, stf(NCDeckCamera.Dir.x));
					NMAddFloat(iSMsg, stf(NCDeckCamera.Dir.y));
					NMAddFloat(iSMsg, stf(NCDeckCamera.Dir.z));
					bSend = true;
				}
			}
			if (sti(NCShipCamera.On))
			{
				NMAddByte(iSMsg, NSC_SHIP_FIRE);
				NMAddByte(iSMsg, 0);
				bSend = true;
			}
		break;
		case "Sea_CameraSwitch":
			NetClient_CameraSwitch();
		break;
		case "hk_charge1": NetClient_PlayChargeVoice(1); NetClient_ShipChangeCharge("Balls");	break;
		case "hk_charge2": NetClient_PlayChargeVoice(2); NetClient_ShipChangeCharge("Grapes"); break;
		case "hk_charge3": NetClient_PlayChargeVoice(3); NetClient_ShipChangeCharge("Knippels"); break;
		case "hk_charge4": NetClient_PlayChargeVoice(4); NetClient_ShipChangeCharge("Bombs"); break;
	}
	if (bSend) { NetClient_SendMessage(iSMsg, false); }

	NMDelete(iSMsg);
}

void NetClient_DropCurrentControls()
{
	int iSMsg = NMCreate();
	NMAddClientID(iSMsg, wClientID);
	NMAddByte(iSMsg, NC_SHIP);
	NMAddByte(iSMsg, NSC_SHIP_TURNLEFT_OFF);
	NetClient_SendMessage(iSMsg, false);
	NMDelete(iSMsg);

	iSMsg = NMCreate();
	NMAddClientID(iSMsg, wClientID);
	NMAddByte(iSMsg, NC_SHIP);
	NMAddByte(iSMsg, NSC_SHIP_TURNRIGHT_OFF);
	NetClient_SendMessage(iSMsg, false);
	NMDelete(iSMsg);
}

void NetClient_ShipControlDeactivation()
{
	string sControl = GetEventData();

	bool bSend = false;

	int iSMsg = NMCreate();
	NMAddClientID(iSMsg, wClientID);
	NMAddByte(iSMsg, NC_SHIP);

	switch (sControl)
	{
		case "Ship_TurnLeft":
			NMAddByte(iSMsg, NSC_SHIP_TURNLEFT_OFF);
			bSend = true;
		break;
		case "Ship_TurnRight":
			NMAddByte(iSMsg, NSC_SHIP_TURNRIGHT_OFF);
			bSend = true;
		break;
	}
	if (bSend) { NetClient_SendMessage(iSMsg, false); }

	NMDelete(iSMsg);
}

void NetClient_ShipActivateFirePlace()
{
	int wOurClientID = GetEventData();
	int iFirePlaceIndex = GetEventData();
	float fFireTime = GetEventData();
	
	SendMessage(NetClient_GetClient(wOurClientID), "llsssf", MSG_SHIP_ACTIVATE_FIRE_PLACE, iFirePlaceIndex, "Ship_Fire", "", "ship_onfire", fFireTime);
}

void NetClient_OnShip(int iMsg)
{
	int iTime; 
	float x, y, z;

	int iSubCode = NMGetByte(iMsg);
	
	switch (iSubCode)
	{
		case NSC_SHIP_MAST_FALL:
			NetClient_OnShipMastFall(iMsg);
		break;
		case NSC_SHIP_INFO:
			NetClient_OnShipInfo(iMsg);
		break;
		case NSC_SHIP_SERIOUS_BOOM:
			iTime = NMGetDword(iMsg);
			x = NMGetFloat(iMsg);
			y = NMGetFloat(iMsg);
			z = NMGetFloat(iMsg);
			PostEvent("NetClient_ShipSeriousBoom", iTime - iClientTime, "fff", x, y, z);
		break;
		case NSC_SHIP_ACTIVATE_FIREPLACE:
			int wOurClientID = NMGetClientID(iMsg);
			int wFireClientID = NMGetClientID(iMsg);
			int iFirePlaceIndex = NMGetWord(iMsg);
			iTime = NMGetDword(iMsg);
			float fTotalFireTime = NMGetFloat(iMsg);
			PostEvent("NetClient_ShipActivateFirePlace", iTime - iClientTime, "llf", wOurClientID, iFirePlaceIndex, fTotalFireTime);
		break;
		case NSC_SHIP_DEAD:
			NetClient_OnShipDead(iMsg);
		break;
		case NSC_SHIP_BORTS_INFO:
			NetClient_OnShipBortsInfo(iMsg);
		break;
		case NSC_SHIP_SAIL_STATE:
			NetClient_OnShipSailDamage(iMsg);
		break;
		case NSC_SHIP_CHANGE_CHARGE:
			NetClient_OnShipChangeCharge(iMsg);
		break;
		case NSC_SHIP_SHIP_COLLISION:
			NetClient_OnShipShipCollision(iMsg);
		break;
		case NSC_SHIP_BOARD_STATE:
			NetClient_OnShipBoardState(iMsg);
		break;
		case NSC_SHIP_BOARD_VOLLEY:
			NetClient_OnShipBoardVolley(iMsg);
		break;
		case NSC_SHIP_BOARD_RESULT:
			NetClient_OnShipBoardResult(iMsg);
		break;
		case NSC_SHIP_ISLAND_COLLISION:
			NetClient_OnShipIslandCollision(iMsg);
		break;
	}
}

void NetClient_OnShipShipCollision(int iMsg)
{
	float x = NMGetFloat(iMsg);
	float y = NMGetFloat(iMsg);
	float z = NMGetFloat(iMsg);

	Play3DSound("coll_ship2ship", x, y, z); 
}

void NetClient_OnShipIslandCollision(int iMsg)
{
	float x = NMGetFloat(iMsg);
	float y = NMGetFloat(iMsg);
	float z = NMGetFloat(iMsg);

	Play3DSound("coll_ship2rock", x, y, z); 
}

void NetClient_OnShipChangeCharge(int iMsg)
{
	int WOurClientID = NMGetClientID(iMsg);
	int iChargeType = NMGetByte(iMsg);

	ref rClient = NetClient_GetClient(WOurClientID);
	rClient.Ship.Cannons.Charge.Type = iChargeType;
}

void NetClient_OnShipBortsInfo(int iMsg)
{
	ref rClient = NetClient_GetThisClient();

	float fRatioL = 1.0;
	float fRatioR = 1.0;

	rClient.Ship.Cannons.Borts = "";
	for (int i=0; i<4; i++)
	{
		aref arBorts; makearef(arBorts, rClient.Ship.Cannons.Borts);
		string sCurBort = sNetBortNames[i];

		arBorts.(sCurBort).MaxFireDistance = NMGetWord(iMsg);
		arBorts.(sCurBort).ChargeRatio = makefloat(NMGetByte(iMsg)) / 255.0;
		arBorts.(sCurBort).DamageRatio = makefloat(NMGetByte(iMsg)) / 255.0;

		if (i == 0) { fRatioL = stf(arBorts.(sCurBort).ChargeRatio); }
		if (i == 1) { fRatioR = stf(arBorts.(sCurBort).ChargeRatio); }
	}

	NetClient_CheckBortsReady(fRatioL, fRatioR);
}

void NetClient_OnShipDead(int iMsg)
{
	int wDeadID = NMGetClientID(iMsg);
	int iKillStatus = NMGetByte(iMsg);
	int wKillerID = NMGetClientID(iMsg);
	bool bDetonate = NMGetByte(iMsg);

	float fSinkSpeedX = NMGetFloat(iMsg);
	float fSinkSpeedY = NMGetFloat(iMsg);
	float fSinkSpeedZ = NMGetFloat(iMsg);

	if (wDeadID == wClientID)
	{
		NetClient_SetDieCamera(wKillerID);

		// single: death screen music (music_ship_dead)
		FadeOutMusic(2);
		PlayStereoOGG("music_ship_dead");
		NetClient_LowHPWarned = false;
	}

	ref rDeadClient = NetClient_GetClient(wDeadID);

	// single: ship_sink sound at the sinking ship
	Play3DSound("ship_sink", stf(rDeadClient.Ship.Pos.x), stf(rDeadClient.Ship.Pos.y), stf(rDeadClient.Ship.Pos.z));

	rDeadClient.Ship.Sink.Speed.x = fSinkSpeedX;
	rDeadClient.Ship.Sink.Speed.y = fSinkSpeedY;
	rDeadClient.Ship.Sink.Speed.z = fSinkSpeedZ;

	if (bDetonate) { NetClient_ShipDetonate(rDeadClient, true); }

	if (NetClient_IsCLient(wKillerID))
	{
		ref rClient = NetClient_GetClient(wKillerID);

		aref arKills; makearef(arKills, rClient.Stat.Kills);
		int iNumKills = GetAttributesNum(arKills) + 1;
		string sKill = "l" + iNumKills;

		rClient.Stat.Kills.(sKill).NickName = rDeadClient.NickName;
		rClient.Stat.Kills.(sKill).FaceTexture = rDeadClient.UserData.Face.Texture;
		rClient.Stat.Kills.(sKill).ShipName = rDeadClient.Ship.Name;
		rClient.Stat.Kills.(sKill).ShipType = rDeadClient.Ship.Type;
	}

	switch (iKillStatus)
	{
		case NETKILL_BY_BALL:
			// if (bCriticalHit)
			//	NetClient_ChatAddGameMessage("Net_ShipDieFromCriticalHit", rgbGameMessage, 2, wKillerID, wDeadID);
			// else
			NetClient_ChatAddGameMessage("Net_ShipDieFromPlayer", rgbGameMessage, 2, wKillerID, wDeadID);
		break;
		case NETKILL_BY_ISLAND:
			NetClient_ChatAddGameMessage("Net_ShipDieFromCollisionIsland", rgbGameMessage, 1, wDeadID, DST_INVALID);
		break;
		case NETKILL_BY_TOUCH_DEAD: 
			NetClient_ChatAddGameMessage("Net_ShipDieFromCollisionDeadShip", rgbGameMessage, 1, wDeadID, DST_INVALID);
		break;
		case NETKILL_BY_TOUCH_ALIVE:
			NetClient_ChatAddGameMessage("Net_ShipDieFromCollisionLiveShip", rgbGameMessage, 2, wKillerID, wDeadID);
		break;
		case NETKILL_BY_LIGHTNING:
			NetClient_ChatAddGameMessage("Net_ShipDieFromLighting", rgbGameMessage, 1, wDeadID, DST_INVALID);
		break;
		case NETKILL_BY_BOARDING:
			// own message id (add it to the ini, same 2 parameters as Net_ShipDieFromPlayer: winner, loser)
			NetClient_ChatAddGameMessage("Net_ShipDieFromBoarding", rgbGameMessage, 2, wKillerID, wDeadID);
			if (wKillerID == wClientID || wDeadID == wClientID) { PlayStereoSound(NET_SOUND_ABORDAGE); }
			// the whole crew of the defeated ship died with it, the sinking ship must not show any crew
			rDeadClient.Ship.Crew.Quantity = 0;
		break;
	}

	if (wDeadID == wClientID)
	{
		NetClient_BoardTarget = -1;
		if (NetClient_BoardWasAvailable) { Log_SetActiveAction("Nothing"); }
		NetClient_BoardWasAvailable = false;
	}

	rDeadClient.Dead = true;
	
	Event("NCEvent_ShipDead");
}

void NetClient_OnShipMastFall(int iMsg)
{
	int wMastClientID = NMGetClientID(iMsg);
	int wBallClientID = NMGetClientID(iMsg);
	int iTouchMode = NMGetByte(iMsg); 
	int iMastNum = NMGetByte(iMsg);

	ref rClient = NetClient_GetClient(wMastClientID);

	switch (iTouchMode)
	{
		case NETSHIP_MAST_TOUCH_SHIP:
			NetClient_ChatAddGameMessage("Net_MastFallFromCollision", rgbGameMessage, 1, wMastClientID, DST_INVALID);
		break;
		case NETSHIP_MAST_TOUCH_ISLAND:
			NetClient_ChatAddGameMessage("Net_MastFallFromCollision", rgbGameMessage, 1, wMastClientID, DST_INVALID);
		break;
		case NETSHIP_MAST_TOUCH_BALL:
			NetClient_ChatAddGameMessage("Net_MastFallFromHit", rgbGameMessage, 2, wBallClientID, wMastClientID);
		break;
	}

	rClient.MastFall = iMastNum;

	// single: mast_fall sound
	Play3DSound("mast_fall", stf(rClient.Ship.Pos.x), stf(rClient.Ship.Pos.y), stf(rClient.Ship.Pos.z));
}

void NetClient_OnShipInfo(int iMsg)
{
	int wOurClientID = NMGetClientID(iMsg);
	ref rClient = NetClient_GetClient(wOurClientID);

	ref rBall = Net_GetGoodByIndex(sti(rClient.Ship.Cannons.Charge.Type));
	string sBallName = rBall.name;

	int iMsgServerTime = NMGetDword(iMsg);

	rClient.Ship.SailState = NMGetByte(iMsg) / 2.0;
	
	float x = NMGetFloat(iMsg);
	float y = NMGetFloat(iMsg);
	float z = NMGetFloat(iMsg);

	//float sx = NMGetWord(iMsg) / 255.0;
	//float sy = NMGetWord(iMsg) / (65535.0);
	//float sz = NMGetWord(iMsg) / 255.0;

	float sx = NMGetFloat(iMsg);
	float sy = NMGetFloat(iMsg);
	float sz = NMGetFloat(iMsg);

	float fHP = NMGetWord(iMsg);

	float fCrew = NMGetWord(iMsg);
	int iBallsNum = NMGetWord(iMsg);

	// statistic
	int iHits = NMGetWord(iMsg);
	int iMisses = NMGetWord(iMsg);
	float fDamageInflicted = NMGetFloat(iMsg);

	// repair
	int iTotalCooldownTime = 0;
	int iCurrentRepairTime = 0;
	int iRepairActive = NMGetByte(iMsg);
	//if (iRepairActive > 0)
	//{
		iTotalCooldownTime = NMGetWord(iMsg);
		iCurrentRepairTime = NMGetWord(iMsg);
	//}

	// COOP: sail state in percent (server value), see NetClient_OnShipInfo below
	int iSailPercent = NMGetByte(iMsg);

	rClient.Ship.HP = fHP;
	rClient.Ship.Crew.Quantity = fCrew;

	// Ship.SP is kept in units of the ship base SP (like single: SP / ship.SP = sail state), so the bars over the ships
	// and Net_GetSailPercent() show the real percent
	ref rSPShip = Net_GetShipByIndex(sti(rClient.Ship.Type));
	rClient.Ship.SP = makefloat(iSailPercent) * stf(rSPShip.SP) / 100.0;

	if (wOurClientID == wClientID)
	{
		NetClient_CheckLowHP(fHP, stf(rClient.Ship.MaxHP));
		NetClient_CheckMinCrew(fCrew, rClient);
	}

	rClient.Ship.NewPos.x = x;
	rClient.Ship.NewPos.y = y;
	rClient.Ship.NewPos.z = z;

	rClient.Ship.Speed.x = sx;
	rClient.Ship.Speed.y = sy;
	rClient.Ship.Speed.z = sz;

	rClient.Ship.Goods.(sBallName) = iBallsNum;

	if (iHits + iMisses > 0) 
		{ rClient.Stat.Accuracy = makeint((iHits * 100) / (iHits + iMisses)); }
	else 
		{ rClient.Stat.Accuracy = 0; }
	rClient.Stat.DamageInflicted = fDamageInflicted;

	rClient.Skills.Repair.Active = iRepairActive;
	rClient.Skills.Repair.TotalCooldownTime = iTotalCooldownTime;
	rClient.Skills.Repair.CurrentCooldownTime = iCurrentRepairTime;

	// COOP: the ship may not exist yet (it is created a moment after the load message, see NetClient_ShipCreateLater)
	if (sti(rClient.CoopReady))
	{
		SendMessage(rClient, "llffffff", MSG_SHIP_CURVES, iMsgServerTime, x, y, z, sx, sy, sz);
	}
}

void NetClient_Ship_PlaySound3DComplex(ref rClient, string sSoundID, float fVolume, float fX, float fY, float fZ)
{
	//return;
	float x = stf(rClient.Ship.Pos.x);
	float y = stf(rClient.Ship.Pos.y);
	float z = stf(rClient.Ship.Pos.z);

	float aY = stf(rClient.Ship.Ang.y);

	float x1 = fX * stf(rClient.Ship.BoxSize.x) / 2.0;
	float z1 = fZ * stf(rClient.Ship.BoxSize.z) / 2.0;
	RotateAroundY(&x1, &z1, cos(aY), sin(aY));

	int iSoundID = Play3DSound(sSoundID, x + x1, y + fY * stf(rClient.Ship.BoxSize.y) / 2.0, z + z1);
	if (iSoundID > 0) { Sound_SetVolume(iSoundID, fVolume); }
}

void NetClient_ShipUpdateSounds()
{
	int wOurClientID = GetEventData();
	PostEvent("NetClient_ShipUpdateSounds", 1000, "l", wOurClientID)

	ref rClient = NetClient_GetClient(wOurClientID);

	float fSailState = stf(rClient.Ship.SailState);

	// start random sounds :: SAILS
	if (rand(40) <= fSailState * 5.0) { NetClient_Ship_PlaySound3DComplex(rClient, "sails_ambient" + sSeaSoundPostfix, fSailState * 0.5, 0.0, 0.0, 0.5 + frnd() * 0.0); }

	// start random sounds :: SHIPS
	if (rand(2) == 1) { NetClient_Ship_PlaySound3DComplex(rClient, "squeak_sea" + sSeaSoundPostfix, 0.9, frnd() * 1.2 - 0.6, 0.0, frnd() * 1.6 - 0.8); }
}

float NetClient_ShipGetSailState(int wOurClientID)
{
	ref rClient = NetClient_GetClient(wOurClientID);
	return stf(rClient.Ship.SailState);
}

void NetClient_ShipSeriousBoom(float x, float y, float z)
{
	CreateBlast(x,y,z);
	CreateParticleSystem("blast_inv", x, y, z, 0.0, 0.0, 0.0, 0);
	Play3DSound("ship_explosion", x, y, z);
}

void NetClient_ShipSeriousBoomEvent()
{
	float x = GetEventData();
	float y = GetEventData();
	float z = GetEventData();

	NetClient_ShipSeriousBoom(x, y, z);
}

void NetClient_ShipDetonateSmall()
{
	int wNetID = GetEventData();

	ref rClient = NetClient_GetClient(wNetID);
	aref arClientShip; makearef(arClientShip, rClient.Ship);
	ref rShip = Net_GetShipByIndex(sti(arClientShip.Type));

	float x = stf(arClientShip.Pos.x);
	float y = stf(arClientShip.Pos.y);
	float z = stf(arClientShip.Pos.z);

	if (y > -3.0)
	{
		int iTime = 200 + sti(rShip.Class) * 300 + rand(900);
		PostEvent("NetClient_ShipDetonateSmall", iTime, "l", wNetID);
	}

	float bx = arClientShip.BoxSize.x;
	float by = arClientShip.BoxSize.y;
	float bz = arClientShip.BoxSize.z;

	float fCos = cos(stf(arClientShip.Ang.y));
	float fSin = sin(stf(arClientShip.Ang.y));

	for (int i=0; i < 1 + rand(1); i++)
	{
		float x1, y1, z1;
		x1 = (frnd() - 0.5) * bx;
		z1 = (frnd() - 0.5) * bz;

		RotateAroundY(&x1, &z1, fCos, fSin);

		y1 = 1.0 + y + frnd() * 3.0;
		Ship_Serious_Boom(x1 + x, y1, z1 + z);

		//CreateParticleSystem("blast", x1 + x, y1, z1 + z, 0.0, 0.0, 0.0, 0);
	}
}

// Big Boom on ship
void NetClient_ShipDetonate(ref rClient, bool bMakeSmallBoom)
{
	aref arClientShip; makearef(arClientShip, rClient.Ship);
	ref rShip = Net_GetShipByIndex(sti(arClientShip.Type));

	float x = stf(arClientShip.Pos.x);
	float y = stf(arClientShip.Pos.y);
	float z = stf(arClientShip.Pos.z);

	int iMinBoom = MakeInt(Bring2Range(6.0, 2.0, 1.0, 7.0, stf(rShip.Class)));

	float bx = arClientShip.BoxSize.x;
	float by = arClientShip.BoxSize.y;
	float bz = arClientShip.BoxSize.z;

	float fCos = cos(stf(arClientShip.Ang.y));
	float fSin = sin(stf(arClientShip.Ang.y));

	for (int i=0; i<iMinBoom + rand(2); i++)
	{
		float x1, y1, z1;
		x1 = (frnd() - 0.5) * bx;
		z1 = (frnd() - 0.5) * bz;

		RotateAroundY(&x1, &z1, fCos, fSin);

		y1 = 1.0 + y + frnd() * 3.0;
		Ship_Serious_Boom(x1 + x, y1, z1 + z);
		CreateParticleSystem("blast", x1 + x, y1, z1 + z, 0.0, 0.0, 0.0, 0);
	}

	if (bMakeSmallBoom) { PostEvent("NetClient_ShipDetonateSmall", 600 + rand(1000), "l", sti(rClient.index)); }
}

void NetClient_ShipChangeCharge(string sNewCharge)
{
	int iSMSg = NMCreate();
	NMAddClientID(iSMSg, wClientID);
	NMAddByte(iSMSg, NC_SHIP);
	NMAddByte(iSMSg, NSC_SHIP_CHANGE_CHARGE);
	NMAddString(iSMSg, sNewCharge, 32);
	NetClient_SendMessage(iSMSg, false);
	NMDelete(iSMsg);
}

// The server checks distance / angle / speed / skills / perks (rules of single player) and tells us
// which enemy ship can be boarded right now. Here we only show / hide the quick action (Space).
void NetClient_OnShipBoardState(int iMsg)
{
	int iHasTarget = NMGetByte(iMsg);
	int wTarget = NMGetClientID(iMsg);

	NetClient_BoardTarget = -1;
	if (iHasTarget != 0) { NetClient_BoardTarget = wTarget; }

	bool bAvailable = (NetClient_BoardTarget >= 0);

	if (bAvailable && !NetClient_BoardWasAvailable)
	{
		Log_SetActiveAction("Board");
		PlaySound("interface\\_GTBoard0.wav");
	}
	if (!bAvailable && NetClient_BoardWasAvailable)
	{
		Log_SetActiveAction("Nothing");
	}

	NetClient_BoardWasAvailable = bAvailable;
}

// Text in the single player log (top corner, the place where "Time x2" and the other ship notices appear).
// sKey is a text id from NetSection.txt, %1 and %2 are replaced by the numbers.
void NetClient_LogMessageNum(string sKey, int iNum1, int iNum2)
{
	object oRes;
	string sText = LanguageConvertString(iLangNetClient, sKey);
	string sNum1 = "" + iNum1;
	string sNum2 = "" + iNum2;

	oRes.Str = "";
	Event(PARSE_STRING, "aslss", &oRes, sText, 2, sNum1, sNum2);
	Log_SetStringToLog(oRes.Str);
}

// Musket volley before the boarding. Only the two captains of the boarding get it, from their point of view.
void NetClient_OnShipBoardVolley(int iMsg)
{
	int iOurShot = NMGetByte(iMsg);
	int iEnemyShot = NMGetByte(iMsg);
	int iKilledByUs = NMGetWord(iMsg);
	int iKilledByEnemy = NMGetWord(iMsg);

	PlaySound(NET_SOUND_MUSKETSHOT);
	PlaySound("interface\_GTBoard2.wav");	// single: boarding starts

	// texts: Net_BoardMusketVolleyOur (our volley killed %1 enemy sailors) / Net_BoardMusketVolleyEnemy (enemy volley killed %1 of ours)
	if (iOurShot != 0) { NetClient_LogMessageNum("Net_BoardMusketVolleyOur", iKilledByUs, 0); }
	if (iEnemyShot != 0) { NetClient_LogMessageNum("Net_BoardMusketVolleyEnemy", iKilledByEnemy, 0); }
}

// Crew lost in the boarding fight itself (without the musket volley): %1 - our crew, %2 - enemy crew. A separate line.
void NetClient_OnShipBoardResult(int iMsg)
{
	int iOurLost = NMGetWord(iMsg);
	int iEnemyLost = NMGetWord(iMsg);

	NetClient_LogMessageNum("Net_BoardCasualties", iOurLost, iEnemyLost);
}

void NetClient_RequestBoarding()
{
	if (NetClient_BoardTarget < 0) { return; }
	int iSMsg = NMCreate();
	NMAddClientID(iSMsg, wClientID);
	NMAddByte(iSMsg, NC_SHIP);
	NMAddByte(iSMsg, NSC_SHIP_BOARD_REQUEST);
	NMAddClientID(iSMsg, NetClient_BoardTarget);
	NetClient_SendMessage(iSMsg, true);
	NMDelete(iSMsg);
}


// =============================================================================================
// COOP: sea battle voices / music like in single player
// =============================================================================================
void NetClient_ResetSeaVoices()
{
	NetClient_LowHPWarned = false;
	NetClient_BortArmL = false;
	NetClient_BortArmR = false;
	NetClient_BortReadyL = false;
	NetClient_BortReadyR = false;
	NetClient_BortEventPosted = false;
	NetClient_PrevCrew = -1.0;
}

bool NetClient_IsThisShipDead()
{
	ref rMe = NetClient_GetThisClient();
	return (sti(rMe.Dead) != 0);
}

// "Load balls / grapes / knippels / bombs, aye captain" (hot keys 1-4; the command menu plays them by itself)
void NetClient_PlayChargeVoice(int iType)
{
	if (NetClient_IsThisShipDead()) { return; }

	switch (iType)
	{
		case 1: PlaySound("interface\_balls.wav"); break;
		case 2: PlaySound("interface\_grapes.wav"); break;
		case 3: PlaySound("interface\_chain.wav"); break;
		case 4: PlaySound("interface\_bombs.wav"); break;
	}
}

// "Left / right side is ready to fire" (single: Ship_OnBortReloaded + Ship_BortReloadEvent).
// Called on every borts info message of this client. A bort must be seen "not loaded" first, so there is no
// voice at the start of the game. When both sides get ready at once nothing is played (as in single).
void NetClient_CheckBortsReady(float fRatioL, float fRatioR)
{
	if (NetClient_IsThisShipDead()) { NetClient_BortArmL = false; NetClient_BortArmR = false; return; }

	if (fRatioL < 0.9) { NetClient_BortArmL = true; }
	if (fRatioR < 0.9) { NetClient_BortArmR = true; }

	bool bNewReady = false;
	if (NetClient_BortArmL && fRatioL >= 0.99) { NetClient_BortArmL = false; NetClient_BortReadyL = true; bNewReady = true; }
	if (NetClient_BortArmR && fRatioR >= 0.99) { NetClient_BortArmR = false; NetClient_BortReadyR = true; bNewReady = true; }

	if (bNewReady && !NetClient_BortEventPosted)
	{
		NetClient_BortEventPosted = true;
		PostEvent("NetClient_BortReloadEvent", 300);
	}
}

void NetClient_BortReloadEvent()
{
	NetClient_BortEventPosted = false;

	if (!NetClient_IsThisShipDead())
	{
		if (NetClient_BortReadyL && !NetClient_BortReadyR) { PlaySound("interface\_GunReadyL.wav"); }
		if (NetClient_BortReadyR && !NetClient_BortReadyL) { PlaySound("interface\_GunReadyR.wav"); }
	}

	NetClient_BortReadyL = false;
	NetClient_BortReadyR = false;
}

// "Water in the hold!" / "The ship is falling apart!" - once, when HP drops below NET_LOWHP_PERCENT
// (single: Interface\_Abandon1.wav or _Abandon2.wav at 10% HP, played again only after HP went back up)
void NetClient_CheckLowHP(float fHP, float fMaxHP)
{
	if (fMaxHP <= 0.0) { return; }

	float fPercent = 100.0 * fHP / fMaxHP;
	if (fHP > 0.0 && fPercent < NET_LOWHP_PERCENT)
	{
		if (!NetClient_LowHPWarned)
		{
			NetClient_LowHPWarned = true;
			if (rand(1) == 0) { PlaySound("interface\_Abandon1.wav"); }
			else { PlaySound("interface\_Abandon2.wav"); }
		}
	}
	else
	{
		NetClient_LowHPWarned = false;
	}
}

// single: sound when the crew falls below the minimum crew of the ship (OBJECTS\SHIPCHARGE\_Damage1.wav)
void NetClient_CheckMinCrew(float fCrew, ref rClient)
{
	ref rShip = Net_GetShipByIndex(sti(rClient.Ship.Type));
	float fMinCrew = stf(rShip.MinCrew);

	if (NetClient_PrevCrew >= fMinCrew && fCrew < fMinCrew && fCrew > 0.0)
	{
		PlaySound("OBJECTS\SHIPCHARGE\_Damage1.wav");
	}
	NetClient_PrevCrew = fCrew;
}
