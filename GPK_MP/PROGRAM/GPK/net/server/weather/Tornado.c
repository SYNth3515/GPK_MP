// ================================================================================================
// COOP: server tornado of the weather "Tornado" (storm + tornado, Create game screen / admin menu)
//
// The class Tornado of Tornado.dll can not be used in a network game: its way is calculated inside the dll from its own
// random numbers, so every player would see it in another place. Here the server owns the tornado: it moves it, does the
// damage and sends the state to all clients (NSC_WEATHER_TORNADO), the clients only draw it (client\weather\Tornado.c).
//
// The damage is the damage of the single player game (AIShip.c, bTornado): inside 100 m of the tornado
//   hull  = k * (max hull / 100) * 8.5     sails = k * (1.5 / 4.5 / 8.5 percent for 0 / 1/2 / full sails)   once a second
//   k = (StormProfessional ? 0.7 : 1.0) * (1 - distance / 100)
// ================================================================================================
#define NET_TORN_DMG_K		2.0		// COOP s27: the damage of the tornado is this times the damage of the single player game (hull and sails)
#define NET_TORN_DMG_R		100.0	// COOP s27: radius of the damage (m), like the single player game
#define NET_TORN_SPIN		1.6		// COOP s27: Ship.Impulse.Rotate.y in the middle of the tornado (the hit of the fort turns a ship with 0.5)
#define NET_TORN_SPIN_MIN	0.5		// at the edge of the spin radius
#define NET_TORN_SPIN_R		130.0	// radius of the spin (m)
float	fCoopTornadoWind = 10.0;		// wind speed of the Tornado weather (the storm has 7.0)
bool	bCoopTornadoMode = false;		// the weather is the Tornado weather
bool	bSrvTornActive = false;			// a tornado is on the sea now
bool	bSrvTornHandler = false;		// the frame handler is set
float	fSrvTornX = 0.0;
float	fSrvTornZ = 0.0;
float	fSrvTornHeading = 0.0;			// radians, the direction is (sin, cos) like everywhere else
float	fSrvTornSpeed = 10.0;			// m/s
float	fSrvTornWander = 0.0;			// the tornado does not go straight to the nearest ship: it is turned by this angle
float	fSrvTornLife = 0.0;				// seconds to live
float	fSrvTornRespawn = 0.0;			// seconds to the next tornado
float	fSrvTornWanderTimer = 0.0;
float	fSrvTornDmgTimer = 0.0;
float	fSrvTornSendTimer = 0.0;
float	fSrvTornTX = 0.0;				// the nearest ship (NetServer_TornadoFindNearest)
float	fSrvTornTZ = 0.0;
float	fSrvTornNearest = 0.0;

// COOP s27: the ships near the tornado are spun round (Ship.Impulse.Rotate.y, the same impulse that the field of the fort uses).
// TornSpin = 1 marks a ship that the tornado spins; the impulse is taken away when it leaves the tornado (unless the fort pushes it).
void NetServer_TornadoSpinOff(ref rS)
{
	if (!CheckAttribute(rS, "TornSpin") || !sti(rS.TornSpin)) { return; }
	rS.TornSpin = 0;
	if (CheckAttribute(rS, "FortPush") && sti(rS.FortPush)) { return; }
	rS.Ship.Impulse.Rotate.y = 0.0;
}

void NetServer_TornadoSpinClear()
{
	for (int i=0; i<NET_MAXCLIENTS; i++)
	{
		ref rS = NetServer_GetClient(i);
		NetServer_TornadoSpinOff(rS);
	}
}

void NetServer_TornadoSpin()
{
	for (int i=0; i<NET_MAXCLIENTS; i++)
	{
		ref rS = NetServer_GetClient(i);
		if (!NetServer_ShipIsAlive(i)) { NetServer_TornadoSpinOff(rS); continue; }

		float fDist = GetDistance2D(fSrvTornX, fSrvTornZ, stf(rS.Ship.Pos.x), stf(rS.Ship.Pos.z));
		if (fDist >= NET_TORN_SPIN_R) { NetServer_TornadoSpinOff(rS); continue; }

		float fSpin = NET_TORN_SPIN_MIN + (NET_TORN_SPIN - NET_TORN_SPIN_MIN) * (1.0 - fDist / NET_TORN_SPIN_R);
		if (CheckAttribute(rS, "FortPush") && sti(rS.FortPush)) { continue; }		// the fort turns it now
		if (!CheckAttribute(rS, "TornSpin") || !sti(rS.TornSpin))
		{
			rS.Ship.Impulse.Rotate.x = 0.0;
			rS.Ship.Impulse.Rotate.z = 0.0;
			if (!CheckAttribute(rS, "Ship.Impulse.Speed.z")) { rS.Ship.Impulse.Speed.x = 0.0; rS.Ship.Impulse.Speed.y = 0.0; rS.Ship.Impulse.Speed.z = 0.0; }
			Trace("COOP: tornado spins ship " + i + " (distance " + fDist + ")");
		}
		rS.TornSpin = 1;
		rS.Ship.Impulse.Rotate.y = fSpin;
	}
}

