// ================================================================================================
// COOP: crew walking on the decks + sailors flying out of a hit ship (single player: sea_ai\ShipWalk.c)
//
// Sailors.dll of single player is reused as is. It is driven by 4 script messages, here they are sent
// for the NetShip entities of the clients (NCClients[i]):
//   AI_MESSAGE_ADD_SHIP               "lis"    ship entity, ship name   -> crew appears on the deck
//   MSG_PEOPLES_ON_SHIP_HULLHIT       "lafff"  ship character, x, y, z  -> sailors fly out of the hit place
//   MSG_SHIP_DELETE                   "la"     ship character           -> crew of this ship is removed
//   Sailors.IsOnDeck                  attribute: 1 = deck camera, the running crew is hidden (like single player)
//
// NetShip (net.dll) is derived from the same SHIP_BASE as the SHIP of ship.dll (same virtual table), and
// NetShip::Init sets the ship character = the client object itself, so Sailors.dll accepts it.
// The layer is created by Sailors.dll itself ("sea_realize", realize only).
//
// iCoopFxSailors = 0 switches everything off (nothing is created, all functions return at once).
// ================================================================================================
int iCoopFxSailors = 1;

// true = the crew is on the deck always, also from the first person camera (for tests). false = like single player:
// the running crew of the own ship is hidden from the deck camera (unless bSeePeoplesOnDeck of sea_ai\AICameras.c is set)
bool bCoopSailorsAlwaysVisible = true;
int iCoopSailorsDeckApplied = -1;		// the last value written to Sailors.IsOnDeck, -1 = not written yet

// Sailors.dll adds itself to the hard-coded layer "sea_realize", which exists since the start of the game and is
// realized BEFORE "net_realize" - so the crew was drawn before the sky (priority 2 of NCSky), and the sky, drawn without
// the depth test, covered the men wherever nothing else was behind them (the horizon seen through the sailor).
// The entity is moved to "net_realize" with a late priority (65530 like in single player: after the ships, the rigging
// and the sky). bCoopSailorsNetLayer = false leaves it in "sea_realize" as it was.
bool bCoopSailorsNetLayer = true;
int iCoopSailorsLayerPrio = 65530;

// The number of the men on the deck = lowpolycrew of the ship type (its maximum, depends on the class) * the share of
// the crew left (Ship.Crew.Quantity / MaxCrew). It is checked every iCoopSailorsCrewPeriod ms; when it changes, the crew of
// the ship is put on the deck again, but not more often than every iCoopSailorsCrewPause ms.
int iCoopSailorsCrewPeriod = 2000;
int iCoopSailorsCrewPause = 6000;
int iCoopSailorsLoop = 0;

void NetClient_SailorsToNetLayer()
{
	if (!bCoopSailorsNetLayer) { return; }
	if (!isEntity(&Sailors)) { return; }
	LayerDelObject("sea_realize", &Sailors);
	LayerAddObject("net_realize", &Sailors, iCoopSailorsLayerPrio);
}

void NetClient_SailorsCreate()
{
	if (iCoopFxSailors == 0) { return; }
	if (isEntity(&Sailors)) { DeleteClass(&Sailors); }

	Trace("COOP: cli sailors create");
	SetEventHandler("NetClient_SailorsReAdd", "NetClient_SailorsReAdd", 0);
	SetEventHandler("NetClient_SailorsDelayedAdd", "NetClient_SailorsDelayedAdd", 0);
	SetEventHandler("NetClient_SailorsCrewCheck", "NetClient_SailorsCrewCheck", 0);
	SetEventHandler("frame", "NetClient_SailorsLayerKeep", 0);
	iCoopSailorsLoop = iCoopSailorsLoop + 1;
	PostEvent("NetClient_SailorsCrewCheck", iCoopSailorsCrewPeriod, "l", iCoopSailorsLoop);
	CreateEntity(&Sailors, "Sailors");
	// Sailors.dll puts itself into the hard-coded layer "sea_realize"; multiplayer only realizes "net_*" layers,
	// so the layer must be switched on explicitly, otherwise the crew is never drawn
	LayerSetRealize("sea_realize", 1);
	LayerFreeze("sea_realize", false);
	NetClient_SailorsToNetLayer();
	iCoopSailorsDeckApplied = -1;
	NetClient_SailorsDeckApply();
	Trace("COOP: sailors dbg create isEntity=" + isEntity(&Sailors) + " IsOnDeck=" + Sailors.IsOnDeck);
}

