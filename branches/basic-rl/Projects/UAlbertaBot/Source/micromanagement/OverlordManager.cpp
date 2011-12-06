#include "Common.h"
#include "OverlordManager.h"

#define FOROVERLORDS(A) foreach(Unit * A, Broodwar->self()->getUnits()) if (A->getType() == UnitTypes::Zerg_Overlord)
#define FORENEMYUNITS(A) foreach(Unit * A, Broodwar->getAllUnits()) if (A->getPlayer() != Broodwar->self())

using namespace BWAPI;

OverlordManager * OverlordManager::instance = NULL;

OverlordManager::OverlordManager() {

	range = 15;
	xi = range/2;
	yi = range/2;

	overlordScout = NULL;
}

// get an instance of this
OverlordManager * OverlordManager::getInstance() {

	// if the instance doesn't exist, create it
	if (!OverlordManager::instance) {
		OverlordManager::instance = new OverlordManager();
	}

	return OverlordManager::instance;
}

void OverlordManager::update() {

	++frame;

	for (int x=xi; x<Broodwar->mapWidth(); x+=range) for (int y=yi; y<Broodwar->mapHeight(); y+=range) {
		Broodwar->drawDot(BWAPI::CoordinateType::Map, x*32, y*32, BWAPI::Colors::Green); 
	}

	// if our overlord is scouting
	if (overlordScout) {

		// if we know the enemy's main base location
		if (UnitInfoState::getInstance()->getMainBaseLocation(BWAPI::Broodwar->enemy())) {

			// get the overlord back home
			overlordScout->attackMove(BWAPI::Position(BWAPI::Broodwar->self()->getStartLocation()));
			overlordScout = NULL;
		}
	}

	if (frame % 30 == 0) {

		std::map<BWAPI::Unit *, BWAPI::Position>::iterator it1;
		std::map<BWAPI::Unit *, bool>::iterator it2;

		// for each overlord
		FOROVERLORDS (o) {

			bool shouldFlee = false;
			Position fleeVector;

			// find its assigned position
			it1 = overlord_positions.find(o);

			// find whether it's tethered
			it2 = overlord_tethered.find(o);

			UnitVector nearEnemies; 
			threats.clear();

			// if it has a position and a tether flag
			if (it1 != overlord_positions.end() && it2 != overlord_tethered.end()) {

				// get all of the enemy units within range of the overlord
				MapGrid::getInstance()->GetUnits(nearEnemies, o->getPosition(), UnitTypes::Zerg_Overlord.sightRange() + 200, false, true);
				for (size_t i(0); i<nearEnemies.size(); ++i) {
					if (nearEnemies[i]->getType().airWeapon() != BWAPI::WeaponTypes::None) {
						shouldFlee = true;
					}
				}
					

				// if it's tethered
				if (it2->second) {

					// if there's a unit which can attack it, move away from it
					if (shouldFlee) {
						
						Position op = o->getPosition();
						Position away = op + getFleeVector(threats, o);

						o->attackMove(away);

						Broodwar->drawTextMap(o->getPosition().x(), o->getPosition().y(), "Fleeing");

					// if there's no attacking unit nearby
					} else {

						// move to tethered position
						o->attackMove(it1->second);

						Broodwar->drawTextMap(o->getPosition().x(), o->getPosition().y(), "Tethered");
					}
				}
			}
		}
	}
}


// what to do when an overlord is created
void OverlordManager::onOverlordCreate(Unit * unit) {

	// assign the overlord to the nearest unoccupied position
	overlord_positions[unit] = closestUnoccupiedPosition(unit);

	// tether it
	tetherOverlord(unit);
}

// what to do when an overlord is destroyed
void OverlordManager::onOverlordDestroy(Unit * unit) {

	// erase its assignment from the maps
	overlord_positions.erase(unit);
	overlord_tethered.erase(unit);
}

