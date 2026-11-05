#ifndef PROGRAM_NET_HPP
#define PROGRAM_NET_HPP

#define NET_DEFAULT_SERVER_PORT			29020

#define NETSHIP_MAST_TOUCH_BALL			0
#define NETSHIP_MAST_TOUCH_ISLAND		1
#define NETSHIP_MAST_TOUCH_SHIP			2

#define NETFILE_FACE					0
#define NETFILE_FLAG					1
#define NETFILE_SAIL					2

#define NETKILL_BY_BALL					0
#define	NETKILL_BY_ISLAND				1
#define NETKILL_BY_TOUCH_DEAD			2
#define NETKILL_BY_TOUCH_ALIVE			3
#define NETKILL_BY_LIGHTNING			4
#define NETKILL_BY_BOARDING				5
#define NETKILL_BY_MINE					6		// COOP: blown up by a mine of another player
#define NETKILL_BY_OWN_MINE				7		// COOP: blown up by the own mine
#define NETKILL_BY_BRANDER				8		// COOP: destroyed by the brander of another player
#define NETKILL_BY_OWN_BRANDER			9		// COOP: the own brander exploded (the ship of the brander owner)
#define NETKILL_BY_FORT					10		// COOP: sunk by the fort

// auto boarding (player vs player, no land fight). Rules are taken from single player:
//   availability: Ship_CheckMainCharacter (distance minus half ship widths, angle, relative speed, grappling skill and perks)
//   result:       Ship_AutoAbordage (crew * grappling strength, winner loses loserPower / divider sailors)
#define NET_BOARD_MIN_CREW_RATIO		0.0		// attacker crew must be >= this part of target crew (0.0 = boarding is allowed at any odds)
#define NET_BOARD_RAND_MIN				0.01	// random bonus to the strength of each side in the boarding fight: from 1%
#define NET_BOARD_RAND_MAX				0.05	// ... to 5% (so with almost equal crews the winner and the losses are random)
#define NET_BOARD_LOSS_DIVIDER			1.8		// winner loses crew * loserPower / (winnerPower * this)
#define NET_BOARD_STATE_TICKS			4		// board availability is checked every N-th ship update (4 * 120 ms)
#define NET_SOUND_ABORDAGE				"Interface\\abordage.wav"
#define NET_SOUND_MUSKETSHOT			"Interface\\_musketshot.wav"
#define NET_SOUND_PLAYER_LEFT			"Interface\\box_locked.wav"		// COOP: chat line "player disconnected"
#define NET_SOUND_PLAYER_JOIN			"Interface\\new_level.wav"		// COOP: chat line "player connected" (reconnect too)
#define NET_SOUND_CHAT					"Interface\\podtverjd_001.wav"	// COOP: a chat message arrived

#define NETSKILL_MAX					10.0

#define MSG_NET_CREATE_SERVER			10000
#define MSG_NET_CREATE_CLIENT			10001
#define MSG_NET_ADD_FORT				10002

// =================================================================================
// Network game types
// =================================================================================
#define NETGAME_DEATHMATCH				0
#define NETGAME_NAVAL					1
#define NETGAME_CONVOY					2
#define NETGAME_FORT					3

// =================================================================================
// Destination address
// =================================================================================
#define DST_INVALID						65534
#define DST_ALL							65535

// =================================================================================
// NET MESSAGES CODES 
// =================================================================================
#define NC_CONNECT						0
#define NC_DISCONNECT					1
#define NC_STARTGAME					2
#define NC_SAY							3
#define NC_BALL							4
#define NC_COMMAND						5
#define NC_SHIP							6
#define NC_FORT							7
#define NC_PING							8
#define NC_WEATHER						9
#define NC_CLIENT						10
#define NC_CAMERA						11
//#define NC_BROADCAST					12		== changed, because some changes implemented to broadcast message
#define NC_MASTERSERVER					13
#define NC_FILE							14
#define NC_BUY							15
#define NC_SERVERINFO					16
#define NC_PLAYERINFO					17
#define NC_STATISTICS					18
#define NC_GARANTED						19
#define NC_GAMEINFO						20
#define NC_BROADCAST					21

// =================================================================================
// SUBCODES
// =================================================================================
// NC_FILE
#define NSC_FILE_DATA					0
#define NSC_FILE_DATA_ACK				1
#define NSC_FILE_GET_FILE				2
#define NSC_FILE_TRANSFER_READY			3
#define NSC_FILE_RECEIVER_READY			4
#define NSC_FILE_CANCEL					5

