// COOP s35: the NPC of the location of a land duel (the barman, the girls and the drunkards of a tavern, the citizens, the
// soldiers...) are made by the single player on every machine by itself: each player saw other people. The machine of the
// server (the host) is the master: it sends the list of its NPC (the model, the animation, the place, the blade) every 3 s and
// their places, angles and animations 7 times a second. The others take their own NPC out of the location and show copies
// of the NPC of the host (actors, immortal: the fight with them is the one of the host). No host in the game (a dedicated
// server): every machine keeps its own NPC as before.
//   NSC_LAND_NPCLIST  word gen, byte total, byte n, n x (byte k, string model, string anim, string sex, string blade,
//                     float height, float x, y, z, ay)
//   NSC_LAND_NPCSTATE word gen, byte n, n x (byte k, float x, y, z, ay, byte flags (1 fight, 2 dead, 4 angry with the
//                     players: COOP s46), byte health %, byte energy % (COOP s46, v114), string action)
// The server sends them on as *_R to the others.
// COOP s36: the places go only for the NPC that have moved (or changed the animation); all of them with the list.
// COOP s38: the places twice as often (75 ms); a copy goes from where it stands to the new place in the time between the
// packets (no jumps and stops). The fight: the blows of an NPC of the host on the puppet of a player go to that player
// (NSC_LAND_NPCHIT_R), the blows of a player on a copy go to the host (NSC_LAND_NPCDMG): the NPC takes the damage there and
// turns on the players.
//   NSC_LAND_NPCDMG    word gen, byte k, word damage, byte kind (0 blade, 1 shot)  (a player -> server -> the host)
//   NSC_LAND_NPCHIT_R  word damage, word damage if blocked, word view (1 the attacker saw a block, 4 a break blow), byte kind

#define NET_NPC_MAX			40		// NPC of one location at most
#define NET_NPC_LIST_MS		3000	// the list again (a player that loads the location late gets it)
#define NET_NPC_STATE_MS	75		// the places (COOP s38: 150 before)
#define NET_NPC_HIDE_MS		500		// the own NPC of a client go out of the location
#define NET_NPC_LIST_CHUNK	1		// COOP s36: a message of a client holds 128 bytes at most (net.dll Client.cpp asserts: the crash in Port Royal)
#define NET_NPC_STATE_CHUNK	2
#define NET_NPC_MAKE_WAIT_MS	300		// COOP s46: the copies wait after characters have gone (the engine frees their places later)
#define NET_NPC_FAIL_MS		3000	// COOP s46: a copy that the engine did not make is tried again after it
#define NET_NPC_GROUP		"NetLandNpc"		// the copies (a client)
#define NET_NPC_GROUP_ANGRY	"NetLandNpcA"		// COOP s46: the copies of the NPC that fight the players

int iNetNpcGen = 0;			// host: the number of the set of the NPC (a new load of the location makes a new set)
int iNetNpcListAt = 0;
int iNetNpcStateAt = 0;
int iNetNpcHideAt = 0;
int iNetNpcRelAt = 0;		// COOP s46: host: the relations of the angry NPC groups to the puppets
int iNetNpcMakeAt = 0;		// COOP s46: client: no copy is made before this time (NET_NPC_MAKE_WAIT_MS)
int iNetNpcCopyRelAt = 0;	// COOP s46: client: the relations of the groups of the copies
#define NET_NPC_REL_MS		500
object oNetNpcRel;			// COOP s46: host: <group> = 1 while that NPC group is an enemy of the puppets (set by us)
object oNetNpcH;			// host: k<N> = the index of the character, num
object oNetNpcs;			// client: gen, seen, n<k>: model, anim, sex, blade, h, x, y, z, ay, flags, act, chr

bool NetLand_NpcIsHost()
{
	if (Dedicated_IsOn()) { return false; }		// COOP s48: nobody plays on a dedicated server: every player keeps his own NPC
	return NetServer_IsStarted();
}

// a new duel (NetLand_OnStart)
void NetLand_NpcReset()
{
	NetLand_NpcDropAll();		// COOP s46: the copies of the last duel are not left with the name of the location
	DeleteAttribute(&oNetNpcH, "");
	oNetNpcH.num = 0;
	DeleteAttribute(&oNetNpcs, "");
	NetLand_NpcCalmAll();
	iNetNpcListAt = 0;
	iNetNpcStateAt = 0;
	iNetNpcHideAt = 0;
	iNetNpcRelAt = 0;
	iNetNpcMakeAt = 0;
	iNetNpcCopyRelAt = 0;
}

// COOP s46l: the NPC groups that we made enemies of the players calm down (the relations stay in the CharactersGroups
// entity: before, a new duel only forgot the list, and the soldiers angered in the last duel attacked the players at once)
void NetLand_NpcCalmAll()
{
	aref aR;
	makearef(aR, oNetNpcRel);
	int iN = GetAttributesNum(aR);
	for (int i=0; i<iN; i++)
	{
		aref aE = GetAttributeN(aR, i);
		string sGr = GetAttributeName(aE);
		LAi_group_SetRelation(sGr, NET_LAND_GROUP, LAI_GROUP_NEITRAL);
		LAi_group_SetRelation(sGr, NET_LAND_GROUP_ALLY, LAI_GROUP_NEITRAL);
	}
	if (iN > 0) { Trace("COOP: land NPC: " + iN + " angry groups calmed down for the new duel"); }
	DeleteAttribute(&oNetNpcRel, "");
}

// a character of the single player in our location (not a player, a bot, the doll or a copy of an NPC of the host)
bool NetLand_NpcCandidate(int iChr)
{
	if (iChr < 0 || iChr >= TOTAL_CHARACTERS) { return false; }
	if (iChr == GetMainCharacterIndex()) { return false; }
	ref ch = &Characters[iChr];
	if (!IsEntity(ch)) { return false; }
	if (CheckAttribute(ch, "NetLandSkin")) { return false; }
	if (CheckAttribute(ch, "NetLandNpcPup")) { return false; }
	if (!CheckAttribute(ch, "location")) { return false; }
	if (ch.location != sNetLandLoc) { return false; }
	if (!CheckAttribute(ch, "model")) { return false; }
	if (ch.model == "") { return false; }
	return true;
}

// the location has been loaded (NetLand_AfterLoad): the host makes a new set, a client puts the copies there again
void NetLand_NpcAfterLoad()
{
	iNetNpcListAt = 0;
	iNetNpcStateAt = 0;
	iNetNpcHideAt = 0;
	if (NetLand_NpcIsHost())
	{
		iNetNpcGen++;
		if (iNetNpcGen > 60000) { iNetNpcGen = 1; }
		DeleteAttribute(&oNetNpcH, "");
		oNetNpcH.num = 0;
		return;
	}
	// the copies of the old load are gone with it
	aref aL;
	makearef(aL, oNetNpcs);
	int iN = GetAttributesNum(aL);
	for (int i=0; i<iN; i++)
	{
		aref aE = GetAttributeN(aL, i);
		if (CheckAttribute(aE, "chr")) { aE.chr = -1; }
	}
	if (CheckAttribute(&oNetNpcs, "seen")) { NetLand_NpcHideOwn(); }
}

