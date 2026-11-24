#ifndef PROGRAM_NET_HPP
#define PROGRAM_NET_HPP

#define NET_DEFAULT_SERVER_PORT			29020
#define NET_LAN_SCAN_PORTS				20		// COOP s47: the LAN search asks the ports 29020 .. 29039 (a server with another port is found)

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
#define NETKILL_BY_TORNADO				11		// COOP: destroyed by the tornado

// auto boarding (player vs player, no land fight). Rules are taken from single player:
//   availability: Ship_CheckMainCharacter (distance minus half ship widths, angle, relative speed, grappling skill and perks)
//   result:       Ship_AutoAbordage (crew * grappling strength, winner loses loserPower / divider sailors)
#define NET_LAND_NPC_FACE			2000	// COOP s45: LandBotFace / BotFace above this = an NPC skin npc_N (N = face - this, NetLand_NpcModel in net\Land.c)
#define NET_BOARD_DIST_MULT			1.5		// COOP s45: the boarding distance of the single player (15..40 m between the hulls by the grappling) times this: with the distance of the single player the ships of the net were in reach for a moment only
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
#define MSG_NET_SHIP_SETPOS				50450	// COOP s27 (patch of net.dll): "lffff" x, y, z, ay: the server ship goes there at once, level, speed 0 (sea noclip)
#define NET_ADMIN_NOCLIP				24		// COOP s27 (NSC_COMMAND_ADMIN_CMD): the sea noclip of the admin: value = -1 off, else keys (1 W, 2 S, 4 A, 8 D, 16 Q, 32 E, 64 Space, 128 Shift) + 256 * step in cm

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
#define NC_LAND							22		// COOP: land duel (Land.c)

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
// COOP s47: the maps of RESOURCE\Downloads (net\NetDl.c); the subcodes from 20 have no file ids after them
#define NSC_DL_LISTREQ					20		// client -> server: the key of the map
#define NSC_DL_LIST						21		// server -> client: the files of the map (path, size, crc)
#define NSC_DL_REQ						22		// client -> server: the file i
#define NSC_DL_DATA						23		// server -> client: the chunk k of the file i (not guaranteed)
#define NSC_DL_ACK						24		// client -> server: the next chunk expected (not guaranteed)
#define NSC_DL_NOMAP					25		// server -> client: no such map

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
#define NSC_CONNECT_TOLOBBY							10	// COOP s30: the game is over, back to the lobby (server\OnStart.c: NetServer_ReturnToLobby)

