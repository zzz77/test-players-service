#pragma once

#include "../CoreLib/PersistentMap.h"

#include <functional>
#include <map>
#include <string>
#include <string_view>

namespace pst
{
	class PlayersStorage
	{
	public:
		bool RegisterPlayerResult(std::string playerName, int playerRating);
		bool UnregisterPlayer(const std::string& playerName);
		bool Rollback(int step);
		int GetPlayerRank(const std::string& playerName) const;
		int GetPlayerRating(const std::string& playerName) const;

	private:
		PersistentMap<std::string, int> m_PlayerRatings;
	};
}