// ================================================================================================
// HOST
// ================================================================================================
void NetLand_NpcScan()
{
	int iNum = sti(oNetNpcH.num);
	// COOP s46: the slot of an NPC that has gone (the game takes the bodies away and makes new soldiers) is free for a new
	// one: before, the list only grew, the new soldiers past 40 were never sent and fought the players unseen
	int k;
	string sK;
	for (k=0; k<iNum; k++)
	{
		sK = "k" + k;
		if (!CheckAttribute(&oNetNpcH, sK)) { continue; }
		int iOld = sti(oNetNpcH.(sK));
		if (iOld < 0) { continue; }
		if (NetLand_NpcCandidate(iOld)) { continue; }
		DeleteAttribute(&oNetNpcH, "c" + iOld);
		oNetNpcH.(sK) = -1;
	}
	for (int i=0; i<LAi_numloginedcharacters; i++)
	{
		int iChr = LAi_loginedcharacters[i];
		if (!NetLand_NpcCandidate(iChr)) { continue; }
		string sHas = "c" + iChr;
		if (CheckAttribute(&oNetNpcH, sHas)) { continue; }
		// COOP s46: a body gets no place: the places go to the living ones
		if (LAi_IsDead(&Characters[iChr])) { continue; }
		int kFree = -1;
		for (k=0; k<iNum; k++)
		{
			sK = "k" + k;
			if (!CheckAttribute(&oNetNpcH, sK)) { kFree = k; break; }
			if (sti(oNetNpcH.(sK)) < 0) { kFree = k; break; }
		}
		if (kFree < 0 && iNum < NET_NPC_MAX)
		{
			kFree = iNum;
			iNum++;
		}
		// COOP s46: the list is full: the place of the oldest body goes to the new NPC (the game makes new soldiers while
		// the bodies lie: the list was full of them, the new ones fought the players unseen). The copy of the body goes
		// on the clients (the new model, or "alive" at the place of a dead one: NetLand_OnNpcList / NetLand_OnNpcState)
		if (kFree < 0) { kFree = NetLand_NpcOldestBody(iNum); }
		if (kFree < 0) { continue; }
		sK = "k" + kFree;
		if (CheckAttribute(&oNetNpcH, sK))
		{
			int iWas = sti(oNetNpcH.(sK));
			if (iWas >= 0)
			{
				DeleteAttribute(&oNetNpcH, "c" + iWas);
				Trace("COOP: land NPC: the place " + kFree + " of a body goes to a new NPC (" + Characters[iChr].model + ")");
			}
		}
		DeleteAttribute(&oNetNpcH, "d" + kFree);
		DeleteAttribute(&oNetNpcH, "s" + kFree);		// its state goes at once
		oNetNpcH.(sHas) = kFree;
		oNetNpcH.(sK) = iChr;
	}
	oNetNpcH.num = iNum;
}

// COOP s46: the place of the body that has lain longest (NetLand_NpcStateNow keeps the time of the death), -1: none
int NetLand_NpcOldestBody(int iNum)
{
	int kBest = -1;
	int iBestT = 0;
	int iNowB = Net_GetTickCount();
	for (int k=0; k<iNum; k++)
	{
		int iC = NetLand_NpcHostChr(k);
		if (iC < 0) { continue; }
		if (!LAi_IsDead(&Characters[iC])) { continue; }
		int iAgo = 0;
		string sD = "d" + k;
		if (CheckAttribute(&oNetNpcH, sD)) { iAgo = iNowB - sti(oNetNpcH.(sD)); }
		if (iAgo < 0) { iAgo = 0; }
		if (kBest < 0 || iAgo > iBestT) { kBest = k; iBestT = iAgo; }
	}
	return kBest;
}

int NetLand_NpcHostChr(int k)
{
	string sK = "k" + k;
	if (!CheckAttribute(&oNetNpcH, sK)) { return -1; }
	int iChr = sti(oNetNpcH.(sK));
	if (iChr < 0 || iChr >= TOTAL_CHARACTERS) { return -1; }
	if (!IsEntity(&Characters[iChr])) { return -1; }
	return iChr;
}

string NetLand_NpcBlade(ref ch)
{
	string sB = GetCharacterEquipByGroup(ch, BLADE_ITEM_TYPE);
	if (sB == "unarmed") { return ""; }
	return sB;
}

void NetLand_NpcSendList()
{
	NetLand_NpcScan();
	int iNum = sti(oNetNpcH.num);
	if (iNum <= 0) { return; }
	int k = 0;
	while (k < iNum)
	{
		int iEnd = k + NET_NPC_LIST_CHUNK;
		if (iEnd > iNum) { iEnd = iNum; }
		int iCnt = 0;
		int j;
		for (j=k; j<iEnd; j++) { if (NetLand_NpcHostChr(j) >= 0) { iCnt++; } }
		if (iCnt > 0)
		{
			int iSMsg = NetClient_LandNewMsg(NSC_LAND_NPCLIST);
			NMAddWord(iSMsg, iNetNpcGen);
			NMAddByte(iSMsg, iNum);
			NMAddByte(iSMsg, iCnt);
			for (j=k; j<iEnd; j++)
			{
				int iChr = NetLand_NpcHostChr(j);
				if (iChr < 0) { continue; }
				ref ch = &Characters[iChr];
				float fX = 0.0;
				float fY = 0.0;
				float fZ = 0.0;
				float fAy = 0.0;
				GetCharacterPos(ch, &fX, &fY, &fZ);
				GetCharacterAy(ch, &fAy);
				string sAni = "man";
				if (CheckAttribute(ch, "model.animation")) { sAni = ch.model.animation; }
				string sSex = "man";
				if (CheckAttribute(ch, "sex")) { sSex = ch.sex; }
				float fH = 1.8;
				if (CheckAttribute(ch, "model.height")) { fH = stf(ch.model.height); }
				NMAddByte(iSMsg, j);
				NMAddString(iSMsg, ch.model, 24);
				NMAddString(iSMsg, sAni, 16);
				NMAddString(iSMsg, sSex, 8);
				NMAddString(iSMsg, NetLand_NpcBlade(ch), 20);
				NMAddFloat(iSMsg, fH);
				NMAddFloat(iSMsg, fX);
				NMAddFloat(iSMsg, fY);
				NMAddFloat(iSMsg, fZ);
				NMAddFloat(iSMsg, fAy);
			}
			NetClient_LandSend(iSMsg, false);
			NMDelete(iSMsg);
		}
		k = iEnd;
	}
}

float NetLand_NpcFAbs(float f)
{
	if (f < 0.0) { return 0.0 - f; }
	return f;
}