void OverlordManager::setAttackPosition(BWAPI::Position attackPosition) {

	UnitVector overlords;
	FOROVERLORDS(o) { overlords.push_back(o); }

	std::sort(overlords.begin(), overlords.end(), OverlordCompareDistance(attackPosition));

	int numOverlordsOnAttack = 2;

	// find the closest number of overlords to the attack Position
	for (size_t i(0); (int)i<numOverlordsOnAttack && i<overlords.size(); ++i) {

		BWAPI::Position pos(attackPosition.x() - i*200, attackPosition.y() - i*200);

		// untether the overlord
		untetherOverlord(overlords[i]);

		// send it to the attack position
		overlords[i]->move(pos);

		// draw a circle on it so we know where it is
		BWAPI::Broodwar->drawCircleMap(overlords[i]->getPosition().x(), overlords[i]->getPosition().y(), 10, BWAPI::Colors::Red, true);
	}

}

void OverlordManager::tetherAllOverlords() {

	FOROVERLORDS(o) {

		// if the overlord current has a tether option
		if (overlord_tethered.find(o) != overlord_tethered.end()) {

			// tether it
			overlord_tethered.find(o)->second = true;
		}
	}

}

// tether an overlord into its assigned position
void OverlordManager::tetherOverlord(Unit * unit) {
	
	overlord_tethered[unit] = true;
}

// untether an overlord, free to do other things
void OverlordManager::untetherOverlord(Unit * unit) {

	overlord_tethered[unit] = false;
}

BWAPI::Unit * OverlordManager::getClosestOverlord(BWAPI::Position p) {

	BWAPI::Unit * overlord = NULL;
	double distance = 1000000;

	FOROVERLORDS(o) {

		if (!overlord || o->getDistance(p) < distance) {

			overlord = o;
			distance = o->getDistance(p);
		}
	}

	return overlord;
}

// returns the closest unassigned position to the unit
Position OverlordManager::closestUnoccupiedPosition(Unit * unit) {

	// placeholders
	double minDist = ( Broodwar->mapWidth() + Broodwar->mapWidth() ) * 32;
	Position closest;

	// for each of our positions
	for (int x=xi; x<Broodwar->mapWidth(); x+=range) for (int y=yi; y<Broodwar->mapHeight(); y+=range) {

		Position temp(x*32,y*32);

		// which is the closest unassigned position?
		if (unit->getDistance(temp) < minDist && !overlordAssigned(temp)) {

			minDist = unit->getDistance(temp);
			closest = temp;
		}
	}

	// return it
	return closest;
}

// is there an overlord within distance 'dist' of position p?
bool OverlordManager::overlordNear(Position p, double dist) {

	// for each overlord
	FOROVERLORDS(unit) {
	
		// if it's closer, return true
		if (unit->getDistance(p) < dist) return true;
	}

	// otherwise return false
	return false;
}

// is there an overlord within distance 'dist' of position p?
bool OverlordManager::overlordAssigned(BWAPI::Position p) {

	// for each overlord assignment
	for (std::map<BWAPI::Unit *,BWAPI::Position>::iterator it = overlord_positions.begin(); it != overlord_positions.end(); it++) {

		// return true if we've already assigned it
		if (p == it->second) return true;
	}

	// otherwise return false
	return false;
}

void OverlordManager::onStart() {

	// get starting locations
	std::set< TilePosition > startLocations = Broodwar->getStartLocations();
	TilePosition scout(0,0);

	// get the first starting location that isn't ours
	foreach (TilePosition tp, startLocations) {

		// if it's not our start position
		if (tp != Broodwar->self()->getStartLocation()) {

			// set the spot to scout as this position and break
			scout = tp;
			break;
		}
	}

	// do clever stuff with starting units
	FOROVERLORDS(o) {
		
		// send first overlord to scout
		//overlordScout = o;
		//o->attackMove(scout);
		break;
	}
}
void OverlordManager::fillAirThreats(std::vector<AirThreat> & threats, UnitVector & candidates)
{
	// for each of the candidate units
	foreach(BWAPI::Unit * e, candidates)
	{
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

double2 OverlordManager::getFleeVector(const std::vector<AirThreat> & threats, BWAPI::Unit * overlord)
{
	double2 fleeVector(0,0);

	foreach(const AirThreat & threat, threats)
	{
		// Get direction from enemy to mutalisk
		const double2 direction(overlord->getPosition() - threat.unit->getPosition());

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

	BWAPI::Position p1(overlord->getPosition());
	BWAPI::Position p2(p1 + fleeVector * 100);

	return fleeVector;
}