void NetServer_TornadoReset()
{
	NetServer_TornadoSpinClear();
	bSrvTornActive = false;
	fSrvTornLife = 0.0;
	fSrvTornRespawn = 0.0;
	fSrvTornWanderTimer = 0.0;
	fSrvTornDmgTimer = 0.0;
	fSrvTornSendTimer = 0.0;
}

void NetServer_WhrCreateTornadoEnvironment()
{
	NetServer_TornadoReset();
	if (!bCoopTornadoMode) { return; }

	fSrvTornRespawn = 20.0;		// the first tornado comes 20 seconds after the start
	if (!bSrvTornHandler)
	{
		SetEventHandler("frame", "NetServer_TornadoFrame", 0);
		bSrvTornHandler = true;
	}
}

void NetServer_WhrDeleteTornadoEnvironment()
{
	if (bSrvTornHandler)
	{
		DelEventHandler("frame", "NetServer_TornadoFrame");
		bSrvTornHandler = false;
	}
	NetServer_TornadoReset();
}

// the admin switched the weather: the tornado appears / disappears (the clients get "no tornado" at once)
void NetServer_TornadoSetMode(bool bOn)
{
	if (bOn == bCoopTornadoMode && bOn == bSrvTornHandler) { return; }

	bool bWasActive = bSrvTornActive;
	NetServer_WhrDeleteTornadoEnvironment();
	bCoopTornadoMode = bOn;
	if (bWasActive) { NetServer_TornadoSendState(true); }		// the state is reset here: active = 0
	NetServer_WhrCreateTornadoEnvironment();
}

// active, x, z, vx, vz. The clients move the tornado with the speed between the packets
void NetServer_TornadoSendState(bool bReliable)
{
	int iActive = 0;
	if (bSrvTornActive) { iActive = 1; }

	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_WEATHER);
	NMAddByte(iSMsg, NSC_WEATHER_TORNADO);
	NMAddByte(iSMsg, iActive);
	NMAddFloat(iSMsg, fSrvTornX);
	NMAddFloat(iSMsg, fSrvTornZ);
	NMAddFloat(iSMsg, fSrvTornSpeed * sin(fSrvTornHeading));
	NMAddFloat(iSMsg, fSrvTornSpeed * cos(fSrvTornHeading));
	NetServer_SendMessage(DST_ALL, iSMsg, bReliable);
	NMDelete(iSMsg);
}

// the nearest ship of a player afloat (the bots are not hunted, but they are damaged) -> fSrvTornTX, fSrvTornTZ, fSrvTornNearest
bool NetServer_TornadoFindNearest()
{
	bool bFound = false;
	fSrvTornNearest = 100000.0;

	for (int i=0; i<NET_MAXCLIENTS; i++)
	{
		if (!sti(NSClients[i].Use)) { continue; }
		if (!NetServer_ShipIsAlive(i)) { continue; }

		float sx = stf(NSClients[i].Ship.Pos.x);
		float sz = stf(NSClients[i].Ship.Pos.z);
		float fD = GetDistance2D(fSrvTornX, fSrvTornZ, sx, sz);
		if (fD < fSrvTornNearest)
		{
			fSrvTornNearest = fD;
			fSrvTornTX = sx;
			fSrvTornTZ = sz;
			bFound = true;
		}
	}
	return bFound;
}

// a new tornado 500..900 m from the middle of the ships of the players (farther it is hidden by the fog of the storm), it goes to them
void NetServer_TornadoSpawn()
{
	float cx = 0.0;
	float cz = 0.0;
	int iNum = 0;

	for (int i=0; i<NET_MAXCLIENTS; i++)
	{
		if (!sti(NSClients[i].Use)) { continue; }
		if (!NetServer_ShipIsAlive(i)) { continue; }
		cx = cx + stf(NSClients[i].Ship.Pos.x);
		cz = cz + stf(NSClients[i].Ship.Pos.z);
		iNum++;
	}

	// nobody to hunt: wait
	if (iNum == 0) { fSrvTornRespawn = 5.0; return; }

	cx = cx / iNum;
	cz = cz / iNum;

	float fAngle = frnd() * PIm2;
	float fDist = 500.0 + frnd() * 400.0;

	fSrvTornX = cx + fDist * sin(fAngle);
	fSrvTornZ = cz + fDist * cos(fAngle);
	fSrvTornHeading = fAngle + PI + (frnd() - 0.5);		// (sin, cos) of fAngle + PI points to the middle
	fSrvTornSpeed = 9.0 + frnd() * 6.0;						// m/s: 9..15, the ships do 5..12 (the dll of the single player game: 3..9)
	fSrvTornWander = (frnd() - 0.5) * 2.0;
	fSrvTornWanderTimer = 6.0 + frnd() * 6.0;
	fSrvTornLife = 300.0 + frnd() * 600.0;				// like the single player game
	fSrvTornDmgTimer = 0.0;
	fSrvTornSendTimer = 0.0;
	bSrvTornActive = true;

	Trace("COOP: srv tornado spawn " + fSrvTornX + ", " + fSrvTornZ);
	NetServer_TornadoSendState(true);
}

