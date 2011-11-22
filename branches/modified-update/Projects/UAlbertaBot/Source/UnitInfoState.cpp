#include "Common.h"
#include "UnitInfoState.h"

#define SELF_INDEX 0
#define ENEMY_INDEX 1

// gotta keep c++ static happy
UnitInfoState * UnitInfoState::instance = NULL;

// constructor
UnitInfoState::UnitInfoState() : goForIt(false), calculatedDistanceToEnemy(false), enemyHasCloakedUnit(false) {

	int maxTypeID = 0;

	BOOST_FOREACH(BWAPI::UnitType type, BWAPI::UnitTypes::allUnitTypes()) 
	{
		if (type.getID() > maxTypeID) 
		{
			maxTypeID = type.getID();
		}
	}

	knownUnits[SELF_INDEX] = std::vector< UnitInfoVector >(maxTypeID+1, std::vector<UnitInfo>());
	knownUnits[ENEMY_INDEX] = std::vector< UnitInfoVector >(maxTypeID+1, std::vector<UnitInfo>());
	numDeadUnits[SELF_INDEX] = std::vector< int >(maxTypeID+1, 0);
	numDeadUnits[ENEMY_INDEX] = std::vector< int >(maxTypeID+1, 0);
	numTotalDeadUnits[SELF_INDEX] = 0;
	numTotalDeadUnits[ENEMY_INDEX] = 0;
	mineralsLost[SELF_INDEX] = 0;
	mineralsLost[ENEMY_INDEX] = 0;
	gasLost[SELF_INDEX] = 0;
	gasLost[ENEMY_INDEX] = 0;

	initializeRegionInformation();
	initializeBaseInfoVector();
	populateUnitInfoVectors();
}

// get an instance of this
UnitInfoState * UnitInfoState::getInstance() {

	// if the instance doesn't exist, create it
	if (!UnitInfoState::instance) {
		UnitInfoState::instance = new UnitInfoState();
	}

	return UnitInfoState::instance;
}

void UnitInfoState::update() 
{
	updateUnitInfo();
	updateBaseInfo();
	updateBaseLocationInfo();

	drawUnitInformation(425,30);
}

void UnitInfoState::initializeRegionInformation() {

	// set initial pointers to null
	mainBaseLocations[SELF_INDEX] = BWTA::getStartLocation(BWAPI::Broodwar->self());
	mainBaseLocations[ENEMY_INDEX] = BWTA::getStartLocation(BWAPI::Broodwar->enemy());

	if (!mainBaseLocations[SELF_INDEX]) {
		BWAPI::Broodwar->printf("SOMETHING IS WRONG, COULDN'T GET OWN BASE LOCATION!");
		return;
	}

	// get a bunch of information
	UnitInfoVector & allSelfUnits = getAllUnits(BWAPI::Broodwar->self());
	const std::set<BWTA::Region *> & allRegions = BWTA::getRegions();
	const std::set<BWTA::BaseLocation *> & allStartLocations = BWTA::getStartLocations();

	// push that region into our occupied vector
	updateOccupiedRegions(BWTA::getRegion(mainBaseLocations[SELF_INDEX]->getTilePosition()), BWAPI::Broodwar->self());
}


