object NSBalls;

int iServerBallIndex;
int iServerCurrentBallIndex;
object NSFortBalls;		// COOP s27: the indexes of the last balls of the fort (see NSBalls.iBallIndex below)

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

	NSBalls.SpeedMultiply = NetServer_BallSpeedMultiply();		// COOP: the balls follow the game speed
	NetServer_AimLeadReset();
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

// ------------------------------------------------------------------------------------------------
// COOP: speed of the balls and aim assist
//
// The ships sail GameSpeed times faster, so the balls must fly faster in the same proportion, otherwise they
// fall behind a ship more and more (the admin changes the speed during a game: NetServer_BallsSpeedChanged).
// The aim assist works on every shot of a ship (players and bots): if an enemy ship stands near the direction
// of the shot, the direction and the elevation are calculated by the ballistics (flight time with the speed of
// the balls, the course and the speed of the target) and only a small part of the dispersion is left.
// ------------------------------------------------------------------------------------------------
object NSAim;

bool bNetAimAssist = true;
float fNetAimCone = 0.6;			// rad: an enemy ship nearer than this to the direction of the shot is aimed at
float fNetAimSpread = 0.15;			// part of the usual dispersion that is left on an aimed shot of a player
float fNetAimHitY = 4.0;			// height of the hit point above the water (the balls came down 10-20 m short of the ships with 2.0)
float fNetAimLeadK = 1.0;			// scale of the lead (target displacement during the flight), learned: see NetServer_AimLeadReset
int iNetAimLogTime = -100000;
float fNetAimCA = 0.0;				// correction of the aim of a bot along / across the line to the target (the spotting)
float fNetAimCX = 0.0;

float NetServer_BallSpeedMultiply()
{
	float fGS = stf(NetServer.GameSpeed);
	if (fGS < 1.0) { fGS = 1.0; }
	return 2.0 * fGS;
}

float NetServer_BallMult()
{
	float fM = 0.0;
	if (CheckAttribute(&NSBalls, "SpeedMultiply")) { fM = stf(NSBalls.SpeedMultiply); }
	if (fM < 0.1) { fM = 2.0; }
	return fM;
}

// The balls do not fly as fast as the multiply says when the game is fast: at game speed 4 the target moves about
// 2.2 times more during the flight than the model with SpeedMultiply gives (measured from the splashes in a log). So the
// lead is scaled by fNetAimLeadK: it starts from a guess for the game speed and every miss of an aimed ball (a splash
// at the target) corrects it (NetServer_AimSplash).
void NetServer_AimLeadReset()
{
	float fGS = stf(NetServer.GameSpeed);
	if (fGS < 1.0) { fGS = 1.0; }
	fNetAimLeadK = 1.0 + 0.4 * (fGS - 1.0);
	Trace("COOP: aim lead scale = " + fNetAimLeadK + " game speed = " + NetServer.GameSpeed);
}

// the admin changed the game speed: server balls and the balls of all clients
void NetServer_BallsSpeedChanged()
{
	if (!bServerGameStarted) { return; }
	NSBalls.SpeedMultiply = NetServer_BallSpeedMultiply();
	NetServer_AimLeadReset();

	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_BALL);
	NMAddByte(iSMsg, NSC_BALL_SPEED);
	NMAddWord(iSMsg, 0);
	NMAddFloat(iSMsg, stf(NSBalls.SpeedMultiply));
	NetServer_SendMessage(DST_ALL, iSMsg, true);
	NMDelete(iSMsg);
	Trace("COOP: balls speed multiply = " + NSBalls.SpeedMultiply + " game speed = " + NetServer.GameSpeed);
}

// horizontal distance of a ball that is shot with the elevation fTheta from the height fH above the hit point
float NetServer_AimRange(float fV0, float fTheta, float fH)
{
	float fVy = fV0 * sin(fTheta);
	float fDisc = fVy * fVy + 2.0 * 9.81 * fH;
	if (fDisc < 0.0) { fDisc = 0.0; }
	float fT = (fVy + sqrt(fDisc)) / 9.81;
	return fV0 * cos(fTheta) * fT;
}

