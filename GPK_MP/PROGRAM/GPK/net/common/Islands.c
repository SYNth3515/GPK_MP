int iNumNetIslands = 0;
int iNetIslandsIndex[MAX_ISLANDS];

ref Net_GetIsland(string sIslandID)
{
	return GetIslandByID(sIslandID);
}
ref Net_GetIslandByIndex(int iIndex) { return GetIslandByIndex(iNetIslandsIndex[iIndex]); }

int Net_GetIslandsNum() { return iNumNetIslands; }
int Net_GetIslandIndex(string sIslandID)
{
	return FindIsland(sIslandID);
}

bool Net_IsIslandHaveForts(ref rIsland)
{
	aref arReload; makearef(arReload, rIsland.reload);
	int iNumReloads = GetAttributesNum(arReload);
	for (int i=0; i<iNumReloads; i++)
	{
		aref arLocator = GetAttributeN(arReload, i);
		if (CheckAttribute(arLocator, "fort")) { return true; }
	}

	return false;
}

int Net_IslandGetNumForts(ref rIsland)
{
	int iNumForts = 0;
	aref arReload; makearef(arReload, rIsland.reload);
	int iNumReloads = GetAttributesNum(arReload);
	for (int i=0; i<iNumReloads; i++)
	{
		aref arLocator = GetAttributeN(arReload, i);
		if (CheckAttribute(arLocator, "fort")) { iNumForts++; }
	}

	return iNumForts;
}

// Названия островов для мультиплеера (в Корсарах 3 строк для островов Мэйна нет)
string Net_GetIslandName(string sID)
{
	if (sID == "Barbados") { return "Барбадос"; }
	if (sID == "Jamaica") { return "Ямайка"; }
	if (sID == "PuertoRico") { return "Пуэрто-Рико"; }
	if (sID == "Cuba1") { return "Куба (Сантьяго)"; }
	if (sID == "Cuba2") { return "Куба (Гавана)"; }
	if (sID == "Curacao") { return "Кюрасао"; }
	if (sID == "Tortuga") { return "Тортуга"; }
	if (sID == "SentMartin") { return "Сен-Мартен"; }
	if (sID == "Trinidad") { return "Тринидад"; }
	if (sID == "Nevis") { return "Невис"; }
	if (sID == "Antigua") { return "Антигуа"; }
	if (sID == "Guadeloupe") { return "Гваделупа"; }
	if (sID == "Martinique") { return "Мартиника"; }
	if (sID == "Hispaniola1") { return "Эспаньола (Санто-Доминго)"; }
	if (sID == "Hispaniola2") { return "Эспаньола (Порт-о-Пренс)"; }
	if (sID == "Bermudes") { return "Бермуды"; }
	if (sID == "Dominica") { return "Доминика"; }
	if (sID == "Terks") { return "Тёркс"; }
	if (sID == "Caiman") { return "Кайман"; }
	if (sID == "PortoBello") { return "Порто-Белло"; }
	if (sID == "Cartahena") { return "Картахена"; }
	if (sID == "Maracaibo") { return "Маракайбо"; }
	if (sID == "Caracas") { return "Каракас"; }
	if (sID == "Cumana") { return "Кумана"; }
	if (sID == "SantaCatalina") { return "Санта-Каталина"; }
	if (sID == "Beliz") { return "Белиз"; }
	if (sID == "LostShipsCity") { return "Город Потерянных Кораблей"; }

	// запасной вариант: строка из локализации, если острова нет в списке выше
	string sName = XI_ConvertString(sID);
	if (sName == "") { sName = sID; }
	return sName;
}

// false = остров не показывать в мультиплеере (нет локаторов/картинки/режимов)
bool Net_IsIslandAllowed(string sID)
{
	if (sID == "") { return false; }
	return true;
}

void Net_InitIslands()
{
	if (iNumIslands == 0 || iNumNetIslands == 0)
	{
		IslandsInit();

		// iNumIslands = индекс последнего острова (n в islands_init.c), а не их количество, поэтому <=
		for (int i=0; i<=iNumIslands; i++)
		{
			ref rIsland = GetIslandByIndex(i);

			if (sti(rIsland.Net) && Net_IsIslandAllowed(rIsland.ID))
			{
				iNetIslandsIndex[iNumNetIslands] = i;
				iNumNetIslands++;
			}
		}
	}
}