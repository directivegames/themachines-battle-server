#include "TheMachinesBattleServer.h"
#include "RakNet/RakPeerInterface.h"
#include "RakNet/BitStream.h"
#include "Session.h"
#include <iostream>

TheMachinesBattleServer::TheMachinesBattleServer()
	: peer(RakNet::RakPeerInterface::GetInstance())
	, clientManager(std::make_unique<ClientManager>())
	, sessionManager(std::make_unique<SessionManager>())
{
	RakNet::SocketDescriptor sd(SERVER_PORT, 0);
	if (RakNet::RAKNET_STARTED == peer->Startup(MAX_CONNECTIONS, &sd, 1))
	{
		peer->SetMaximumIncomingConnections(MAX_CONNECTIONS);

		printf("============================================\n");
		printf("The Machines(TM) battle server has started at port %d\n", SERVER_PORT);
		//printf("Session capacity: %d\n", SESSION_CAPACITY);
		printf("Max connections: %d\n", MAX_CONNECTIONS);
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
				OnClientRequestSessionStart(clientAddress);
				break;
			case (int)TheMachinesGameMessages::ID_GAME_COMMAND_PLACE_HERO:
			case (int)TheMachinesGameMessages::ID_GAME_COMMAND_OFFMAP_SUPPORT:
			case (int)TheMachinesGameMessages::ID_GAME_COMMAND_USE_ABILITY:
			case (int)TheMachinesGameMessages::ID_GAME_COMMAND_END_BATTLE:
				BroadcastGameInstruction(clientAddress, packet);
				break;
			case (int)TheMachinesGameMessages::ID_GAME_COMMAND_LOCKSTEP_COUNT:
			{
				const auto frame = ExtractFrameCount(packet);
				OnClientReportFrameCount(clientAddress, frame);
			}
				break;
			default:
				printf("Message with identifier %i has arrived.\n\n", packet->data[0]);
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
	clientManager->UnregisterClient(address);
}

void TheMachinesBattleServer::OnClientRequestSessionStart(const RakNet::SystemAddress& address)
{
	if (auto client = clientManager->GetClient(address))
	{
		auto session = client->GetSession();
		if (!session)
		{
			sessionManager->AssignSession(*client);
			session = client->GetSession();
		}

		session->OnClientRequestSessionStart(*client);
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
			const std::int32_t MAX_ALLOWED_BEHIND_FRAMES = 20;
			if (behind >= MAX_ALLOWED_BEHIND_FRAMES)
			{
				RakNet::BitStream bsOut;
				bsOut.Write((RakNet::MessageID)TheMachinesGameMessages::ID_GAME_COMMAND_CATCH_UP);
				bsOut.Write((std::int32_t)behind);
				peer->Send(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, address, false);
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