// NC_LAND (COOP: land duel, Land.c)
// client -> server
#define NSC_LAND_STATE			0		// float x, y, z, ay; byte flags (1 fight mode, 2 dead, 4 blocks, 8 runs, 16 W is down, 32 S is down, 64 the animation is sent by NSC_LAND_ANIM, 128 the fight states are mirrored); word hp; word hp max; float vx, vz (m/s); word energy, word energy max
#define NSC_LAND_ACT			1		// byte action (NET_LANDACT_* + 16 * the variant of the animation 1..4), byte packet number (the packet is sent twice)
#define NSC_LAND_HIT			2		// clientID victim, word damage, word damage if blocked, word what the attacker saw at the touch (1 block, 2 dodge, 3 feint, +4 a break blow, +8 HardHitter, +16 * the variant 1..3 of the hit reaction, +64 the perk Sliding: a blow into the block does its full damage; the victim applies the hit at once), byte kind (0 blade, 1 shot), byte sequence number (the packet goes twice, not guaranteed)
#define NSC_LAND_DEAD			3		// clientID killer (DST_INVALID - none), byte variant of the death animation 1..4 (death_0..3, the sword out), 5..6 (death_citizen_1..2, no sword)
#define NSC_LAND_ANIM			6		// string action of player 0, string action of player 1, string action of player 2 of the animation of the hero (Location.dll "GetAnim"), byte state and byte variant of the fight state machine of the hero ("GetFightState"); byte clientID + 1 of the attacker of the last hit that has reached the hero (0 none) and byte its sequence number (the attacker keeps the hit reaction of the puppet until then)
#define NSC_LAND_HITRES			5		// clientID attacker, byte NET_LANDRES_* (the victim tells what became of the hit)
#define NSC_LAND_LOADOUT		4		// string blade, string gun, byte level, 7 bytes PIRATES, 5 bytes skills, 13 bytes perks
// server -> client
#define NSC_LAND_START			10		// string location, byte spawn index, byte respawn, byte respawn seconds, byte no pistol, string weather, dword ms to the end of the match (0 = no limit; team duel only)
#define NSC_LAND_STATE_R		11		// clientID + the data of NSC_LAND_STATE
#define NSC_LAND_ACT_R			12		// clientID, byte action, byte packet number
#define NSC_LAND_HIT_R			13		// clientID attacker, word damage, word damage if blocked, word what the attacker saw, byte kind, byte sequence number (only to the victim)
#define NSC_LAND_DEAD_R			14		// clientID dead, clientID killer, word kills of the killer, byte variant of the death animation
#define NSC_LAND_RESPAWN_R		15		// clientID, byte spawn index
#define NSC_LAND_ANIM_R			18		// clientID + the data of NSC_LAND_ANIM
#define NSC_LAND_HITRES_R		17		// clientID victim, byte NET_LANDRES_* (only to the attacker)
#define NSC_LAND_ADMIN_R		19		// byte NET_LANDADM_*, clientID (the other player / DST_INVALID), byte value (the admin menu in a land duel, only to the player it concerns)
#define NET_LANDADM_HEAL		1		// the player gets his health back
#define NET_LANDADM_KILL		2		// the player dies
#define NET_LANDADM_TPTO		3		// the player goes to the player of the clientID
#define NET_LANDADM_BOTADD		4		// a bot (value = NET_BOTREL_*) comes into the location (on every machine)
#define NET_LANDADM_BOTCLEAR	5		// all the bots go
#define NSC_LAND_DMG			23		// clientID victim (the sender or his bot), clientID attacker, word damage, word hp, word hp max
#define NSC_LAND_DMG_R			24		// the data of NSC_LAND_DMG (the number above the victim)
#define NSC_LAND_STAT_R			25		// clientID, word kills, word deaths, float damage done, byte dead (F1)
#define NSC_LAND_BOTEVT			27		// COOP s28: the owner of a bot -> server: clientID bot, clientID the other (DST_INVALID none), byte NET_LANDBOTEVT_*
#define NET_LANDBOTEVT_HIT		1		// our hero / our bot has hit our neutral bot: it turns into an enemy
#define NET_LANDBOTEVT_KILLED	2		// the bot has been killed by the other (team duel: the score of the teams)
#define NSC_LAND_TEAMSCORE_R	26		// COOP: team duel: word kills of team 0, word kills of team 1 (kills of one team over the other)
#define NSC_LAND_NPCLIST		28		// COOP s35: the host -> server: the NPC of the location (net\LandNpc.c)
#define NSC_LAND_NPCLIST_R		29
#define NSC_LAND_NPCSTATE		30		// COOP s35: the host -> server: the places of the NPC
#define NSC_LAND_NPCSTATE_R		31
#define NSC_LAND_BOARD_START	32		// COOP s37: server -> captain: the real boarding begins (net\Board.c)
#define NSC_LAND_BOARD_END		33		// COOP s37: server -> captain: byte won
#define NSC_LAND_NPCDMG			34		// COOP s38: a player -> server -> the host: word gen, byte k, word damage, byte kind (net\LandNpc.c)
#define NSC_LAND_NPCHIT_R		35		// COOP s38: the host -> a player: word damage, word damage if blocked, word view, byte kind
#define NSC_LAND_BOTSTATE		7		// clientID bot (of the sender), float x, y, z, ay, byte flags (1 fight, 2 dead), word hp, word hp max, byte death variant, string action
#define NSC_LAND_BOTHIT			8		// clientID attacker (the sender or his bot), clientID bot, word damage, word damage if blocked, byte kind (to the owner of the bot)
#define NSC_LAND_BOTHITP		9		// clientID bot (of the sender), clientID victim player, word damage, word damage if blocked, byte kind, byte sequence
#define NSC_LAND_BOTSTATE_R		20		// the data of NSC_LAND_BOTSTATE
#define NSC_LAND_BOTHIT_R		21		// the data of NSC_LAND_BOTHIT (only to the owner of the bot)
#define NSC_LAND_BOTINFO_R		22		// clientID bot, clientID owner, byte relation, byte removed, word face, string blade, string name, byte level, string gun, string armor (s44)
#define NET_LANDADM_BOTSKILL	7		// to all: value = the difficulty of the bots 0..3
#define NET_LANDADM_SPEED		8		// to all: value = the game speed * 4
#define NET_LANDADM_BOTCMD		6		// to the owner of the bot (clientID): value NET_LANDBOT_* + 16 * the admin
#define NSC_LAND_LOADOUT_R		16		// clientID + the data of NSC_LAND_LOADOUT

