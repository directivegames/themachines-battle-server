#include <stdio.h>
#include <string.h>
#include <iostream>
#include "RakNet/RakPeerInterface.h"
#include "RakNet/MessageIdentifiers.h"
#include "RakNet/BitStream.h"
#include "RakNet/RakNetTypes.h"  // MessageID
#include <vector>
#include <algorithm>

#define MAX_CLIENTS 10
#define SERVER_PORT 7888

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

std::vector<RakNet::SystemAddress> new_clients;
std::vector<std::vector<RakNet::SystemAddress>> sessions;
const auto total_players = 2;

void RemoveClient(const RakNet::SystemAddress& clientSystemAddr)
{
	{
		const auto count = new_clients.size();
		new_clients.erase(std::remove(new_clients.begin(), new_clients.end(), clientSystemAddr), new_clients.end());
		if (count != new_clients.size())
		{
			std::cout << "Client " << clientSystemAddr.ToString() << " removed from pending list\n";
			std::cout << "There are " << new_clients.size() << " pending clients\n";
		}
	}

	for (auto& session : sessions)
	{
		const auto count = session.size();
		session.erase(std::remove(session.begin(), session.end(), clientSystemAddr), session.end());
		if (count != session.size())
		{
			std::cout << "Client " << clientSystemAddr.ToString() << " removed from ongoing session.\n";
		}
	}

	{
		const auto count = sessions.size();
		sessions.erase(std::remove_if(sessions.begin(), sessions.end(), [](const auto& s) {return s.size() == 0; }), sessions.end());
		if (count != sessions.size())
		{
			std::cout << "Onging session cleared. There are " << sessions.size() << " ongoing sessions.\n";
		}
	}
}

void AddIncomingClient(const RakNet::SystemAddress& clientSystemAddr)
{
	std::cout << "Get an incoming client " << clientSystemAddr.ToString() << "\n";
	RemoveClient(clientSystemAddr);
	new_clients.push_back(clientSystemAddr);
}

void StartSession(RakNet::RakPeerInterface *peer)
{
	for (auto index = 0; index < new_clients.size(); ++index)
	{
		RakNet::BitStream bsOut;
		bsOut.Write((RakNet::MessageID)ID_GAME_COMMAND_BATTLE_STARTED);
		bsOut.Write((RakNet::MessageID)index);
		bsOut.Write((RakNet::MessageID)total_players);
		peer->Send(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, new_clients[index], false);
	}

	sessions.emplace_back(std::move(new_clients));

	std::cout << "NEW SESSION STARTED, TADA!!!!\n";
	for (const auto& client : sessions.back())
	{
		std::cout << "\t" << client.ToString() << std::endl;
	}
	std::cout << "There are " << sessions.size() << " ongoing sessions\n";
}

int main(void)
{
	RakNet::RakPeerInterface *peer = RakNet::RakPeerInterface::GetInstance();
	RakNet::Packet *packet;

	RakNet::SocketDescriptor sd(SERVER_PORT, 0);
	peer->Startup(MAX_CLIENTS, &sd, 1);
	peer->SetMaximumIncomingConnections(MAX_CLIENTS);
	printf("The Machines(TM) battle server has started at port %d\n\n", SERVER_PORT);

	while (1)
	{
		for (packet = peer->Receive(); packet; peer->DeallocatePacket(packet), packet = peer->Receive())
		{
			switch (packet->data[0])
			{
			case ID_REMOTE_DISCONNECTION_NOTIFICATION:
				printf("Another client has disconnected.\n");
				printf("\n");
				break;
			case ID_REMOTE_CONNECTION_LOST:
				printf("Another client has lost the connection.\n");
				printf("\n");
				break;
			case ID_REMOTE_NEW_INCOMING_CONNECTION:
				printf("Another client has connected.\n");
				printf("\n");
				break;
			case ID_NEW_INCOMING_CONNECTION:
				printf("A connection %s is incoming.\n", packet->systemAddress.ToString());
				AddIncomingClient(packet->systemAddress);
				printf("\n");
				break;
			case ID_NO_FREE_INCOMING_CONNECTIONS:
				printf("The server is full.\n");
				printf("\n");
				break;
			case ID_DISCONNECTION_NOTIFICATION:
				printf("A client %s has disconnected.\n", packet->systemAddress.ToString());
				RemoveClient(packet->systemAddress);
				printf("\n");
				break;
			case ID_CONNECTION_LOST:
				printf("A client %s lost the connection.\n", packet->systemAddress.ToString());
				RemoveClient(packet->systemAddress);
				printf("\n");
				break;

			case ID_GAME_COMMAND_REQUEST_BATTLE_START:
				{
					if (new_clients.size() >= total_players)
					{					
						StartSession(peer);
					}
					printf("\n");
				}
				break;
			case ID_GAME_COMMAND_PLACE_HERO:
			case ID_GAME_COMMAND_OFFMAP_SUPPORT:
			case ID_GAME_COMMAND_USE_ABILITY:
			case ID_GAME_COMMAND_END_BATTLE:
			case ID_GAME_COMMAND_LOCKSTEP_COUNT:
				{
					RakNet::BitStream bsOut(packet->data, packet->length, false);
					for (const auto& session : sessions)
					{
						bool seesionFound = false;
						for (const auto& client : session)
						{
							if (client == packet->systemAddress)
							{
								seesionFound = true;
								break;
							}
						}
						if (seesionFound)
						{
							for (const auto& client : session)
							{
								peer->Send(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, client, false);
							}
							break;
						}
					}
				}
				break;

			default:
				printf("Message with identifier %i has arrived.\n", packet->data[0]);
				printf("\n");
				break;
			}
		}

		Sleep(1);
	}


	RakNet::RakPeerInterface::DestroyInstance(peer);

	return 0;
}
