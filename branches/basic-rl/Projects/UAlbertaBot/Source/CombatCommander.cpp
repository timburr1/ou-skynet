#include "Common.h"
#include "CombatCommander.h"

CombatCommander::CombatCommander() :attacking(false), foundEnemy(false), attackSent(false) 
{
	enemyExpansion = BWAPI::Position(0,0);
	currentAction = std::string("\x04 No Attacks Yet");
}

bool CombatCommander::squadUpdateFrame()
{
	return BWAPI::Broodwar->getFrameCount() % 24 == 0;
}

void CombatCommander::update(const UnitVector & allCombatUnits)
{
	// Construct squads
	if(squadUpdateFrame())
	{
		// clear all squad data
		squadData.clearSquadData();
		
		// give back combat workers to worker manager
		WorkerManager::getInstance()->finishedWithCombatWorkers();

		// our starting region and our enemy's closest region
		BWTA::Region *				startRegion = BWTA::getRegion(BWAPI::Broodwar->self()->getStartLocation());
		BWTA::Region *				enemyRegion = getClosestEnemyRegion();

		// Get all units that belong to us
		std::set<BWAPI::Unit *>		unitsToAssign(allCombatUnits.begin(), allCombatUnits.end());

		// if there are no units to assign, there's nothing to do
		if (unitsToAssign.empty())
		{
			return;
		}

		// do we have workers in combat
		bool workersInCombat = allCombatUnits[0]->getType().isWorker();

		// Assign our defense forces if they seem necessary
		assignDefenders(unitsToAssign);

		// Determine how to assign remaining units
		UnitVector					freeUnits(unitsToAssign.begin(), unitsToAssign.end());

		// the default order is to defend near a choke point
		SquadOrder					order(SquadOrder::Defend, getDefendLocation(), 1000);
		if(BWAPI::Broodwar->getFrameCount() > 24*60*5) //This line makes units attack if they haven't won or lost after five minutes
		{
			SquadOrder order(SquadOrder::Attack,getDefendLocation(),10000);
		}
		
		// if we are attacking, what area are we attacking?
		if (!workersInCombat && StrategyManager::getInstance()->doAttack(freeUnits)) 
		{	
			// Decide/Assign if we are going to attack an enemy region
			assignAttackRegion(unitsToAssign, enemyRegion, order);
			
			// If the order is still defend
			if(order.type == SquadOrder::Defend)
			{
				// Decide/Assign if we need to attack known buildings
				assignAttackKnownBuildings(unitsToAssign, order);
			}

			// If the order is still defend
			if(order.type == SquadOrder::Defend)
			{
				// Decide/Assign if we need to attack visible enemy units
				assignAttackVisibleUnits(unitsToAssign, order);
			}

			// If the order is still defend
			if(order.type == SquadOrder::Defend)
			{
				// Decide/Assign if we need to explore for unknown building locations
				assignAttackExplore(unitsToAssign, order);
			}
		} 

		// grab all the remaining free units
		freeUnits = UnitVector(unitsToAssign.begin(), unitsToAssign.end());

		if (!freeUnits.empty()) 
		{
			squadData.setSquad(Squad(freeUnits, order));
		}
	}

	// update the squads
	squadData.update();

	//if (DRAW_UALBERTABOT_DEBUG) BWAPI::Broodwar->drawBoxScreen(200, 319, 325, 341, BWAPI::Colors::Black, true);
	if (DRAW_UALBERTABOT_DEBUG) BWAPI::Broodwar->drawTextScreen(200, 320, "%s", currentAction.c_str());
}

BWTA::Region * CombatCommander::getClosestEnemyRegion()
{
	BWTA::Region * closestEnemyRegion = NULL;
	double closestDistance = 100000;

	// for each region that our opponent occupies
	BOOST_FOREACH (BWTA::Region * region, UnitInfoState::getInstance()->getOccupiedRegions(BWAPI::Broodwar->enemy()))
	{
		double distance = region->getCenter().getDistance(BWAPI::Position(BWAPI::Broodwar->self()->getStartLocation()));

		if (!closestEnemyRegion || distance < closestDistance)
		{
			closestDistance = distance;
			closestEnemyRegion = region;
		}
	}

	return closestEnemyRegion;
}

