#include "PlayerInfo.h"

#include <cassert>

void ps::PlayerInfo::RegisterRating(Timestamp time, int rating)
{
	State& state = m_RatingHistory[time];
	state.m_Rating = rating;
	state.m_Deleted = false;
}

void ps::PlayerInfo::MarkDeleted(Timestamp time)
{
	// 1. Iterator will point to either state we have to rewrite or next state, or end.
	auto iter = m_RatingHistory.lower_bound(time);

	// 2. If iterator pointing to the first item then we're trying to rewrite first state so player doesn't exist yet!
	if (iter == m_RatingHistory.begin())
	{
		return;
	}

	// 3. Iterator will point to last valid state.
	assert(!m_RatingHistory.empty());
	iter--;

	// 4. Trying to delete player which is already deleted!
	if (iter->second.m_Deleted)
	{
		return;
	}

	// 5. Create new item (or aqcuire iterator to correct one).
	iter++;
	iter = m_RatingHistory.try_emplace(iter, time);

	// 6. Actually mark player as deleted.
	iter->second.m_Deleted = true;
}

int ps::PlayerInfo::GetRating(Timestamp time) const
{
	// 1. Now ititerator points to next state (or end).
	auto iter = m_RatingHistory.upper_bound(time);

	// 2. If this is the first state in history of this player then currently player doesn't exist yet.
	if (iter == m_RatingHistory.begin())
	{
		return -1;
	}

	// 3. Now iterator points to last valid state.
	assert(!m_RatingHistory.empty());
	iter--;

	// 4. Player has been deleted sometime ago.
	if (iter->second.m_Deleted)
	{
		return -1;
	}

	return iter->second.m_Rating;
}
