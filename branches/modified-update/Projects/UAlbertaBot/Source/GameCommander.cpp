#include "Common.h"
#include "GameCommander.h"

GameCommander::GameCommander() : numWorkerScouts(0), currentScout(NULL)
{

}

void GameCommander::update()
{
	timerManager.startTimer(TimerManager::All);

	// populate the unit vectors we will pass into various managers
	populateUnitVectors();

	// economy and base managers
	timerManager.startTimer(TimerManager::Worker);
	WorkerManager::getInstance()->update();
	timerManager.stopTimer(TimerManager::Worker);

	timerManager.startTimer(TimerManager::Production);
	ProductionManager::getInstance()->update();
	timerManager.stopTimer(TimerManager::Production);

	timerManager.startTimer(TimerManager::Building);
	BuildingManager::getInstance()->update();
	timerManager.stopTimer(TimerManager::Building);

	// combat and scouting managers
	timerManager.startTimer(TimerManager::Combat);
	combatCommander.update(combatUnits);
	timerManager.startTimer(TimerManager::Combat);

	timerManager.startTimer(TimerManager::Scout);
	scoutManager.update(scoutUnits);
	timerManager.stopTimer(TimerManager::Scout);

	// utility managers
	timerManager.startTimer(TimerManager::UnitInfoState);
	UnitInfoState::getInstance()->update();
	timerManager.stopTimer(TimerManager::UnitInfoState);

	timerManager.startTimer(TimerManager::MapGrid);
	MapGrid::getInstance()->update();
	timerManager.stopTimer(TimerManager::MapGrid);

	timerManager.startTimer(TimerManager::MapTools);
	MapTools::getInstance()->update();
	timerManager.stopTimer(TimerManager::MapTools);

	timerManager.startTimer(TimerManager::Search);
	double searchTimeLimit = 35 - timerManager.getTotalElapsed();
	StarcraftBuildOrderSearchManager::getInstance()->update(searchTimeLimit);
	timerManager.stopTimer(TimerManager::Search);
		
	
	timerManager.stopTimer(TimerManager::All);

	timerManager.displayTimers(490, 225);
}

// assigns units to various managers
void GameCommander::populateUnitVectors()
{
	// filter our units for those which are valid and usable
	setValidUnits();

	// set each type of unit
	setScoutUnits();
	setCombatUnits();
	setWorkerUnits();
}

// validates units as usable for distribution to various managers
void GameCommander::setValidUnits()
{
	validUnits.clear();

	// make sure the unit is completed and alive and usable
	BOOST_FOREACH(BWAPI::Unit * unit, BWAPI::Broodwar->self()->getUnits())
	{
		if (isValidUnit(unit))
		{	
			validUnits.push_back(UnitToAssign(unit));
		}
	}
}

// selects which units will be scouting
// currently only selects the worker scout after first pylon built
// this does NOT take that worker away from worker manager, but it still works
// TODO: take this worker away from worker manager in a clever way
void GameCommander::setScoutUnits()
{
	// add anything that can attack or heal to the combat force
	for (size_t i(0); i<validUnits.size(); ++i)
	{
		// if this unit is already assigned, skip it
		if (validUnits[i].isAssigned)
		{
			continue;
		}

		// if we have just built our first suply provider, set the worker to a scout
		if (numWorkerScouts == 0)
		{
			// get the first supply provider we come across in our units, this should be the first one we make
			BWAPI::Unit * supplyProvider = getFirstSupplyProvider();

			// if it exists
			if (supplyProvider)
			{
				// increase the number of worker scouts so this won't happen again
				numWorkerScouts++;

				// grab the closest worker to the supply provider to send to scout
				int workerScoutIndex = getClosestUnitToTarget(BWAPI::Broodwar->self()->getRace().getWorker(), supplyProvider->getPosition());

				// if we find a worker (which we should) add it to the scout vector
				if (workerScoutIndex != -1)
				{
					scoutUnits.push_back(validUnits[workerScoutIndex].unit);
					validUnits[workerScoutIndex].isAssigned = true;
					WorkerManager::getInstance()->setScoutWorker(validUnits[workerScoutIndex].unit);
				}
			}
		}
	}
}

// sets combat units to be passed to CombatCommander
void GameCommander::setCombatUnits()
{
	combatUnits.clear();

	// add anything that can attack or heal to the combat force
	for (size_t i(0); i<validUnits.size(); ++i)
	{
		// if this unit is already assigned, skip it
		if (validUnits[i].isAssigned)
		{
			continue;
		}

		if (isCombatUnit(validUnits[i].unit))		
		{	
			combatUnits.push_back(validUnits[i].unit);
			validUnits[i].isAssigned = true;
		}
	}


	// emergency situation, enemy is in our base and we have no combat units
	// add our workers to the combat force
	if (combatUnits.empty() && StrategyManager::getInstance()->defendWithWorkers())
	{
		// add anything that can attack or heal to the combat force
		for (size_t i(0); i<validUnits.size(); ++i)
		{
			// if this unit is already assigned, skip it
			if (validUnits[i].isAssigned)
			{
				continue;
			}

			// if it's a worker
			if (validUnits[i].unit->getType().isWorker())			
			{	
				// only assign it to combat if it's not building something
				if (!WorkerManager::getInstance()->isBuilder(validUnits[i].unit))
				{
					combatUnits.push_back(validUnits[i].unit);
					validUnits[i].isAssigned = true;
				}
			}
		}
	}
}

