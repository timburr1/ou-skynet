#ifndef MUTAMANAGER_H_
#define MUTAMANAGER_H_
#include <Common.h>
#include <BWAPI.h>
#include "MicroManager.h"
#include "MicroUtil.h"

class CompareMutaTargetSSD {
	
	// the zergling we are comparing this to
	const UnitVector & mutas;

public:

	// constructor, takes in a zergling to compare distance to
	CompareMutaTargetSSD(const UnitVector & z) : mutas(z) {}

	// the sorting operator
	bool operator() (BWAPI::Unit * u1, BWAPI::Unit * u2) {

		int p1 = getAttackPriority(u1->getType());
		int p2 = getAttackPriority(u2->getType());

		if (p1 != p2) {
			return p1 > p2;
		} else {
			return ssd(u1) < ssd(u2);
		}
    }

	// get the attack priority of a type in relation to a zergling
	int getAttackPriority(BWAPI::UnitType type) {

		int priority = 0;
		if (type == BWAPI::UnitTypes::Terran_Medic || type.airWeapon() != BWAPI::WeaponTypes::None) {
			priority = 10;
		} else if (type ==  BWAPI::UnitTypes::Terran_Bunker) {
			priority = 9;
		} else if (type == BWAPI::UnitTypes::Protoss_Reaver) {
			priority = 8;
		} else if (type.isWorker()) {
			priority = 7;
		} else if (type ==  BWAPI::UnitTypes::Protoss_High_Templar) {
			priority = 6;
		} else if (type.groundWeapon() != BWAPI::WeaponTypes::None) {
			priority = 2;
		}  else if (type.supplyProvided() > 0) {
			priority = 1;
		} 													
		
		return priority;
	}

	int ssd(BWAPI::Unit * target) {

		int sum = 0;
		for (size_t i(0); i<mutas.size(); ++i) {
			
			int xdiff(mutas[i]->getPosition().x() - target->getPosition().x());
			int ydiff(mutas[i]->getPosition().y() - target->getPosition().y());

			sum += xdiff*xdiff + ydiff*ydiff;
		}

		return sum;
	}
};

class MutaManager : public MicroManager
{

public:

    MutaManager();
    virtual ~MutaManager(){}
    void executeMicro(const UnitVector & targets);

	// Making these public so I can use them in overlord manager
	void fillAirThreats(std::vector<AirThreat> & threats, BWAPI::Unit * target);
	double2 getFleeVector(const std::vector<AirThreat> & threats, BWAPI::Unit * mutalisk);

private:

    const double speed;
	const int cooldown, range;
    std::vector<AirThreat> threats;

	double totalDistance;
	size_t numShots;
	size_t numFrames;

	BWAPI::Unit * chooseTarget(const UnitVector & mutas, const UnitVector & targets);
	void executeMutaDance(const UnitVector & mutas, BWAPI::Unit * target);
	int sumSquaredDistance(const UnitVector & mutas, BWAPI::Unit * target);
	void harassMove(BWAPI::Unit * muta, BWAPI::Position location);
};
#endif

