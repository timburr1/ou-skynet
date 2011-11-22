#include "Common.h"
#include "StrategyManager.h"

const double Q_THRESHOLD = .75;
const double EPSILON = .1;

// gotta keep c++ static happy
StrategyManager * StrategyManager::instance = NULL;

// constructor
StrategyManager::StrategyManager() :	firstAttackSent(false),
enemyRace(BWAPI::Broodwar->enemy()->getRace())
{
	//setup neural nets
	BOOST_FOREACH(std::string name, protossUnits)
	{
		NeuralNet *n = new NeuralNet(BWAPI::UnitTypes::getUnitType(name));
		nets.push_back(n);
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
		BWAPI::Broodwar->printf("Expanding the base.");

		int nexi = BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Nexus);
		goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Protoss_Nexus, nexi + 1));
	}	

	double r = ((double)rand()/(double)RAND_MAX); 
	if(r < EPSILON)
	{
		int randomIndex = ((int) rand()) % nets.size();
		
		NeuralNet *n = nets.at(randomIndex);

		BWAPI::UnitType unitType = n->getUnit();
		//int newNumWanted = n->getNumWanted() + 1;
		//n->setNumWanted(newNumWanted);

		int num = BWAPI::Broodwar->self()->allUnitCount(unitType);
		int numWanted = num + 1;

		goal.push_back(std::pair<MetaType, int>(unitType, numWanted));

		BWAPI::Broodwar->printf("Randomly adding %s to goal. Have %d, want %d", 
			unitType.getName().c_str(), num, numWanted);		

		if(!contains(netsToUpdate, n))
			netsToUpdate.push_back(n);
	}
	else
	{
		double maxQ = -1000.0;
		NeuralNet *netToBuild = NULL;
		
		//setup parameters
		double params [33];

		params[0] = 1.0; //bias
		params[1] = 0.0; //total units
		params[2] = //available unit capacity
			BWAPI::Broodwar->self()->supplyTotal() - BWAPI::Broodwar->self()->supplyUsed(); 
		params[3] = log10((double) BWAPI::Broodwar->self()->minerals());
		params[4] = log10((double) BWAPI::Broodwar->self()->gas());

		for(int x=0; x < 28; x++) 
		{
			int num = BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::getUnitType(protossUnitsAndBuildings[x]));

			params[x+5] = num;				
			params[1] += num;
		}

		BOOST_FOREACH(NeuralNet *n, nets)
		{		
			double thisQ = n->getQ(params);

			if(thisQ > maxQ)
			{
				maxQ = thisQ;
				netToBuild = n;		
			}
		}

		if(netToBuild != NULL)
		{
			BWAPI::UnitType unitType = netToBuild->getUnit();
			int num = BWAPI::Broodwar->self()->allUnitCount(unitType);
			int numWanted = num + 1;
			/*netToBuild->getNumWanted() + 1;
			netToBuild->setNumWanted(newNumWanted);*/

			BWAPI::Broodwar->printf("Adding %s to goal. Q = %f.Have, %d, want %d", 
				unitType.getName().c_str(), maxQ, num, numWanted);
	
			goal.push_back(std::pair<MetaType, int>(unitType, numWanted));

			if(!contains(netsToUpdate, netToBuild))
				netsToUpdate.push_back(netToBuild);
		}
	}

	currentGoal = goal;
	return goal;	
}

void StrategyManager::updateNeuralNets(int score)
{
	//setup parameters
	double params [33];

	params[0] = 1.0; //bias
	params[1] = 0.0; //total units
	params[2] = //available unit capacity
		BWAPI::Broodwar->self()->supplyTotal() - BWAPI::Broodwar->self()->supplyUsed(); 
	params[3] = log10((double) BWAPI::Broodwar->self()->minerals());
	params[4] = log10((double) BWAPI::Broodwar->self()->gas());

	for(int x=0; x < 28; x++) 
	{
		int num = BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::getUnitType(protossUnitsAndBuildings[x]));

		params[x+5] = num;				
		params[1] += num;
	}

	//estimate the max q value of the next state
	double maxQ = -1000.0;
	BOOST_FOREACH(NeuralNet *net, nets)
	{
		double thisQ = net->getQ(params);
		if(thisQ > maxQ) 
			maxQ = thisQ;
	}

	//now actually update the weights
	BOOST_FOREACH(NeuralNet *net, netsToUpdate)
	{
		net->updateWeights((double) score, maxQ);
	}

	netsToUpdate.clear();
}

void StrategyManager::onEnd(int score)
{
	/*BOOST_FOREACH(NeuralNet *net, netsToUpdate)
	{
		net->updateWeights((double) score);
	}*/

	BOOST_FOREACH(NeuralNet *net, nets)
	{
		net->writeWeightsToFile();
	}
}

bool StrategyManager::contains(std::vector<NeuralNet*> nets, NeuralNet *netToTest)
{
	BOOST_FOREACH(NeuralNet *thisNet, nets)
	{
		if(thisNet->getUnit().getName().compare(netToTest->getUnit().getName()) == 0)
			return true;
	}

	return false;
}