bool GameCommander::isCombatUnit(BWAPI::Unit * unit) const
{
	// no workers or buildings allowed
	if (unit->getType().isWorker() || unit->getType().isBuilding())
	{
		return false;
	}

	// check for various types of combat units
	if (unit->getType().canAttack() || 
		unit->getType() == BWAPI::UnitTypes::Terran_Medic ||
		unit->getType() == BWAPI::UnitTypes::Protoss_High_Templar ||
		unit->getType() == BWAPI::UnitTypes::Protoss_Observer)
	{
		return true;
	}
		
	return false;
}

BWAPI::Unit * GameCommander::getFirstSupplyProvider()
{
	BWAPI::Unit * supplyProvider = NULL;

	BOOST_FOREACH(BWAPI::Unit * unit, BWAPI::Broodwar->self()->getUnits())
	{
		if (unit->getType() == BWAPI::Broodwar->self()->getRace().getSupplyProvider())
		{
			supplyProvider = unit;
		}
	}

	return supplyProvider;
}

void GameCommander::setWorkerUnits()
{
	workerUnits.clear();

	// add all workers that have yet to be assigned
	for (size_t i(0); i<validUnits.size(); ++i)
	{
		// if this unit is already assigned, skip it
		if (validUnits[i].isAssigned)
		{
			continue;
		}

		if (validUnits[i].unit->getType().isWorker())			
		{	
			workerUnits.push_back(validUnits[i].unit);
			validUnits[i].isAssigned = true;
		}
	}

}

bool GameCommander::isValidUnit(BWAPI::Unit * unit)
{
	if (	unit->isCompleted() && unit->getHitPoints() > 0 && unit->exists() &&
			unit->getType() != BWAPI::UnitTypes::Unknown && 
			unit->getPosition().x() != BWAPI::Positions::Unknown.x() &&
			unit->getPosition().y() != BWAPI::Positions::Unknown.y()) 
	{
		return true;
	}
	else
	{
		return false;
	}
}

void GameCommander::onUnitShow(BWAPI::Unit * unit)			
{ 
	UnitInfoState::getInstance()->onUnitShow(unit); 
	WorkerManager::getInstance()->onUnitShow(unit);
}

void GameCommander::onUnitHide(BWAPI::Unit * unit)			
{ 
	UnitInfoState::getInstance()->onUnitHide(unit); 
}

void GameCommander::onUnitCreate(BWAPI::Unit * unit)		
{ 
	UnitInfoState::getInstance()->onUnitCreate(unit); 
}

void GameCommander::onUnitRenegade(BWAPI::Unit * unit)		
{ 
	UnitInfoState::getInstance()->onUnitRenegade(unit); 
}

void GameCommander::onUnitDestroy(BWAPI::Unit * unit)		
{ 	
	ProductionManager::getInstance()->onUnitDestroy(unit);
	WorkerManager::getInstance()->onUnitDestroy(unit);
	BuildingManager::getInstance()->onUnitDestroy(unit);
	UnitInfoState::getInstance()->onUnitDestroy(unit); 
}

void GameCommander::onUnitMorph(BWAPI::Unit * unit)		
{ 
	//ProductionManager::getInstance()->onUnitMorph(unit);
	BuildingManager::getInstance()->onUnitMorph(unit);
	UnitInfoState::getInstance()->onUnitMorph(unit);
	WorkerManager::getInstance()->onUnitMorph(unit);
}

void GameCommander::onSendText(std::string text)
{
	ProductionManager::getInstance()->onSendText(text);

	if (text.compare("0") == 0)
	{
		BWAPI::Broodwar->setLocalSpeed(0);
	}
	else if (atoi(text.c_str()) > 0)
	{
		BWAPI::Broodwar->setLocalSpeed(atoi(text.c_str()));
	}
}

int GameCommander::getClosestUnitToTarget(BWAPI::UnitType type, BWAPI::Position target)
{
	int closestUnitIndex = -1;
	double closestDist = 100000;

	for (size_t i(0); i < validUnits.size(); ++i)
	{
		if (validUnits[i].unit->getType() == type)
		{
			double dist = validUnits[i].unit->getDistance(target);
			if (dist < closestDist)
			{
				closestUnitIndex = i;
				closestDist = dist;
			}
		}
	}

	return closestUnitIndex;
}