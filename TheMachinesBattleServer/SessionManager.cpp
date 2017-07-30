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
		if (freeGuestSession->GetCurrentClients() == Session::PARTICIPANTS_PER_SESSION)
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
	auto session = client.GetSession();
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
			guestSessions.erase(std::find_if(guestSessions.begin(), guestSessions.end()
				, [session](const auto& s) { return s.get() == session; }));
		}
	}

	client.AssignSession(nullptr);
}