void UnitInfoState::updateBaseLocationInfo() {

	occupiedRegions[SELF_INDEX].clear();
	occupiedRegions[ENEMY_INDEX].clear();
		
	// if we haven't found the enemy main base location yet
	if (!mainBaseLocations[ENEMY_INDEX]) { 
		
		// how many start locations have we explored
		int exploredStartLocations = 0;
		bool baseFound = false;

		// an undexplored base location holder
		BWTA::BaseLocation * unexplored = NULL;

		BOOST_FOREACH (BWTA::BaseLocation * startLocation, BWTA::getStartLocations()) 
		{
			if (isEnemyBuildingInRegion(BWTA::getRegion(startLocation->getTilePosition()))) 
			{
				baseFound = true;
				BWAPI::Broodwar->printf("Enemy base found by seeing it");
				mainBaseLocations[ENEMY_INDEX] = startLocation;
				updateOccupiedRegions(BWTA::getRegion(startLocation->getTilePosition()), BWAPI::Broodwar->enemy());
			}

			// if it's explored, increment
			if (BWAPI::Broodwar->isExplored(startLocation->getTilePosition())) 
			{
				exploredStartLocations++;

			// otherwise set the unexplored base
			} 
			else 
			{
				unexplored = startLocation;
			}
		}

		// if we've explored every start location except one, it's the enemy
		if (!baseFound && exploredStartLocations == (BWTA::getStartLocations().size() - 1)) 
		{
			BWAPI::Broodwar->printf("Enemy base found by process of elimination");
			mainBaseLocations[ENEMY_INDEX] = unexplored;
			updateOccupiedRegions(BWTA::getRegion(unexplored->getTilePosition()), BWAPI::Broodwar->enemy());
		}

	// otherwise we do know it, so push it back
	} 
	else 
	{
		updateOccupiedRegions(BWTA::getRegion(mainBaseLocations[ENEMY_INDEX]->getTilePosition()), BWAPI::Broodwar->enemy());
	}

	// for each enemy unit we know about
	UnitInfoVector & enemyUnits = getAllUnits(BWAPI::Broodwar->enemy());
	for (size_t i(0); i < enemyUnits.size(); i++) 
	{
		BWAPI::UnitType type = enemyUnits[i].type;

		// if the unit is a building
		if (type.isBuilding()) 
		{
			// update the enemy occupied regions
			updateOccupiedRegions(BWTA::getRegion(BWAPI::TilePosition(enemyUnits[i].lastPosition)), BWAPI::Broodwar->enemy());
		}
	}

	// for each of our units
	UnitInfoVector & ourUnits = getAllUnits(BWAPI::Broodwar->self());
	for (size_t i(0); i < ourUnits.size(); i++) 
	{
		BWAPI::UnitType type = ourUnits[i].type;

		// if the unit is a building
		if (type.isBuilding()) 
		{
			// update the enemy occupied regions
			updateOccupiedRegions(BWTA::getRegion(BWAPI::TilePosition(ourUnits[i].lastPosition)), BWAPI::Broodwar->self());
		}
	}
}

void UnitInfoState::updateOccupiedRegions(BWTA::Region * region, BWAPI::Player * player) 
{
	// otherwise, add it to the vector
	occupiedRegions[getIndex(player)].insert(region);
}

bool UnitInfoState::isEnemyBuildingInRegion(BWTA::Region * region) 
{
	for (size_t i(0); i<allUnits[ENEMY_INDEX].size(); ++i) 
	{
		if (allUnits[ENEMY_INDEX][i].type.isBuilding()) 
		{
			if (BWTA::getRegion(BWAPI::TilePosition(allUnits[ENEMY_INDEX][i].lastPosition)) == region) 
			{
				return true;
			}
		}
	}

	return false;
}

int UnitInfoState::numEnemyUnitsInRegion(BWTA::Region * region) 
{
	int unitsInRegion(0);
	BOOST_FOREACH(BWAPI::Unit * unit, BWAPI::Broodwar->enemy()->getUnits()) 
	{
		if (unit->isVisible() && BWTA::getRegion(BWAPI::TilePosition(unit->getPosition())) == region) 
		{
			unitsInRegion++;
		}
	}

	return unitsInRegion;
}

int UnitInfoState::numEnemyFlyingUnitsInRegion(BWTA::Region * region) 
{
	int unitsInRegion(0);
	BOOST_FOREACH(BWAPI::Unit * unit, BWAPI::Broodwar->enemy()->getUnits()) 
	{
		if (unit->isVisible() && BWTA::getRegion(BWAPI::TilePosition(unit->getPosition())) == region && unit->getType().isFlyer()) 
		{
			unitsInRegion++;
		}
	}

	return unitsInRegion;
}

std::set<BWTA::Region *> & UnitInfoState::getOccupiedRegions(BWAPI::Player * player)
{
	return occupiedRegions[getIndex(player)];
}

BWTA::BaseLocation * UnitInfoState::getMainBaseLocation(BWAPI::Player * player) 
{
	return mainBaseLocations[getIndex(player)];
}

void UnitInfoState::initializeBaseInfoVector() 
{

	// all of the base locations
	std::set<BWTA::BaseLocation *> baseLocations = BWTA::getBaseLocations();
	
	// our start location
	BWAPI::TilePosition startTilePosition = BWAPI::Broodwar->self()->getStartLocation();

	// for each of the base locations
	BOOST_FOREACH (BWTA::BaseLocation * baseLocation, baseLocations) 
	{
		// if we can get to it from the start location
		if (BWTA::isConnected(startTilePosition, baseLocation->getTilePosition())) 
		{
			// put it into the base info vector
			allBases.push_back(BaseInfo(baseLocation));
		}
	}
}

