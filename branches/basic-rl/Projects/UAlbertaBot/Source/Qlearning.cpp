/* 
 * Scott Lowe
 * 11-5-11
 */
#include "Qlearning.h"

#include <assert.h>
#include <fstream>
#include <iostream>
#include <math.h>
#include <string>
#include <time.h>
using namespace std;

Qlearning::Qlearning(BWAPI::UnitType unit)
{
	thisUnit = unit;

	BWAPI::Broodwar->printf("Creating neural net for: %s,\n", thisUnit.getName().c_str());	

	//read weights from file:
	string line;
	
	string fileName("C:\\Q_Values\\");
	fileName += unit.getName();
	fileName += ".txt";

	//printf("\nFilename is: %s", fileName);

	ifstream weightsFile(fileName.c_str());
	
	if(!weightsFile.is_open()) //i.e. file doesn't exist yet
	{
		BWAPI::Broodwar->printf("Did not open weights file.\n");
		weightsFile.close();
		initializeRandomQs(); 
		ifstream weightsFile(fileName.c_str());
	}


	for(int i=0; i < 2; i++)
	{
		for(int j=0; j < 2; j++)
		{
			for(int k=0; k < 2; k++)
			{
				for(int l=0; l < 2; l++)
				{
					for(int m=0; m < 5; m++)
					{
						for(int n=0; n < 5; n++){

							getline(weightsFile, line);

							char *temp = new char[line.size()+1];
							strcpy(temp, line.c_str());

							Q[i][j][k][l][m][n] = atof(temp); 
						}
					}
				}
			}
		}
	}

	weightsFile.close();
	timeActionWasChosen = 0;
}

Qlearning::~Qlearning()
{
	delete [] Q;
}

void Qlearning::writeQsToFile()
{
	string fileName("C:\\Q_Values\\");
	fileName += thisUnit.getName();
	fileName += ".txt";
	
	ofstream weightsFile(fileName.c_str());

	assert(weightsFile.is_open());

	for(int i=0; i < 2; i++)
	{
		for(int j=0; j < 2; j++)
		{
			for(int k=0; k < 2; k++)
			{
				for(int l=0; l < 2; l++)
				{
					for(int m=0; m < 5; m++)
					{
						for(int n=0; n < 5; n++){
							weightsFile << Q[i][j][k][l][m][n];
							weightsFile << "\n";
						}
					}
				}
			}
		}
	}

	weightsFile.close();	
}

double Qlearning::getQ()
{
	//to determine Q, find what zone we're in
	
	//does enemy have flying units?
	int enemyHasFlying = 0;
	BOOST_FOREACH(BWAPI::Unit * unit, BWAPI::Broodwar->enemy()->getUnits()) 
	{
		if (unit->isVisible() && unit->getType().isFlyer()) 
		{
			enemyHasFlying = 1; //enemy has flying units
			break;
		}
	}

	//does enemy have cloaked units?
	int enemyHasCloaked = 0;
	BOOST_FOREACH(BWAPI::Unit * unit, BWAPI::Broodwar->enemy()->getUnits()) 
	{
		if (unit->isVisible() && unit->isCloaked()) 
		{
			enemyHasCloaked = 1; //enemy has cloaked units
			break;
		}
	}

	//do we have dragoons to shoot flyers?
	int weHaveDragoons = 0;
	BOOST_FOREACH(BWAPI::Unit * unit, BWAPI::Broodwar->self()->getUnits()) 
	{
		if (unit->getType().getName().compare(BWAPI::UnitTypes::Protoss_Dragoon.getName())==0) 
		{
			weHaveDragoons = 1; //we have dragoon units
			break;
		}
	}

	//do we have observers to shoot flyers?
	int weHaveObservers = 0;
	BOOST_FOREACH(BWAPI::Unit * unit, BWAPI::Broodwar->self()->getUnits()) 
	{
		if (unit->getType().getName().compare(BWAPI::UnitTypes::Protoss_Observer.getName())==0) 
		{
			weHaveObservers = 1; //we have dragoon units
			break;
		}
	}

	//what minerals category do we fall in?
	int mineralsBin = 0;
	int minerals = BWAPI::Broodwar->self()->minerals();
	if(minerals < 10){
		mineralsBin = 0;
	} else if (minerals < 100) {
		mineralsBin = 1;
	} else if(minerals < 1000) {
		mineralsBin = 2;
	} else if(minerals < 10000){
		mineralsBin = 3;
	} else {
		mineralsBin = 4;
	}

	//what gas category do we fall in?
	int gasBin = 0;
	int gas = BWAPI::Broodwar->self()->gas();
	if(gas < 10){
		gasBin = 0;
	} else if (gas < 100) {
		gasBin = 1;
	} else if(gas < 1000) {
		gasBin = 2;
	} else if(gas < 10000){
		gasBin = 3;
	} else {
		gasBin = 4;
	}

	return Q[enemyHasFlying][enemyHasCloaked][weHaveDragoons][weHaveObservers][mineralsBin][gasBin];

}

