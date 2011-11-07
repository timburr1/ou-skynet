#ifndef STARCRAFT_ACTION_H
#define STARCRAFT_ACTION_H

///////////////////////////////////////////////////////////////////////////////
//
// StarcraftAction
//
// StarcraftActions are to ONLY be constructed by StarcraftData
//
// The int id passed in is its location within the ActionMap vector
//
// If you wish to get a StarcraftAction, use ActionMap::getStarcraftAction()
//
///////////////////////////////////////////////////////////////////////////////
#include "BWAPI.h"
#include "ActionSet.hpp"
#include <assert.h>
#include <sstream>

typedef unsigned char Action;

class StarcraftAction { 

public:

	// types of actions
	enum ActionType {UnitType, UpgradeType, TechType, Error};

private:

	ActionType type;						// the enum type of action this is
	BWAPI::Race race;						// the race of this action
	BWAPI::UnitType unit;					// holds UnitType, if it's a unit
	BWAPI::UpgradeType upgrade;				// holds UpgradeType, if it's an upgrade
	BWAPI::TechType tech;					// holds TechType, if it's a tech

	int		actionID,						// unique identifier to this action
		 	mineralPriceVal, 				// mineral price of the action
			gasPriceVal, 					// gas price of the action
			supplyRequiredVal, 				// supply price of the action
			supplyProvidedVal,				// supply created by the action
			buildTimeVal,					// build time of the action
			repeat,							// how many times to repeat this action (meta action)
			numberProduced;					// the number of units produced
			
	std::string name;						// name of the action
	std::string metaName;					// meta name of this action, adds 'repeat' to the string

	ActionSet pre;							// prerequisites of the action
	
	
	bool 	building,						// is this a building
			worker,							// is this a worker
			refinery,						// is this a refinery
			resourceDepot,					// is this a resource depot
			supplyProvider,					// is this a supply provider
			canProduceBool,
			whatBuildsIsBuildingBool,		// is what builds this action a building?
			whatBuildsIsLarvaBool;
	
	BWAPI::UnitType whatBuildsUnitType;		// the unit type that builds this
		
	Action whatBuildsActionUnit;			// the action that builds this 

public:

	// default constructor, should never be called
	StarcraftAction() : type(Error) { assert(false); }

	// UnitType constructor
	StarcraftAction(BWAPI::UnitType t, int id) : 
		type(UnitType), 
		unit(t), 
		actionID(id), 
		mineralPriceVal(t.mineralPrice()), 
		gasPriceVal(t.gasPrice()), 
		supplyRequiredVal(t.supplyRequired()), 
		supplyProvidedVal(t.supplyProvided()), 
		buildTimeVal(t.buildTime()), 
		repeat(1), 
		numberProduced(1),
		name(t.getName()), 
		metaName(t.getName()), 
		building(t.isBuilding()),
		worker(t.isWorker()), 
		refinery(t.isRefinery()), 
		resourceDepot(t.isResourceDepot()), 
		supplyProvider(t.supplyProvided() > 0 && !t.isResourceDepot()), 
		canProduceBool(t.isBuilding() && t.canProduce()),
		whatBuildsUnitType(t.whatBuilds().first)
	{
		if (t == BWAPI::UnitTypes::Zerg_Zergling || t == BWAPI::UnitTypes::Zerg_Scourge)
		{
			numberProduced = 2;
		}
		
		std::stringstream ss("");
		ss << repeat << " " << name;
		metaName = ss.str();
	}

	// UpgradeType action
	StarcraftAction(BWAPI::UpgradeType t, int id) : 
		type(UpgradeType), 
		upgrade(t), 
		actionID(id), 
		mineralPriceVal(t.mineralPrice()), 
		gasPriceVal(t.gasPrice()), 
		supplyRequiredVal(0),  
		supplyProvidedVal(0),
		buildTimeVal(t.upgradeTime()), 
		repeat(1), 
		numberProduced(1), 
		name(t.getName()), 
		metaName(t.getName()), 
		building(false),
		worker(false), 
		refinery(false), 
		resourceDepot(false), 
		supplyProvider(false), 
		canProduceBool(false),
		whatBuildsUnitType(t.whatUpgrades())
	{}

	// TechType action
	StarcraftAction(BWAPI::TechType t, int id) : 
		type(TechType), 
		tech(t), 
		actionID(id), 
		mineralPriceVal(t.mineralPrice()), 
		gasPriceVal(t.gasPrice()), 
		supplyRequiredVal(0), 
		supplyProvidedVal(0),
		buildTimeVal(t.researchTime()), 
		repeat(1), 
		numberProduced(1), 
		name(t.getName()), 
		metaName(t.getName()), 
		building(false),
		worker(false), 
		refinery(false), 
		resourceDepot(false), 
		supplyProvider(false), 
		canProduceBool(false),
		whatBuildsUnitType(t.whatResearches())
	{}

	bool operator==(const StarcraftAction & a) const { return actionID == a.actionID; }
	bool operator< (const StarcraftAction & a) const { return actionID < a.actionID; }

	BWAPI::UnitType		getUnitType()			const	{ return unit; }
	BWAPI::UnitType		whatBuilds() 			const	{ return whatBuildsUnitType; }
	BWAPI::UpgradeType	getUpgradeType()		const	{ return upgrade; }
	BWAPI::TechType		getTechType()			const	{ return tech; }
	
	Action				whatBuildsAction()		const	{ return whatBuildsActionUnit; }	
	ActionSet 			getPrerequisites() 		const	{ return pre; }
	ActionType 			getType() 				const	{ return type; }
	
	std::string 		getName() 				const	{ return name; }
	std::string 		getMetaName() 			const	{ return metaName; }
	
	int 				buildTime() 			const	{ return buildTimeVal; }
	int					mineralPrice()			const	{ return mineralPriceVal; }
	int					mineralPriceScaled()	const	{ return mineralPriceVal * 100; }
	int					gasPrice()				const	{ return gasPriceVal; }
	int					gasPriceScaled()		const	{ return gasPriceVal * 100; }
	int 				supplyRequired() 		const	{ return supplyRequiredVal; }
	int					supplyProvided() 		const	{ return supplyProvidedVal; }
	int 				numProduced() 			const	{ return numberProduced; }
	int					repetitions()			const	{ return repeat; }

	bool				isRefinery() 			const	{ return refinery; }
	bool				isWorker() 				const	{ return worker;   }
	bool				isBuilding()			const	{ return building; }
	bool				isResourceDepot()		const	{ return resourceDepot; }
	bool				isSupplyProvider()		const	{ return supplyProvider; }
	bool				isUnit()				const	{ return type == UnitType; }
	bool				isTech()				const	{ return type == TechType; }
	bool				isUpgrade()				const	{ return type == UpgradeType; }
	
	bool				whatBuildsIsBuilding() 	const	{ return whatBuildsIsBuildingBool; }
	bool				whatBuildsIsLarva() 	const	{ return whatBuildsIsLarvaBool; }
	bool				canProduce()			const	{ return canProduceBool; }
		
	void 				setPrerequisites(const ActionSet a) 	{ pre = a; }
	void 				setRepetitions(const int r) 			{ repeat = r; }
	
	void setWhatBuildsAction(const Action a, const bool wbib, const bool wbil)
	{
		whatBuildsActionUnit = a;
		whatBuildsIsBuildingBool = wbib;
		whatBuildsIsLarvaBool = wbil;
	}
	
};


#endif
