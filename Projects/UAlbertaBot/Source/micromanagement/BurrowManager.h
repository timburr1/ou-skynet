#ifndef BURROWMANAGER_H_
#define BURROWMANAGER_H_
#include <Common.h>
#include <BWAPI.h>
#include "../MapGrid.h"
#include "../UnitInfoState.h"


class BurrowManager {
	
	BurrowManager();

	int hydraRange;

	static BurrowManager * instance;

	void doBurrow(BWAPI::Unit * unit, UnitVector & enemyNear);

public:

	BWAPI::Unit * exception;

	static BurrowManager * getInstance();

	void update();
	void clearExceptions();
	void addException(BWAPI::Unit * unit);

 
};
#endif

