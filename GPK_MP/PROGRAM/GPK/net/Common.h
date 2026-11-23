#include "GPK\Net\Common\Cannons.c"
#include "GPK\Net\Common\Goods.c"
#include "GPK\Net\Common\Islands.c"
#include "GPK\Net\Common\Perks.c"
#include "GPK\Net\Common\Ships.c"
#include "GPK\Net\Common\Skills.c"
#include "GPK\Net\Common\Rewards.c"
#include "GPK\Net\Common\Upgrades.c"

#define NET_DEBUG				0
//#define NET_DEBUG				1
#define CONVOY_POINT_RADIUS		40.0

#define NET_TEST_DWORD			12345678		// hack
#define NET_SCRIPT_VERSION		114		// COOP s46 114: the health, the energy and the anger of the NPC copies; 113: 32 players, the bots in the slots 32..81; s45 111: a bot captain can have an NPC skin (BotFace 2001+); s44 110: the armor in the info of a land bot (s41 109: bot portraits, the bots board players on the deck, the prize of a boarding (s39: 108)

string sUserFacesPath = "net\faces\";
string sUserSailsPath = "net\sails\";
string sUserFlagsPath = "net\flags\";

string	sNetBortNames[4] = {"cannonl", "cannonr", "cannonf", "cannonb"};
object	NetModes[4];
int		NetTeamColor[4];
string	NetTeamName[4];
