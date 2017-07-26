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
#define SERVER_PORT 58888

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
std::vector<RakNet::SystemAddress> current_clients;
const auto total_players = 2;

int main(void)
{
	RakNet::RakPeerInterface *peer = RakNet::RakPeerInterface::GetInstance();
	RakNet::Packet *packet;

	RakNet::SocketDescriptor sd(SERVER_PORT, 0);
	peer->Startup(MAX_CLIENTS, &sd, 1);
	peer->SetMaximumIncomingConnections(MAX_CLIENTS);
	printf("The Machines(TM) battle server has started!.\n");

	while (1)
	{
		for (packet = peer->Receive(); packet; peer->DeallocatePacket(packet), packet = peer->Receive())
		{
			switch (packet->data[0])
			{
			case ID_REMOTE_DISCONNECTION_NOTIFICATION:
				printf("Another client has disconnected.\n");
				break;
			case ID_REMOTE_CONNECTION_LOST:
				printf("Another client has lost the connection.\n");
				break;
			case ID_REMOTE_NEW_INCOMING_CONNECTION:
				printf("Another client has connected.\n");
				break;
			case ID_NEW_INCOMING_CONNECTION:
				printf("A connection is incoming.\n");
				new_clients.push_back(packet->systemAddress);
				break;
			case ID_NO_FREE_INCOMING_CONNECTIONS:
				printf("The server is full.\n");
				break;
			case ID_DISCONNECTION_NOTIFICATION:
				new_clients.erase(std::remove(new_clients.begin(), new_clients.end(), packet->systemAddress), new_clients.end());
				printf("A client has disconnected. Now there are %zd pending clients\n", new_clients.size());
				break;
			case ID_CONNECTION_LOST:
				new_clients.erase(std::remove(new_clients.begin(), new_clients.end(), packet->systemAddress), new_clients.end());
				printf("A client lost the connection. Now there are %zd pending clients\n", new_clients.size());
				break;

			case ID_GAME_COMMAND_REQUEST_BATTLE_START:
				{
					if (new_clients.size() >= total_players)
					{					
						for (auto index = 0; index < new_clients.size(); ++index)
						{
							RakNet::BitStream bsOut;
							bsOut.Write((RakNet::MessageID)ID_GAME_COMMAND_BATTLE_STARTED);
							bsOut.Write((RakNet::MessageID)index);
							bsOut.Write((RakNet::MessageID)total_players);
							peer->Send(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, new_clients[index], false);
						}

						current_clients = std::move(new_clients);
					}
				}
				break;
			case ID_GAME_COMMAND_PLACE_HERO:
			case ID_GAME_COMMAND_OFFMAP_SUPPORT:
			case ID_GAME_COMMAND_USE_ABILITY:
			case ID_GAME_COMMAND_END_BATTLE:
			case ID_GAME_COMMAND_LOCKSTEP_COUNT:
				{
					RakNet::BitStream bsOut(packet->data, packet->length, false);
					for (const auto& client : current_clients)
					{
						peer->Send(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, client, false);
					}
				}
				break;

			default:
				printf("Message with identifier %i has arrived.\n", packet->data[0]);
				break;
			}
		}
	}


	RakNet::RakPeerInterface::DestroyInstance(peer);

	return 0;
}
