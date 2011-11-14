#include "Common.h"
#include "ProductionManager.h"

#include "starcraftsearch\ActionSet.hpp"
#include "starcraftsearch\ProtossState.hpp"
#include "starcraftsearch\DFBBStarcraftSearch.hpp"
#include "starcraftsearch\StarcraftData.hpp"

#define BOADD(N, T, B) for (int i=0; i<N; ++i) { queue.queueAsLowestPriority(MetaType(T), B); }

#define GOAL_ADD(G, M, N) G.push_back(std::pair<MetaType, int>(M, N))

ProductionManager::ProductionManager() : reservedMinerals(0), reservedGas(0), 
	assignedWorkerForThisBuilding(false), haveLocationForThisBuilding(false),
	enemyCloakedDetected(false), rushDetected(false)
{
	populateTypeCharMap();

	//setup neural nets. initially, we just want to test w/ gateways and zealots.
	//nets.push_back(NeuralNet(BWAPI::UnitTypes::Protoss_Zealot));
	//nets.push_back(NeuralNet(BWAPI::UnitTypes::Protoss_Gateway));

	setBuildOrder(StarcraftBuildOrderSearchManager::getInstance()->getOpeningBuildOrder());
}


void ProductionManager::setBuildOrder(std::vector<MetaType> & buildOrder)
{
	// clear the current build order
	queue.clearAll();

	// for each item in the results build order, add it
	for (size_t i(0); i<buildOrder.size(); ++i)
	{
		// queue the item
		queue.queueAsLowestPriority(buildOrder[i], true);
	}
}

void ProductionManager::testBuildOrderSearch(std::vector< std::pair<MetaType, int> > & goal)
{	
	// get the build order from the search
	BWAPI::Race ourRace = BWAPI::Broodwar->self()->getRace();
	std::vector<MetaType> buildOrder = StarcraftBuildOrderSearchManager::getInstance()->findBuildOrder(ourRace, goal);

	// set the build order
	setBuildOrder(buildOrder);
}

void ProductionManager::update() 
{
	// check the queue for stuff we can build
	manageBuildOrderQueue();

	// if nothing is currently building, get a new goal from the strategy manager
	if (queue.size() == 0 && BWAPI::Broodwar->getFrameCount() > 100)
	{
		testBuildOrderSearch(StrategyManager::getInstance()->getBuildOrderGoal());
	}

	// detect if there's a build order deadlock once per second
	if ((BWAPI::Broodwar->getFrameCount() % 24 == 0) && detectBuildOrderDeadlock())
	{
		BWAPI::Broodwar->printf("Supply deadlock detected, building pylon");
		queue.queueAsHighestPriority(MetaType(BWAPI::UnitTypes::Protoss_Pylon), true);
	}

	// if they have cloaked units get a new goal asap
	if (!enemyCloakedDetected && UnitInfoState::getInstance()->enemyHasCloakedUnits())
	{
		BWAPI::Broodwar->printf("Enemy Cloaked Unit Detected!");
		testBuildOrderSearch(StrategyManager::getInstance()->getBuildOrderGoal());
		enemyCloakedDetected = true;
	}

	printProductionInfo(10, 30);

	StrategyManager::getInstance()->drawGoalInformation(300,200);

//	if (DRAW_UALBERTABOT_DEBUG) BWAPI::Broodwar->drawTextScreen(447, 17, "\x07 %d", BuildingManager::getInstance()->getReservedMinerals());

	//Here I'm printing all the things I can build
	//BWAPI::Broodwar->printf("%d",canMakeList().size());

	BOOST_FOREACH(BWAPI::UnitType t,canMakeList()){
		BWAPI::Broodwar->printf(t.getName().c_str());
	}

	
}

