#include "Tests/PersistentMapTest.h"
#include "Tests/PlayerStorageTest.h"

#if defined(_WIN32) || defined(_WIN64)
	#define _CRTDBG_MAP_ALLOC
	#include <stdlib.h>
	#include <crtdbg.h>
#endif

int main()
{
	pst::PersistentMapTest::Run();
	pst::PlayerStorageTest::Run();

#if defined(_WIN32) || defined(_WIN64)
	_CrtDumpMemoryLeaks();
#endif
}