// the state of the NPC k now; true when it is not the one sent last time (bAll: every one)
bool NetLand_NpcStateNow(int k, bool bAll)
{
	int iChr = NetLand_NpcHostChr(k);
	if (iChr < 0) { return false; }
	ref ch = &Characters[iChr];
	float fX = 0.0;
	float fY = 0.0;
	float fZ = 0.0;
	float fAy = 0.0;
	GetCharacterPos(ch, &fX, &fY, &fZ);
	GetCharacterAy(ch, &fAy);
	int iFl = 0;
	if (SendMessage(ch, "ls", MSG_CHARACTER_EX_MSG, "IsFightMode") != 0) { iFl = iFl + 1; }
	string sAct = "";
	int iHP = 0;
	int iEny = 0;
	string sDk = "d" + k;
	if (LAi_IsDead(ch))
	{
		iFl = iFl + 2;
		// COOP s46: the time of the death: the place of the oldest body goes to a new NPC when the list is full
		if (!CheckAttribute(&oNetNpcH, sDk)) { oNetNpcH.(sDk) = Net_GetTickCount(); }
	}
	else
	{
		sAct = NetLand_GetAnim(ch, 0);
		DeleteAttribute(&oNetNpcH, sDk);
		// COOP s46: angry with the players (NetLand_NpcRelations): the copy is an enemy of the hero of a client
		if (NetLand_NpcAngry(ch)) { iFl = iFl + 4; }
		// COOP s46: the health and the energy for the bars over the copy (percent; the energy by 5)
		iHP = makeint(LAi_GetCharacterRelHP(ch) * 100.0 + 0.5);
		iEny = makeint(LAi_GetCharacterRelEnergy(ch) * 20.0 + 0.5) * 5;
		if (iHP < 0) { iHP = 0; }
		if (iHP > 100) { iHP = 100; }
		if (iEny < 0) { iEny = 0; }
		if (iEny > 100) { iEny = 100; }
	}
	if (strlen(sAct) > 20) { sAct = ""; }
	string sS = "s" + k;
	bool bSend = bAll;
	if (!CheckAttribute(&oNetNpcH, sS + ".x")) { bSend = true; }
	else
	{
		float fD = NetLand_NpcFAbs(fX - stf(oNetNpcH.(sS).x)) + NetLand_NpcFAbs(fY - stf(oNetNpcH.(sS).y)) + NetLand_NpcFAbs(fZ - stf(oNetNpcH.(sS).z));
		if (fD > 0.02) { bSend = true; }
		if (NetLand_NpcFAbs(fAy - stf(oNetNpcH.(sS).ay)) > 0.03) { bSend = true; }
		if (sti(oNetNpcH.(sS).fl) != iFl) { bSend = true; }
		if (oNetNpcH.(sS).act != sAct) { bSend = true; }
		if (!CheckAttribute(&oNetNpcH, sS + ".hp")) { bSend = true; }
		else
		{
			if (sti(oNetNpcH.(sS).hp) != iHP) { bSend = true; }
			if (sti(oNetNpcH.(sS).eny) != iEny) { bSend = true; }
		}
	}
	if (CheckAttribute(&oNetNpcH, sS + ".fl"))
	{
		int iOldFl = sti(oNetNpcH.(sS).fl);
		int iOldF1 = iOldFl - (iOldFl / 2) * 2;
		int iNewF1 = iFl - (iFl / 2) * 2;
		if (iOldF1 != iNewF1) { Trace("COOP: land NPC " + k + " (" + ch.model + ") fight " + iNewF1 + ", action " + sAct); }
	}
	oNetNpcH.(sS).nx = fX;
	oNetNpcH.(sS).ny = fY;
	oNetNpcH.(sS).nz = fZ;
	oNetNpcH.(sS).nay = fAy;
	oNetNpcH.(sS).nfl = iFl;
	oNetNpcH.(sS).nact = sAct;
	oNetNpcH.(sS).nhp = iHP;
	oNetNpcH.(sS).neny = iEny;
	return bSend;
}

void NetLand_NpcAddState(int iSMsg, int k)
{
	string sS = "s" + k;
	aref aS;
	makearef(aS, oNetNpcH.(sS));
	aS.x = aS.nx;
	aS.y = aS.ny;
	aS.z = aS.nz;
	aS.ay = aS.nay;
	aS.fl = aS.nfl;
	aS.act = aS.nact;
	aS.hp = aS.nhp;
	aS.eny = aS.neny;
	NMAddByte(iSMsg, k);
	NMAddFloat(iSMsg, stf(aS.x));
	NMAddFloat(iSMsg, stf(aS.y));
	NMAddFloat(iSMsg, stf(aS.z));
	NMAddFloat(iSMsg, stf(aS.ay));
	NMAddByte(iSMsg, sti(aS.fl));
	NMAddByte(iSMsg, sti(aS.hp));		// COOP s46 (v114)
	NMAddByte(iSMsg, sti(aS.eny));
	NMAddString(iSMsg, aS.act, 20);
}

void NetLand_NpcSendStates(bool bAll)
{
	int iNum = sti(oNetNpcH.num);
	if (iNum <= 0) { return; }
	int iList[NET_NPC_MAX];
	int iCnt = 0;
	int j;
	for (j=0; j<iNum; j++)
	{
		if (iCnt >= NET_NPC_MAX) { break; }
		if (NetLand_NpcStateNow(j, bAll)) { iList[iCnt] = j; iCnt++; }
	}
	int k = 0;
	while (k < iCnt)
	{
		int iEnd = k + NET_NPC_STATE_CHUNK;
		if (iEnd > iCnt) { iEnd = iCnt; }
		int iSMsg = NetClient_LandNewMsg(NSC_LAND_NPCSTATE);
		NMAddWord(iSMsg, iNetNpcGen);
		NMAddByte(iSMsg, iEnd - k);
		for (j=k; j<iEnd; j++) { NetLand_NpcAddState(iSMsg, iList[j]); }
		NetClient_LandSend(iSMsg, false);
		NMDelete(iSMsg);
		k = iEnd;
	}
}

// ================================================================================================
// SERVER: on to the others
// ================================================================================================
void NetServer_OnLandNpc(int wID, int iSub, int iMsg)
{
	int iGen = NMGetWord(iMsg);
	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_LAND);
	int iCnt = 0;
	int i;
	if (iSub == NSC_LAND_NPCLIST)
	{
		int iTotal = NMGetByte(iMsg);
		iCnt = NMGetByte(iMsg);
		NMAddByte(iSMsg, NSC_LAND_NPCLIST_R);
		NMAddWord(iSMsg, iGen);
		NMAddByte(iSMsg, iTotal);
		NMAddByte(iSMsg, iCnt);
		for (i=0; i<iCnt; i++)
		{
			NMAddByte(iSMsg, NMGetByte(iMsg));
			NMAddString(iSMsg, NMGetString(iMsg), 24);
			NMAddString(iSMsg, NMGetString(iMsg), 16);
			NMAddString(iSMsg, NMGetString(iMsg), 8);
			NMAddString(iSMsg, NMGetString(iMsg), 20);
			NMAddFloat(iSMsg, NMGetFloat(iMsg));
			NMAddFloat(iSMsg, NMGetFloat(iMsg));
			NMAddFloat(iSMsg, NMGetFloat(iMsg));
			NMAddFloat(iSMsg, NMGetFloat(iMsg));
			NMAddFloat(iSMsg, NMGetFloat(iMsg));
		}
	}
	else
	{
		iCnt = NMGetByte(iMsg);
		NMAddByte(iSMsg, NSC_LAND_NPCSTATE_R);
		NMAddWord(iSMsg, iGen);
		NMAddByte(iSMsg, iCnt);
		for (i=0; i<iCnt; i++)
		{
			NMAddByte(iSMsg, NMGetByte(iMsg));
			NMAddFloat(iSMsg, NMGetFloat(iMsg));
			NMAddFloat(iSMsg, NMGetFloat(iMsg));
			NMAddFloat(iSMsg, NMGetFloat(iMsg));
			NMAddFloat(iSMsg, NMGetFloat(iMsg));
			NMAddByte(iSMsg, NMGetByte(iMsg));
			NMAddByte(iSMsg, NMGetByte(iMsg));		// COOP s46 (v114): the health
			NMAddByte(iSMsg, NMGetByte(iMsg));		// the energy
			NMAddString(iSMsg, NMGetString(iMsg), 20);
		}
	}
	NetServer_LandSendOthers(wID, iSMsg, false);
	NMDelete(iSMsg);
}

