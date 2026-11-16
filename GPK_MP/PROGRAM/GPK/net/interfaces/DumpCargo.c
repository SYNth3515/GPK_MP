// Jettison menu: throw goods and crew overboard to make the ship lighter.
// Layout and controls are made like the goods table of the lobby (Buy.ini) / ship goods trade screen of single player.
// Row 0..goods-1: goods, row goods: crew.
// COOP: the cannons are managed in the cannon menu (button "Cannons", like the cannon menu of the ship screen of single
// player): cannons are moved between the borts; removed cannons that are not placed on a bort go overboard.

int iDumpRows = 0;
int iDumpHave[10];
int iDumpThrow[10];
int iDumpLoad = 0;
int iDumpCapacity = 1;
int iDumpCannonWeight = 0;

// cannon menu: 0 bow, 1 stern, 2 left, 3 right
bool bDumpCannonsOpen = false;
int iDCanHave[4];
int iDCanMax[4];
int iDCanSet[4];
int iDCanOnBoard = 0;
int iDCanPlaces = 0;		// places for cannons on the ship (sum of the borts)

void InitInterface(string iniName)
{
	GameInterface.title = "";

	SendMessage(&GameInterface, "ls", MSG_INTERFACE_INIT, iniName);

	SetEventHandler("OnCancel", "Dump_ICancel", 0);
	SetEventHandler("exitCancel", "Dump_ICancel", 0);
	SetEventHandler("DumpOk", "Dump_OnOk", 0);
	SetEventHandler("NCEvent_LoadInfo", "Dump_OnLoadInfo", 0);
	SetEventHandler("DumpAdd0", "Dump_Add0", 0);	SetEventHandler("DumpDec0", "Dump_Dec0", 0);
	SetEventHandler("DumpAdd1", "Dump_Add1", 0);	SetEventHandler("DumpDec1", "Dump_Dec1", 0);
	SetEventHandler("DumpAdd2", "Dump_Add2", 0);	SetEventHandler("DumpDec2", "Dump_Dec2", 0);
	SetEventHandler("DumpAdd3", "Dump_Add3", 0);	SetEventHandler("DumpDec3", "Dump_Dec3", 0);
	SetEventHandler("DumpAdd4", "Dump_Add4", 0);	SetEventHandler("DumpDec4", "Dump_Dec4", 0);
	SetEventHandler("DumpAdd5", "Dump_Add5", 0);	SetEventHandler("DumpDec5", "Dump_Dec5", 0);
	SetEventHandler("DumpAdd6", "Dump_Add6", 0);	SetEventHandler("DumpDec6", "Dump_Dec6", 0);
	SetEventHandler("DumpAdd7", "Dump_Add7", 0);	SetEventHandler("DumpDec7", "Dump_Dec7", 0);
	SetEventHandler("DumpAdd8", "Dump_Add8", 0);	SetEventHandler("DumpDec8", "Dump_Dec8", 0);
	SetEventHandler("DumpAdd9", "Dump_Add9", 0);	SetEventHandler("DumpDec9", "Dump_Dec9", 0);

	SetEventHandler("DumpCannons", "DCan_Open", 0);
	SetEventHandler("DCanOk", "DCan_OnOk", 0);
	SetEventHandler("DCanCancel", "DCan_Close", 0);
	SetEventHandler("DCanRemoveAll", "DCan_RemoveAll", 0);
	SetEventHandler("DCanSetAll", "DCan_SetAll", 0);
	SetEventHandler("DCanAdd0", "DCan_Add0", 0);	SetEventHandler("DCanDec0", "DCan_Dec0", 0);
	SetEventHandler("DCanAdd1", "DCan_Add1", 0);	SetEventHandler("DCanDec1", "DCan_Dec1", 0);
	SetEventHandler("DCanAdd2", "DCan_Add2", 0);	SetEventHandler("DCanDec2", "DCan_Dec2", 0);
	SetEventHandler("DCanAdd3", "DCan_Add3", 0);	SetEventHandler("DCanDec3", "DCan_Dec3", 0);

	FreezeGroupControls("BattleInterfaceControls", true);

	iDumpRows = Net_GetGoodsNum() + 1;
	iDumpCannonWeight = 0;
	ref rThis = NetClient_GetThisClient();
	if (CheckAttribute(rThis, "Ship.Cannons.Type"))
	{
		int iCT = sti(rThis.Ship.Cannons.Type);
		if (iCT >= 0) { ref rCan = Net_GetCannonByIndex(iCT); iDumpCannonWeight = sti(rCan.SPWeight); }
	}

	// captions of the rows
	int i;
	for (i=0; i<Net_GetGoodsNum(); i++) { StringCollection_SetText("STRINGS_DUMP_NAME", i + 1, "#" + XI_ConvertString(NetGoods[i].Name)); }
	StringCollection_SetText("STRINGS_DUMP_NAME", Net_GetGoodsNum() + 1, "#Экипаж");

	Button_SetText("BTN_DUMP_CANNONS", "#Пушки");
	Button_SetText("BTN_DCAN_REMOVEALL", "#Снять все");
	Button_SetText("BTN_DCAN_SETALL", "#Поставить все");
	Button_SetText("BTN_DCAN_OK", "#Применить");
	Button_SetText("BTN_DCAN_CANCEL", "#Отмена");

	for (i=0; i<10; i++) { iDumpHave[i] = 0; iDumpThrow[i] = 0; }
	for (i=0; i<4; i++) { iDCanHave[i] = 0; iDCanMax[i] = 0; iDCanSet[i] = 0; }
	bDumpCannonsOpen = false;
	DCan_Show(false);
	Dump_Refresh();

	NetClient_ShipLoadRequest();
}

