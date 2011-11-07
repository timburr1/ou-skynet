#pragma once

#include <Common.h>
#include "MicroManager.h"
#include "../UnitInfoState.h"

class MicroManager;

class CompareDarkTemplarAttackTarget {

public:


	const UnitVector & darkTemplars;

	// constructor, takes in a zergling to compare distance to
	CompareDarkTemplarAttackTarget(const UnitVector & z) : darkTemplars(z) {}

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
			return distanceToClosestDarkTemplar(u1) < distanceToClosestDarkTemplar(u2); 	
		}
    }

	// get the attack priority of a type in relation to a zergling
	int getAttackPriority(BWAPI::Unit * unit) {

		BWAPI::UnitType type = unit->getType();

		//UnitInfoVector & enemyDetectors = UnitInfoState::getInstance()->getEnemyDetectors();

		if (type ==  BWAPI::UnitTypes::Protoss_Photon_Cannon)
		{
			return 15;
		}
		if (type == BWAPI::UnitTypes::Protoss_Probe || type == BWAPI::UnitTypes::Zerg_Drone)
		{
			return 10;
		}
		else if (type.isWorker()) 
		{
			return 8;
		} 
		else if (type == BWAPI::UnitTypes::Terran_Medic || type.groundWeapon() != BWAPI::WeaponTypes::None || type ==  BWAPI::UnitTypes::Terran_Bunker) 
		{
			return 9;
		} 
		else if (type ==  BWAPI::UnitTypes::Protoss_High_Templar) 
		{
			return 7;
		} 
		else if (type ==  BWAPI::UnitTypes::Protoss_Photon_Cannon || type == BWAPI::UnitTypes::Zerg_Sunken_Colony) 
		{
			return 3;
		} 
		else if (type.groundWeapon() != BWAPI::WeaponTypes::None) 
		{
			return 2;
		} 
		else if (type.supplyProvided() > 0) 
		{
			return 1;
		} 
		
		return 0;
	}

	double distanceToClosestDarkTemplar(BWAPI::Unit * unit)
	{
		double minDistance = 0;

		BOOST_FOREACH (BWAPI::Unit * darkTemplar, darkTemplars)
		{
			double distance = darkTemplar->getDistance(unit);
			if (!minDistance || distance < minDistance)
			{
				minDistance = distance;
			}
		}
	
		return minDistance;
	}
};

class DarkTemplarManager : public MicroManager
{
public:

	
	bool goForIt;

	DarkTemplarManager();
	~DarkTemplarManager() {}
	void executeMicro(const UnitVector & targets, BWAPI::Position regroup = BWAPI::Position(0,0));

	BWAPI::Unit * chooseTarget(BWAPI::Unit * zealot, const UnitVector & targets, std::map<BWAPI::Unit *, int> & numTargeting);
	BWAPI::Unit * closestDarkTemplar(BWAPI::Unit * target, std::set<BWAPI::Unit *> & zealotsToAssign);
	int getAttackPriority(BWAPI::Unit * unit);
	BWAPI::Unit * getTarget(BWAPI::Unit * zealot, UnitVector & targets);
};
