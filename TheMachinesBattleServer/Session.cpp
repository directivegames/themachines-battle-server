#include "Session.h"
#include "TheMachinesClient.h"
#include "ClientManager.h"
#include "RakNet/RakPeerInterface.h"
#include "RakNet/BitStream.h"
#include "TheMachinesServerTypes.h"
#include <algorithm>

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

void Session::AddClient(TheMachinesClient& client)
{
	clients.push_back(SessionClient(client.GetAddress()));
}

void Session::RemoveClient(TheMachinesClient& client)
{
	clients.erase(std::find_if(clients.begin(), clients.end()
		, [client](const auto& sessionClient) { return sessionClient.address == client.GetAddress(); }));
}

void Session::Broadcast(RakNet::RakPeerInterface& peer, const RakNet::BitStream& message)
{
	for (const auto& client : clients)
	{
		peer.Send(&message, HIGH_PRIORITY, RELIABLE_ORDERED, 0, client.address, false);
	}
}

std::int32_t Session::CalcFramesBehindFastestClient(const TheMachinesClient& client)
{
	auto frameBehind = 0;
	for (const auto& sessionClient : clients)
	{
		if (auto otherClient = clientManager.GetClient(sessionClient.address))
		{
			auto behind = std::max<std::int32_t>(0, otherClient->GetLastReportedFrame() - client.GetLastReportedFrame());
			if (frameBehind < behind)
			{
				frameBehind = behind;
			}
		}
	}

	return frameBehind;
}