void Dump_IDoExit(int exitCode)
{
	DelEventHandler("OnCancel", "Dump_ICancel");
	DelEventHandler("exitCancel", "Dump_ICancel");
	DelEventHandler("DumpOk", "Dump_OnOk");
	DelEventHandler("NCEvent_LoadInfo", "Dump_OnLoadInfo");
	DelEventHandler("DumpAdd0", "Dump_Add0");	DelEventHandler("DumpDec0", "Dump_Dec0");
	DelEventHandler("DumpAdd1", "Dump_Add1");	DelEventHandler("DumpDec1", "Dump_Dec1");
	DelEventHandler("DumpAdd2", "Dump_Add2");	DelEventHandler("DumpDec2", "Dump_Dec2");
	DelEventHandler("DumpAdd3", "Dump_Add3");	DelEventHandler("DumpDec3", "Dump_Dec3");
	DelEventHandler("DumpAdd4", "Dump_Add4");	DelEventHandler("DumpDec4", "Dump_Dec4");
	DelEventHandler("DumpAdd5", "Dump_Add5");	DelEventHandler("DumpDec5", "Dump_Dec5");
	DelEventHandler("DumpAdd6", "Dump_Add6");	DelEventHandler("DumpDec6", "Dump_Dec6");
	DelEventHandler("DumpAdd7", "Dump_Add7");	DelEventHandler("DumpDec7", "Dump_Dec7");
	DelEventHandler("DumpAdd8", "Dump_Add8");	DelEventHandler("DumpDec8", "Dump_Dec8");
	DelEventHandler("DumpAdd9", "Dump_Add9");	DelEventHandler("DumpDec9", "Dump_Dec9");

	DelEventHandler("DumpCannons", "DCan_Open");
	DelEventHandler("DCanOk", "DCan_OnOk");
	DelEventHandler("DCanCancel", "DCan_Close");
	DelEventHandler("DCanRemoveAll", "DCan_RemoveAll");
	DelEventHandler("DCanSetAll", "DCan_SetAll");
	DelEventHandler("DCanAdd0", "DCan_Add0");	DelEventHandler("DCanDec0", "DCan_Dec0");
	DelEventHandler("DCanAdd1", "DCan_Add1");	DelEventHandler("DCanDec1", "DCan_Dec1");
	DelEventHandler("DCanAdd2", "DCan_Add2");	DelEventHandler("DCanDec2", "DCan_Dec2");
	DelEventHandler("DCanAdd3", "DCan_Add3");	DelEventHandler("DCanDec3", "DCan_Dec3");

	FreezeGroupControls("BattleInterfaceControls", false);

	interfaceResultCommand = exitCode;
	EndCancelInterface(true);
}

// Esc: the cannon menu is closed first, then the jettison menu
void Dump_ICancel()
{
	if (bDumpCannonsOpen) { DCan_Close(); return; }
	Dump_IDoExit(RC_INTERFACE_NET_DUMP);
}