// NC_CONNECT
#define NSC_CONNECT_REQUEST							0
#define NSC_CONNECT_ACCEPTED						1
#define NSC_CONNECT_REJECTED						2
#define NSC_CONNECT_REJECTED_NEED_CLIENT_PASSWORD	3
#define NSC_CONNECT_REJECTED_SERVER_FULL			4
#define NSC_CONNECT_NEED_PASSWORD					5
#define NSC_CONNECT_BANNED							6
#define NSC_CONNECT_RECONNECT						7
#define NSC_CONNECT_DISCONNECT						8
#define NSC_CONNECT_REJECTED_DIFFERENT_VERSION		9

// NC_CLIENT
#define NSC_CLIENT_CONNECTED			0
#define NSC_CLIENT_LOAD					1
#define NSC_CLIENT_DROP					2

// NC_WEATHER
#define NSC_WEATHER_ADD_LIGHTNING		0
#define NSC_WEATHER_WIND				1

// NC_COMMAND	
#define NSC_COMMAND_STARTGAME			0
#define NSC_COMMAND_RESTARTGAME			1
#define NSC_COMMAND_VOTE_KICK			2
// COOP: admin system. Client -> server:
#define NSC_COMMAND_ADMIN_LOGIN         3       // string password
#define NSC_COMMAND_ADMIN_CMD               4       // byte action (NET_ADMIN_*), clientID target, float value
// COOP: server -> client:
#define NSC_COMMAND_ADMIN_STATUS        10      // byte: 1 = you are an admin, 0 = refused
#define NSC_COMMAND_ADMIN_FLAGS         11      // clientID, byte admin, byte chat muted (sent to all)
#define NSC_COMMAND_ADMIN_ENV           12      // byte game speed, float wind angle, float wind speed, string weather id, byte time idx, byte weather idx
#define NSC_COMMAND_SET_WEATHER         13      // string weather id, float wind angle, float wind speed (the same data as in the start message)
#define NSC_COMMAND_SET_WIND            14      // float wind angle, float wind speed
#define NSC_COMMAND_RELOAD_SHIPS        15      // ships were recreated on the server

// COOP: admin actions (NSC_COMMAND_ADMIN_CMD)
#define NET_ADMIN_SYNC                  0       // send me the flags of all players and the environment
#define NET_ADMIN_KICK                  1
#define NET_ADMIN_MUTE                  2       // toggle chat of the player
#define NET_ADMIN_TP_TO                 3       // my ship to the target ship
#define NET_ADMIN_TP_BRING              4       // target ship to my ship
#define NET_ADMIN_MAKE_ADMIN            5
#define NET_ADMIN_RELOAD_SHIPS          6
#define NET_ADMIN_HEAL                  7
#define NET_ADMIN_ALL_HEAL              8
#define NET_ADMIN_DESTROY               9
#define NET_ADMIN_WEATHER               10      // value = +1 / -1
#define NET_ADMIN_TIME                  11      // value = +1 / -1
#define NET_ADMIN_SPEED                 12      // value = +1 / -1
#define NET_ADMIN_WIND_ANGLE            13      // value = change in radians
#define NET_ADMIN_WIND_SPEED            14      // value = change

// NC_STARTGAME
#define NSC_STARTGAME_GETBUYINFO		0
#define NSC_STARTGAME_ENVIRONMENT		1
#define NSC_STARTGAME_GAMEOVER			2

// NC_PING
#define NSC_PING						0
#define NSC_PONG						1

// NC_SAY
#define NSC_SAY_CHAT					0
#define NSC_SAY_TEAM					1
#define NSC_SAY_PERSON					2

// NC_BALL
#define NSC_BALL_ADD					0
#define NSC_BALL_DEL					1
#define NSC_BALL_HIT_WATER 				2
#define NSC_BALL_HIT_SAIL 				3
#define NSC_BALL_HIT_HULL 				4
#define NSC_BALL_HIT_MAST 				5
#define NSC_BALL_HIT_FORT 				6
#define NSC_BALL_HIT_ISLAND				7