// time of flight (physics, without the speed multiply of the balls)
float NetServer_AimFlight(float fV0, float fTheta, float fH)
{
	float fVy = fV0 * sin(fTheta);
	float fDisc = fVy * fVy + 2.0 * 9.81 * fH;
	if (fDisc < 0.0) { fDisc = 0.0; }
	return (fVy + sqrt(fDisc)) / 9.81;
}

// elevation for the distance fD; 1000 = too far
float NetServer_AimTheta(float fV0, float fD, float fH, float fAngMin, float fAngMax)
{
	float fLo = fAngMin;
	float fHi = fAngMax;
	float fMid = 0.0;
	if (NetServer_AimRange(fV0, fHi, fH) < fD) { return 1000.0; }
	if (NetServer_AimRange(fV0, fLo, fH) >= fD) { return fLo; }
	for (int k=0; k<18; k++)
	{
		fMid = 0.5 * (fLo + fHi);
		if (NetServer_AimRange(fV0, fMid, fH) < fD) { fLo = fMid; } else { fHi = fMid; }
	}
	return 0.5 * (fLo + fHi);
}

// ------------------------------------------------------------------------------------------------
// COOP: where the target will be. The velocity of a ship is measured from its positions (every >= 150 ms), the speed
// along the heading is mixed in; the yaw rate gives an arc instead of a straight line for a turning ship.
// ------------------------------------------------------------------------------------------------
void NetServer_AimTrack(int j)
{
	ref rJ = NetServer_GetClient(j);
	float fX = stf(rJ.Ship.Pos.x);
	float fZ = stf(rJ.Ship.Pos.z);
	float fAy = stf(rJ.Ship.Ang.y);
	float fDt = 0.0;
	float fNVX = 0.0;
	float fNVZ = 0.0;
	float fNW = 0.0;
	int iDt = 0;
	if (CheckAttribute(rJ, "AimTrack.t"))
	{
		iDt = iServerTime - sti(rJ.AimTrack.t);
		if (iDt < 150) { return; }
		rJ.AimTrack.ok = 0;
		if (iDt <= 1200)
		{
			fDt = 0.001 * iDt;
			fNVX = (fX - stf(rJ.AimTrack.x)) / fDt;
			fNVZ = (fZ - stf(rJ.AimTrack.z)) / fDt;
			fNW = NetServer_BotAngDiff(fAy, stf(rJ.AimTrack.ay)) / fDt;
			if (fNVX * fNVX + fNVZ * fNVZ < 360000.0 && fNW < 3.0 && fNW > -3.0)
			{
				if (CheckAttribute(rJ, "AimTrack.vx") && iDt < 700)
				{
					fNVX = 0.5 * fNVX + 0.5 * stf(rJ.AimTrack.vx);
					fNVZ = 0.5 * fNVZ + 0.5 * stf(rJ.AimTrack.vz);
					fNW = 0.5 * fNW + 0.5 * stf(rJ.AimTrack.w);
				}
				rJ.AimTrack.vx = fNVX;
				rJ.AimTrack.vz = fNVZ;
				rJ.AimTrack.w = fNW;
				rJ.AimTrack.ok = 1;
			}
		}
	}
	rJ.AimTrack.x = fX;
	rJ.AimTrack.z = fZ;
	rJ.AimTrack.ay = fAy;
	rJ.AimTrack.t = iServerTime;
}

