#pragma once

#include "RakNet/RakNetTypes.h"
#include <cstdint>
#include <vector>

class TheMachinesClient;
class ClientManager;

namespace RakNet
{
	class RakPeerInterface;
	class BitStream;
}

struct SessionClient
{
	SessionClient(const RakNet::SystemAddress& addr) : address(addr), isReady(false) {}
	RakNet::SystemAddress address;
	bool isReady;
};

class Session
{
public:
	Session(ClientManager& _clientManager);

	void OnClientRequestSessionStart(const TheMachinesClient& requestingClient, RakNet::RakPeerInterface* peer);
	void Broadcast(RakNet::RakPeerInterface& peer, const RakNet::BitStream& message);
	std::int32_t GetFastestFrameInSession() const;
	TheMachinesClient* GetFastestClientInSession() const;

	void AddClient(TheMachinesClient& client);
	void RemoveClient(TheMachinesClient& client);

	size_t GetCurrentClients() const { return clients.size(); }

	static const int PARTICIPANTS_PER_SESSION = 2;

private:
	std::vector<SessionClient> clients;
	ClientManager& clientManager;	// damn this is ugly

private:
	bool AllRequiredParticipantsJoined() const;
};
