#include "TheMachinesBattleServer.h"
#include "RakNet/RakPeerInterface.h"
#include "RakNet/BitStream.h"
#include "Session.h"
#include <iostream>

const std::chrono::milliseconds TheMachinesBattleServer::BATTLE_WORLD_TICK_INTERVAL(10);
const std::chrono::milliseconds TheMachinesBattleServer::TIME_BUFF_TO_DISCARD_GAME_MESSAGE(10);

TheMachinesBattleServer::TheMachinesBattleServer()
	: peer(RakNet::RakPeerInterface::GetInstance())
	, clientManager(std::make_unique<ClientManager>())
	, sessionManager(std::make_unique<SessionManager>(*clientManager))
{
	RakNet::SocketDescriptor sd(SERVER_PORT, 0);
	if (RakNet::RAKNET_STARTED == peer->Startup(MAX_CONNECTIONS, &sd, 1))
	{
		peer->SetMaximumIncomingConnections(MAX_CONNECTIONS);

		printf("============================================\n");
		printf("The Machines(TM) battle server has started at port %d\n", SERVER_PORT);
		printf("Participants per battle: %d\n", Session::PARTICIPANTS_PER_SESSION);
		printf("Max connections: %d\n", MAX_CONNECTIONS);
		printf("Catch up threshold: %d.\n", CATCH_UP_REQUIRED_THRESHOLD);
		printf("Battle command execution delay: %d frames.\n", COMMAND_EXECUTE_DELAY);
		printf("Battle world tick interval: %lld ms.\n", BATTLE_WORLD_TICK_INTERVAL.count());
		printf("Time buff to discard game message: %lld ms.\n", TIME_BUFF_TO_DISCARD_GAME_MESSAGE.count());
		printf("============================================\n\n");
	}
}

void TheMachinesBattleServer::Update()
{
	RakNet::Packet* packet = nullptr;
	while (1)
	{
		for (packet = peer->Receive(); packet; peer->DeallocatePacket(packet), packet = peer->Receive())
		{
			auto clientAddress = packet->systemAddress;
			switch (packet->data[0])
			{
			case ID_REMOTE_DISCONNECTION_NOTIFICATION:
			case ID_REMOTE_CONNECTION_LOST:
			case ID_DISCONNECTION_NOTIFICATION:
			case ID_CONNECTION_LOST:
				printf("Client disconnected: %s\n", clientAddress.ToString(true));
				UnregisterClient(clientAddress);
				break;
			case ID_REMOTE_NEW_INCOMING_CONNECTION:
			case ID_NEW_INCOMING_CONNECTION:
				printf("Client incoming: %s\n", clientAddress.ToString(true));
				RegisterClient(clientAddress);
				break;
			case (int)TheMachinesGameMessages::ID_GAME_COMMAND_REQUEST_BATTLE_START:
				printf("Client %s requesting battle start\n", clientAddress.ToString(true));
				OnClientRequestSessionStart(clientAddress);
				break;
			case (int)TheMachinesGameMessages::ID_GAME_COMMAND_LOCKSTEP_COUNT:
			{
				const auto frame = ExtractFrameCount(packet);
				OnClientReportFrameCount(clientAddress, frame);
			}
			break;
			case (int)TheMachinesGameMessages::ID_GAME_COMMAND_PLACE_HERO:
			case (int)TheMachinesGameMessages::ID_GAME_COMMAND_OFFMAP_SUPPORT:
			case (int)TheMachinesGameMessages::ID_GAME_COMMAND_USE_ABILITY:
			case (int)TheMachinesGameMessages::ID_GAME_COMMAND_END_BATTLE:
				printf("Client %s sends battle command with round tirp %lld ms\n", clientAddress.ToString(true), std::chrono::milliseconds(peer->GetLastPing(clientAddress)).count());
				BroadcastGameInstruction(clientAddress, packet);
				break;
			
			default:
				printf("Message with unknown identifier %i has arrived.\n\n", packet->data[0]);
				break;
			}
		}

		Sleep(1);
	}
}

