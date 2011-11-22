/* 
 * Tim Burr
 * 11-5-11
 */
#pragma once

#include "BWAPI/UnitType.h"
#include "BWAPI/Game.h"

//bias, 28 unit types, total number of units, 
//available capacity, log minerals, log vespene gas
#define NUM_INPUTS 33
#define NUM_HIDDEN_NODES 7
#define LEARNING_RATE .000001

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
	void updateWeights(double reward);

private:	
	double xzWeights [NUM_INPUTS][NUM_HIDDEN_NODES];
	double zyWeights [NUM_HIDDEN_NODES];
	double prevInputs[NUM_INPUTS];
	double prevHiddenLayerOutputs[NUM_HIDDEN_NODES];
	double prevPredictedQ;
	int numWanted;

	void initializeRandomWeights();
	double randomGaussian(double min, double max);
	double sigmoid(double input);
	double sigmoidDerivative(double input);
	BWAPI::UnitType thisUnit;
};