void CombatCommander::assignDefenders(std::set<BWAPI::Unit *> & unitsToAssign) 
{
	// for each of our occupied regions
	BOOST_FOREACH(BWTA::Region * myRegion, UnitInfoState::getInstance()->getOccupiedRegions(BWAPI::Broodwar->self()))
	{
		BWAPI::Position regionCenter = myRegion->getCenter();
		if (!regionCenter.isValid())
		{
			continue;
		}

		// start off assuming all enemy units in region are just workers
		int numDefendersPerEnemyUnit = 1;

		// all of the enemy units in this region
		std::set<BWAPI::Unit *> enemyUnitsInRegion;
		BOOST_FOREACH (BWAPI::Unit * enemyUnit, BWAPI::Broodwar->enemy()->getUnits())
		{			
			if (BWTA::getRegion(BWAPI::TilePosition(enemyUnit->getPosition())) == myRegion)
			{
				enemyUnitsInRegion.insert(enemyUnit);

				// if the enemy isn't a worker, increase the amount of defenders for it
				if (!enemyUnit->getType().isWorker())
				{
					numDefendersPerEnemyUnit = 3;
				}
			}
		}

		// figure out how many units we need on defense
		const int numFlyingNeeded = numDefendersPerEnemyUnit * UnitInfoState::getInstance()->numEnemyFlyingUnitsInRegion(myRegion);
		const int numGroundNeeded = numDefendersPerEnemyUnit * UnitInfoState::getInstance()->numEnemyUnitsInRegion(myRegion);

		if(numGroundNeeded > 0 || numFlyingNeeded > 0)
		{
			// our defenders
			std::set<BWAPI::Unit *> flyingDefenders;
			std::set<BWAPI::Unit *> groundDefenders;

			BOOST_FOREACH (BWAPI::Unit * unit, unitsToAssign)
			{
				if (unit->getType().airWeapon() != BWAPI::WeaponTypes::None)
				{
					flyingDefenders.insert(unit);
				}
				else if (unit->getType().groundWeapon() != BWAPI::WeaponTypes::None)
				{
					groundDefenders.insert(unit);
				}
			}

			// the defense force we want to send
			UnitVector defenseForce;

			// get flying defenders
			for (int i=0; i<numFlyingNeeded && !flyingDefenders.empty(); ++i)
			{
				BWAPI::Unit * flyingDefender = findClosestDefender(enemyUnitsInRegion, flyingDefenders);
				defenseForce.push_back(flyingDefender);
				unitsToAssign.erase(flyingDefender);
				flyingDefenders.erase(flyingDefender);
			}

			// get ground defenders
			for (int i=0; i<numGroundNeeded && !groundDefenders.empty(); ++i)
			{
				BWAPI::Unit * groundDefender = findClosestDefender(enemyUnitsInRegion, groundDefenders);

				if (groundDefender->getType().isWorker())
				{
					WorkerManager::getInstance()->setCombatWorker(groundDefender);
				}

				defenseForce.push_back(groundDefender);
				unitsToAssign.erase(groundDefender);
				groundDefenders.erase(groundDefender);
			}

			// if we need a defense force, make the squad and give the order
			if (!defenseForce.empty()) 
			{
				currentAction = std::string("\x08 Defending base against force");
				squadData.setSquad(Squad(defenseForce, SquadOrder(SquadOrder::Defend, regionCenter, 1000)));

				// only defend one region at a time?
				return;
			}
		}
	}
}

