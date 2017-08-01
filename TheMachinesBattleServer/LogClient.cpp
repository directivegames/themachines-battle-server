#include "LogClient.h"
#include "RakNet/RakPeerInterface.h"
#include "RakNet/BitStream.h"
#include "TheMachinesServerTypes.h"

LogClient::LogClient(const char* ipStr, unsigned short port)
	: peer(RakNet::RakPeerInterface::GetInstance())
{
	RakNet::SocketDescriptor sd;
	peer->Startup(1, &sd, 1);
	peer->Connect(ipStr, port, 0, 0);
}

void LogClient::Start()
{
	RakNet::Packet *packet;
	bool block = true;

	while (block)
	{
		for (packet = peer->Receive(); packet; peer->DeallocatePacket(packet), packet = peer->Receive())
		{
			switch (packet->data[0])
			{
			case ID_CONNECTION_REQUEST_ACCEPTED:
				{
					RakNet::BitStream bsOut;
					bsOut.Write((RakNet::MessageID)TheMachinesGameMessages::ID_PRINT_SERVER_INFO);
					peer->Send(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, false);
					break;
				}
			case (RakNet::MessageID)TheMachinesGameMessages::ID_SERVER_INFO_REPLICATED:
				{
					printf("server info returned..\n");
					RakNet::RakString rs;
					RakNet::BitStream bsIn(packet->data, packet->length, false);
					bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
					bsIn.Read(rs);
					printf("%s\n", rs.C_String());
					block = false;
					break;
				}
			}
		}
	}
}