// the server sent the current load
void Dump_OnLoadInfo()
{
	int i;
	iDumpLoad = sti(NCLoad.Load);
	iDumpCapacity = sti(NCLoad.Capacity);
	if (iDumpCapacity < 1) { iDumpCapacity = 1; }
	for (i=0; i<Net_GetGoodsNum(); i++)
	{
		string sGood = NetGoods[i].Name;
		iDumpHave[i] = sti(NCLoad.Goods.(sGood));
		iDumpThrow[i] = 0;
	}
	iDumpHave[Net_GetGoodsNum()] = sti(NCLoad.Crew) - 1;	// at least one sailor stays
	if (iDumpHave[Net_GetGoodsNum()] < 0) { iDumpHave[Net_GetGoodsNum()] = 0; }
	iDumpThrow[Net_GetGoodsNum()] = 0;

	// cannons on the borts
	iDCanOnBoard = 0;
	iDCanPlaces = 0;
	for (i=0; i<4; i++)
	{
		string sB = Net_BortLetter(i);
		iDCanHave[i] = 0;
		iDCanMax[i] = 0;
		iDCanHave[i] = sti(NCLoad.Bort.(sB).Have);
		iDCanMax[i] = sti(NCLoad.Bort.(sB).Max);
		iDCanOnBoard = iDCanOnBoard + iDCanHave[i];
		iDCanPlaces = iDCanPlaces + iDCanMax[i];
		iDCanSet[i] = iDCanHave[i];
	}
	Trace("DUMP: cannons f=" + iDCanHave[0] + "/" + iDCanMax[0] + " b=" + iDCanHave[1] + "/" + iDCanMax[1] + " l=" + iDCanHave[2] + "/" + iDCanMax[2] + " r=" + iDCanHave[3] + "/" + iDCanMax[3]);
	Dump_Refresh();
	if (bDumpCannonsOpen) { DCan_Refresh(); }
}

int Dump_Step(int iRow)
{
	int iStep = 1;
	if (iRow < Net_GetGoodsNum()) { iStep = sti(NetGoods[iRow].Units) / 10; }
	if (iStep < 1) { iStep = 1; }
	if (XI_IsKeyPressed("shift")) { iStep = iStep * 10; }
	return iStep;
}

void Dump_Change(int iRow, int iDir)
{
	if (bDumpCannonsOpen) { return; }
	if (iRow >= iDumpRows) { return; }
	int iNew = iDumpThrow[iRow] + iDir * Dump_Step(iRow);
	if (iNew < 0) { iNew = 0; }
	if (iNew > iDumpHave[iRow]) { iNew = iDumpHave[iRow]; }
	iDumpThrow[iRow] = iNew;
	Dump_Refresh();
}

void Dump_Refresh()
{
	int i;
	int iNewLoad = iDumpLoad;
	for (i=0; i<iDumpRows; i++)
	{
		StringCollection_SetTextValue("STRINGS_DUMP_HAVE", i + 1, iDumpHave[i]);
		StringCollection_SetTextValue("STRINGS_DUMP_THROW", i + 1, iDumpThrow[i]);
		if (i < Net_GetGoodsNum()) { iNewLoad = iNewLoad - Net_GetGoodWeight(i, iDumpThrow[i]); }
	}
	iNewLoad = iNewLoad - iDumpThrow[Net_GetGoodsNum()];
	if (iNewLoad < 0) { iNewLoad = 0; }
	int iPercent = (iNewLoad * 100) / iDumpCapacity;
	StringCollection_SetText("STRINGS_DUMP_LOAD", 1, "#Грузоподъёмность: " + iNewLoad + " / " + iDumpCapacity + " (" + iPercent + "%)");
	StringCollection_SetText("STRINGS_DUMP_CANINFO", 1, "#Пушки: на борту " + iDCanOnBoard + " из " + iDCanPlaces + " мест, выброшено " + (iDCanPlaces - iDCanOnBoard));
}

void Dump_OnOk()
{
	if (bDumpCannonsOpen) { return; }
	int i;
	int iNothing = 0;
	for (i=0; i<iDumpRows; i++) { iNothing = iNothing + iDumpThrow[i]; }
	if (iNothing <= 0) { return; }

	for (i=0; i<Net_GetGoodsNum(); i++)
	{
		string sGood = NetGoods[i].Name;
		NCJettison.Goods.(sGood) = iDumpThrow[i];
	}
	NetClient_ShipJettison(0, iDumpThrow[Net_GetGoodsNum()]);
}
void Dump_Add0() { Dump_Change(0, 1); }
void Dump_Dec0() { Dump_Change(0, -1); }
void Dump_Add1() { Dump_Change(1, 1); }
void Dump_Dec1() { Dump_Change(1, -1); }
void Dump_Add2() { Dump_Change(2, 1); }
void Dump_Dec2() { Dump_Change(2, -1); }
void Dump_Add3() { Dump_Change(3, 1); }
void Dump_Dec3() { Dump_Change(3, -1); }
void Dump_Add4() { Dump_Change(4, 1); }
void Dump_Dec4() { Dump_Change(4, -1); }
void Dump_Add5() { Dump_Change(5, 1); }
void Dump_Dec5() { Dump_Change(5, -1); }
void Dump_Add6() { Dump_Change(6, 1); }
void Dump_Dec6() { Dump_Change(6, -1); }
void Dump_Add7() { Dump_Change(7, 1); }
void Dump_Dec7() { Dump_Change(7, -1); }
void Dump_Add8() { Dump_Change(8, 1); }
void Dump_Dec8() { Dump_Change(8, -1); }
void Dump_Add9() { Dump_Change(9, 1); }
void Dump_Dec9() { Dump_Change(9, -1); }

