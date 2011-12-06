#ifndef STARCRAFT_STATE_H
#define STARCRAFT_STATE_H

#define GSN_DEBUG 0
#define MAX_ACTIONS 30
#define BUILDING_PLACEMENT 24 * 5

#define DATA (StarcraftData::getInstance())

#include <string.h>
#include <queue>
#include <algorithm>
#include <fstream>

#include "BWAPI.h"
#include <limits.h>
#include "ActionSet.hpp"
#include "StarcraftAction.hpp"
#include "StarcraftData.hpp"
#include "ActionInProgress.hpp"
#include "BuildingData.hpp"
#include "StarcraftSearchGoal.hpp"
#include "StarcraftSearchConstraint.hpp"
#include <math.h>

// #define DEBUG_LEGAL

class StarcraftState {

protected:

	/*********************************************************************
	* DATA
	**********************************************************************/

	StarcraftState *	parent;				        // the state that generated this state

	ActionsInProgress	progress;					
	BuildingData<unsigned short>	buildings;

	ActionSet 		    completedUnitSet,			// ActionSet of completed units
			            progressUnitSet;			// ActionSet bitmask of units in progress

	char		        mineralWorkers, 		    // number of workers currently mining
			            gasWorkers; 			    // number of workers currently getting gas

	unsigned char	    actionPerformed, 		    // the action which generated this state
						actionPerformedK;

	int		            maxSupply, 			        // our maximum allowed supply
			            currentSupply; 			    // our current allocated supply
			            
	char				padding[2];

	unsigned short 	    currentFrame; 			    // the current frame of the game
			            
	int	    			minerals, 			        // current mineral count
			            gas;				        // current gas count
			          
	unsigned char 	    numUnits[MAX_ACTIONS];		// how many of each unit are completed

	/*********************************************************************
	* VIRTUAL FUNCTIONS
	*********************************************************************/

	virtual ~StarcraftState() {}

	virtual bool specificIsLegal(Action a) = 0;
	virtual int  specificWhenReady(Action a) = 0;
	virtual void specificDoAction(const Action action, const int ffTime) = 0;
	virtual void specificFastForward(const int toFrame) = 0;
	virtual void specificFinishAction(const Action action) = 0;
	virtual void setGameStartData() = 0;

public: 

	/**********************************************************************************
	*
	*                 Action Performing Functions - Changes State
	*
	**********************************************************************************/
	

	// do an action, action must be legal for this not to break
	void doAction(const Action action, const int ffTime)
	{
		if (GSN_DEBUG) printf("Do Action - %s\n", DATA[action].getName().c_str());

		// set the actionPerformed
		actionPerformed = (unsigned char)action;
		actionPerformedK = 1;

		// fast forward to when this action can be performed
		if (ffTime != -1) 
		{ 
			fastForward(ffTime);
		} 
		else 
		{
			fastForward(resourcesReady(action));
		}

		// create the struct
		unsigned int tempTime = currentFrame + DATA[action].buildTime();

		// if building, add a constant amount of time to find a building placement
		if (DATA[action].isBuilding()) 
		{
			// add 2 seconds
			tempTime += BUILDING_PLACEMENT;
		}

		// add it to the actions in progress
		progress.addAction(action, tempTime);

		// update the progress bitmask
		progressUnitSet.add(action);
			
		// if what builds this unit is a building
		if (DATA[action].whatBuildsIsBuilding())
		{
			// add it to a free building, which MUST be free since it's called from doAction
			// which must be already fastForwarded to the correct time
			buildings.queueAction(action);
		}

		// modify our resources
		minerals 		-= DATA[action].mineralPrice();
		gas	 			-= DATA[action].gasPrice();
		currentSupply 	+= DATA[action].supplyRequired();
		
		if (minerals < 0 || gas < 0)
		{
			printf("Resource Error: m:%d g:%d\n", minerals, gas);
		}

		// do race specific things here, like subtract a larva
		specificDoAction(action, ffTime);

		if (GSN_DEBUG) printf("Do Action End\n");
	}

	// fast forwards the current state to time toFrame
	void fastForward(const int toFrame) 
	{
		if (GSN_DEBUG) printf("\tFast Forward to %d from %d progress size %d\n", toFrame, currentFrame, progress.size());

		// do specific things here, like figure out how many larva we will have
		specificFastForward(toFrame);
		
		// fast forward the building timers to the current frame
		buildings.fastForwardBuildings(toFrame - currentFrame);

		// update resources & finish each action
		int lastActionFinished(currentFrame), totalTime(0);
		double moreGas = 0, moreMinerals = 0;

		while (!progress.isEmpty() && progress.nextActionFinishTime() <= (unsigned int)toFrame) 
		{		
			// figure out how long since the last action was finished
			int timeElapsed 	= progress.nextActionFinishTime() - lastActionFinished;
			totalTime 			+= timeElapsed;

			// update our mineral and gas count for that period
			moreMinerals 		+= timeElapsed * getMineralsPerFrame();
			moreGas 			+= timeElapsed * getGasPerFrame();

			// update when the last action was finished
			lastActionFinished = progress.nextActionFinishTime();

			// finish the action, which updates mineral and gas rates if required
			finishNextActionInProgress();
		}

		// update resources from the last action finished to toFrame
		int elapsed 	=  toFrame - lastActionFinished;
		moreMinerals 	+= elapsed * getMineralsPerFrame();
		moreGas 		+= elapsed * getGasPerFrame();
		totalTime 		+= elapsed;

		minerals 		+= (int)(ceil(moreMinerals));
		gas 			+= (int)(ceil(moreGas));

		// we are now in the FUTURE... "the future, conan?"
		currentFrame 	= toFrame;

		if (GSN_DEBUG) printf("\tFast Forward End\n");
	}


