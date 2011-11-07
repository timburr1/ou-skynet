#include <Common.h>
#include <iostream>
#include <cmath>
#include <list>
#include <vector>
#include <BWTA.h>
#include "UltraliskManager.h"
#include "MicroUtil.h"

UltraliskManager::UltraliskManager() { }


void UltraliskManager::executeMicro(const UnitVector & targets) 
{
	const UnitVector & ultras = getUnits();

	int range = BWAPI::UnitTypes::Zerg_Ultralisk.groundWeapon().maxRange();

	// Attack the target unit

	BWAPI::Position center = calcCenter();
	BWAPI::Unit * target = NULL;
	double minDist = 100000;

	UnitVector ultraTargets;
	for (size_t i(0); i<targets.size(); i++) 
	{
		
		if (!targets[i]->getType().isFlyer()) 
		{
			ultraTargets.push_back(targets[i]);

			if (!target || targets[i]->getDistance(center) < minDist) 
			{
				minDist = targets[i]->getDistance(center);
				target = targets[i];
			}
		}
	}

	foreach (BWAPI::Unit * ultra, ultras) 
		ultra->attackUnit(target);


}

