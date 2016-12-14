#pragma once

class Hello : public CBase_Hello {
public:
	Hello();
	Hello(CkMigrateMessage *msg);

	// The entry method called from other chares
	void sayHi();
};

