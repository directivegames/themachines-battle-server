#include "SessionManager.h"
#include "TheMachinesClient.h"
#include "Session.h"

void SessionManager::AssignSession(TheMachinesClient& client)
{
	auto battleID = client.GetBattleUID();
	if (battleID > 0)
	{
		auto sessionItr = sessions.find(battleID);
		if (sessionItr != sessions.end())
		{
			auto session = sessionItr->second.lock();
			if (session)
			{
				client.AssignSession(session);
			}
		}

		// no session, meaning it's a new one
		if (!client.GetSession())
		{
			auto session = std::make_shared<Session>();
			client.AssignSession(session);
			sessions.emplace(battleID, session);
		}
	}
	else
	{
		// no valid battle id.
		client.AssignSession(freeGuestSession);
	}
}