#define NET_LANDACT_FAST		1
#define NET_LANDACT_FORCE		2
#define NET_LANDACT_ROUND		3
#define NET_LANDACT_BREAK		4
#define NET_LANDACT_FEINT		5
#define NET_LANDACT_SHOT		6
#define NET_LANDACT_PARRY		7		// Shift + RMB (ChrParry)
#define NET_LANDACT_FEINTC		8
#define NET_LANDACT_RECOIL		9		// S in the fight: the hop back
#define NET_LANDACT_STRAFEL		10		// A in the fight: the hop to the left
#define NET_LANDACT_STRAFER		11		// D in the fight: the hop to the right

#define NET_LANDRES_BLOCK		1		// the hit was blocked
#define NET_LANDRES_PARRY		2		// the hit was parried
#define NET_LANDRES_MISS		3		// the victim was out of reach
#define NET_LANDRES_HIT			4		// the hit landed
#define NET_LANDRES_FEINT		5		// the hit went into a feint: the victim strikes back, the attacker takes hit_feint

// NC_CLIENT
#define NSC_CLIENT_CONNECTED			0
#define NSC_CLIENT_LOAD					1
#define NSC_CLIENT_DROP					2
#define NSC_CLIENT_CHOICE				4		// COOP s30: clientID, string: the choice of the lobby of that player (NSC_BUY_CHOICE)
#define NSC_CLIENT_TOLOBBY				5		// COOP s31: clientID: that player went to the lobby, the game goes on (his puppet goes away)
#define NSC_CLIENT_BOT					3		// COOP: bots of the admin: byte op (NET_BOTOP_*), then the data of the op

// NC_WEATHER
#define NSC_WEATHER_ADD_LIGHTNING		0
#define NSC_WEATHER_WIND				1
#define NSC_WEATHER_TORNADO				2		// COOP: byte active, float x, z, vx, vz (server -> client, tornado of the Tornado weather)

// NC_COMMAND	
#define NSC_COMMAND_STARTGAME			0
#define NSC_COMMAND_RESTARTGAME			1
#define NSC_COMMAND_VOTE_KICK			2
// COOP: admin system. Client -> server:
#define NSC_COMMAND_ADMIN_LOGIN         3       // string password
#define NSC_COMMAND_ADMIN_CMD               4       // byte action (NET_ADMIN_*), clientID target, float value
#define NSC_COMMAND_ADMIN_BOTADD            5       // COOP: word ship type, byte cannon type, byte crew %, byte hull %, byte sails %, byte relation
// COOP: server -> client:
#define NSC_COMMAND_ADMIN_STATUS        10      // byte: 1 = you are an admin, 0 = refused
#define NSC_COMMAND_ADMIN_FLAGS         11      // clientID, byte admin, byte chat muted (sent to all)
#define NSC_COMMAND_ADMIN_ENV           12      // byte game speed, float wind angle, float wind speed, string weather id, byte time idx, byte weather idx
#define NSC_COMMAND_SET_WEATHER         13      // string weather id, float wind angle, float wind speed (the same data as in the start message)
#define NSC_COMMAND_SET_WIND            14      // float wind angle, float wind speed
#define NSC_COMMAND_RELOAD_SHIPS        15      // ships were recreated on the server
#define NSC_COMMAND_NOCLIP_SHIP         16      // COOP s27: server -> all: clientID, byte on, float y, float vy (m/s): the ship is in the sea noclip (no waves / collisions on the clients)

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
// COOP: bots (target = client ID of the bot)
#define NET_ADMIN_BOT_REL               15      // value = NET_BOTREL_*
#define NET_ADMIN_BOT_BRING             16      // the bot to my ship
#define NET_ADMIN_BOT_GOTO              17      // my ship to the bot
#define NET_ADMIN_BOT_HEAL              18
#define NET_ADMIN_BOT_DESTROY           19
#define NET_ADMIN_BOT_DELETE            20
#define NET_ADMIN_BOT_SKILL		22		// COOP: value = the difficulty of the bots 0..3 (all modes)
#define NET_ADMIN_BOT_RESPAWN           21      // value = 1 / 0: the bots come back after a death like the players
#define NET_ADMIN_LATEJOIN              25      // COOP s30: value = 1 / 0: new players can join the running game
#define NET_ADMIN_TOLOBBY               26      // COOP s30: the game ends, everybody goes back to the lobby
#define NET_ADMIN_LBOT_DEL              27      // COOP s30: value = the number of a bot of the lobby (0..) to remove
#define NET_ADMIN_GOD                   23      // COOP s27: toggle the immortality of the player (sea: no hull / crew damage, no death; land: no death)