	// when we want to 'complete' an action (its time is up) do this
	void finishNextActionInProgress() 
	{	
		if (GSN_DEBUG) printf("\t\tFinishing Action %s\n", DATA[progress.nextAction()].getName().c_str());

		// get the actionUnit from the progress data
		int actionUnit = progress.nextAction();

		// add the unit to the unit counter
		addNumUnits(actionUnit, DATA[actionUnit].numProduced());
	
		// add to the max supply if it's a supplying unit
		maxSupply += DATA[actionUnit].supplyProvided();
	
		// if it's a worker, put it on minerals
		if (DATA[actionUnit].isWorker()) 
		{ 
			mineralWorkers++;
		}

		// if it's an extractor
		if (DATA[actionUnit].isRefinery()) 
		{
			// take those workers from minerals and put them into it
			mineralWorkers -= 3; gasWorkers += 3;
		}	
		
		// if it's a building that can produce units, add it to the building data
		if (DATA[actionUnit].isBuilding() && !DATA[actionUnit].isSupplyProvider())
		{
			buildings.addBuilding(actionUnit);
		}

		// pop it from the progress vector
		progress.popNextAction();
	
		// we need to re-set the whole progress mask again :(
		setProgressMask();
		
		// do race specific action finishings
		specificFinishAction(actionUnit);

		if (GSN_DEBUG) printf("\t\tFinish Action End\n");
	}
	
	/**********************************************************************************
	*
	*                 When Resources Ready Function + Sub Functions
	*
	**********************************************************************************/

	// returns the time at which all resources to perform an action will be available
	int resourcesReady(const int action) 
	{
		if (GSN_DEBUG) printf(">\n>\n> NEW ACTION: %s\n>\n> Progress Size %d\n>\n\nResource Calculation - %s\n", DATA[action].getName().c_str(), progress.size(), DATA[action].getName().c_str());

		// the resource times we care about
		int 	m(currentFrame), 	// minerals
				g(currentFrame), 	// gas
				l(currentFrame), 	// class-specific
				s(currentFrame), 	// supply
				p(currentFrame), 	// prerequisites
				maxVal(currentFrame);
	
		// figure out when prerequisites will be ready
		p = whenPrerequisitesReady(action);

		// check minerals
		m = whenMineralsReady(action);

		// check gas
		g = whenGasReady(action);

		// race specific timings (Zerg Larva)
		l = specificWhenReady(action);

		// set when we will have enough supply for this unit
		s = whenSupplyReady(action);

		// figure out the max of all these times
		maxVal = (m > maxVal) ? m : maxVal;
		maxVal = (g > maxVal) ? g : maxVal;
		maxVal = (l > maxVal) ? l : maxVal;
		maxVal = (s > maxVal) ? s : maxVal;
		maxVal = (p > maxVal) ? p : maxVal;
	
		if (GSN_DEBUG) 
		{
			printf("\tMinerals  \t%d\n\tGas  \t\t%d\n\tSpecific  \t%d\n\tSupply  \t%d\n\tPreReq  \t%d\n", m, g, l, s, p);
			printf("Resource Calculation End (return %d)\n\n", maxVal);
		}
		
		// return the time
		return maxVal + 1;
	}
	
	int whenSupplyReady(const Action action) const
	{
		if (GSN_DEBUG) printf("\tWhen Supply Ready\n");
		int s = currentFrame;
	
		// set when we will have enough supply for this unit
		int supplyNeeded = DATA[action].supplyRequired() + currentSupply - maxSupply;
		if (supplyNeeded > 0) 
		{	
			//if (GSN_DEBUG) printf("\t\tSupply Is Needed: %d\n", supplyNeeded);
		
			// placeholder for minimum olord time
			int min = 9999999;

			// if we don't have the resources, this action would only be legal if there is an
			// overlord in progress, so check to see when the first overlord will finish
			for (int i(0); i<progress.size(); ++i) 
			{
				if (GSN_DEBUG) printf("\t\tSupply Check: %s %d Progress Size %d\n", DATA[progress.getAction(i)].getName().c_str(), DATA[progress.getAction(i)].supplyProvided(), progress.size());
			
				// so, if the unit provides the supply we need
				if (DATA[progress.getAction(i)].supplyProvided() > supplyNeeded) 
				{
					// set 'min' to the min of these times
					min = (progress.getTime(i) < min) ? progress.getTime(i) : min;
					
					//if (GSN_DEBUG) printf("\t\tSupply Found: %s %d\n", DATA[progress.getAction(i)].getName().c_str(), min);
				}

				// then set supply time to min
				s = min;
			}
		}
		
		return s;
	}
	
	int whenPrerequisitesReady(const Action action) const
	{
		if (GSN_DEBUG) printf("\tCalculating Prerequisites\n");
	
		int p = currentFrame;
	
		// if a building builds this action
		if (DATA[action].whatBuildsIsBuilding())
		{
			if (GSN_DEBUG) printf("\t\tAction Needs Building\n");
		
			// get when the building / prereqs will be ready
			p = whenBuildingPrereqReady(action);
		}
		// otherwise something else builds this action so we don't worry about buildings
		else
		{
			// if requirement in progress (and not already made), set when it will be finished
			ActionSet reqInProgress = (DATA[action].getPrerequisites() & progressUnitSet) - completedUnitSet;
			
			if (GSN_DEBUG) printf("\t\tAction Does Not Need Building\n");
			
			// if it's not empty, check when they will be done
			if (!reqInProgress.isEmpty())
			{
				if (GSN_DEBUG) printf("\t\tAction Has Prerequisites In Progress\n");
			
				p = progress.whenActionsFinished(reqInProgress);
			}
		}
		
		if (GSN_DEBUG) printf("\tCalculating Prerequisites End (return %d)\n", p);
		return p ;
	}
	
