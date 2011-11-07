#ifndef ZERGLINGMANAGER_H_
#define ZERGLINGMANAGER_H_
#include <Common.h>
#include <BWAPI.h>
#include "MicroManager.h"

class CompareZerglingAttackTarget {
	
	// the zergling we are comparing this to
	BWAPI::Unit * zergling;

public:

	// constructor, takes in a zergling to compare distance to
	CompareZerglingAttackTarget(BWAPI::Unit * z) {
		zergling = z;
	}

	// the sorting operator
	bool operator() (BWAPI::Unit * u1, BWAPI::Unit * u2) {

		int p1 = getAttackPriority(u1->getType());
		int p2 = getAttackPriority(u2->getType());

		return p1 > p2;
    }

	// get the attack priority of a type in relation to a zergling
	int getAttackPriority(BWAPI::UnitType type) {

		if (type.isWorker()) {
			return 8;
		} else if (type == BWAPI::UnitTypes::Terran_Medic || type.groundWeapon() != BWAPI::WeaponTypes::None || type ==  BWAPI::UnitTypes::Terran_Bunker) {
			return 9;
		} else if (type ==  BWAPI::UnitTypes::Protoss_High_Templar) {
			return 7;
		} else if (type ==  BWAPI::UnitTypes::Protoss_Photon_Cannon || type == BWAPI::UnitTypes::Zerg_Sunken_Colony) {
			return 3;
		} else if (type.groundWeapon() != BWAPI::WeaponTypes::None) {
			return 2;
		} else if (type.supplyProvided() > 0) {
			return 1;
		} 
		
		return 0;
	}
};

class CompareZerglingAttackTargetSSD {
	
	// the zergling we are comparing this to
	const UnitVector & zerglings;

public:

	// constructor, takes in a zergling to compare distance to
	CompareZerglingAttackTargetSSD(const UnitVector & z) : zerglings(z) {}

	// the sorting operator
	bool operator() (BWAPI::Unit * u1, BWAPI::Unit * u2) {

		int p1 = getAttackPriority(u1->getType());
		int p2 = getAttackPriority(u2->getType());

		if (p1 != p2) {
			return p1 > p2;
		} else {
			return ssd(u1) < ssd(u2);
		}
    }

	// get the attack priority of a type in relation to a zergling
	int getAttackPriority(BWAPI::UnitType type) {

		if (type == BWAPI::UnitTypes::Protoss_Reaver)				{ return 10; } 
		else if (type == BWAPI::UnitTypes::Terran_Medic 
			|| type.groundWeapon() != BWAPI::WeaponTypes::None)		{ return 9; } 
		else if (type == BWAPI::UnitTypes::Protoss_Dragoon)			{ return 9; } 
		else if (type.isWorker())									{ return 8; } 
		else if (type ==  BWAPI::UnitTypes::Protoss_High_Templar)	{ return 7; } 
		else if (type ==  BWAPI::UnitTypes::Terran_Bunker)			{ return 6; } 
		else if (type.supplyProvided() > 0)							{ return 5; }
		else														{ return 0; }
	}

	int ssd(BWAPI::Unit * target) {

		int sum = 0;
		for (size_t i(0); i<zerglings.size(); ++i) {
			
			int xdiff(zerglings[i]->getPosition().x() - target->getPosition().x());
			int ydiff(zerglings[i]->getPosition().y() - target->getPosition().y());

			sum += xdiff*xdiff + ydiff*ydiff;
		}

		return sum;
	}
};

class ZerglingManager : public MicroManager
{
public:

	ZerglingManager();
	~ZerglingManager() {}
	void executeMicro(const UnitVector & targets);

	std::vector<GroundThreat> threats;
	void fillGroundThreats(std::vector<GroundThreat> & threats, BWAPI::Position target);
	double2 getFleeVector(const std::vector<GroundThreat> & threats, BWAPI::Unit * zergling);
	BWAPI::Unit * immediateThreat(std::vector<GroundThreat> & threats, BWAPI::Unit * zergling);

	BWAPI::Position calcFleePosition(BWAPI::Unit * zergling, BWAPI::Unit * target);

	BWAPI::Unit * getClosestHarassTarget(UnitVector & targets, BWAPI::Unit * zergling, int workerFirst);
	BWAPI::Unit * getClosestAttackTarget(UnitVector & targets, BWAPI::Unit * zergling);
};

#endif