// =============================================================================================
// COOP: cannon menu - cannons on the bow, stern, left and right bort
// =============================================================================================
void DCan_Show(bool bShow)
{
	bDumpCannonsOpen = bShow;
	XI_WindowShow("WND_DUMPCANNONS", bShow);
	XI_WindowDisable("WND_DUMPCANNONS", !bShow);
	XI_WindowDisable("WND_DUMP", bShow);
	if (bShow) { SetCurrentNode("BTN_DCAN_CANCEL"); }
	else { SetCurrentNode("BTN_DUMP_CANCEL"); }
}

void DCan_Open()
{
	for (int i=0; i<4; i++) { iDCanSet[i] = iDCanHave[i]; }
	DCan_Show(true);
	DCan_Refresh();
}

void DCan_Close()
{
	DCan_Show(false);
}

int DCan_Placed()
{
	int iSum = 0;
	for (int i=0; i<4; i++) { iSum = iSum + iDCanSet[i]; }
	return iSum;
}

void DCan_Refresh()
{
	int i;
	for (i=0; i<4; i++)
	{
		StringCollection_SetText("STRINGS_DCAN_HAVE", i + 1, "#" + iDCanHave[i]);
		StringCollection_SetText("STRINGS_DCAN_VALUE", i + 1, "#" + iDCanSet[i] + " / " + iDCanMax[i]);
	}
	int iFree = iDCanOnBoard - DCan_Placed();
	StringCollection_SetText("STRINGS_DCAN_INFO", 1, "#Всего на борту: " + iDCanOnBoard + ". Будет стоять: " + DCan_Placed());
	StringCollection_SetText("STRINGS_DCAN_INFO", 2, "#На выброс: " + iFree + " шт. (" + (iFree * iDumpCannonWeight) + " ц)");
}

// arrows of a bort: a cannon is taken from the removed ones / removed from the bort
void DCan_Change(int iBort, int iDir)
{
	int iStep = 1;
	if (XI_IsKeyPressed("shift")) { iStep = 100; }
	int iNew = iDCanSet[iBort] + iDir * iStep;
	if (iNew < 0) { iNew = 0; }
	if (iNew > iDCanMax[iBort]) { iNew = iDCanMax[iBort]; }
	int iFree = iDCanOnBoard - DCan_Placed() + iDCanSet[iBort];		// cannons that may stand on this bort
	if (iNew > iFree) { iNew = iFree; }
	iDCanSet[iBort] = iNew;
	DCan_Refresh();
}

void DCan_RemoveAll()
{
	for (int i=0; i<4; i++) { iDCanSet[i] = 0; }
	DCan_Refresh();
}

// all cannons on board go to the borts (sides first, like a new ship)
void DCan_SetAll()
{
	ref rMe = NetClient_GetThisClient();
	int iType = sti(rMe.Ship.Type);
	for (int i=0; i<4; i++) { iDCanSet[i] = Net_DistributeCannons(iType, iDCanOnBoard, i); }
	DCan_Refresh();
}

void DCan_OnOk()
{
	NetClient_ShipCannonsSet(iDCanSet[0], iDCanSet[1], iDCanSet[2], iDCanSet[3]);
	DCan_Show(false);
}

void DCan_Add0() { DCan_Change(0, 1); }
void DCan_Dec0() { DCan_Change(0, -1); }
void DCan_Add1() { DCan_Change(1, 1); }
void DCan_Dec1() { DCan_Change(1, -1); }
void DCan_Add2() { DCan_Change(2, 1); }
void DCan_Dec2() { DCan_Change(2, -1); }
void DCan_Add3() { DCan_Change(3, 1); }
void DCan_Dec3() { DCan_Change(3, -1); }