void CombatCommander::assignAttackRegion(std::set<BWAPI::Unit *> & unitsToAssign, BWTA::Region * enemyRegion, SquadOrder & order) 
{
	// If we have found the enemy base region
	if (enemyRegion && enemyRegion->getCenter().isValid()) 
	{
		// get all the units in a 1000 radius of this area
		UnitVector oppUnitsInArea, ourUnitsInArea;
		MapGrid::getInstance()->GetUnits(oppUnitsInArea, enemyRegion->getCenter(), 800, false, true);
		MapGrid::getInstance()->GetUnits(ourUnitsInArea, enemyRegion->getCenter(), 200, true, false);


		// if our units are here but no enemy units are, leave
		bool ourUnitsAlone = !ourUnitsInArea.empty() && oppUnitsInArea.empty();

		if (!oppUnitsInArea.empty()) 
		{
			order.type		= SquadOrder::Attack;
			order.position	= enemyRegion->getCenter();
			currentAction	= std::string("\x08 Attacking enemy region");
		} 
		else 
		{
			currentAction = std::string("\x04 No Units In Area");
		}
	}
}

void CombatCommander::assignAttackVisibleUnits(std::set<BWAPI::Unit *> & unitsToAssign, SquadOrder & order) 
{
	// If there are any visible enemies anywhere
	std::set<BWAPI::Unit *> targets;

	BOOST_FOREACH (BWAPI::Unit * unit, BWAPI::Broodwar->enemy()->getUnits())
	{
		if (unit->isVisible())
		{
			targets.insert(unit);
		}
	}

	if(!targets.empty())
	{
		currentAction = std::string("\x08 Attack Visible");
		// Attack the first enemy (TODO: Attack the closest enemy?)
		order.type		= SquadOrder::Attack;
		order.position	= (*targets.begin())->getPosition();
	} 
	else 
	{
		//BWAPI::Broodwar->printf("I cannot see any visible units");
	}
}


void CombatCommander::assignAttackKnownBuildings(std::set<BWAPI::Unit *> & unitsToAssign, SquadOrder & order) 
{
	currentAction = std::string("\x08 Attack Known Buildings");

	// get all the units we think the enemy still has
	UnitInfoVector previouslySeen = UnitInfoState::getInstance()->getAllUnits(BWAPI::Broodwar->enemy());

	BOOST_FOREACH(UnitInfo ui, previouslySeen)
	{
		// If we saw a building, attack it
		if(ui.type.isBuilding())
		{
			//BWAPI::Broodwar->printf("I saw a %s, going to attack it!", ui.type.getName().c_str());
			
			order.type		= SquadOrder::Attack;
			order.position	= ui.lastPosition;	
			
			break;
		}
	}
}

void CombatCommander::assignAttackExplore(std::set<BWAPI::Unit *> & unitsToAssign, SquadOrder & order) 
{
	currentAction = std::string("\x07 Explore For Buildings");

	order.type = SquadOrder::Attack;
	order.position = MapGrid::getInstance()->getLeastExplored();
}

BWAPI::Unit* CombatCommander::findClosestDefender(std::set<BWAPI::Unit *> & enemyUnitsInRegion, const std::set<BWAPI::Unit *> & units) 
{
	BWAPI::Unit * closestUnit = NULL;
	double minDistance = 1000000;

	BOOST_FOREACH (BWAPI::Unit * enemyUnit, enemyUnitsInRegion) 
	{
		BOOST_FOREACH (BWAPI::Unit * unit, units)
		{
			double dist = unit->getDistance(enemyUnit);
			if (!closestUnit || dist < minDistance) 
			{
				closestUnit = unit;
				minDistance = dist;
			}
		}
	}

	return closestUnit;
}

BWAPI::Position CombatCommander::getDefendLocation()
{
	if (BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Nexus) >= 2)
	{
		return HardCodedInfo::getInstance()->getChokepoint(HardCodedInfo::NATURAL_CHOKE, BWAPI::Broodwar->self());
	}

	return HardCodedInfo::getInstance()->getChokepoint(HardCodedInfo::MAIN_CHOKE, BWAPI::Broodwar->self());
}