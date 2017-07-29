#pragma once

#include <cstdint>

class TheMachinesClient;
namespace RakNet
{
	class RakPeerInterface;
	class BitStream;
}

class Session
{
public:
	void OnClientRequestSessionStart(const TheMachinesClient& client);
	void Broadcast(RakNet::RakPeerInterface& peer, const RakNet::BitStream& message);
	std::int32_t CalcFramesBehindFastestClient(const TheMachinesClient& client);
	void OnAssignedToClient(const TheMachinesClient& client);
};
