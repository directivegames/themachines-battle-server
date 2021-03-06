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
	int longestPing = 0;
	for (const auto& sessionClient : clients)
	{
		if (!sessionClient.isReady)
		{
			allClientsReady = false;
			break;
		}
		
		auto clientPing = peer->GetLastPing(sessionClient.address);
		if (clientPing > longestPing)
		{
			longestPing = clientPing;
		}
	}

	if (allClientsReady)
	{
		negotiatedCommandDelayFrames = std::max<std::int32_t>(
			BattleServerConsts::LEAST_COMMAND_EXECUTE_DELAY
			, (longestPing + BattleServerConsts::NEGOTIATED_COMMAND_EXECUTION_DELAY_MINUS_LONGEST_PING) / (int)BattleServerConsts::BATTLE_WORLD_TICK_INTERVAL.count());

		int team = 0;
		for (const auto& client : clients)
		{
			RakNet::BitStream bsOut;
			bsOut.Write((RakNet::MessageID)TheMachinesGameMessages::ID_GAME_COMMAND_BATTLE_STARTED);
			bsOut.Write((RakNet::MessageID)team);
			bsOut.Write((RakNet::MessageID)clients.size());
			bsOut.Write(negotiatedCommandDelayFrames);
			peer->Send(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, client.address, false);
			++team;
		}

		printf("Session started with negotiated command execution delay %d frames (Slowest client has the ping at %d ms).\n\n", negotiatedCommandDelayFrames, longestPing);
	}
}

bool Session::AllRequiredParticipantsJoined() const
{
	return GetCurrentClients() == BattleServerConsts::PARTICIPANTS_PER_SESSION;
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

bool Session::Broadcast(RakNet::RakPeerInterface& peer, RakNet::BitStream& message)
{
	RakNet::MessageID messageID;
	std::int32_t commandStartFrame;
	message.Read(messageID);
	message.Read(commandStartFrame);

	auto success = true;
	for (const auto& client : clients)
	{
		auto clientPing = peer.GetLastPing(client.address);
		std::chrono::milliseconds roundTripDelay(clientPing);

		const auto clientInfo = clientManager.GetClient(client.address);
		const auto clientFrame = clientInfo->GetLastReportedFrame();

		auto timeBehind = (clientFrame - commandStartFrame) * BattleServerConsts::BATTLE_WORLD_TICK_INTERVAL + roundTripDelay + clientInfo->TimeSinceLastFrameReported();
		if (timeBehind > negotiatedCommandDelayFrames * BattleServerConsts::BATTLE_WORLD_TICK_INTERVAL - BattleServerConsts::TIME_BUFF_TO_DISCARD_GAME_MESSAGE)
		{
			printf("Game Command %d broadcast failed. It is %lld ms behind client %s.\n", messageID, timeBehind.count(), clientInfo->GetAddress().ToString());
			printf("\tThe faster client last reported frame: %d, command start frame: %d, frame behind: %d\n", clientFrame, commandStartFrame, clientFrame - commandStartFrame);
			printf("\tClient whose command was discarded' roundtrip duration: %d ms\n", peer.GetLastPing(clientInfo->GetAddress()));
			printf("\tFastest client's round trip %lld ms, duration since fastest client's last frame report: %lld ms\n", roundTripDelay.count(), clientInfo->TimeSinceLastFrameReported().count());

			success = false;
			break;
		}
	}

	if (success)
	{
		for (const auto& client : clients)
		{
			peer.Send(&message, HIGH_PRIORITY, RELIABLE_ORDERED, 0, client.address, false);
		}
	}
	return success;
}

std::int32_t Session::GetFastestFrameInSession() const
{
	auto fastestClient = GetFastestClientInSession();
	return fastestClient ? fastestClient->GetLastReportedFrame() : 0;
}

TheMachinesClient* Session::GetFastestClientInSession() const
{
	TheMachinesClient* fastestClient = nullptr;
	for (const auto& sessionClient : clients)
	{
		if (auto client = clientManager.GetClient(sessionClient.address))
		{
			auto frame = client->GetLastReportedFrame();
			if (!fastestClient || fastestClient->GetLastReportedFrame() < client->GetLastReportedFrame())
			{
				fastestClient = client;
			}
		}
	}
	return fastestClient;
}

std::ostringstream& Session::Write(std::ostringstream& oss) const
{
	oss << "There are " << clients.size() << " clients within this session:\n";
	oss << "Session has negotiated command execution delay " << negotiatedCommandDelayFrames << "frames.\n";
	for (const auto& sessionClient : clients)
	{
		oss << sessionClient.address.ToString() << ", " << "isReady = " << sessionClient.isReady << std::endl;
		if (auto client = clientManager.GetClient(sessionClient.address))
		{
			oss << *client;
		}
		else
		{
			oss << "invalid client!\n";
		}
		oss << std::endl;
	}
	return oss;
}

RakNet::RakPeerInterface* Session::GetPeer() const
{
	return clientManager.GetPeer(); 
}