	int whenBuildingPrereqReady(const Action action) const
	{
		if (GSN_DEBUG) printf("\t\tWhen Building Prereq Ready\n");
	
		Action 				builder 		= DATA[action].whatBuildsAction();
	
		assert(DATA[builder].isBuilding());
	
		// is the building currently constructed
		bool buildingConstructed = numUnits[builder] > 0;
		
		// is the building currently in progress
		bool buildingProgress = progress.numInProgress(builder) > 0;
	
		// when the building will be available
		int buildingAvailableTime;
	
		// if the building is both finished and in progress
		if (buildingConstructed && buildingProgress)
		{
			if (GSN_DEBUG) printf("\t\t\tBuilding Constructed %d and In Progress %d\n", numUnits[builder], progress.numInProgress(builder));
		
			// get the time the constructed version will be free
			int A = whenConstructedBuildingReady(builder);
			
			// get the time the progress version will be free
			int B = progress.nextActionFinishTime(builder);
			
			// take the 
			buildingAvailableTime = (A < B) ? A : B;
		}
		// otherwise if the constructed version is all we have
		else if (buildingConstructed)
		{
			if (GSN_DEBUG) printf("\t\t\tBuilding Constructed Only\n");
		
			// set the time accordingly
			buildingAvailableTime = whenConstructedBuildingReady(builder);
		}
		// otherwise the progress version is all we have
		else
		{
			if (GSN_DEBUG) printf("\t\t\tBuilding In Progress Only\n");
		
			// set the time accordingly
			buildingAvailableTime = progress.nextActionFinishTime(builder);
		}

		// get all prerequisites currently in progress but do not have any completed
		ActionSet prereqInProgress = (DATA[action].getPrerequisites() & progressUnitSet) - completedUnitSet;
		
		// remove the specific builder from this list since we calculated that earlier
		prereqInProgress.subtract(builder);
		
		// if we actually have some prerequisites in progress other than the building
		if (!prereqInProgress.isEmpty())
		{
			// get the max time the earliest of each type will be finished in
			int C = progress.whenActionsFinished(prereqInProgress);
			
			// take the maximum of this value and when the building was available
			buildingAvailableTime = (C > buildingAvailableTime) ? C : buildingAvailableTime;
		}
	
		if (GSN_DEBUG) printf("\t\tWhen Building Prereq Ready End (return %d)\n", buildingAvailableTime);
		return buildingAvailableTime;
	}
	
	int whenConstructedBuildingReady(const Action builder) const
	{
		if (GSN_DEBUG) printf("\t\t\tWhen Constructed Building Ready\n");
		
		// if what builds a is a building and we have at least one of them completed so far
		if (DATA[builder].isBuilding() && numUnits[builder] > 0 )
		{
			int returnTime = currentFrame + buildings.timeUntilFree(builder);
		
			if (GSN_DEBUG) printf("\t\t\tWhen Constructed Building Ready End (return %d)\n", returnTime);
		
			// get when the next building is available
			return returnTime;
		}
		
		return currentFrame;
	}

	// when will minerals be ready
	int whenMineralsReady(const int action) const 
	{
		int difference = DATA[action].mineralPrice() - minerals;

		double m = currentFrame;
		double addMinerals = 0, addTime = 0;

		if (difference > 0) 
		{	
			int lastAction = currentFrame;
			int tmw = mineralWorkers, tgw = gasWorkers;
			

			for (int i(0); i<progress.size(); ++i) 
			{
				// the vector is sorted in descending order
				int ri = progress.size() - i - 1;

				// the time elapsed and the current minerals per frame
				int elapsed = progress.getTime(ri) - lastAction;
				double mpf = (tmw * DATA.mpwpf);
	
				// the amount of minerals that would be added this time step
				double tempAdd = elapsed * mpf, tempTime = elapsed;

				// if this amount is enough to push us over what we need
				if (addMinerals + tempAdd >= difference) 
				{				
					// figure out when we go over
					tempTime = (difference - addMinerals) / mpf;

					// add the minerals and time
					addMinerals += tempTime * mpf;
					addTime += tempTime;

					//if (GSN_DEBUG) printf("Necessary Minerals Acquired Mid-Iteration: %lf\n", addMinerals); 

					// break out of the loop
					break;

				// otherwise, add the whole interval
				} 
				else 
				{
					addMinerals += tempAdd;
					addTime += elapsed;
					
					//if (GSN_DEBUG) printf("Another Mineral Iteration Necessary: %lf\n", addMinerals);
				}

				// if it was a drone or extractor update the temp variables
				if (DATA[progress.getAction(ri)].isWorker()) 
				{
					tmw++;
				} 
				else if (DATA[progress.getAction(ri)].isRefinery()) 
				{
					tmw -= 3; tgw += 3;
				}

				// update the last action
				lastAction = progress.getTime(ri);
			}

			// if we still haven't added enough minerals, add more time
			if (addMinerals < difference) 
			{
				addTime += (difference - addMinerals) / (tmw * DATA.mpwpf);
				
				//if (GSN_DEBUG) printf("\t\tNot Enough Minerals, Adding: minerals(%lf) time(%lf)\n", (difference - addMinerals), addTime); 
			}

			m += addTime;
		}

		//if (GSN_DEBUG) printf("\tMinerals Needs Adding: Minerals(%d, %lf) Frames(%lf, %d > %d)\n", difference, addMinerals, addTime, currentFrame, (int)ceil(m));

		// for some reason if i don't return +1, i mine 1 less mineral in the interval
		return (int)(ceil(m) + 1);
	}

