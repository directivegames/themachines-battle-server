#include "TheMachinesServerTypes.h"
#include "SessionManager.h"
#include "TheMachinesClient.h"
#include "Session.h"
#include <algorithm>

SessionManager::SessionManager(ClientManager& _clientManager)
	: clientManager(_clientManager) 
	, freeGuestSession(std::make_shared<Session>(clientManager))
{

}

void SessionManager::AddToSession(TheMachinesClient& client)
{
	auto battleID = client.GetBattleUID();
	if (battleID > 0)
	{
		auto sessionItr = sessions.find(battleID);
		if (sessionItr != sessions.end())
		{
			if (auto session = sessionItr->second)
			{
				AddToSession(client, session);
			}
		}

		// no session, meaning we need a new one
		if (!client.GetSession())
		{
			auto session = std::make_shared<Session>(clientManager);
			AddToSession(client, session);
			sessions.emplace(battleID, session);
		}
	}
	else
	{
		// no valid battle id.
		AddToSession(client, freeGuestSession);
		if (freeGuestSession->GetCurrentClients() == BattleServerConsts::PARTICIPANTS_PER_SESSION)
		{
			guestSessions.push_back(std::move(freeGuestSession));
			freeGuestSession = std::make_shared<Session>(clientManager);
		}
	}
}

void SessionManager::AddToSession(TheMachinesClient& client, std::shared_ptr<Session> session)
{
	if (!session)
	{
		printf("Error: adding client %s to an invalid session!", client.GetAddress().ToString());
		return;
	}

	client.AssignSession(session);
	session->AddClient(client);
}


void SessionManager::RemoveFromSession(TheMachinesClient& client)
{
	if (auto session = client.GetSession())
	{
		session->RemoveClient(client);

		// destroy the session if there are no clients in it
		if (session->GetCurrentClients() == 0)
		{
			const auto battleID = client.GetBattleUID();
			if (battleID > 0)
			{
				sessions.erase(battleID);
			}
			else
			{
				auto removed = std::find_if(guestSessions.begin(), guestSessions.end()
					, [session](const auto& s) { return s.get() == session; });
				if (removed != guestSessions.end())
				{
					guestSessions.erase(removed);
				}
			}
		}

		client.AssignSession(nullptr);
	}
}


std::ostringstream& SessionManager::Write(std::ostringstream& oss) const
{
	oss << "There are " << sessions.size() << " formal sessions with battle IDs" << std::endl;
	for (const auto& sessionEntry : sessions)
	{
		oss << sessionEntry.first << std::endl;
		if (sessionEntry.second)
		{
			oss << *sessionEntry.second;
			oss << std::endl;
		}
		else
		{
			oss << "invalid session!\n";
		}
	}
	oss << std::endl << std::endl;

	oss << "There are " << guestSessions.size() << " sessions without battle IDs" << std::endl;
	for (auto i = 0; i < guestSessions.size(); ++i)
	{
		oss << "session" << i << ":" << std::endl;
		if (guestSessions[i])
		{
			oss << *guestSessions[i];
			oss << std::endl;
		}
		else
		{
			oss << "invalid session!\n";
		}
	}
	oss << std::endl << std::endl;

	oss << "There is one pending guest session." << std::endl;
	oss << "info" << ":" << std::endl;
	if (freeGuestSession)
	{
		oss << *freeGuestSession;
		oss << std::endl;
	}
	else
	{
		oss << "invalid session!\n";
	}
	oss << std::endl << std::endl;

	return oss;
}
