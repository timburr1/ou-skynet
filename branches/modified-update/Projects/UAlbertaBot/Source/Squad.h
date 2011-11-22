#pragma once

#include "Common.h"
#include "micromanagement/ZealotManager.h"
#include "micromanagement/DarkTemplarManager.h"
#include "micromanagement/DragoonManager.h"
#include "micromanagement/ObserverManager.h"
#include "SquadOrder.h"
#include "DistanceMap.hpp"
#include "StrategyManager.h"
#include "HardCodedInfo.h"

class ZealotManager;
class DarkTemplarManager;
class DragoonManager;
class ObserverManager;

class Squad
{
	UnitVector			units;

	void				updateUnits();
	DistanceMap			distanceMap;

	std::string			regroupStatus;

	std::map<BWAPI::Unit *, bool> nearEnemy;

	bool					squadObserverNear(BWAPI::Position p);

public:

	int					lastRegroup;
	SquadOrder			order;

	ZealotManager		zealotManager;
	DarkTemplarManager	darkTemplarManager;
	DragoonManager		dragoonManager;
	ObserverManager		observerManager;

						Squad(const UnitVector & units, SquadOrder order);
						Squad() {}
						~Squad() {}

	BWAPI::Position calcCenter();
	BWAPI::Position calcRegroupPosition();

	void				clear();
	void				update();
	
	void				setUnits(const UnitVector & u)	{ units = u; }
	UnitVector &		getUnits()						{ return units;} 
	SquadOrder &		getSquadOrder()					{ return order;}
	bool				unitNearEnemy(BWAPI::Unit * unit);
	bool				needsToRegroup();
	BWAPI::Unit *		getRegroupUnit();
	int					squadUnitsNear(BWAPI::Position p);

	BWAPI::Unit *		unitClosestToEnemy();

	void				setSquadOrder(SquadOrder & so);
};
