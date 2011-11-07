#include <Common.h>
#include <iostream>
#include <cmath>
#include <BWTA.h>
#include "HydraliskManager.h"
#include "MicroUtil.h"

using namespace BWAPI;

HydraliskManager::HydraliskManager() {

	speed =		BWAPI::UnitTypes::Zerg_Hydralisk.topSpeed();
	cooldown =	BWAPI::UnitTypes::Zerg_Hydralisk.groundWeapon().damageCooldown();
	range =		BWAPI::UnitTypes::Zerg_Hydralisk.groundWeapon().maxRange();
}

void HydraliskManager::executeMicro(const UnitVector & targets) {

	// the hydralisks
	const UnitVector & hydras = getUnits();

	if (BWAPI::Broodwar->self()->getUpgradeLevel(BWAPI::UpgradeTypes::Muscular_Augments) > 0) {
		speed = BWAPI::UnitTypes::Terran_SCV.topSpeed() * 1.105f;
	}

	if (BWAPI::Broodwar->self()->getUpgradeLevel(BWAPI::UpgradeTypes::Grooved_Spines) > 0) {
		range = 6*32;
	}

	// the target they will attack
	BWAPI::Unit * target = chooseTarget(hydras, targets);

	// if there are no targets, return
	if (!target) { return; }

	// do the hydra dance
	executeHydraliskDance(hydras, target, targets);

	//firebats, dark templar, zealot, ultralisks, maybe mutas.
}

void HydraliskManager::executeHydraliskDance(const UnitVector & hydralisks, BWAPI::Unit * target, const UnitVector & targets) 
{
	drawDebugVectors = true;
	// Used to determine when to issue attack commands
	const int latency(BWAPI::Broodwar->getLatency());

	// clear and re-fill the air threats vector
	threats.clear();
	fillGroundThreats(threats, target->getPosition());

	// Determine behavior for each mutalisk individually
	foreach(BWAPI::Unit * hydralisk, hydralisks) 
	{
		// this is hack for now that prevents hydras from running through chokes without attack
		target = newChooseTarget(hydralisk, targets);
		if (!target) continue;

		// If we were to issue the attack order this frame, how many frames would it take to enter firing range?
		const double distanceToTarget(hydralisk->getDistance(target));
		const double distanceToFiringRange(std::max(distanceToTarget - range,0.0));
		const double timeToEnterFiringRange(distanceToFiringRange / speed);
		const int framesToAttack(static_cast<int>(timeToEnterFiringRange) + 2*latency);

		// How many frames are left before we can attack?
		const double currentCooldown = hydralisk->isStartingAttack() ? cooldown : hydralisk->getGroundWeaponCooldown();

		if (currentCooldown <= latency) {

			// if we can attack, attack it
			hydralisk->attackUnit(target);
			if (drawDebugVectors) {
				BWAPI::Broodwar->drawLineMap(hydralisk->getPosition().x(), hydralisk->getPosition().y(), target->getPosition().x(), target->getPosition().y(), BWAPI::Colors::Red);
			}

		} else {

			hydralisk->rightClick(calcFleePosition(hydralisk, target));
		}

		
	}
}
// fills the GroundThreat vector within a radius of a target
void HydraliskManager::fillGroundThreats(std::vector<GroundThreat> & threats, BWAPI::Position target)
{
	// radius of caring
	const int radiusWeCareAbout(1000);
	const int radiusSq(radiusWeCareAbout * radiusWeCareAbout);

	// for each of the candidate units
	const std::set<BWAPI::Unit*> & candidates(BWAPI::Broodwar->enemy()->getUnits());
	foreach(BWAPI::Unit * e, candidates)
	{
		// if they're not within the radius of caring, who cares?
		const BWAPI::Position delta(e->getPosition() - target);
		if(delta.x() * delta.x() + delta.y() * delta.y() > radiusSq)
		{
			continue;
		}

		// default threat
		GroundThreat threat;
		threat.unit		= e;
		threat.weight	= 1;

		// get the air weapon of the unit
		BWAPI::WeaponType groundWeapon(e->getType().groundWeapon());

		// if it's a bunker, weight it as if it were 4 marines
		if(e->getType() == BWAPI::UnitTypes::Terran_Bunker)
		{
			groundWeapon	= BWAPI::WeaponTypes::Gauss_Rifle;
			threat.weight	= 4;
		}

		// weight the threat based on the highest DPS
		if(groundWeapon != BWAPI::WeaponTypes::None && !e->getType().isWorker())
		{
			threat.weight *= (static_cast<double>(groundWeapon.damageAmount()) / groundWeapon.damageCooldown());
			threats.push_back(threat);
		}
	}
}