	int whenGasReady(const int action) const
	{
		double g = currentFrame;
		int difference = DATA[action].gasPrice() - gas;
		double addGas = 0, addTime = 0;
	
		if (difference > 0) 
		{
			int lastAction = currentFrame;
			int tmw = mineralWorkers, tgw = gasWorkers;
			

			for (int i(0); i<progress.size(); ++i) 
			{
				// the vector is sorted in descending order
				int ri = progress.size() - i - 1;

				// the time elapsed and the current minerals per frame
				int elapsed = progress.getTime(ri) - lastAction;
				double gpf = (tgw * DATA.gpwpf);
	
				// the amount of minerals that would be added this time step
				double tempAdd = elapsed * gpf, tempTime = elapsed;

				// if this amount is enough to push us over what we need
				if (addGas + tempAdd >= difference) 
				{
					// figure out when we go over
					tempTime = (difference - addGas) / gpf;

					// add the minerals and time
					addGas += tempTime * gpf;
					addTime += tempTime;

					// break out of the loop
					break;

				// otherwise, add the whole interval
				} else {

					addGas += tempAdd;
					addTime += elapsed;
				}

				// if it was a drone or extractor update temp variables
				if (DATA[progress.getAction(ri)].isWorker()) 
				{
					tmw++;
				} 
				else if (DATA[progress.getAction(ri)].isRefinery()) 
				{
					tmw -= 3; tgw += 3;
				}

				// update the last action
				lastAction = progress.getTime(ri);
			}

			// if we still haven't added enough minerals, add more time
			if (addGas < difference) 
			{
				addTime += (difference - addGas) / (tgw * DATA.gpwpf);
			}

			g += addTime;
		}

		//if (GSN_DEBUG) printf("\tGas Needs Adding: Gas(%d, %lf) Frames(%lf, %d > %d)\n", difference, addGas, addTime, currentFrame, (int)ceil(g));

		return (int)(ceil(g) + 1);
	}
	
	/**********************************************************************************
	*
	*                 Heuristic Search + Pruning + Evaluation Functions
	*
	**********************************************************************************/
	
	// Gets an upper bound on the time it will take to complete goal:
	// Sum the following:
	// calculateGoalResourceLowerBound()
	// calculateDependencyTimeRemaining()
	// sum the build time of everything in the goal
	int calculateUpperBoundHeuristic(const StarcraftSearchGoal & goal) const
	{
		// the upper bound
		int upperBound(0);
	
		// dependency chain build time heuristic
		upperBound += calculateDependencyHeuristic(goal);
		
		// resources ready heuristic
		upperBound += calculateResourcesReadyHeuristic(goal);
		
		// add build time for everything in the goal
		for (int a = 0; a<DATA.size(); ++a)
		{	
			// how many of this action we still need to build
			int need = goal[a] - numUnits[a];
		
			// if this action is in the goal
			if (need > 0)
			{
				upperBound += need * DATA[a].buildTime();
			}
		}
		
		return currentFrame + upperBound;
	}
	
	int workerUpperBound(const StarcraftSearchGoal & goal) const
	{
		std::pair<short,short> grlb = calculateGoalResourceLowerBound(goal);
		std::pair<int, int> resourceReadyPair = resourceReadyLowerBound(grlb.first, grlb.second);
		
		return resourceReadyPair.second;
	}

	// heuristic evaluation function for this state
	int eval(const StarcraftSearchGoal & goal, const bool useLandmark = true) const
	{
		// dependency chain build time heuristic
		int dependencyHeuristic = calculateDependencyHeuristic(goal);
		
		if (useLandmark)
		{
			return dependencyHeuristic;
		}
		else
		{
			return 1;
		}
	}	
	
	int calculateResourcesReadyHeuristic(const StarcraftSearchGoal & goal) const
	{
		// resources ready heuristic
		std::pair<short,short> grlb = calculateGoalResourceLowerBound(goal);
		std::pair<int, int> resourceReadyPair = resourceReadyLowerBound(grlb.first, grlb.second);
		int resourceReadyLowerBoundVal = resourceReadyPair.first - currentFrame;
		
		return resourceReadyLowerBoundVal;
	}
	
	int calculateDependencyHeuristic(const StarcraftSearchGoal & goal) const
	{
		// the maximum dependency time for all actions in the goal
		int max = 0;
	
		// for each action which exists
		for (int a = 0; a<DATA.size(); ++a)
		{	
			// if this action is in the goal
			if (goal[a] > numUnits[a])
			{
				// calculate the time remaining for this action's dependencies
				int actionDTR = calculateDependencyTimeRemaining(a);
				
				// set the maximum value
				max = (actionDTR > max) ? actionDTR : max;
			}
		}
	
		// return the maximum value
		return max;
	}
	
