#include "BWAPI.h"
#include <stdio.h>
#include <iostream>
#include <fstream>

#include "DependencyGraph.hpp"
#include "ProtossState.hpp"
#include "TerranState.hpp"
#include "ZergState.hpp"
#include "DFBBStarcraftSearch.hpp"
#include "StarcraftSearchConstraint.hpp"
#include "SmartStarcraftSearch.hpp"
#include "BuildOrderComparator.hpp"

// returns a sample protoss goal
StarcraftSearchGoal defaultProtossGoal()
{
	// the goal itself
	StarcraftSearchGoal goal;
	
	// the things we want to limit in the search
	goal.setGoalMax(DATA.getAction(BWAPI::UnitTypes::Protoss_Nexus), 1);
	goal.setGoalMax(DATA.getAction(BWAPI::UnitTypes::Protoss_Cybernetics_Core), 1);
	goal.setGoalMax(DATA.getAction(BWAPI::UnitTypes::Protoss_Assimilator), 1);
	goal.setGoalMax(DATA.getAction(BWAPI::UnitTypes::Protoss_Citadel_of_Adun), 1);
	goal.setGoalMax(DATA.getAction(BWAPI::UnitTypes::Protoss_Templar_Archives), 1);
	goal.setGoalMax(DATA.getAction(BWAPI::UnitTypes::Protoss_Robotics_Facility), 1);
	goal.setGoalMax(DATA.getAction(BWAPI::UnitTypes::Protoss_Fleet_Beacon), 1);
	goal.setGoalMax(DATA.getAction(BWAPI::UnitTypes::Protoss_Arbiter_Tribunal), 1);
	
	// return the goal!
	return goal;
}

StarcraftSearchGoal defaultTerranGoal()
{
	// the goal itself
	StarcraftSearchGoal goal;
	
	// the things we want to limit in the search
	goal.setGoalMax(DATA.getAction(BWAPI::UnitTypes::Terran_Command_Center), 1);
	goal.setGoalMax(DATA.getAction(BWAPI::UnitTypes::Terran_Academy), 1);
	goal.setGoalMax(DATA.getAction(BWAPI::UnitTypes::Terran_Refinery), 1);
	goal.setGoalMax(DATA.getAction(BWAPI::UnitTypes::Terran_Engineering_Bay), 1);
	
	// return the goal!
	return goal;
}

StarcraftSearchGoal defaultZergGoal()
{
	// the goal itself
	StarcraftSearchGoal goal;
	
	// the things we want to limit in the search
	goal.setGoalMax(DATA.getAction(BWAPI::UnitTypes::Zerg_Hatchery), 1);
	goal.setGoalMax(DATA.getAction(BWAPI::UnitTypes::Zerg_Spawning_Pool), 1);
	goal.setGoalMax(DATA.getAction(BWAPI::UnitTypes::Zerg_Lair), 1);
	goal.setGoalMax(DATA.getAction(BWAPI::UnitTypes::Zerg_Hydralisk_Den), 1);
	
	// return the goal!
	return goal;
}

ProtossState openingBookState()
{
	ProtossState state(true);
	
	Action probe = DATA.getWorker();
	Action pylon = DATA.getSupplyProvider();
	Action gateway = DATA.getAction(BWAPI::UnitTypes::Protoss_Gateway);
	
	state.doAction(probe, state.resourcesReady(probe));
	state.doAction(probe, state.resourcesReady(probe));
	state.doAction(probe, state.resourcesReady(probe));
	state.doAction(probe, state.resourcesReady(probe));
	state.doAction(pylon, state.resourcesReady(pylon));
	state.doAction(probe, state.resourcesReady(probe));
	state.doAction(probe, state.resourcesReady(probe));
	state.doAction(gateway, state.resourcesReady(gateway));
	state.doAction(probe, state.resourcesReady(probe));
	state.doAction(probe, state.resourcesReady(probe));
	
	return state;
}

TerranState openingBookStateTerran()
{
	TerranState state(true);
	
	Action scv = DATA.getWorker();
	Action supply = DATA.getSupplyProvider();
	Action barracks = DATA.getAction(BWAPI::UnitTypes::Terran_Barracks);
	
	state.doAction(scv, state.resourcesReady(scv));
	state.doAction(scv, state.resourcesReady(scv));
	state.doAction(scv, state.resourcesReady(scv));
	state.doAction(scv, state.resourcesReady(scv));
	state.doAction(scv, state.resourcesReady(scv));
	state.doAction(supply, state.resourcesReady(supply));
	state.doAction(scv, state.resourcesReady(scv));
	state.doAction(scv, state.resourcesReady(scv));
	state.doAction(barracks, state.resourcesReady(barracks));
	state.doAction(scv, state.resourcesReady(scv));
	state.doAction(barracks, state.resourcesReady(barracks));
	
	return state;
}

ZergState openingBookState2()
{
	ZergState state(true);
	state.printData();
	Action drone = DATA.getWorker();
	//Action overlord = DATA.getSupplyProvider();
	Action zergling = DATA.getAction(BWAPI::UnitTypes::Zerg_Zergling);
	Action spawning = DATA.getAction(BWAPI::UnitTypes::Zerg_Spawning_Pool);
	
	state.doAction(spawning, state.resourcesReady(spawning)); state.printData();
	state.doAction(drone, state.resourcesReady(drone)); state.printData();
	state.doAction(drone, state.resourcesReady(drone)); state.printData();
	state.doAction(drone, state.resourcesReady(drone)); state.printData();
	state.doAction(zergling, state.resourcesReady(zergling)); state.printData();
	state.doAction(zergling, state.resourcesReady(zergling)); state.printData();
	state.doAction(zergling, state.resourcesReady(zergling)); state.printData();
	state.doAction(drone, state.resourcesReady(drone)); state.printData();
	
	return state;
}