void UnitInfoState::drawUnitInformation(int x, int y) {
	
	std::string prefix = "\x04";

	if (DRAW_UALBERTABOT_DEBUG) BWAPI::Broodwar->drawTextScreen(x, y-10, "\x03Lost:\x04 S \x1f%d \x07%d\x04 E \x1f%d \x07%d ", 
		mineralsLost[SELF_INDEX], gasLost[SELF_INDEX], mineralsLost[ENEMY_INDEX], gasLost[ENEMY_INDEX]);
	if (DRAW_UALBERTABOT_DEBUG) BWAPI::Broodwar->drawTextScreen(x, y, "\x04 Enemy Unit Information: %s", BWAPI::Broodwar->enemy()->getRace().getName().c_str());
	if (DRAW_UALBERTABOT_DEBUG) BWAPI::Broodwar->drawTextScreen(x, y+20, "\x04UNIT NAME");
	if (DRAW_UALBERTABOT_DEBUG) BWAPI::Broodwar->drawTextScreen(x+140, y+20, "\x04#");
	if (DRAW_UALBERTABOT_DEBUG) BWAPI::Broodwar->drawTextScreen(x+160, y+20, "\x04X");

	int yspace = 0;

	// for each unit in the queue
	BOOST_FOREACH (BWAPI::UnitType t, BWAPI::UnitTypes::allUnitTypes()) 
	{
		int numUnits = getNumUnits(t, BWAPI::Broodwar->enemy());
		int numDeadUnits = getNumDeadUnits(t, BWAPI::Broodwar->enemy());

		// if there exist units in the vector
		if (numUnits > 0) 
		{
			if (t.isDetector())			{ prefix = "\x10"; }		
			else if (t.canAttack())		{ prefix = "\x08"; }		
			else if (t.isBuilding())	{ prefix = "\x03"; }
			else						{ prefix = "\x04"; }

			if (DRAW_UALBERTABOT_DEBUG) BWAPI::Broodwar->drawTextScreen(x, y+40+((yspace)*10), "%s%s", prefix.c_str(), t.getName().c_str());
			if (DRAW_UALBERTABOT_DEBUG) BWAPI::Broodwar->drawTextScreen(x+140, y+40+((yspace)*10), "%s%d", prefix.c_str(), numUnits);
			if (DRAW_UALBERTABOT_DEBUG) BWAPI::Broodwar->drawTextScreen(x+160, y+40+((yspace++)*10), "%s%d", prefix.c_str(), numDeadUnits);
		}
	}

	BOOST_FOREACH (UnitInfo & u, allUnits[getIndex(BWAPI::Broodwar->enemy())])
	{
		if (DRAW_UALBERTABOT_DEBUG) BWAPI::Broodwar->drawCircleMap(u.lastPosition.x(), u.lastPosition.y(), 3, BWAPI::Colors::Red, true);
	}
}

// populate the info vectors with what we can see at instanciation
void UnitInfoState::populateUnitInfoVectors() 
{
	BOOST_FOREACH(BWAPI::Unit * unit, BWAPI::Broodwar->getAllUnits()) 
	{
		updateUnit(unit);
	}
}



void UnitInfoState::updateBaseInfo() 
{
	for (size_t i(0); i<allBases.size(); ++i) 
	{
		if (DRAW_UALBERTABOT_DEBUG) BWAPI::Broodwar->drawCircleMap(allBases[i].baseLocation->getPosition().x(), allBases[i].baseLocation->getPosition().y(), 5, BWAPI::Colors::Yellow, true);

		if (allBases[i].isVisible()) 
		{
			allBases[i].lastFrameSeen = BWAPI::Broodwar->getFrameCount();
		}
	}
}

