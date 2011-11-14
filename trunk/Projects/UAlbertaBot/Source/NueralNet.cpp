/* 
 * Tim Burr
 * 11-5-11
 */
#include "NeuralNet.h"

#include <assert.h>
#include <fstream>
#include <iostream>
#include <math.h>
#include <string>
#include <time.h>
using namespace std;

NeuralNet::NeuralNet(BWAPI::UnitType unit)
{
	thisUnit = unit;

	BWAPI::Broodwar->printf("Creating neural net for: %s\n", thisUnit.getName());	

	//read weights from file:
	string line;
	
	string fileName("C:\\NN_weights\\");
	fileName += unit.getName();
	fileName += ".txt";

	printf("\nFilename is: %s", fileName);

	ifstream weightsFile(fileName.c_str());
	
	if(!weightsFile.is_open()) //i.e. file doesn't exist yet
	{
		BWAPI::Broodwar->printf("Did not open weights file.\n");
		initializeRandomWeights(); 
	}

	for(int i=0; i < NUM_INPUTS; i++)
	{
		for(int j=0; j < NUM_HIDDEN_NODES; j++)
		{
			getline(weightsFile, line);

			char *temp = new char[line.size()+1];
			strcpy(temp, line.c_str());

			xzWeights[i][j] = atof(temp); 
		}
	}

	for(int j=0; j < NUM_HIDDEN_NODES; j++)
	{
		getline(weightsFile, line);

		char *temp = new char[line.size()+1];
		strcpy(temp, line.c_str());

		zyWeights[j] = atof(temp); 
	}

	weightsFile.close();
}

void NeuralNet::writeWeightsToFile()
{
	string fileName("C:\\NN_weights\\");
	fileName += thisUnit.getName();
	fileName += ".txt";
	
	ofstream weightsFile(fileName.c_str());

	assert(weightsFile.is_open());

	for(int i=0; i < NUM_INPUTS; i++)
	{
		for(int j=0; j < NUM_HIDDEN_NODES; j++)
		{
			weightsFile << xzWeights[i][j];
			weightsFile << "\n";
		}
	}

	for(int j=0; j < NUM_HIDDEN_NODES; j++)
	{
		weightsFile << zyWeights[j];
		weightsFile << "\n"; 
	}

	weightsFile.close();	
}

double NeuralNet::getQ(double inputs[])
{
	assert(inputs[0] > 0.9 && inputs[0] < 1.1); //bias

	double predictedQ = 1.0 * zyWeights[0]; //bias

	for(int j=1; j < NUM_HIDDEN_NODES; j++)
	{		
		double sum = 0; 

		for(int i=0; i < NUM_INPUTS; i++)
		{
			sum += inputs[i] * xzWeights[i][j];

			//save inputs for updating weights later
			prevInputs[i] = inputs[i];
		}
	
		//save hidden layer outputs for updating weights
		prevHiddenLayerOutputs[j] = sigmoid(sum);

		predictedQ += sigmoid(sum) * zyWeights[j];
	}

	//save prediction for updating weights
	prevPredictedQ = predictedQ;

	return predictedQ;
}

void NeuralNet::updateWeights(double reward)
{
	double error = reward - prevPredictedQ; 

	//update zy weights
	for(int j = 0; j < NUM_HIDDEN_NODES; j++)
	{		
		zyWeights[j] = zyWeights[j] + LEARNING_RATE * error * prevHiddenLayerOutputs[j];
	}

	//update xz weights
	for(int i=0; i < NUM_INPUTS; i++)
	{
		for(int j=0; j < NUM_HIDDEN_NODES; j++)
		{
			double deltaJ = sigmoidDerivative(prevHiddenLayerOutputs[j]) * zyWeights[j] * error;
			xzWeights[i][j] = xzWeights[i][j] + LEARNING_RATE * deltaJ * prevInputs[i];
		}
	}
}

void NeuralNet::initializeRandomWeights()
{
	BWAPI::Broodwar->printf("Initializing random weights.\n");

	string fileName("C:\\NN_weights\\");
	fileName += thisUnit.getName();
	fileName += ".txt";
	
	ofstream weightsFile(fileName.c_str());

	assert(weightsFile.is_open());

	srand((unsigned) time(0));

	for(int i=0; i < NUM_INPUTS; i++)
	{
		for(int j=0; j < NUM_HIDDEN_NODES; j++)
		{
			weightsFile << randomGaussian(-1.0, 1.0);
			weightsFile << "\n";
		}
	}

	for(int j=0; j < NUM_HIDDEN_NODES; j++)
	{
		weightsFile << randomGaussian(-1.0, 1.0);
		weightsFile << "\n"; 
	}

	weightsFile.close();
}

double NeuralNet::randomGaussian(double min, double max)
{
	return ((max - min) * ((float)rand() / RAND_MAX)) + min;
}

double NeuralNet::sigmoid(double input)
{
	return 1.0/(1.0 + exp(-input));
}

double NeuralNet::sigmoidDerivative(double input)
{
	double sig = sigmoid(input); 
	return sig * (1 - sig);
}