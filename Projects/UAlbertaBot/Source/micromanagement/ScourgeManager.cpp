#include <Common.h>
#include <iostream>
#include <cmath>
#include <BWTA.h>
#include "ScourgeManager.h"


ScourgeManager::ScourgeManager() { }

void ScourgeManager::executeMicro(const UnitVector & targets) 
{
/*	assert(!targets.empty());

	// Get our mutalisks
	const UnitVector & scourge = getUnits();

	UnitVector enemyAirUnits;
	foreach (BWAPI::Unit * unit, BWAPI::Broodwar->enemy()->getUnits()) {

		if (unit->getType().isFlyer()) {
			enemyAirUnits.push_back(unit);
		}
	}

	// for each scourge
	for (size_t i(0); i<scourge.size(); ++i) {

		// get the closest threat to it
		BWAPI::Unit * threat = getClosestThreat(scourge[i]);

		// if it exists, get away from it
		if (threat) {

			scourge[i]->rightClick(getFleePosition(scourge[i], threat));

		// otherwise there is no threat
		} else {

			BWAPI::Unit * target = getClosestTarget(enemyAirUnits, scourge[i]);

			// if there's a target, go get it!
			if (target) {

				scourge[i]->attackUnit(target);

			// otherwise, go to our rally position
			} else {

				//BWAPI::Position patrol(order.position.x() - i*200, order.position.y() - i*200);
				BWAPI::Position start(GameState::getInstance()->getStartRegion()->getCenter());

				BWAPI::Position patrol(start.x() - i*50, start.y() - i*50);
				scourge[i]->rightClick(patrol);
			}
		}
	}*/
}

BWAPI::Unit * ScourgeManager::getClosestTarget(UnitVector & airUnits, BWAPI::Unit * scourge) {

	BWAPI::Unit * target = NULL;
	double minDistance = 1000000;

	for (size_t i(0); i<airUnits.size(); ++i) {
	
		if (!target || airUnits[i]->getDistance(scourge) < minDistance) {
			minDistance = airUnits[i]->getDistance(scourge);
			target = airUnits[i];
		}
	}

	return target;
}

BWAPI::Unit * ScourgeManager::getClosestThreat(BWAPI::Unit * scourge) {

	// range buffer to prevent lag from killing us
	int rangeBuffer = 100;

	// get the units in range of the scourge
	UnitVector inRange;
	MapGrid::getInstance()->GetUnits(inRange, scourge->getPosition(), BWAPI::UnitTypes::Protoss_Photon_Cannon.airWeapon().maxRange() + 100, false, true);

	BWAPI::Unit * closest = NULL;
	double closestDistance = 1000000;

	// for each of the units in range
	foreach (BWAPI::Unit * unit, inRange) {

		BWAPI::WeaponType airWeapon = unit->getType().airWeapon();
		double distance = unit->getDistance(scourge);
	
		// if it's got an air weapon and it isn't a flyer, we care about it
		if (unit->getType() == BWAPI::UnitTypes::Terran_Bunker || (airWeapon != BWAPI::WeaponTypes::None && !unit->getType().isFlyer())) {
			
			// if it's the closest unit so far
			if ((!closest || distance < closestDistance)) {
				closest = unit;
				closestDistance = distance;
			}
		}
	}

	return closest;
}

BWAPI::Position ScourgeManager::getFleePosition(BWAPI::Unit * scourge, BWAPI::Unit * threat)
{
	// Get direction from enemy to mutalisk
	double2 direction(scourge->getPosition() - threat->getPosition());

	direction.normalise();

	direction = direction * 32;

	return BWAPI::Position(scourge->getPosition() + direction);
}