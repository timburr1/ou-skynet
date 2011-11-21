#pragma once

#include "Common.h"
#include "BWTA.h"
#include "base/BuildOrderQueue.h"
#include "UnitInfoState.h"
#include "base/WorkerManager.h"

#include "base/ProductionManager.h"
#include "NeuralNet.h"

const std::string protossUnits[] = {
	"Protoss Zealot",
	"Protoss Dragoon",
	"Protoss High Templar",
	"Protoss Scout",
	"Protoss Arbiter",
	"Protoss Dark Templar",
	"Protoss Shuttle",
	"Protoss Reaver",
	"Protoss Observer",
	"Protoss Corsair",
	"Protoss Carrier"};

const const std::string zergUnits[] = {
	"Zerg Drone",
	"Zerg Overlord",
	"Zerg Hatchery",
	"Zerg Spawning Pool",
	"Zerg Zergling",
	"Zerg Extractor",
	"Zerg Lair",
	"Zerg Hydralisk Den",
	"Zerg Hydralisk",
	"Zerg Spire",
	"Zerg Mutalisk"};

class StrategyManager 
{
	StrategyManager();
	static StrategyManager *				instance;

	std::vector<std::string>	protossOpeningBook;
	std::vector<std::string>	terranOpeningBook;
	std::vector<std::string>	zergOpeningBook;

	BWAPI::Race					enemyRace;

	bool						firstAttackSent;

	std::vector< std::pair<MetaType,int> > currentGoal;

	
	std::vector<NeuralNet> nets;
	std::vector<NeuralNet> netsToUpdate;

public:

	enum ProtossStrategy { ProtossZealotRush, ProtossDarkTemplar, NumProtossStrategies };
	enum TerranStrategy  { TerranZealotRush, TerranDarkTemplar, NumTerranStrategies };
	enum ZergStrategy    { ZergZealotRush, NumZergStrategies };

	static StrategyManager *	getInstance();

	bool	expand();
	bool	regroup(int numInRadius);
	bool	doAttack(const UnitVector & freeUnits);
	bool	defendWithWorkers();
	void	drawGoalInformation(int x, int y);

	bool	rushDetected();

	std::vector< std::pair<MetaType,int> > getBuildOrderGoal();
	void updateNeuralNets(int score);
};
