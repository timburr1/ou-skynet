#include "Common.h"
#include "StrategyManager.h"

const double Q_THRESHOLD = 0.5;
const double EPSILON = .01;
const std::string names[] = {
	"Protoss Probe",
	"Protoss Pylon",
	"Protoss Nexus",
	"Protoss Gateway",
	"Protoss Zealot",
	"Protoss Cybernetics Core",
	"Protoss Dragoon",
	"Protoss Assimilator",
	"Protoss Forge",
	"Protoss Photon Cannon",
	"Protoss High Templar",
	"Protoss Citadel of Adun",
	"Protoss Templar Archives",
	"Protoss Robotics Facility",
	"Protoss Robotics Support Bay",
	"Protoss Observatory",
	"Protoss Stargate",
	"Protoss Scout",
	"Protoss Arbiter Tribunal",
	"Protoss Arbiter",
	"Protoss Shield Battery",
	"Protoss Dark Templar",
	"Protoss Shuttle",
	"Protoss Reaver",
	"Protoss Observer",
	"Protoss Corsair",
	"Protoss Fleet Beacon",
	"Protoss Carrier"};

// gotta keep c++ static happy
StrategyManager * StrategyManager::instance = NULL;

// constructor
StrategyManager::StrategyManager() :	firstAttackSent(false),
										enemyRace(BWAPI::Broodwar->enemy()->getRace())
{
	//setup neural nets
	BOOST_FOREACH(std::string name, names)
	{
		nets.push_back(NeuralNet(BWAPI::UnitTypes::getUnitType(name)));
	}

	//seed RNG for epsilon later
	srand((unsigned) time(NULL));
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

	if (expand())
	{
		int nexi = BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Nexus);
		goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Protoss_Nexus, nexi + 1));
	}	
	
	BOOST_FOREACH(NeuralNet n, nets)
	{
		MetaType unit = MetaType(n.getUnit());

		if(ProductionManager::getInstance()->canProduce(unit))
		{
			double params [33];

			params[0] = 1.0; //bias
			params[1] = 0.0; //total units
			params[2] = //available unit capacity
				BWAPI::Broodwar->self()->supplyTotal() - BWAPI::Broodwar->self()->supplyUsed(); 
			params[3] = log10((double) BWAPI::Broodwar->self()->minerals());
			params[4] = log10((double) BWAPI::Broodwar->self()->gas());

			for(int x=0; x <= 27; x++) 
			{
				int num = BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::getUnitType(names[x]));
				
				params[x+5] = num;				
				params[1] += num;
			}			
	
			double thisQ = n.getQ(params);
			double r = ((double)rand()/(double)RAND_MAX); 

			if(thisQ > Q_THRESHOLD || r < EPSILON)
			{
				BWAPI::Broodwar->printf("Adding %s to build order, Q = %d", 
					unit.getName().c_str(), thisQ);
				
				goal.push_back(std::pair<MetaType, int>(unit, 1));
				netsToUpdate.push_back(n);
			}
		}
	}

	currentGoal = goal;
	return goal;	
}