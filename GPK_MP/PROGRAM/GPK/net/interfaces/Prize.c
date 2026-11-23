// COOP s41: the prize of a boarding (net\Board.c: NCPrize). Like the transfer screen of the single player after a boarding:
// the goods, the crew and the cannons go between our ship (left column) and the prize (right column), the arrows of a row
// move them: left = to us, right = to the prize (shift: x10). Not more than the capacity, the crew of the ship, and the cannons
// only when the two ships have the same ones. Then:
//   "Обменяться" - we take the prize (with its column), our old ship goes down
//   "Захватить"  - the prize sails on as an allied bot (the difficulty of the bots) with its column
//   "Утопить"    - (Esc too) the prize goes down with what is left on it
// Rows 0..goods-1: goods, row goods: crew, row goods + 1: cannons.

int iPzRows = 0;
int iPzG = 0;
int iPzMine[10];
int iPzPrize[10];
int iPzTypeM = -1;
int iPzTypeP = -1;
int iPzCanTM = -1;
int iPzCanTP = -1;
bool bPzCanSame = false;

void InitInterface(string iniName)
{
	GameInterface.title = "";
	SendMessage(&GameInterface, "ls", MSG_INTERFACE_INIT, iniName);

	SetEventHandler("OnCancel", "Prize_ICancel", 0);
	SetEventHandler("exitCancel", "Prize_ICancel", 0);
	SetEventHandler("PrizeSwap", "Prize_OnSwap", 0);
	SetEventHandler("PrizeTake", "Prize_OnTake", 0);
	SetEventHandler("PrizeSink", "Prize_OnSink", 0);
	SetEventHandler("NetPrize_Close", "Prize_OnClose", 0);
	SetEventHandler("PrizeL0", "Prize_L0", 0);	SetEventHandler("PrizeR0", "Prize_R0", 0);
	SetEventHandler("PrizeL1", "Prize_L1", 0);	SetEventHandler("PrizeR1", "Prize_R1", 0);
	SetEventHandler("PrizeL2", "Prize_L2", 0);	SetEventHandler("PrizeR2", "Prize_R2", 0);
	SetEventHandler("PrizeL3", "Prize_L3", 0);	SetEventHandler("PrizeR3", "Prize_R3", 0);
	SetEventHandler("PrizeL4", "Prize_L4", 0);	SetEventHandler("PrizeR4", "Prize_R4", 0);
	SetEventHandler("PrizeL5", "Prize_L5", 0);	SetEventHandler("PrizeR5", "Prize_R5", 0);
	SetEventHandler("PrizeL6", "Prize_L6", 0);	SetEventHandler("PrizeR6", "Prize_R6", 0);
	SetEventHandler("PrizeL7", "Prize_L7", 0);	SetEventHandler("PrizeR7", "Prize_R7", 0);
	SetEventHandler("PrizeL8", "Prize_L8", 0);	SetEventHandler("PrizeR8", "Prize_R8", 0);
	SetEventHandler("PrizeL9", "Prize_L9", 0);	SetEventHandler("PrizeR9", "Prize_R9", 0);

	FreezeGroupControls("BattleInterfaceControls", true);
	bNetPrizeWnd = true;

	int i;
	string sG;
	iPzG = Net_GetGoodsNum();
	if (iPzG > 8) { iPzG = 8; }
	iPzRows = iPzG + 2;
	for (i=0; i<10; i++) { iPzMine[i] = 0; iPzPrize[i] = 0; }
	for (i=0; i<iPzG; i++)
	{
		sG = NetGoods[i].Name;
		iPzMine[i] = sti(NCPrize.my.goods.(sG));
		iPzPrize[i] = sti(NCPrize.goods.(sG));
		StringCollection_SetText("STRINGS_PRIZE_NAME", i + 1, "#" + XI_ConvertString(sG));
	}
	iPzMine[iPzG] = sti(NCPrize.my.crew);
	iPzPrize[iPzG] = sti(NCPrize.crew);
	iPzMine[iPzG + 1] = sti(NCPrize.my.cannons);
	iPzPrize[iPzG + 1] = sti(NCPrize.cannons);

	ref rMe = NetClient_GetThisClient();
	iPzTypeM = sti(rMe.Ship.Type);
	iPzCanTM = sti(rMe.Ship.Cannons.Type);
	iPzTypeP = sti(NCPrize.type);
	iPzCanTP = sti(NCPrize.cantype);
	bPzCanSame = false;
	if (iPzCanTM == iPzCanTP) { bPzCanSame = true; }

	StringCollection_SetText("STRINGS_PRIZE_NAME", iPzG + 1, "#Команда");
	StringCollection_SetText("STRINGS_PRIZE_NAME", iPzG + 2, "#Пушки, калибр " + Prize_Caliber(iPzCanTM) + " / " + Prize_Caliber(iPzCanTP));

	ref rShipM = Net_GetShipByIndex(iPzTypeM);
	ref rShipP = Net_GetShipByIndex(iPzTypeP);
	string sPrizeNick = "";
	int wP = sti(NCPrize.id);
	if (wP >= 0 && wP < NET_MAXCLIENTS) { if (CheckAttribute(&NCClients[wP], "NickName")) { sPrizeNick = NCClients[wP].NickName; } }
	StringCollection_SetText("STRINGS_PRIZE_CAPTION", 1, "#Добыча: " + XI_ConvertString(rShipP.Name) + " (" + sPrizeNick + ")");
	StringCollection_SetText("STRINGS_PRIZE_CAPTION", 2, "#Ваш: " + XI_ConvertString(rShipM.Name));
	StringCollection_SetText("STRINGS_PRIZE_CAPTION", 3, "#Приз: " + XI_ConvertString(rShipP.Name));

	Button_SetText("BTN_PRIZE_SWAP", "#Обменяться");
	Button_SetText("BTN_PRIZE_TAKE", "#Захватить");
	Button_SetText("BTN_PRIZE_SINK", "#Утопить");

	Prize_Refresh();
	for (i=iPzRows; i<10; i++) { SetNodeUsing("SBAR_PRIZE_" + i, false); }		// COOP s44: no arrows under an empty row
	SetCurrentNode("BTN_PRIZE_SINK");
}

