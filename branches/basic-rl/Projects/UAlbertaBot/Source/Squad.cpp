#include "Common.h"
#include "Squad.h"

Squad::Squad(const UnitVector & units, SquadOrder order) 
: units(units), lastRegroup(0), order(order)
{

}

void Squad::update()
{
	updateUnits();

	BWAPI::Position regroup = needsToRegroup() ? calcRegroupPosition() : BWAPI::Position(0,0);
	
	if (order.type == SquadOrder::Attack)
	{
		if (DRAW_UALBERTABOT_DEBUG) if (DRAW_UALBERTABOT_DEBUG) BWAPI::Broodwar->drawTextScreen(200, 330, "%s", regroupStatus.c_str());
	}

	zealotManager.execute(order, regroup);
	dragoonManager.execute(order, regroup);
	darkTemplarManager.execute(order, regroup);

	// observer manager is a little special
	observerManager.execute(order, regroup);
	observerManager.setUnitClosestToEnemy(unitClosestToEnemy());
}

void Squad::clear()
{
	units.clear();
}

// calculates whether or not to regroup
bool Squad::needsToRegroup()
{
	// if we are not attacking, never regroup
	if (units.empty() || (order.type != SquadOrder::Attack))
	{
		regroupStatus = std::string("\x04 No combat units available");
		return false;
	}	

	// a radius we care about
	int radius = 300;

	
	// the regrouping position, if we are to regroup
	BWAPI::Position regroupPosition = calcRegroupPosition();

	// number of our units in 'combat', ie radius of an opponent attack unit
	int numUnitsInCombat(0);
	BOOST_FOREACH(BWAPI::Unit * unit, units)
	{
		if (nearEnemy[unit])
		{
			numUnitsInCombat++;
		}
	}

	// number of our units in radius of regroup position
	int numUnitsInRadius(0);
	BOOST_FOREACH(BWAPI::Unit * otherUnit, units)
	{
		if (!nearEnemy[otherUnit] && regroupPosition.getDistance(otherUnit->getPosition()) < radius)
		{
			numUnitsInRadius++;
		}
	}

	// the visible enemy force size
	int visibleEnemyForceSize = UnitInfoState::getInstance()->visibleForceSize(BWAPI::Broodwar->enemy());

	if (DRAW_UALBERTABOT_DEBUG) BWAPI::Broodwar->drawCircleMap(regroupPosition.x(), regroupPosition.y(), 300, BWAPI::Colors::Orange);

	// if we have a ton of units, send them in to attack since it's too hard to be sure who will win
	if ((numUnitsInRadius >= 16) || (numUnitsInCombat >= 16) || units.size() > 30)
	{
		regroupStatus = std::string("\x08 We have a lot of units");
		return false;
	}

	BWAPI::Unit * closestToEnemy = unitClosestToEnemy();
	if (closestToEnemy)
	{
		bool nearbyHasCloaked = UnitInfoState::getInstance()->nearbyForceHasCloaked(closestToEnemy->getPosition(), BWAPI::Broodwar->enemy(), 400);
		bool hasObserver = squadObserverNear(closestToEnemy->getPosition());

		if (visibleEnemyForceSize == 0 && nearbyHasCloaked && !hasObserver)
		{
			regroupStatus = std::string("\x0E No observer");
			return true;
		}
	}

	// if we can see enemy units
	if (visibleEnemyForceSize > 0)
	{
		// if we see they have less than us in 'combat', attack them
		if (visibleEnemyForceSize <= numUnitsInCombat)
		{
			regroupStatus = std::string("\x08 We Beat Visible Enemies");
			return false;
		}
		// otherwise they have more than us 'in combat' so retreat
		else 
		{
			regroupStatus = std::string("\x0E Visible Enemies Beat Us");
			return true;
		}
	}
	// we cannot see enemy units, so remember where they were last seen and see if we can press in
	else
	{
		// our unit closest to the enemy base
		BWAPI::Unit * closestToEnemy = unitClosestToEnemy();

		// if we have a unit near the enemy base
		if (closestToEnemy)
		{
			// calculate how many oppnent units are near it, and our units are near it
			int myUnitsNear =  squadUnitsNear(closestToEnemy->getPosition());
			int oppUnitsNear = UnitInfoState::getInstance()->nearbyForceSize(closestToEnemy->getPosition(), BWAPI::Broodwar->enemy(), 500);

			if (DRAW_UALBERTABOT_DEBUG) BWAPI::Broodwar->drawTextScreen(200, 150, "Unit Debg: %d, %d", myUnitsNear, oppUnitsNear);
			if (DRAW_UALBERTABOT_DEBUG) BWAPI::Broodwar->drawCircleMap(closestToEnemy->getPosition().x(), closestToEnemy->getPosition().y(), 15, BWAPI::Colors::Green);

			int moreThanEnemy = BWAPI::Broodwar->getFrameCount() > 15000 ? 8 : 2;

			// if there are opponent units near and we outnumber them, attack
			if ((oppUnitsNear == 0) || (myUnitsNear >= oppUnitsNear + 2))
			{
				regroupStatus = std::string("\x08 We have more near");
				return false;
			}
			// otherwise they outnumber us so regroup
			else
			{
				regroupStatus = std::string("\x08 Opponent has more near");
				return true;
			}
		}
	}

	if ((visibleEnemyForceSize > 0) && visibleEnemyForceSize > (numUnitsInRadius + numUnitsInCombat))
	{
		return true;
	}
	else
	{
		return false;
	}
}

