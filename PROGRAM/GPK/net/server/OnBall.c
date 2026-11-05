object NSBalls;

int iServerBallIndex;
int iServerCurrentBallIndex;

void NetServer_DeleteBallsEnvironment()
{
	DelEventHandler("NetServer_OnCannonFire", "NetServer_OnCannonFire");
	DelEventHandler("NetServer_OnBallWaterHit", "NetServer_OnBallWaterHit");

	DeleteClass(&NSBalls);
}

float NetServer_GetSpeedModifier()
{
	return (stf(NetServer.GameSpeed) + 1.0) / 2.0;
}

void NetServer_CreateBallsEnvironment(int iSMsg)
{
	aref	Grapes, Knippels, Balls, Bombs;

	NSBalls.Server = true;
	CreateEntity(&NSBalls, "NetBalls");

	NSBalls.SpeedMultiply = 2.0 * NetServer_GetSpeedModifier();
	NSBalls.Texture = "AllBalls.tga";
	NSBalls.SubTexX = 2;
	NSBalls.SubTexY = 2;

	NSBalls.CurrentBallDistance = 0.0;

	makearef(Grapes, NSBalls.Balls.Grapes);
	makearef(Knippels, NSBalls.Balls.Knippels);
	makearef(Balls, NSBalls.Balls.Balls);
	makearef(Bombs, NSBalls.Balls.Bombs);

	Bombs.SubTexIndex = 0;		Bombs.Size = 0.3;		Bombs.GoodIndex = Net_GetGoodIndex("Bombs");		Bombs.Particle = "bomb_smoke";
	Grapes.SubTexIndex = 1;		Grapes.Size = 0.2;		Grapes.GoodIndex = Net_GetGoodIndex("Grapes");
	Balls.SubTexIndex = 2;		Balls.Size = 0.2;		Balls.GoodIndex = Net_GetGoodIndex("Balls");
	Knippels.SubTexIndex = 3;	Knippels.Size = 0.2;	Knippels.GoodIndex = Net_GetGoodIndex("Knippels");

	NSBalls.IsDone = true;

	LayerAddObject("net_execute", &NSBalls, -1);

	NMAddFloat(iSMsg, stf(NSBalls.SpeedMultiply));

	SetEventHandler("NetServer_OnCannonFire", "NetServer_OnCannonFire", 0);
	SetEventHandler("NetServer_OnBallWaterHit", "NetServer_OnBallWaterHit", 0);
}

void NetServer_OnCannonFire()
{
	aref arClient = GetEventData();
	//int wFireClientID = GetEventData();
	float x = GetEventData();
	float y = GetEventData();
	float z = GetEventData();
	float fSpeedV0 = GetEventData();
	float fDirAng = GetEventData();
	float fHeightAng = GetEventData();
	float fCannonDirAng = GetEventData();

	ref rBall = Net_GetGoodByIndex(sti(arClient.Ship.Cannons.Charge.Type));
	int iBallType = sti(rBall.Index);

	// COOP: thrown away / not bought cannons do not fire. The engine fires every cannon of the model, so only the
	// share of the shots that the ship really has goes out; the charge of a missing cannon goes back to the hold.
	if (!NetServer_CannonShotAllowed(arClient, fDirAng))
	{
		string sSkipBall = rBall.Name;
		arClient.Ship.Goods.(sSkipBall) = sti(arClient.Ship.Goods.(sSkipBall)) + 1;
		return;
	}

	EntityUpdate(0);

	NSBalls.x = x;
	NSBalls.y = y;
	NSBalls.z = z;
	NSBalls.iBallIndex = iServerBallIndex;
	NSBalls.ClientID = arClient.index;
	NSBalls.Type = rBall.Name;

	float fAccuracy = 1.2 - stf(arClient.Skills.Accuracy) / NETSKILL_MAX;
	float fK = Bring2Range(0.4, 1.2, 0.2, 1.2, fAccuracy);

	float fTempDispersionY = Degree2Radian(20.0);
	float fTempDispersionX = Degree2Radian(10.0);

	fDirAng = fDirAng + fK * fTempDispersionY * (frnd() - 0.5);
	fSpeedV0 = fSpeedV0;// + fAccuracy * (10.0 * fTempDispersionY) * (frnd() - 0.5);
	fHeightAng = fHeightAng + fAccuracy * (fTempDispersionX) * (frnd() - 0.5);

	NSBalls.Dir = fDirAng;
	NSBalls.SpdV0 = fSpeedV0;
	NSBalls.Ang = fHeightAng;

	EntityUpdate(1);
	NSBalls.ServerAdd = "";

	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_BALL);
	NMAddByte(iSMsg, NSC_BALL_ADD);
	NMAddWord(iSMsg, iServerBallIndex);
	NMAddDword(iSMsg, iServerTime);
	NMAddByte(iSMsg, sti(rBall.Index));
	NMAddFloat(iSMsg, x);
	NMAddFloat(iSMsg, y);
	NMAddFloat(iSMsg, z);
	NMAddFloat(iSMsg, fSpeedV0);
	NMAddFloat(iSMsg, fDirAng);
	NMAddFloat(iSMsg, fHeightAng);
	NMAddByte(iSMsg, sti(arClient.Ship.Cannons.Type));	// cannon type of the shooter: the client plays the shot sound of this caliber
	NetServer_SendMessage(DST_ALL, iSMsg, false);
	NMDelete(iSMsg);

	iServerBallIndex++;
}

