#include "hello.decl.h"
#include "main.decl.h"

#include "hello.h"

// readonly Charm++ globals
extern CProxy_Main mainProxy;
extern int numElements;

Hello::Hello() {}
Hello::Hello(CkMigrateMessage *msg) {}

void Hello::sayHi(int from) {
	CkPrintf("'Hello' from Hello chare #%d on processor %d (told by %d)\n",
			thisIndex, CkMyPe(), from);

	// Tell the next chare to say hello if we're not the last one
	if (thisIndex < numElements - 1) {
		thisProxy[thisIndex + 1].sayHi(thisIndex);
	} else {
		mainProxy.done();
	}
}

#include "hello.def.h"