void TheMachinesBattleServer::RegisterClient(const RakNet::SystemAddress& address)
{
	clientManager->RegisterClient(address);
}

void TheMachinesBattleServer::UnregisterClient(const RakNet::SystemAddress& address)
{
	if (auto client = clientManager->GetClient(address))
	{
		sessionManager->RemoveFromSession(*client);
	}

	clientManager->UnregisterClient(address);
}

void TheMachinesBattleServer::OnClientRequestSessionStart(const RakNet::SystemAddress& address)
{
	if (auto client = clientManager->GetClient(address))
	{
		auto session = client->GetSession();
		if (!session)
		{
			sessionManager->AddToSession(*client);
			session = client->GetSession();
		}

		session->OnClientRequestSessionStart(*client, peer);
	}

	// TODO: error log for client or session not found
}

void TheMachinesBattleServer::BroadcastGameInstruction(const RakNet::SystemAddress& instigatingClientAddress, RakNet::Packet* packet)
{
	if (auto client = clientManager->GetClient(instigatingClientAddress))
	{
		if (auto session = client->GetSession())
		{
			auto fastestClient = session->GetFastestClientInSession();
			auto fastestFrame = session->GetFastestFrameInSession();

			RakNet::BitStream bs(packet->data, packet->length, false);
			RakNet::MessageID messageID;
			std::int32_t frame;
			bs.Read(messageID);
			bs.Read(frame);

			auto fastestClientPing = peer->GetLastPing(fastestClient->GetAddress());
			std::chrono::milliseconds roundTripDelay(fastestClientPing);
			auto timeBehind = (fastestFrame - frame) * BATTLE_WORLD_TICK_INTERVAL + roundTripDelay + fastestClient->TimeSinceLastFrameReported();
			if (timeBehind < BATTLE_WORLD_TICK_INTERVAL * COMMAND_EXECUTE_DELAY - TIME_BUFF_TO_DISCARD_GAME_MESSAGE)
			{
				session->Broadcast(*peer, bs);
			}
			else
			{
				printf("Client %s game message %d is discarded because this message is %d frames (plus round trip, plus time since it's last report, approx. %lld ms) behind the fastest client in the same session.\n"
					, instigatingClientAddress.ToString(), messageID, fastestFrame - frame, timeBehind.count());
			}
		}
	}

	// TODO: error log for client or session not found
}

void TheMachinesBattleServer::OnClientReportFrameCount(const RakNet::SystemAddress& address, std::int32_t frame)
{
	if (auto client = clientManager->GetClient(address))
	{
		client->SetLastReportedFrame(frame);

		if (auto session = client->GetSession())
		{
			auto fastestFrame = session->GetFastestFrameInSession();
			if (fastestFrame - frame >= CATCH_UP_REQUIRED_THRESHOLD && frame >= client->GetCatchupTargetFrame())	// if already catching up, don't constantly send catchup commands
			{
				client->SetCatchupTargetFrame(fastestFrame);

				RakNet::BitStream bsOut;
				bsOut.Write((RakNet::MessageID)TheMachinesGameMessages::ID_GAME_COMMAND_CATCH_UP);
				bsOut.Write((std::int32_t)fastestFrame);
				peer->Send(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, address, false);

				printf("Client %s is %d frames behind the fastest client. Requesting it to catch up.\n", address.ToString(), fastestFrame - frame);
			}
		}
	}
}

std::int32_t TheMachinesBattleServer::ExtractFrameCount(RakNet::Packet* packet) const
{
	RakNet::BitStream bsIn(packet->data, packet->length, false);
	if (packet->data[0] == (int)TheMachinesGameMessages::ID_GAME_COMMAND_LOCKSTEP_COUNT)
	{
		bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
		std::int32_t frame;
		bsIn.Read(frame);

		return frame;
	}
	
	return -1;
}

void TheMachinesBattleServer::PrintCurrentState()
{

}