// on unit destroy
void ProductionManager::onUnitDestroy(BWAPI::Unit * unit)
{
	// we don't care if it's not our unit
	if (unit->getPlayer() != BWAPI::Broodwar->self())
	{
		return;
	}
		
	// if it's a worker or a building, we need to re-search for the current goal
	if ((unit->getType().isWorker() && !WorkerManager::getInstance()->isWorkerScout(unit)) || unit->getType().isBuilding())
	{
		BWAPI::Broodwar->printf("Critical unit died, re-searching build order");
		testBuildOrderSearch(StrategyManager::getInstance()->getBuildOrderGoal());
	}
}

void ProductionManager::manageBuildOrderQueue() 
{
	// if there is nothing in the queue, oh well
	if (queue.isEmpty()) 
	{
		return;
	}

	// the current item to be used
	BuildOrderItem<PRIORITY_TYPE> & currentItem = queue.getHighestPriorityItem();

	// while there is still something left in the queue
	while (!queue.isEmpty()) 
	{
		// this is the unit which can produce the currentItem
		BWAPI::Unit * producer = selectUnitOfType(currentItem.metaType.whatBuilds());

		// check to see if we can make it right now
		bool canMake = canMakeNow(producer, currentItem.metaType);

		// if we try to build too many refineries manually remove it
		if (currentItem.metaType.isRefinery() && (BWAPI::Broodwar->self()->allUnitCount(BWAPI::Broodwar->self()->getRace().getRefinery() >= 3)))
		{
			queue.removeCurrentHighestPriorityItem();
			break;
		}

		// if the next item in the list is a building and we can't yet make it
		if (currentItem.metaType.isBuilding() && !(producer && canMake))
		{
			// construct a temporary building object
			Building b(currentItem.metaType.unitType, BWAPI::Broodwar->self()->getStartLocation());

			// set the producer as the closest worker, but do not set its job yet
			producer = WorkerManager::getInstance()->getBuilder(b, false);

			// predict the worker movement to that building location
			predictWorkerMovement(b);
		}

		// if we can make the current item
		if (producer && canMake) 
		{
			// create it
			createMetaType(producer, currentItem.metaType);
			assignedWorkerForThisBuilding = false;
			haveLocationForThisBuilding = false;

			// and remove it from the queue
			queue.removeCurrentHighestPriorityItem();

			// don't actually loop around in here
			break;
		}
		// otherwise, if we can skip the current item
		else if (queue.canSkipItem())
		{
			// skip it
			queue.skipItem();

			// and get the next one
			currentItem = queue.getNextHighestPriorityItem();				
		}
		else 
		{
			// so break out
			break;
		}
	}
}

bool ProductionManager::canMakeNow(BWAPI::Unit * producer, MetaType t)
{
	bool canMake = meetsReservedResources(t);
	if (canMake)
	{
		if (t.isUnit())
		{
			canMake = BWAPI::Broodwar->canMake(producer, t.unitType);
		}
		else if (t.isTech())
		{
			canMake = BWAPI::Broodwar->canResearch(producer, t.techType);
		}
		else if (t.isUpgrade())
		{
			canMake = BWAPI::Broodwar->canUpgrade(producer, t.upgradeType);
		}
		else
		{	
			assert(false);
		}
	}

	return canMake;
}

/**
* This method allows you to determine if any of your units can build this MetaType t
*/
bool ProductionManager::canProduce(MetaType t)
{
	
	bool canMake = meetsReservedResources(t);
	if(canMake == false){
		return canMake;
	} else {
		BOOST_FOREACH(BWAPI::Unit * unit, BWAPI::Broodwar->self()->getUnits())
		{
			if (t.isUnit())
			{
				canMake = BWAPI::Broodwar->canMake(unit, t.unitType);
			}
			else if (t.isTech())
			{
				canMake = BWAPI::Broodwar->canResearch(unit, t.techType);
			}
			else if (t.isUpgrade())
			{
				canMake = BWAPI::Broodwar->canUpgrade(unit, t.upgradeType);
			}
			else
			{		
				assert(false);
			}
			if(canMake){
				return canMake;
			}
		}
	}

	return false;
}

