// COOP s47: maps from RESOURCE\Downloads (land duel locations and sea islands), sent by the server to a player who does not
// have them.
// A map is a folder RESOURCE\Downloads\<Dir>\ with all its files (models, .col, patches, textures .tx) and map.ini:
//   [map]
//   name=the name in the lists          (cp1251)
//   type=land | sea
//   count=N                             (the number of the lines aK, all of them must be there)
//   a1=attribute.path=value             (of the location / of the island, like in the init scripts of the single player)
// The key of the map is "Dl_" + <Dir> (Dir: latin letters, digits, '_' and '-', up to 24 chars). A land map is put into the
// slot of the TDM locations (NET_TDM_SLOT, LandTdm.c), a sea map into the free island slot NETDL_ISLAND_SLOT.
// The models and the textures are taken from the folder of the map (filespath.models / textures = "..\Downloads\<Dir>"),
// the textures that are not there from the root of the textures (the textures of the game).
// Transfer (net.dll p56: messages up to 992 bytes, the bounds of NFRReceiveData, the folders of the written file):
//   client -> NSC_DL_LISTREQ(key); server -> NSC_DL_LIST (path, size, crc32 of every file; the crc is from the loader of the
//   file sender of net.dll, NFSCreateFile -> NetFileSend_TransferReady); the client compares its files (NFRIsEqualFile) and
//   asks the missing ones one by one: NSC_DL_REQ(i) -> the server sends the chunks of 768 bytes NSC_DL_DATA(i, k) without the
//   guaranteed delivery (one packet per ping is too slow) in a window, the client answers NSC_DL_ACK(i, next k).
//   The chunk k < last covers the units of 256 bytes 3k..3k+2, the last chunk is the unit size/256 (its write makes the file).

#define NETDL_PREFIX		"Dl_"
#define NETDL_ROOT			"resource\downloads\"
#define NETDL_ISLAND_SLOT	29			// MAX_ISLANDS - 1: islands_init.c fills 0..26
#define NETDL_WINDOW		64			// chunks in flight
#define NETDL_PERFRAME		16			// chunks sent per frame and client
#define NETDL_RESEND		500			// ms without progress -> the chunks from the last acknowledged one again
#define NETDL_MAXFILES		1000
#define NETDL_MAXSIZE		67108864	// 64 MB per file
#define NETDL_H_CRC			-100000		// receive handles of the file sender of net.dll: the crc pass (- index)
#define NETDL_H_SEND		-1000000	// the transfer (- client * 4096 - index)

object NetDlMaps;		// the maps of RESOURCE\Downloads: n, mI.dir / name / type / key / ini
object NetDlSrv;		// the server: the files of its map (n, fI.path / full / size / crc / done), cC = the transfer to the client C
object NetDlCli;		// the client: the download

bool bNetDlFrame = false;
int iNetDlTime = 0;		// ms

// ---------------------------------------------------------------- the maps

bool NetDl_IsKey(string sKey)
{
	if (strlen(sKey) <= 3) { return false; }
	if (findSubStr(sKey, NETDL_PREFIX, 0) == 0) { return true; }
	return false;
}

string NetDl_KeyDir(string sKey)
{
	if (!NetDl_IsKey(sKey)) { return ""; }
	return strcut(sKey, 3, strlen(sKey) - 1);
}

