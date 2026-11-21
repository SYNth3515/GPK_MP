void NetClient_DeleteBallsEnvironment()
{
	DeleteClass(&NCBalls);

	DelEventHandler("NetClient_BallFlyNearCamera", "NetClient_BallFlyNearCamera");
}

void NetClient_CreateBallsEnvironment(int iMsg)
{
	aref	Grapes, Knippels, Balls, Bombs;

	NCBalls.Server = false;
	CreateEntity(&NCBalls, "NetBalls");

	NCBalls.SpeedMultiply = NMGetFloat(iMsg);
	NCBalls.Texture = "AllBalls.tga";
	NCBalls.SubTexX = 2;
	NCBalls.SubTexY = 2;

	NCBalls.BallFlySoundDistance = 5.0;
	NCBalls.BallFlySoundStereoMultiplyer = 1.0;

	makearef(Grapes, NCBalls.Balls.Grapes);
	makearef(Knippels, NCBalls.Balls.Knippels);
	makearef(Balls, NCBalls.Balls.Balls);
	makearef(Bombs, NCBalls.Balls.Bombs);

	Bombs.SubTexIndex = 0;		Bombs.Size = 0.3;		Bombs.GoodIndex = Net_GetGoodIndex("Bombs");		Bombs.Particle = "bomb_smoke";
	Grapes.SubTexIndex = 1;		Grapes.Size = 1.0;		Grapes.GoodIndex = Net_GetGoodIndex("Grapes");
	Balls.SubTexIndex = 2;		Balls.Size = 0.2;		Balls.GoodIndex = Net_GetGoodIndex("Balls");
	Knippels.SubTexIndex = 3;	Knippels.Size = 0.2;	Knippels.GoodIndex = Net_GetGoodIndex("Knippels");

	NCBalls.IsDone = true;

	LayerAddObject("net_execute", &NCBalls, -1);
	LayerAddObject("net_realize", &NCBalls, 65532);

	SetEventHandler("NetClient_BallFlyNearCamera", "NetClient_BallFlyNearCamera", 0);
}

void NetClient_BallFlyNearCamera()
{
	float x = GetEventData();
	float y = GetEventData();
	float z = GetEventData();

	Play3DSound("fly_ball", x, y, z);
}

void NetClient_OnBall(int iMsg)
{
	float x, y, z;
	int iSubCode = NMGetByte(iMsg);
	int iBallIndex = NMGetWord(iMsg);

	switch (iSubCode) 
	{
		case NSC_BALL_ADD:
			NetClient_OnBallAdd(iMsg, iBallIndex);
		break;
		case NSC_BALL_DEL:
			NCBalls.ClientDel = iBallIndex;
		break;
		case NSC_BALL_HIT_WATER:
			NetClient_OnBallHitWater(iMsg, iBallIndex);
		break;
		case NSC_BALL_HIT_SAIL:
			NetClient_OnBallHitSail(iMsg, iBallIndex);
		break;
		case NSC_BALL_HIT_HULL:
			NetClient_OnBallHitHull(iMsg, iBallIndex);
		break;
		case NSC_BALL_HIT_MAST:
			NetClient_OnBallHitMast(iMsg, iBallIndex);
		break;
		case NSC_BALL_HIT_FORT:
			NetClient_OnBallHitFort(iMsg, iBallIndex);
		break;
		case NSC_BALL_HIT_ISLAND:
			NetClient_OnBallHitIsland(iMsg, iBallIndex);
		break;
		case NSC_BALL_SPEED:
			NCBalls.SpeedMultiply = NMGetFloat(iMsg);		// COOP: the balls follow the game speed
		break;
	}
}