	// calculates a lower bound on the amount of time it would take us to
	// reach the resources necessary to reach our goal
	std::pair<short,short> calculateGoalResourceLowerBound(const StarcraftSearchGoal & goal) const
	{
		std::pair<short,short> totalResources(0,0);
		
		// first calculate the resources needed directly by goal actions
		for (int a(0); a < MAX_ACTIONS; ++a)
		{
			// if this action is in the goal
			if (goal.get(a))
			{
				// calculate how many we still need of this action
				int stillNeed(goal[a] - (numUnits[a] + progress[a]));
				
				// if it is greater than zero, add it to the total
				if (stillNeed > 0)
				{
					//printf("Adding price of %d %s (%d,%d)\n", stillNeed, DATA[a].getName().c_str(), DATA[a].mineralPrice(), DATA[a].gasPrice());
				
					totalResources.first  += stillNeed * DATA[a].mineralPrice();
					totalResources.second += stillNeed * DATA[a].gasPrice();
				}
			}
		}
		
		// calculate how many resources are still needed by dependencies remaining
		// for all actions in the goal
		
		// an actionset of what we've added so far
		// this will prevent shared dependencies from adding resources twice
		ActionSet addedSoFar(0);
		
		// for each action in the goal
		for (int a(0); a < MAX_ACTIONS; ++a)
		{
			// if we need some of this action 
			// and we don't have any already (since this would always return time zero
			if (goal[a])
			{
				//printf("Calculating Dependency Resources For %s\n", DATA[a].getName().c_str());
			
				// calculate how many resources we would need for these dependencies
				calculateDependencyResourcesRemaining(a, totalResources, addedSoFar);
			}
		}
		
		// if the total resources requires gas and we do not have a refinery, add the price of one
		if (totalResources.second > 0 && !numUnits[DATA.getRefinery()] && !progress[DATA.getRefinery()])
		{
			//printf("Goal requires gas, adding refinery price\n");
			totalResources.first  += DATA[DATA.getRefinery()].mineralPrice();
			totalResources.second += DATA[DATA.getRefinery()].gasPrice();
		}
		
		// return the total
		return totalResources;
	}
	
	void calculateDependencyResourcesRemaining(const int action, std::pair<short,short> & totalResources, ActionSet & addedSoFar) const
	{
		// get the strict dependencies of this action
		ActionSet strictDependencies = DATA.getStrictDependency(action);
		
		// for each of the top level dependencies this action has
		while (!strictDependencies.isEmpty())
		{
			// get the next action from this dependency
			int nextStrictDependency = strictDependencies.popAction();
	
			// if we have already started, completed, or added this action already, continue
			if (completedUnitSet.contains(nextStrictDependency) || progressUnitSet.contains(nextStrictDependency))
			{
				continue;
			}
			// otherwise we haven't seen it yet
			else

			{
				// if we haven't added it yet, add it
				if (!addedSoFar.contains(nextStrictDependency))
				{
					//printf("\tAdding resources for dependency %s, (%d,%d)\n", DATA[nextStrictDependency].getName().c_str(), DATA[nextStrictDependency].mineralPrice(), DATA[nextStrictDependency].gasPrice());
			
					// add the resources
					totalResources.first  += DATA[nextStrictDependency].mineralPrice();
					totalResources.second += DATA[nextStrictDependency].gasPrice();
				
					// mark the action as added
					addedSoFar.add(nextStrictDependency);
				}
				else
				{
					//printf("\tPreviously added price of %s, skipping\n",DATA[nextStrictDependency].getName().c_str()); 
				}
				
				// recurse down the line
				calculateDependencyResourcesRemaining(nextStrictDependency, totalResources, addedSoFar);
			}
		}
	}
	
	// calculates the time remaining in the chain of dependencies of action given 'have' already completed
	int calculateDependencyTimeRemaining(const int action) const
	{
		// get the strict dependencies of this action
		ActionSet strictDependencies = DATA.getStrictDependency(action);
		
		//bool actionCompleted = completedUnitSet.contains(action);
		bool actionInProgress = progressUnitSet.contains(action);
		
		// this will hold the max return time of all dependencies
		int max = 0;
		
		// if one of the unit is in progress, get the remaining time on it
		if (actionInProgress)
		{
			max += progress.nextActionFinishTime(action) - currentFrame;
		}
		// otherwise get the build time of the unit
		else
		{
			max += DATA[action].buildTime();
		}
		
		// for each of the top level dependencies this action has
		while (!strictDependencies.isEmpty())
		{
			// get the next action from this dependency
			int nextStrictDependency = strictDependencies.popAction();
	
			// if we have this dependency completed, break out of the loop
			if (completedUnitSet.contains(nextStrictDependency))
			{
				//if (1) printf("DEPCALC:  Have  %s, stop\n", DATA[nextStrictDependency].getName().c_str());
				continue;
			}
			// if we have this dependency in progress
			else if (progressUnitSet.contains(nextStrictDependency))
			{
				// return the time left on this action
				int timeRemaining = progress.nextActionFinishTime(nextStrictDependency) - currentFrame;
				//if (1) printf("DEPCALC:  Prog  %s, %d\n", DATA[nextStrictDependency].getName().c_str(), timeRemaining);
				
				max = (timeRemaining > max) ? timeRemaining : max;
			}
			// we do not have this dependency at all
			else
			{
				//if (1) printf("DEPCALC:  Need  %s\n", DATA[nextStrictDependency].getName().c_str());
			
				// sum the current build time as well as that of the next strict dependency
				int sum = DATA[nextStrictDependency].buildTime() + calculateDependencyTimeRemaining(nextStrictDependency);
		
				// set the maximum
				max = (sum > max) ? sum : max;
			}
		}
		
		// return the maxium over all strict dependencies recursively
		// if this action had no dependencies max will be zero
		
		if (max == 0)
		{
			printf("WTF\n");
		}
		
		return max;
	}
	
