#pragma once

#include "Common.h"
#include "../MapGrid.h"
#include "../SquadOrder.h"
#include "../MapTools.h"

struct AirThreat
{
	BWAPI::Unit *	unit;
	double			weight;
};

struct GroundThreat
{
	BWAPI::Unit *	unit;
	double			weight;
};

class MicroManager
{
	UnitVector			units;
	int					lastRegroupPerformed;

protected:

	virtual void		executeMicro(const UnitVector & targets, BWAPI::Position regroup = BWAPI::Position(0,0)) {}
	virtual void		harassMove(BWAPI::Unit * muta, BWAPI::Position location) {}
	bool				checkPositionWalkable(BWAPI::Position pos);
	bool				drawDebugVectors;
	void				drawOrderText();
	void				smartAttackUnit(BWAPI::Unit * attacker, BWAPI::Unit * target);
	void				smartAttackMove(BWAPI::Unit * attacker, BWAPI::Position targetPosition);
	void				smartMove(BWAPI::Unit * attacker, BWAPI::Position targetPosition);
	bool				unitNearEnemy(BWAPI::Unit * unit);
	bool				unitNearChokepoint(BWAPI::Unit * unit) const;

	SquadOrder			order;

public:
						MicroManager() : drawDebugVectors(true), lastRegroupPerformed(0) {}
    virtual				~MicroManager(){}

	const UnitVector &	getUnits() const { return units; }
	BWAPI::Position     calcCenter() const;

	void				setUnits(const UnitVector & units) { this->units = units; }
	void				execute(const SquadOrder & order, BWAPI::Position regroup);
};
