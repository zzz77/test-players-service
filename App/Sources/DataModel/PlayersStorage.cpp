#include "PlayersStorage.h"

bool pst::PlayersStorage::RegisterPlayerResult(std::string playerName, int playerRating)
{
	m_PlayerRatings.Insert(playerName)->m_Value = playerRating;
	return true;
}

bool pst::PlayersStorage::UnregisterPlayer(const std::string& playerName)
{
	m_PlayerRatings.Delete(playerName);
	return true;
}

bool pst::PlayersStorage::Rollback(int step)
{
	m_PlayerRatings.Rollback(step);
	return true;
}

int pst::PlayersStorage::GetPlayerRating(const std::string& playerName) const
{
	auto* node = m_PlayerRatings.Search(playerName);
	if (node)
	{
		return node->m_Value;
	}

	return -1;
}