// the place of the ship t after fTime seconds -> NSAim.PX, NSAim.PZ
void NetServer_AimPredict(int t, float fTime)
{
	ref rT = NetServer_GetClient(t);
	float fTX = stf(rT.Ship.Pos.x);
	float fTZ = stf(rT.Ship.Pos.z);
	float fTAy = stf(rT.Ship.Ang.y);
	float fSpd = 0.0;
	if (CheckAttribute(rT, "Ship.Speed.z")) { fSpd = stf(rT.Ship.Speed.z); }
	if (fSpd > 500.0 || fSpd < -500.0) { fSpd = 0.0; }
	float fVX = sin(fTAy) * fSpd;
	float fVZ = cos(fTAy) * fSpd;
	float fW = 0.0;
	if (CheckAttribute(rT, "AimTrack.ok") && sti(rT.AimTrack.ok) && iServerTime - sti(rT.AimTrack.t) < 700)
	{
		fVX = 0.5 * fVX + 0.5 * stf(rT.AimTrack.vx);
		fVZ = 0.5 * fVZ + 0.5 * stf(rT.AimTrack.vz);
		fW = stf(rT.AimTrack.w);
	}
	float fWt = fW * fTime * fNetAimLeadK;
	float fTk = fTime * fNetAimLeadK;
	if (fWt < 0.02 && fWt > -0.02)
	{
		NSAim.PX = fTX + fVX * fTk;
		NSAim.PZ = fTZ + fVZ * fTk;
	}
	else
	{
		NSAim.PX = fTX + (fVX * sin(fWt) + fVZ * (1.0 - cos(fWt))) / fW;
		NSAim.PZ = fTZ + (0.0 - fVX * (1.0 - cos(fWt)) + fVZ * sin(fWt)) / fW;
	}
}

// the part of the dispersion that is left on an aimed shot of the ship s; -1 = no aim assist (easy bots)
float NetServer_AimSpreadOf(int s)
{
	if (s == NET_FORTCLIENT) { return 0.0; }		// COOP: the fort of the hard difficulty aims like a hard bot
	if (NetServer_IsBot(s))
	{
		int iL = NetServer_BotSkillLevel(NetServer_GetClient(s));
		if (iL >= 3) { return 0.0; }
		if (iL == 2) { return 0.35; }
		return 0.0 - 1.0;
	}
	return fNetAimSpread;
}

// the aimed direction and elevation to the ship t -> NSAim.Dir, NSAim.Ang, NSAim.D
bool NetServer_AimSolve(float x, float y, float z, float fV0, int t, float fAngMin, float fAngMax)
{
	ref rT = NetServer_GetClient(t);
	float fMult = NetServer_BallMult();
	float fH = y - fNetAimHitY;
	float fPX = stf(rT.Ship.Pos.x);
	float fPZ = stf(rT.Ship.Pos.z);
	float fD = 0.0;
	float fTheta = 0.0;
	float fFlight = 0.0;
	float fLen = 0.0;
	float fUX = 0.0;
	float fUZ = 0.0;
	for (int k=0; k<3; k++)
	{
		fD = sqrt((fPX - x) * (fPX - x) + (fPZ - z) * (fPZ - z));
		fTheta = NetServer_AimTheta(fV0, fD, fH, fAngMin, fAngMax);
		if (fTheta > 100.0) { return false; }
		fFlight = NetServer_AimFlight(fV0, fTheta, fH) / fMult;
		NetServer_AimPredict(t, fFlight);
		fPX = stf(NSAim.PX);
		fPZ = stf(NSAim.PZ);
		fLen = sqrt((fPX - x) * (fPX - x) + (fPZ - z) * (fPZ - z));
		if (fLen > 1.0)
		{
			fUX = (fPX - x) / fLen;
			fUZ = (fPZ - z) / fLen;
			fPX = fPX + fNetAimCA * fUX + fNetAimCX * fUZ;
			fPZ = fPZ + fNetAimCA * fUZ - fNetAimCX * fUX;
		}
	}
	fD = sqrt((fPX - x) * (fPX - x) + (fPZ - z) * (fPZ - z));
	fTheta = NetServer_AimTheta(fV0, fD, fH, fAngMin, fAngMax);
	if (fTheta > 100.0) { return false; }

	NSAim.Dir = atan2(fPX - x, fPZ - z);
	NSAim.Ang = fTheta;
	NSAim.D = fD;
	NSAim.T = fFlight;
	return true;
}

