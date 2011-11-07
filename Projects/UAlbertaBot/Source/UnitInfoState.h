#pragma once

#include "Common.h"
#include "BWTA.h"

struct UnitInfo
{
	// we need to store all of this data because if the unit is not visible, we
	// can't reference it from the unit pointer

	int					unitID;
	int					lastHealth;
	BWAPI::Unit *		unit;
	BWAPI::Position		lastPosition;
	BWAPI::UnitType		type;

	UnitInfo(int id, BWAPI::Unit * u, BWAPI::Position last, BWAPI::UnitType t) 
	{
		unitID = id;
		unit = u;
		lastHealth = u->getHitPoints() + u->getShields();
		lastPosition = last;
		type = t;
	}
};

struct BaseInfo;
typedef std::vector<UnitInfo> UnitInfoVector;
typedef std::vector<BaseInfo> BaseInfoVector;

class UnitInfoState {

	UnitInfoState();

	static UnitInfoState *				instance;

	enum RegionType { Main, Expansion, Forces };

	std::vector< UnitInfoVector >		knownUnits[2];
	std::vector< int >					numDeadUnits[2];
	UnitInfoVector						allUnits[2];
	UnitInfoVector						enemyDetectors;
	int									numTotalDeadUnits[2];

	BaseInfoVector						allBases;

	BWTA::BaseLocation *				mainBaseLocations[2];

	std::set<BWTA::Region *>			occupiedRegions[2];

	int						mineralsLost[2];
	int						gasLost[2];

	void					populateUnitInfoVectors();

	void					eraseUnit(BWAPI::Unit * unit);
	void					updateUnit(BWAPI::Unit * unit);
	void					updateUnitInVector(BWAPI::Unit * unit, UnitInfoVector & units);
	void					eraseUnitFromVector(BWAPI::Unit * unit, UnitInfoVector & units);
	void					initializeRegionInformation();
	void					initializeBaseInfoVector();
	void					updateUnitInfo();
	void					updateBaseInfo();
	void					updateBaseLocationInfo();
	void					updateOccupiedRegions(BWTA::Region * region, BWAPI::Player * player);
	
	bool					isValidUnit(BWAPI::Unit * unit);
	bool					unitDiedNearHarass(BWAPI::Unit * unit);

	int						getIndex(BWAPI::Player * player);

	int						mutalisksKilled;

	bool					calculatedDistanceToEnemy;

	bool					enemyHasCloakedUnit;

public:

	bool goForIt;

	// yay for singletons!
	static UnitInfoState *	getInstance();

	void					update();
	void					onStart();

	// event driven stuff
	void					onUnitShow(BWAPI::Unit * unit)			{ updateUnit(unit); }
	void					onUnitHide(BWAPI::Unit * unit)			{ updateUnit(unit); }
	void					onUnitCreate(BWAPI::Unit * unit)		{ updateUnit(unit); }
	void					onUnitMorph(BWAPI::Unit * unit)			{ updateUnit(unit); }
	void					onUnitRenegade(BWAPI::Unit * unit)		{ updateUnit(unit); }
	void					onUnitDestroy(BWAPI::Unit * unit);

	bool					positionInRangeOfEnemyDetector(BWAPI::Position p);
	bool					enemyFlyerThreat();
	bool					isEnemyBuildingInRegion(BWTA::Region * region);
	int						getNumUnits(BWAPI::UnitType type, BWAPI::Player * player);
	int						getNumDeadUnits(BWAPI::UnitType type, BWAPI::Player * player);
	int						getNumTotalDeadUnits(BWAPI::Player * player);
	int						knownForceSize(BWAPI::Player * player);
	int						visibleForceSize(BWAPI::Player * player);
	int						numEnemyUnitsInRegion(BWTA::Region * region);
	int						numEnemyFlyingUnitsInRegion(BWTA::Region * region);
	int						nearbyForceSize(BWAPI::Position p, BWAPI::Player * player, int radius);
	bool					nearbyForceHasCloaked(BWAPI::Position p, BWAPI::Player * player, int radius);
	bool					canWinNearby(BWAPI::Position p);

	
	std::pair<double, double>	nearbyCombatInfo(BWAPI::Position p, BWAPI::Player * player);
	double					getDPS(BWAPI::UnitType type);

	UnitInfoVector &		getEnemyDetectors();
	UnitInfoVector &		getKnownUnitInfo(BWAPI::UnitType type, BWAPI::Player * player);
	UnitInfoVector &		getAllUnits(BWAPI::Player * player);
	BaseInfoVector &		getAllBases() { return allBases; }

	BWAPI::Unit *			getClosestUnitToTarget(BWAPI::UnitType type, BWAPI::Position target);

	std::set<BWTA::Region *> &	getOccupiedRegions(BWAPI::Player * player);
	BWTA::BaseLocation *			getMainBaseLocation(BWAPI::Player * player);

	bool					enemyHasCloakedUnits();

	void					drawUnitInformation(int x, int y);
};




struct BaseInfo {

	BWTA::BaseLocation *	baseLocation;
	int						lastFrameSeen;

	BaseInfo (BWTA::BaseLocation * base) {

		baseLocation = base;
	}

	bool isExplored() { return BWAPI::Broodwar->isExplored(baseLocation->getTilePosition()); }
	bool isVisible()  {	return BWAPI::Broodwar->isVisible(baseLocation->getTilePosition());  }

	bool isBuiltOn() {

		const UnitInfoVector & ourUnits = UnitInfoState::getInstance()->getAllUnits(BWAPI::Broodwar->self());
		const UnitInfoVector & enemyUnits = UnitInfoState::getInstance()->getAllUnits(BWAPI::Broodwar->self());

		int radius = 400;
		
		// check each of our units
		for (size_t i(0); i<ourUnits.size(); ++i) {

			if (ourUnits[i].type.isBuilding() && ourUnits[i].lastPosition.getDistance(baseLocation->getPosition()) < radius) {

				return true;
			}
		}

		// check each of the enemy units
		for (size_t i(0); i<enemyUnits.size(); ++i) {

			if (enemyUnits[i].type.isBuilding() && enemyUnits[i].lastPosition.getDistance(baseLocation->getPosition()) < radius) {

				return true;
			}
		}

		return false;
	}

};
