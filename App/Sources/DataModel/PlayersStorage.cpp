#include "PlayersStorage.h"

bool ps::PlayersStorage::RegisterPlayerResult(std::string playerName, int playerRating)
{
	m_PlayerRatings.Insert(playerName)->m_Value = playerRating;
	return true;
}

bool ps::PlayersStorage::UnregisterPlayer(const std::string& playerName)
{
	m_PlayerRatings.Delete(playerName);
	return true;
}

bool ps::PlayersStorage::Rollback(int step)
{
	m_PlayerRatings.Rollback(step);
	return true;
}

int ps::PlayersStorage::GetPlayerRating(const std::string& playerName)
{
	auto* node = m_PlayerRatings.Search(playerName);
	if (node)
	{
		return node->m_Value;
	}

	return -1;
}
