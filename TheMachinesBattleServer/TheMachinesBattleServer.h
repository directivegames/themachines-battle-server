#pragma once

#include "RakNet/RakNetTypes.h"
#include "ClientManager.h"
#include "SessionManager.h"
#include "TheMachinesServerTypes.h"
#include <memory>

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
	static const int MAX_ALLOWED_BEHIND_FRAMES = 20;

	RakNet::RakPeerInterface* peer;
	std::unique_ptr<ClientManager> clientManager;
	std::unique_ptr<SessionManager> sessionManager;

private:
	void OnClientRequestSessionStart(const RakNet::SystemAddress& address);
	void BroadcastGameInstruction(const RakNet::SystemAddress& instigatingClientAddress, RakNet::Packet* packet);
	void OnClientReportFrameCount(const RakNet::SystemAddress& address, std::int32_t frame);

	std::int32_t ExtractFrameCount(RakNet::Packet* packet) const;
};