void UnitInfoState::updateUnitInfo() 
{
	if (!enemyHasCloakedUnit)
	{
		BOOST_FOREACH (BWAPI::Unit * unit, BWAPI::Broodwar->enemy()->getUnits())
		{
			BWAPI::UnitType type(unit->getType());

			if (type == BWAPI::UnitTypes::Zerg_Lurker ||
				type == BWAPI::UnitTypes::Protoss_Dark_Templar ||
				type == BWAPI::UnitTypes::Terran_Wraith)
			{
				enemyHasCloakedUnit = true;
			}
		}
	}

	BOOST_FOREACH (BWAPI::Unit * unit, BWAPI::Broodwar->enemy()->getUnits())
	{
		updateUnit(unit);
	}

	// Iterate through all enemy units we have seen
	for(UnitInfoVector::iterator it(allUnits[ENEMY_INDEX].begin()); it!=allUnits[ENEMY_INDEX].end(); )
	{
		UnitInfo & ui(*it);

		// If the unit is a building
		if(ui.type.isBuilding())
		{
			// Cull away any refineries/assimilators/extractors that were destroyed and reverted to vespene geysers
			if(ui.unit->getType() == BWAPI::UnitTypes::Resource_Vespene_Geyser)
			{
				//BWAPI::Broodwar->printf("Removing bad building: %s", ui.type.getName().c_str());
				it = allUnits[ENEMY_INDEX].erase(it);
				continue;
			}

			// If we cannot currently see the building
			if(!ui.unit->isVisible()) 
			{
				// If we can see the tile where the building was, then the building must have burned down or been moved
				if(BWAPI::Broodwar->isVisible(ui.lastPosition.x()/32, ui.lastPosition.y()/32)) 
				{
					//BWAPI::Broodwar->printf("Removing bad building: %s", ui.type.getName().c_str());

					// Remove the record of this building and move on
					it = allUnits[ENEMY_INDEX].erase(it);
					continue;
				} 
			}

			if (DRAW_UALBERTABOT_DEBUG) BWAPI::Broodwar->drawCircleMap(ui.lastPosition.x(), ui.lastPosition.y(), 5, BWAPI::Colors::Orange, true);
			//if (DRAW_UALBERTABOT_DEBUG) BWAPI::Broodwar->drawTextMap(ui.lastPosition.x(), ui.lastPosition.y(), "%s (%d)", ui.type.getName().c_str(), ui.unitID);
		}

		

		++it;
	}
}

void UnitInfoState::onStart() {}

// erase a unit from the correct set in the map
void UnitInfoState::eraseUnit(BWAPI::Unit * unit) 
{
	// check to see if the unit is valid
	if (!isValidUnit(unit)) { return; }

	// erase it
	eraseUnitFromVector(unit, knownUnits[getIndex(unit->getPlayer())][unit->getType().getID()]);
	eraseUnitFromVector(unit, allUnits[getIndex(unit->getPlayer())]);
	eraseUnitFromVector(unit, enemyDetectors);
}

// update the unit within the map sets
void UnitInfoState::updateUnit(BWAPI::Unit * unit) 
{
	// check to see if the unit is valid
	if (!isValidUnit(unit)) { return; }

	// check to see if the unit already exists in the set
	updateUnitInVector(unit, knownUnits[getIndex(unit->getPlayer())][unit->getType().getID()]);
	updateUnitInVector(unit, allUnits[getIndex(unit->getPlayer())]);

	if (unit->getType().isDetector() && unit->getPlayer() == BWAPI::Broodwar->enemy())
	{
		updateUnitInVector(unit, enemyDetectors);
	}
}

void UnitInfoState::updateUnitInVector(BWAPI::Unit * unit, UnitInfoVector & units) 
{
	// check to see if the unit already exists in the set
	bool alreadyExists = false;
	for (size_t i(0); i<units.size(); ++i) 
	{
		// if it does, update its last seen position
		if (units[i].unitID == unit->getID()) 
		{
			// if the unit is now 

			alreadyExists = true;
			units[i].lastPosition = unit->getPosition();
			units[i].lastHealth = unit->getHitPoints() + unit->getShields();
			units[i].unitID = unit->getID();
			units[i].type = unit->getType();
			break;
		}
	}

	// otherwise, it doesn't exist, so add it
	if (!alreadyExists) 
	{
		units.push_back(UnitInfo(unit->getID(), unit, unit->getPosition(), unit->getType()));
	}
}

// if a UnitInfoVector contains a UnitInfo which contains the unit, it is erased
void UnitInfoState::eraseUnitFromVector(BWAPI::Unit * unit, UnitInfoVector & units) 
{
	// for each unitinfo
	for (size_t i(0); i<units.size(); ++i) 
	{
		// if we have a match
		if (units[i].unitID == unit->getID()) 
		{
			// swap with back and pop it
			units[i] = units.back();
			units.pop_back();
			
			break;
		}
	}
}