void Prize_IDoExit(int exitCode)
{
	DelEventHandler("OnCancel", "Prize_ICancel");
	DelEventHandler("exitCancel", "Prize_ICancel");
	DelEventHandler("PrizeSwap", "Prize_OnSwap");
	DelEventHandler("PrizeTake", "Prize_OnTake");
	DelEventHandler("PrizeSink", "Prize_OnSink");
	DelEventHandler("NetPrize_Close", "Prize_OnClose");
	DelEventHandler("PrizeL0", "Prize_L0");	DelEventHandler("PrizeR0", "Prize_R0");
	DelEventHandler("PrizeL1", "Prize_L1");	DelEventHandler("PrizeR1", "Prize_R1");
	DelEventHandler("PrizeL2", "Prize_L2");	DelEventHandler("PrizeR2", "Prize_R2");
	DelEventHandler("PrizeL3", "Prize_L3");	DelEventHandler("PrizeR3", "Prize_R3");
	DelEventHandler("PrizeL4", "Prize_L4");	DelEventHandler("PrizeR4", "Prize_R4");
	DelEventHandler("PrizeL5", "Prize_L5");	DelEventHandler("PrizeR5", "Prize_R5");
	DelEventHandler("PrizeL6", "Prize_L6");	DelEventHandler("PrizeR6", "Prize_R6");
	DelEventHandler("PrizeL7", "Prize_L7");	DelEventHandler("PrizeR7", "Prize_R7");
	DelEventHandler("PrizeL8", "Prize_L8");	DelEventHandler("PrizeR8", "Prize_R8");
	DelEventHandler("PrizeL9", "Prize_L9");	DelEventHandler("PrizeR9", "Prize_R9");

	FreezeGroupControls("BattleInterfaceControls", false);
	bNetPrizeWnd = false;

	interfaceResultCommand = exitCode;
	EndCancelInterface(true);
}