void NetServer_TornadoEnd(float fPause)
{
	NetServer_TornadoSpinClear();
	bSrvTornActive = false;
	fSrvTornRespawn = fPause;
	NetServer_TornadoSendState(true);		// active = 0
}

// damage of the single player game, once a second
void NetServer_TornadoDamage()
{
	// every ship afloat, the bots too (they have Use = 0, like the parked ships)
	for (int i=0; i<NET_MAXCLIENTS; i++)
	{
		if (!NetServer_ShipIsAlive(i)) { continue; }

		ref rClient = NetServer_GetClient(i);
		float fDist = GetDistance2D(fSrvTornX, fSrvTornZ, stf(rClient.Ship.Pos.x), stf(rClient.Ship.Pos.z));
		if (fDist >= NET_TORN_DMG_R) { continue; }

		float fPerk = 1.0;
		if (NetServer_SeaPerk(i, NET_SPERK_STORM)) { fPerk = 0.7; }		// passive perk StormProfessional
		float fMul = NET_TORN_DMG_K * fPerk * Bring2Range(1.0, 0.0, 0.0, NET_TORN_DMG_R, fDist);

		// hull
		NetServer_ShipApplyHullHitpoints(rClient, fMul * (stf(rClient.Ship.MaxHP) / 100.0) * 8.5, NETKILL_BY_TORNADO, DST_INVALID);
		if (!NetServer_ShipIsAlive(i)) { continue; }

		// sails: percents of the sail points, more with the sails up
		int iSail = MakeInt(stf(rClient.Ship.SailState) * 2.0);
		float fSailK = 1.5;
		if (iSail == 1) { fSailK = 4.5; }
		if (iSail >= 2) { fSailK = 8.5; }

		float fSP = stf(rClient.Ship.SP) - fMul * fSailK;
		if (fSP < 0.0) { fSP = 0.0; }
		rClient.Ship.SP = fSP;
	}
}

void NetServer_TornadoFrame()
{
	if (!bServerGameStarted) { return; }

	float fDt = MakeFloat(GetDeltaTime()) * 0.001;
	if (fDt > 0.25) { fDt = 0.25; }

	// waiting for the next tornado
	if (!bSrvTornActive)
	{
		fSrvTornRespawn = fSrvTornRespawn - fDt;
		if (fSrvTornRespawn <= 0.0) { NetServer_TornadoSpawn(); }
		return;
	}

	// the end of its life
	fSrvTornLife = fSrvTornLife - fDt;
	if (fSrvTornLife <= 0.0) { NetServer_TornadoEnd(20.0 + frnd() * 40.0); return; }

	// steering: slowly to the nearest ship, turned by the wander angle; closer than 150 m it goes straight (passes the ship)
	if (NetServer_TornadoFindNearest())
	{
		// lost: nobody near, a new tornado will come to the ships
		if (fSrvTornNearest > 3000.0) { NetServer_TornadoEnd(3.0); return; }

		fSrvTornWanderTimer = fSrvTornWanderTimer - fDt;
		if (fSrvTornWanderTimer <= 0.0)
		{
			fSrvTornWanderTimer = 6.0 + frnd() * 6.0;
			fSrvTornWander = (frnd() - 0.5) * 2.0;
		}

		if (fSrvTornNearest > 150.0)
		{
			float dx = fSrvTornTX - fSrvTornX;
			float dz = fSrvTornTZ - fSrvTornZ;
			float rx = dx * cos(fSrvTornWander) + dz * sin(fSrvTornWander);
			float rz = dz * cos(fSrvTornWander) - dx * sin(fSrvTornWander);
			// the part of the wanted direction on the right of the heading (the direction is (sin h, cos h))
			float fSide = rx * cos(fSrvTornHeading) - rz * sin(fSrvTornHeading);
			if (fSide > 0.0) { fSrvTornHeading = fSrvTornHeading + 0.2 * fDt; }
			if (fSide < 0.0) { fSrvTornHeading = fSrvTornHeading - 0.2 * fDt; }
		}
	}
	else
	{
		// no ships afloat
		NetServer_TornadoEnd(5.0);
		return;
	}

	fSrvTornX = fSrvTornX + fSrvTornSpeed * sin(fSrvTornHeading) * fDt;
	fSrvTornZ = fSrvTornZ + fSrvTornSpeed * cos(fSrvTornHeading) * fDt;

	NetServer_TornadoSpin();		// COOP s27: every frame, the ships near the tornado turn round

	fSrvTornDmgTimer = fSrvTornDmgTimer + fDt;
	if (fSrvTornDmgTimer >= 1.0)
	{
		fSrvTornDmgTimer = fSrvTornDmgTimer - 1.0;
		NetServer_TornadoDamage();
	}

	// 5 packets a second, not reliable: the next one corrects a lost one
	fSrvTornSendTimer = fSrvTornSendTimer + fDt;
	if (fSrvTornSendTimer >= 0.2)
	{
		fSrvTornSendTimer = 0.0;
		NetServer_TornadoSendState(false);
	}
}
