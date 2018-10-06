#include "PlayersStorage.h"

#include "PlayerInfo.h"

ps::PlayersStorage::~PlayersStorage()
{
	for (const auto& iter : m_Players)
	{
		delete iter.second;
	}

	m_Players.clear();
}

bool ps::PlayersStorage::RegisterPlayerResult(std::string playerName, int playerRating)
{
	m_CurrentTime.Tick();
	PlayerInfo*& info = m_Players[std::move(playerName)];
	if (info == nullptr)
	{
		info = new PlayerInfo();
	}

	info->RegisterRating(m_CurrentTime, playerRating);
	return true;
}

bool ps::PlayersStorage::UnregisterPlayer(std::string_view playerName)
{
	auto iter = m_Players.find(playerName);
	if (iter == m_Players.end())
	{
		return false;
	}

	m_CurrentTime.Tick();
	iter->second->MarkDeleted(m_CurrentTime);
	return true;
}

bool ps::PlayersStorage::Rollback(int step)
{
	return m_CurrentTime.Rollback(step);
}

int ps::PlayersStorage::GetPlayerRating(std::string_view playerName) const
{
	auto iter = m_Players.find(playerName);
	if (iter == m_Players.end())
	{
		return -1;
	}

	return iter->second->GetRating(m_CurrentTime);
}