	std::pair<int, int> resourceReadyLowerBound(const int goalMinerals, const int goalGas) const
	{
	
		double currentMinerals = minerals;
		double currentGas = gas;
		double currentWorkers = mineralWorkers;
		double mineralTimeElapsed = 0;
		double remainingMinerals, mineralTimeRemaining;
	
		while (true)
		{
			remainingMinerals = goalMinerals - currentMinerals;
			mineralTimeRemaining = remainingMinerals / (currentWorkers * DATA.mpwpf);
		
			//printf("%lf remaining, %lf frames, %lf\n", remainingMinerals, mineralTimeRemaining, (mineralTimeRemaining - DATA[DATA.getWorker()].buildTime()) * DATA.mpwpf);
		
			// if we were to build another worker, would it make its resources back?
			if ( (mineralTimeRemaining - DATA[DATA.getWorker()].buildTime()) * DATA.mpwpf >= DATA[DATA.getWorker()].mineralPrice() )
			{		
				currentMinerals += currentWorkers * DATA.mpwpf * DATA[DATA.getWorker()].buildTime() - DATA[DATA.getWorker()].mineralPrice();
				currentWorkers++;
				mineralTimeElapsed += DATA[DATA.getWorker()].buildTime();
			}
			else
			{
				break;
			}
		}
	
		remainingMinerals = goalMinerals - currentMinerals;
		if (remainingMinerals >= 0)
		{
			mineralTimeRemaining = remainingMinerals / (currentWorkers * DATA.mpwpf);
			mineralTimeElapsed += mineralTimeRemaining;
		}
		
		//if (goalMinerals) printf("%d minerals gathered in %lf frames with %d workers.\n", goalMinerals, mineralTimeElapsed, (int)currentWorkers);
		
		double gasRemaining = goalGas - currentGas;
		double gasTimeRemaining = gasRemaining / (3 * DATA.gpwpf);
		if (goalGas && !numUnits[DATA.getRefinery()] && !progress[DATA.getRefinery()])
		{
			gasTimeRemaining += DATA[DATA.getRefinery()].buildTime();
		}
		
		//if (goalGas) printf("%d gas gathered in %lf frames with %d workers.\n", goalGas, gasTimeRemaining, 3);
		
		int workers = (int)(currentWorkers + (goalGas ? 3 : 0));
		
		return std::pair<int, int>((int)ceil(currentFrame + (gasTimeRemaining > mineralTimeElapsed ? gasTimeRemaining : mineralTimeElapsed)), workers);
	}
	
	int hash(const std::vector<int> & random) const
	{
		int hash = 0;
		int * p = (int *)(this);
	
		for (size_t i=1; i<sizeof(*this)/4; ++i)
		{
			int randData = (*(p+i)) ^ random[i];
			hash ^= randData;
		}

		return hash;
	}
	
	/**********************************************************************************
	*
	*                            Action Legality Functions
	*
	**********************************************************************************/

	ActionSet getLegalActions(const StarcraftSearchGoal & goal) const
	{
		// initially empty bitmask
		ActionSet legal;

		// for each possible action, check for legality
		for (int i = 0; i < DATA.size(); ++i) 
		{
			// if we have the prerequisite units
			if (isLegal(i, goal)) 
			{
				// set the bitmask bit to a 1
				legal.add(i);
			}
		}

		// return the bitmask
		return legal;
	}

	// given the goal, is this action legal
	bool isLegal(const Action a, const StarcraftSearchGoal & goal) const
	{
		if (currentSupply > maxSupply)
		{
			printData();
		}
	
		if (!goal.get(a) && !goal.getMax(a))
		{
			#ifdef DEBUG_LEGAL
				printf("NONE IN GOAL %d\n", a);
			#endif
			return false;
		}
	
		// if we have enough of this thing don't make any more
		if (goal.get(a) && (getNumUnits(a) >= goal.get(a)))
		{
			#ifdef DEBUG_LEGAL
				printf("HAVE ALREADY %d\n", a);
			#endif
			return false;
		}
		
		// if we have more than the max we want, return false
		if (goal.getMax(a) && (getNumUnits(a) >= goal.getMax(a)))
		{
			#ifdef DEBUG_LEGAL
				printf("HAVE MAX ALREADY %d\n", a);
			#endif
			return false;
		}
	
		// check if the tech requirements are met
		if (!hasRequirements(DATA[a].getPrerequisites())) 
		{
			#ifdef DEBUG_LEGAL
				printf("PREREQ %d\n", a);
			#endif
			return false;
		}

		// if it's a unit and we are out of supply and aren't making an overlord, it's not legal
		int supplyInProgress = progress[DATA.getSupplyProvider()]*DATA[DATA.getSupplyProvider()].supplyProvided() +
							   progress[DATA.getResourceDepot()]*DATA[DATA.getResourceDepot()].supplyProvided();
		if ( (currentSupply + DATA[a].supplyRequired()) > maxSupply + supplyInProgress) 
		{
			#ifdef DEBUG_LEGAL
				printf("NO SUPPLY %d\n", a);
			#endif
			//printData();
			return false;
		}

		// specific rule for never leaving 0 workers on minerals
		if (DATA[a].isRefinery() && (mineralWorkers <= 4 + 3*getNumUnits(DATA.getRefinery()))) 
		{		
			#ifdef DEBUG_LEGAL
				printf("REFINERY 1 %d\n", a);
			#endif
			return false;
		}

		// if it's a new building and no drones are available, it's not legal
		if (DATA[a].isBuilding() && (mineralWorkers <= 1)) 
		{
			#ifdef DEBUG_LEGAL
				printf("NO MINERAL WORKERS %d\n", a);
			#endif
			return false;
		}
		
		// we can't build a building with our last worker
		if (DATA[a].isBuilding() && (mineralWorkers <= 1 + 3*getNumUnits(DATA.getRefinery())))
		{
			#ifdef DEBUG_LEGAL
				printf("NO MINERAL WORKERS - (%d, %d) %d\n", mineralWorkers, getNumUnits(DATA.getRefinery()), a);
			#endif
			return false;
		}

		// if we have no gas income we can't make a gas unit
		bool noGas = (gasWorkers == 0) && (getNumUnits(DATA.getRefinery()) == 0);
		if (((DATA[a].gasPrice() - gas) > 0) && noGas) 
		{ 
			#ifdef DEBUG_LEGAL
				printf("NO GAS %d\n", a);
			#endif
			return false; 
		}

		// if we have no mineral income we'll never have a minerla unit
		bool noMoney = (mineralWorkers == 0) && (progress.numInProgress(DATA.getWorker()) == 0);
		if (((DATA[a].mineralPrice() - minerals) > 0) && noMoney) 
		{ 
			#ifdef DEBUG_LEGAL
				printf("NO MONEY %d\n", a);
			#endif
			return false; 
		}

		// don't build more refineries than resource depots
		if (DATA[a].isRefinery() && (getNumUnits(DATA.getRefinery()) >= getNumUnits(DATA.getResourceDepot())))
		{
			#ifdef DEBUG_LEGAL
				printf("NOT ENOUGH DEPOTS FOR REFINERY %d", a);
			#endif
			return false;
		}

		// we don't need to go over the maximum supply limit with supply providers
		if (DATA[a].isSupplyProvider() && (maxSupply + getSupplyInProgress() >= 400))
		{
			#ifdef DEBUG_LEGAL
				printf("TOO MUCH SUPPLY FOR PROVIDER %d\n", a);
			#endif
			return false;
		}

		#ifdef DEBUG_LEGAL
			printf("Action Legal %d\n", a);
		#endif
		return true;
	}
	

