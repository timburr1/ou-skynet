#include <Common.h>
#include <iostream>
#include <cmath>
#include <list>
#include <vector>
#include <BWTA.h>
#include "ZerglingManager.h"
#include "MicroUtil.h"

ZerglingManager::ZerglingManager() { }



void ZerglingManager::executeMicro(const UnitVector & targets) 
{
	const UnitVector & zerglings = getUnits();

	int range = BWAPI::UnitTypes::Zerg_Zergling.groundWeapon().maxRange();

	BWAPI::Position center = calcCenter();

	BWAPI::Unit * target(0);
	BWAPI::UnitType best;
	//int targetPriority;

	UnitVector harassTargets;
	foreach(BWAPI::Unit * unit, targets) {
		
		if ((unit->getType().isWorker() || unit->getType().groundWeapon() == BWAPI::WeaponTypes::None) && !unit->getType().isFlyer()) {
			harassTargets.push_back(unit);
		}
	}

	// if the order is to harass the enemy location
	if (order.type == order.ZerglingHarass) {

		// fill the ground threats around the order position target
		threats.clear();
		fillGroundThreats(threats, order.position);

		// for each zergling
		for (size_t i(0); i<zerglings.size(); ++i) {

			BWAPI::Unit * zergling = zerglings[i];

			// get the closest target to the zergling
			target = getClosestHarassTarget(harassTargets, zergling, 0);

			// draw the vector to the target
			if (target && drawDebugVectors) {
				BWAPI::Broodwar->drawLineMap(zergling->getPosition().x(), zergling->getPosition().y(), target->getPosition().x(), target->getPosition().y(), BWAPI::Colors::Yellow);
			}

			// if there is an immediate threat, run away
			if (immediateThreat(threats, zergling)) {	

				// if we can immediately attack our target without moving, do so, otherwise flee
				//if (target && zergling->getGroundWeaponCooldown() == 0 && zergling->getDistance(target) <= range) {
				//	zergling->attackUnit(target);
				//	continue;
				//}

				// calculate where to flee
				BWAPI::Position fleeTo = calcFleePosition(zergling, target);

				// move there
				zergling->rightClick(fleeTo);

			// otherwise attack the target if we have a valid one
			} else if (target) {
				zergling->attackUnit(target);

			// otherwise continue moving to the target
			} else {
				zergling->rightClick(order.position);
			}
		}

	// if we're scouting
	} else if (order.type == order.ZerglingScout) {

		for (size_t i(0); i<zerglings.size(); ++i) {
			zerglings[i]->rightClick(order.position);
		}

	// If the order is any normal order, just attack the target unit
	} else {

		UnitVector zerglingTargets;
		for (size_t i(0); i<targets.size(); i++) {
			
			if (!targets[i]->getType().isFlyer()) {
				zerglingTargets.push_back(targets[i]);
			}
		}

		if (zerglingTargets.empty()) return;

		// 6 zerglings per target please
		int zPerTarget(6), assigned(0);

		// sort the targets based on priority and SSD to zergling group
		std::sort(zerglingTargets.begin(), zerglingTargets.end(), CompareZerglingAttackTargetSSD(zerglings));

		// for each zergling we have
		foreach (BWAPI::Unit * zergling, zerglings) {

			int targetIndex = assigned/zPerTarget;

			// if there are still targets to assign
			if (targetIndex < (int)zerglingTargets.size()) {

				// attack the assigned target
				zergling->attackUnit(zerglingTargets[targetIndex]);

				BWAPI::Broodwar->drawLineMap(zergling->getPosition().x(), zergling->getPosition().y(), zerglingTargets[targetIndex]->getPosition().x(), zerglingTargets[targetIndex]->getPosition().y(), BWAPI::Colors::Red);
				
			// otherwise we're out of targets so move to position
			} else {

				zergling->rightClick(order.position);
				BWAPI::Broodwar->drawLineMap(zergling->getPosition().x(), zergling->getPosition().y(), order.position.x(), order.position.y(), BWAPI::Colors::Green);
			}

			assigned++;
		}
	}
}



