#pragma once

#include "RakNet/RakNetTypes.h"
#include "RakNet/MessageIdentifiers.h"
#include "ClientManager.h"
#include "SessionManager.h"
#include <memory>

namespace RakNet
{
	class BitStream;
}

class TheMachinesBattleServer
{
public:
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

	TheMachinesBattleServer();

	void Update();
	void RegisterClient(const RakNet::SystemAddress& address);
	void UnregisterClient(const RakNet::SystemAddress& address);
	

private:
	static const int SERVER_PORT = 7888;
	static const int MAX_CONNECTIONS = 100;
	//static const int SESSION_CAPACITY = 2;

	RakNet::RakPeerInterface* peer;
	std::unique_ptr<ClientManager> clientManager;
	std::unique_ptr<SessionManager> sessionManager;

private:
	void OnClientRequestSessionStart(const RakNet::SystemAddress& address);
	void BroadcastGameInstruction(const RakNet::SystemAddress& instigatingClientAddress, RakNet::Packet* packet);
	void OnClientReportFrameCount(const RakNet::SystemAddress& address, std::int32_t frame);
	std::int32_t ExtractFrameCount(RakNet::Packet* packet) const;
};
