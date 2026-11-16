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
#define NET_SCRIPT_VERSION		104

string sUserFacesPath = "net\faces\";
string sUserSailsPath = "net\sails\";
string sUserFlagsPath = "net\flags\";

string	sNetBortNames[4] = {"cannonl", "cannonr", "cannonf", "cannonb"};
object	NetModes[4];
int		NetTeamColor[4];
string	NetTeamName[4];
