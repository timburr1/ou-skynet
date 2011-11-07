#ifndef ULTRALISKMANAGER_H_
#define ULTRALISKMANAGER_H_
#include <Common.h>
#include <BWAPI.h>
#include "MicroManager.h"

class UltraliskManager : public MicroManager
{
public:

	UltraliskManager();
	~UltraliskManager() {}
	virtual void executeMicro(const UnitVector & targets);
};

#endif

