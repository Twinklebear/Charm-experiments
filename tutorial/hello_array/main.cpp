#include <cstdlib>

#include "main.decl.h"
#include "hello.decl.h"
#include "main.h"

// readonly Charm++ vars
CProxy_Main mainProxy;
int numElements;

Main::Main(CkArgMsg *msg) {
	numElements = 5;
	// If the user has passed some args specifying the number of hello Chares to make
	if (msg->argc > 1) {
		numElements = std::atoi(msg->argv[1]);
	}
	delete msg;

	CkPrintf("Running Hello Array with %d elements using %d processors",
			numElements, CkNumPes());

	mainProxy = thisProxy;
	CProxy_Hello helloArray = CProxy_Hello::ckNew(numElements);
	helloArray[0].sayHi(-1);
}
Main::Main(CkMigrateMessage *msg) {}

void Main::done() {
	CkExit();
}

#include "main.def.h"