	// does the current state meet the goal requirements
	bool meetsGoal(const StarcraftSearchGoal & goal) const
	{ 	
		// for each unit in the array
		for (int i=0; i<MAX_ACTIONS; ++i) 
		{
			// if we don't have enough of them, no good
			if (getNumUnits(i) < goal.get(i)) 
			{
				return false;
			}
		}

		// otherwise it's fine
		return true;
	}

	bool meetsGoalCompleted(const StarcraftSearchGoal & goal) const
	{ 
		if (minerals < goal.getMineralGoal() || gas < goal.getGasGoal())
		{
			return false;
		}
	
		// for each unit in the array
		for (int i=0; i<MAX_ACTIONS; ++i) 
		{
			// if we don't have enough of them, no good
			if (numUnits[i] < goal.get(i)) return false;
		}

		// otherwise it's fine
		return true;
	}
	
	bool meetsConstraints(const StarcraftSearchConstraints & ssc) const
	{
		for (int i=0; i<ssc.size(); ++i)
		{
			StarcraftSearchConstraint & c = ssc.getConstraint(i);
		
			if (currentFrame > c.frame && numUnits[c.action] < c.actionCount)
			{
				return false;
			}
		}
		
		return true;
	}
	
	/**********************************************************************************
	*
	*                                Getter + Setter Functions
	*
	**********************************************************************************/

	int getCurrentFrame() const
	{
		return currentFrame;
	}
	
	ActionSet getCompletedUnitSet() const
	{
		return completedUnitSet;
	}
	
	ActionSet getProgressUnitSet() const
	{
		return progressUnitSet;
	}
	
	Action getActionPerformed() const
	{
		return actionPerformed;
	}
	
	void setParent(StarcraftState * p)
	{
		parent = p;
	}
	
	StarcraftState * getParent() const
	{
		return parent;
	}
	
	ActionSet getUnitsCompleteAndProgress() const
	{
		return completedUnitSet + progressUnitSet;
	}
	
	int getLastFinishTime() const
	{
		return progress.getLastFinishTime();
	}
	
	int getWorkerCount() const
	{
		return getNumUnits(DATA.getWorker());
	}
	
	int getNumResourceDepots() const
	{
		return getNumUnits(DATA.getResourceDepot());
	}

	// sets a bitmask of units we have based on the unit count array
	void setUnitMask() 
	{
		// reset the bits
		completedUnitSet = 0;

		// for each possible action
		for (int i=0; i<DATA.size(); ++i) 
		{
			// if we have more than 0 of the unit, set the bit to 1
			if (numUnits[i] > 0) completedUnitSet.add(i);
		}
	}

	// sets the progress mask based on the progress vector
	inline void setProgressMask() 
	{
		// reset the bits
		progressUnitSet = ActionSet(__ZERO);

		// for each unit in the inProgress vector
		for (int i(0); i<progress.size(); ++i) 
		{
			// set the bit of the progress mask appropriately
			progressUnitSet.add(progress.getAction(i));
		}
	}

	void addBuilding(const int action, const int timeUntilFree)
	{
		buildings.addBuilding(action, timeUntilFree);
	}

	void addActionInProgress(const int action, const int completionFrame)
	{
		// add it to the actions in progress
		progress.addAction(action, completionFrame);

		// update the progress bitmask
		progressUnitSet.add(action);
	}

	// add a number of a unit type
	void addNumUnits(const int t, const char d) 	
	{
		// set the unit amount appropriately
		numUnits[t] += d;

		// if the new number is more than zero, set the bit in the unitMask
		if (numUnits[t] > 0) 	{ completedUnitSet.add(t); }

		// otherwise, clear the bit in the unitMask
		else					{ completedUnitSet.subtract(t); }
	}

	// set the number of a particular unit type
	void setNumUnits(const int t, const unsigned char n) 
	{ 
		// set the unit amount appropriately
		numUnits[t] = n; 

		// if the new number is more than zero, set the bit in the unitMask
		if (numUnits[t] > 0) 	{ completedUnitSet.add(t); }

		// otherwise, clear the bit in the unitMask
		else				{ completedUnitSet.subtract(t); }
	}

	void setFrame(const int frame)
	{
		currentFrame = frame;
	}

	void setSupply(const int cSupply, const int mSupply)
	{
		currentSupply = cSupply;
		maxSupply = mSupply;
	}

	void setResources(const int m, const int g)
	{
		minerals = m;
		gas = g;
	}
	
