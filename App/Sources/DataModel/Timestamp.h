#pragma once

struct Timestamp
{
	void Tick() { m_Time++; }
	bool Rollback(int delta)
	{
		if (m_Time < delta)
		{
			return false;
		}

		m_Time -= delta;
		return true;
	}

	bool operator==(const Timestamp& other) const { return m_Time == other.m_Time; }
	bool operator!=(const Timestamp& other) const { return m_Time != other.m_Time; }
	bool operator<(const Timestamp& other) const { return m_Time < other.m_Time; }
	bool operator>(const Timestamp& other) const { return m_Time > other.m_Time; }
	bool operator<=(const Timestamp& other) const { return m_Time <= other.m_Time; }
	bool operator>=(const Timestamp& other) const { return m_Time >= other.m_Time; }

	int m_Time;
};