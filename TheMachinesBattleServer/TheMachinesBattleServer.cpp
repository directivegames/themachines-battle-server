#include "TheMachinesBattleServer.h"
#include "RakNet/RakPeerInterface.h"
#include "RakNet/BitStream.h"
#include "Session.h"
#include <iostream>
#include <sstream>
#include <chrono>
#include <ctime>

const std::chrono::milliseconds BattleServerConsts::BATTLE_WORLD_TICK_INTERVAL(10);
const std::chrono::milliseconds BattleServerConsts::TIME_BUFF_TO_DISCARD_GAME_MESSAGE(10);
const std::chrono::milliseconds BattleServerConsts::CATCH_UP_REQUIRED_THRESHOLD(200);

std::string GetTime()
{
	auto now = std::chrono::system_clock::now();
	std::time_t now_t = std::chrono::system_clock::to_time_t(now);
	return std::string(std::ctime(&now_t));
}

TheMachinesBattleServer::TheMachinesBattleServer()
	: peer(RakNet::RakPeerInterface::GetInstance())
	, clientManager(std::make_unique<ClientManager>(peer))
	, sessionManager(std::make_unique<SessionManager>(*clientManager))
{
	RakNet::SocketDescriptor sd(BattleServerConsts::SERVER_PORT, 0);
	if (RakNet::RAKNET_STARTED == peer->Startup(BattleServerConsts::MAX_CONNECTIONS, &sd, 1))
	{
		peer->SetMaximumIncomingConnections(BattleServerConsts::MAX_CONNECTIONS);

		printf("============================================\n");
		printf("%s", GetTime().c_str());
		printf("The Machines(TM) battle server has started at port %d\n", BattleServerConsts::SERVER_PORT);
		printf("Participants per battle: %d\n", BattleServerConsts::PARTICIPANTS_PER_SESSION);
		printf("Max connections: %d\n", BattleServerConsts::MAX_CONNECTIONS);
		printf("Catch up threshold: %lld ms.\n", BattleServerConsts::CATCH_UP_REQUIRED_THRESHOLD.count());
		printf("Battle command execution delay %dms longer than the biggest ping (or at least %d frames).\n", BattleServerConsts::NEGOTIATED_COMMAND_EXECUTION_DELAY_MINUS_LONGEST_PING, BattleServerConsts::LEAST_COMMAND_EXECUTE_DELAY);
		printf("Battle world tick interval: %lld ms.\n", BattleServerConsts::BATTLE_WORLD_TICK_INTERVAL.count());
		printf("Time buff to discard game message: %lld ms.\n", BattleServerConsts::TIME_BUFF_TO_DISCARD_GAME_MESSAGE.count());
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
				printf("%s", GetTime().c_str());
				printf("Client disconnected: %s\n", clientAddress.ToString(true));
				UnregisterClient(clientAddress);
				break;
			case ID_REMOTE_NEW_INCOMING_CONNECTION:
			case ID_NEW_INCOMING_CONNECTION:
				printf("%s", GetTime().c_str());
				printf("Client incoming: %s\n", clientAddress.ToString(true));
				RegisterClient(clientAddress);
				break;
			case (int)TheMachinesGameMessages::ID_GAME_COMMAND_REQUEST_BATTLE_START:
				printf("%s", GetTime().c_str());
				printf("Client %s requesting battle start. Client round trip: %lld ms.\n", clientAddress.ToString(true), std::chrono::milliseconds(peer->GetLastPing(clientAddress)).count());
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
				BroadcastGameInstruction(clientAddress, packet);
				break;
			case (int)TheMachinesGameMessages::ID_PRINT_SERVER_INFO:
				SendCurrentState(packet->systemAddress);
				break;
			default:
				printf("%s", GetTime().c_str());
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

			RakNet::BitStream bs(packet->data, packet->length, false);
			if (!session->Broadcast(*peer, bs))
			{
				printf("%s", GetTime().c_str());
				printf("Client %s game message discarded.\n", instigatingClientAddress.ToString());
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
			// fastest client is NOW actually at time:
			//		FastestClientLastestReportedFrame * gameInverval + Ping(FastestClient) / 2 + TimeSinceLastFastestClientReportFrame
			// querying client is NOW actually at time:
			//		ReportedFrame * gameInterval + Ping(QueryingClient) / 2
			const auto fastestClient = session->GetFastestClientInSession();
			const auto fastestReportedFrame = fastestClient->GetLastReportedFrame();
			const auto fastestClientPing = std::chrono::milliseconds(peer->GetLastPing(fastestClient->GetAddress()));
			const auto timeSinceLastFastestFrameReported = fastestClient->TimeSinceLastFrameReported();
			const auto fastestClientCurrentEstimatedTime = fastestReportedFrame * BattleServerConsts::BATTLE_WORLD_TICK_INTERVAL + fastestClientPing / 2 + timeSinceLastFastestFrameReported;

			const auto queryingClientPing = std::chrono::milliseconds(peer->GetLastPing(address));
			const auto queringClientCurrentEstimatedTime = frame * BattleServerConsts::BATTLE_WORLD_TICK_INTERVAL + queryingClientPing / 2;

			const auto timeDiff = fastestClientCurrentEstimatedTime - queringClientCurrentEstimatedTime;

			if (timeDiff >= BattleServerConsts::CATCH_UP_REQUIRED_THRESHOLD 
				// if already catching up, don't constantly send catchup commands. and give some buff for catching up, since catching up to frame X won't actually catch up at frame 30, will meet a few frames after
				&& frame >= client->GetCatchupTargetFrame() + 30)
			{
				const auto catchupTargetFrame = (int)((fastestClientCurrentEstimatedTime + queryingClientPing  / 2 )/ BattleServerConsts::BATTLE_WORLD_TICK_INTERVAL);	// compensate the quering client's ping
				client->SetCatchupTargetFrame(catchupTargetFrame);

				RakNet::BitStream bsOut;
				bsOut.Write((RakNet::MessageID)TheMachinesGameMessages::ID_GAME_COMMAND_CATCH_UP);
				bsOut.Write((std::int32_t)catchupTargetFrame);
				peer->Send(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, address, false);

				printf("%s", GetTime().c_str());
				printf("Client %s (frame %d) is %lld ms behind the fastest client. Requesting it to catch up to frame %d.\n", address.ToString(), frame, timeDiff.count(), catchupTargetFrame);
				printf("Fastet client %s last reported frame is %d, and was %lld ms ago.\n", fastestClient->GetAddress().ToString(), fastestReportedFrame, timeSinceLastFastestFrameReported.count());
				printf("Fastet client ping %lld ms, catching up client ping %lld ms.\n", fastestClientPing.count(), queryingClientPing.count());
				printf("\tCatchup target frame is fastst client's current frame %d,\n\t+time since reported %d frames,\n\t+quering client ping %d frames,\n\t-fastest client half ping %d frames.\n"
					, fastestReportedFrame
					, (int)(timeSinceLastFastestFrameReported/BattleServerConsts::BATTLE_WORLD_TICK_INTERVAL) 
					, (int)(fastestClientPing / BattleServerConsts::BATTLE_WORLD_TICK_INTERVAL)
					, (int)(queryingClientPing / 2 / BattleServerConsts::BATTLE_WORLD_TICK_INTERVAL));
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

void TheMachinesBattleServer::SendCurrentState(const RakNet::SystemAddress& targetAddress)
{
	RakNet::BitStream bsOut;
	bsOut.Write((RakNet::MessageID)TheMachinesGameMessages::ID_SERVER_INFO_REPLICATED);
	bsOut.Write(GetServerState().c_str());
	peer->Send(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, targetAddress, false);
}

std::string TheMachinesBattleServer::GetServerState() const
{
	std::ostringstream oss;
	oss << *sessionManager;

	std::string serverState = oss.str();

	printf("============Logging server state============\n");
	printf("%s", GetTime().c_str());
	printf("%s", serverState.c_str());
	printf("============================================\n");

	return serverState;
}
