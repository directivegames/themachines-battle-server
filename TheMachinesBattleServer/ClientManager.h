#pragma once

#include "RakNet/RakNetTypes.h"
#include "TheMachinesClient.h"
#include <map>
#include <memory>

class ClientManager
{
public:
	TheMachinesClient* GetClient(const RakNet::SystemAddress& address);

	void RegisterClient(const RakNet::SystemAddress& address);
	void UnregisterClient(const RakNet::SystemAddress& address);

private:
	std::map<RakNet::SystemAddress, TheMachinesClient> clients;
};