BWAPI::Position ZerglingManager::calcFleePosition(BWAPI::Unit * zergling, BWAPI::Unit * target) {

	// calculate the standard flee vector
	double2 fleeVector = getFleeVector(threats, zergling);

	// vector to the target, if one exists
	double2 targetVector(0,0);

	// normalise the target vector
	if (target) {
		targetVector = target->getPosition() - zergling->getPosition();
		targetVector.normalise();
	}

	int mult = 1;

	if (zergling->getID() % 2) {
		mult = -1;
	}

	// rotate the flee vector by 30 degrees, this will allow us to circle around and come back at a target
	fleeVector.rotate(mult*30);
	double2 newFleeVector;
		
	// keep rotating the vector until the new position is able to be walked on
	for (int r=0; r<360; r+=10) {

		// rotate the flee vector
		fleeVector.rotate(mult*r);

		// re-normalize it
		fleeVector.normalise();

		// new vector should semi point back at the target
		newFleeVector = fleeVector * 2 + targetVector;

		// the position we will attempt to go to
		BWAPI::Position test(zergling->getPosition() + newFleeVector * 24);

		// draw the debug vector
		if (drawDebugVectors) {
			BWAPI::Broodwar->drawLineMap(zergling->getPosition().x(), zergling->getPosition().y(), test.x(), test.y(), BWAPI::Colors::Cyan);
		}

		// if the position is able to be walked on, break out of the loop
		if (checkPositionWalkable(test)) {
			break;
		}
	}

	// go to the calculated 'good' position
	BWAPI::Position fleeTo(zergling->getPosition() + newFleeVector * 24);
	
	// draw the vector
	if (drawDebugVectors) {
		BWAPI::Broodwar->drawLineMap(zergling->getPosition().x(), zergling->getPosition().y(), fleeTo.x(), fleeTo.y(), BWAPI::Colors::Orange);
	}

	return fleeTo;
}

BWAPI::Unit * ZerglingManager::getClosestHarassTarget(UnitVector & targets, BWAPI::Unit * zergling, int workerFirst) {

	double targetDistance = 100000.0;
	BWAPI::Unit * target(0);
	int targetPriority;

	for (size_t i(0); i<targets.size(); ++i) {

		BWAPI::Unit * unit = targets[i];
		// Additions by Sterling: Simple targeting priority system
		const BWAPI::UnitType type(unit->getType());
		int priority(0);

		if (type.isWorker()) {
			priority = 8 + workerFirst;
		} else if (type.isBuilding() && type.groundWeapon() == BWAPI::WeaponTypes::None) {
			priority = 8;
		} 

		if(unit->getTransport()){
			// we can't attack units in bunkers
			continue;
		}

		const double distance(unit->getDistance(zergling));
		if(!target || priority > targetPriority || (priority == targetPriority && distance < targetDistance))
		{
			target			= unit;
			targetPriority	= priority;
			targetDistance	= distance;
		}
	}

	return target;
}

BWAPI::Unit * ZerglingManager::getClosestAttackTarget(UnitVector & targets, BWAPI::Unit * zergling) {

	double targetDistance = 100000.0;
	BWAPI::Unit * target(0);
	int targetPriority;

	for (size_t i(0); i<targets.size(); ++i) {

		BWAPI::Unit * unit = targets[i];
		int priority = 0;
	
		if(unit->getTransport()){
			// we can't attack units in bunkers
			continue;
		}

		const double distance(unit->getDistance(zergling));
		if(!target || priority > targetPriority || (priority == targetPriority && distance < targetDistance))
		{
			target			= unit;
			targetPriority	= priority;
			targetDistance	= distance;
		}
	}

	return target;
}