// Any interface (Esc menu, jettison window, ...) calls EngineLayersOffOn(true) in seadogs.c and freezes "sea_realize",
// the layer of Sailors.dll. On closing, EngineLayersOffOn(false) unfreezes the sea layers only if bSeaActive is set,
// which is never true in multiplayer (it unfreezes "realize"/"execute" instead), so the layer stayed frozen for good
// and the crew was gone. It is unfrozen every frame while Sailors exists; the game runs behind the menu anyway.
void NetClient_SailorsLayerKeep()
{
	if (!isEntity(&Sailors)) { return; }
	LayerFreeze("sea_realize", false);
	NetClient_SailorsDeckApply();
}

void NetClient_SailorsDelete()
{
	DelEventHandler("NetClient_SailorsReAdd", "NetClient_SailorsReAdd");
	DelEventHandler("NetClient_SailorsDelayedAdd", "NetClient_SailorsDelayedAdd");
	DelEventHandler("NetClient_SailorsCrewCheck", "NetClient_SailorsCrewCheck");
	DelEventHandler("frame", "NetClient_SailorsLayerKeep");
	iCoopSailorsLoop = iCoopSailorsLoop + 1;		// the events of the old loop are ignored
	if (!isEntity(&Sailors)) { return; }

	Trace("COOP: cli sailors delete");
	DeleteClass(&Sailors);
	for (int i=0; i<NET_MAXCLIENTS; i++)
	{
		DeleteAttribute(&NCClients[i], "CoopSailors");
		DeleteAttribute(&NCClients[i], "CoopSailorsCount");
		DeleteAttribute(&NCClients[i], "CoopSailorsTime");
		DeleteAttribute(&NCClients[i], "CoopSailorsGone");
	}
}

// like SeaCameras_Switch of single player: the running crew is not drawn from the deck camera.
// The wanted state is computed from the camera and written to Sailors.IsOnDeck every time it differs from the written
// one (so a respawn, a new ship or a camera that was switched without NetClient_SetDeckCameraOn are all covered).
void NetClient_SailorsDeckApply()
{
	if (!isEntity(&Sailors)) { return; }
	int iHide = 0;
	if (!bCoopSailorsAlwaysVisible && !bSeePeoplesOnDeck && sti(NCDeckCamera.On)) { iHide = 1; }
	if (iHide == iCoopSailorsDeckApplied) { return; }
	iCoopSailorsDeckApplied = iHide;
	// Sailors.dll applies the hide flag only when the attribute is written: 0 first, so that it really changes
	Sailors.IsOnDeck = 0;
	Sailors.IsOnDeck = iHide;
	Trace("COOP: sailors dbg IsOnDeck=" + iHide + " deckCamera=" + NCDeckCamera.On + " alwaysVisible=" + bCoopSailorsAlwaysVisible);
}

// called when the camera is switched (client\Cameras.c)
void NetClient_SailorsDeck(bool bOnDeck)
{
	NetClient_SailorsDeckApply();
}

bool NetClient_SailorsHasShip(int wID)
{
	if (wID < 0 || wID >= NET_MAXCLIENTS) { return false; }
	if (!CheckAttribute(&NCClients[wID], "CoopSailors")) { return false; }
	return sti(NCClients[wID].CoopSailors) != 0;
}