void NetClient_OnBallAdd(int iMsg, int iBallIndex)
{
	EntityUpdate(0);

	NCBalls.Time = NMGetDword(iMsg);
	int iBallType = NMGetByte(iMsg);
	float x = NMGetFloat(iMsg);
	float y = NMGetFloat(iMsg);
	float z = NMGetFloat(iMsg);
	NCBalls.x = x;
	NCBalls.y = y;
	NCBalls.z = z;

	NCBalls.SpdV0 = NMGetFloat(iMsg);
	NCBalls.Dir = NMGetFloat(iMsg);
	NCBalls.Ang = NMGetFloat(iMsg);
	int iCannonType = NMGetByte(iMsg);

	NCBalls.Index = iBallIndex;

	ref rBall = Net_GetGoodByIndex(iBallType);
	NCBalls.Type = rBall.Name;

	EntityUpdate(1);

	CreateParticleSystem("cancloud_fire", x, y, z, -stf(NCBalls.Ang), stf(NCBalls.Dir), 0.0, 5);
	// shot sound depends on the caliber / cannon type (cannon_fire_<type>_<caliber> from sound_alias.ini, like in single player)
	string sFireSound = "cannon_fire";
	if (iCannonType >= 0 && iCannonType < Net_GetCannonsNum())
	{
		ref rFireCannon = Net_GetCannonByIndex(iCannonType);
		if (CheckAttribute(rFireCannon, "Sound")) { sFireSound = rFireCannon.Sound; }
	}
	Play3DSound(sFireSound, x, y, z);
	// COOP: the cannon crew of the shooting ship goes to reload (client\Sailors.c)
	NetClient_SailorsBallFired(x, y, z, stf(NCBalls.Dir));

	NCBalls.ClientAdd = "";
}

void NetClient_OnBallHitWater(int iMsg, int iBallIndex)
{
	float x = NMGetFloat(iMsg); 
	float y = NMGetFloat(iMsg);	
	float z = NMGetFloat(iMsg);

	CreateParticleSystem("splash", x, y, z, 0.0, 0.0, 0.0, 5);	
	Play3DSound("ball_splash", x, y, z);

	NCBalls.ClientDel = iBallIndex;
}

void NetClient_OnBallHitSail(int iMsg, int iBallIndex)
{
	int wHitID = NMGetWord(iMsg);	// FIX-ME
	int bSailIdx = NMGetByte(iMsg);
	float x = NMGetFloat(iMsg); 
	float y = NMGetFloat(iMsg);	
	float z = NMGetFloat(iMsg);
	// SendMessage 2 sail
	Play3DSound("sail_damage", x, y, z);
}

void NetClient_OnBallHitIsland(int iMsg, int iBallIndex)
{
	NCBalls.ClientDel = iBallIndex;
}

// COOP: CreateBlast of single player (particles.c) puts the "blast" entity (flying barrels, planks and men) into the layers
// "realize" and "sea_realize", which are realized BEFORE "net_realize" - i.e. before the sky. The sky is drawn without the
// depth test, so it covered the debris wherever nothing else was behind it (the sky seen through the effect, the same as with
// the sailors). Here the entity goes to "net_realize", late: after the sky, the island and the ships.
int iCoopBlastPrio = 65535;
int NetClient_CreateBlast(float x, float y, float z)
{
	int iRes;
	object blast;
	iRes = CreateEntity(&blast, "blast");
	SendMessage(&blast, "lfff", LM_SETPOINT, x, y, z);
	LayerAddObject("net_realize", blast, iCoopBlastPrio);
	return iRes;
}

void NetClient_OnBallHitFort(int iMsg, int iBallIndex)
{
	float x = NMGetFloat(iMsg); 
	float y = NMGetFloat(iMsg);	
	float z = NMGetFloat(iMsg);

	// COOP: the server marks the balls the fort itself has fired (they hit the fort model right at the muzzle): no effect then
	int iSilent = NMGetByte(iMsg);
	if (iSilent != 0)
	{
		NCBalls.ClientDel = iBallIndex;
		return;
	}

	NetClient_CreateBlast(x, y, z);
	Play3DSound("ship_explosion", x, y, z);

	NCBalls.ClientDel = iBallIndex;
}