// ================================================================================================
// CLIENT
// ================================================================================================
// our own NPC go out of the location: the ones of the host are shown instead
void NetLand_NpcHideOwn()
{
	int iHidden = 0;
	int i = 0;
	while (i < LAi_numloginedcharacters)
	{
		int iChr = LAi_loginedcharacters[i];
		if (NetLand_NpcCandidate(iChr))
		{
			ChangeCharacterAddressGroup(&Characters[iChr], "none", "", "");
			// COOP s46: the character stays an entity of the location after it: the location holds 64 characters at most
			// ("Many characters in location"), the copies of the host past 31 were not made. The entity goes, the
			// character (its attributes) stays for the single player
			if (IsEntity(&Characters[iChr])) { DeleteCharacter(&Characters[iChr]); }
			iHidden++;
			// the list of the location is shorter now: the same place holds the next one
			if (i < LAi_numloginedcharacters)
			{
				if (LAi_loginedcharacters[i] != iChr) { continue; }
			}
		}
		i++;
	}
	if (iHidden > 0)
	{
		iNetNpcMakeAt = Net_GetTickCount() + NET_NPC_MAKE_WAIT_MS;		// COOP s46: their places are free after this frame
		Trace("COOP: land NPC: " + iHidden + " NPC of ours have gone, the ones of the host are shown");
	}
}

// COOP s46: a copy goes away for good: its entity (a place of the location) and its character (the place in Characters)
void NetLand_NpcDropCopy(int iChr)
{
	if (iChr < 0 || iChr >= TOTAL_CHARACTERS) { return; }
	ref ch = &Characters[iChr];
	if (!CheckAttribute(ch, "NetLandNpcPup")) { return; }
	if (IsEntity(ch)) { iNetNpcMakeAt = Net_GetTickCount() + NET_NPC_MAKE_WAIT_MS; }		// COOP s46
	ChangeCharacterAddressGroup(ch, "none", "", "");
	if (IsEntity(ch)) { DeleteCharacter(ch); }
	DeleteAttribute(ch, "");
	ch.id = "0";
	ch.index = iChr;
}

// a new set of the host: the copies of the old one go away
void NetLand_NpcNewGen(int iGen)
{
	aref aL;
	makearef(aL, oNetNpcs);
	int iN = GetAttributesNum(aL);
	for (int i=0; i<iN; i++)
	{
		aref aE = GetAttributeN(aL, i);
		if (!CheckAttribute(aE, "chr")) { continue; }
		NetLand_NpcDropCopy(sti(aE.chr));
	}
	DeleteAttribute(&oNetNpcs, "");
	oNetNpcs.gen = iGen;
	oNetNpcs.seen = 1;
	Trace("COOP: land NPC: the set " + iGen + " of the host");
}

int NetLand_NpcMake(int k)
{
	string sN = "n" + k;
	if (!CheckAttribute(&oNetNpcs, sN + ".model")) { return -1; }
	aref d;
	makearef(d, oNetNpcs.(sN));
	// COOP s46: the location holds 64 characters at most; the puppets of the players (32 now) and the bots come first
	if (LAi_numloginedcharacters >= NET_LAND_CHARS_MAX) { return -1; }
	// COOP s46: a character taken out of the location keeps its place there until the end of the frame (the engine deletes
	// it later): the copies made at once after the NPC of ours had gone found the location full ("Many characters in
	// location"). The copies wait a little (NetLand_NpcMakePending makes them)
	int iNowMk = Net_GetTickCount();
	int iWaitMk = iNetNpcMakeAt - iNowMk;
	if (iWaitMk > 0 && iWaitMk < 5000) { return -1; }
	if (CheckAttribute(d, "failAt"))
	{
		int iFailAgo = iNowMk - sti(d.failAt);
		if (iFailAgo >= 0 && iFailAgo < NET_NPC_FAIL_MS) { return -1; }
	}
	string sId = "NetLandN_" + k;
	// COOP s46: an old copy with this id goes first: the new one is a clean character (like the puppets of the players)
	int iOldC = GetCharacterIndex(sId);
	if (iOldC >= 0)
	{
		bool bOldEnt = IsEntity(&Characters[iOldC]);
		NetLand_NpcDropCopy(iOldC);
		if (bOldEnt) { return -1; }		// its place in the location is free after this frame
	}
	int iChr = NPC_GenerateCharacter(sId, d.model, d.sex, d.anim, 10, PIRATE, 0, false);
	if (iChr < 0) { return -1; }
	ref ch = &Characters[iChr];
	ch.NetLandNpcPup = 1;
	ch.NetLandNpcK = k;
	ch.model.height = stf(d.h);
	DeleteAttribute(ch, "model.animfile");
	DeleteAttribute(ch, "NetLandBotAnim");
	DeleteAttribute(ch, "NetLandNpcDead");
	ch.RebirthPhantom = true;
	ch.name = "";
	ch.lastname = "";
	LAi_SetImmortal(ch, true);
	LAi_CharacterDisableDialog(ch);
	LAi_SetActorType(ch);
	SendMessage(ch, "lsl", MSG_CHARACTER_EX_MSG, "SetFightWOWeapon", false);
	LAi_group_MoveCharacter(ch, NET_NPC_GROUP);
	NetLand_NpcCopyRel();
	string sBl = d.blade;
	if (sBl != "")
	{
		if (GetItemIndex(sBl) >= 0)
		{
			GiveItem2Character(ch, sBl);
			EquipCharacterByItem(ch, sBl);
		}
	}
	ChangeCharacterAddressGroup(ch, sNetLandLoc, sNetLandEntryGroup, sNetLandEntryLocator);
	if (!IsEntity(ch))
	{
		// COOP s46: the engine has not made it (the location is full): the place in Characters goes back, another try later
		NetLand_NpcDropCopy(iChr);
		d.chr = -1;
		d.failAt = iNowMk;
		Trace("COOP: land NPC copy " + k + " (" + d.model + ") is not made: the location is full, " + LAi_numloginedcharacters + " logined");
		return -1;
	}
	DeleteAttribute(d, "failAt");
	SendMessage(ch, "lsl", MSG_CHARACTER_EX_MSG, "SetFightWOWeapon", false);
	TeleportCharacterToPosAy(ch, stf(d.x), stf(d.y), stf(d.z), stf(d.ay));
	d.chr = iChr;
	return iChr;
}

// COOP s46: the relations of the groups of the copies to our hero (the game changes them by itself: set again now and then)
void NetLand_NpcCopyRel()
{
	LAi_group_SetRelation(NET_NPC_GROUP, LAI_GROUP_PLAYER, LAI_GROUP_NEITRAL);
	LAi_group_SetRelation(NET_NPC_GROUP_ANGRY, LAI_GROUP_PLAYER, LAI_GROUP_ENEMY);
}

