#pragma once;

#include <Common.h>
#include "MicroManager.h"

class MicroManager;

class CompareDragoonAttackTarget {

public:

	const UnitVector & dragoons;

	// constructor, takes in a zergling to compare distance to
	CompareDragoonAttackTarget(const UnitVector & z) : dragoons(z) {}

	// the sorting operator
	bool operator() (BWAPI::Unit * u1, BWAPI::Unit * u2) {

		int p1 = getAttackPriority(u1);
		int p2 = getAttackPriority(u2);

		if (p1 != p2)
		{
			return p1 > p2;
		}
		else
		{
			return distanceToClosestDragoon(u1) < distanceToClosestDragoon(u2); 	
		}
    }

	// get the attack priority of a type in relation to a zergling
	int getAttackPriority(BWAPI::Unit * unit) {

		BWAPI::UnitType type = unit->getType();

		//UnitInfoVector & enemyDetectors = UnitInfoState::getInstance()->getEnemyDetectors();

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

	double distanceToClosestDragoon(BWAPI::Unit * unit)
	{
		double minDistance = 0;

		BOOST_FOREACH (BWAPI::Unit * dragoon, dragoons)
		{
			double distance = dragoon->getDistance(unit);
			if (!minDistance || distance < minDistance)
			{
				minDistance = distance;
			}
		}
	
		return minDistance;
	}
};

class DragoonManager : public MicroManager
{
public:

	DragoonManager();
	~DragoonManager() {}
	void executeMicro(const UnitVector & targets, BWAPI::Position regroup = BWAPI::Position(0,0));

	BWAPI::Unit * chooseTarget(BWAPI::Unit * dragoon, const UnitVector & targets, std::map<BWAPI::Unit *, int> & numTargeting);
	BWAPI::Unit * closestDragoon(BWAPI::Unit * target, std::set<BWAPI::Unit *> & dragoonsToAssign);

	int getAttackPriority(BWAPI::Unit * unit);
	BWAPI::Unit * getTarget(BWAPI::Unit * dragoon, UnitVector & targets);
};
