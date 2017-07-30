#include "TheMachinesClient.h"
#include "Session.h"

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

