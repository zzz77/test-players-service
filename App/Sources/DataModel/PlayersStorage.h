#pragma once

#include "Timestamp.h"

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
		bool UnregisterPlayer(std::string_view playerName);
		bool Rollback(int step);
		int GetPlayerRank(std::string_view playerName) const;
		int GetPlayerRating(std::string_view playerName) const;

	private:
		std::map<std::string, PlayerInfo*, std::less<>> m_Players;

		Timestamp m_CurrentTime;
	};
}
