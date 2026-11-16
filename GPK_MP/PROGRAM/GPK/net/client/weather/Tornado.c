// ================================================================================================
// COOP: client part of the tornado of the weather "Tornado" (server\weather\Tornado.c)
//
// The tornado is the real class "Tornado" of Tornado.dll (the same model, clouds, debris and sound as in the single player game).
// The server moves it and does the damage, the dll of this build (modules\Tornado.dll, patched) takes the position from the
// script: the message "fffff" = life time, x, z, vx, vz of the pillar. Between the packets (5 a second) the dll moves the pillar
// with the speed vx, vz itself. If the packets stop for 3 seconds, or the server says "no tornado", the tornado fades out
// (life time 5 s) and is deleted.
// The original Tornado.dll does not read these numbers and the tornado would stay in the point (0, 0): the dll must be replaced.
// ================================================================================================
bool	bCliTornActive = false;		// the server has a tornado now
bool	bCliTornHandler = false;		// the frame handler is set
bool	bCliTornFading = false;			// the tornado fades out, the entity is deleted after fTornFadeTimer
float	fCliTornX = 0.0;
float	fCliTornZ = 0.0;
float	fCliTornVX = 0.0;
float	fCliTornVZ = 0.0;
float	fCliTornAge = 0.0;			// seconds since the last packet
float	fCliTornFadeTimer = 0.0;

void NetClient_TornadoDeleteEntity()
{
	if (isEntity(&Tornado)) { DeleteClass(&Tornado); }
	DeleteAttribute(&Tornado, "");
}

// life time, position and velocity of the pillar (the patched dll reads all five numbers)
void NetClient_TornadoSend(float fLife)
{
	if (!isEntity(&Tornado)) { return; }
	SendMessage(&Tornado, "fffff", fLife, fCliTornX, fCliTornZ, fCliTornVX, fCliTornVZ);
}

bool NetClient_TornadoCreateEntity()
{
	NetClient_TornadoDeleteEntity();

	if (!CreateEntity(&Tornado, "Tornado"))
	{
		Trace("COOP: cli tornado: CreateEntity failed");
		return false;
	}
	Tornado.Server = false;

	// the position is set before the entity is put to the layers: it is drawn at the right place from the first frame
	NetClient_TornadoSend(3600.0);
	LayerAddObject("net_execute", &Tornado, 65535);
	LayerAddObject("net_realize", &Tornado, 65535);
	Trace("COOP: cli tornado created at " + fCliTornX + ", " + fCliTornZ);
	return true;
}

void NetClient_TornadoSetHandler()
{
	if (bCliTornHandler) { return; }
	SetEventHandler("frame", "NetClient_TornadoFrame", 0);
	bCliTornHandler = true;
}

void NetClient_WhrDeleteTornadoEnvironment()
{
	if (bCliTornHandler)
	{
		DelEventHandler("frame", "NetClient_TornadoFrame");
		bCliTornHandler = false;
	}
	NetClient_TornadoDeleteEntity();
	bCliTornActive = false;
	bCliTornFading = false;
}

// called with the other weather parts: nothing to create, the tornado comes with the packets of the server (and stays
// when the admin changes the weather: the server says when it ends)
void NetClient_WhrCreateTornadoEnvironment()
{
}

// the tornado ends: the dll fades it out in about 10 seconds (life time 5 s, then the fade) and deletes it, here the entity is
// deleted after 14 seconds for sure
void NetClient_TornadoStop()
{
	if (!bCliTornActive) { return; }
	bCliTornActive = false;

	if (isEntity(&Tornado))
	{
		NetClient_TornadoSend(5.0);
		bCliTornFading = true;
		fCliTornFadeTimer = 14.0;
	}
}

void NetClient_TornadoOnMessage(int iMsg)
{
	int iActive = NMGetByte(iMsg);
	float x = NMGetFloat(iMsg);
	float z = NMGetFloat(iMsg);
	float vx = NMGetFloat(iMsg);
	float vz = NMGetFloat(iMsg);

	// no weather here (lobby of a running game)
	if (!bWeatherLoaded) { return; }

	if (iActive == 0)
	{
		NetClient_TornadoStop();
		return;
	}

	fCliTornX = x;
	fCliTornZ = z;
	fCliTornVX = vx;
	fCliTornVZ = vz;
	fCliTornAge = 0.0;

	// a new tornado (or the old one was fading or was lost): a fresh entity
	if (!bCliTornActive || bCliTornFading || !isEntity(&Tornado))
	{
		bCliTornFading = false;
		if (!NetClient_TornadoCreateEntity()) { return; }
		bCliTornActive = true;
	}
	else
	{
		NetClient_TornadoSend(3600.0);
	}

	NetClient_TornadoSetHandler();
}

void NetClient_TornadoFrame()
{
	float fDt = MakeFloat(GetDeltaTime()) * 0.001;
	if (fDt > 0.25) { fDt = 0.25; }

	if (bCliTornFading)
	{
		fCliTornFadeTimer = fCliTornFadeTimer - fDt;
		if (fCliTornFadeTimer <= 0.0)
		{
			NetClient_TornadoDeleteEntity();
			bCliTornFading = false;
		}
		return;
	}

	if (!bCliTornActive) { return; }

	// the packets stopped: the tornado fades out
	fCliTornAge = fCliTornAge + fDt;
	if (fCliTornAge > 3.0) { NetClient_TornadoStop(); }
}
