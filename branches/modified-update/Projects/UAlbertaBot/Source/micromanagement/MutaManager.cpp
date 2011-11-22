#include <Common.h>
#include <iostream>
#include <cmath>
#include <BWTA.h>
#include "MutaManager.h"


MutaManager::MutaManager() :
	speed(BWAPI::UnitTypes::Zerg_Mutalisk.topSpeed()),
	cooldown(BWAPI::UnitTypes::Zerg_Mutalisk.groundWeapon().damageCooldown()),
	range(BWAPI::UnitTypes::Zerg_Mutalisk.groundWeapon().maxRange()),
	totalDistance(0), numShots(0), numFrames(0)
	
{

}

void MutaManager::executeMicro(const UnitVector & targets) 
{
	assert(!targets.empty());

	// Get our mutalisks
	const UnitVector & mutas = getUnits();

	// choose the target the mutas will attack
	UnitVector newTargets(targets.begin(), targets.end());

	// sort the target list according to our priority system
	std::sort(newTargets.begin(), newTargets.end(), CompareMutaTargetSSD(mutas));

	for (size_t i(0); i<newTargets.size(); ++i) {
		
		BWAPI::Broodwar->drawTextMap(newTargets[i]->getPosition().x(), newTargets[i]->getPosition().y()-10, "\x08%d", i);
	}

	BWAPI::Unit * target = newTargets[0];

	// if there are no targets, return
	if (!target) { return; }

	// do the dance on the target
	executeMutaDance(mutas, target);
}


BWAPI::Unit * MutaManager::chooseTarget(const UnitVector & mutas, const UnitVector & targets) {

	BWAPI::Unit * target(0);
	int targetDistance = 1000000000;
	int targetPriority;

	//loop through the targets and target the closest, highest priority unit
	foreach(BWAPI::Unit * unit, targets)
	{
		// Additions by Sterling: Simple targeting priority system
		const BWAPI::UnitType type(unit->getType());
		int priority(0);

		if (type == BWAPI::UnitTypes::Terran_Medic || type.airWeapon() != BWAPI::WeaponTypes::None) {
			priority = 10;
		} else if (type ==  BWAPI::UnitTypes::Terran_Bunker) {
			priority = 9;
		} else if (type == BWAPI::UnitTypes::Protoss_Reaver) {
			priority = 8;
		} else if (type.isWorker()) {
			priority = 7;
		} else if (type ==  BWAPI::UnitTypes::Protoss_High_Templar) {
			priority = 6;
		} else if (type.groundWeapon() != BWAPI::WeaponTypes::None) {
			priority = 2;
		}  else if (type.supplyProvided() > 0) {
			priority = 1;
		} 

		if(unit->getTransport()){
			// we can't attack units in bunkers
			continue;
		}

		if(!unit->isDetected())//can't attack cloaked units
			continue;

		const int distance(sumSquaredDistance(mutas, unit));
		if(!target || priority > targetPriority || (priority == targetPriority && distance < targetDistance))
		{
			target			= unit;
			targetPriority	= priority;
			targetDistance	= distance;
		}
	}

	return target;
}

int MutaManager::sumSquaredDistance(const UnitVector & mutas, BWAPI::Unit * target) {

	int sum = 0;
	int tx(target->getPosition().x()), ty(target->getPosition().y());

	for (size_t i(0); i<mutas.size(); ++i) {
		
		int xdiff(mutas[i]->getPosition().x() - tx);
		int ydiff(mutas[i]->getPosition().y() - ty);

		sum += xdiff*xdiff + ydiff*ydiff;
	}

	return sum;
}

void MutaManager::harassMove(BWAPI::Unit * muta, BWAPI::Position location) {

	muta->attackMove(location);

	// the straight vector to the target
/*	double2 toTarget(location.x() - muta->getPosition().x(), location.y() - muta->getPosition().y());
	toTarget.normalise();

	// the threats we are worried about running into
	UnitInfoVector threats[4] = {
		UnitInfoState::getInstance()->getKnownUnitInfo(BWAPI::UnitTypes::Protoss_Photon_Cannon, BWAPI::Broodwar->enemy()),
		UnitInfoState::getInstance()->getKnownUnitInfo(BWAPI::UnitTypes::Terran_Bunker, BWAPI::Broodwar->enemy()),
		UnitInfoState::getInstance()->getKnownUnitInfo(BWAPI::UnitTypes::Terran_Missile_Turret, BWAPI::Broodwar->enemy()),
		UnitInfoState::getInstance()->getKnownUnitInfo(BWAPI::UnitTypes::Zerg_Spore_Colony, BWAPI::Broodwar->enemy()) 
	};

	BWAPI::Position closest(0,0);
	double minDistance = 100000;

	// get the closest threat
	for (int i=0; i<4; ++i) for (size_t j(0); j<threats[i].size(); ++j) {

		if (muta->getDistance(threats[i][j].lastPosition) < minDistance) {
			closest = threats[i][j].lastPosition;
			minDistance = muta->getDistance(threats[i][j].lastPosition);
		}
	}

	double2 threat(0,0);

	// if there is a threat
	if (closest != BWAPI::Position(0,0) && muta->getDistance(closest) < 350) {

		threat = double2(muta->getPosition().x() - closest.x(), muta->getPosition().y() - closest.y());
	}

	double2 direction = toTarget + threat;
	direction.normalise();
	direction = direction * 32;

	BWAPI::Position moveTo(muta->getPosition() + direction);
	BWAPI::Broodwar->drawLineMap(muta->getPosition().x(), muta->getPosition().y(), moveTo.x(), moveTo.y(), BWAPI::Colors::Yellow);

	muta->rightClick(moveTo);*/
}

