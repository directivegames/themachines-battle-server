#pragma once


#include "RakNet/MessageIdentifiers.h"
#include <chrono>

enum class TheMachinesGameMessages
{
	ID_GAME_COMMAND_PLACE_HERO = ID_USER_PACKET_ENUM + 1,
	ID_GAME_COMMAND_OFFMAP_SUPPORT,
	ID_GAME_COMMAND_USE_ABILITY,
	ID_GAME_COMMAND_END_BATTLE,

	ID_GAME_COMMAND_REQUEST_BATTLE_START,   // client to server
	ID_GAME_COMMAND_BATTLE_STARTED,          // server to client

	ID_GAME_COMMAND_LOCKSTEP_COUNT,

	ID_GAME_COMMAND_CATCH_UP,               // server to client



	ID_PRINT_SERVER_INFO = 250,				// for a special client to ask server to print server info
	ID_SERVER_INFO_REPLICATED				// return the server info to the special client
};

struct BattleServerConsts
{
	static const int SERVER_PORT = 7890;
	static const int MAX_CONNECTIONS = 100;
	static const int CATCH_UP_REQUIRED_THRESHOLD = 20;	// A client will be required to catch up if it is this many frames behind the fastest client in the same session
	static const std::chrono::milliseconds BATTLE_WORLD_TICK_INTERVAL;	// corresponding to BattleWorld::tickInterval
	static const int LEAST_COMMAND_EXECUTE_DELAY = 30;					// smallest command execution delay, even if all clients' ping are very small
	static const int NEGOTIATED_COMMAND_EXECUTION_DELAY_MINUS_LONGEST_PING = 120;	// the negotiated command execution delay will be the biggest ping plus this duration (in ms)

	// the game message will be discarded if 
	// (fastestFrame - commandFrame) * tickInverval + fastestClientRoundtrip + timeSinceFastestClientLastFrameReportTime 
	// < BATTLE_WORLD_TICK_INTERVAL * commandExecutionDelay - TIME_BUFF_TO_DISCARD_GAME_MESSAGE
	static const std::chrono::milliseconds TIME_BUFF_TO_DISCARD_GAME_MESSAGE;
	static const int PARTICIPANTS_PER_SESSION = 2;
};