	int getNumUnits(const int t) const
	{
		return numUnits[t] + (progress[t] * DATA[t].numProduced());
	}
	
	void setActionPerformedK(const unsigned char k)
	{
		actionPerformedK = k;
	}
	
	int getActionPerformedK() const
	{
		return actionPerformedK;
	}

	int getSupplyInProgress() const
	{
		int supplyInProgress =	progress[DATA.getSupplyProvider()]*DATA[DATA.getSupplyProvider()].supplyProvided() +
								progress[DATA.getResourceDepot()]*DATA[DATA.getResourceDepot()].supplyProvided();

		return supplyInProgress;
	}
	
	bool hasEnoughSupplyForGoal(const StarcraftSearchGoal & goal) const
	{
		int goalSupplyNeeded = 0;
		
		for (int a=0; a<DATA.size(); ++a)
		{
			int unitsNeeded = (getNumUnits(a)) - goal[a];
			if (unitsNeeded > 0)
			{
				goalSupplyNeeded += DATA[a].supplyRequired();
			}
		}
		
		return (maxSupply - currentSupply) >= goalSupplyNeeded;
	}
	
	bool meetsActionGoal(const StarcraftSearchGoal & goal, const Action a) const
	{
		return getNumUnits(a) >= goal[a];
	}

	// determines if an action has prerequisites met via bitmask operations
	bool hasRequirements(const ActionSet required) const 
	{
		return (completedUnitSet + progressUnitSet).contains(required); 
	}

	// setters
	void setMineralWorkers	(const unsigned char mw)	{ mineralWorkers	= mw; }
	void setGasWorkers		(const unsigned char gw)	{ gasWorkers		= gw; }
	void addMineralWorkers	(const int d)				{ mineralWorkers	= mineralWorkers+d; }
	void addGasWorkers		(const int d)				{ gasWorkers		= gasWorkers+d;}

	// getter methods for the internal variables
	double 			getMineralsPerFrame()		const	{ return DATA.mpwpf * mineralWorkers; }
	double 			getGasPerFrame()	        const   { return DATA.gpwpf * gasWorkers; } 
	int 			getMaxSupply()				const	{ return maxSupply; }						
	int 			getCurrentSupply()			const	{ return currentSupply; }						
	unsigned char 	getMineralWorkers()	        const   { return mineralWorkers; }					
	unsigned char 	getGasWorkers()		        const   { return gasWorkers; }						
	int 			getMinerals() 				const	{ return minerals; }	
	int				getGas()					const	{ return gas; }	
	
	int getMinerals(const int frame) const
	{
		//assert(frame >= currentFrame);
	
		return minerals + (int)(getMineralsPerFrame() * (frame-currentFrame));
	}
	
	int getGas(const int frame) const
	{
		//assert(frame >= currentFrame);
	
		return gas + (int)(getGasPerFrame() * (frame-currentFrame));
	}
	
	int getFinishTimeMinerals() const
	{
		return getMinerals(getLastFinishTime());
	}
	
	int getFinishTimeGas() const
	{
		return getGas(getLastFinishTime());
	}

	void printData() const
	{
		printf("\n-----------------------------------------------------------\n");
		printf("TEST %d\n", minerals);

		printf("Current Frame: %d (%dm %ds)\n\n", currentFrame, (currentFrame / 24) / 60, (currentFrame / 24) % 60);

		printf("Completed:\n\n");
		for (int i=0; i<MAX_ACTIONS; i++) if (numUnits[i] > 0) {
			printf("\t%d\t%s\n", numUnits[i], DATA[i].getName().c_str());
		}

		printf("\nIn Progress:\n\n");
		for (int i(0); i<progress.size(); i++) {
			printf("\t%d\t%s\n", progress.getTime(i), DATA[progress.getAction(i)].getName().c_str());
		}

		//printf("\nLegal Actions:\n\n");
		//LOOP_BITS (getLegalActions(), action) {
		//
		//	printf("\t%s\n", data->action_strings[action].c_str());
		//}

		printf("\n%6d Minerals\n%6d Gas\n%6d Max Supply\n%6d Current Supply\n", minerals, gas, maxSupply, currentSupply);
		printf("\n%6d Mineral Drones\n%6d Gas Drones\n", mineralWorkers, gasWorkers);
		printf("\n-----------------------------------------------------------\n");
		//printPath();
	}
	


	/*void printDataToFile(std::string fileName) 
	{
		FILE * fout = fopen(fileName.c_s1tr(), "w");

		fprintf(fout, "\n-----------------------------------------------------------\n");
		fprintf(fout, "TEST %d\n", minerals);

		fprintf(fout, "Current Frame: %d (%dm %ds)\n\n", currentFrame, (currentFrame / 24) / 60, (currentFrame / 24) % 60);

		fprintf(fout, "Completed:\n\n");
		for (int i=0; i<MAX_ACTIONS; i++) if (numUnits[i] > 0) {
			fprintf(fout, "\t%d\t%s\n", numUnits[i], DATA[i].getName().c_str());
		}

		fprintf(fout, "\nIn Progress:\n\n");
		for (int i(0); i<progress.size(); i++) {
			fprintf(fout, "\t%d\t%s\n", progress.getTime(i), DATA[progress.getAction(i)].getName().c_str());
		}

		fprintf(fout, "\n%6d Minerals\n%6d Gas\n%6d Max Supply\n%6d Current Supply\n", minerals, gas, maxSupply, currentSupply);
		fprintf(fout, "\n%6d Mineral Drones\n%6d Gas Drones\n", mineralWorkers, gasWorkers);
		fprintf(fout, "\n-----------------------------------------------------------\n");
		//printPath();
	}*/

};



#endif