void MutaManager::executeMutaDance(const UnitVector & mutas, BWAPI::Unit * target) 
{
	// Used to determine when to issue attack commands
	const int latency(BWAPI::Broodwar->getLatency());

	// clear and re-fill the air threats vector
	threats.clear();
	fillAirThreats(threats, target);

	// Determine behavior for each mutalisk individually
	foreach(BWAPI::Unit * muta, mutas) 
	{
		// If we were to issue the attack order this frame, how many frames would it take to enter firing range?
		const double distanceToTarget(muta->getDistance(target));
		const double distanceToFiringRange(std::max(distanceToTarget - range,0.0));
		const double timeToEnterFiringRange(distanceToFiringRange / speed);
		const int framesToAttack(static_cast<int>(timeToEnterFiringRange) + 2*latency);

		// How many frames are left before we can attack?
		const int currentCooldown = muta->isStartingAttack() ? cooldown : muta->getGroundWeaponCooldown();

		// If we can attack by the time we reach our firing range
		if(currentCooldown <= framesToAttack)
		{
			// Move towards and attack the target
			muta->attackUnit(target);
		}
		else // Otherwise we cannot attack and should temporarily back off
		{
			// Determine direction to flee
			const double2 fleeVector(getFleeVector(threats, muta));

			// Determine point to flee to
			BWAPI::Position moveToPosition(muta->getPosition() + fleeVector * speed * cooldown);
			if(moveToPosition.isValid()) 
			{
				muta->rightClick(moveToPosition);
			}
		}

		if (drawDebugVectors) {
			BWAPI::Broodwar->drawLineMap(muta->getPosition().x(), muta->getPosition().y(), target->getPosition().x(), target->getPosition().y(), BWAPI::Colors::Red);
		}
	}
}

void MutaManager::fillAirThreats(std::vector<AirThreat> & threats, BWAPI::Unit * target)
{
	// radius of caring
	const int radiusWeCareAbout(500);
	const int radiusSq(radiusWeCareAbout * radiusWeCareAbout);

	// for each of the candidate units
	const std::set<BWAPI::Unit*> & candidates(BWAPI::Broodwar->enemy()->getUnits());
	foreach(BWAPI::Unit * e, candidates)
	{
		// if they're not within the radius of caring, who cares?
		const BWAPI::Position delta(e->getPosition() - target->getPosition());
		if(delta.x() * delta.x() + delta.y() * delta.y() > radiusSq)
		{
			continue;
		}

		// default threat
		AirThreat threat;
		threat.unit		= e;
		threat.weight	= 1;

		// get the air weapon of the unit
		BWAPI::WeaponType airWeapon(e->getType().airWeapon());

		// if it's a bunker, weight it as if it were 4 marines
		if(e->getType() == BWAPI::UnitTypes::Terran_Bunker)
		{
			airWeapon		= BWAPI::WeaponTypes::Gauss_Rifle;
			threat.weight	= 4;
		}

		// weight the threat based on the highest DPS
		if(airWeapon != BWAPI::WeaponTypes::None)
		{
			threat.weight *= (static_cast<double>(airWeapon.damageAmount()) / airWeapon.damageCooldown());
			threats.push_back(threat);
		}
	}
}

double2 MutaManager::getFleeVector(const std::vector<AirThreat> & threats, BWAPI::Unit * mutalisk)
{
	double2 fleeVector(0,0);

	foreach(const AirThreat & threat, threats)
	{
		// Get direction from enemy to mutalisk
		const double2 direction(mutalisk->getPosition() - threat.unit->getPosition());

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

	BWAPI::Position p1(mutalisk->getPosition());
	BWAPI::Position p2(p1 + fleeVector * 100);

	if (drawDebugVectors) {
		BWAPI::Broodwar->drawLineMap(p1.x(), p1.y(), p2.x(), p2.y(), BWAPI::Colors::White);
	}

	return fleeVector;
}