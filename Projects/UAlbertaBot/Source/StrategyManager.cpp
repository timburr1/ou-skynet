#include "Common.h"
#include "StrategyManager.h"

// gotta keep c++ static happy
StrategyManager * StrategyManager::instance = NULL;

// constructor
StrategyManager::StrategyManager() :	firstAttackSent(false),
										enemyRace(BWAPI::Broodwar->enemy()->getRace())
{

}

// get an instance of this
StrategyManager * StrategyManager::getInstance() 
{
	// if the instance doesn't exist, create it
	if (!StrategyManager::instance) 
	{
		StrategyManager::instance = new StrategyManager();
	}

	return StrategyManager::instance;
}

void StrategyManager::drawGoalInformation(int x, int y)
{
	if (DRAW_UALBERTABOT_DEBUG) BWAPI::Broodwar->drawTextScreen(x, y-10, "Search Goal:");

	for (int i=0; i<(int)currentGoal.size(); ++i)
	{
		if (DRAW_UALBERTABOT_DEBUG) BWAPI::Broodwar->drawTextScreen(x, y+(i*10), "%s", currentGoal[i].first.getName().c_str());
		if (DRAW_UALBERTABOT_DEBUG) BWAPI::Broodwar->drawTextScreen(x+100, y+(i*10), "%d", currentGoal[i].second);
	}
}

// when do we want to defend with our workers?
// this function can only be called if we have no fighters to defend with
bool StrategyManager::defendWithWorkers()
{
	// our home nexus position
	BWAPI::Position homePosition = BWTA::getStartLocation(BWAPI::Broodwar->self())->getPosition();;

	// enemy units near our workers
	int enemyUnitsNearWorkers = 0;

	// defense radius of nexus
	int defenseRadius = 300;

	// fill the set with the types of units we're concerned about
	BOOST_FOREACH (BWAPI::Unit * unit, BWAPI::Broodwar->enemy()->getUnits())
	{
		// if it's a zergling or a worker we want to defend
		if (unit->getType() == BWAPI::UnitTypes::Zerg_Zergling || unit->getType().isWorker())
		{
			if (unit->getDistance(homePosition) < defenseRadius)
			{
				enemyUnitsNearWorkers++;
			}
		}
	}

	// if there are enemy units near our workers, we want to defend
	return enemyUnitsNearWorkers > 0;
}

// called by combat commander to determine whether or not to send an attack force
// freeUnits are the units available to do this attack
bool StrategyManager::doAttack(const UnitVector & freeUnits)
{
	int enemyForceSize = UnitInfoState::getInstance()->knownForceSize(BWAPI::Broodwar->enemy());
	int ourForceSize = (int)freeUnits.size();

	int numUnitsNeededForAttack = 3;

	bool doAttack  = BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Dark_Templar) >= 1
					|| ourForceSize >= numUnitsNeededForAttack;

	if (doAttack)
	{
		firstAttackSent = true;
	}

	return doAttack || firstAttackSent;
}

bool StrategyManager::expand()
{
	// if there is no place to expand to, we can't expand
	if (MapTools::getInstance()->getNextExpansion() == BWAPI::TilePositions::None)
	{
		return false;
	}

	int numNexus =				BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Nexus);
	int numZealots =			BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Zealot);
	int frame =					BWAPI::Broodwar->getFrameCount();

	// if there are more than 10 idle workers, expand
	if (WorkerManager::getInstance()->getNumIdleWorkers() > 10)
	{
		return true;
	}

	// 2nd Nexus Conditions:
	//		We have 12 or more zealots
	//		It is past frame 7000
	if ((numNexus < 2) && (numZealots > 12 || frame > 9000))
	{
		return true;
	}

	// 3nd Nexus Conditions:
	//		We have 24 or more zealots
	//		It is past frame 12000
	if ((numNexus < 3) && (numZealots > 24 || frame > 15000))
	{
		return true;
	}

	if ((numNexus < 4) && (numZealots > 24 || frame > 21000))
	{
		return true;
	}

	if ((numNexus < 5) && (numZealots > 24 || frame > 26000))
	{
		return true;
	}

	if ((numNexus < 6) && (numZealots > 24 || frame > 30000))
	{
		return true;
	}

	return false;
}

std::vector< std::pair<MetaType,int> > StrategyManager::getBuildOrderGoal()
{
	// the goal to return
	std::vector< std::pair<MetaType, int> > goal;

	//goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Protoss_Probe, 1));


	int numZealots =			BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Zealot);
	int numDragoons =			BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Dragoon);
	int numProbes =				BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Probe);
	int numNexusCompleted =		BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Nexus);
	int numNexusAll =			BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Nexus);
	int numCyber =				BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Cybernetics_Core);
	int numCannon =				BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Photon_Cannon);
	int numGas =				BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Assimilator);

	bool nexusInProgress =		BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Nexus) > 
								BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Nexus);

	int zealotsWanted = numZealots + 8;
	int dragoonsWanted = numDragoons;
	int gatewayWanted = 3;
	int probesWanted = numProbes + 4;

	if (UnitInfoState::getInstance()->enemyHasCloakedUnits())
	{
		goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Protoss_Photon_Cannon, 3));
		if (numCannon > 0)
		{
			goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Protoss_Robotics_Facility, 1));
		}
		if (BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Robotics_Facility) > 0)
		{
			goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Protoss_Observatory, 1));
		}
		if (BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Observatory) > 0)
		{
			goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Protoss_Observer, 1));
		}
	}

	if (numNexusAll >= 2 || BWAPI::Broodwar->getFrameCount() > 9000)
	{
		gatewayWanted = 6;
		goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Protoss_Assimilator, 1));
		goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Protoss_Cybernetics_Core, 1));
	}

	if (numCyber > 0)
	{
		dragoonsWanted = numDragoons + 2;
		goal.push_back(std::pair<MetaType, int>(BWAPI::UpgradeTypes::Singularity_Charge, 1));
	}

	if (numNexusCompleted >= 3)
	{
		gatewayWanted = 8;
		dragoonsWanted = numDragoons + 6;
		goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Protoss_Observer, 1));
	}

	if (numNexusAll > 1)
	{
		probesWanted = numProbes + 6;
	}

	if (expand())
	{
		goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Protoss_Nexus, numNexusAll + 1));
	}

	goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Protoss_Dragoon,	dragoonsWanted));
	goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Protoss_Zealot,	zealotsWanted));
	goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Protoss_Gateway,	gatewayWanted));
	goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Protoss_Probe,	std::min(90, probesWanted)));

	currentGoal = goal;
	return goal;
	
}
