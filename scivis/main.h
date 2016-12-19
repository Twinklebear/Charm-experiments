#pragma once

#include <chrono>
#include <cstdint>
#include <vector>

class Main : public CBase_Main {
	uint64_t spp;

public:
	Main(CkArgMsg *msg);
	Main(CkMigrateMessage *msg);
};


