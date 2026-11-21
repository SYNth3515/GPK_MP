#define NUM_PERKS		6
object NetPerks[NUM_PERKS];

// ================================================================================================
// COOP s28: the sea perks are BOUGHT in the lobby, one by one (the table of perks of the buy screen, interfaces\Buy.c).
// The switches "all passive / all active perks" of the Create game screen still give every captain all of them for free.
// The perks a captain has go to the server as a bit mask (the last dword of NSC_BUY_FULLINFO): NSClients[i].SeaPerks,
// and with the load message of the ship to every client: NCClients[i].SeaPerks.
// WindCatcher (+10% acceleration) is a value of the ship type, the same for every ship of that type: it stays with the switch.
// The bots have the perks only with the switches (as before).
// ================================================================================================
#define NET_SPERK_NUM				17
#define NET_SPERK_PASSIVE_NUM		12
#define NET_SPERK_SAILING			0		// SailingProfessional: speed x1.20, turn rate x1.20
#define NET_SPERK_FASTRELOAD		1		// FastReload: reload time x0.9
#define NET_SPERK_LONGRANGE			2		// LongRangeShoot: range x1.15
#define NET_SPERK_CANNONPRO			3		// CannonProfessional: hull damage +30%, +10% critical hits
#define NET_SPERK_CRITICAL			4		// CriticalShoot: +5% critical hits
#define NET_SPERK_CREWDAMAGE		5		// CrewDamageUp: crew damage x1.15
#define NET_SPERK_SAILSDAMAGE		6		// SailsDamageUp: sail damage x1.15
#define NET_SPERK_DEFENCE			7		// ShipDefenseProfessional: hull damage taken -35%, 70% of the critical hits cancelled
#define NET_SPERK_DOCTOR			8		// Doctor2: crew losses x0.8
#define NET_SPERK_STORM				9		// StormProfessional: storm damage x0.7
#define NET_SPERK_CARPENTER			10		// Carpenter: repair x1.1
#define NET_SPERK_BUILDER			11		// Builder: planks / sailcloth x0.9
#define NET_SPERK_ACTIVE0			12		// + NET_APERK_* (ImmediateReload, Turn180, LightRepair, InstantRepair)
#define NET_SPERK_BRANDER			16

string sNetSeaPerkName[17] = {"SailingProfessional", "FastReload", "LongRangeShoot", "CannonProfessional", "CriticalShoot", "CrewDamageUp", "SailsDamageUp", "ShipDefenseProfessional", "Doctor2", "StormProfessional", "Carpenter", "Builder", "ImmediateReload", "Turn180", "LightRepair", "InstantRepair", "Brander"};
// the price of all of them is 42000 (COOP s29, was 68000). The budget: a fast manowar 440000 + crew 32000 + 106 48-pounders
// 63600 + their upgrade 26500 + hull and rigging 33000 + skills 81000 + perks 42000 + 500 of every good 60050 = 778150,
// the dearest land loadout (Land.c, NET_LAND_ITEM_MULT) 108000: 886150 of 1 000 000
int iNetSeaPerkCost[17] = {5000, 3000, 3000, 5000, 1500, 1500, 1500, 5000, 2000, 1000, 1000, 1000, 3000, 2000, 2000, 3000, 1500};

int Net_GetSeaPerkCost(int iPerk)
{
	if (iPerk < 0 || iPerk >= NET_SPERK_NUM) { return 0; }
	return iNetSeaPerkCost[iPerk];
}

// bit iPerk of the mask (the scripts have no bit operators)
bool Net_SeaPerkBit(int iMask, int iPerk)
{
	if (iPerk < 0 || iPerk >= NET_SPERK_NUM) { return false; }
	int iP2 = 1;
	for (int k=0; k<iPerk; k++) { iP2 = iP2 * 2; }
	int iQ = iMask / iP2;
	return (iQ - (iQ / 2) * 2) != 0;
}

