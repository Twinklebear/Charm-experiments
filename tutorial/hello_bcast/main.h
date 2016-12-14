#pragma once

class Main : public CBase_Main {
	int numElements;
	int doneCount;

public:
	Main(CkArgMsg *msg);
	Main(CkMigrateMessage *msg);

	void done();
};

