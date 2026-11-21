// ================================================================================================
// COOP s31: the right click (as in the single player screens: interface\character.c, ship.c) shows a description of the
// item, skill, perk, ship, cannon or good under the mouse. The nodes of the INI have "rclick,select:NODE,event:NetInfoShow";
// the interfaces (net\interfaces\Buy.c, NetMenu.c) take the event and call NetInfo_Land / their own code.
// The tooltip goes away when the right button is released (MouseRClickUp), interface\utilite.c CreateTooltip / CloseTooltip.
// ================================================================================================

void NetInfo_Tip(string sHeader, string sText1, string sText2, string sText3, string sGroup, string sImage)
{
	string sPicture = "-1";
	string sImg = sImage;
	if (sGroup == "") { sImg = ""; }
	CreateTooltip("#" + sHeader, sText1, argb(255,255,255,255), sText2, argb(255,255,192,192), sText3, argb(255,192,255,192), "", argb(255,255,255,255), sPicture, sGroup, sImg, 64, 64);
}

void NetInfo_Hide()
{
	CloseTooltip();
}

// the row (1..) of a table that the click chose
int NetInfo_Row(string sTable)
{
	if (!CheckAttribute(&GameInterface, sTable + ".select")) { return 0; }
	return sti(GameInterface.(sTable).select);
}

// an item of the single player (blade, gun, cuirass, healing): its description as in the item screens
void NetInfo_Item(string sId, string sNone, string sExtra)
{
	if (sId == "")
	{
		NetInfo_Tip(sNone, "Ничего не выбрано: стрелки по бокам меняют предмет.", "", "", "", "");
		return;
	}
	int iIdx = GetItemIndex(sId);
	if (iIdx < 0)
	{
		NetInfo_Tip(sId, "", "", "", "", "");
		return;
	}
	string sHead = GetConvertStr(Items[iIdx].name, "ItemsDescribe.txt");
	if (sHead == "") { sHead = sId; }
	string sText = GetItemDescribe(iIdx);
	NetInfo_Tip(sHead, sText, "", sExtra, Items[iIdx].picTexture, "itm" + Items[iIdx].picIndex);
}

void NetInfo_Rpg(string sName, string sHint)
{
	NetInfo_Tip(XI_ConvertString(sName), GetRPGText(sName), "", sHint, "", "");
}

void NetInfo_Perk(string sPerk, string sHint)
{
	string sHead = GetConvertStr(sPerk, "AbilityDescribe.txt");
	if (sHead == "") { sHead = sPerk; }
	NetInfo_Tip(sHead, GetConvertStr("perk" + sPerk, "AbilityDescribe.txt"), "", sHint, "", "");
}

// the land duel loadout (the window WND_LOADOUT of NetMenu.ini and of Buy.ini): true = the node is one of it
bool NetInfo_Land(string sNode)
{
	int iRow;
	aref lo;
	makearef(lo, oNetLandLo);
	switch (sNode)
	{
		case "PIC_LO_BLADE":
			NetInfo_Item(NetLand_LoBladeId(), "Клинок", "Цена в снаряжении: " + NetLand_ItemPrice(NetLand_LoBladeId()));
			return true;
		break;
		case "PIC_LO_GUN":
			NetInfo_Item(NetLand_LoGunId(), "Пистолет", "Цена в снаряжении: " + NetLand_ItemPrice(NetLand_LoGunId()));
			return true;
		break;
		case "PIC_LO_ARMOR":
			NetInfo_Item(NetLand_LoArmor(lo), "Кираса", NetLand_LoArmorText());
			return true;
		break;
		case "TABLE_LO_POT":
			iRow = NetInfo_Row(sNode);
			if (iRow < 1 || iRow > NET_LAND_POTS) { return true; }
			NetInfo_Item(NetLand_PotId(iRow - 1), "Лечение", "Цена одной штуки: " + NetLand_ItemPrice(NetLand_PotId(iRow - 1)));
			return true;
		break;
		case "TABLE_LO_SKILL":
			iRow = NetInfo_Row(sNode);
			if (iRow < 1 || iRow > 5) { return true; }
			NetInfo_Rpg(NetLand_SkillName(iRow - 1), "Стрелки в строке меняют навык (за деньги снаряжения).");
			return true;
		break;
		case "TABLE_LO_SPECIAL":
			iRow = NetInfo_Row(sNode);
			if (iRow < 1 || iRow > 7) { return true; }
			NetInfo_Rpg(NetLand_SpecialName(NetLand_UiLoSpecAt(iRow - 1)), "Стрелки в строке меняют характеристику (за деньги снаряжения).");
			return true;
		break;
		case "TABLE_LO_PERKS":
			iRow = NetInfo_Row(sNode);
			if (iRow < 1 || iRow > 13) { return true; }
			NetInfo_Perk(NetLand_PerkName(iRow - 1), "Двойной щелчок по строке берёт или убирает умение.");
			return true;
		break;
	}
	return false;
}

