#ifndef SCOURGEMANAGER_H_
#define SCOURGEMANAGER_H_
#include <Common.h>
#include <BWAPI.h>
#include "MicroManager.h"


class ScourgeManager : public MicroManager
{

public:

    ScourgeManager();
    virtual ~ScourgeManager(){}
    void executeMicro(const UnitVector & targets);

private:

	BWAPI::Unit *		getClosestThreat(BWAPI::Unit * scourge);
	BWAPI::Unit *		getClosestTarget(UnitVector & airUnits, BWAPI::Unit * scourge);

	BWAPI::Position		getFleePosition(BWAPI::Unit * scourge, BWAPI::Unit * threat);
};
#endif