int Prize_Caliber(int iCanType)
{
	if (iCanType < 0 || iCanType >= Net_GetCannonsNum()) { return 0; }
	ref rCan = Net_GetCannonByIndex(iCanType);
	return sti(rCan.caliber);
}

int Prize_CanWeight(int iCanType)
{
	if (iCanType < 0 || iCanType >= Net_GetCannonsNum()) { return 0; }
	ref rCan = Net_GetCannonByIndex(iCanType);
	return sti(rCan.SPWeight);
}

// the load of a column (bMine: our ship), the rule of Net_GetShipLoadWeight
int Prize_Load(bool bMine)
{
	int iLoad = 0;
	for (int i=0; i<iPzG; i++)
	{
		if (bMine) { iLoad = iLoad + Net_GetGoodWeight(i, iPzMine[i]); }
		else { iLoad = iLoad + Net_GetGoodWeight(i, iPzPrize[i]); }
	}
	if (bMine) { return iLoad + iPzMine[iPzG] + iPzMine[iPzG + 1] * Prize_CanWeight(iPzCanTM); }
	return iLoad + iPzPrize[iPzG] + iPzPrize[iPzG + 1] * Prize_CanWeight(iPzCanTP);
}

int Prize_Step(int iRow)
{
	int iStep = 1;
	if (iRow < iPzG) { iStep = sti(NetGoods[iRow].Units) / 10; }
	if (iStep < 1) { iStep = 1; }
	if (XI_IsKeyPressed("shift")) { iStep = iStep * 10; }
	return iStep;
}

// can bMine (our ship) take this row as it is now (crew / cannon places, the capacity when it got more)
bool Prize_Fits(int iRow, bool bMine, int iAdded)
{
	int iType = iPzTypeP;
	if (bMine) { iType = iPzTypeM; }
	int iVal = iPzPrize[iRow];
	if (bMine) { iVal = iPzMine[iRow]; }
	if (iRow == iPzG && iVal > Net_GetCrewCount(iType, 100)) { return false; }
	if (iRow == iPzG + 1 && iVal > Net_GetShipCannonsNum(iType)) { return false; }
	if (iAdded <= 0) { return true; }
	return Prize_Load(bMine) <= Net_GetShipCapacity(iType);
}

// iDir -1: to our ship, +1: to the prize
void Prize_Move(int iRow, int iDir)
{
	if (iRow >= iPzRows) { return; }
	if (iRow == iPzG + 1 && !bPzCanSame)
	{
		Log_SetStringToLog("Пушки другого калибра не перегрузить");
		return;
	}
	int iA = Prize_Step(iRow);
	if (iDir < 0) { if (iA > iPzPrize[iRow]) { iA = iPzPrize[iRow]; } }
	else { if (iA > iPzMine[iRow]) { iA = iPzMine[iRow]; } }
	bool bToMine = false;
	if (iDir < 0) { bToMine = true; }
	while (iA > 0)
	{
		iPzMine[iRow] = iPzMine[iRow] - iDir * iA;
		iPzPrize[iRow] = iPzPrize[iRow] + iDir * iA;
		if (Prize_Fits(iRow, bToMine, iA)) { break; }
		iPzMine[iRow] = iPzMine[iRow] + iDir * iA;
		iPzPrize[iRow] = iPzPrize[iRow] - iDir * iA;
		int iLess = iA / 4;
		if (iLess < 1) { iLess = 1; }
		iA = iA - iLess;
	}
	if (iA <= 0) { Log_SetStringToLog("Не помещается"); }
	Prize_Refresh();
}