int Net_SeaPerkPow2(int iPerk)
{
	int iP2 = 1;
	for (int k=0; k<iPerk; k++) { iP2 = iP2 * 2; }
	return iP2;
}

int Net_GetPerksNum() { return NUM_PERKS; }
ref Net_GetPerkByIndex(int iPerkIndex) { return &NetPerks[iPerkIndex]; }

int Net_GetPerkCost(int iPerkIndex)
{
	return sti(NetPerks[iPerkIndex].Cost);
}

void Net_InitPerks()
{
	NetPerks[0].Name = "TriggerPester";
	NetPerks[0].Cost = 1000;

	NetPerks[1].Name = "Protector";
	NetPerks[1].Cost = 2000;

	NetPerks[2].Name = "HolyShit";
	NetPerks[2].Cost = 3000;

	NetPerks[3].Name = "Flayer";
	NetPerks[3].Cost = 3000;

	NetPerks[4].Name = "CrazyGunner";
	NetPerks[4].Cost = 4000;

	NetPerks[5].Name = "MuscetFire";
	NetPerks[5].Cost = 15000;
}

// ================================================================================================
// COOP: perks of the single player sea battle, switched on for ALL captains on the "Create game" screen
// (NetServer.PassivePerks / NetServer.ActivePerks, the clients get them in the common server info).
//
// Passive perks (every captain has all of them, numbers from sea_ai\AIShip.c, AICannon.c, scripts\ShipsUtilites.c):
//   SailingProfessional (includes ShipSpeedUp / ShipTurnRateUp)	speed x1.20, turn rate x1.20
//   WindCatcher														ship acceleration +10%
//   FastReload														reload time x0.9
//   LongRangeShoot													ball speed (range) x1.15
//   CannonProfessional (includes HullDamageUp)						hull damage dealt +30%, +10% critical hits
//   CriticalShoot														+5% critical hits
//   CrewDamageUp														crew damage dealt x1.15
//   SailsDamageUp														sail damage dealt x1.15
//   ShipDefenseProfessional (includes Basic/AdvancedBattleState)		hull damage taken -35%, 70% of critical hits cancelled
//   Doctor2 (includes Doctor1)										crew losses x0.8
//   StormProfessional													lightning (storm) damage x0.7
//   Carpenter															repair speed of the "Repair" command x1.1
//   Builder															planks / sailcloth for repairs x0.9
//   LongRangeGrappling, GrapplingProfessional, MusketsShoot			were already on for everybody (server\OnShip.c)
//
// Active perks (command menu of the battle interface, cooldown = TimeDelay of single player, from the moment of the use):
//   ImmediateReload, Turn180, LightRepair, InstantRepair  (+ Brander, which needs this switch too)
// (arrays have the literal size 4 = NET_APERK_NUM: Common.h is included before Net_Defines.h)
// ================================================================================================
string	sNetActivePerkName[4] = {"ImmediateReload", "Turn180", "LightRepair", "InstantRepair"};

// seconds, SP TimeDelay (counted from the use of the perk, the duration is inside it)
int Net_GetActivePerkDelay(int iPerk)
{
	switch (iPerk)
	{
		case NET_APERK_IMMEDIATERELOAD:	return 300; break;
		case NET_APERK_TURN180:			return 240; break;
		case NET_APERK_LIGHTREPAIR:		return 240; break;
		case NET_APERK_INSTANTREPAIR:	return 240; break;
	}
	return 240;
}

// seconds, SP TimeDuration
int Net_GetActivePerkDuration(int iPerk)
{
	switch (iPerk)
	{
		case NET_APERK_IMMEDIATERELOAD:	return 60; break;
		case NET_APERK_TURN180:			return 60; break;
		case NET_APERK_LIGHTREPAIR:		return 60; break;
		case NET_APERK_INSTANTREPAIR:	return 30; break;
	}
	return 30;
}

int Net_GetActivePerkIndex(string sName)
{
	for (int i=0; i<4; i++)
	{
		if (sNetActivePerkName[i] == sName) { return i; }
	}
	return -1;
}