void Squad::setSquadOrder(SquadOrder & so)
{
	if (so.position != order.position)
	{
		order = so;
		distanceMap.reset();
		MapTools::getInstance()->computeDistance(distanceMap, order.position);
	}
}

bool Squad::unitNearEnemy(BWAPI::Unit * unit)
{
	UnitVector enemyNear;

	MapGrid::getInstance()->GetUnits(enemyNear, unit->getPosition(), 400, false, true);

	return enemyNear.size() > 0;
}

BWAPI::Position Squad::calcCenter()
{
	BWAPI::Position accum(0,0);
	BOOST_FOREACH(BWAPI::Unit * unit, units)
	{
		accum += unit->getPosition();
	}
	return BWAPI::Position(accum.x() / units.size(), accum.y() / units.size());
}

BWAPI::Position Squad::calcRegroupPosition()
{
	int radius = 100;
	int maxInRadius = 0;
	BWAPI::Position regroup(0,0);

	int minDist(100000);

	BOOST_FOREACH(BWAPI::Unit * unit, units)
	{
		if (!nearEnemy[unit])
		{
			int dist = unit->getDistance(order.position);
			if (dist < minDist)
			{
				minDist = dist;
				regroup = unit->getPosition();
			}
		}
	}

	if (regroup == BWAPI::Position(0,0))
	{
		return HardCodedInfo::getInstance()->getChokepoint(HardCodedInfo::NATURAL_CHOKE, BWAPI::Broodwar->self());
	}
	else
	{
		return regroup;
	}
}

BWAPI::Unit * Squad::unitClosestToEnemy()
{
	BWAPI::Unit * closest = NULL;
	int closestDist = 10000;

	BOOST_FOREACH (BWAPI::Unit * unit, units)
	{
		if (unit->getType() == BWAPI::UnitTypes::Protoss_Observer)
		{
			continue;
		}

		// the distance to the order position
		int dist = distanceMap[unit->getPosition()];

		if (dist != -1 && (!closest || dist < closestDist))
		{
			closest = unit;
			closestDist = dist;
		}
	}

	return closest;
}

int Squad::squadUnitsNear(BWAPI::Position p)
{
	int numUnits = 0;

	BOOST_FOREACH (BWAPI::Unit * unit, units)
	{
		if (unit->getDistance(p) < 600)
		{
			numUnits++;
		}
	}

	return numUnits;
}

bool Squad::squadObserverNear(BWAPI::Position p)
{
	BOOST_FOREACH (BWAPI::Unit * unit, units)
	{
		if (unit->getType().isDetector() && unit->getDistance(p) < 300)
		{
			return true;
		}
	}

	return false;
}

void Squad::updateUnits()
{
	// clean up the units vector just in case one of them died
	UnitVector goodUnits;
	BOOST_FOREACH(BWAPI::Unit * unit, units)
	{
		if( unit->isCompleted() && 
			unit->getHitPoints() > 0 && 
			unit->exists() &&
			unit->getPosition().isValid() &&
			unit->getType() != BWAPI::UnitTypes::Unknown)
		{
			goodUnits.push_back(unit);
		}
	}
	units = goodUnits;

	nearEnemy.clear();
	BOOST_FOREACH(BWAPI::Unit * unit, units)
	{
		int x = unit->getPosition().x();
		int y = unit->getPosition().y();

		int left = unit->getType().dimensionLeft();
		int right = unit->getType().dimensionRight();
		int top = unit->getType().dimensionUp();
		int bottom = unit->getType().dimensionDown();

		nearEnemy[unit] = unitNearEnemy(unit);
		if (nearEnemy[unit])
		{
			if (DRAW_UALBERTABOT_DEBUG) BWAPI::Broodwar->drawBoxMap(x-left, y - top, x + right, y + bottom, BWAPI::Colors::Red);
		}
		else
		{
			if (DRAW_UALBERTABOT_DEBUG) BWAPI::Broodwar->drawBoxMap(x-left, y - top, x + right, y + bottom, BWAPI::Colors::Green);
		}
	}

	UnitVector zealots;
	UnitVector dragoons;
	UnitVector darkTemplars;
	UnitVector observers;

	BOOST_FOREACH(BWAPI::Unit * unit, units)
	{
		if(unit->isCompleted() && unit->getHitPoints() > 0 && unit->exists())
		{
			if (unit->getType() == BWAPI::UnitTypes::Protoss_Zealot)
			{
				zealots.push_back(unit);
			}
			else if (unit->getType() == BWAPI::UnitTypes::Protoss_Dark_Templar)
			{
				darkTemplars.push_back(unit);
			}
			else if (unit->getType() == BWAPI::UnitTypes::Protoss_Dragoon)
			{
				dragoons.push_back(unit);
			}
			else if (unit->getType() == BWAPI::UnitTypes::Protoss_Observer)
			{
				observers.push_back(unit);
			}
		}
	}

	zealotManager.setUnits(zealots);
	dragoonManager.setUnits(dragoons);
	darkTemplarManager.setUnits(darkTemplars);
	observerManager.setUnits(observers);
}