// bort of a shot by its direction against the heading of the ship: 0 bow, 1 stern, 2 left, 3 right
int NetServer_ShotBort(aref arClient, float fDirAng)
{
	float fRel = fDirAng - stf(arClient.Ship.Ang.y);
	while (fRel > PI) { fRel = fRel - PIm2; }
	while (fRel < -PI) { fRel = fRel + PIm2; }
	float fAbs = fRel;
	if (fAbs < 0.0) { fAbs = -fAbs; }
	if (fAbs <= PI * 0.25) { return 0; }
	if (fAbs >= PI * 0.75) { return 1; }
	if (fRel > 0.0) { return 3; }
	return 2;
}

// share of the shots of a bort = cannons standing there / places of the bort (an accumulator spreads them over the broadside)
bool NetServer_CannonShotAllowed(aref arClient, float fDirAng)
{
	if (!CheckAttribute(arClient, "Ship.CannonsDumped")) { return true; }
	if (!CheckAttribute(arClient, "ID") || sti(arClient.ID) >= NET_MAXCLIENTS) { return true; }	// forts
	int iType = sti(arClient.Ship.Type);
	int iTotal = Net_GetShipCannonsNum(iType);
	if (iTotal <= 0) { return true; }
	if (sti(arClient.Ship.CannonsDumped) <= 0 && !CheckAttribute(arClient, "Ship.Bort")) { return true; }

	int iBort = NetServer_ShotBort(arClient, fDirAng);
	int iMax = Net_GetBortMax(iType, iBort);
	int iHave = 0;
	if (iMax > 0)
	{
		iHave = NetServer_BortHave(arClient, iBort);
	}
	else
	{
		iMax = iTotal;		// the model has a cannon where the ship type has no place: the share of the whole ship
		iHave = NetServer_ShipCannonsNum(arClient);
	}
	if (iHave >= iMax) { return true; }
	if (iHave <= 0) { return false; }

	string sB = Net_BortLetter(iBort);
	float fAcc = 0.0;
	if (CheckAttribute(arClient, "Ship.ShotAcc." + sB)) { fAcc = stf(arClient.Ship.ShotAcc.(sB)); }
	fAcc = fAcc + MakeFloat(iHave) / MakeFloat(iMax);
	if (fAcc >= 1.0)
	{
		arClient.Ship.ShotAcc.(sB) = fAcc - 1.0;
		return true;
	}
	arClient.Ship.ShotAcc.(sB) = fAcc;
	return false;
}

void NetServer_OnBallWaterHit()
{
	int wFireClientID = GetEventData();
	float x = GetEventData();
	float y = GetEventData();
	float z = GetEventData();

	ref rClient = NetServer_GetClient(wFireClientID);
	rClient.Stat.Misses = sti(rClient.Stat.Misses) + 1;

	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_BALL);
	NMAddByte(iSMsg, NSC_BALL_HIT_WATER);
	NMAddWord(iSMsg, iServerCurrentBallIndex);
	NMAddFloat(iSMsg, x);
	NMAddFloat(iSMsg, y);
	NMAddFloat(iSMsg, z);
	NetServer_SendMessage(DST_ALL, iSMsg, false);
	NMDelete(iSMsg);
}
