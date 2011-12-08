/* 
 * Scott Lowe
 * 11-5-11
 */
#pragma once

#include "BWAPI/UnitType.h"
#include "BWAPI/Game.h"
#include "Common.h"
#include "BWTA.h"

//this is how to define a constant
#define STATE_SIZE 400
#define ALPHA .1
#define GAMMA .95

class Qlearning
{
public:
	Qlearning(BWAPI::UnitType unit);
	~Qlearning();

	BWAPI::UnitType getUnit(); 

	void writeQsToFile();
	double getQ();
	void setTimeActionWasChosen(double time);
	void setPrevPredictedQ(double Q);
	void updateQ(double reward, double maxQ);

private:	
	double Q[2][2][2][2][5][5];//This means Q[enemyHasFlying][enemyHasCloaked][aiHasDragoon][aiHasObserver][logMinerals{less10,10to99,100to999,more1000}][logGas{less10,10to99,100to999,more1000}][action{zealot,dragoon,darkTemplar,Observer}]
	double timeActionWasChosen;
	double prevPredictedQ;
	BWAPI::UnitType thisUnit;

	void initializeRandomQs();
	double randomGaussian(double min, double max);
	
};