void Prize_Refresh()
{
	for (int i=0; i<iPzRows; i++)
	{
		StringCollection_SetText("STRINGS_PRIZE_MINE", i + 1, "#" + iPzMine[i]);
		StringCollection_SetText("STRINGS_PRIZE_PRIZE", i + 1, "#" + iPzPrize[i]);
	}
	int iCapM = Net_GetShipCapacity(iPzTypeM);
	int iCapP = Net_GetShipCapacity(iPzTypeP);
	StringCollection_SetText("STRINGS_PRIZE_LOAD", 1, "#Груз " + Prize_Load(true) + " / " + iCapM + ", команда до " + Net_GetCrewCount(iPzTypeM, 100) + ", пушек до " + Net_GetShipCannonsNum(iPzTypeM));
	StringCollection_SetText("STRINGS_PRIZE_LOAD", 2, "#Груз " + Prize_Load(false) + " / " + iCapP + ", команда до " + Net_GetCrewCount(iPzTypeP, 100) + ", пушек до " + Net_GetShipCannonsNum(iPzTypeP));
	int iLeft = 0;
	if (CheckAttribute(&NCPrize, "until")) { iLeft = (sti(NCPrize.until) - iClientServerTime) / 1000; }
	if (iLeft < 0) { iLeft = 0; }
	StringCollection_SetText("STRINGS_PRIZE_INFO", 1, "#Обменяться: ваш корабль утонет, вы перейдёте на приз. Захватить: приз уйдёт к боту-союзнику.");
	StringCollection_SetText("STRINGS_PRIZE_INFO", 2, "#Утопить (Esc): приз пойдёт ко дну. Время на решение: около " + iLeft + " сек.");
}

void Prize_SendChoice(int iAct)
{
	for (int i=0; i<iPzG; i++)
	{
		string sG = NetGoods[i].Name;
		NCPrize.send.goods.(sG) = iPzMine[i];
	}
	NetPrize_Send(iAct, iPzMine[iPzG], iPzMine[iPzG + 1]);
	Prize_IDoExit(RC_INTERFACE_NET_DUMP);
}

void Prize_OnSwap()
{
	if (iPzPrize[iPzG] < 1) { Log_SetStringToLog("Переведите на приз команду"); return; }
	Prize_SendChoice(1);
}

void Prize_OnTake()
{
	if (iPzPrize[iPzG] < 1) { Log_SetStringToLog("Переведите на приз команду"); return; }
	if (iPzMine[iPzG] < 1) { Log_SetStringToLog("Оставьте команду на своём корабле"); return; }
	Prize_SendChoice(2);
}

void Prize_OnSink()
{
	if (iPzMine[iPzG] < 1) { Log_SetStringToLog("Оставьте команду на своём корабле"); return; }
	Prize_SendChoice(0);
}

void Prize_ICancel()
{
	// Esc: the prize goes down; all the crew comes back to our ship first
	iPzMine[iPzG] = iPzMine[iPzG] + iPzPrize[iPzG];
	iPzPrize[iPzG] = 0;
	if (iPzMine[iPzG] > Net_GetCrewCount(iPzTypeM, 100)) { iPzMine[iPzG] = Net_GetCrewCount(iPzTypeM, 100); }
	Prize_SendChoice(0);
}

// the server: the time is over
void Prize_OnClose()
{
	Prize_IDoExit(RC_INTERFACE_NET_DUMP);
}

void Prize_L0() { Prize_Move(0, -1); }
void Prize_R0() { Prize_Move(0, 1); }
void Prize_L1() { Prize_Move(1, -1); }
void Prize_R1() { Prize_Move(1, 1); }
void Prize_L2() { Prize_Move(2, -1); }
void Prize_R2() { Prize_Move(2, 1); }
void Prize_L3() { Prize_Move(3, -1); }
void Prize_R3() { Prize_Move(3, 1); }
void Prize_L4() { Prize_Move(4, -1); }
void Prize_R4() { Prize_Move(4, 1); }
void Prize_L5() { Prize_Move(5, -1); }
void Prize_R5() { Prize_Move(5, 1); }
void Prize_L6() { Prize_Move(6, -1); }
void Prize_R6() { Prize_Move(6, 1); }
void Prize_L7() { Prize_Move(7, -1); }
void Prize_R7() { Prize_Move(7, 1); }
// COOP s44: the bar N moves the row N (the net has 7 goods: the crew is the row 7, the cannons the row 8; the bar 8 moved the
// crew - "iPzG" - and the bar 9 of the cannons stood under an empty row)
void Prize_L8() { Prize_Move(8, -1); }
void Prize_R8() { Prize_Move(8, 1); }
void Prize_L9() { Prize_Move(9, -1); }
void Prize_R9() { Prize_Move(9, 1); }
