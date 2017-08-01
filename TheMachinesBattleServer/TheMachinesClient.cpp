#include "TheMachinesClient.h"
#include "Session.h"
#include "RakNet/RakPeerInterface.h"

TheMachinesClient::TheMachinesClient(const RakNet::SystemAddress& addr)
	: address(addr)
	, battleUID(-1)
	, lastReportedFrame(0)
{
}

Session* TheMachinesClient::GetSession() const
{
	if (auto s = session.lock())
	{
		return s.get();
	}
	return nullptr;
}

void TheMachinesClient::AssignSession(std::shared_ptr<Session> newSession)
{
	session = newSession;
}

void TheMachinesClient::SetLastReportedFrame(std::int32_t frame) 
{ 
	lastFrameReportTimeStamp = std::chrono::steady_clock::now();
	lastReportedFrame = frame; 
}

std::chrono::milliseconds TheMachinesClient::TimeSinceLastFrameReported() const
{
	auto now = std::chrono::steady_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastFrameReportTimeStamp);
	return duration;
}


std::ostringstream& TheMachinesClient::Write(std::ostringstream& oss) const
{
	auto session = GetSession();
	auto peer = session ? session->GetPeer() : nullptr;


	oss << "Client " << address.ToString()
		<< "(ping: " << (peer ? peer->GetLastPing(address) : -1) << "ms).\n"
		<< "Last Reported Frame was " << lastReportedFrame
		<< " which was set " << TimeSinceLastFrameReported().count() << "ms ago.\n";
	if (catchupTargetFrame > lastReportedFrame)
	{
		oss << "Trying to catch up to frame " << catchupTargetFrame;
	}
	return oss;
}