// the enemy ship nearest to the direction of the shot, or DST_INVALID
int NetServer_AimTarget(int s, float x, float z, float fDir)
{
	int iBest = DST_INVALID;
	float fBestA = fNetAimCone;
	for (int j=0; j<NET_MAXCLIENTS; j++)
	{
		if (j == s) { continue; }
		if (!NetServer_ShipIsAlive(j)) { continue; }
		NetServer_AimTrack(j);
		float fDX = stf(NSClients[j].Ship.Pos.x) - x;
		float fDZ = stf(NSClients[j].Ship.Pos.z) - z;
		float fA = NetServer_BotAbs(NetServer_BotAngDiff(atan2(fDX, fDZ), fDir));
		if (fA >= fBestA) { continue; }
		if (fDX * fDX + fDZ * fDZ < 625.0) { continue; }
		if (!NetServer_ShouldTarget(s, j)) { continue; }
		fBestA = fA;
		iBest = j;
	}
	return iBest;
}

// aim assist of one shot -> NSAim.Dir / NSAim.Ang, false = shoot as the cannon aimed
bool NetServer_AimShot(aref arClient, float x, float y, float z, float fV0, float fDir)
{
	if (!bNetAimAssist) { return false; }
	int s = sti(arClient.index);
	if (s == NET_FORTCLIENT)
	{
		if (!NetServer_FortHard()) { return false; }		// COOP: the aim assist is for the fort of the hard difficulty only
	}
	else
	{
		if (s < 0 || s >= NET_MAXCLIENTS) { return false; }
	}
	if (fV0 < 1.0) { return false; }
	float fSpread = NetServer_AimSpreadOf(s);
	if (fSpread < 0.0) { return false; }
	arClient.AimSpread = fSpread;

	float fAngMin = -0.35;
	float fAngMax = 0.6;
	int iCannon = sti(arClient.Ship.Cannons.Type);
	if (iCannon >= 0 && iCannon < Net_GetCannonsNum())
	{
		ref rCannon = Net_GetCannonByIndex(iCannon);
		if (CheckAttribute(rCannon, "FireAngMax")) { fAngMax = stf(rCannon.FireAngMax); }
		if (CheckAttribute(rCannon, "FireAngMin")) { fAngMin = stf(rCannon.FireAngMin); }
	}

	int t = NetServer_AimTarget(s, x, z, fDir);
	if (t == DST_INVALID) { return false; }

	// spotting: the correction of the shooter for this target (from where its last balls fell)
	fNetAimCA = 0.0;
	fNetAimCX = 0.0;
	{
		if (!CheckAttribute(arClient, "AimCT") || sti(arClient.AimCT) != t || iServerTime - sti(arClient.AimCTime) > 25000)
		{
			arClient.AimCA = 0.0;
			arClient.AimCX = 0.0;
			arClient.AimCT = t;
		}
		arClient.AimCTime = iServerTime;
		fNetAimCA = stf(arClient.AimCA);
		fNetAimCX = stf(arClient.AimCX);
		arClient.AimCAs = fNetAimCA;
		arClient.AimCXs = fNetAimCX;
	}
	if (!NetServer_AimSolve(x, y, z, fV0, t, fAngMin, fAngMax)) { return false; }

	arClient.AimD = stf(NSAim.D);
	arClient.AimTgt = t;
	arClient.AimTime = iServerTime;
	arClient.AimDX = stf(NSAim.PX) - stf(NSClients[t].Ship.Pos.x);		// the lead of this shot (world)
	arClient.AimDZ = stf(NSAim.PZ) - stf(NSClients[t].Ship.Pos.z);
	arClient.AimLK = fNetAimLeadK;
	if (iServerTime - iNetAimLogTime > 1500)
	{
		iNetAimLogTime = iServerTime;
		Trace("COOP: aim shooter=" + s + " tgt=" + t + " D=" + makeint(stf(NSAim.D)) + " ang=" + stf(NSAim.Ang) + " flight=" + stf(NSAim.T) + " mult=" + NetServer_BallMult() + " v0=" + fV0);
	}
	return true;
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
	// COOP s27: the balls of the fort are remembered by their index: when such a ball touches the fort model (at the muzzle)
	// the client plays no hit effect (server\Fort.c: NetServer_OnFortHit). 64 slots, the newest index wins its slot.
	if (CheckAttribute(arClient, "ID"))
	{
		if (sti(arClient.ID) == NET_FORTCLIENT)
		{
			string sFortBallSlot = "s" + (iServerBallIndex - (iServerBallIndex / 64) * 64);
			NSFortBalls.(sFortBallSlot) = iServerBallIndex;
		}
	}
	NSBalls.ClientID = arClient.index;
	NSBalls.Type = rBall.Name;

	float fAccuracy = 1.2 - stf(arClient.Skills.Accuracy) / NETSKILL_MAX;
	float fK = Bring2Range(0.4, 1.2, 0.2, 1.2, fAccuracy);

	float fTempDispersionY = Degree2Radian(20.0);
	float fTempDispersionX = Degree2Radian(10.0);

	if (NetServer_AimShot(arClient, x, y, z, fSpeedV0, fDirAng))
	{
		// COOP: aimed shot, a small part of the dispersion is left
		fDirAng = stf(NSAim.Dir) + fK * fTempDispersionY * (frnd() - 0.5) * stf(arClient.AimSpread);
		fHeightAng = stf(NSAim.Ang) + fAccuracy * (fTempDispersionX) * (frnd() - 0.5) * stf(arClient.AimSpread);
	}
	else
	{
		fDirAng = fDirAng + fK * fTempDispersionY * (frnd() - 0.5);
		fHeightAng = fHeightAng + fAccuracy * (fTempDispersionX) * (frnd() - 0.5);
	}
	fSpeedV0 = fSpeedV0;// + fAccuracy * (10.0 * fTempDispersionY) * (frnd() - 0.5);

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
	// COOP: the fort of the harmless difficulty only stands: no shot leaves it
	if (CheckAttribute(arClient, "ID") && sti(arClient.ID) == NET_FORTCLIENT && NetServer_FortSilent()) { return false; }
	if (CheckAttribute(arClient, "ID") && sti(arClient.ID) == NET_FORTCLIENT) { iNetFortLastShot = iServerTime; }
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

// the splash of an aimed ball: the miss against the target (the target position now): along = beyond (+) / short (-)
// of the target seen from the shooter, across = to the right (+) / left (-) -> NSAim.MA, NSAim.MX, NSAim.MOK.
// A shooter corrects its next aims by the miss (a gunner's spotting): the correction goes to the shooter
// as AimCA / AimCX (in the frame shooter -> target) and is used by NetServer_AimSolve.
float NetServer_AimShooterX(int s, ref rClient)
{
	if (s == NET_FORTCLIENT) { return stf(NetServer.FortX); }
	return stf(rClient.Ship.Pos.x);
}

float NetServer_AimShooterZ(int s, ref rClient)
{
	if (s == NET_FORTCLIENT) { return stf(NetServer.FortZ); }
	return stf(rClient.Ship.Pos.z);
}

void NetServer_AimSplash(int s, ref rClient, float x, float z)
{
	NSAim.MOK = false;
	if (!CheckAttribute(rClient, "AimTime")) { return; }
	if (iServerTime - sti(rClient.AimTime) > 15000) { return; }
	int t = sti(rClient.AimTgt);
	if (t < 0 || t >= NET_MAXCLIENTS) { return; }
	ref rT = NetServer_GetClient(t);
	float fSX = NetServer_AimShooterX(s, rClient);
	float fSZ = NetServer_AimShooterZ(s, rClient);
	float fDX = stf(rT.Ship.Pos.x) - fSX;
	float fDZ = stf(rT.Ship.Pos.z) - fSZ;
	float fLen = sqrt(fDX * fDX + fDZ * fDZ);
	if (fLen < 1.0) { return; }
	float fUX = fDX / fLen;
	float fUZ = fDZ / fLen;
	float fMX = x - stf(rT.Ship.Pos.x);
	float fMZ = z - stf(rT.Ship.Pos.z);
	float fAlong = fMX * fUX + fMZ * fUZ;
	float fAcross = fMX * fUZ - fMZ * fUX;
	NSAim.MA = fAlong;
	NSAim.MX = fAcross;
	NSAim.MOK = true;

	// the scale of the lead: the miss along the lead of the shot (without the spotting correction of that shot)
	if (CheckAttribute(rClient, "AimDX") && iServerTime - sti(rClient.AimTime) < 3000)
	{
		float fDUX = stf(rClient.AimDX);
		float fDUZ = stf(rClient.AimDZ);
		float fDU = sqrt(fDUX * fDUX + fDUZ * fDUZ);
		float fEX = fMX - (stf(rClient.AimCAs) * fUX + stf(rClient.AimCXs) * fUZ);
		float fEZ = fMZ - (stf(rClient.AimCAs) * fUZ - stf(rClient.AimCXs) * fUX);
		if (fDU > 25.0 && fEX * fEX + fEZ * fEZ < 22500.0)
		{
			float fS = (fEX * fDUX + fEZ * fDUZ) / fDU;
			float fKNew = stf(rClient.AimLK) * (1.0 - fS / fDU);
			if (fKNew < 0.3) { fKNew = 0.3; }
			if (fKNew > 5.0) { fKNew = 5.0; }
			fNetAimLeadK = fNetAimLeadK + 0.25 * (fKNew - fNetAimLeadK);
			if (fNetAimLeadK < 0.5) { fNetAimLeadK = 0.5; }
			if (fNetAimLeadK > 4.0) { fNetAimLeadK = 4.0; }
		}
	}

	if (!CheckAttribute(rClient, "AimCT")) { return; }
	if (sti(rClient.AimCT) != t) { return; }
	if (iServerTime - sti(rClient.AimTime) > 3000) { return; }
	if (fAlong > 80.0 || fAlong < -80.0 || fAcross > 80.0 || fAcross < -80.0) { return; }
	float fCA = stf(rClient.AimCA);
	float fCX = stf(rClient.AimCX);
	// the correction that would have given no miss is (the one used for that shot - the miss)
	fCA = fCA + 0.3 * ((stf(rClient.AimCAs) - fAlong) - fCA);
	fCX = fCX + 0.3 * ((stf(rClient.AimCXs) - fAcross) - fCX);
	if (fCA > 25.0) { fCA = 25.0; }
	if (fCA < -25.0) { fCA = -25.0; }
	if (fCX > 25.0) { fCX = 25.0; }
	if (fCX < -25.0) { fCX = -25.0; }
	rClient.AimCA = fCA;
	rClient.AimCX = fCX;
}

string NetServer_AimMissText(ref rClient)
{
	if (!sti(NSAim.MOK)) { return ""; }
	string sCorr = "";
	if (CheckAttribute(rClient, "AimCA")) { sCorr = " corr=" + makeint(stf(rClient.AimCA)) + "/" + makeint(stf(rClient.AimCX)); }
	return " off along=" + makeint(stf(NSAim.MA)) + " across=" + makeint(stf(NSAim.MX)) + sCorr + " lead=" + fNetAimLeadK;
}

void NetServer_OnBallWaterHit()
{
	int wFireClientID = GetEventData();
	float x = GetEventData();
	float y = GetEventData();
	float z = GetEventData();

	ref rClient = NetServer_GetClient(wFireClientID);
	rClient.Stat.Misses = sti(rClient.Stat.Misses) + 1;

	// COOP: where an aimed ball fell (for tuning): the splash distance from the shooter against the aimed distance
	NetServer_AimSplash(wFireClientID, rClient, x, z);
	if (CheckAttribute(rClient, "AimTime") && iServerTime - sti(rClient.AimTime) < 15000 && iServerTime - iNetAimLogTime > 600)
	{
		iNetAimLogTime = iServerTime;
		Trace("COOP: aim splash shooter=" + wFireClientID + " tgt=" + rClient.AimTgt + " aimD=" + makeint(stf(rClient.AimD)) + " splashD=" + makeint(GetDistance2D(NetServer_AimShooterX(wFireClientID, rClient), NetServer_AimShooterZ(wFireClientID, rClient), x, z)) + " dt=" + (iServerTime - sti(rClient.AimTime)) + NetServer_AimMissText(rClient));
	}

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