/**
* Returns a vector of all possible MetaTypes that can be built
*/
std::vector<BWAPI::UnitType> ProductionManager::canMakeList()
{
	std::vector<BWAPI::UnitType> list;

	BOOST_FOREACH(BWAPI::UnitType t, BWAPI::UnitTypes::allUnitTypes()){
		if(canProduce( MetaType(t) )){
			list.push_back(t);
		}
	}

	return list;
}


bool ProductionManager::detectBuildOrderDeadlock()
{
	// if the queue is empty there is no deadlock
	if (queue.size() == 0 || BWAPI::Broodwar->self()->supplyTotal() >= 390)
	{
		return false;
	}

	// are any supply providers being built currently
	bool supplyInProgress =		BuildingManager::getInstance()->isBeingBuilt(BWAPI::Broodwar->self()->getRace().getCenter()) || 
								BuildingManager::getInstance()->isBeingBuilt(BWAPI::Broodwar->self()->getRace().getSupplyProvider());

	// does the current item being built require more supply
	int supplyCost			= queue.getHighestPriorityItem().metaType.supplyRequired();
	int supplyAvailable		= std::max(0, BWAPI::Broodwar->self()->supplyTotal() - BWAPI::Broodwar->self()->supplyUsed());

	// if we don't have enough supply and none is being built, there's a deadlock
	if ((supplyAvailable < supplyCost) && !supplyInProgress)
	{
		return true;
	}

	return false;
}

// When the next item in the queue is a building, this checks to see if we should move to it
// This function is here as it needs to access prodction manager's reserved resources info
void ProductionManager::predictWorkerMovement(const Building & b)
{
	// get a possible building location for the building
	if (!haveLocationForThisBuilding)
	{
		predictedTilePosition			= BuildingManager::getInstance()->getBuildingLocation(b);
	}

	if (predictedTilePosition != BWAPI::TilePositions::None)
	{
		haveLocationForThisBuilding		= true;
	}
	else
	{
		return;
	}
	
	// draw a box where the building will be placed
	int x1 = predictedTilePosition.x() * 32;
	int x2 = x1 + (b.type.tileWidth()) * 32;
	int y1 = predictedTilePosition.y() * 32;
	int y2 = y1 + (b.type.tileHeight()) * 32;
	if (DRAW_UALBERTABOT_DEBUG) BWAPI::Broodwar->drawBoxMap(x1, y1, x2, y2, BWAPI::Colors::Blue, false);

	// where we want the worker to walk to
	BWAPI::Position walkToPosition		= BWAPI::Position(x1 + (b.type.tileWidth()/2)*32, y1 + (b.type.tileHeight()/2)*32);

	// compute how many resources we need to construct this building
	int mineralsRequired				= std::max(0, b.type.mineralPrice() - getFreeMinerals());
	int gasRequired						= std::max(0, b.type.gasPrice() - getFreeGas());

	// get a candidate worker to move to this location
	BWAPI::Unit * moveWorker			= WorkerManager::getInstance()->getMoveWorker(walkToPosition);

	// Conditions under which to move the worker: 
	//		- there's a valid worker to move
	//		- we haven't yet assigned a worker to move to this location
	//		- the build position is valid
	//		- we will have the required resources by the time the worker gets there
	if (moveWorker && haveLocationForThisBuilding && !assignedWorkerForThisBuilding && (predictedTilePosition != BWAPI::TilePositions::None) &&
		WorkerManager::getInstance()->willHaveResources(mineralsRequired, gasRequired, moveWorker->getDistance(walkToPosition)) )
	{
		// we have assigned a worker
		assignedWorkerForThisBuilding = true;

		// tell the worker manager to move this worker
		WorkerManager::getInstance()->setMoveWorker(mineralsRequired, gasRequired, walkToPosition);
	}
}

void ProductionManager::performCommand(BWAPI::UnitCommandType t) {

	// if it is a cancel construction, it is probably the extractor trick
	if (t == BWAPI::UnitCommandTypes::Cancel_Construction)
	{
		BWAPI::Unit * extractor = NULL;
		BOOST_FOREACH(BWAPI::Unit * unit, BWAPI::Broodwar->self()->getUnits())
		{
			if (unit->getType() == BWAPI::UnitTypes::Zerg_Extractor)
			{
				extractor = unit;
			}
		}

		if (extractor)
		{
			extractor->cancelConstruction();
		}
	}
}