// COOP s46: a client: the copies that are not there yet (the location was full, the old ones were going) are made here,
// not only at the packets of the host (a standing NPC sends no state for 3 s)
void NetLand_NpcMakePending(int iNow)
{
	aref aL;
	makearef(aL, oNetNpcs);
	int iN = GetAttributesNum(aL);
	for (int i=0; i<iN; i++)
	{
		aref aE = GetAttributeN(aL, i);
		if (!CheckAttribute(aE, "model")) { continue; }
		string sName = GetAttributeName(aE);
		if (strlen(sName) < 2) { continue; }
		if (strcut(sName, 0, 0) != "n") { continue; }
		int k = sti(strcut(sName, 1, strlen(sName) - 1));
		if (NetLand_NpcPupChr(k) >= 0) { continue; }
		if (CheckAttribute(aE, "lt"))
		{
			int iStale = sti(aE.lt) - iNow;
			if (iStale > 0) { continue; }		// gone on the host (NetLand_NpcDropStale) until it is listed again
		}
		NetLand_NpcMake(k);
	}
}

// COOP s46: every copy goes (a new duel, the hero loads the location again): the copies left with the name of the location
// were logged into the next load by the game (with the NPC of ours: "many logined characters in location")
void NetLand_NpcDropAll()
{
	int iDropped = 0;
	for (int k=0; k<NET_NPC_MAX; k++)
	{
		int iC = GetCharacterIndex("NetLandN_" + k);
		if (iC < 0) { continue; }
		NetLand_NpcDropCopy(iC);
		iDropped++;
		string sN = "n" + k;
		if (CheckAttribute(&oNetNpcs, sN + ".chr")) { oNetNpcs.(sN).chr = -1; }
	}
	if (iDropped > 0) { Trace("COOP: land NPC: " + iDropped + " copies have gone"); }
}

int NetLand_NpcPupChr(int k)
{
	string sN = "n" + k;
	if (!CheckAttribute(&oNetNpcs, sN + ".chr")) { return -1; }
	int iChr = sti(oNetNpcs.(sN).chr);
	if (iChr < 0) { return -1; }
	if (!IsEntity(&Characters[iChr])) { return -1; }
	return iChr;
}

void NetLand_OnNpcList(int iMsg)
{
	int iGen = NMGetWord(iMsg);
	int iTotal = NMGetByte(iMsg);
	int iCnt = NMGetByte(iMsg);
	bool bUse = true;
	if (!bNetLandActive || !bNetLandLocReady || NetLand_NpcIsHost()) { bUse = false; }
	if (bUse)
	{
		bool bNew = true;
		if (CheckAttribute(&oNetNpcs, "gen")) { if (sti(oNetNpcs.gen) == iGen) { bNew = false; } }
		if (bNew)
		{
			NetLand_NpcNewGen(iGen);
			NetLand_NpcHideOwn();
		}
	}
	for (int i=0; i<iCnt; i++)
	{
		int k = NMGetByte(iMsg);
		string sModel = NMGetString(iMsg);
		string sAni = NMGetString(iMsg);
		string sSex = NMGetString(iMsg);
		string sBlade = NMGetString(iMsg);
		float fH = NMGetFloat(iMsg);
		float fX = NMGetFloat(iMsg);
		float fY = NMGetFloat(iMsg);
		float fZ = NMGetFloat(iMsg);
		float fAy = NMGetFloat(iMsg);
		if (!bUse) { continue; }
		string sN = "n" + k;
		bool bHad = CheckAttribute(&oNetNpcs, sN + ".model");
		// COOP s46: the host gives the slot of a gone NPC to a new one: another model is another copy
		if (bHad)
		{
			if (oNetNpcs.(sN).model != sModel || oNetNpcs.(sN).anim != sAni)
			{
				int iOldM = NetLand_NpcPupChr(k);
				if (iOldM >= 0) { NetLand_NpcDropCopy(iOldM); }
				oNetNpcs.(sN).chr = -1;
			}
		}
		oNetNpcs.(sN).lt = Net_GetTickCount();
		oNetNpcs.(sN).model = sModel;
		oNetNpcs.(sN).anim = sAni;
		oNetNpcs.(sN).sex = sSex;
		oNetNpcs.(sN).blade = sBlade;
		oNetNpcs.(sN).h = fH;
		if (!bHad)
		{
			oNetNpcs.(sN).x = fX;
			oNetNpcs.(sN).y = fY;
			oNetNpcs.(sN).z = fZ;
			oNetNpcs.(sN).ay = fAy;
			oNetNpcs.(sN).chr = -1;
		}
		if (NetLand_NpcPupChr(k) < 0) { NetLand_NpcMake(k); }
	}
}

void NetLand_OnNpcState(int iMsg)
{
	int iGen = NMGetWord(iMsg);
	int iCnt = NMGetByte(iMsg);
	bool bUse = true;
	if (!bNetLandActive || !bNetLandLocReady || NetLand_NpcIsHost()) { bUse = false; }
	if (bUse)
	{
		if (!CheckAttribute(&oNetNpcs, "gen")) { bUse = false; }
		else { if (sti(oNetNpcs.gen) != iGen) { bUse = false; } }		// the list of this set comes first
	}
	for (int i=0; i<iCnt; i++)
	{
		int k = NMGetByte(iMsg);
		float fX = NMGetFloat(iMsg);
		float fY = NMGetFloat(iMsg);
		float fZ = NMGetFloat(iMsg);
		float fAy = NMGetFloat(iMsg);
		int iFl = NMGetByte(iMsg);
		int iHP = NMGetByte(iMsg);		// COOP s46 (v114)
		int iEny = NMGetByte(iMsg);
		string sAct = NMGetString(iMsg);
		if (!bUse) { continue; }
		string sN = "n" + k;
		if (!CheckAttribute(&oNetNpcs, sN + ".model")) { continue; }
		oNetNpcs.(sN).x = fX;
		oNetNpcs.(sN).y = fY;
		oNetNpcs.(sN).z = fZ;
		oNetNpcs.(sN).ay = fAy;
		int iChr = NetLand_NpcPupChr(k);
		int iDeadS = (iFl / 2) - (iFl / 4) * 2;
		if (iChr >= 0 && iDeadS == 0 && CheckAttribute(&Characters[iChr], "NetLandNpcDead"))
		{
			NetLand_NpcDropCopy(iChr);		// COOP s46: the NPC of the host lives again: a new copy
			oNetNpcs.(sN).chr = -1;
			iChr = -1;
		}
		if (iChr < 0) { iChr = NetLand_NpcMake(k); }
		if (iChr < 0) { continue; }
		// COOP s38: the way from the place of the copy now to the new one takes the time between the packets
		int iNowS = Net_GetTickCount();
		int iDur = NET_NPC_STATE_MS;
		if (CheckAttribute(&oNetNpcs, sN + ".t")) { iDur = iNowS - sti(oNetNpcs.(sN).t); }
		if (iDur < 40) { iDur = 40; }
		if (iDur > 300) { iDur = 300; }
		float fSX = fX;
		float fSY = fY;
		float fSZ = fZ;
		float fSAy = fAy;
		if (GetCharacterPos(&Characters[iChr], &fSX, &fSY, &fSZ)) { GetCharacterAy(&Characters[iChr], &fSAy); }
		oNetNpcs.(sN).sx = fSX;
		oNetNpcs.(sN).sz = fSZ;
		oNetNpcs.(sN).say = fSAy;
		oNetNpcs.(sN).t = iNowS;
		oNetNpcs.(sN).dur = iDur;
		NetLand_NpcApply(&Characters[iChr], iFl, sAct, iHP, iEny);
	}
}