// NC_SHIP
#define NSC_SHIP_MAST_FALL				0
#define NSC_SHIP_DEAD					1
#define NSC_SHIP_TURNLEFT_ON			2
#define NSC_SHIP_TURNLEFT_OFF			3
#define NSC_SHIP_TURNRIGHT_ON			4
#define NSC_SHIP_TURNRIGHT_OFF			5
#define NSC_SHIP_SAILUP					6
#define NSC_SHIP_SAILDOWN				7
#define NSC_SHIP_SETSAILSTATE			8
#define NSC_SHIP_FIRE					9
#define NSC_SHIP_INFO					10
#define NSC_SHIP_SERIOUS_BOOM			11
#define NSC_SHIP_ACTIVATE_FIREPLACE		12
#define NSC_SHIP_CHANGE_CHARGE			13
#define NSC_SHIP_BORTS_INFO				14
#define NSC_SHIP_SAIL_STATE				15
#define NSC_SHIP_START_REPAIR			16
#define NSC_SHIP_SHIP_COLLISION			17
#define NSC_SHIP_ISLAND_COLLISION		18
#define NSC_SHIP_BOARD_REQUEST			19
#define NSC_SHIP_BOARD_STATE			20		// server -> client: which enemy ship can be boarded right now
#define NSC_SHIP_BOARD_RESULT			22		// server -> all: crew lost on both ships when the boarding is over
// ---- ship load / powder / mines / brander (server/OnLoad.c) ----
#define NET_TURN_WEIGHT_MULT		1.0		// 1.0 - arcade (SP iArcadeSails = 1), 2.0 - simulation (SP iArcadeSails = 0)
#define NET_MINE_POWDER				200		// powder for one mine (MINE_POWDER)
#define NET_MINE_DAMAGE				600.0	// hull damage of a mine (powder1 in SP)
#define NET_MINE_LIFETIME			600000	// 600 s
#define NET_MINE_ARM_TIME			6000	// own mine does not hurt the owner for this time (ms)
#define NET_MINE_COOLDOWN			3000	// ms between two mines of a ship
#define NET_BRANDER_DELAY			800		// ms before the explosion (Ship_SetExplosion)
#define NET_BRANDER_MIN_POWDER		50
#define NET_POWDER_DETONATE_RATIO		0.25	// SP (boal, AIShip.c): a critical hit blows the powder up if its weight > 25% of the capacity...
#define NET_POWDER_DETONATE_CHANCE		50		// ...with this chance, %
#define NET_BRANDER_CREW_DAMAGE		0.6		// part of the crew killed by the brander blast at the epicenter (less with distance)
#define NET_CREW_COST					50		// price of one hired sailor (the crew on your own ship is paid from the credit)
#define NET_BRANDER_ARM_TIME			30000	// ms: after the first press the brander is armed and goes off if the ship is boarded

#define NET_MINE_MAX					32
#define NSC_SHIP_DROP_MINE				23		// client -> server: drop a mine behind the stern
#define NSC_SHIP_BRANDER				24		// client -> server: blow up own ship (brander)
#define NSC_SHIP_JETTISON				25		// client -> server: throw goods / cannons / crew overboard
#define NSC_SHIP_LOAD_REQUEST			26		// client -> server: send ship load info
#define NSC_SHIP_LOAD_INFO				27		// server -> client: ship load, capacity, crew, cannons, goods
#define NSC_SHIP_MINE_ADD				28		// server -> all: a mine appeared
#define NSC_SHIP_MINE_DEL				29		// server -> all: a mine disappeared (time is over)
#define NSC_SHIP_MINE_BOOM				30		// server -> all: a mine exploded
#define NSC_SHIP_BRANDER_BOOM			31		// server -> all: brander explosion
#define NSC_SHIP_ACTION_DENIED			32		// server -> client: mine / brander refused (byte reason, word have, word need)
#define NET_DENY_MINE_COOLDOWN			1		// have = seconds left
#define NET_DENY_POWDER					2		// have = powder in the hold, need = powder required
#define NET_DENY_MINE_LIMIT				3
#define NSC_SHIP_BRANDER_ARM			33		// client -> server: first press of the brander (armed for NET_BRANDER_ARM_TIME ms)
#define NSC_SHIP_SPLASH					34		// server -> all: splash of the cargo / mine thrown into the water (3D sound at x, y, z)
#define NET_SOUND_SPLASH				"net_water_plash"	// alias in sound_alias.ini (Ships\\jakor_002.wav, the same file as in single player)
#define NSC_SHIP_BOARD_VOLLEY			21		// server -> both captains: musket volley before the boarding (plays the sound)