int ProductionManager::getFreeMinerals()
{
	return BWAPI::Broodwar->self()->minerals() - BuildingManager::getInstance()->getReservedMinerals();
}

int ProductionManager::getFreeGas()
{
	return BWAPI::Broodwar->self()->gas() - BuildingManager::getInstance()->getReservedGas();
}

// return whether or not we meet resources, including building reserves
bool ProductionManager::meetsReservedResources(MetaType type) 
{
	// return whether or not we meet the resources
	return (type.mineralPrice() <= getFreeMinerals()) && (type.gasPrice() <= getFreeGas());
}

// this function will check to see if all preconditions are met and then create a unit
void ProductionManager::createMetaType(BWAPI::Unit * producer, MetaType t) 
{
	// TODO: special case of evolved zerg buildings needs to be handled

	// if we're dealing with a building
	if (t.isUnit() && t.unitType.isBuilding())
	{
		// send the building task to the building manager
		BuildingManager::getInstance()->addBuildingTask(t.unitType, BWAPI::Broodwar->self()->getStartLocation());
	}
	// if we're dealing with a non-building unit
	else if (t.isUnit()) 
	{
		// if the race is zerg, morph the unit
		if (t.unitType.getRace() == BWAPI::Races::Zerg) {
			producer->morph(t.unitType);

		// if not, train the unit
		} else {
			producer->train(t.unitType);
		}
	}
	// if we're dealing with a tech research
	else if (t.isTech())
	{
		producer->research(t.techType);
	}
	else if (t.isUpgrade())
	{
		//Logger::getInstance()->log("Produce Upgrade: " + t.getName() + "\n");
		producer->upgrade(t.upgradeType);
	}
	else
	{	
		// critical error check
//		assert(false);

		//Logger::getInstance()->log("createMetaType error: " + t.getName() + "\n");
	}
	
}

// selects a unit of a given type
BWAPI::Unit * ProductionManager::selectUnitOfType(BWAPI::UnitType type, bool leastTrainingTimeRemaining, BWAPI::Position closestTo) {

	// if we have none of the unit type, return NULL right away
	if (BWAPI::Broodwar->self()->completedUnitCount(type) == 0) 
	{
		return NULL;
	}

	BWAPI::Unit * unit = NULL;

	// if we are concerned about the position of the unit, that takes priority
	if (closestTo != BWAPI::Position(0,0)) {

		double minDist;

		BOOST_FOREACH (BWAPI::Unit * u, BWAPI::Broodwar->self()->getUnits()) {

			if (u->getType() == type) {

				double distance = u->getDistance(closestTo);
				if (!unit || distance < minDist) {
					unit = u;
					minDist = distance;
				}
			}
		}

		// if it is a building and we are worried about selecting the unit with the least
		// amount of training time remaining
	} else if (type.isBuilding() && leastTrainingTimeRemaining) {

		BOOST_FOREACH (BWAPI::Unit * u, BWAPI::Broodwar->self()->getUnits()) {

			if (u->getType() == type && u->isCompleted() && !u->isTraining() && !u->isLifted() &&!u->isUnpowered()) {

				return u;
			}
		}
		// otherwise just return the first unit we come across
	} else {

		BOOST_FOREACH(BWAPI::Unit * u, BWAPI::Broodwar->self()->getUnits()) 
		{
			if (u->getType() == type && u->isCompleted() && u->getHitPoints() > 0 && !u->isLifted() &&!u->isUnpowered()) 
			{
				return u;
			}
		}
	}

	// return what we've found so far
	return NULL;
}

void ProductionManager::onUnitMorph(BWAPI::Unit * unit) {


}

