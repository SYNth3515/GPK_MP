// ============================================================================
// Client side: mines (floating barrels), brander explosion, ship load info for the jettison menu
// ============================================================================
object	NCMines[NET_MINE_MAX];
int		NCMineUse[NET_MINE_MAX];
object	NCLoad;				// last ship load info from the server
object	NCJettison;			// what the player wants to throw away (Goods.<name>), filled by the jettison interface

// папка текстур бочки: ITEMS.TGA искался в textures\SwimGoods\ и не нашёлся, поэтому по умолчанию корень textures\
// (если ITEMS.TGA лежит в другой папке, впиши её сюда, например "items\\")
#define NET_MINE_TEXDIR		""

// шейдер бочки: NetMineBarrel из modules\techniques\netmine.sha (RandItem без альфа-смешивания и с CullMode=none).
// Если файл netmine.sha не скопирован, в system.log будут строки "unknown technique"; тогда можно временно поставить "WdmModelDrawStd"
#define NET_MINE_TECH			"NetMineBarrel"
// уровень воды у бочки = сглаженная высота корабля игрока + это смещение (подбирается по виду: бочка тонет - увеличить, парит - уменьшить)
#define NET_MINE_Y_OFFSET		0.0
// размах собственного покачивания бочки, м
#define NET_MINE_BOB			0.3

float NCMineLevel = 0.0;
bool NCMineLevelInit = false;
bool NCMineBobOn = false;
int NCMineBobTick = 0;

void NetClient_MineRemove(int iSlot)
{
	if (iSlot < 0 || iSlot >= NET_MINE_MAX) { return; }
	if (!NCMineUse[iSlot]) { return; }
	NCMineUse[iSlot] = 0;
	if (isEntity(&NCMines[iSlot])) { DeleteClass(&NCMines[iSlot]); }
}

void NetClient_MinesClear()
{
	if (NCMineBobOn)
	{
		NCMineBobOn = false;
		NCMineLevelInit = false;
		DelEventHandler("NetClient_MinesBob", "NetClient_MinesBob");
	}
	for (int i=0; i<NET_MINE_MAX; i++) { NetClient_MineRemove(i); }
}

void NetClient_OnShipMineAdd(int iMsg)
{
	int iSlot = NMGetByte(iMsg);
	float x = NMGetFloat(iMsg);
	float z = NMGetFloat(iMsg);
	int wOwner = NMGetClientID(iMsg);
	if (iSlot < 0 || iSlot >= NET_MINE_MAX) { return; }
	if (!bNetClientInGame) { return; }		// COOP: lobby of a running game: no sea here (the mines come when we join)
	NetClient_MineRemove(iSlot);

	Trace("COOP: mine add slot=" + iSlot + " x=" + x + " z=" + z + " owner=" + wOwner);

	NCMineUse[iSlot] = 1;
	// модель SwimGoods\barrel как в одиночке; у неё нет своего шейдера (лог: unknown technique <>), поэтому задаём его явно, как у предметов (Items_LoadModel)
	CreateEntity(&NCMines[iSlot], "MODELR");
	SendMessage(&NCMines[iSlot], "ls", MSG_MODEL_SET_DIRPATH, NET_MINE_TEXDIR);
	SendMessage(&NCMines[iSlot], "ls", MSG_MODEL_LOAD_GEO, "SwimGoods\barrel");
	SendMessage(&NCMines[iSlot], "ls", MSG_MODEL_SET_DIRPATH, "");
	SendMessage(&NCMines[iSlot], "ls", MSG_MODEL_SET_TECHNIQUE, NET_MINE_TECH);
	NCMines[iSlot].px = x;
	NCMines[iSlot].pz = z;
	NetClient_MineSetY(iSlot, 0.0);
	LayerAddObject("net_execute", &NCMines[iSlot], 65540);
	LayerAddObject("net_realize", &NCMines[iSlot], 65540);

	if (!NCMineBobOn)
	{
		NCMineBobOn = true;
		SetEventHandler("NetClient_MinesBob", "NetClient_MinesBob", 0);
		PostEvent("NetClient_MinesBob", 50);
	}
}

