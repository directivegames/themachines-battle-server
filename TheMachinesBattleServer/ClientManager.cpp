#include "ClientManager.h"

TheMachinesClient* ClientManager::GetClient(const RakNet::SystemAddress& address)
{
	auto itr = clients.find(address);
	if (itr != clients.end())
	{
		return &itr->second;
	}

	return nullptr;
}

void ClientManager::RegisterClient(const RakNet::SystemAddress& address)
{
	clients.emplace(address, TheMachinesClient(address));
}

void ClientManager::UnregisterClient(const RakNet::SystemAddress& address)
{
	clients.erase(address);
}