BWAPI::Position HydraliskManager::calcFleePosition(BWAPI::Unit * hydralisk, BWAPI::Unit * target) {

	// calculate the standard flee vector
	double2 fleeVector = getFleeVector(threats, hydralisk);

	// vector to the target, if one exists
	double2 targetVector(0,0);

	// normalise the target vector
	if (target) {
		targetVector = target->getPosition() - hydralisk->getPosition();
		targetVector.normalise();
	}

	// rotate the flee vector by 30 degrees, this will allow us to circle around and come back at a target
	//fleeVector.rotate(30);
	double2 newFleeVector;
		
	// keep rotating the vector until the new position is able to be walked on
	for (int r=0; r<360; r+=10) {

		// rotate the flee vector
		fleeVector.rotate(r);

		// re-normalize it
		fleeVector.normalise();

		// new vector should semi point back at the target
		newFleeVector = fleeVector * 2 + targetVector;

		// the position we will attempt to go to
		BWAPI::Position test(hydralisk->getPosition() + newFleeVector * 24);

		// draw the debug vector
		if (drawDebugVectors) {
			BWAPI::Broodwar->drawLineMap(hydralisk->getPosition().x(), hydralisk->getPosition().y(), test.x(), test.y(), BWAPI::Colors::Cyan);
		}

		// if the position is able to be walked on, break out of the loop
		if (checkPositionWalkable(test)) {
			break;
		}
	}

	// go to the calculated 'good' position
	BWAPI::Position fleeTo(hydralisk->getPosition() + newFleeVector * 24);
	
	// draw the vector
	if (drawDebugVectors) {
		BWAPI::Broodwar->drawLineMap(hydralisk->getPosition().x(), hydralisk->getPosition().y(), fleeTo.x(), fleeTo.y(), BWAPI::Colors::Orange);
	}

	return fleeTo;
}

// gets the direction to flee away from a vector of GroundThreats
double2 HydraliskManager::getFleeVector(const std::vector<GroundThreat> & threats, BWAPI::Unit * hydralisk)
{
	double2 fleeVector(0,0);

	foreach(const GroundThreat & threat, threats)
	{
		// Get direction from enemy to mutalisk
		const double2 direction(hydralisk->getPosition() - threat.unit->getPosition());

		// Divide direction by square distance, weighting closer enemies higher
		//  Dividing once normalises the vector
		//  Dividing a second time reduces the effect of far away units
		const double distanceSq(direction.lenSq());
		if(distanceSq > 0)
		{
			// Enemy influence is direction to flee from enemy weighted by danger posed by enemy
			const double2 enemyInfluence( (direction / distanceSq) * threat.weight );

			fleeVector = fleeVector + enemyInfluence;
		}
	}

	if(fleeVector.lenSq() == 0)
	{
		// Flee towards our base
		fleeVector = double2(1,0);	
	}

	fleeVector.normalise();

	BWAPI::Position p1(hydralisk->getPosition());
	BWAPI::Position p2(p1 + fleeVector * 100);

	return fleeVector;
}