// the ship of the client is created (NetClient_LoadCreateShip) or got back to life (respawn mode)
void NetClient_SailorsAddShip(int wID)
{
	if (!isEntity(&Sailors)) { return; }
	if (wID < 0 || wID >= NET_MAXCLIENTS) { return; }
	if (NetClient_SailorsHasShip(wID)) { return; }
	if (!CheckAttribute(&NCClients[wID], "Ship.Type")) { return; }
	if (sti(NCClients[wID].Ship.Type) < 0) { return; }

	ref rShip = Net_GetShipByIndex(sti(NCClients[wID].Ship.Type));
	// single player passes RealShips[].name = base name + "1" (ShipsUtilites.c), the SAILORSPOINTS\<name>.ini files use it
	string sName = rShip.Name + "1";

	int iWant = NetClient_SailorsWantedCount(wID, rShip);
	if (iWant <= 0)
	{
		// no crew left: nobody on the deck, the check below puts the men back when the crew is hired
		NCClients[wID].CoopSailorsGone = 1;
		return;
	}
	DeleteAttribute(&NCClients[wID], "CoopSailorsGone");

	// Sailors.dll hides the running crew for IsOnDeck=1 only on the ship whose character has the attribute
	// "MainCharacter" (AttributeChanged), so the ship of this client has to be marked like the player in single player
	if (wID == NetClient_GetThisClientID()) { NCClients[wID].MainCharacter = 1; }
	NCClients[wID].CoopSailors = 1;
	Trace("COOP: cli sailors add ship id=" + wID + " name=" + sName + " men=" + iWant + " crew=" + NCClients[wID].Ship.Crew.Quantity);
	// Sailors.dll reads the number of the men once, from lowpolycrew of the ship type, inside this message:
	// the value of the type is replaced for the time of the message and put back at once
	bool bHadCrew = CheckAttribute(rShip, "lowpolycrew");
	int iTypeCrew = 0;
	if (bHadCrew) { iTypeCrew = sti(rShip.lowpolycrew); }
	rShip.lowpolycrew = iWant;
	SendMessage(&Sailors, "lis", AI_MESSAGE_ADD_SHIP, &NCClients[wID], sName);
	if (bHadCrew) { rShip.lowpolycrew = iTypeCrew; }
	else { DeleteAttribute(rShip, "lowpolycrew"); }
	NCClients[wID].CoopSailorsCount = iWant;
	NCClients[wID].CoopSailorsTime = iClientTime;
	// the hide flag is applied by Sailors.dll only when IsOnDeck is written, and a new ship starts visible:
	// write the camera state again
	iCoopSailorsDeckApplied = -1;
	NetClient_SailorsDeckApply();
}

// Single player adds the ship on SHIP_CREATE, when the ship is completely ready. In multiplayer the NetShip gets its
// model/position a bit later, so the crew is added with a delay (iCoopSailorsDelay ms).
int iCoopSailorsDelay = 1500;

void NetClient_SailorsAddShipLater(int wID)
{
	if (!isEntity(&Sailors)) { return; }
	PostEvent("NetClient_SailorsDelayedAdd", iCoopSailorsDelay, "l", wID);
}

void NetClient_SailorsDelayedAdd()
{
	int wID = GetEventData();
	if (!bNetClientInGame) { return; }
	if (wID < 0 || wID >= NET_MAXCLIENTS) { return; }
	if (CheckAttribute(&NCClients[wID], "Dead") && sti(NCClients[wID].Dead)) { return; }
	if (CheckAttribute(&NCClients[wID], "Wrecked") && sti(NCClients[wID].Wrecked)) { return; }
	Trace("COOP: sailors dbg delayed add id=" + wID + " isEntity(ship)=" + isEntity(&NCClients[wID]) + " isEntity(Sailors)=" + isEntity(&Sailors) + " IsOnDeck=" + Sailors.IsOnDeck);
	NetClient_SailorsAddShip(wID);
}

// men on the deck for the current crew: lowpolycrew of the type (maximum of the class) * crew share, at least 1 while
// somebody is alive, 0 when the crew is dead
int NetClient_SailorsWantedCount(int wID, ref rShip)
{
	int iBase = 5;		// the same default as in Sailors.dll
	if (CheckAttribute(rShip, "lowpolycrew")) { iBase = sti(rShip.lowpolycrew); }
	if (iBase <= 0) { return 0; }

	float fCrew = 0.0;
	if (CheckAttribute(&NCClients[wID], "Ship.Crew.Quantity")) { fCrew = stf(NCClients[wID].Ship.Crew.Quantity); }
	if (fCrew <= 0.0) { return 0; }

	float fMax = 0.0;
	if (CheckAttribute(rShip, "MaxCrew")) { fMax = stf(rShip.MaxCrew); }
	if (fMax <= 0.0) { return iBase; }

	float fShare = fCrew / fMax;
	if (fShare > 1.0) { fShare = 1.0; }
	int iMen = makeint(makefloat(iBase) * fShare + 0.5);
	if (iMen < 1) { iMen = 1; }
	return iMen;
}

