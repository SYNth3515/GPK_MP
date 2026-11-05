object NetGoods[8];
int iNetGoodsNum = 0;

ref Net_GetGoodByName(string sGood) 
{ 
	for (int i=0; i<iNetGoodsNum; i++) { if (sGood == NetGoods[i].Name) return &NetGoods[i]; }
	trace("ERROR! Can't find NetGood " + sGood);
}
ref Net_GetGoodByIndex(int iIndex) { return &NetGoods[iIndex]; }
int Net_GetGoodsNum() { return iNetGoodsNum; }
int Net_GetGoodIndex(string sName) 
{ 
	for (int i=0; i<iNetGoodsNum; i++) 
	{
		if (NetGoods[i].Name == sName) { return i; }
	}
	Trace("ERROR: Can't find NetGood " + sName);
	return -1; 
}

// weight (cwt) of iGoodNum pieces of a good, same as GetGoodWeightByType() in single player
int Net_GetGoodWeight(int iGoodIndex, int iGoodNum)
{
	if (iGoodNum <= 0) { return 0; }
	int iWUnits = sti(NetGoods[iGoodIndex].WUnits);
	if (iWUnits < 1) { iWUnits = 1; }
	int iUnitQuantity = (iGoodNum + iWUnits - 1) / iWUnits;
	return makeint(iUnitQuantity * stf(NetGoods[iGoodIndex].Weight) + 0.05);
}

int Net_GetGoodCost(int iGoodIndex, int iGoodNum)
{
	return (sti(NetGoods[iGoodIndex].Cost) * iGoodNum) / sti(NetGoods[iGoodIndex].Units);
}

void Net_InitGoods()
{
	ref rG;
	int n = 0;

	// BALLS
	makeref(rG, NetGoods[n]); 
	rG.Name = "Balls";
	rG.Index = n;
	rG.Cost = 100;
	rG.Weight = 2;			// weight for WUnits pieces (single player value)
	rG.WUnits = 20;
	rG.Units = 100;
	rG.SpeedV0 = 1.0;
	rG.DamageHull = 8.0;
	rG.DamageRig = 4.0;
	rG.DamageCrew = 0.25;

	// GRAPES
	n++; 
	makeref(rG, NetGoods[n]);
	rG.Name = "Grapes";
	rG.Index = n;
	rG.Cost = 150;
	rG.Weight = 1;
	rG.WUnits = 20;
	rG.Units = 100;
	rG.SpeedV0 = 0.7;
	rG.DamageHull = 1.5;
	rG.DamageRig = 1.5;
	rG.DamageCrew = 2.0;

	// KNIPPELS
	n++; 
	makeref(rG, NetGoods[n]);
	rG.Name = "Knippels";
	rG.Index = n;
	rG.Cost = 200;
	rG.Weight = 2;
	rG.WUnits = 20;
	rG.Units = 100;
	rG.SpeedV0 = 0.9;
	rG.DamageHull = 2.0;
	rG.DamageRig = 17.0;
	rG.DamageCrew = 0.35;

	// BOMBS
	n++; 
	makeref(rG, NetGoods[n]); 
	rG.Name = "Bombs";
	rG.Index = n;
	rG.Cost = 500;
	rG.Weight = 1;
	rG.WUnits = 20;
	rG.Units = 100;
	rG.SpeedV0 = 0.8;
	rG.DamageHull = 17.0;
	rG.DamageRig = 3.5;
	rG.DamageCrew = 0.5;

	// PLANKS
	n++; 
	makeref(rG, NetGoods[n]); 
	rG.Name = "Planks";
	rG.Index = n;
	rG.Cost = 1500;
	rG.Weight = 3;
	rG.WUnits = 1;
	rG.Units = 10;

	// SAILCLOTH
	n++; 
	makeref(rG, NetGoods[n]); 
	rG.Name = "Sailcloth";
	rG.Index = n;
	rG.Cost = 2000;
	rG.Weight = 2;
	rG.WUnits = 1;
	rG.Units = 10;

	// POWDER: 1 piece is used for every cannon shot (like single player: Cannon_LoadBall takes 1 ball + 1 powder),
	// a mine takes NET_MINE_POWDER, a brander explosion is stronger the more powder is in the hold.
	// Always keep it the LAST good: indexes of the other goods are used in the lobby ini.
	n++;
	makeref(rG, NetGoods[n]);
	rG.Name = "Powder";
	rG.Index = n;
	rG.Cost = 60;
	rG.Weight = 1;
	rG.WUnits = 20;
	rG.Units = 100;

	iNetGoodsNum = n + 1;
}