BWAPI::Unit * HydraliskManager::chooseTarget(const UnitVector & hydras, const UnitVector & targets) {

	BWAPI::Unit * target(0);
	double targetDistance = 100000.0;
	int targetPriority;

	// get the center
	BWAPI::Position center = calcCenter();

	//loop through the targets and target the closest, highest priority unit
	foreach(BWAPI::Unit * unit, targets)
	{
		// Additions by Sterling: Simple targeting priority system
		const BWAPI::UnitType type(unit->getType());
		int priority(0);
		
		if (type == BWAPI::UnitTypes::Protoss_Reaver) { 
			priority = 10;
		} else if (type == BWAPI::UnitTypes::Terran_Medic || type.groundWeapon() != BWAPI::WeaponTypes::None || type ==  BWAPI::UnitTypes::Terran_Bunker) {
			priority = 9;
		} else if (type.isWorker()) {
			priority = 8;
		} else if (type ==  BWAPI::UnitTypes::Protoss_High_Templar) {
			priority = 7;
		} else if (type ==  BWAPI::UnitTypes::Protoss_Photon_Cannon || type == BWAPI::UnitTypes::Zerg_Sunken_Colony) {
			priority = 3;
		} else if (type.groundWeapon() != BWAPI::WeaponTypes::None) {
			priority = 2;
		} else if (type.supplyProvided() > 0) {
			priority = 1;
		} 

		if(unit->getTransport()){
			// we can't attack units in bunkers
			continue;
		}
		if(unit->isCloaked() || unit->isBurrowed())
		{
			//get detector in squad
			//BWAPI::Broodwar->printf("CLOAKED UNIT DETECTED: %s", unit->getType().getName().c_str());
			
		}
		if(!unit->isDetected())//can't attack cloaked units
			continue;
		

		const double distance(sumSquaredDistance(hydras, unit));
		if(!target || priority > targetPriority || (priority == targetPriority && distance < targetDistance))
		{
			target			= unit;
			targetPriority	= priority;
			targetDistance	= distance;
		}
	}

	return target;
}

BWAPI::Unit * HydraliskManager::newChooseTarget(BWAPI::Unit * hydra, const UnitVector & targets) {

	BWAPI::Unit * target(0);
	double targetDistance = 100000.0;
	int targetPriority;
	int targetHitPoints = 100000;

	//loop through the targets and target the closest, highest priority unit
	foreach(BWAPI::Unit * unit, targets)
	{
		// Additions by Sterling: Simple targeting priority system
		const BWAPI::UnitType type(unit->getType());
		int priority(0);

		if (type == BWAPI::UnitTypes::Terran_Medic || type.groundWeapon() != BWAPI::WeaponTypes::None || type ==  BWAPI::UnitTypes::Terran_Bunker) {
			priority = 9;
		} else if (type.isWorker()) {
			priority = 8;
		} else if (type ==  BWAPI::UnitTypes::Protoss_High_Templar) {
			priority = 7;
		} else if (type.spaceProvided() > 0) {
			priority = 6;
		} else if (type.supplyProvided() > 0) {
			priority = 1;
		} 

		if (unit->getDistance(hydra) > 500) {
			priority = 2;
		}

		if(unit->getTransport() || !unit->isDetected()){
		
			continue;
		}

		bool targetInRange = unit->getDistance(hydra) <= range;
		const double distance(unit->getDistance(hydra));
		if((!target) || 
			(priority > targetPriority) ||
			(targetInRange && priority == targetPriority && target->getHitPoints() < targetHitPoints) ||
			(!targetInRange && priority == targetPriority && distance < targetDistance)	)
		{
				target			= unit;
				targetPriority	= priority;
				targetDistance	= distance;
				if (targetInRange) targetHitPoints = target->getHitPoints();
		}
	}

	return target;
}

int HydraliskManager::sumSquaredDistance(const UnitVector & hydras, BWAPI::Unit * target) {

	int sum = 0;
	int tx(target->getPosition().x()), ty(target->getPosition().y());

	for (size_t i(0); i<hydras.size(); ++i) {
		
		int xdiff(hydras[i]->getPosition().x() - tx);
		int ydiff(hydras[i]->getPosition().y() - ty);

		sum += xdiff*xdiff + ydiff*ydiff;
	}

	return sum;
}
/*	

	Unit * nearTarget = NULL;
	int nearUnitHealth = 10000;
	Unit * farTarget = NULL

	foreach(BWAPI::Unit * unit, targets) {
		// Additions by Sterling: Simple targeting priority system
		const BWAPI::UnitType type(unit->getType());
		
		int unitHealth = unit->getHitPoints() + unit->getShields();
		if (

		if(unit->getTransport()) {
			// we can't attack units in bunkers
			continue;
		}

		const int distance(unit->getDistance(center));
		if(!target || priority > targetPriority || (priority == targetPriority && distance < targetDistance))
		{
			target			= unit;
			targetPriority	= priority;
			targetDistance	= distance;
		}
	}
*/
/*}

bool HydraliskManager::checkBounds(int i, int j) {

	if (i < 0) return false;
	if (j < 0) return false;
	if (i >= gridX) return false;
	if (j >= gridY) return false;

	return true;
}*/