// бочка: общий уровень воды + своё небольшое покачивание
void NetClient_MineSetY(int iSlot, float fTime)
{
	float y = NCMineLevel + NET_MINE_BOB * sin(fTime * 1.3 + iSlot * 1.7) + 0.5 * NET_MINE_BOB * sin(fTime * 2.1 + iSlot * 3.4);
	SendMessage(&NCMines[iSlot], "lffffffffffff", MSG_MODEL_SET_POSITION, stf(NCMines[iSlot].px), y, stf(NCMines[iSlot].pz), 1, 0, 0, 0, 1, 0, 0, 0, 1);
}

// высоту воды скрипт получить не может (в одиночке это делает движковый AISeaGoods), поэтому берём
// высоту собственного корабля (его двигают волны) и слегка сглаживаем её, чтобы бочка следовала за волной
void NetClient_MineUpdateLevel()
{
	ref rMe = NetClient_GetClient(NetClient_GetThisClientID());
	float fShipY = stf(rMe.Ship.Pos.y);
	if (fShipY < -50.0 || fShipY > 50.0) { return; }
	if (!NCMineLevelInit)
	{
		NCMineLevel = fShipY + NET_MINE_Y_OFFSET;
		NCMineLevelInit = true;
		return;
	}
	NCMineLevel = NCMineLevel + (fShipY + NET_MINE_Y_OFFSET - NCMineLevel) * 0.3;
}

void NetClient_MinesBob()
{
	NCMineBobTick++;
	NetClient_MineUpdateLevel();
	if (NCMineBobTick % 40 == 1)
	{
		ref rMeTr = NetClient_GetClient(NetClient_GetThisClientID());
		Trace("COOP: mine level=" + NCMineLevel + " shipY=" + stf(rMeTr.Ship.Pos.y));
	}
	float fTime = NCMineBobTick * 0.05;
	for (int i=0; i<NET_MINE_MAX; i++)
	{
		if (NCMineUse[i]) { NetClient_MineSetY(i, fTime); }
	}
	PostEvent("NetClient_MinesBob", 50);
}

void NetClient_OnShipMineDel(int iMsg)
{
	int iSlot = NMGetByte(iMsg);
	float x = NMGetFloat(iMsg);
	float z = NMGetFloat(iMsg);
	int wOwner = NMGetClientID(iMsg);
	NetClient_MineRemove(iSlot);
}

void NetClient_OnShipMineBoom(int iMsg)
{
	int iSlot = NMGetByte(iMsg);
	float x = NMGetFloat(iMsg);
	float z = NMGetFloat(iMsg);
	int wOwner = NMGetClientID(iMsg);
	NetClient_MineRemove(iSlot);
	if (!bNetClientInGame) { return; }
	NetClient_ShipSeriousBoom(x, 1.0, z);
}

void NetClient_OnShipBranderBoom(int iMsg)
{
	float x = NMGetFloat(iMsg);
	float y = NMGetFloat(iMsg);
	float z = NMGetFloat(iMsg);
	if (!bNetClientInGame) { return; }
	NetClient_ShipSeriousBoom(x, y + 3.0, z);
	CreateParticleSystem("blast_inv", x, y + 6.0, z, 0.0, 0.0, 0.0, 0);
}

// something was thrown into the water (cargo, mine): 3D splash sound, the volume depends on the distance to the camera
void NetClient_OnShipSplash(int iMsg)
{
	float x = NMGetFloat(iMsg);
	float y = NMGetFloat(iMsg);
	float z = NMGetFloat(iMsg);
	Play3DSound(NET_SOUND_SPLASH, x, y, z);
}

// the server refused a mine / brander: tell the player why
void NetClient_OnShipActionDenied(int iMsg)
{
	int iReason = NMGetByte(iMsg);
	int iHave = NMGetWord(iMsg);
	int iNeed = NMGetWord(iMsg);

	Trace("COOP: ship action denied reason=" + iReason + " have=" + iHave + " need=" + iNeed);

	switch (iReason)
	{
		case NET_DENY_POWDER:
			Log_SetStringToLog("Не хватает пороха: в трюме " + iHave + ", нужно " + iNeed);
		break;
		case NET_DENY_MINE_COOLDOWN:
			Log_SetStringToLog("Мина ещё не готова, подождите " + iHave + " с");
		break;
		case NET_DENY_MINE_LIMIT:
			Log_SetStringToLog("На море уже слишком много мин");
		break;
	}
}