// COOP: relation of a bot to the players
#define NET_BOTREL_ALLY                 1       // fights the enemy bots, follows the admin who made it an ally
#define NET_BOTREL_ENEMY                2       // fights the players and the allied bots
#define NET_BOTREL_NEUTRAL              3       // does not fire, sails around (COOP s28: turns into an enemy when it is attacked)
#define NET_BOTREL_TEAM0                4       // COOP s28: a bot of a team (NET_BOTREL_TEAM0 + team 0..3) of the team games (team deathmatch,
                                                // capture the fort, convoy, the land duel of 2 teams): it fights like a player of that team,
                                                // counts for the victory of its team and its kills go to its team

// COOP: NSC_CLIENT_BOT operations
#define NET_BOTOP_INFO                  1       // clientID, byte number, byte relation, word ship type, string nick, byte announce
#define NET_BOTOP_REMOVED               2       // clientID, float sink speed x, y, z
#define NET_BOTOP_NOTE                  3       // byte note (NET_BOTNOTE_*), byte value (to the admin)
#define NET_BOTOP_OPTS                  4       // byte respawn of the bots (0 / 1)
#define NET_BOTOP_LOBBY                 5       // COOP s30: the bots of the lobby: byte n, then n x (byte rel, word ship, byte cannon, crew, hull, sails)

#define NET_BOTNOTE_NOSHIP              1
#define NET_BOTNOTE_LIMIT               2
#define NET_BOTNOTE_NOGAME              3
#define NET_BOTNOTE_NOFORT              4       // the island has no fort
#define NET_BOTNOTE_FORTEXISTS          5       // the fort of this game already stands
#define NET_BOTNOTE_FORTADDED           6       // byte value = relation (NET_BOTREL_*)
#define NET_BOTNOTE_FORTFIXED           7       // the fort of the game mode cannot be moved
#define NET_BOTNOTE_FORTMOVEOFF         8       // "fort to the admin" is switched off (bNetFortMove in server\Fort.c)
#define NET_BOTNOTE_LANDFULL            10      // COOP s46: byte value = the characters in the location of the land duel (NET_LAND_CHARS_MAX)
#define NET_LAND_CHARS_MAX              60      // COOP s46: characters of one location at most (the engine stops at 64)
#define NET_BOTNOTE_FORTGONE            9       // the fort of Bot+ was removed in this game: it cannot be built again (bNetFortMove in server\Fort.c)

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
#define NSC_SAY_SYSTEM					3	// COOP s30: a line of the server (no sender)

