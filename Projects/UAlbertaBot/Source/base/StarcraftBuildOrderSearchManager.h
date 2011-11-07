#pragma once

#include "Common.h"
#include "BuildOrderQueue.h"
#include "WorkerManager.h"
#include "starcraftsearch\ActionSet.hpp"
#include "starcraftsearch\ProtossState.hpp"
#include "starcraftsearch\TerranState.hpp"
#include "starcraftsearch\DFBBStarcraftSearch.hpp"
#include "starcraftsearch\StarcraftState.hpp"
#include "starcraftsearch\StarcraftSearchGoal.hpp"
#include "starcraftsearch\SmartStarcraftSearch.hpp"
#include "starcraftsearch\StarcraftData.hpp"
#include "starcraftsearch\SearchSaveState.hpp"

#include "StarcraftSearchData.h"

class StarcraftBuildOrderSearchManager
{
	// starcraftSearchData is hard coded to be protoss state for now
	StarcraftSearchData<ProtossState> starcraftSearchData;

	int lastSearchFinishTime;

	// returns StarcraftStateType from current BWAPI state
	template <class StarcraftStateType>
	StarcraftStateType			getCurrentState();

	// gets a sample starting state for a race
	template <class StarcraftStateType>
	StarcraftStateType			getStartState();
	
	// starts a search
	template <class StarcraftStateType>
	SearchResults				search(std::vector< std::pair<MetaType, int> > & goalUnits);

	SearchResults				previousResults;

	std::vector<MetaType>		getMetaVector(SearchResults & results);

	Action						getAction(MetaType t);

	StarcraftSearchGoal			getGoal(std::vector< std::pair<MetaType, int> > & goalUnits);

	void						setRepetitions(StarcraftSearchGoal goal);

	void						loadOpeningBook();
	std::vector<std::vector<MetaType>> openingBook;
	std::vector<MetaType>		getMetaVector(std::string buildString);
	MetaType					getMetaType(Action a);
	
	StarcraftBuildOrderSearchManager();
	static StarcraftBuildOrderSearchManager *	instance;

public:

	~StarcraftBuildOrderSearchManager() {}

	static StarcraftBuildOrderSearchManager *	getInstance();

	void						update(double timeLimit);

	void						reset();

	void						setActionGoal(Action a, int count);
	void						setActionK(Action a, int k);

	std::vector<MetaType>		getOpeningBuildOrder();
	
	std::vector<MetaType>		findBuildOrder(BWAPI::Race race, std::vector< std::pair<MetaType, int> > & goalUnits);
};