#include <Common.h>
#ifndef OVERLORDMANAGER_H
#define OVERLORDMANAGER_H
#include <BWAPI.h>
#include "MicroManager.h"
#include "../MapGrid.h"
#include "../UnitInfoState.h"
#include <map>
#include <algorithm>

class OverlordCompareDistance {
	
	// the zergling we are comparing this to
	BWAPI::Position attackPosition;

public:

	// constructor, takes in a zergling to compare distance to
	OverlordCompareDistance(BWAPI::Position p) : attackPosition(p) {}

	// the sorting comparison operator
	bool operator() (BWAPI::Unit * u1, BWAPI::Unit * u2) {

		return u1->getDistance(attackPosition) < u2->getDistance(attackPosition);
    }
};


class OverlordManager {

	int range, frame, xi, yi;

	OverlordManager();

	static OverlordManager *			instance;

	std::map<BWAPI::Unit *, BWAPI::Position> overlord_positions;
	std::map<BWAPI::Unit *, bool> overlord_tethered;
	
	BWAPI::Position closestUnoccupiedPosition(BWAPI::Unit * unit);
	void fillAirThreats(std::vector<AirThreat> & threats, UnitVector & candidates);
	double2 getFleeVector(const std::vector<AirThreat> & threats, BWAPI::Unit * overlord);

	BWAPI::Unit * overlordScout;
	
	std::vector<AirThreat> threats;

	// Dave added these
	BWAPI::Position ourStartPosition;
	BWAPI::Position firstOverlordScoutPosition;

public:


	static OverlordManager *	getInstance();
  
	void update();
	void onStart();
	void setAttackPosition(BWAPI::Position attackPosition);
	void onOverlordCreate(BWAPI::Unit * unit);
	void onOverlordDestroy(BWAPI::Unit * unit);
	void tetherAllOverlords();

	BWAPI::Unit * getClosestOverlord(BWAPI::Position p);

	void tetherOverlord(BWAPI::Unit * unit);
	void untetherOverlord(BWAPI::Unit * unit);

	bool overlordNear(BWAPI::Position p, double dist);
	bool overlordAssigned(BWAPI::Position p);




};
#endif

