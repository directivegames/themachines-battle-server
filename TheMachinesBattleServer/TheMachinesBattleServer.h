#pragma once

#include "RakNet/RakNetTypes.h"
#include "ClientManager.h"
#include "SessionManager.h"
#include "TheMachinesServerTypes.h"
#include <memory>
#include <chrono>
#include <string>

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

	void SendCurrentState(const RakNet::SystemAddress& targetAddress);
	std::string GetServerState() const;

private:
	RakNet::RakPeerInterface* peer;
	std::unique_ptr<ClientManager> clientManager;
	std::unique_ptr<SessionManager> sessionManager;

private:
	void OnClientRequestSessionStart(const RakNet::SystemAddress& address);
	void BroadcastGameInstruction(const RakNet::SystemAddress& instigatingClientAddress, RakNet::Packet* packet);
	void OnClientReportFrameCount(const RakNet::SystemAddress& address, std::int32_t frame);

	std::int32_t ExtractFrameCount(RakNet::Packet* packet) const;
};
