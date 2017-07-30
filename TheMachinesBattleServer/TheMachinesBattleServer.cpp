#include "TheMachinesBattleServer.h"
#include "RakNet/RakPeerInterface.h"
#include "RakNet/BitStream.h"
#include "Session.h"
#include <iostream>

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
		printf("A client will be required to catch up if it is behind %d frame the fastest client in the same session.\n", MAX_ALLOWED_BEHIND_FRAMES);
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
				// TODO: uncomment the following once the client is ready for "catch up instead of wait"
			/*{
				const auto frame = ExtractFrameCount(packet);
				OnClientReportFrameCount(clientAddress, frame);
			}
			break;*/
			case (int)TheMachinesGameMessages::ID_GAME_COMMAND_PLACE_HERO:
			case (int)TheMachinesGameMessages::ID_GAME_COMMAND_OFFMAP_SUPPORT:
			case (int)TheMachinesGameMessages::ID_GAME_COMMAND_USE_ABILITY:
			case (int)TheMachinesGameMessages::ID_GAME_COMMAND_END_BATTLE:
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
			RakNet::BitStream bsOut(packet->data, packet->length, false);
			session->Broadcast(*peer, bsOut);
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
			auto behind = session->CalcFramesBehindFastestClient(*client);
			if (behind >= MAX_ALLOWED_BEHIND_FRAMES)
			{
				RakNet::BitStream bsOut;
				bsOut.Write((RakNet::MessageID)TheMachinesGameMessages::ID_GAME_COMMAND_CATCH_UP);
				bsOut.Write((std::int32_t)behind);
				peer->Send(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, address, false);

				printf("Client %s is %d frames behind the fastest client. Requesting it to catch up.", address.ToString(), behind);
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

