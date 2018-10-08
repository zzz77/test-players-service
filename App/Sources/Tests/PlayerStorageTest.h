#pragma once

namespace pst
{
	class PlayerStorageTest
	{
	public:
		static void Run();

	private:
		static void TestRegistration();
		static void TestRollback();
	};
}