void ProductionManager::onSendText(std::string text)
{
	MetaType typeWanted(BWAPI::UnitTypes::None);
	bool makeUnit = false;
	int numWanted = 0;

	if (text.compare("clear") == 0)
	{
		searchGoal.clear();
	}
	else if (text.compare("search") == 0)
	{
		testBuildOrderSearch(searchGoal);
		searchGoal.clear();
	}
	else if (text[0] >= 'a' && text[0] <= 'z')
	{
		MetaType typeWanted = typeCharMap[text[0]];
		text = text.substr(2,text.size());
		numWanted = atoi(text.c_str());

		searchGoal.push_back(std::pair<MetaType, int>(typeWanted, numWanted));
	}
}

void ProductionManager::populateTypeCharMap()
{
	typeCharMap['p'] = MetaType(BWAPI::UnitTypes::Protoss_Probe);
	typeCharMap['z'] = MetaType(BWAPI::UnitTypes::Protoss_Zealot);
	typeCharMap['d'] = MetaType(BWAPI::UnitTypes::Protoss_Dragoon);
	typeCharMap['t'] = MetaType(BWAPI::UnitTypes::Protoss_Dark_Templar);
	typeCharMap['c'] = MetaType(BWAPI::UnitTypes::Protoss_Corsair);
	typeCharMap['e'] = MetaType(BWAPI::UnitTypes::Protoss_Carrier);
	typeCharMap['h'] = MetaType(BWAPI::UnitTypes::Protoss_High_Templar);
	typeCharMap['n'] = MetaType(BWAPI::UnitTypes::Protoss_Photon_Cannon);
	typeCharMap['a'] = MetaType(BWAPI::UnitTypes::Protoss_Arbiter);
	typeCharMap['r'] = MetaType(BWAPI::UnitTypes::Protoss_Reaver);
	typeCharMap['o'] = MetaType(BWAPI::UnitTypes::Protoss_Observer);
	typeCharMap['s'] = MetaType(BWAPI::UnitTypes::Protoss_Scout);
	typeCharMap['l'] = MetaType(BWAPI::UpgradeTypes::Leg_Enhancements);
	typeCharMap['v'] = MetaType(BWAPI::UpgradeTypes::Singularity_Charge);
}

void ProductionManager::printProductionInfo(int x, int y)
{
	// fill prod with each unit which is under construction
	std::vector<BWAPI::Unit *> prod;
	BOOST_FOREACH (BWAPI::Unit * unit, BWAPI::Broodwar->self()->getUnits())
	{
		if (unit->isBeingConstructed())
		{
			prod.push_back(unit);
		}
	}
	
	// sort it based on the time it was started
	std::sort(prod.begin(), prod.end(), CompareWhenStarted());

	if (DRAW_UALBERTABOT_DEBUG) BWAPI::Broodwar->drawTextScreen(x, y, "\x04 Build Order Info:");
	if (DRAW_UALBERTABOT_DEBUG) BWAPI::Broodwar->drawTextScreen(x, y+20, "\x04UNIT NAME");

	size_t reps = prod.size() < 10 ? prod.size() : 10;

	y += 40;
	int yy = y;

	// for each unit in the queue
	for (size_t i(0); i<reps; i++) {

		std::string prefix = "\x07";

		yy += 10;

		BWAPI::UnitType t = prod[i]->getType();
		int started = BWAPI::Broodwar->getFrameCount() - (t.buildTime() - prod[i]->getRemainingBuildTime());

		if (DRAW_UALBERTABOT_DEBUG) BWAPI::Broodwar->drawTextScreen(x, yy, "%s%s", prefix.c_str(), t.getName().c_str());
		if (DRAW_UALBERTABOT_DEBUG) BWAPI::Broodwar->drawTextScreen(x+150, yy, "%s%6d", prefix.c_str(), prod[i]->getRemainingBuildTime());
	}

	queue.drawQueueInformation(x, yy+10, -1);
}

ProductionManager * ProductionManager::instance = NULL;
ProductionManager * ProductionManager::getInstance() {

	// if the instance doesn't exist, create it
	if (!ProductionManager::instance) {
		ProductionManager::instance = new ProductionManager();
	}

	return ProductionManager::instance;
}