// every iCoopSailorsCrewPeriod ms: the crew of the ship changed (battle losses, hiring) -> other number of men
void NetClient_SailorsCrewCheck()
{
	int iLoop = GetEventData();
	if (iLoop != iCoopSailorsLoop) { return; }
	if (!isEntity(&Sailors)) { return; }
	if (!bNetClientInGame) { return; }
	PostEvent("NetClient_SailorsCrewCheck", iCoopSailorsCrewPeriod, "l", iLoop);

	ref rShip;
	int iWant, iHave;
	bool bRebuild = false;
	for (int i=0; i<NET_MAXCLIENTS; i++)
	{
		if (!CheckAttribute(&NCClients[i], "Ship.Type")) { continue; }
		if (sti(NCClients[i].Ship.Type) < 0) { continue; }
		rShip = Net_GetShipByIndex(sti(NCClients[i].Ship.Type));
		iWant = NetClient_SailorsWantedCount(i, rShip);

		if (!NetClient_SailorsHasShip(i))
		{
			// the crew was dead at the moment of adding: the men come back with the hired crew
			if (CheckAttribute(&NCClients[i], "CoopSailorsGone") && iWant > 0)
			{
				if (CheckAttribute(&NCClients[i], "Dead") && sti(NCClients[i].Dead)) { continue; }
				if (CheckAttribute(&NCClients[i], "Wrecked") && sti(NCClients[i].Wrecked)) { continue; }
				DeleteAttribute(&NCClients[i], "CoopSailorsGone");
				NetClient_SailorsAddShip(i);
			}
			continue;
		}

		iHave = sti(NCClients[i].CoopSailorsCount);
		if (iWant == iHave) { continue; }
		if (iWant > 0 && CheckAttribute(&NCClients[i], "CoopSailorsTime"))
		{
			if (iClientTime - sti(NCClients[i].CoopSailorsTime) < iCoopSailorsCrewPause) { continue; }
		}

		Trace("COOP: cli sailors crew changed id=" + i + " men " + iHave + " -> " + iWant);
		NCClients[i].CoopSailors = 0;
		if (iWant <= 0) { NCClients[i].CoopSailorsGone = 1; }
		else { NCClients[i].CoopSailorsRedo = 1; }
		bRebuild = true;
	}
	if (bRebuild) { NetClient_SailorsRebuild(); }
}

// the ball hit the hull at x,y,z: sailors close to this place fly away (single player: Ship_Walk_Hull_Damage)
void NetClient_SailorsHullHit(int wID, float x, float y, float z)
{
	if (!isEntity(&Sailors)) { return; }
	if (!NetClient_SailorsHasShip(wID)) { return; }
	SendMessage(&Sailors, "lafff", MSG_PEOPLES_ON_SHIP_HULLHIT, &NCClients[wID], x, y, z);
}

// ------------------------------------------------------------------------------------------------
// The cannon crew: the client gets a shot (NetClient_OnBallAdd) without the shooter, so the ship is the nearest one
// to the muzzle and the bort is found like the server does it (NetServer_ShotBort): the shot direction against the
// course of the ship. One message per ship and bort for a whole broadside (single player: SHIP_BORT_FIRE).
// ------------------------------------------------------------------------------------------------
int iCoopSailorsReloadPause = 2500;		// ms, one broadside = many balls
float fCoopSailorsReloadRadius = 45.0;	// m from the center of the ship to the muzzle

void NetClient_SailorsBallFired(float x, float y, float z, float fDir)
{
	if (!isEntity(&Sailors)) { return; }

	int iBest = -1;
	float fBest = fCoopSailorsReloadRadius;
	for (int i=0; i<NET_MAXCLIENTS; i++)
	{
		if (!NetClient_SailorsHasShip(i)) { continue; }
		if (!CheckAttribute(&NCClients[i], "Ship.Pos.x") || !CheckAttribute(&NCClients[i], "Ship.Ang.y")) { continue; }
		float fDX = x - stf(NCClients[i].Ship.Pos.x);
		float fDZ = z - stf(NCClients[i].Ship.Pos.z);
		float fLen = sqrt(fDX * fDX + fDZ * fDZ);
		if (fLen < fBest) { fBest = fLen; iBest = i; }
	}
	if (iBest < 0) { return; }

	float fRel = fDir - stf(NCClients[iBest].Ship.Ang.y);
	while (fRel > PI) { fRel = fRel - PIm2; }
	while (fRel < -PI) { fRel = fRel + PIm2; }
	float fAbs = fRel;
	if (fAbs < 0.0) { fAbs = -fAbs; }

	string sBort = "cannonl";
	if (fAbs <= PI * 0.25) { sBort = "cannonf"; }
	else
	{
		if (fAbs >= PI * 0.75) { sBort = "cannonb"; }
		else
		{
			if (fRel > 0.0) { sBort = "cannonr"; }
		}
	}

	string sKey = "CoopSailorsReload_" + sBort;
	if (CheckAttribute(&NCClients[iBest], sKey))
	{
		if (iClientTime - sti(NCClients[iBest].(sKey)) < iCoopSailorsReloadPause) { return; }
	}
	NCClients[iBest].(sKey) = iClientTime;

	Trace("COOP: cli sailors reload id=" + iBest + " bort=" + sBort);
	SendMessage(&Sailors, "lis", AI_MESSAGE_CANNON_RELOAD, &NCClients[iBest], sBort);
}

