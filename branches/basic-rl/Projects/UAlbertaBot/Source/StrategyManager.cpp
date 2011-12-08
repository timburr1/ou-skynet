#include "Common.h"
#include "StrategyManager.h"

const double Q_THRESHOLD = .75;
const double EPSILON = .05;

// gotta keep c++ static happy
StrategyManager * StrategyManager::instance = NULL;

// constructor
StrategyManager::StrategyManager() :	firstAttackSent(false),
enemyRace(BWAPI::Broodwar->enemy()->getRace())
{
	//setup Q functions
	BOOST_FOREACH(std::string name, protossUnits)
	{
		Qlearning *q = new Qlearning(BWAPI::UnitTypes::getUnitType(name));
		Qs.push_back(q);
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
		int randomIndex = ((int) rand()) % Qs.size();
		
		Qlearning *q = Qs.at(randomIndex);

		BWAPI::UnitType unitType = q->getUnit();
		//int newNumWanted = n->getNumWanted() + 1;
		//n->setNumWanted(newNumWanted);

		int num = BWAPI::Broodwar->self()->allUnitCount(unitType);
		int numWanted = num + 1;

		goal.push_back(std::pair<MetaType, int>(unitType, numWanted));

		BWAPI::Broodwar->printf("Randomly adding %s to goal. Have %d, want %d", 
			unitType.getName().c_str(), num, numWanted);		

		if(!contains(QsToUpdate, q))
			QsToUpdate.push_back(q);
	}
	else
	{
		double maxQ = -100000.0;
		Qlearning *qToBuild = NULL;
		
		BOOST_FOREACH(Qlearning *q, Qs)
		{
			double thisQ = q->getQ();

			if(thisQ > maxQ)
			{
				maxQ = thisQ;
				qToBuild = q;		
			}
		}

		if(qToBuild != NULL)
		{
			BWAPI::UnitType unitType = qToBuild->getUnit();
			int num = BWAPI::Broodwar->self()->allUnitCount(unitType);
			int numWanted = num + 1;
			/*netToBuild->getNumWanted() + 1;
			netToBuild->setNumWanted(newNumWanted);*/

			BWAPI::Broodwar->printf("Adding %s to goal. Q = %f.Have, %d, want %d", 
				unitType.getName().c_str(), maxQ, num, numWanted);
	
			goal.push_back(std::pair<MetaType, int>(unitType, numWanted));

			if(!contains(QsToUpdate, qToBuild))
			{
				QsToUpdate.push_back(qToBuild);
				qToBuild->setTimeActionWasChosen(BWAPI::Broodwar->getFrameCount());
				qToBuild->setPrevPredictedQ(maxQ);
			}
		}
	}

	currentGoal = goal;
	return goal;	
}

void StrategyManager::updateQs(int score)
{
	//determine the maxQ at this time
	double maxQ = -100000.0;
	BOOST_FOREACH(Qlearning *q, Qs)
		{

			double thisQ = q->getQ();

			if(thisQ > maxQ)
			{
				maxQ = thisQ;	
			}
		}


	BOOST_FOREACH(Qlearning *q, QsToUpdate)
	{
		q->updateQ((double) score, maxQ);//score is our reward
	}

	QsToUpdate.clear();
}

void StrategyManager::onEnd(int score)
{
	StrategyManager::updateQs(score);

	BOOST_FOREACH(Qlearning *q, Qs)
	{
		q->writeQsToFile();
	}
}

bool StrategyManager::contains(std::vector<Qlearning*> updateQs, Qlearning *QToTest)
{
	BOOST_FOREACH(Qlearning *thisQ, updateQs)
	{
		if(thisQ->getUnit().getName().compare(QToTest->getUnit().getName()) == 0)
			return true;
	}

	return false;
}