#include "Common.h"
#include "StarcraftBuildOrderSearchManager.h"

typedef std::map<MetaType, int> mapType;

// gotta keep c++ static happy
StarcraftBuildOrderSearchManager * StarcraftBuildOrderSearchManager::instance = NULL;

// get an instance of this
StarcraftBuildOrderSearchManager * StarcraftBuildOrderSearchManager::getInstance() 
{
	// if the instance doesn't exist, create it
	if (!StarcraftBuildOrderSearchManager::instance) 
	{
		StarcraftBuildOrderSearchManager::instance = new StarcraftBuildOrderSearchManager();
	}

	return StarcraftBuildOrderSearchManager::instance;
}

// constructor
StarcraftBuildOrderSearchManager::StarcraftBuildOrderSearchManager() : lastSearchFinishTime(0)
{
	StarcraftData::getInstance().init(BWAPI::Races::Protoss);
}

void StarcraftBuildOrderSearchManager::update(double timeLimit)
{
	starcraftSearchData.update(timeLimit);
}

// function which is called from the bot
std::vector<MetaType> StarcraftBuildOrderSearchManager::findBuildOrder(BWAPI::Race race, std::vector< std::pair<MetaType, int> > & goalUnits)
{
	return getMetaVector(search<ProtossState>(goalUnits));
}

// function which does all the searching
template <class StarcraftStateType>
SearchResults StarcraftBuildOrderSearchManager::search(std::vector< std::pair<MetaType, int> > & goalUnits)
{	
	// construct the Smart Starcraft Search
	SmartStarcraftSearch<StarcraftStateType> sss;

	// set the goal based on the input MetaType vector
	sss.setGoal(getGoal(goalUnits));

	// set the initial state to the current state of the BW game
	sss.setState(getCurrentState<StarcraftStateType>());

	// get the parameters from the smart search
	SearchParameters<StarcraftStateType> params = sss.getParameters();

	// pass this goal into the searcher
	starcraftSearchData.startNewSearch(params);

	// do the search and store the results
	SearchResults result = starcraftSearchData.getResults();

	if (result.solved)
	{
		lastSearchFinishTime = result.solutionLength;
		//BWAPI::Broodwar->printf("%12d[opt]%9llu[nodes]%7d[ms]", result.solutionLength, result.nodesExpanded, (int)result.timeElapsed);
	}
	else
	{
		//BWAPI::Broodwar->printf("No solution found!");
		//BWAPI::Broodwar->printf("%12d%12d%12d%14llu", result.upperBound, result.lowerBound, 0, result.nodesExpanded);
	}

	return result;
}


// gets the StarcraftState corresponding to the beginning of a Melee game
template <class StarcraftStateType>
StarcraftStateType StarcraftBuildOrderSearchManager::getStartState()
{
	return StarcraftStateType(true);
}

// parses current BWAPI game state and turns it into a StarcraftState of given type
template <class StarcraftStateType>
StarcraftStateType StarcraftBuildOrderSearchManager::getCurrentState()
{
	StarcraftStateType s;
	s.setFrame(BWAPI::Broodwar->getFrameCount());
	s.setSupply(BWAPI::Broodwar->self()->supplyUsed(), BWAPI::Broodwar->self()->supplyTotal());
	s.setResources(BWAPI::Broodwar->self()->minerals(), BWAPI::Broodwar->self()->gas());
	s.setMineralWorkers(WorkerManager::getInstance()->getNumMineralWorkers());
	s.setGasWorkers(WorkerManager::getInstance()->getNumGasWorkers());

	// for each unit we have
	BOOST_FOREACH(BWAPI::Unit * unit, BWAPI::Broodwar->self()->getUnits())
	{
		Action action(DATA.getAction(unit->getType()));

		// if the unit is completed
		if (unit->isCompleted())
		{
			// add the unit to the state
			s.addNumUnits(action, 1);

			// if it is a building
			if (unit->getType().isBuilding())
			{
				// add the building data accordingly
				s.addBuilding(action, unit->getRemainingTrainTime() + unit->getRemainingResearchTime() + unit->getRemainingUpgradeTime());

				if (unit->getRemainingResearchTime() > 0)
				{
					s.addActionInProgress(DATA.getAction(unit->getTech()), BWAPI::Broodwar->getFrameCount() + unit->getRemainingTrainTime());
				}
				else if (unit->getRemainingUpgradeTime() > 0)
				{
					s.addActionInProgress(DATA.getAction(unit->getUpgrade()), BWAPI::Broodwar->getFrameCount() + unit->getRemainingTrainTime());
				}
			}
		}

		// if the unit is under construction (building)
		if (unit->isBeingConstructed())
		{
			s.addActionInProgress(DATA.getAction(unit->getType()), BWAPI::Broodwar->getFrameCount() + unit->getRemainingBuildTime());
		}
	}

	BOOST_FOREACH (BWAPI::UpgradeType type, BWAPI::UpgradeTypes::allUpgradeTypes())
	{
		if (BWAPI::Broodwar->self()->getUpgradeLevel(type) > 0)
		{
			//BWAPI::Broodwar->printf("I have %s", type.getName().c_str());
			s.addNumUnits(DATA.getAction(type), BWAPI::Broodwar->self()->getUpgradeLevel(type));
		}
	}

	return s;
}

