#include "DataModel/PlayersStorage.h"

#if defined(_WIN32) || defined(_WIN64)
	#define _CRTDBG_MAP_ALLOC
	#include <stdlib.h>
	#include <crtdbg.h>
#endif

void RunSimulation();

int main()
{
	RunSimulation();
#if defined(_WIN32) || defined(_WIN64)
	_CrtDumpMemoryLeaks();
#endif
}

void RunSimulation()
{
}
