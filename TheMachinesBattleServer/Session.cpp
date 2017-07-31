#include "Session.h"
#include "TheMachinesClient.h"
#include "ClientManager.h"
#include "RakNet/RakPeerInterface.h"
#include "RakNet/BitStream.h"
#include "TheMachinesServerTypes.h"
#include <algorithm>

Session::Session(ClientManager& _clientManager) : clientManager(_clientManager)
{
}

void Session::OnClientRequestSessionStart(const TheMachinesClient& requestingClient, RakNet::RakPeerInterface* peer)
{
	for (auto& sessionClient : clients)
	{
		if (sessionClient.address == requestingClient.GetAddress())
		{
			sessionClient.isReady = true;
			break;
		}
	}

	if (AllRequiredParticipantsJoined() == false)
	{
		return;
	}

	bool allClientsReady = true;
	for (const auto& sessionClient : clients)
	{
		if (!sessionClient.isReady)
		{
			allClientsReady = false;
			break;
		}
	}

	if (allClientsReady)
	{
		int team = 0;
		for (const auto& client : clients)
		{
			RakNet::BitStream bsOut;
			bsOut.Write((RakNet::MessageID)TheMachinesGameMessages::ID_GAME_COMMAND_BATTLE_STARTED);
			bsOut.Write((RakNet::MessageID)team);
			bsOut.Write((RakNet::MessageID)clients.size());
			peer->Send(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, client.address, false);
			++team;
		}

		//printf("Session %d started\n\n", sessionID);
	}
}

bool Session::AllRequiredParticipantsJoined() const
{
	return GetCurrentClients() == PARTICIPANTS_PER_SESSION;
}

void Session::AddClient(TheMachinesClient& client)
{
	clients.push_back(SessionClient(client.GetAddress()));
}

void Session::RemoveClient(TheMachinesClient& client)
{
	auto removed = std::find_if(clients.begin(), clients.end()
		, [client](const auto& sessionClient) { return sessionClient.address == client.GetAddress(); });
	if (removed != clients.end())
	{
		clients.erase(removed);
	}
}

void Session::Broadcast(RakNet::RakPeerInterface& peer, const RakNet::BitStream& message)
{
	for (const auto& client : clients)
	{
		peer.Send(&message, HIGH_PRIORITY, RELIABLE_ORDERED, 0, client.address, false);
	}
}

std::int32_t Session::GetFastestFrameInSession() const
{
	auto fastestClient = GetFastestClientInSession();
	return fastestClient ? fastestClient->GetLastReportedFrame() : 0;
}

TheMachinesClient* Session::GetFastestClientInSession() const
{
	TheMachinesClient* fastestClient = nullptr;
	auto fastestFrame = 0;
	for (const auto& sessionClient : clients)
	{
		if (auto otherClient = clientManager.GetClient(sessionClient.address))
		{
			auto frame = otherClient->GetLastReportedFrame();
			if (fastestFrame < frame)
			{
				fastestClient = otherClient;
				fastestFrame = frame;
			}
		}
	}
	return fastestClient;
}