// the fight mode, the death and the animation of the copy (like the puppet of a bot: NetLand_OnBotState)
void NetLand_NpcApply(ref ch, int iFl, string sAct, int iHP, int iEny)
{
	if (!IsEntity(ch)) { return; }
	// COOP s46: the bars over the copy show the health and the energy of the NPC of the host (LAi_fightparams.c)
	ch.NetLandNpcHP = makefloat(iHP) / 100.0;
	ch.NetLandNpcEny = makefloat(iEny) / 100.0;
	int iDead = (iFl / 2) - (iFl / 4) * 2;
	if (iDead == 1)
	{
		if (CheckAttribute(ch, "NetLandNpcDead")) { return; }
		ch.NetLandNpcDead = 1;
		CharacterPlayAction(ch, "");
		DeleteAttribute(ch, "NetLandBotAnim");
		SetCharacterTask_Dead(ch);
		return;
	}
	if (CheckAttribute(ch, "NetLandNpcDead")) { return; }
	// COOP s46: the NPC of the host is angry with the players: the copy is an enemy of our hero (the game aims the blows of
	// the hero at it and draws its bars); calm again, it is neutral
	int iAngry = (iFl / 4) - (iFl / 8) * 2;
	string sGrp = NET_NPC_GROUP;
	if (iAngry == 1) { sGrp = NET_NPC_GROUP_ANGRY; }
	string sGrpNow = "";
	if (CheckAttribute(ch, "chr_ai.group")) { sGrpNow = ch.chr_ai.group; }
	if (sGrpNow != sGrp)
	{
		LAi_group_MoveCharacter(ch, sGrp);
		NetLand_NpcCopyRel();
		Trace("COOP: land NPC copy " + ch.NetLandNpcK + " angry " + iAngry);
	}
	int iFight = iFl - (iFl / 2) * 2;
	// COOP s38: a soldier that fights the host stood still here: its blows need the fight mode of the copy (the host may not
	// tell it while the swing goes)
	if (NetLand_NpcFightAct(sAct)) { iFight = 1; }
	int iWasF = -1;
	if (CheckAttribute(ch, "NetLandNpcF")) { iWasF = sti(ch.NetLandNpcF); }
	if (iWasF != iFight)
	{
		ch.NetLandNpcF = iFight;
		Trace("COOP: land NPC copy " + ch.NetLandNpcK + " fight " + iFight + ", action " + sAct);
	}
	// COOP s46: the game changes the fight mode only while no action of the script plays (like the puppets of the bots,
	// NetLand_OnBotState): the blows played at once held the copy out of the fight ("game says 0" every packet), it swung
	// an empty hand. The actions wait for the blade 1.5 s at most
	int iEngF = 0;
	if (SendMessage(ch, "ls", MSG_CHARACTER_EX_MSG, "IsFightMode") != 0) { iEngF = 1; }
	if (iEngF != iFight)
	{
		int iNowW = Net_GetTickCount();
		if (!CheckAttribute(ch, "NetLandFWait")) { ch.NetLandFWait = iNowW; }
		int iWaitF = iNowW - sti(ch.NetLandFWait);
		bool bWaitF = false;
		if (iWaitF >= 0 && iWaitF < 1500) { bWaitF = true; }
		if (bWaitF && CheckAttribute(ch, "NetLandBotAnim")) { CharacterPlayAction(ch, ""); DeleteAttribute(ch, "NetLandBotAnim"); }
		NetLand_PuppetFight(ch, iFight);
		iEngF = 0;
		if (SendMessage(ch, "ls", MSG_CHARACTER_EX_MSG, "IsFightMode") != 0) { iEngF = 1; }
		if (iEngF == iFight) { DeleteAttribute(ch, "NetLandFWait"); }
		if (iEngF != iFight && bWaitF) { return; }		// the next packet tries again
	}
	else { DeleteAttribute(ch, "NetLandFWait"); }
	if (findSubStr(sAct, "death", 0) == 0) { sAct = ""; }
	string sCur = "";
	if (CheckAttribute(ch, "NetLandBotAnim")) { sCur = ch.NetLandBotAnim; }
	if (sAct != "" && sAct != sCur)
	{
		ch.NetLandBotAnim = sAct;
		CharacterPlayAction(ch, sAct);
	}
}

// the copies go to the places of the host smoothly (like NetLand_BotSmooth)
void NetLand_NpcSmooth()
{
	int iNowM = Net_GetTickCount();		// COOP s38
	aref aL;
	makearef(aL, oNetNpcs);
	int iN = GetAttributesNum(aL);
	for (int i=0; i<iN; i++)
	{
		aref aE = GetAttributeN(aL, i);
		if (!CheckAttribute(aE, "chr")) { continue; }
		int iChr = sti(aE.chr);
		if (iChr < 0) { continue; }
		ref ch = &Characters[iChr];
		if (!IsEntity(ch)) { continue; }
		if (CheckAttribute(ch, "NetLandNpcDead")) { continue; }
		float fX = 0.0;
		float fY = 0.0;
		float fZ = 0.0;
		float fAy = 0.0;
		if (!GetCharacterPos(ch, &fX, &fY, &fZ)) { continue; }
		GetCharacterAy(ch, &fAy);
		float fTX = stf(aE.x);
		float fTY = stf(aE.y);
		float fTZ = stf(aE.z);
		float fTAy = stf(aE.ay);
		float fD = sqrt((fTX - fX) * (fTX - fX) + (fTZ - fZ) * (fTZ - fZ));
		float fDAy = fTAy - fAy;
		while (fDAy > 3.14159265) { fDAy = fDAy - 6.2831853; }
		while (fDAy < -3.14159265) { fDAy = fDAy + 6.2831853; }
		if (fD < 0.02 && fDAy < 0.02 && fDAy > -0.02) { continue; }
		// COOP s38: along the way from the place at the packet to the new place, by the time (NetLand_OnNpcState)
		float fNX = fTX;
		float fNZ = fTZ;
		float fNAy = fTAy;
		if (fD <= 3.0 && CheckAttribute(aE, "t") && CheckAttribute(aE, "sx"))
		{
			float fA = makefloat(iNowM - sti(aE.t)) / makefloat(sti(aE.dur));
			if (fA < 0.0) { fA = 0.0; }
			if (fA > 1.0) { fA = 1.0; }
			float fSX = stf(aE.sx);
			float fSZ = stf(aE.sz);
			float fSAy = stf(aE.say);
			float fSDAy = fTAy - fSAy;
			while (fSDAy > 3.14159265) { fSDAy = fSDAy - 6.2831853; }
			while (fSDAy < -3.14159265) { fSDAy = fSDAy + 6.2831853; }
			fNX = fSX + (fTX - fSX) * fA;
			fNZ = fSZ + (fTZ - fSZ) * fA;
			fNAy = fSAy + fSDAy * fA;
		}
		if (TeleportCharacterToPosAy(ch, fNX, fTY, fNZ, fNAy)) { NetLand_FloorFix(ch, fTY, fNAy); }
	}
}

