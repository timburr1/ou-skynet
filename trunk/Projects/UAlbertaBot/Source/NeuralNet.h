/* 
 * Tim Burr
 * 11-5-11
 */
#pragma once

#include "BWAPI/UnitType.h"
#include "BWAPI/Game.h"
#include "Common.h"
#include "BWTA.h"
#include <math.h>

//bias, 28 unit types, total number of units, 
//available capacity, log minerals, log vespene gas
#define NUM_INPUTS 33
#define NUM_HIDDEN_NODES 7
#define LEARNING_RATE .001
#define GAMMA .95

class NeuralNet
{
public:
	NeuralNet(BWAPI::UnitType unit);
	~NeuralNet();

	BWAPI::UnitType getUnit(); 

	void writeWeightsToFile();
	double getQ(double inputs[]);
	int getNumWanted();
	void setNumWanted(int num);
	void setTimeActionWasChosen(double time);
	void setPrevPredictedQ(double Q);
	void updateWeights(double reward, double maxQ, std::vector<NeuralNet*> nets);

private:	
	double xzWeights [NUM_INPUTS][NUM_HIDDEN_NODES];
	double zyWeights [NUM_HIDDEN_NODES];
	double prevInputs[NUM_INPUTS];
	double prevHiddenLayerOutputs[NUM_HIDDEN_NODES];
	double prevPredictedQ;
	double timeActionWasChosen;
	int numWanted;
	BWAPI::UnitType thisUnit;

	void initializeRandomWeights();
	double randomGaussian(double min, double max);
	double sigmoid(double input);
	double sigmoidDerivative(double input);
	
};