StarcraftSearchGoal StarcraftBuildOrderSearchManager::getGoal(std::vector< std::pair<MetaType, int> > & goalUnits)
{
	StarcraftSearchGoal goal;

	for (size_t i=0; i<goalUnits.size(); ++i)
	{
		goal.setGoal(getAction(goalUnits[i].first), goalUnits[i].second);
	}

	return goal;
}

// converts SearchResults.buildOrder vector into vector of MetaType
std::vector<MetaType> StarcraftBuildOrderSearchManager::getMetaVector(SearchResults & results)
{
	std::vector<MetaType> metaVector;

	std::vector<Action> & buildOrder = results.buildOrder;
	
	//Logger::getInstance()->log("Get Meta Vector:\n");

	// for each item in the results build order, add it
	for (size_t i(0); i<buildOrder.size(); ++i)
	{
		// retrieve the action from the build order
		Action a = buildOrder[buildOrder.size()-1-i];

		metaVector.push_back(getMetaType(a));
	}

	return metaVector;
}

// converts from MetaType to StarcraftSearch Action
Action StarcraftBuildOrderSearchManager::getAction(MetaType t)
{
	//Logger::getInstance()->log("Action StarcraftBuildOrderSearchManager::getAction(" + t.getName() + ")\n");

	return t.isUnit() ? DATA.getAction(t.unitType) : (t.isTech() ? DATA.getAction(t.techType) : DATA.getAction(t.upgradeType));
}

std::vector<MetaType> StarcraftBuildOrderSearchManager::getOpeningBuildOrder()
{
	std::vector<std::string> buildStrings;
	
	//initial build order
	buildStrings.push_back("0 0 0 0 1 0 0 3"); //0 = probe, 1 = pylon, 3 = gateway

	//buildStrings.push_back("0 0 0 0 1 0 0 3 0 0 3 0 1 3 0 4 4 4 4 4 1 0 4 4 4");

	return getMetaVector(buildStrings[rand() % buildStrings.size()]);
}

MetaType StarcraftBuildOrderSearchManager::getMetaType(Action a)
{
	const StarcraftAction & s = DATA[a];

	MetaType meta;

	// set the appropriate type
	if (s.getType() == StarcraftAction::UnitType)
	{
		meta = MetaType(s.getUnitType());
	}
	else if (s.getType() == StarcraftAction::UpgradeType)
	{
		meta = MetaType(s.getUpgradeType());
	} 
	else if (s.getType() == StarcraftAction::TechType)
	{
		meta = MetaType(s.getTechType());
	}
	else
	{
		assert(false);
	}
	
	return meta;
}

std::vector<MetaType> StarcraftBuildOrderSearchManager::getMetaVector(std::string buildString)
{
	std::stringstream ss;
	ss << buildString;
	std::vector<MetaType> meta;

	int action(0);
	while (ss >> action)
	{
		meta.push_back(getMetaType(action));
	}

	return meta;
}