// fills the GroundThreat vector within a radius of a target
void ZerglingManager::fillGroundThreats(std::vector<GroundThreat> & threats, BWAPI::Position target)
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

// gets the direction to flee away from a vector of GroundThreats
double2 ZerglingManager::getFleeVector(const std::vector<GroundThreat> & threats, BWAPI::Unit * zergling)
{
	double2 fleeVector(0,0);

	foreach(const GroundThreat & threat, threats)
	{
		// Get direction from enemy to mutalisk
		const double2 direction(zergling->getPosition() - threat.unit->getPosition());

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

	BWAPI::Position p1(zergling->getPosition());
	BWAPI::Position p2(p1 + fleeVector * 100);


	return fleeVector;
}

// returns true if something is capable of attacking the zergling right now
BWAPI::Unit * ZerglingManager::immediateThreat(std::vector<GroundThreat> & threats, BWAPI::Unit * zergling) {

	if (threats.empty()) {
		return NULL;
	}

	double buffer = 130;

	for (size_t i(0); i<threats.size(); ++i) {

		double range = threats[i].unit->getType().groundWeapon().maxRange() + buffer;

		if (zergling->getDistance(threats[i].unit) < range) {
			return threats[i].unit;
		}
	}

	return false;
}

/*
	// find all targets that we are capable of attacking
	std::list<BWAPI::Unit*> targetlist;
	foreach(BWAPI::Unit* enemyUnit, targets){
		const BWAPI::UnitType enemyUnitType = enemyUnit->getType();
		if(!enemyUnitType.isFlyer() && !enemyUnitType.isInvincible())
			targetlist.push_back(enemyUnit);
	}
	if(targetlist.empty())
		return;

	// sort targets by distance
	compDistance.unit = center;
	targetlist.sort(compDistance);

	int numForEach = (int)ceil((double)zerglings.size()/(double)targetlist.size());
	if(numForEach < 6)
		numForEach = 6;
	double angle = 360.0 / (double)(numForEach-1);

	std::vector<BWAPI::Unit *> remaining(zerglings.begin(), zerglings.end());
	while(remaining.size() > 0){
		if(targetlist.empty())
			break;

		BWAPI::Unit* target(0);
		BWAPI::UnitType best;
		int targetPriority;

		double targetDistance = 100000.0;

		//map that will hold all enemy unit types and their target priority
		std::map<BWAPI::UnitType, int> myTargetMap;

		bool canAttack = true;
		//loop through the targets and target the closest, highest priority unit
		foreach(BWAPI::Unit * unit, targets)
		{
			// Additions by Sterling: Simple targeting priority system
			const BWAPI::UnitType type(unit->getType());
			int priority(0);

			if (type ==  BWAPI::UnitTypes::Terran_Medic) {
				priority = 10;
			} else if(type.groundWeapon() != BWAPI::WeaponTypes::None) {
				priority = 9;
			} else if (type.isWorker()) {
				priority = 8;
			}  

			if(unit->getTransport()){
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

		if(target == NULL) return;

		//targetlist.remove(target);

		BWAPI::Position posTarget = target->getPosition();
		compDistance.unit = posTarget;
		sort(remaining.begin(), remaining.end(), compDistance);

		double vx = center.x() - posTarget.x();
		double vy = center.y() - posTarget.y();
		MicroUtil::normalize(vx,vy);

		for(int i = 0; i<numForEach; i++){
			if(remaining.size()<= 0)
				break;

			BWAPI::Position np(posTarget.x() + (4*range*vx),
				posTarget.y() + (4*range*vy));
			compDistance.unit = np;
			int num = numForEach < remaining.size() ? numForEach : remaining.size();
			std::vector<BWAPI::Unit *>::iterator ite = (min_element(remaining.begin(), remaining.begin()+num, compDistance));
			BWAPI::Unit * zergling = *ite;

			if(zergling->getTarget() != target)
				zergling->attackUnit(target);

			remaining.erase(ite);
		}
	}*/

