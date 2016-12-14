#include "hello.decl.h"
#include "main.decl.h"

#include "hello.h"

// readonly Charm++ globals
extern CProxy_Main mainProxy;

Hello::Hello() {}
Hello::Hello(CkMigrateMessage *msg) {}

void Hello::sayHi() {
	CkPrintf("'Hello' from Hello chare #%d on processor %d\n",
			thisIndex, CkMyPe());
	
	mainProxy.done();
}

#include "hello.def.h"

