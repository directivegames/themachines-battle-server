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
};

struct BattleServerConsts
{
	static const int SERVER_PORT = 7888;
	static const int MAX_CONNECTIONS = 100;
	static const int CATCH_UP_REQUIRED_THRESHOLD = 20;	// A client will be required to catch up if it is this many frames behind the fastest client in the same session
	static const std::chrono::milliseconds BATTLE_WORLD_TICK_INTERVAL;	// corresponding to BattleWorld::tickInterval
	static const int COMMAND_EXECUTE_DELAY = 30;	// corresponding to BattleCommands::COMMAND_EXECUTE_DELAY

	// the game message will be discarded if 
	// (fastestFrame - commandFrame) * tickInverval + fastestClientRoundtrip + timeSinceFastestClientLastFrameReportTime 
	// < BATTLE_WORLD_TICK_INTERVAL * COMMAND_EXECUTE_DELAY - TIME_BUFF_TO_DISCARD_GAME_MESSAGE
	static const std::chrono::milliseconds TIME_BUFF_TO_DISCARD_GAME_MESSAGE;
	static const int PARTICIPANTS_PER_SESSION = 2;
};