void NetClient_OnShipLoadInfo(int iMsg)
{
	NCLoad.Load = NMGetWord(iMsg);
	NCLoad.Capacity = NMGetWord(iMsg);
	NCLoad.Crew = NMGetWord(iMsg);
	NCLoad.Cannons = NMGetWord(iMsg);
	for (int i=0; i<Net_GetGoodsNum(); i++)
	{
		string sGood = NetGoods[i].Name;
		NCLoad.Goods.(sGood) = NMGetWord(iMsg);
	}
	for (int b=0; b<NET_BORT_NUM; b++)		// COOP: cannons on the borts and their places
	{
		string sB = Net_BortLetter(b);
		NCLoad.Bort.(sB).Have = NMGetByte(iMsg);
		NCLoad.Bort.(sB).Max = NMGetByte(iMsg);
	}
	Event("NCEvent_LoadInfo");
}

// ---------- requests to the server ----------
void NetClient_ShipSimpleRequest(int iSubCode)
{
	int iSMsg = NMCreate();
	NMAddClientID(iSMsg, wClientID);
	NMAddByte(iSMsg, NC_SHIP);
	NMAddByte(iSMsg, iSubCode);
	NetClient_SendMessage(iSMsg, true);
	NMDelete(iSMsg);
}

void NetClient_ShipDropMine() { NetClient_ShipSimpleRequest(NSC_SHIP_DROP_MINE); }
void NetClient_ShipBrander() { NetClient_ShipSimpleRequest(NSC_SHIP_BRANDER); }
void NetClient_ShipBranderArm() { NetClient_ShipSimpleRequest(NSC_SHIP_BRANDER_ARM); }
void NetClient_ShipLoadRequest() { NetClient_ShipSimpleRequest(NSC_SHIP_LOAD_REQUEST); }

// COOP: cannon menu of the jettison screen: cannons on the bow, stern, left, right (the rest goes overboard)
void NetClient_ShipCannonsSet(int iBow, int iStern, int iLeft, int iRight)
{
	int iSMsg = NMCreate();
	NMAddClientID(iSMsg, wClientID);
	NMAddByte(iSMsg, NC_SHIP);
	NMAddByte(iSMsg, NSC_SHIP_CANNONS_SET);
	NMAddByte(iSMsg, iBow);
	NMAddByte(iSMsg, iStern);
	NMAddByte(iSMsg, iLeft);
	NMAddByte(iSMsg, iRight);
	NetClient_SendMessage(iSMsg, true);
	NMDelete(iSMsg);
}

// iGoods[] - how many pieces of every good to throw away
void NetClient_ShipJettison(int iCannons, int iCrew)
{
	int iSMsg = NMCreate();
	NMAddClientID(iSMsg, wClientID);
	NMAddByte(iSMsg, NC_SHIP);
	NMAddByte(iSMsg, NSC_SHIP_JETTISON);
	for (int i=0; i<Net_GetGoodsNum(); i++)
	{
		string sGood = NetGoods[i].Name;
		NMAddWord(iSMsg, sti(NCJettison.Goods.(sGood)));
	}
	NMAddWord(iSMsg, iCannons);
	NMAddWord(iSMsg, iCrew);
	NetClient_SendMessage(iSMsg, true);
	NMDelete(iSMsg);
}

// Brander blows the own ship up, so it needs a second press within NET_BRANDER_ARM_TIME (30 seconds)
bool NetClient_BranderArmed = false;

void NetClient_BranderDisarm() { NetClient_BranderArmed = false; }

void NetClient_ShipBranderPress()
{
	// COOP: the brander is an active perk of single player: only with the "All active perks" switch of the server
	if (!sti(NCServer.ActivePerks)) { return; }
	if (NetClient_IsThisShipDead()) { return; }
	if (!NetClient_BranderArmed)
	{
		NetClient_BranderArmed = true;
		PostEvent("NetClient_BranderDisarm", NET_BRANDER_ARM_TIME);
		NetClient_ShipBranderArm();	// the server remembers it: if we are boarded in this time, the brander goes off by itself
		Log_SetStringToLog("Брандер: нажмите ещё раз в течение " + (NET_BRANDER_ARM_TIME / 1000) + " секунд, чтобы взорвать корабль (при абордаже взорвётся сам)");
		return;
	}
	NetClient_BranderArmed = false;
	NetClient_ShipBrander();
}
