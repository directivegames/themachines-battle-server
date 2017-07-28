// Copyright 2017 Directive Games Limited - All Rights Reserved

#include <stdio.h>
#include <string.h>
#include <iostream>
#include "RakNet/RakPeerInterface.h"
#include "RakNet/MessageIdentifiers.h"
#include "RakNet/BitStream.h"
#include "RakNet/RakNetTypes.h"  // MessageID
#include "BattleSession.h"

#include <vector>
#include <map>
#include <memory>
#include <algorithm>

#define MAX_CONNECTIONS 100
#define SESSION_CAPACITY 2
#define SERVER_PORT 7888

// client address : the session it belongs to
std::map<RakNet::SystemAddress, std::shared_ptr<BattleSession>> sessions;

static std::shared_ptr<BattleSession> FindSession(const RakNet::SystemAddress& client)
{
	auto itr = sessions.find(client);
	if (itr != sessions.end())
	{
		return itr->second;
	}

	return nullptr;
}

static int GetNewSessionID()
{
	static int nextSessionID = 0;
	return nextSessionID++;
}

int main(void)
{
	RakNet::RakPeerInterface* peer = RakNet::RakPeerInterface::GetInstance();
	RakNet::Packet* packet;

	RakNet::SocketDescriptor sd(SERVER_PORT, 0);
	peer->Startup(MAX_CONNECTIONS, &sd, 1);
	peer->SetMaximumIncomingConnections(MAX_CONNECTIONS);
	printf("============================================\n");
	printf("The Machines(TM) battle server has started at port %d\n", SERVER_PORT);
	printf("Session capacity: %d\n", SESSION_CAPACITY);
	printf("Max connections: %d\n", MAX_CONNECTIONS);
	printf("============================================\n\n");

	while (1)
	{
		for (packet = peer->Receive(); packet; peer->DeallocatePacket(packet), packet = peer->Receive())
		{
			auto client = packet->systemAddress;
			switch (packet->data[0])
			{
				case ID_REMOTE_DISCONNECTION_NOTIFICATION:
				case ID_REMOTE_CONNECTION_LOST:
				case ID_DISCONNECTION_NOTIFICATION:
				case ID_CONNECTION_LOST:
				{
					printf("Client disconnected: %s\n", client.ToString(true));
					if (auto session = FindSession(client))
					{
						session->RemoveClient(client);
						// when all the clients in the session disconnect, the session itself will be destroyed
						sessions.erase(client);
					}
					break;
				}

				case ID_REMOTE_NEW_INCOMING_CONNECTION:
				case ID_NEW_INCOMING_CONNECTION:
				{
					auto session = FindSession(client);
					if (!session)
					{
						// try to add the client to existing session
						for (auto itr : sessions)
						{
							if (itr.second->AddClient(client))
							{
								session = itr.second;
								sessions[client] = session;
								break;
							}
						}

						// no existing session is available, create a new one
						if (!session)
						{
							session = std::make_shared<BattleSession>(SESSION_CAPACITY, GetNewSessionID());
							printf("Session %d created\n", session->GetSessionID());
							session->AddClient(client);
							sessions[client] = session;
						}
					}
					break;
				}

				case ID_GAME_COMMAND_REQUEST_BATTLE_START:
				{
					if (auto session = FindSession(client))
					{
						session->TryStartBattle(peer);						
					}
					break;
				}

				case ID_GAME_COMMAND_PLACE_HERO:
				case ID_GAME_COMMAND_OFFMAP_SUPPORT:
				case ID_GAME_COMMAND_USE_ABILITY:
				case ID_GAME_COMMAND_END_BATTLE:
				{
					RakNet::BitStream bsOut(packet->data, packet->length, false);
					if (auto session = FindSession(client))
					{
						session->BroadcastMessage(peer, bsOut);
					}
					break;
				}
				case ID_GAME_COMMAND_LOCKSTEP_COUNT:
				{
					RakNet::BitStream bsIn(packet->data, packet->length, false);
					bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
					std::int32_t reportedFrame;
					bsIn.Read(reportedFrame);

					if (auto session = FindSession(client))
					{
						if (auto clientInfo = session->GetClientInfo(client))
						{
							clientInfo->lastReportedFrame = reportedFrame;
						}

						auto& sessionClients = session->GetClients();
						auto frameBehind = 0;
						for (const auto& clientEntry : sessionClients)
						{
							auto behind = std::max<std::int32_t>(0, clientEntry.second.lastReportedFrame - reportedFrame);
							if (frameBehind < behind)
							{
								frameBehind = behind;
							}
						}

						RakNet::BitStream bsOut;
						bsOut.Write((RakNet::MessageID)ID_GAME_COMMAND_CATCH_UP);
						bsOut.Write((std::int32_t)frameBehind);
						peer->Send(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, client, false);
					}
					break;
				}

				default:
					printf("Message with identifier %i has arrived.\n\n", packet->data[0]);
					break;
			}
		}

		Sleep(1);
	}


	RakNet::RakPeerInterface::DestroyInstance(peer);

	return 0;
}
