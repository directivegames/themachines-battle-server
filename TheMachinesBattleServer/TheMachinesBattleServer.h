#pragma once

#include "RakNet/RakNetTypes.h"
#include "ClientManager.h"
#include "SessionManager.h"
#include "TheMachinesServerTypes.h"
#include <memory>
#include <chrono>

namespace RakNet
{
	class BitStream;
}

class TheMachinesBattleServer
{
public:
	TheMachinesBattleServer();

	void Update();
	void RegisterClient(const RakNet::SystemAddress& address);
	void UnregisterClient(const RakNet::SystemAddress& address);

	void PrintCurrentState();

private:
	static const int SERVER_PORT = 7888;
	static const int MAX_CONNECTIONS = 100;
	static const int CATCH_UP_REQUIRED_THRESHOLD = 20;	// A client will be required to catch up if it is this many frames behind the fastest client in the same session
	static const std::chrono::milliseconds BATTLE_WORLD_TICK_INTERVAL;	// corresponding to BattleWorld::tickInterval
	static const int COMMAND_EXECUTE_DELAY = 30;	// corresponding to BattleCommands::COMMAND_EXECUTE_DELAY

	// the game message will be discarded if 
	// (fastestFrame - commandFrame) * tickInverval + fastestClientRoundtrip + timeSinceFastestClientLastFrameReportTime 
	// < BATTLE_WORLD_TICK_INTERVAL * COMMAND_EXECUTE_DELAY - TIME_BUFF_TO_DISCARD_GAME_MESSAGE
	static const std::chrono::milliseconds TIME_BUFF_TO_DISCARD_GAME_MESSAGE;	

	RakNet::RakPeerInterface* peer;
	std::unique_ptr<ClientManager> clientManager;
	std::unique_ptr<SessionManager> sessionManager;

private:
	void OnClientRequestSessionStart(const RakNet::SystemAddress& address);
	void BroadcastGameInstruction(const RakNet::SystemAddress& instigatingClientAddress, RakNet::Packet* packet);
	void OnClientReportFrameCount(const RakNet::SystemAddress& address, std::int32_t frame);

	std::int32_t ExtractFrameCount(RakNet::Packet* packet) const;
};
