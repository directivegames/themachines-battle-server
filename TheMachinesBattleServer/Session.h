#pragma once

#include "RakNet/RakNetTypes.h"
#include <cstdint>
#include <vector>
#include <sstream>
#include <chrono>

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
	bool Broadcast(RakNet::RakPeerInterface& peer, RakNet::BitStream& message);
	std::int32_t GetFastestFrameInSession() const;
	TheMachinesClient* GetFastestClientInSession() const;

	void AddClient(TheMachinesClient& client);
	void RemoveClient(TheMachinesClient& client);

	size_t GetCurrentClients() const { return clients.size(); }

	std::ostringstream& Write(std::ostringstream& oss) const;
	RakNet::RakPeerInterface* GetPeer() const;

private:
	std::vector<SessionClient> clients;
	std::int32_t negotiatedCommandDelayFrames;
	ClientManager& clientManager;	// damn this is ugly

private:
	bool AllRequiredParticipantsJoined() const;
};

inline std::ostream& operator <<(std::ostringstream& oss, const Session& session) {	return session.Write(oss); }