// the ship is dead / wrecked: the whole crew goes down with it.
// MSG_SHIP_DELETE of Sailors.dll only marks the crew as deleted and leaves it to vanish together with the sinking
// ship (the men are removed one by one, mode 2); a NetShip of the client never sinks, so that crew stayed on the deck
// for good and the new crew of the respawn was put on it (twice as many men after every respawn). So the whole
// Sailors entity is rebuilt without this ship, the other ships get their crews back at once.
void NetClient_SailorsDeleteShip(int wID)
{
	if (!isEntity(&Sailors)) { return; }
	if (!NetClient_SailorsHasShip(wID)) { return; }
	NCClients[wID].CoopSailors = 0;
	Trace("COOP: cli sailors delete ship id=" + wID);
	NetClient_SailorsRebuild();
}

// Sailors is deleted and created again; the ships that had the crew (and the ones marked CoopSailorsRedo) get it again
void NetClient_SailorsRebuild()
{
	if (iCoopFxSailors == 0) { return; }
	if (!isEntity(&Sailors)) { return; }

	int i;
	for (i=0; i<NET_MAXCLIENTS; i++)
	{
		if (NetClient_SailorsHasShip(i) || CheckAttribute(&NCClients[i], "CoopSailorsRedo")) { NCClients[i].CoopSailorsKeep = 1; }
	}

	Trace("COOP: cli sailors rebuild");
	DeleteClass(&Sailors);
	for (i=0; i<NET_MAXCLIENTS; i++)
	{
		DeleteAttribute(&NCClients[i], "CoopSailors");
		DeleteAttribute(&NCClients[i], "CoopSailorsCount");
		DeleteAttribute(&NCClients[i], "CoopSailorsTime");
		DeleteAttribute(&NCClients[i], "CoopSailorsRedo");
	}

	CreateEntity(&Sailors, "Sailors");
	LayerSetRealize("sea_realize", 1);
	LayerFreeze("sea_realize", false);
	NetClient_SailorsToNetLayer();
	iCoopSailorsDeckApplied = -1;
	NetClient_SailorsDeckApply();

	for (i=0; i<NET_MAXCLIENTS; i++)
	{
		if (!CheckAttribute(&NCClients[i], "CoopSailorsKeep")) { continue; }
		DeleteAttribute(&NCClients[i], "CoopSailorsKeep");
		NetClient_SailorsAddShip(i);
	}
}

// respawn mode: the ship stays alive as an entity on the client, so the crew is put on it again with a delay
// (the old crew must be removed by Sailors.dll first)
void NetClient_SailorsRespawn(int wID)
{
	if (!isEntity(&Sailors)) { return; }
	PostEvent("NetClient_SailorsReAdd", 1500, "l", wID);
}

void NetClient_SailorsReAdd()
{
	int wID = GetEventData();
	if (!bNetClientInGame) { return; }
	if (wID < 0 || wID >= NET_MAXCLIENTS) { return; }
	if (!CheckAttribute(&NCClients[wID], "CoopReady") || !sti(NCClients[wID].CoopReady)) { return; }
	if (CheckAttribute(&NCClients[wID], "Dead") && sti(NCClients[wID].Dead)) { return; }
	if (CheckAttribute(&NCClients[wID], "Wrecked") && sti(NCClients[wID].Wrecked)) { return; }
	NetClient_SailorsAddShip(wID);
}