// latin letters, digits, '_' '-' (and '.' ' ' '\' in a path)
bool NetDl_CharsOk(string s, bool bPath)
{
	string sOk = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-";
	int n = strlen(s);
	for (int i=0; i<n; i++)
	{
		string c = GetSymbol(s, i);
		if (findSubStr(sOk, c, 0) >= 0) { continue; }
		if (bPath)
		{
			if (c == "." || c == " " || c == "\") { continue; }
		}
		return false;
	}
	return true;
}

bool NetDl_DirOk(string sDir)
{
	int n = strlen(sDir);
	if (n < 1 || n > 24) { return false; }
	return NetDl_CharsOk(sDir, false);
}

// a path inside the folder of the map: no "..", no drive, no leading '\', known extensions only
bool NetDl_PathOk(string sPath)
{
	int n = strlen(sPath);
	if (n < 3 || n > 100) { return false; }
	if (!NetDl_CharsOk(sPath, true)) { return false; }
	if (findSubStr(sPath, "..", 0) >= 0) { return false; }
	if (findSubStr(sPath, "\\", 0) >= 0) { return false; }
	if (GetSymbol(sPath, 0) == "\" || GetSymbol(sPath, 0) == ".") { return false; }
	if (GetSymbol(sPath, n - 1) == "\") { return false; }
	int iDot = -1;
	for (int i=n-1; i>=0; i--)
	{
		string c = GetSymbol(sPath, i);
		if (c == ".") { iDot = i; break; }
		if (c == "\") { break; }
	}
	if (iDot < 0 || iDot >= n - 1) { return false; }
	string sExt = GetStrSmallRegister(strcut(sPath, iDot + 1, n - 1));
	if (sExt == "gm" || sExt == "col" || sExt == "tx" || sExt == "ptc" || sExt == "grs") { return true; }
	if (sExt == "ini" || sExt == "txt" || sExt == "tga") { return true; }
	return false;
}

string NetDl_Lower(string s) { return GetStrSmallRegister(s); }

// the folders in sDir (it ends with '\') into arOut.d0..dN-1; returns N. (The recursion of NFFindFiles in net.dll does not work:
// it is never used by the game - the folders are found by XI_FindFolders of the interface, like the profiles of SAVE)
int NetDl_SubFolders(string sDir, aref arOut)
{
	object oDirs;
	XI_FindFolders(sDir + "*", &oDirs);
	int iNum = GetAttributesNum(&oDirs);
	int iN = 0;
	for (int i=0; i<iNum; i++)
	{
		string sName = GetAttributeValue(GetAttributeN(&oDirs, i));
		if (sName == "" || sName == "." || sName == "..") { continue; }
		string sD = "d" + iN;
		arOut.(sD) = sName;
		iN++;
	}
	return iN;
}

void NetDl_Scan()
{
	DeleteAttribute(&NetDlMaps, "");
	NetDlMaps.n = 0;
	NetDlMaps.Scanned = true;
	object oList;
	aref arList;
	makearef(arList, oList);
	int iDirs = NetDl_SubFolders(NETDL_ROOT, arList);
	int iN = 0;
	for (int i=0; i<iDirs; i++)
	{
		string sD = "d" + i;
		string sDir = oList.(sD);
		if (!NetDl_DirOk(sDir)) { Trace("COOP: downloads: skipped the folder " + sDir + " (only A-Z a-z 0-9 _ -, up to 24 letters)"); continue; }
		string sFull = NETDL_ROOT + sDir + "\map.ini";
		if (NFExist(sFull) == 0) { Trace("COOP: downloads: the folder " + sDir + " has no map.ini"); continue; }
		string sType = NetDl_Lower(NetIni_GetString(sFull, "map", "type"));
		if (sType != "land" && sType != "sea") { Trace("COOP: downloads: " + sFull + " has no type=land|sea"); continue; }
		string sM = "m" + iN;
		NetDlMaps.(sM).dir = sDir;
		NetDlMaps.(sM).key = NETDL_PREFIX + sDir;
		NetDlMaps.(sM).type = sType;
		NetDlMaps.(sM).ini = sFull;
		string sName = NetIni_GetString(sFull, "map", "name");
		if (sName == "") { sName = sDir; }
		NetDlMaps.(sM).name = sName;
		iN++;
		Trace("COOP: downloads: map " + NETDL_PREFIX + sDir + " (" + sType + ") " + sName);
	}
	NetDlMaps.n = iN;
	Trace("COOP: downloads: " + NETDL_ROOT + ": " + iDirs + " folders, " + iN + " maps");
}

void NetDl_ScanIfNeeded()
{
	if (!CheckAttribute(&NetDlMaps, "Scanned")) { NetDl_Scan(); }
}

int NetDl_MapsNum() { NetDl_ScanIfNeeded(); return sti(NetDlMaps.n); }
string NetDl_MapKeyN(int i) { string s = "m" + i; return NetDlMaps.(s).key; }
string NetDl_MapNameN(int i) { string s = "m" + i; return NetDlMaps.(s).name; }
string NetDl_MapTypeN(int i) { string s = "m" + i; return NetDlMaps.(s).type; }

int NetDl_FindMap(string sKey)
{
	NetDl_ScanIfNeeded();
	for (int i=0; i<sti(NetDlMaps.n); i++)
	{
		string s = "m" + i;
		if (NetDlMaps.(s).key == sKey) { return i; }
	}
	return -1;
}

// the name of a map in the lists of this machine ("Другое" when it has no such map)
string NetDl_Name(string sKey)
{
	int i = NetDl_FindMap(sKey);
	if (i < 0) { return "Другое"; }
	return NetDl_MapNameN(i);
}

string NetDl_Picture() { return "Interfaces\Net\Islands\OpenSea.tga"; }

// "a.b.c" = v on rObj
void NetDl_SetAttr(aref ar, string sPath, string sValue)
{
	string sRest = sPath;
	int iDot = findSubStr(sRest, ".", 0);
	while (iDot > 0)
	{
		string sPart = strcut(sRest, 0, iDot - 1);
		if (!CheckAttribute(ar, sPart)) { ar.(sPart) = ""; }
		makearef(ar, ar.(sPart));
		sRest = strcut(sRest, iDot + 1, strlen(sRest) - 1);
		iDot = findSubStr(sRest, ".", 0);
	}
	if (sRest == "") { return; }
	ar.(sRest) = sValue;
}

// the lines aK of map.ini on the object (false = a bad line)
bool NetDl_ApplyIni(aref arObj, string sIni)
{
	int iCount = NetIni_GetLong(sIni, "map", "count");
	if (iCount < 0 || iCount > 400) { Trace("COOP: downloads: bad count in " + sIni); return false; }
	for (int i=1; i<=iCount; i++)
	{
		string sLine = NetIni_GetString(sIni, "map", "a" + i);
		int iEq = findSubStr(sLine, "=", 0);
		if (iEq <= 0 || iEq >= strlen(sLine) - 1) { Trace("COOP: downloads: bad line a" + i + " in " + sIni + ": " + sLine); continue; }
		string sPath = strcut(sLine, 0, iEq - 1);
		string sValue = strcut(sLine, iEq + 1, strlen(sLine) - 1);
		if (sPath == "id" || sPath == "index" || findSubStr(sPath, "filespath", 0) == 0) { continue; }
		NetDl_SetAttr(arObj, sPath, sValue);
	}
	return true;
}

// a land map into the slot of the TDM locations; returns the id of the location ("" = no such map)
string NetDl_PrepareLand(string sKey)
{
	int iMap = NetDl_FindMap(sKey);
	if (iMap < 0) { NetDl_Scan(); iMap = NetDl_FindMap(sKey); }
	if (iMap < 0) { return ""; }
	if (NetDl_MapTypeN(iMap) != "land") { return ""; }
	int iSlot = FindLocation(NET_TDM_SLOT);
	if (iSlot < 0) { return ""; }
	string sM = "m" + iMap;
	string sDir = NetDlMaps.(sM).dir;
	ref rLoc = &Locations[iSlot];
	DeleteAttribute(rLoc, "");
	rLoc.id = NET_TDM_SLOT;
	rLoc.index = iSlot;
	rLoc.image = "loading\inside\tavern.tga";
	rLoc.type = "house";
	rLoc.environment.weather = "true";
	rLoc.environment.sea = "false";
	aref arLoc; makearef(arLoc, Locations[iSlot]);
	NetDl_ApplyIni(arLoc, NetDlMaps.(sM).ini);
	rLoc.filespath.models = "..\Downloads\" + sDir;
	rLoc.filespath.textures = "..\Downloads\" + sDir;
	rLoc.NetDl = sKey;
	// the doors lead nowhere (like the TDM locations)
	if (CheckAttribute(rLoc, "reload"))
	{
		aref arRel; makearef(arRel, rLoc.reload);
		int iNum = GetAttributesNum(arRel);
		for (int i=0; i<iNum; i++)
		{
			aref arR = GetAttributeN(arRel, i);
			arR.go = NET_TDM_SLOT;
			if (!CheckAttribute(arR, "emerge")) { arR.emerge = arR.name; }
			arR.autoreload = "0";
			arR.disable = 1;
		}
	}
	LAi_LocationFantomsGen(rLoc, false);
	Trace("COOP: land duel: the downloaded map " + sKey + " is in the slot " + NET_TDM_SLOT);
	return NET_TDM_SLOT;
}

// a sea map into the island slot NETDL_ISLAND_SLOT; returns its index (-1 = no such map)
int NetDl_PrepareIsland(string sKey)
{
	if (CheckAttribute(&Islands[NETDL_ISLAND_SLOT], "id"))
	{
		if (Islands[NETDL_ISLAND_SLOT].id == sKey) { return NETDL_ISLAND_SLOT; }
		if (Islands[NETDL_ISLAND_SLOT].id != "" && !NetDl_IsKey(Islands[NETDL_ISLAND_SLOT].id))
		{
			Trace("COOP: downloads: the island slot " + NETDL_ISLAND_SLOT + " is used by " + Islands[NETDL_ISLAND_SLOT].id);
			return -1;
		}
	}
	int iMap = NetDl_FindMap(sKey);
	if (iMap < 0) { NetDl_Scan(); iMap = NetDl_FindMap(sKey); }
	if (iMap < 0) { return -1; }
	if (NetDl_MapTypeN(iMap) != "sea") { return -1; }
	string sM = "m" + iMap;
	string sDir = NetDlMaps.(sM).dir;
	ref rIsl = &Islands[NETDL_ISLAND_SLOT];
	DeleteAttribute(rIsl, "");
	rIsl.id = sKey;
	rIsl.reload_enable = true;
	rIsl.visible = true;
	rIsl.colonyquantity = 0;
	rIsl.net = false;				// not in the list of the islands of the game (Net_InitIslands)
	rIsl.ImmersionDistance = 450000.0;
	rIsl.ImmersionDepth = 0.0;
	rIsl.maxviewdist = 5000.0;
	rIsl.model = sDir;
	aref arIsl; makearef(arIsl, Islands[NETDL_ISLAND_SLOT]);
	NetDl_ApplyIni(arIsl, NetDlMaps.(sM).ini);
	rIsl.id = sKey;
	rIsl.filespath.models = "..\Downloads\" + sDir;
	rIsl.TexturePath = "..\..\Downloads\" + sDir;		// SetTexturePath("islands\" + it + "\")
	rIsl.NetDl = sDir;
	if (CheckAttribute(rIsl, "locators"))
	{
		object oDlLocator;
		CreateEntity(&oDlLocator, "locator");
		SendMessage(&oDlLocator, "le", LM_LOCATE_I, &Islands[NETDL_ISLAND_SLOT]);
		DeleteClass(&oDlLocator);
	}
	Trace("COOP: sea: the downloaded map " + sKey + " is in the island slot " + NETDL_ISLAND_SLOT);
	return NETDL_ISLAND_SLOT;
}

// client\Island.c: the folder of the grass of an island
string NetDl_IslandFilesDir(int iIsland)
{
	if (CheckAttribute(&Islands[iIsland], "NetDl")) { return NETDL_ROOT + Islands[iIsland].NetDl + "\"; }
	return "resource\models\islands\" + Islands[iIsland].id + "\";
}

bool NetDl_IsDlIsland(int iIsland)
{
	if (iIsland < 0) { return false; }
	return CheckAttribute(&Islands[iIsland], "NetDl");
}

// the map of a server: "LAND:Dl_x" / "Dl_x" -> "Dl_x" ("" = not a downloaded map)
string NetDl_KeyOfAdvert(string sIsland)
{
	string s = sIsland;
	if (NetLand_IsAdvert(s)) { s = NetLand_AdvertLoc(s); }
	if (NetDl_IsKey(s)) { return s; }
	return "";
}

// ---------------------------------------------------------------- the frame

void NetDl_FrameOn()
{
	if (bNetDlFrame) { return; }
	bNetDlFrame = true;
	SetEventHandler("frame", "NetDl_Frame", 0);
}

void NetDl_FrameOff()
{
	if (!bNetDlFrame) { return; }
	bNetDlFrame = false;
	DelEventHandler("frame", "NetDl_Frame");
}

void NetDl_Frame()
{
	int iDt = iRealDeltaTime;
	if (iDt < 0 || iDt > 1000) { iDt = 16; }
	iNetDlTime = iNetDlTime + iDt;
	bool bSrv = NetDl_SrvFrame();
	bool bCli = NetDl_CliFrame();
	if (!bSrv && !bCli) { NetDl_FrameOff(); }
}

// ---------------------------------------------------------------- the server

void NetDl_SrvReset()
{
	if (CheckAttribute(&NetDlSrv, "n"))
	{
		for (int c=0; c<NET_MAXCLIENTS; c++) { NetDl_SrvStop(c); }
		for (int i=0; i<sti(NetDlSrv.n); i++)
		{
			string sF = "f" + i;
			if (CheckAttribute(&NetDlSrv, sF + ".send")) { NFSDeleteFile(sti(NetDlSrv.(sF).send)); }
		}
	}
	DeleteAttribute(&NetDlSrv, "");
}

// the key of the map of the running server ("" = not a downloaded map)
string NetDl_SrvKey()
{
	if (!NetServer_IsStarted()) { return ""; }
	return NetDl_KeyOfAdvert(NetServer_AdvertisedIsland());
}

// the list of the files of the map and the crc pass
bool NetDl_SrvPrepare(string sKey)
{
	if (CheckAttribute(&NetDlSrv, "key"))
	{
		if (NetDlSrv.key == sKey) { return true; }
	}
	NetDl_SrvReset();
	int iMap = NetDl_FindMap(sKey);
	if (iMap < 0) { NetDl_Scan(); iMap = NetDl_FindMap(sKey); }
	if (iMap < 0) { Trace("COOP: downloads: the server has no map " + sKey); return false; }
	string sM = "m" + iMap;
	string sDir = NetDlMaps.(sM).dir;
	string sBase = NETDL_ROOT + sDir + "\";
	NetDlSrv.key = sKey;
	NetDlSrv.dir = sDir;
	NetDlSrv.ready = false;
	NetDlSrv.next = 0;
	// all the files of the map folder and of its subfolders (a queue of the folders, relative to sBase)
	object oQueue;
	oQueue.q0 = "";
	int iQueue = 1;
	int iN = 0;
	for (int iQ=0; iQ<iQueue; iQ++)
	{
		string sQ = "q" + iQ;
		string sRelDir = oQueue.(sQ);
		object oFiles;
		DeleteAttribute(&oFiles, "");
		int iFound = NFFindFiles(&oFiles, sBase + sRelDir, "*", false);
		for (int i=0; i<iFound; i++)
		{
			string sF = "f" + i;
			if (!CheckAttribute(&oFiles, sF + ".FileName")) { continue; }
			string sRel = sRelDir + oFiles.(sF).FileName;
			string sFull = sBase + sRel;
			if (!NetDl_PathOk(sRel)) { Trace("COOP: downloads: not sent " + sFull); continue; }
			if (iN >= NETDL_MAXFILES) { break; }
			string sT = "f" + iN;
			NetDlSrv.(sT).path = sRel;
			NetDlSrv.(sT).full = sFull;
			NetDlSrv.(sT).size = -1;
			NetDlSrv.(sT).crc = 0;
			iN++;
		}
		object oSub;
		DeleteAttribute(&oSub, "");
		aref arSub;
		makearef(arSub, oSub);
		int iSub = NetDl_SubFolders(sBase + sRelDir, arSub);
		for (int j=0; j<iSub && iQueue < 64; j++)
		{
			string sD = "d" + j;
			string sNewQ = "q" + iQueue;
			oQueue.(sNewQ) = sRelDir + oSub.(sD) + "\";
			iQueue++;
		}
	}
	NetDlSrv.n = iN;
	Trace("COOP: downloads: the server map " + sKey + ": " + iN + " files, crc pass");
	NetDl_FrameOn();
	return true;
}

// the file sender of net.dll loaded a file: its crc and size (the crc pass), or the start of a transfer
bool NetDl_OnTransferReady(int wNetClientID, int iSendID, int iHandle, int iCRC32, int iSize)
{
	int i, c;
	string sF, sC;
	if (iHandle <= NETDL_H_CRC && iHandle > NETDL_H_CRC - 100000)
	{
		i = NETDL_H_CRC - iHandle;
		if (!CheckAttribute(&NetDlSrv, "n")) { return true; }
		if (i >= sti(NetDlSrv.n)) { return true; }
		sF = "f" + i;
		NetDlSrv.(sF).crc = iCRC32;
		NetDlSrv.(sF).size = iSize;
		NetDlSrv.(sF).loaded = true;		// the sender is deleted in the next frame (not inside its own event)
		return true;
	}
	if (iHandle <= NETDL_H_SEND && iHandle > NETDL_H_SEND - 64 * 4096)
	{
		int iH = NETDL_H_SEND - iHandle;
		c = iH / 4096;
		i = iH - c * 4096;
		sC = "c" + c;
		if (!CheckAttribute(&NetDlSrv, sC + ".send")) { return true; }
		if (sti(NetDlSrv.(sC).send) != iSendID || sti(NetDlSrv.(sC).file) != i) { return true; }
		sF = "f" + i;
		if (iSize != sti(NetDlSrv.(sF).size))
		{
			// the file was changed since the list: the client gets what it expects only from the list
			Trace("COOP: downloads: " + NetDlSrv.(sF).path + " changed on the server (" + iSize + " / " + NetDlSrv.(sF).size + ")");
			NetDlSrv.(sF).size = iSize;
			NetDlSrv.(sF).crc = iCRC32;
		}
		NFSReceiverReady(iSendID);
		return true;
	}
	return false;
}

bool NetDl_OnSendData(int wNetClientID, int iSendID, int iHandle, int iIndex, int pData, int iDataSize)
{
	if (iHandle > NETDL_H_CRC) { return false; }
	if (iHandle > NETDL_H_SEND) { return true; }		// the crc pass sends nothing
	if (iHandle <= NETDL_H_SEND - 64 * 4096) { return false; }
	int iH = NETDL_H_SEND - iHandle;
	int c = iH / 4096;
	string sC = "c" + c;
	if (!CheckAttribute(&NetDlSrv, sC + ".send")) { return true; }
	if (sti(NetDlSrv.(sC).send) != iSendID) { return true; }
	if (iIndex != 0) { return true; }
	if (CheckAttribute(&NetDlSrv, sC + ".base")) { return true; }		// the sender repeats the unit 0 every frame
	NetDlSrv.(sC).base = pData;
	NetDlSrv.(sC).next = 0;
	NetDlSrv.(sC).acked = 0;
	NetDlSrv.(sC).time = iNetDlTime;
	NetDlSrv.(sC).prog = iNetDlTime;
	return true;
}

void NetDl_SrvStop(int c)
{
	string sC = "c" + c;
	if (CheckAttribute(&NetDlSrv, sC + ".send")) { NFSDeleteFile(sti(NetDlSrv.(sC).send)); }
	DeleteAttribute(&NetDlSrv, sC);
}

void NetDl_SrvDropClient(int c)
{
	if (c < 0 || c >= NET_MAXCLIENTS) { return; }
	if (!CheckAttribute(&NetDlSrv, "key")) { return; }
	NetDl_SrvStop(c);
}

void NetDl_SrvSendNoMap(int c)
{
	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_FILE);
	NMAddByte(iSMsg, NSC_DL_NOMAP);
	NetServer_SendMessage(c, iSMsg, true);
	NMDelete(iSMsg);
}

void NetDl_SrvSendList(int c)
{
	int n = sti(NetDlSrv.n);
	int i = 0;
	bool bMore = true;
	while (bMore)
	{
		int iSMsg = NMCreate();
		NMAddByte(iSMsg, NC_FILE);
		NMAddByte(iSMsg, NSC_DL_LIST);
		NMAddString(iSMsg, NetDlSrv.key, 32);
		NMAddWord(iSMsg, n);
		NMAddWord(iSMsg, i);
		// the items of this message: about 700 bytes
		int iCnt = 0;
		int iBytes = 0;
		int j = i;
		while (j < n && iBytes < 700)
		{
			string sF = "f" + j;
			iBytes = iBytes + strlen(NetDlSrv.(sF).path) + 10;
			iCnt++;
			j++;
		}
		NMAddByte(iSMsg, iCnt);
		for (j=i; j<i+iCnt; j++)
		{
			string sJ = "f" + j;
			NMAddString(iSMsg, NetDlSrv.(sJ).path, 100);
			NMAddDword(iSMsg, sti(NetDlSrv.(sJ).size));
			NMAddDword(iSMsg, sti(NetDlSrv.(sJ).crc));
		}
		NetServer_SendMessage(c, iSMsg, true);
		NMDelete(iSMsg);
		i = i + iCnt;
		if (i >= n || iCnt <= 0) { bMore = false; }
	}
	Trace("COOP: downloads: the list of " + NetDlSrv.key + " (" + n + " files) to the client " + c);
}

void NetDl_SrvOnListReq(int c, int iMsg)
{
	string sKey = NMGetString(iMsg);
	if (c < 0 || c >= NET_MAXCLIENTS) { return; }
	if (sKey == "" || sKey != NetDl_SrvKey()) { NetDl_SrvSendNoMap(c); return; }
	if (!NetDl_SrvPrepare(sKey)) { NetDl_SrvSendNoMap(c); return; }
	NetDl_SrvStop(c);
	string sC = "c" + c;
	NetDlSrv.(sC).listreq = true;		// the list goes when the crc pass is over (NetDl_SrvFrame)
	NetDl_FrameOn();
}

void NetDl_SrvOnReq(int c, int iMsg)
{
	int i = NMGetWord(iMsg);
	if (c < 0 || c >= NET_MAXCLIENTS) { return; }
	if (!CheckAttribute(&NetDlSrv, "ready")) { return; }
	if (!sti(NetDlSrv.ready)) { return; }
	if (i < 0 || i >= sti(NetDlSrv.n)) { return; }
	string sC = "c" + c;
	if (CheckAttribute(&NetDlSrv, sC + ".file"))
	{
		if (sti(NetDlSrv.(sC).file) == i) { return; }		// a repeated request of the same file
	}
	NetDl_SrvStop(c);
	string sF = "f" + i;
	int iSize = sti(NetDlSrv.(sF).size);
	if (iSize <= 0) { return; }
	NetDlSrv.(sC).file = i;
	NetDlSrv.(sC).last = iSize / 256;
	NetDlSrv.(sC).chunks = (iSize / 256 + 2) / 3 + 1;
	NetDlSrv.(sC).send = NFSCreateFile(c, NETDL_H_SEND - c * 4096 - i, NetDlSrv.(sF).full);
	NetDl_FrameOn();
}

void NetDl_SrvOnAck(int c, int iMsg)
{
	int i = NMGetWord(iMsg);
	int k = NMGetDword(iMsg);
	if (c < 0 || c >= NET_MAXCLIENTS) { return; }
	string sC = "c" + c;
	if (!CheckAttribute(&NetDlSrv, sC + ".base")) { return; }
	if (sti(NetDlSrv.(sC).file) != i) { return; }
	if (k >= sti(NetDlSrv.(sC).chunks)) { NetDl_SrvStop(c); return; }		// the file is there
	if (k > sti(NetDlSrv.(sC).acked))
	{
		NetDlSrv.(sC).acked = k;
		NetDlSrv.(sC).time = iNetDlTime;
		NetDlSrv.(sC).prog = iNetDlTime;
		if (sti(NetDlSrv.(sC).next) < k) { NetDlSrv.(sC).next = k; }
	}
}

void NetDl_SrvSendChunk(int c, int k)
{
	string sC = "c" + c;
	int i = sti(NetDlSrv.(sC).file);
	string sF = "f" + i;
	int iSize = sti(NetDlSrv.(sF).size);
	int iLast = sti(NetDlSrv.(sC).last);
	int iChunks = sti(NetDlSrv.(sC).chunks);
	int iUnit = iLast;
	int iLen = iSize - iLast * 256;
	if (k < iChunks - 1)
	{
		iUnit = k * 3;
		int iUnits = iLast - iUnit;
		if (iUnits > 3) { iUnits = 3; }
		iLen = iUnits * 256;
	}
	int iSMsg = NMCreate();
	NMAddByte(iSMsg, NC_FILE);
	NMAddByte(iSMsg, NSC_DL_DATA);
	NMAddWord(iSMsg, i);
	NMAddDword(iSMsg, k);
	NMAddData(iSMsg, sti(NetDlSrv.(sC).base) + iUnit * 256, iLen);
	NetServer_SendMessage(c, iSMsg, false);
	NMDelete(iSMsg);
}

// true = there is work
bool NetDl_SrvFrame()
{
	if (!CheckAttribute(&NetDlSrv, "n")) { return false; }
	if (!NetServer_IsStarted()) { NetDl_SrvReset(); return false; }
	int i, c, k;
	string sF, sC;
	int n = sti(NetDlSrv.n);
	bool bWork = false;

	// the crc pass: 4 files at a time
	if (!sti(NetDlSrv.ready))
	{
		bWork = true;
		int iBusy = 0;
		bool bAll = true;
		for (i=0; i<n; i++)
		{
			sF = "f" + i;
			if (CheckAttribute(&NetDlSrv, sF + ".loaded"))
			{
				NFSDeleteFile(sti(NetDlSrv.(sF).send));
				DeleteAttribute(&NetDlSrv, sF + ".send");
				DeleteAttribute(&NetDlSrv, sF + ".loaded");
			}
			if (CheckAttribute(&NetDlSrv, sF + ".send"))
			{
				if (iNetDlTime - sti(NetDlSrv.(sF).t0) > 10000)
				{
					// net.dll could not load it (it tries every 2 s): not sent
					Trace("COOP: downloads: can not read " + NetDlSrv.(sF).full);
					NFSDeleteFile(sti(NetDlSrv.(sF).send));
					DeleteAttribute(&NetDlSrv, sF + ".send");
					NetDlSrv.(sF).size = 0;
					continue;
				}
				iBusy++; bAll = false; continue;
			}
			if (sti(NetDlSrv.(sF).size) < 0) { bAll = false; }
		}
		int iNext = sti(NetDlSrv.next);
		while (iBusy < 4 && iNext < n)
		{
			sF = "f" + iNext;
			NetDlSrv.(sF).send = NFSCreateFile(0, NETDL_H_CRC - iNext, NetDlSrv.(sF).full);
			NetDlSrv.(sF).t0 = iNetDlTime;
			iBusy++;
			iNext++;
			bAll = false;
		}
		NetDlSrv.next = iNext;
		if (bAll)
		{
			NetDlSrv.ready = true;
			Trace("COOP: downloads: crc pass of " + NetDlSrv.key + " done");
		}
	}

	for (c=0; c<NET_MAXCLIENTS; c++)
	{
		sC = "c" + c;
		if (!CheckAttribute(&NetDlSrv, sC)) { continue; }
		bWork = true;
		if (CheckAttribute(&NetDlSrv, sC + ".listreq"))
		{
			if (sti(NetDlSrv.ready))
			{
				DeleteAttribute(&NetDlSrv, sC + ".listreq");
				NetDl_SrvSendList(c);
			}
			continue;
		}
		if (!CheckAttribute(&NetDlSrv, sC + ".base")) { continue; }
		if (iNetDlTime - sti(NetDlSrv.(sC).prog) > 15000)
		{
			Trace("COOP: downloads: the client " + c + " does not answer, the transfer stops");
			NetDl_SrvStop(c);
			continue;
		}
		int iChunks = sti(NetDlSrv.(sC).chunks);
		int iAcked = sti(NetDlSrv.(sC).acked);
		int iNextK = sti(NetDlSrv.(sC).next);
		if (iNetDlTime - sti(NetDlSrv.(sC).time) > NETDL_RESEND)
		{
			iNextK = iAcked;		// go back to the last acknowledged chunk
			NetDlSrv.(sC).time = iNetDlTime;
		}
		int iSent = 0;
		while (iSent < NETDL_PERFRAME && iNextK < iChunks && iNextK < iAcked + NETDL_WINDOW)
		{
			NetDl_SrvSendChunk(c, iNextK);
			iNextK++;
			iSent++;
		}
		NetDlSrv.(sC).next = iNextK;
	}
	return bWork;
}

// net\server\File.c
void NetDl_SrvOnFile(int wNetClientID, int iSubCode, int iMsg)
{
	switch (iSubCode)
	{
		case NSC_DL_LISTREQ:	NetDl_SrvOnListReq(wNetClientID, iMsg);	break;
		case NSC_DL_REQ:		NetDl_SrvOnReq(wNetClientID, iMsg);		break;
		case NSC_DL_ACK:		NetDl_SrvOnAck(wNetClientID, iMsg);		break;
	}
}

// ---------------------------------------------------------------- the client
// NetDlCli.state: 0 nothing, 1 waits for the list, 2 downloads, 3 done, -1 failed

int NetDl_CliState()
{
	if (!CheckAttribute(&NetDlCli, "state")) { return 0; }
	return sti(NetDlCli.state);
}

bool NetDl_CliBusy()
{
	int s = NetDl_CliState();
	return s == 1 || s == 2;
}

void NetDl_CliReset()
{
	if (CheckAttribute(&NetDlCli, "recv")) { NFRDeleteFile(sti(NetDlCli.recv)); }
	DeleteAttribute(&NetDlCli, "");
	NetDlCli.state = 0;
}

void NetDl_CliFail(string sReason)
{
	if (CheckAttribute(&NetDlCli, "recv")) { NFRDeleteFile(sti(NetDlCli.recv)); DeleteAttribute(&NetDlCli, "recv"); }
	NetDlCli.state = -1;
	NetDlCli.reason = sReason;
	Trace("COOP: downloads: failed: " + sReason);
	if (CheckAttribute(&NetDlCli, "background")) { NetClient_ChatAddSystemLine("Загрузка карты не удалась: " + sReason); }
	Event("NCEvent_DlDone", "l", 0);
}

void NetDl_CliDone()
{
	NetDlCli.state = 3;
	NetDl_Scan();
	Trace("COOP: downloads: " + NetDlCli.key + " ready, downloaded " + NetDlCli.got + " bytes");
	if (CheckAttribute(&NetDlCli, "background")) { NetClient_ChatAddSystemLine("Карта «" + NetDl_Name(NetDlCli.key) + "» загружена"); }
	Event("NCEvent_DlDone", "l", 1);
}

// after the connect (client\OnConnect.c) and after a change of the map in the lobby (bBackground): true = a download starts
bool NetDl_CliStart(string sKey, bool bBackground)
{
	if (!NetDl_IsKey(sKey)) { return false; }
	if (NetServer_IsStarted()) { return false; }		// the host has the map
	if (NetDl_CliBusy())
	{
		if (NetDlCli.key == sKey) { return true; }
	}
	NetDl_CliReset();
	string sDir = NetDl_KeyDir(sKey);
	if (!NetDl_DirOk(sDir)) { Trace("COOP: downloads: bad map key " + sKey); return false; }
	NetDlCli.key = sKey;
	NetDlCli.dir = sDir;
	NetDlCli.state = 1;
	NetDlCli.time = iNetDlTime;
	NetDlCli.got = 0;
	NetDlCli.listn = 0;
	if (bBackground) { NetDlCli.background = true; }

	int iSMsg = NMCreate();
	NMAddClientID(iSMsg, wClientID);
	NMAddByte(iSMsg, NC_FILE);
	NMAddByte(iSMsg, NSC_DL_LISTREQ);
	NMAddString(iSMsg, sKey, 32);
	NetClient_SendMessage(iSMsg, true);
	NMDelete(iSMsg);
	NetDl_FrameOn();
	Trace("COOP: downloads: the list of " + sKey + " is asked");
	return true;
}

void NetDl_CliOnList(int iMsg)
{
	string sListKey = NMGetString(iMsg);
	int n = NMGetWord(iMsg);
	int iFirst = NMGetWord(iMsg);
	int iCnt = NMGetByte(iMsg);
	if (NetDl_CliState() != 1) { return; }
	if (sListKey != NetDlCli.key) { return; }
	if (n <= 0 || n > NETDL_MAXFILES) { NetDl_CliFail("неверный список файлов"); return; }
	if (iFirst != sti(NetDlCli.listn)) { return; }
	NetDlCli.n = n;
	for (int j=0; j<iCnt; j++)
	{
		string sF = "f" + (iFirst + j);
		NetDlCli.(sF).path = NMGetString(iMsg);
		NetDlCli.(sF).size = NMGetDword(iMsg);
		NetDlCli.(sF).crc = NMGetDword(iMsg);
	}
	NetDlCli.listn = iFirst + iCnt;
	NetDlCli.time = iNetDlTime;
	if (sti(NetDlCli.listn) < n) { return; }
	NetDl_CliCheck();
}

string NetDl_CliDest(int i)
{
	string sF = "f" + i;
	return NETDL_ROOT + NetDlCli.dir + "\" + NetDlCli.(sF).path;
}

// which files are missing or other
void NetDl_CliCheck()
{
	int n = sti(NetDlCli.n);
	int iNeed = 0;
	int iBytes = 0;
	int iTotal = 0;
	for (int i=0; i<n; i++)
	{
		string sF = "f" + i;
		string sPath = NetDlCli.(sF).path;
		int iSize = sti(NetDlCli.(sF).size);
		if (!NetDl_PathOk(sPath)) { NetDl_CliFail("недопустимое имя файла " + sPath); return; }
		if (iSize < 0 || iSize > NETDL_MAXSIZE) { NetDl_CliFail("недопустимый размер файла " + sPath); return; }
		iTotal = iTotal + iSize;
		if (iSize == 0) { continue; }
		string sDest = NetDl_CliDest(i);
		bool bSame = false;
		if (NFExist(sDest) != 0)
		{
			int iProbe = NFRCreateFile(-1, sDest, -1, -1);
			bSame = NFRIsEqualFile(iProbe, sti(NetDlCli.(sF).crc), iSize);
			NFRDeleteFile(iProbe);
		}
		if (bSame) { continue; }
		string sN = "n" + iNeed;
		NetDlCli.need.(sN) = i;
		iNeed++;
		iBytes = iBytes + iSize;
	}
	if (iTotal > 268435456) { NetDl_CliFail("карта больше 256 МБ"); return; }
	NetDlCli.needn = iNeed;
	NetDlCli.needcur = -1;
	NetDlCli.bytes = iBytes;
	Trace("COOP: downloads: " + NetDlCli.key + ": " + n + " files, " + iNeed + " to download (" + iBytes + " bytes)");
	if (iNeed == 0) { NetDl_CliDone(); return; }
	NetDlCli.state = 2;
	NetDl_CliNextFile();
}

void NetDl_CliNextFile()
{
	if (CheckAttribute(&NetDlCli, "recv")) { NFRDeleteFile(sti(NetDlCli.recv)); DeleteAttribute(&NetDlCli, "recv"); }
	int iCur = sti(NetDlCli.needcur) + 1;
	NetDlCli.needcur = iCur;
	if (iCur >= sti(NetDlCli.needn)) { NetDl_CliDone(); return; }
	string sN = "n" + iCur;
	int i = sti(NetDlCli.need.(sN));
	string sF = "f" + i;
	int iSize = sti(NetDlCli.(sF).size);
	NetDlCli.file = i;
	NetDlCli.expect = 0;
	NetDlCli.last = iSize / 256;
	NetDlCli.chunks = (iSize / 256 + 2) / 3 + 1;
	NetDlCli.filegot = 0;
	NetDlCli.time = iNetDlTime;
	NetDlCli.acktime = iNetDlTime;
	int iRecv = NFRCreateFile(-1, NetDl_CliDest(i), -1, -1);
	NFRSetSendHandle(iRecv, 0, iSize);
	NetDlCli.recv = iRecv;

	int iSMsg = NMCreate();
	NMAddClientID(iSMsg, wClientID);
	NMAddByte(iSMsg, NC_FILE);
	NMAddByte(iSMsg, NSC_DL_REQ);
	NMAddWord(iSMsg, i);
	NetClient_SendMessage(iSMsg, true);
	NMDelete(iSMsg);
}

void NetDl_CliSendAck()
{
	int iSMsg = NMCreate();
	NMAddClientID(iSMsg, wClientID);
	NMAddByte(iSMsg, NC_FILE);
	NMAddByte(iSMsg, NSC_DL_ACK);
	NMAddWord(iSMsg, sti(NetDlCli.file));
	NMAddDword(iSMsg, sti(NetDlCli.expect));
	NetClient_SendMessage(iSMsg, false);
	NMDelete(iSMsg);
	NetDlCli.acktime = iNetDlTime;
}

void NetDl_CliOnData(int iMsg)
{
	int i = NMGetWord(iMsg);
	int k = NMGetDword(iMsg);
	if (NetDl_CliState() != 2) { return; }
	if (!CheckAttribute(&NetDlCli, "recv")) { return; }
	if (i != sti(NetDlCli.file)) { return; }
	int iExpect = sti(NetDlCli.expect);
	if (k != iExpect)
	{
		if (iNetDlTime - sti(NetDlCli.acktime) > 100) { NetDl_CliSendAck(); }
		return;
	}
	int iChunks = sti(NetDlCli.chunks);
	int iLast = sti(NetDlCli.last);
	int iUnit = iLast;
	if (k < iChunks - 1) { iUnit = k * 3; }
	string sF = "f" + i;
	int iSize = sti(NetDlCli.(sF).size);
	int iLen = iSize - iLast * 256;
	if (k < iChunks - 1)
	{
		iLen = (iLast - iUnit) * 256;
		if (iLen > 768) { iLen = 768; }
	}
	NFRReceiveData(sti(NetDlCli.recv), iUnit, iMsg);		// the last unit writes the file and frees the receiver
	iExpect++;
	NetDlCli.expect = iExpect;
	NetDlCli.got = sti(NetDlCli.got) + iLen;
	NetDlCli.filegot = sti(NetDlCli.filegot) + iLen;
	NetDlCli.time = iNetDlTime;
	NetDl_CliSendAck();
	if (iExpect >= iChunks)
	{
		DeleteAttribute(&NetDlCli, "recv");
		NetDl_CliNextFile();
	}
}

void NetDl_CliOnNoMap()
{
	if (!NetDl_CliBusy()) { return; }
	NetDl_CliFail("на сервере нет этой карты");
}

// true = there is work
bool NetDl_CliFrame()
{
	int s = NetDl_CliState();
	if (s != 1 && s != 2) { return false; }
	if (!bNetClientConnected) { NetDl_CliReset(); return false; }
	if (iNetDlTime - sti(NetDlCli.time) > 20000) { NetDl_CliFail("сервер не отвечает"); return false; }
	if (s == 2)
	{
		if (iNetDlTime - sti(NetDlCli.acktime) > 200) { NetDl_CliSendAck(); }
	}
	return true;
}

// "Загрузка ресурсов: <name> - file i of N (x%)"
string NetDl_CliProgress()
{
	int s = NetDl_CliState();
	string sName = NetDl_Name(NetDlCli.key);
	if (sName == "Другое") { sName = NetDlCli.dir; }
	if (s == 1) { return "Карта «" + sName + "»: список файлов..."; }
	if (s == 3) { return "Карта «" + sName + "» загружена"; }
	if (s == -1) { return "Не удалось загрузить карту «" + sName + "»: " + NetDlCli.reason; }
	if (s != 2) { return ""; }
	int iPct = 0;
	int iBytes = sti(NetDlCli.bytes);
	int iGot = sti(NetDlCli.got);
	if (iBytes > 0) { iPct = makeint(makefloat(iGot) * 100.0 / makefloat(iBytes)); }
	if (iPct > 100) { iPct = 100; }
	string sKb = "" + (iGot / 1024) + " / " + (iBytes / 1024) + " КБ";
	return "Карта «" + sName + "»: файл " + (sti(NetDlCli.needcur) + 1) + " из " + NetDlCli.needn + " (" + iPct + "%, " + sKb + ")";
}

// net\client\File.c
void NetDl_CliOnFile(int iSubCode, int iMsg)
{
	switch (iSubCode)
	{
		case NSC_DL_LIST:		NetDl_CliOnList(iMsg);		break;
		case NSC_DL_DATA:		NetDl_CliOnData(iMsg);		break;
		case NSC_DL_NOMAP:		NetDl_CliOnNoMap();			break;
	}
}

// the lobby: the server changed the map (client\ServerInfo.c)
void NetDl_CliMapChanged(string sIsland)
{
	string sKey = NetDl_KeyOfAdvert(sIsland);
	if (sKey == "") { return; }
	if (NetDl_CliBusy()) { return; }
	if (NetDl_CliState() != 0 && NetDlCli.key == sKey) { return; }		// done or failed: once per map
	if (NetDl_CliStart(sKey, true)) { NetClient_ChatAddSystemLine("Загрузка карты сервера..."); }
}
