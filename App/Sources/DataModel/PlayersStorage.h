#pragma once

#include "../CoreLib/RBTree.h"

#include <functional>
#include <map>
#include <string>
#include <string_view>

namespace ps
{
	class PlayerInfo;

	class PlayersStorage
	{
	public:
		bool RegisterPlayerResult(std::string playerName, int playerRating);
		bool UnregisterPlayer(const std::string& playerName);
		bool Rollback(int step);
		int GetPlayerRank(const std::string& playerName) const;
		int GetPlayerRating(const std::string& playerName);

	private:
		RBTree<std::string, int> m_PlayerRatings;
	};
}