void doSaveSearch(SearchParameters<ProtossState> params, int timeOut)
{
	params.print();
	
	params.searchTimeLimit = timeOut;
	SearchResults result;
	result.timedOut = true;
	
	int totalTime = 0;
	
	while (result.timedOut)
	{
		// do the search
		DFBBStarcraftSearch<ProtossState> SCSearch(params);
		result = SCSearch.search();
		
		params.useSaveState = true;
		params.saveState = result.saveState;
		
		totalTime += result.timeElapsed;
	}
	
	//printf("\nTotal Time Elapsed: %d ms\n", totalTime);
}

void testSingleSearch()
{
	SearchParameters<ProtossState> params(defaultProtossGoal(), openingBookState(), StarcraftSearchConstraints());

	params.goal.setGoal( DATA.getAction(BWAPI::UnitTypes::Protoss_Zealot), 6);
	params.goal.setGoalMax( DATA.getAction(BWAPI::UnitTypes::Protoss_Probe), 15 );
	params.goal.setGoalMax( DATA.getAction(BWAPI::UnitTypes::Protoss_Pylon), 4 );
	params.goal.setGoalMax( DATA.getAction(BWAPI::UnitTypes::Protoss_Gateway), 4 );
	
	params.useRepetitions = true;
	//params.useSupplyBounding = true;
	//params.useAlwaysMakeWorkers = true;
	//params.setRepetitions( DATA.getAction(BWAPI::UnitTypes::Protoss_Probe), 2);
	//params.setRepetitions( DATA.getAction(BWAPI::UnitTypes::Protoss_Zealot), 2);
	//params.setRepetitions( DATA.getAction(BWAPI::UnitTypes::Protoss_Dragoon), 2);
	//params.useIncreasingRepetitions = true;
	
	//params.setRepetitions( DATA.getAction(BWAPI::UnitTypes::Protoss_Probe), 2);
	//params.setRepetitions( DATA.getAction(BWAPI::UnitTypes::Protoss_Pylon), 2);
	//params.setRepetitionThreshold( DATA.getAction(BWAPI::UnitTypes::Protoss_Pylon), 1);
	//params.setRepetitions( DATA.getAction(BWAPI::UnitTypes::Protoss_Carrier), 2);
	//params.setRepetitions( DATA.getAction(BWAPI::UnitTypes::Protoss_Stargate), 2);

	Timer t;

	doSaveSearch(params, 20);
	
	printf("Total Time Elapsed: %lf ms\n", t.getElapsedTimeInMilliSec());
}

void testSmartSearch()
{
	// default state
	ProtossState initialState(true);

	SmartStarcraftSearch<ProtossState> sss;
	sss.addGoal(DATA.getAction(BWAPI::UnitTypes::Protoss_Zealot), 8);
	sss.addGoal(DATA.getAction(BWAPI::UpgradeTypes::Leg_Enhancements), 1);
	sss.setState(initialState);
	
	// do the search
	SearchResults result = sss.search();
	
	if (result.solved)
	{
		result.printResults(true);
	}
}


void testState()
{
	ProtossState p1(true);
	
	p1.doAction(0, p1.resourcesReady(0));
	p1.doAction(0, p1.resourcesReady(0));
	p1.doAction(0, p1.resourcesReady(0));

	p1.save("p1.dat");


	ProtossState p2("p1.dat");	
	
}


void testTerranSearch()
{
	SearchParameters<TerranState> params(defaultTerranGoal(), openingBookStateTerran(), StarcraftSearchConstraints());
	
	params.goal.setGoal( DATA.getAction(BWAPI::UnitTypes::Terran_Wraith), 2);
	params.goal.setGoalMax( DATA.getAction(BWAPI::UnitTypes::Terran_Starport), 2);
	params.goal.setGoalMax( DATA.getAction(BWAPI::UnitTypes::Terran_Factory), 1);
	params.goal.setGoalMax( DATA.getAction(BWAPI::UnitTypes::Terran_SCV), 15 );
	params.goal.setGoalMax( DATA.getAction(BWAPI::UnitTypes::Terran_Supply_Depot), 4 );
	params.goal.setGoalMax( DATA.getAction(BWAPI::UnitTypes::Terran_Barracks), 1 );
	params.goal.setGoalMax( DATA.getAction(BWAPI::UnitTypes::Terran_Starport), 1 );
	//params.useAlwaysMakeWorkers = true;
	
	params.print();
	
	DFBBStarcraftSearch<TerranState> SCSearch(params);
	
	SearchResults result = SCSearch.search();
	
	if (result.solved)
	{
		result.printResults(true);
	}
}

void testZergSearch()
{
	SearchParameters<ZergState> params(defaultZergGoal(), ZergState(true), StarcraftSearchConstraints());
	
	params.goal.setGoal( DATA.getAction(BWAPI::UnitTypes::Zerg_Zergling), 30);
	params.goal.setGoalMax( DATA.getAction(BWAPI::UnitTypes::Zerg_Drone), 15 );
	params.goal.setGoalMax( DATA.getAction(BWAPI::UnitTypes::Zerg_Overlord), 2 );
	params.useLandmarkLowerBoundHeuristic = false;
	
	params.print();
	
	DFBBStarcraftSearch<ZergState> SCSearch(params);
	
	SearchResults result = SCSearch.search();
	
	if (result.solved)
	{
		result.printResults(true);
	}
}


int main(int argc, char ** argv)
{
    BWAPI::BWAPI_init();
	StarcraftData::getInstance().init(BWAPI::Races::Protoss);
	
	testSingleSearch();
    return 0;
}

