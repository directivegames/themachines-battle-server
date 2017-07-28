// Copyright 2017 Directive Games Limited - All Rights Reserved

#include "RakNet/MessageIdentifiers.h"

#include <vector>

namespace RakNet
{
	class RakPeerInterface;
	class BitStream;
	struct SystemAddress;	
}

enum GameMessages
{
	ID_GAME_COMMAND_PLACE_HERO = ID_USER_PACKET_ENUM + 1,
	ID_GAME_COMMAND_OFFMAP_SUPPORT,
	ID_GAME_COMMAND_USE_ABILITY,
	ID_GAME_COMMAND_END_BATTLE,

	ID_GAME_COMMAND_REQUEST_BATTLE_START,   // client to server
	ID_GAME_COMMAND_BATTLE_STARTED,          // server to client

	ID_GAME_COMMAND_LOCKSTEP_COUNT
};

class BattleSession
{
public:
	BattleSession(size_t capacity, int sessionID);

	// returns if the client is successfully added
	bool AddClient(const RakNet::SystemAddress& client);

	// returns if the client is successfully removed
	bool RemoveClient(const RakNet::SystemAddress& client);

	bool TryStartBattle(RakNet::RakPeerInterface* peer);

	void BroadcastMessage(RakNet::RakPeerInterface* peer, const RakNet::BitStream& message) const;
	
	bool ContainsClient(const RakNet::SystemAddress& client) const;
	bool CanAddNewClient() const;
	int GetSessionID() const { return sessionID; }

private:
	std::vector<RakNet::SystemAddress> clients;
	size_t capacity;
	int sessionID;
	bool battleStarted = false;
};
