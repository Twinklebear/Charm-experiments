#include "main.decl.h"
#include "main.h"

Main::Main(CkArgMsg *msg) {
	CkPrintf("Hello world\n");
	CkExit();
}
Main::Main(CkMigrateMessage *msg) {}

#include "main.def.h"

