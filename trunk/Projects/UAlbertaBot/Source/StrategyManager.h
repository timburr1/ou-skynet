#pragma once

#include "Common.h"
#include "BWTA.h"
#include "base/BuildOrderQueue.h"
#include "UnitInfoState.h"
#include "base/WorkerManager.h"

#include "base/ProductionManager.h"
#include "NeuralNet.h"

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
};