// is the unit valid?
bool UnitInfoState::isValidUnit(BWAPI::Unit * unit) 
{
	// we only care about our units and enemy units
	if (unit->getPlayer() != BWAPI::Broodwar->self() && unit->getPlayer() != BWAPI::Broodwar->enemy()) 
	{
		return false;
	}

	// if it's a weird unit, don't bother
	if (unit->getType() == BWAPI::UnitTypes::None || unit->getType() == BWAPI::UnitTypes::Unknown ||
		unit->getType() == BWAPI::UnitTypes::Zerg_Larva || unit->getType() == BWAPI::UnitTypes::Zerg_Egg) 
	{
		return false;
	}

	// if the position isn't valid throw it out
	if (!unit->getPosition().isValid()) 
	{
		return false;	
	}

	// s'all good baby baby
	return true;
}

void UnitInfoState::onUnitDestroy(BWAPI::Unit * unit) 
{ 
	// erase the unit
	eraseUnit(unit); 

	// increase the number of dead units of that type
	numDeadUnits[getIndex(unit->getPlayer())][unit->getType().getID()]++;
	numTotalDeadUnits[getIndex(unit->getPlayer())]++;

	// increase the resource lost count
	mineralsLost[getIndex(unit->getPlayer())]		+= unit->getType().mineralPrice();
	gasLost[getIndex(unit->getPlayer())]			+= unit->getType().gasPrice();
}

bool UnitInfoState::positionInRangeOfEnemyDetector(BWAPI::Position p)
{
	for (size_t i(0); i < enemyDetectors.size(); ++i)
	{
		if (DRAW_UALBERTABOT_DEBUG) BWAPI::Broodwar->drawCircleMap(enemyDetectors[i].lastPosition.x(), enemyDetectors[i].lastPosition.y(), enemyDetectors[i].type.sightRange(), false);

		if (p.getDistance(enemyDetectors[i].lastPosition) < enemyDetectors[i].type.sightRange())
		{
			return true;
		}
	}

	return false;
}

UnitInfoVector & UnitInfoState::getEnemyDetectors() 
{
	return enemyDetectors;
}

int UnitInfoState::getNumDeadUnits(BWAPI::UnitType type, BWAPI::Player * player) 
{
	return numDeadUnits[getIndex(player)][type.getID()];
}

int UnitInfoState::getNumTotalDeadUnits(BWAPI::Player * player)
{
	return numTotalDeadUnits[getIndex(player)];
}

int UnitInfoState::getNumUnits(BWAPI::UnitType type, BWAPI::Player * player) 
{
	return knownUnits[getIndex(player)][type.getID()].size();
}

UnitInfoVector & UnitInfoState::getKnownUnitInfo(BWAPI::UnitType type, BWAPI::Player * player) 
{
	return knownUnits[getIndex(player)][type.getID()];
}

UnitInfoVector & UnitInfoState::getAllUnits(BWAPI::Player * player) 
{
	return allUnits[getIndex(player)];
}

int UnitInfoState::getIndex(BWAPI::Player * player) 
{
	return (player == BWAPI::Broodwar->self()) ? SELF_INDEX : ENEMY_INDEX;
}

int UnitInfoState::knownForceSize(BWAPI::Player * player) 
{
	double forceSize = 0;

	for (size_t i(0); i<allUnits[getIndex(player)].size(); ++i) 
	{
		BWAPI::UnitType type(allUnits[getIndex(player)][i].type);

		// we don't care about workers
		if (type.isWorker())
		{
			continue;
		}

		// attacking unit
		if (type == BWAPI::UnitTypes::Zerg_Zergling)
		{
			forceSize += 0.5;
		}
		else if (type == BWAPI::UnitTypes::Terran_Marine)
		{
			forceSize += 0.75;
		}
		else if (!type.isBuilding() && type.canAttack() || type == BWAPI::UnitTypes::Terran_Medic ||
			type == BWAPI::UnitTypes::Protoss_High_Templar) 
		{
			forceSize++;
		}
		// bunker
		else if (type == BWAPI::UnitTypes::Terran_Bunker)
		{
			forceSize += 5;
		}
		else if (type == BWAPI::UnitTypes::Zerg_Sunken_Colony)
		{
			forceSize += 2.5;
		}
	}

	return (int)forceSize;
}