// COOP s46: a client: the host lists its NPC every 3 s; a slot not listed for 7 s has gone there (its NPC taken away)
void NetLand_NpcDropStale(int iNow)
{
	aref aL;
	makearef(aL, oNetNpcs);
	int iN = GetAttributesNum(aL);
	for (int i=0; i<iN; i++)
	{
		aref aE = GetAttributeN(aL, i);
		if (!CheckAttribute(aE, "lt")) { continue; }
		if (!CheckAttribute(aE, "chr")) { continue; }
		if (sti(aE.chr) < 0) { continue; }
		int iAgo = iNow - sti(aE.lt);
		if (iAgo >= 0 && iAgo < 7000) { continue; }
		Trace("COOP: land NPC copy " + GetAttributeName(aE) + " has gone on the host");
		NetLand_NpcDropCopy(sti(aE.chr));
		aE.chr = -1;
		aE.lt = iNow + 600000;		// not again until the host lists it (the list sets the time)
	}
}

// COOP s46: the host: an NPC that is an enemy of the hero of the host (the host has angered it: a blow, the alarm of the
// soldiers) is an enemy of every player: its group fights the groups of the puppets. Once the game calms it down for the
// host, it calms down for the players too (only the relations that we have set are turned back)
bool NetLand_NpcAngry(ref ch)
{
	ref mc = GetMainCharacter();
	if (!IsEntity(mc)) { return false; }
	if (LAi_IsDead(ch)) { return false; }
	return LAi_group_IsEnemy(ch, mc);
}

void NetLand_NpcRelations()
{
	if (!CheckAttribute(&oNetNpcH, "num")) { return; }
	object oG;
	int iNum = sti(oNetNpcH.num);
	for (int k=0; k<iNum; k++)
	{
		int iChr = NetLand_NpcHostChr(k);
		if (iChr < 0) { continue; }
		ref ch = &Characters[iChr];
		if (!CheckAttribute(ch, "chr_ai.group")) { continue; }
		string sG = ch.chr_ai.group;
		if (sG == "" || sG == LAI_GROUP_PLAYER || sG == NET_LAND_GROUP || sG == NET_LAND_GROUP_ALLY) { continue; }
		if (NetLand_NpcAngry(ch)) { oG.(sG) = 1; }
		else { if (!CheckAttribute(&oG, sG)) { oG.(sG) = 0; } }
	}
	aref aG;
	makearef(aG, oG);
	int iN = GetAttributesNum(aG);
	for (int i=0; i<iN; i++)
	{
		aref aE = GetAttributeN(aG, i);
		string sGr = GetAttributeName(aE);
		bool bSet = CheckAttribute(&oNetNpcRel, sGr);
		if (sti(GetAttributeValue(aE)) == 1)
		{
			if (bSet) { continue; }
			oNetNpcRel.(sGr) = 1;
			LAi_group_SetRelation(sGr, NET_LAND_GROUP, LAI_GROUP_ENEMY);
			LAi_group_SetRelation(sGr, NET_LAND_GROUP_ALLY, LAI_GROUP_ENEMY);
			Trace("COOP: land NPC group " + sGr + " is angry: it fights the players too");
		}
		else
		{
			if (!bSet) { continue; }
			DeleteAttribute(&oNetNpcRel, sGr);
			LAi_group_SetRelation(sGr, NET_LAND_GROUP, LAI_GROUP_NEITRAL);
			LAi_group_SetRelation(sGr, NET_LAND_GROUP_ALLY, LAI_GROUP_NEITRAL);
			Trace("COOP: land NPC group " + sGr + " has calmed down");
		}
	}
}

// every NET_LAND_SMOOTH ms (NetLand_Smooth)
void NetLand_NpcTick(int iNow)
{
	if (!bNetLandLocReady) { return; }
	if (!bNetClientConnected) { return; }
	int iAgo;
	if (NetLand_NpcIsHost())
	{
		iAgo = iNow - iNetNpcListAt;
		if (iNetNpcListAt == 0 || iAgo < 0 || iAgo >= NET_NPC_LIST_MS)
		{
			iNetNpcListAt = iNow;
			iNetNpcStateAt = iNow;
			NetLand_NpcSendList();
			NetLand_NpcSendStates(true);		// all of them now and then (a lost packet)
		}
		iAgo = iNow - iNetNpcStateAt;
		if (iAgo < 0 || iAgo >= NET_NPC_STATE_MS)
		{
			iNetNpcStateAt = iNow;
			NetLand_NpcSendStates(false);
		}
		iAgo = iNow - iNetNpcRelAt;
		if (iAgo < 0 || iAgo >= NET_NPC_REL_MS)
		{
			iNetNpcRelAt = iNow;
			NetLand_NpcRelations();
		}
		return;
	}
	if (!CheckAttribute(&oNetNpcs, "seen")) { return; }		// no host (a dedicated server): our own NPC stay
	iAgo = iNow - iNetNpcHideAt;
	if (iAgo < 0 || iAgo >= NET_NPC_HIDE_MS)
	{
		iNetNpcHideAt = iNow;
		NetLand_NpcHideOwn();
		NetLand_NpcDropStale(iNow);
	}
	// COOP s46: the game changes the relations of the groups by itself (a blow, a death): the ones of the copies again
	iAgo = iNow - iNetNpcCopyRelAt;
	if (iAgo < 0 || iAgo >= 2000)
	{
		iNetNpcCopyRelAt = iNow;
		NetLand_NpcCopyRel();
	}
	NetLand_NpcMakePending(iNow);		// COOP s46
	NetLand_NpcSmooth();
}

// ================================================================================================
// COOP s38: THE FIGHT WITH THE NPC
// ================================================================================================
// an action of the fight (the copy has to be in the fight mode to play it)
bool NetLand_NpcFightAct(string sAct)
{
	if (sAct == "") { return false; }
	if (findSubStr(sAct, "attack", 0) == 0) { return true; }
	if (findSubStr(sAct, "fight", 0) == 0) { return true; }
	if (findSubStr(sAct, "hit_", 0) == 0) { return true; }
	if (findSubStr(sAct, "block", 0) == 0) { return true; }
	if (findSubStr(sAct, "parry", 0) == 0) { return true; }
	if (findSubStr(sAct, "recoil", 0) == 0) { return true; }
	if (findSubStr(sAct, "Shot", 0) == 0) { return true; }
	return false;
}

// the host: a character of the single player here (a soldier, a citizen...), not a player, a bot or a copy
bool NetLand_NpcIsLocalNpc(aref chr)
{
	if (!NetLand_NpcIsHost()) { return false; }
	if (!CheckAttribute(chr, "index")) { return false; }
	return NetLand_NpcCandidate(sti(chr.index));
}

// COOP s46: a character of the single player of our own machine (the host, or anybody when no host is in the game)
bool NetLand_NpcIsOwnNpc(aref chr)
{
	if (!CheckAttribute(chr, "index")) { return false; }
	return NetLand_NpcCandidate(sti(chr.index));
}

// the host: an NPC has hit the puppet of the player wVictim (iKind 0 a blade, 1 a shot)
void NetLand_NpcHitPlayer(aref attack, aref enemy, int wVictim, float fDmg, float fDmgB, bool bBlocked, bool bBreak, int iKind)
{
	if (wVictim < 0 || wVictim >= NET_MAXPLAYERS) { return; }
	if (CheckAttribute(enemy, "NetLandDead")) { if (sti(enemy.NetLandDead)) { return; } }
	if (!sti(NSClients[wVictim].Use) || !sti(NSClients[wVictim].InGame)) { return; }
	if (CheckAttribute(&NSClients[wVictim], "LandDead")) { if (sti(NSClients[wVictim].LandDead)) { return; } }
	int iD = makeint(fDmg + 0.5);
	int iDB = makeint(fDmgB + 0.5);
	if (iD <= 0 && iDB <= 0) { return; }
	if (iDB > iD) { iDB = iD; }
	int iView = 0;
	if (bBlocked) { iView = 1; }
	if (bBreak) { iView = iView + 4; }
	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_LAND);
	NMAddByte(iSMsg, NSC_LAND_NPCHIT_R);
	NMAddWord(iSMsg, iD);
	NMAddWord(iSMsg, iDB);
	NMAddWord(iSMsg, iView);
	NMAddByte(iSMsg, iKind);
	NetServer_SendMessage(wVictim, iSMsg, true);
	NMDelete(iSMsg);
	Trace("COOP: land NPC " + attack.model + " hits " + wVictim + ": " + iD + "/" + iDB + " kind " + iKind);
}

