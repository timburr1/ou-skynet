#include "Common.h"
#include "BurrowManager.h"

using namespace BWAPI;

BurrowManager * BurrowManager::instance = NULL;

BurrowManager::BurrowManager() {

	hydraRange = UnitTypes::Zerg_Hydralisk.groundWeapon().maxRange();

	exception = NULL;
}

// get an instance of this
BurrowManager * BurrowManager::getInstance() {

	// if the instance doesn't exist, create it
	if (!BurrowManager::instance) {
		BurrowManager::instance = new BurrowManager();
	}

	return BurrowManager::instance;
}

void BurrowManager::update() {

	if (!Broodwar->self()->hasResearched(TechTypes::Burrowing)) return;

	// for each of our units
	foreach(BWAPI::Unit * unit, BWAPI::Broodwar->self()->getUnits()) {

		int maxRadius = 13*32;

		// get all enemy units within a radius
		UnitVector unitsNear;
		MapGrid::getInstance()->GetUnits(unitsNear, unit->getPosition(), maxRadius, false, true);

		doBurrow(unit, unitsNear);
	}
}

void BurrowManager::doBurrow(BWAPI::Unit * unit, UnitVector & unitsNear) {

/*	bool detectorNear = false;
	bool droneHide = false;
	bool zerglingHide = false;
	bool hydraliskHide = false;

	BWAPI::UnitType type = unit->getType();

	// for each enemy unit in range of our unit
	foreach (BWAPI::Unit * oppUnit, unitsNear) {

		// is it a detector?
		if (oppUnit->getType().isDetector() && oppUnit->getDistance(unit) <= oppUnit->getType().sightRange()) { detectorNear = true; }

		// get the ground weapon of the unit
		BWAPI::WeaponType oppWeapon = oppUnit->getType().groundWeapon();
		double oppRange = oppUnit->getType().groundWeapon().maxRange();
		
		// if it doesn't have a ground weapon we don't really care about it
		if (oppWeapon == BWAPI::WeaponTypes::None || oppUnit->getType().isWorker()) { continue; }
		
		// get the distance
		double distance = oppUnit->getDistance(unit);

		if (distance < oppRange + 80) { 
			BWAPI::Broodwar->drawCircleMap(oppUnit->getPosition().x(), oppUnit->getPosition().y(), (int)oppRange+80, BWAPI::Colors::Red);
			droneHide = true; 
		}

		if (distance < oppRange + 60) { zerglingHide = true; }
		if (distance < oppRange + 80) { hydraliskHide = true; }
	}

	// decide what to do if we are unburrowed
	if (!unit->isBurrowed() && !detectorNear) {

		// Burrow a drone if something is close
		if (type == UnitTypes::Zerg_Drone && droneHide) {
			unit->burrow();

		// Burrow a hydra if something is close and it's low on HP
		} else if (type == UnitTypes::Zerg_Hydralisk && hydraliskHide && unit->getHitPoints() < 50) {
			unit->burrow(); 

		// Burrow a zergling if something is close and it is harassing
		} else if (type == UnitTypes::Zerg_Zergling && zerglingHide && 
			((unitOrder == SquadOrder::ZerglingHarass) || (unitOrder == SquadOrder::ZerglingScout))) {
			unit->burrow(); 
		}
	} 
	
	
	// rules to un-burrow
	if (unit->isBurrowed()) {

		if (type == UnitTypes::Zerg_Drone && !droneHide) {
			unit->unburrow();
		} else if (type == UnitTypes::Zerg_Hydralisk && !hydraliskHide) {
			unit->unburrow(); 
		} else if (type == UnitTypes::Zerg_Zergling && !zerglingHide && !(unitOrder == SquadOrder::Burrow)) {
			unit->unburrow(); 
		}

		if (detectorNear) {
			unit->unburrow();
		} 
	}*/
}