int UnitInfoState::visibleForceSize(BWAPI::Player * player) 
{
	double forceSize = 0;

	BOOST_FOREACH (BWAPI::Unit * unit, player->getUnits()) 
	{
		BWAPI::UnitType type(unit->getType());

		// we don't care about workers
		if (type.isWorker() || !unit->isVisible())
		{
			continue;
		}

		// attacking unit
		if (type == BWAPI::UnitTypes::Zerg_Zergling)
		{
			forceSize += 0.5;
		}
		else if (type == BWAPI::UnitTypes::Terran_Marine)
		{
			forceSize += 0.75;
		}
		else if (!type.isBuilding() && type.canAttack() || 
			type == BWAPI::UnitTypes::Terran_Medic ||
			type == BWAPI::UnitTypes::Protoss_High_Templar) 
		{
			forceSize++;
		}
		// bunker
		else if (type == BWAPI::UnitTypes::Terran_Bunker)
		{
			forceSize += 3;
		}
		else if (type == BWAPI::UnitTypes::Zerg_Sunken_Colony)
		{
			forceSize += 1.5;
		}
		else if (type == BWAPI::UnitTypes::Protoss_Photon_Cannon)
		{
			forceSize += 2;
		}
	}

	return (int)forceSize;
}

BWAPI::Unit * UnitInfoState::getClosestUnitToTarget(BWAPI::UnitType type, BWAPI::Position target)
{
	BWAPI::Unit * closestUnit = NULL;
	double closestDist = 100000;

	BOOST_FOREACH (BWAPI::Unit * unit, BWAPI::Broodwar->self()->getUnits())
	{
		if (unit->getType() == type)
		{
			double dist = unit->getDistance(target);
			if (!closestUnit || dist < closestDist)
			{
				closestUnit = unit;
				closestDist = dist;
			}
		}
	}

	return closestUnit;
}

bool UnitInfoState::enemyHasCloakedUnits()
{
	return enemyHasCloakedUnit;
}

int UnitInfoState::nearbyForceSize(BWAPI::Position p, BWAPI::Player * player, int radius) 
{
	double forceSize = 0;

	if (DRAW_UALBERTABOT_DEBUG) BWAPI::Broodwar->drawCircleMap(p.x(), p.y(), radius, BWAPI::Colors::Red);

	for (size_t i(0); i<allUnits[getIndex(player)].size(); ++i) 
	{
		UnitInfo ui = allUnits[getIndex(player)][i];
		BWAPI::UnitType type(allUnits[getIndex(player)][i].type);

		// we don't care about workers
		if (type.isWorker())
		{
			continue;
		}

		int range = 0;
		if (type.groundWeapon() != BWAPI::WeaponTypes::None)
		{
			range = type.groundWeapon().maxRange() + 40;
		}

		// if it's outside the radius we don't care
		if (ui.lastPosition.getDistance(p) > (radius + range))
		{
			continue;
		}

		// attacking unit
		if (type == BWAPI::UnitTypes::Zerg_Zergling)
		{
			forceSize += 0.5;
		}
		else if (!type.isBuilding() && type.canAttack() || 
			type == BWAPI::UnitTypes::Terran_Medic ||
			type == BWAPI::UnitTypes::Protoss_High_Templar) 
		{
			forceSize++;
		}
		// bunker
		else if (type == BWAPI::UnitTypes::Terran_Bunker)
		{
			forceSize += 3;
		}
		else if (type == BWAPI::UnitTypes::Zerg_Sunken_Colony)
		{
			forceSize += 2;
		}
		else if (type == BWAPI::UnitTypes::Zerg_Sunken_Colony)
		{
			forceSize += 2;
		}
	}

	return (int)forceSize;
}


bool UnitInfoState::nearbyForceHasCloaked(BWAPI::Position p, BWAPI::Player * player, int radius) 
{
	if (DRAW_UALBERTABOT_DEBUG) BWAPI::Broodwar->drawCircleMap(p.x(), p.y(), radius, BWAPI::Colors::Red);

	for (size_t i(0); i<allUnits[getIndex(player)].size(); ++i) 
	{
		UnitInfo ui = allUnits[getIndex(player)][i];
		BWAPI::UnitType type(allUnits[getIndex(player)][i].type);

		// we don't care about workers
		if (type.isWorker())
		{
			continue;
		}

		int range = 0;
		if (type.groundWeapon() != BWAPI::WeaponTypes::None)
		{
			range = type.groundWeapon().maxRange() + 40;
		}

		// if it's outside the radius we don't care
		if (ui.lastPosition.getDistance(p) > (radius + range))
		{
			continue;
		}

		if (type == BWAPI::UnitTypes::Zerg_Lurker ||
				type == BWAPI::UnitTypes::Protoss_Dark_Templar ||
				type == BWAPI::UnitTypes::Terran_Wraith)
		{
			return true;
		}
	}

	return false;
}

