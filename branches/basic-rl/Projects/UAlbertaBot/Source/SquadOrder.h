#pragma once

#include "Common.h"

class SquadOrder
{
public:

	enum Type { None, Attack, Defend, SquadOrderTypes };

	Type				type;
	BWAPI::Position		position;
	int					radius;

	SquadOrder() : type(None) {}
	SquadOrder(Type type, BWAPI::Position position, int radius) :
		type(type), position(position), radius(radius) {}

	std::string getOrderString() {

		if (type == SquadOrder::Attack)					{ return "Attack"; } 
		else if (type == SquadOrder::Defend)			{ return "Defend"; } 

		return "Not Formed";
	}
};
