#ifndef STARCRAFT_SEARCH_GOAL_H
#define STARCRAFT_SEARCH_GOAL_H

#include <string.h>
#include <queue>
#include <algorithm>

#include "BWAPI.h"
#include "ActionSet.hpp"
#include "StarcraftAction.hpp"
#include "StarcraftData.hpp"
#include <math.h>

class StarcraftSearchGoal
{

	unsigned char 	goalUnits[MAX_ACTIONS];
	unsigned char	goalUnitsMax[MAX_ACTIONS];
	
	int				supplyRequiredVal,
					mineralGoal,
					gasGoal;
	
	void calculateSupplyRequired()
	{
		supplyRequiredVal = 0;
		for (int i=0; i<DATA.size(); ++i)
		{
			supplyRequiredVal += goalUnits[i] * DATA[i].supplyRequired();
		}
	}
	
public:	
	
	StarcraftSearchGoal() : supplyRequiredVal(0), mineralGoal(0), gasGoal(0)
	{
		for (int i=0; i<MAX_ACTIONS; ++i)
		{
			goalUnits[i] = 0;
			goalUnitsMax[i] = 0;
		}
	}
	
	StarcraftSearchGoal(const Action a) : supplyRequiredVal(0), mineralGoal(0), gasGoal(0)
	{
		for (int i=0; i<MAX_ACTIONS; ++i)
		{
			goalUnits[i] = 0;
			goalUnitsMax[i] = 0;
		}
		
		goalUnits[a] = 1;
	}
	
	~StarcraftSearchGoal() {}
	
	bool operator == (const StarcraftSearchGoal & g)
	{
		for (int i=0; i<MAX_ACTIONS; ++i)
		{
			if ((goalUnits[i] != g[i]) || (goalUnitsMax[i] != g.getMax(i)))
			{
				return false;
			}
		}

		return true;
	}

	int operator [] (const int a) const
	{
		return goalUnits[a];
	}
	
	void setMineralGoal(const int m)
	{
		mineralGoal = m;
	}
	
	void setGasGoal(const int g)
	{
		gasGoal = g;
	}
	
	int getMineralGoal() const
	{
		return mineralGoal;
	}
	
	int getGasGoal() const
	{
		return gasGoal;
	}

	void setGoal(const Action a, const unsigned char num)
	{
		assert(a >= 0 && a < DATA.size());
	
		goalUnits[a] = num;
		
		calculateSupplyRequired();
	}
	
	bool hasGoal() const
	{
		for (int i=0; i<MAX_ACTIONS; ++i)
		{
			if (goalUnits[i] > 0)
			{
				return true;
			}
		}
		
		return false;
	}

	void addToGoal(Action a, unsigned char num)
	{
		assert(a >= 0 && a < DATA.size());
	
		goalUnits[a] += num;
		
		calculateSupplyRequired();
	}
	
	void setGoalMax(const Action a, const unsigned char num)
	{
		goalUnitsMax[a] = num;
	}
	
	unsigned char get(const Action a) const
	{
		return goalUnits[a];
	}
	
	unsigned char getMax(const Action a) const
	{
		return goalUnitsMax[a];
	}
	
	int supplyRequired() const
	{
		return supplyRequiredVal;
	}
	
	void printGoal() const
	{
		printf("\nSearch Goal Information\n\n");
	
		for (int i=0; i<DATA.size(); ++i)
		{
			if (goalUnits[i])
			{
				printf("        REQ %7d %s\n", goalUnits[i], DATA[i].getName().c_str());
			}
		}
		
		for (int i=0; i<DATA.size(); ++i)
		{
			if (goalUnitsMax[i])
			{
				printf("        MAX %7d %s\n", goalUnitsMax[i], DATA[i].getName().c_str());
			}
		}
	}
};

#endif
