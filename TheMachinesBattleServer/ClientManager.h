#pragma once

#include "RakNet/RakNetTypes.h"
#include "TheMachinesClient.h"
#include <map>
#include <memory>

namespace RakNet
{
	class RakPeerInterface;
}

class ClientManager
{
public:
	ClientManager(RakNet::RakPeerInterface* p) : peer(p) {}
	RakNet::RakPeerInterface* GetPeer() const { return peer; }

	TheMachinesClient* GetClient(const RakNet::SystemAddress& address);

	void RegisterClient(const RakNet::SystemAddress& address);
	void UnregisterClient(const RakNet::SystemAddress& address);

private:
	std::map<RakNet::SystemAddress, TheMachinesClient> clients;
	RakNet::RakPeerInterface* peer;	// ugly method to get peer
};
