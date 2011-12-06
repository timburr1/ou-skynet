#ifndef HYDRALISKMANAGER_H_
#define HYDRALISKMANAGER_H_
#include <Common.h>
#include <BWAPI.h>
#include "MicroManager.h"

class HydraliskManager : public MicroManager
{

public:

	HydraliskManager();
	virtual ~HydraliskManager(){}

	void executeMicro(const UnitVector & targets);

private:

	double speed, cooldown, range;
	std::vector<GroundThreat> threats;

	BWAPI::Unit * chooseTarget(const UnitVector& hydras, const UnitVector & targets);
	BWAPI::Unit * newChooseTarget(BWAPI::Unit * hydra, const UnitVector & targets);

	int HydraliskManager::sumSquaredDistance(const UnitVector & hydras, BWAPI::Unit * target);

	
	double2 getFleeVector(const std::vector<GroundThreat> & threats, BWAPI::Unit * hydralisk);

	void fillGroundThreats(std::vector<GroundThreat> & threats, BWAPI::Position pos);
	void executeHydraliskDance(const UnitVector & hydralisks, BWAPI::Unit * target, const UnitVector & targets) ;
	BWAPI::Position HydraliskManager::calcFleePosition(BWAPI::Unit * hydralisk, BWAPI::Unit * target);

};
#endif