// ---------- the sea: the buy screen (net\interfaces\Buy.c gives its choice) ----------
string NetInfo_Pct(float fV)
{
	if (fV > 0.0) { return "+" + FloatToString(fV, 1) + "%"; }
	return FloatToString(fV, 1) + "%";
}

void NetInfo_Ship(int iShip)
{
	if (iShip < 0 || iShip >= Net_GetShipsNum()) { return; }
	ref rShip = Net_GetShipByIndex(iShip);
	string sName = rShip.Name;
	string sHead = XI_ConvertString(sName);
	if (sHead == "") { sHead = sName; }
	string sText2 = "Класс: " + sti(rShip.Class);
	sText2 = sText2 + NewStr() + "Корпус: " + sti(rShip.HP) + ", паруса: " + sti(rShip.SP);
	sText2 = sText2 + NewStr() + "Скорость: " + FloatToString(stf(rShip.SpeedRate), 1) + ", манёвренность: " + FloatToString(stf(rShip.TurnRate), 1);
	sText2 = sText2 + NewStr() + "Орудий: " + sti(rShip.CannonsQuantity) + ", калибр до " + sti(rShip.MaxCaliber);
	sText2 = sText2 + NewStr() + "Команда: " + sti(rShip.MaxCrew) + ", трюм: " + Net_GetShipCapacity(iShip) + " ц.";
	string sText3 = "Цена: " + Net_GetShipCost(iShip) + ". Стрелки по бокам меняют корабль.";
	NetInfo_Tip(sHead, GetConvertStr(sName, "ShipsDescribe.txt"), sText2, sText3, "SHIPS_" + sName, "ship");
}

void NetInfo_HullUpg(int iShip, int iCur)
{
	if (iShip < 0) { return; }
	string sText = "Обшивка корпуса: больше прочности, но корабль тяжелее и медленнее.";
	string sText2 = "";
	for (int k=0; k<3; k++)
	{
		if (k > 0) { sText2 = sText2 + NewStr(); }
		string sMark = "";
		if (k == iCur) { sMark = " <- выбрано"; }
		sText2 = sText2 + "Уровень " + (k + 1) + ": корпус " + NetInfo_Pct(stf(NetShipHullUpgrades[k].hp)) + ", скорость " + NetInfo_Pct(stf(NetShipHullUpgrades[k].speedrate)) + ", поворот " + NetInfo_Pct(stf(NetShipHullUpgrades[k].turnrate)) + ", цена " + Net_GetShipUpgradeHullCost(iShip, k) + sMark;
	}
	NetInfo_Tip("Улучшение корпуса", sText, sText2, "Стрелки меняют уровень.", "HULL_UPGRADE_" + (iCur + 1), "hull");
}

void NetInfo_RigUpg(int iShip, int iCur)
{
	if (iShip < 0) { return; }
	string sText = "Паруса: быстрее и поворотливее, но рвутся сильнее (урон по своим парусам больше).";
	string sText2 = "";
	for (int k=0; k<3; k++)
	{
		if (k > 0) { sText2 = sText2 + NewStr(); }
		string sMark = "";
		if (k == iCur) { sMark = " <- выбрано"; }
		sText2 = sText2 + "Уровень " + (k + 1) + ": скорость " + NetInfo_Pct(stf(NetShipSailUpgrade[k].speedrate)) + ", поворот " + NetInfo_Pct(stf(NetShipSailUpgrade[k].turnrate)) + ", урон парусам " + NetInfo_Pct(stf(NetShipSailUpgrade[k].dmgtosails)) + ", цена " + Net_GetShipUpgradeRiggingCost(iShip, k) + sMark;
	}
	if (iCur == 3) { sText2 = sText2 + NewStr() + "Выбраны паруса Летучего Голландца: только внешний вид, без прибавок."; }
	NetInfo_Tip("Улучшение парусов", sText, sText2, "Стрелки меняют уровень.", "", "");
}

