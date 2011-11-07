/* 
 * Tim Burr
 * 11-5-11
 */

//15 unit types, 16 buiding types, total number of units, 
//available capacity, log minerals, log vespene gas
#define NUM_INPUTS 36
#define NUM_HIDDEN_NODES 7
#define LEARNING_RATE .0000000001

enum unitTypes {
	NEXUS,
	ASSIMILATOR,
	PYLON,
	GATEWAY,
	FORGE,
	PHOTON_CANNON,
	SHIELD_BATTERY,
	CYBERNETICS_CORE,
	ROBOTICS_FACILITY,
	SUPPORT_BAY,
	OBSERVATORY,
	CITADEL,
	ARCHIVE,
	STARGATE,
	FLEET_BEACON,
	TRIBUNAL,
	ARBITER,
	ARCHON,
	CARRIER,
	CORSAIR,
	DARK_ARCHON,
	DARK_TEMPLAR,
	DRAGOON,
	HIGH_TEMPLAR,
	INTERCEPTOR,
	OBSERVER,
	PROBE,
	REAVER,
	SCOUT,
	SHUTTLE,
	ZEALOT
};

class NeuralNet
{
public:
	NeuralNet(unitTypes unit);
	void writeWeightsToFile();
	double getQ(double inputs[]);
	void updateWeights(double reward);

private:	
	double xzWeights [NUM_INPUTS][NUM_HIDDEN_NODES];
	double zyWeights [NUM_HIDDEN_NODES];
	double prevInputs[NUM_INPUTS];
	double prevHiddenLayerOutputs[NUM_HIDDEN_NODES];
	double prevPredictedQ;

	void initializeRandomWeights();
	double randomGaussian(double min, double max);
	double sigmoid(double input);
	double sigmoidDerivative(double input);
	unitTypes thisUnit;
};