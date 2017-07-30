#include "ClientManager.h"
#include <iostream>

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
	if (clients.count(address) == 0)
	{
		clients.emplace(address, TheMachinesClient(address));
	}
	else
	{
		printf("Error. A registered client %s is required to be regestered again.", address.ToString());
	}
}

void ClientManager::UnregisterClient(const RakNet::SystemAddress& address)
{
	clients.erase(address);
}
