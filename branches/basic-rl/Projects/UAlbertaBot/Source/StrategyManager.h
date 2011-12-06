#pragma once

#include "Common.h"
#include "BWTA.h"
#include "base/BuildOrderQueue.h"
#include "UnitInfoState.h"
#include "base/WorkerManager.h"

#include "base/ProductionManager.h"
#include "NeuralNet.h"

const std::string protossUnitsAndBuildings[] = {
	"Protoss Probe",
	"Protoss Pylon",
	"Protoss Nexus",
	"Protoss Gateway",
	"Protoss Zealot",
	"Protoss Cybernetics Core",
	"Protoss Dragoon",
	"Protoss Assimilator",
	"Protoss Forge",
	"Protoss Photon Cannon",
	"Protoss High Templar",
	"Protoss Citadel of Adun",
	"Protoss Templar Archives",
	"Protoss Robotics Facility",
	"Protoss Robotics Support Bay",
	"Protoss Observatory",
	"Protoss Stargate",
	"Protoss Scout",
	"Protoss Arbiter Tribunal",
	"Protoss Arbiter",
	"Protoss Shield Battery",
	"Protoss Dark Templar",
	"Protoss Shuttle",
	"Protoss Reaver",
	"Protoss Observer",
	"Protoss Corsair",
	"Protoss Fleet Beacon",
	"Protoss Carrier"};

const std::string protossUnits[] = {
	"Protoss Dark Templar",
	"Protoss Dragoon",
	"Protoss Observer",
	"Protoss Zealot"};

const std::string zergUnits[] = {
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

	
	std::vector<NeuralNet*> nets;
	std::vector<NeuralNet*> netsToUpdate;

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
	void onEnd(int score);

private:
	bool contains(std::vector<NeuralNet*> nets, NeuralNet* n);
};
