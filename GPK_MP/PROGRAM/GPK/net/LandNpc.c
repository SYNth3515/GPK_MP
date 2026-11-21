// COOP s35: the NPC of the location of a land duel (the barman, the girls and the drunkards of a tavern, the citizens, the
// soldiers...) are made by the single player on every machine by itself: each player saw other people. The machine of the
// server (the host) is the master: it sends the list of its NPC (the model, the animation, the place, the blade) every 3 s and
// their places, angles and animations 7 times a second. The others take their own NPC out of the location and show copies
// of the NPC of the host (actors, immortal: the fight with them is the one of the host). No host in the game (a dedicated
// server): every machine keeps its own NPC as before.
//   NSC_LAND_NPCLIST  word gen, byte total, byte n, n x (byte k, string model, string anim, string sex, string blade,
//                     float height, float x, y, z, ay)
//   NSC_LAND_NPCSTATE word gen, byte n, n x (byte k, float x, y, z, ay, byte flags (1 fight, 2 dead), string action)
// The server sends them on as *_R to the others.
// COOP s36: the places go only for the NPC that have moved (or changed the animation); all of them with the list.

#define NET_NPC_MAX			40		// NPC of one location at most
#define NET_NPC_LIST_MS		3000	// the list again (a player that loads the location late gets it)
#define NET_NPC_STATE_MS	150		// the places
#define NET_NPC_HIDE_MS		500		// the own NPC of a client go out of the location
#define NET_NPC_LIST_CHUNK	1		// COOP s36: a message of a client holds 128 bytes at most (net.dll Client.cpp asserts: the crash in Port Royal)
#define NET_NPC_STATE_CHUNK	2

int iNetNpcGen = 0;			// host: the number of the set of the NPC (a new load of the location makes a new set)
int iNetNpcListAt = 0;
int iNetNpcStateAt = 0;
int iNetNpcHideAt = 0;
object oNetNpcH;			// host: k<N> = the index of the character, num
object oNetNpcs;			// client: gen, seen, n<k>: model, anim, sex, blade, h, x, y, z, ay, flags, act, chr

bool NetLand_NpcIsHost()
{
	return NetServer_IsStarted();
}

// a new duel (NetLand_OnStart)
void NetLand_NpcReset()
{
	DeleteAttribute(&oNetNpcH, "");
	oNetNpcH.num = 0;
	DeleteAttribute(&oNetNpcs, "");
	iNetNpcListAt = 0;
	iNetNpcStateAt = 0;
	iNetNpcHideAt = 0;
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
	for (int i=0; i<LAi_numloginedcharacters; i++)
	{
		if (iNum >= NET_NPC_MAX) { break; }
		int iChr = LAi_loginedcharacters[i];
		if (!NetLand_NpcCandidate(iChr)) { continue; }
		string sHas = "c" + iChr;
		if (CheckAttribute(&oNetNpcH, sHas)) { continue; }
		oNetNpcH.(sHas) = iNum;
		string sK = "k" + iNum;
		oNetNpcH.(sK) = iChr;
		iNum++;
	}
	oNetNpcH.num = iNum;
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
	if (LAi_IsDead(ch)) { iFl = iFl + 2; }
	else { sAct = NetLand_GetAnim(ch, 0); }
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
	}
	oNetNpcH.(sS).nx = fX;
	oNetNpcH.(sS).ny = fY;
	oNetNpcH.(sS).nz = fZ;
	oNetNpcH.(sS).nay = fAy;
	oNetNpcH.(sS).nfl = iFl;
	oNetNpcH.(sS).nact = sAct;
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
	NMAddByte(iSMsg, k);
	NMAddFloat(iSMsg, stf(aS.x));
	NMAddFloat(iSMsg, stf(aS.y));
	NMAddFloat(iSMsg, stf(aS.z));
	NMAddFloat(iSMsg, stf(aS.ay));
	NMAddByte(iSMsg, sti(aS.fl));
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
			iHidden++;
			// the list of the location is shorter now: the same place holds the next one
			if (i < LAi_numloginedcharacters)
			{
				if (LAi_loginedcharacters[i] != iChr) { continue; }
			}
		}
		i++;
	}
	if (iHidden > 0) { Trace("COOP: land NPC: " + iHidden + " NPC of ours have gone, the ones of the host are shown"); }
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
		int iChr = sti(aE.chr);
		if (iChr >= 0) { ChangeCharacterAddressGroup(&Characters[iChr], "none", "", ""); }
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
	string sId = "NetLandN_" + k;
	int iChr = NPC_GenerateCharacter(sId, d.model, d.sex, d.anim, 1, PIRATE, 0, false);
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
	LAi_group_MoveCharacter(ch, "NetLandNpc");
	LAi_group_SetRelation("NetLandNpc", LAI_GROUP_PLAYER, LAI_GROUP_NEITRAL);
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
	SetNewModelToChar(ch);		// the same character as the copy of the last set: the entity keeps the old model without this
	SendMessage(ch, "lsl", MSG_CHARACTER_EX_MSG, "SetFightWOWeapon", false);
	TeleportCharacterToPosAy(ch, stf(d.x), stf(d.y), stf(d.z), stf(d.ay));
	d.chr = iChr;
	return iChr;
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
		string sAct = NMGetString(iMsg);
		if (!bUse) { continue; }
		string sN = "n" + k;
		if (!CheckAttribute(&oNetNpcs, sN + ".model")) { continue; }
		oNetNpcs.(sN).x = fX;
		oNetNpcs.(sN).y = fY;
		oNetNpcs.(sN).z = fZ;
		oNetNpcs.(sN).ay = fAy;
		int iChr = NetLand_NpcPupChr(k);
		if (iChr < 0) { iChr = NetLand_NpcMake(k); }
		if (iChr < 0) { continue; }
		NetLand_NpcApply(&Characters[iChr], iFl, sAct);
	}
}

// the fight mode, the death and the animation of the copy (like the puppet of a bot: NetLand_OnBotState)
void NetLand_NpcApply(ref ch, int iFl, string sAct)
{
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
	int iFight = iFl - (iFl / 2) * 2;
	int iEngF = 0;
	if (SendMessage(ch, "ls", MSG_CHARACTER_EX_MSG, "IsFightMode") != 0) { iEngF = 1; }
	if (iEngF != iFight)
	{
		if (CheckAttribute(ch, "NetLandBotAnim")) { CharacterPlayAction(ch, ""); DeleteAttribute(ch, "NetLandBotAnim"); }
		NetLand_PuppetFight(ch, iFight);
		return;		// the next packet plays the animation
	}
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
		float fK = 0.3;
		if (fD > 3.0) { fK = 1.0; }
		if (TeleportCharacterToPosAy(ch, fX + (fTX - fX) * fK, fTY, fZ + (fTZ - fZ) * fK, fAy + fDAy * fK)) { NetLand_FloorFix(ch, fTY, fAy + fDAy * fK); }
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
		return;
	}
	if (!CheckAttribute(&oNetNpcs, "seen")) { return; }		// no host (a dedicated server): our own NPC stay
	iAgo = iNow - iNetNpcHideAt;
	if (iAgo < 0 || iAgo >= NET_NPC_HIDE_MS)
	{
		iNetNpcHideAt = iNow;
		NetLand_NpcHideOwn();
	}
	NetLand_NpcSmooth();
}