// a player: an NPC of the host has hit us
void NetLand_OnNpcHit(int iMsg)
{
	int iDmg = NMGetWord(iMsg);
	int iDmgB = NMGetWord(iMsg);
	int iView = NMGetWord(iMsg);
	int iKind = NMGetByte(iMsg);
	if (!bNetLandActive || bNetLandDead || !bNetLandLocReady) { return; }
	if (Net_GetTickCount() < iNetLandProtectUntil) { return; }
	ref mc = GetMainCharacter();
	int iOwn = 0;
	if (iKind == 0) { iOwn = NetLand_HeroDefence(); }
	if (iOwn == 2 || iOwn == 3) { return; }		// a dodge, a feint
	bool bBreak = false;
	int iV4 = iView / 4;
	if (iV4 - (iV4 / 2) * 2 == 1) { bBreak = true; }
	bool bBlk = false;
	if (iKind == 0 && !bBreak)
	{
		if (iOwn == 1) { bBlk = true; }
		if (iView - (iView / 2) * 2 == 1) { bBlk = true; }
	}
	if (bBlk)
	{
		LAi_CharacterPlaySound(mc, "fgt_collision");
		NetLand_HeroFightReact(14, 0, "block_hit", NET_LAND_T_BLOCKHIT);
		if (iDmgB > 0)
		{
			float fHPB = LAi_GetCharacterHP(mc) - makefloat(iDmgB);
			if (fHPB < 0.0) { fHPB = 0.0; }
			LAi_SetHP(mc, fHPB, LAi_GetCharacterMaxHP(mc));
		}
	}
	else
	{
		if (iDmg <= 0) { return; }
		LAi_ApplyCharacterDamage(mc, iDmg);
		if (LAi_GetCharacterHP(mc) >= 1.0)
		{
			if (iKind == 1) { NetLand_HeroFightReact(12, 0, "hit_fire", NET_LAND_T_HIT); }
			else
			{
				int iHV = rand(2);
				NetLand_HeroFightReact(8, iHV, "hit_attack_" + (iHV + 1), NET_LAND_T_HIT);
			}
		}
	}
	Trace("COOP: land an NPC hits us: " + iDmg + "/" + iDmgB + " kind " + iKind + " blocked " + bBlk + " hp " + LAi_GetCharacterHP(mc));
	if (LAi_GetCharacterHP(mc) < 1.0) { NetLand_LocalDie(DST_INVALID); }
}

// COOP s46: the host: an NPC of ours hits our own hero. The hero of the net game is immortal for the single player (his death
// is NetLand_LocalDie) and the single player skips the damage of an immortal: the host never took the blows of the NPC.
// The animation and the sound of the hit are the game's (the blow touches the hero here), the damage is ours
void NetLand_NpcHitHost(aref attack, float fDmg, float fDmgB, bool bBlocked, int iKind)
{
	if (!bNetLandActive || bNetLandDead || !bNetLandLocReady) { return; }
	if (Net_GetTickCount() < iNetLandProtectUntil) { return; }
	ref mc = GetMainCharacter();
	float fD = fDmg;
	if (bBlocked) { fD = fDmgB; }
	int iD = makeint(fD + 0.5);
	if (iD <= 0) { return; }
	if (bBlocked)
	{
		float fHP = LAi_GetCharacterHP(mc) - makefloat(iD);
		if (fHP < 0.0) { fHP = 0.0; }
		LAi_SetHP(mc, fHP, LAi_GetCharacterMaxHP(mc));
	}
	else { LAi_ApplyCharacterDamage(mc, iD); }
	Trace("COOP: land NPC " + attack.model + " hits the host: " + iD + " kind " + iKind + " blocked " + bBlocked + " hp " + LAi_GetCharacterHP(mc));
	if (LAi_GetCharacterHP(mc) < 1.0) { NetLand_LocalDie(DST_INVALID); }
}

// a player: our hero has hit the copy of an NPC of the host
void NetLand_NpcHitCopy(aref enemy, float fDmg, int iKind)
{
	if (NetLand_NpcIsHost()) { return; }
	if (!CheckAttribute(enemy, "NetLandNpcK")) { return; }
	if (CheckAttribute(enemy, "NetLandNpcDead")) { return; }
	if (!CheckAttribute(&oNetNpcs, "gen")) { return; }
	int iD = makeint(fDmg + 0.5);
	if (iD <= 0) { return; }
	int iSMsg = NetClient_LandNewMsg(NSC_LAND_NPCDMG);
	NMAddWord(iSMsg, sti(oNetNpcs.gen));
	NMAddByte(iSMsg, sti(enemy.NetLandNpcK));
	NMAddWord(iSMsg, iD);
	NMAddByte(iSMsg, iKind);
	NetClient_LandSend(iSMsg, true);
	NMDelete(iSMsg);
	Trace("COOP: land we hit the NPC " + enemy.NetLandNpcK + ": " + iD);
}

// the server: the blow of a player on an NPC goes to the host (the machine of the server, its client is here)
void NetServer_OnLandNpcDmg(int wID, int iMsg)
{
	int iGen = NMGetWord(iMsg);
	int k = NMGetByte(iMsg);
	int iD = NMGetWord(iMsg);
	int iKind = NMGetByte(iMsg);
	if (!bNetClientConnected) { return; }
	NetLand_NpcTakeHit(wID, iGen, k, iD, iKind);
}

// the host: the NPC k takes the blow of the player wAtt and turns on him (and the other players)
void NetLand_NpcTakeHit(int wAtt, int iGen, int k, int iD, int iKind)
{
	if (!bNetLandActive || !bNetLandLocReady) { return; }
	if (iGen != iNetNpcGen) { return; }
	int iChr = NetLand_NpcHostChr(k);
	if (iChr < 0) { return; }
	ref ch = &Characters[iChr];
	if (LAi_IsDead(ch)) { return; }
	// COOP s46: the players fight the NPC only after the host has started the fight with them
	if (!NetLand_NpcAngry(ch))
	{
		Trace("COOP: land the blow of " + wAtt + " on the NPC " + k + " is ignored: the host does not fight it");
		return;
	}
	int iPup = NetLand_GetPuppet(wAtt);
	if (iPup >= 0) { LAi_group_Attack(&Characters[iPup], ch); }
	if (LAi_IsImmortal(ch)) { return; }
	LAi_ApplyCharacterDamage(ch, iD);
	LAi_CheckKillCharacter(ch);
	Trace("COOP: land the NPC " + k + " (" + ch.model + ") takes " + iD + " from " + wAtt + ", hp " + LAi_GetCharacterHP(ch));
}
