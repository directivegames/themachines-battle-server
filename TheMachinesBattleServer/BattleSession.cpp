// Copyright 2017 Directive Games Limited - All Rights Reserved

#include "BattleSession.h"
#include "RakNet/RakNetTypes.h"
#include "RakNet/RakPeerInterface.h"
#include "RakNet/BitStream.h"

#include <algorithm>

BattleSession::BattleSession(size_t _capacity, int _sessionID)
: capacity(_capacity)
, sessionID(_sessionID)
{
}

bool BattleSession::AddClient(const RakNet::SystemAddress& client)
{
	if (!CanAddNewClient() || ContainsClient(client))
	{
		return false;
	}

	clients.push_back(client);
	printf("Client added to session %d: %s\n\n", sessionID, client.ToString(true));

	return true;
}

bool BattleSession::RemoveClient(const RakNet::SystemAddress& client)
{
	auto itr = std::find(clients.begin(), clients.end(), client);
	if (itr != clients.end())
	{
		clients.erase(itr);
		printf("Client removed from session %d: %s\n\n", sessionID, client.ToString(true));
		return true;
	}

	return false;
}

bool BattleSession::ContainsClient(const RakNet::SystemAddress& client) const
{
	return std::find(clients.begin(), clients.end(), client) != clients.end();
}

bool BattleSession::CanAddNewClient() const
{
	if (battleStarted)
	{
		return false;
	}

	return clients.size() < capacity;
}

bool BattleSession::TryStartBattle(RakNet::RakPeerInterface* peer)
{
	if (clients.size() != capacity || battleStarted)
	{
		return false;
	}

	for (auto index = 0; index < clients.size(); ++index)
	{
		RakNet::BitStream bsOut;
		bsOut.Write((RakNet::MessageID)ID_GAME_COMMAND_BATTLE_STARTED);
		bsOut.Write((RakNet::MessageID)index);
		bsOut.Write((RakNet::MessageID)clients.size());
		peer->Send(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, clients[index], false);
	}

	battleStarted = true;

	printf("Session %d started\n\n", sessionID);

	return true;
}

void BattleSession::BroadcastMessage(RakNet::RakPeerInterface* peer, const RakNet::BitStream& message) const
{
	if (!peer)
	{
		return;
	}

	for (auto client: clients)
	{
		peer->Send(&message, HIGH_PRIORITY, RELIABLE_ORDERED, 0, client, false);
	}	
}