void NetInfo_Cannon(int iCannon, int iNum)
{
	if (iCannon < 0 || iCannon >= Net_GetCannonsNum())
	{
		NetInfo_Tip("Орудия", "На этом корабле нет орудий.", "", "", "", "");
		return;
	}
	ref rC = Net_GetCannonByIndex(iCannon);
	string sHead = XI_ConvertString(rC.Name) + " " + XI_ConvertString("caliber" + rC.caliber);
	string sText2 = "Дальность: " + sti(rC.FireRange);
	sText2 = sText2 + NewStr() + "Урон: x" + FloatToString(stf(rC.DamageMultiply), 2);
	sText2 = sText2 + NewStr() + "Перезарядка: " + sti(rC.ReloadTime) + " сек.";
	sText2 = sText2 + NewStr() + "Вес одного орудия: " + sti(rC.Weight) + " ц.";
	string sText3 = "Цена одного орудия: " + Net_GetCannonCost(iCannon) + ", на корабле " + iNum + " шт.: " + (iNum * Net_GetCannonCost(iCannon)) + ". Стрелки меняют орудия.";
	NetInfo_Tip(sHead, GetConvertStr("CannonType", "ShipsDescribe.txt"), sText2, sText3, "CANNONS_UPGRADE_1", rC.picture);
}

void NetInfo_CannonUpg(int iCannon, int iNum, int iCur)
{
	if (iCannon < 0) { return; }
	ref rC = Net_GetCannonByIndex(iCannon);
	string sText2 = "";
	for (int k=0; k<3; k++)
	{
		if (k > 0) { sText2 = sText2 + NewStr(); }
		string sMark = "";
		if (k == iCur) { sMark = " <- выбрано"; }
		string sMetal = XI_ConvertString("radioNetBuy_Copper");
		if (k == 1) { sMetal = XI_ConvertString("radioNetBuy_Bronze"); }
		if (k == 2) { sMetal = XI_ConvertString("radioNetBuy_Iron"); }
		sText2 = sText2 + sMetal + ": " + (iNum * Net_GetCannonUpgradeCost(iCannon, k)) + sMark;
	}
	NetInfo_Tip("Металл орудий", "Металл, из которого отлиты орудия. Цена за все орудия корабля:", sText2, "", "CANNONS_UPGRADE_" + (iCur + 1), rC.picture);
}

// iSkill: 0 Accuracy, 1 Cannons, 2 Sailing, 3 Defence, 4 Repair, 5 Grappling (common\Skills.c)
void NetInfo_SeaSkill(int iSkill, int iValue)
{
	if (iSkill < 0 || iSkill >= Net_GetSkillsNum()) { return; }
	ref rS = Net_GetSkillByIndex(iSkill);
	string sName = rS.Name;
	string sText3 = "Сейчас: " + iValue + ". Цена очка: " + sti(rS.Cost) + " (первое очко бесплатно).";
	NetInfo_Tip(XI_ConvertString(sName), GetRPGText(sName), "", sText3, "", "");
}

void NetInfo_Good(string sGood, int iQty)
{
	int iG = Net_GetGoodIndex(sGood);
	if (iG < 0) { return; }
	string sHead = GetConvertStr(sGood, "GoodsDescribe.txt");
	if (sHead == "") { sHead = sGood; }
	string sText = GetAssembledString(GetConvertStr(sGood + "_descr", "GoodsDescribe.txt"), &NetGoods[iG]);
	string sText3 = "Сейчас: " + iQty + ". Цена " + sti(NetGoods[iG].Units) + " шт.: " + sti(NetGoods[iG].Cost) + ", вес: " + Net_GetGoodWeight(iG, iQty) + " ц.";
	NetInfo_Tip(sHead, sText, "", sText3, "GOODS", sGood);
}

void NetInfo_Crew(int iShip, int iPercent)
{
	string sText = "Матросы управляют парусами, работают у орудий и идут на абордаж. Количество задаётся в процентах от полной команды корабля.";
	string sText3 = "";
	if (iShip >= 0) { sText3 = "Сейчас: " + Net_GetCrewCount(iShip, iPercent) + " из " + sti(NetShips[iShip].MaxCrew) + " (" + iPercent + "%)."; }
	NetInfo_Tip("Команда", sText, "", sText3, "", "");
}

void NetInfo_SeaPerk(int iPerk, string sHint)
{
	if (iPerk < 0 || iPerk >= NET_SPERK_NUM) { return; }
	NetInfo_Perk(sNetSeaPerkName[iPerk], sHint);
}
