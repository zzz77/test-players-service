#pragma once

#include "Timestamp.h"

#include <map>

namespace ps
{
	class PlayerInfo
	{
	public:
		void RegisterRating(Timestamp time, int rating);
		void MarkDeleted(Timestamp time);

		// TODO: Introduce helper class Result for proper error reporting
		int GetRating(Timestamp time) const;

	private:
		struct State
		{
			// TODO: Possibly to store m_Deleted as a special rating value IF we know the interval of valid rating values
			bool m_Deleted;
			int m_Rating;
		};

		std::map<Timestamp, State> m_RatingHistory;

		// ... store any other player related information ... 
	};
}