void Qlearning::updateQ(double reward,double maxQ)
{

	//determine the time since the action was chosen
	double discountRate = (BWAPI::Broodwar->getFrameCount()-timeActionWasChosen)/24;//this gives seconds since action was chosen

	double error = reward + pow(GAMMA ,discountRate)*maxQ - prevPredictedQ; 

	//to determine what Q to update
	
	//does enemy have flying units?
	int enemyHasFlying = 0;
	BOOST_FOREACH(BWAPI::Unit * unit, BWAPI::Broodwar->enemy()->getUnits()) 
	{
		if (unit->isVisible() && unit->getType().isFlyer()) 
		{
			enemyHasFlying = 1; //enemy has flying units
			break;
		}
	}

	//does enemy have cloaked units?
	int enemyHasCloaked = 0;
	BOOST_FOREACH(BWAPI::Unit * unit, BWAPI::Broodwar->enemy()->getUnits()) 
	{
		if (unit->isVisible() && unit->isCloaked()) 
		{
			enemyHasCloaked = 1; //enemy has cloaked units
			break;
		}
	}

	//do we have dragoons to shoot flyers?
	int weHaveDragoons = 0;
	BOOST_FOREACH(BWAPI::Unit * unit, BWAPI::Broodwar->self()->getUnits()) 
	{
		if (unit->getType().getName().compare(BWAPI::UnitTypes::Protoss_Dragoon.getName())==0) 
		{
			weHaveDragoons = 1; //we have dragoon units
			break;
		}
	}

	//do we have observers to shoot flyers?
	int weHaveObservers = 0;
	BOOST_FOREACH(BWAPI::Unit * unit, BWAPI::Broodwar->self()->getUnits()) 
	{
		if (unit->getType().getName().compare(BWAPI::UnitTypes::Protoss_Observer.getName())==0) 
		{
			weHaveObservers = 1; //we have dragoon units
			break;
		}
	}

	//what minerals category do we fall in?
	int mineralsBin = 0;
	int minerals = BWAPI::Broodwar->self()->minerals();
	if(minerals < 10){
		mineralsBin = 0;
	} else if (minerals < 100) {
		mineralsBin = 1;
	} else if(minerals < 1000) {
		mineralsBin = 2;
	} else if(minerals < 10000){
		mineralsBin = 3;
	} else {
		mineralsBin = 4;
	}

	//what gas category do we fall in?
	int gasBin = 0;
	int gas = BWAPI::Broodwar->self()->gas();
	if(gas < 10){
		gasBin = 0;
	} else if (gas < 100) {
		gasBin = 1;
	} else if(gas < 1000) {
		gasBin = 2;
	} else if(gas < 10000){
		gasBin = 3;
	} else {
		gasBin = 4;
	}

	Q[enemyHasFlying][enemyHasCloaked][weHaveDragoons][weHaveObservers][mineralsBin][gasBin] =
		Q[enemyHasFlying][enemyHasCloaked][weHaveDragoons][weHaveObservers][mineralsBin][gasBin] + .1*(reward + maxQ - Q[enemyHasFlying][enemyHasCloaked][weHaveDragoons][weHaveObservers][mineralsBin][gasBin]);
}

void Qlearning::initializeRandomQs()
{
	BWAPI::Broodwar->printf("Initializing random Qs.\n");

	string fileName("C:\\Q_Values\\");
	fileName += thisUnit.getName();
	fileName += ".txt";
	
	ofstream weightsFile(fileName.c_str());

	assert(weightsFile.is_open());

	srand((unsigned) time(0));

	for(int i=0; i < STATE_SIZE; i++)
	{
		weightsFile << randomGaussian(0.0, 100.0);
		weightsFile << "\n";
	}

	weightsFile.close();
}

double Qlearning::randomGaussian(double min, double max)
{
	return ((max - min) * ((float)rand() / RAND_MAX)) + min;
}

BWAPI::UnitType Qlearning::getUnit()
{
	return thisUnit;
}

void Qlearning::setTimeActionWasChosen(double time)
{
	timeActionWasChosen = time;
}

void Qlearning::setPrevPredictedQ(double Q)
{
	prevPredictedQ = Q;
}