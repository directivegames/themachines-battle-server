#include "Session.h"
#include "TheMachinesClient.h"
#include "RakNet/RakPeerInterface.h"

void Session::OnClientRequestSessionStart(const TheMachinesClient& client)
{
}

void Session::OnAssignedToClient(const TheMachinesClient& client)
{

}

void Session::Broadcast(RakNet::RakPeerInterface& peer, const RakNet::BitStream& message)
{
	RakNet::SystemAddress clientAddress;
	//for (const auto& clientEntry : clients)
	{
		peer.Send(&message, HIGH_PRIORITY, RELIABLE_ORDERED, 0, clientAddress, false);
	}
}

std::int32_t Session::CalcFramesBehindFastestClient(const TheMachinesClient& client)
{
	/*auto& sessionClients = GetClients();
	auto frameBehind = 0;
	for (const auto& clientEntry : sessionClients)
	{
		auto behind = std::max<std::int32_t>(0, clientEntry.second.lastReportedFrame - reportedFrame);
		if (frameBehind < behind)
		{
			frameBehind = behind;
		}
	}*/

	return 0;
}