void NetClient_OnBallHitMast(int iMsg, int iBallIndex)
{
	int wOurClientID = NMGetClientID(iMsg);
	int wBallClientID = NMGetClientID(iMsg);
	float x = NMGetFloat(iMsg);
	float y = NMGetFloat(iMsg);
	float z = NMGetFloat(iMsg);
	int iDamageType = NMGetByte(iMsg);
	int iMastNum = NMGetByte(iMsg);
	
	NCBalls.ClientDel = iBallIndex;
}

// COOP s28: a volley of the fort (dozens of bombs, the hard fort has 15% critical hits) made several heavy explosions with
// flying barrels and men (CreateBlast) and a crew flight for every ball at once: the game froze. The heavy explosion is
// made at most once in iCoopBoomPause ms (the others are a light flash), the critical hit line in the chat as well.
int iCoopBoomLast = -100000;
int iCoopBoomPause = 1500;

void NetClient_OnBallHitHull(int iMsg, int iBallIndex)
{
	int wOurClientID = NMGetClientID(iMsg);
	int wBallClientID = NMGetClientID(iMsg);

	int iBallType = NMGetByte(iMsg);
	ref rGood = Net_GetGoodByIndex(iBallType);

	float x = NMGetFloat(iMsg); 
	float y = NMGetFloat(iMsg);	
	float z = NMGetFloat(iMsg);

	// COOP: sailors fly out of the hit place like in single player (client\Sailors.c)
	NetClient_SailorsHullHit(wOurClientID, x, y, z);

	switch (rGood.Name)
	{
		case "Balls":
			Play3DSound("ball2bort", x, y, z);
			CreateParticleSystem("blast", x, y, z, 0.0, 0.0, 0.0, 0);
			break;
		case "Grapes":
			Play3DSound("grapes2bort", x, y, z);
			CreateParticleSystem("blast", x, y, z, 0.0, 0.0, 0.0, 0);
			break;
		case "Knippels":
			Play3DSound("knippel2bort", x, y, z);
			CreateParticleSystem("blast", x, y, z, 0.0, 0.0, 0.0, 0);
			break;
		case "Bombs":
			Play3DSound("bomb2bort", x, y, z);
			CreateParticleSystem("blast", x, y, z, 0.0, 0.0, 0.0, 0);
			break;
	}

	bool bSeriousBoom = NMGetByte(iMsg);
	bool bFirePlaceActivate = NMGetByte(iMsg);

	if (bFirePlaceActivate)
	{
		int iFirePlaceIndex = NMGetWord(iMsg);
		int iStartFireTime = NMGetDword(iMsg);
		float fTotalFireTime = NMGetFloat(iMsg);

		PostEvent("NetClient_ShipActivateFirePlace", iStartFireTime - iClientTime, "llf", wOurClientID, iFirePlaceIndex, fTotalFireTime);
		if (wBallClientID == NET_FORTCLIENT)
		{
			NetClient_ChatAddGameMessage("Net_FireActivation", rgbGameMessage, 2, wBallClientID, wOurClientID);
		}
	}

	// if critical hit (COOP s28: the heavy explosion and its line not more often than every iCoopBoomPause ms)
	if (bSeriousBoom)
	{
		int iBoomNow = Net_GetTickCount();
		int iBoomAgo = iBoomNow - iCoopBoomLast;
		if (iBoomAgo < 0 || iBoomAgo >= iCoopBoomPause)
		{
			iCoopBoomLast = iBoomNow;
			NetClient_ChatAddGameMessage("Net_CriticalHit", rgbGameMessage, 2, wBallClientID, wOurClientID);		// fort can do it
			NetClient_ShipSeriousBoom(x, y, z);
		}
		else
		{
			CreateParticleSystem("blast_inv", x, y, z, 0.0, 0.0, 0.0, 0);
		}
	}

	NCBalls.ClientDel = iBallIndex;
}

