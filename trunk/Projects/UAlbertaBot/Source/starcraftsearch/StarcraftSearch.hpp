#ifndef STARCRAFT_SEARCH_H
#define STARCRAFT_SEARCH_H

#include "BWAPI.h"
#include <boost/foreach.hpp>
#include "assert.h"
#include <stdio.h>

#include "StarcraftSearchGoal.hpp"
#include "ProtossState.hpp"
#include "TerranState.hpp"
#include "SearchResults.hpp"
#include "SearchParameters.hpp"
#include "Timer.hpp"

class StarcraftSearch
{

protected:

	bool					goalSet;				// bool flag to let us know if we have set a goal
	Timer 					searchTimer;			// a timer we can use to time the search
	ActionSet 				relevantActions;		// the relevant actions to this particular search	
	ActionSet 				maxOneActions;			// the actions we only need one of
	
	virtual ~StarcraftSearch() {}
	
	// stores all recursive prerequisites for an action in the input ActionSet
	void calculateRecursivePrerequisites(Action action, ActionSet & all) {

		// get the set of prerequisites for this action
		ActionSet pre = DATA[action].getPrerequisites();
	
		// remove things we already have in all
		pre.subtract(all);
	
		// if it's empty, stop the recursion
		if (pre.isEmpty()) 
		{
			return;
		}
	
		// add prerequisites to all
		all.add(pre);

		// while prerequisites exist
		while (!pre.isEmpty())
		{		
			// add p's prerequisites
			calculateRecursivePrerequisites(pre.popAction(), all);
		}
	}
	
	std::vector<Action> getBuildOrder(StarcraftState & state)
	{
		std::vector<Action> buildOrder;
		
		StarcraftState * s = &state;
		while (s->getParent() != NULL)
		{
			for (int i=0; i<s->getActionPerformedK(); ++i)
			{
				buildOrder.push_back(s->getActionPerformed());
			}
			s = s->getParent();
		}
		
		return buildOrder;
	}
		
};


#endif