// ---- respawn (CS-like deathmatch): the ship is "wrecked" for NET_RESPAWN_DELAY ms, then comes back with the same loadout ----
// The ship entity of the clients is never deleted / sunk (deleting a ship on a client crashed the game, see client/Time.c):
// the wrecked ship stays afloat, burns and does not move, then the server puts it to a spawn point.
#define NSC_SHIP_WRECKED				35		// server -> in game clients: wDead, byte kill status, wKiller, byte detonate, byte seconds to respawn
#define NSC_SHIP_RESPAWN				36		// server -> in game clients: wID, float maxhp, word crew, goods..., byte charge, float x, z, ay
#define NET_RESPAWN_DELAY				30000	// ms between the death and the respawn: default of the slider on the Create game screen
#define NET_RESPAWN_TIME_MIN			10		// s, slider of the Create game screen
#define NET_RESPAWN_TIME_MAX			180		// s
#define NET_RESPAWN_PROTECT				3000	// ms of no hull damage after the respawn

// ---- active perks of single player (sea): the "All active perks" switch of the Create game screen ----
#define NSC_SHIP_PERK_USE				37		// client -> server: byte perk index (NET_APERK_*)
#define NSC_SHIP_PERK_STATE				38		// server -> owner: byte perk, dword active until (server time), dword ready at (server time)
#define NSC_SHIP_PERK_DENIED			39		// server -> owner: byte perk, word seconds left
#define NSC_SHIP_CANNONS_SET			40		// client -> server: byte cannons on the bow, stern, left, right; removed and not placed ones go overboard
#define NET_BORT_NUM					4		// cannon borts: 0 bow (fcannon), 1 stern (bcannon), 2 left (lcannon), 3 right (rcannon)
#define NET_APERK_NUM					4
#define NET_APERK_IMMEDIATERELOAD		0		// "Предварительная перезарядка": reload time x0.5 (SP: TimeDelay 300, TimeDuration 60)
#define NET_APERK_TURN180				1		// "Маневренный разворот": turn rate x4 (SP: 240 / 60)
#define NET_APERK_LIGHTREPAIR			2		// "Легкий ремонт": +1% hull and sails per second up to 10%, no materials (SP: 240 / 60)
#define NET_APERK_INSTANTREPAIR			3		// "Быстрая починка": +5% per second up to 65%, planks / sailcloth (SP: 240 / 30)

// ---- reconnect / late join ----
#define NET_INGAME_PING_TIMEOUT			45000	// ms without pings while the game runs = the client is gone (its ship is parked for a reconnect)

// NC_FORT
#define NSC_FORT_CANNON_DESTROY			0
#define NSC_FORT_INFO					1		// COOP: server -> all: cannons total / destroyed and crew of the fort (spyglass)

// NC_CAMERA
#define NSC_DECK_CAMERA_UPDATE			0

// NC_BROADCAST
#define NSC_BROADCAST_LOCAL				0
#define NSC_BROADCAST_INET				1
#define NSC_BROADCAST_FAVORITE			2

// NC_MASTERSERVER
#define NSC_MASTERSERVER_CONNECT		0
#define NSC_MASTERSERVER_GETSERVERS		1
#define NSC_MASTERSERVER_UPDATESERVER	2
#define NSC_MASTERSERVER_ADDSERVER		3
#define NSC_MASTERSERVER_DELSERVER		4
#define NSC_MASTERSERVER_DISCONNECT		5
#define NSC_MASTERSERVER_PUNCH			6	// P2P: client -> master: "I connect to this server"; master -> host: address of that client

// NC_BUY
#define NSC_BUY_OK						0
#define NSC_BUY_FAILED					1
#define NSC_BUY_READY					2
#define	NSC_BUY_TEAMCHANGE				3
#define	NSC_BUY_SHIPCHANGE				4
#define NSC_BUY_FULLINFO				5

// NC_SERVERINFO
#define NSC_SERVERINFO_GET				0
#define	NSC_SERVERINFO_COMMON			1
#define NSC_SERVERINFO_PLAYER			2
#define NSC_SERVERINFO_SHIP				3
#define NSC_SERVERINFO_CANNONS			4
#define NSC_SERVERINFO_SKILLS			5
#define NSC_SERVERINFO_PERKS			6
#define NSC_SERVERINFO_GOODS			7
#define NSC_SERVERINFO_TOPLIST_PLAYER	8

// NC_PLAYERINFO
#define NSC_PLAYERINFO_SHIPCHANGE		0
#define NSC_PLAYERINFO_TEAMCHANGE		1
#define NSC_PLAYERINFO_BUYREADY			2
#define NSC_PLAYERINFO_STARTREADY		3
#define NSC_PLAYERINFO_STARTCOUNTDOWN	4
#define NSC_PLAYERINFO_STOPCOUNTDOWN	5

// NC_STATISTICS
#define NSC_STATISTICS_TOTAL_PLAYER		0
#endif