// NC_BALL
#define NSC_BALL_ADD					0
#define NSC_BALL_DEL					1
#define NSC_BALL_HIT_WATER 				2
#define NSC_BALL_HIT_SAIL 				3
#define NSC_BALL_HIT_HULL 				4
#define NSC_BALL_HIT_MAST 				5
#define NSC_BALL_HIT_FORT 				6
#define NSC_BALL_HIT_ISLAND				7
#define NSC_BALL_SPEED					8		// COOP: float, new SpeedMultiply of the balls (the admin changed the game speed)

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
#define NET_CREW_COST					40		// COOP s29: 50 -> 40. price of one hired sailor (the crew on your own ship is paid from the credit)
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
// COOP s27: the ship entity of a client is deleted the safe way now (the way of "Delete" of the admin menu: crew, battle
// interface, sounds, DeleteClass, then the foam is made again: client\OnClient.c NetClient_ShipEntityDelete). The wrecked
// ship sinks (NET_WRECK_SINK of server\OnShip.c) and is created again at the respawn.
#define NSC_SHIP_WRECKED				35		// server -> in game clients: wDead, byte kill status, wKiller, byte detonate, byte seconds to respawn, byte sink, float sink speed x, y, z
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
#define NSC_SHIP_GONE					41		// COOP s27: server -> in game clients: wID; the sunk ship is deleted (some time after its death)
#define NSC_SHIP_REBUILD				42		// COOP s27: server -> in game clients: wID, float x, z, ay; the ship is deleted and created again (the masts after a repair)
#define NSC_SHIP_PRIZE					43		// COOP s41: server -> the winner of a boarding: the prize and the two loads (net\Board.c)
#define NSC_SHIP_PRIZE_DONE				44		// COOP s41: winner -> server: the choice (sink / exchange / capture) and our column
#define NSC_SHIP_PRIZE_SWAP				45		// COOP s41: server -> in game clients: a ship has the type of its prize now
#define NSC_SHIP_PRIZE_END				46		// COOP s41: server -> the winner: the choice is over (the window closes)
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
#define NSC_FORT_INFO					1		// COOP: server -> all: word cannons total, word destroyed, word crew of the fort (spyglass), byte relation (0 = fort of the game mode, else NET_BOTREL_*)
#define NET_FORT_DROP					-1.0		// COOP: the fort of Bot+ is set this much lower (m)
#define NET_FORT_TRACE_SHIFT			1			// COOP: 1 = net.dll shifts the rays of the balls to the model of the fort (the server model stays where it was loaded),
												// 0 = the server model is moved by MSG_MODEL_SET_POSITION like the client model
#define NET_FORT_COLL_R					40.0		// 50 COOP: ships cannot come closer than this to the middle of a fort (the push starts here)
#define NET_FORT_COLL_PUSH				60.0		// COOP: the push (Ship.Impulse.Speed, an acceleration) at the middle of the fort, 0 at the edge of NET_FORT_COLL_R
#define NET_FORT_COLL_K					20.0			// COOP: the push against the speed of the ship towards the fort: acceleration = K * that speed (1/s)
#define NET_FORT_COLL_MAX				300.0		// COOP: the push is never stronger than this
#define NSC_FORT_REMOVE					3		// COOP: server -> all: the fort of Bot+ is removed (Bots table: Delete / Destroy, or before it is built again)
#define NSC_FORT_SPAWN					2		// COOP: server -> all: the admin added the fort in a running game (string 16: locator, string 24: island whose fort model is used, float dx, float dz: shift of the fort from that locator)

// NC_CAMERA
#define NSC_DECK_CAMERA_UPDATE			0

// NC_BROADCAST
#define NSC_BROADCAST_LOCAL				0
#define NSC_BROADCAST_INET				1
#define NSC_BROADCAST_FAVORITE			2
#define NSC_BROADCAST_DETAILS			3		// COOP s27: client -> a server of the list: its parameters (answer: NC_BROADCAST, 3, tick, string Net_DetailsCode)

// NC_MASTERSERVER
#define NSC_MASTERSERVER_CONNECT		0
#define NSC_MASTERSERVER_GETSERVERS		1
#define NSC_MASTERSERVER_UPDATESERVER	2
#define NSC_MASTERSERVER_ADDSERVER		3
#define NSC_MASTERSERVER_DELSERVER		4
#define NSC_MASTERSERVER_DISCONNECT		5
#define NSC_MASTERSERVER_PUNCH			6	// P2P: client -> master: "I connect to this server"; master -> host: address of that client
#define NSC_MASTERSERVER_ADDRESS		7	// COOP s47d: master -> host, the answer to ADD / UPDATE: [ip4][port BE] the public address of this server

// NC_BUY
#define NSC_BUY_OK						0
#define NSC_BUY_FAILED					1
#define NSC_BUY_READY					2
#define	NSC_BUY_TEAMCHANGE				3
#define	NSC_BUY_SHIPCHANGE				4
#define NSC_BUY_FULLINFO				5
#define NSC_BUY_RESPAWN					7	// COOP s46: client -> server: the loadout of NSC_BUY_FULLINFO for the next respawn (Esc menu "Корабль")
#define NSC_BUY_CHOICE					6	// COOP s30: client -> server -> all: the choice of the lobby as a string (interfaces\Buy.c: Buy_ChoiceCode)

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
#define NSC_SERVERINFO_CHANGED			10		// COOP s30: server -> clients: the game settings changed in the lobby (read the info again)
#define NSC_SERVERINFO_DETAILS			9		// COOP s27: client <-> the server of the lobby: its parameters (answer: string Net_DetailsCode)

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
