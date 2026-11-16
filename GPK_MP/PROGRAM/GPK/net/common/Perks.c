#define NUM_PERKS		6
object NetPerks[NUM_PERKS];

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
