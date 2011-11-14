#ifndef UALBERTABOTMODULE_H
#define UALBERTABOTMODULE_H
#include <BWAPI.h>
#include "GameCommander.h"
#include <iostream>
#include <fstream>
#include "Logger.h"
#include "MapTools.h"
#include "HardCodedInfo.h"

#include "NeuralNet.h"

class UAlbertaBotModule : public BWAPI::AIModule
{
	GameCommander	gameCommander;

public:
			
	UAlbertaBotModule();
	~UAlbertaBotModule();

	void	onStart();
	void	onFrame();
	void	onEnd(bool isWinner);
	void	onUnitDestroy(BWAPI::Unit * unit);
	void	onUnitMorph(BWAPI::Unit * unit);
	void	onPlayerLeft(BWAPI::Player * player)	{}
	void	onNukeDetect(BWAPI::Position target)	{}
	void	onSendText(std::string text);
	void	onUnitCreate(BWAPI::Unit * unit);
	void	onUnitShow(BWAPI::Unit * unit);
	void	onUnitHide(BWAPI::Unit * unit);
	void	onUnitRenegade(BWAPI::Unit * unit);

	void parseMap();
	void moveSelectedUnit(int x, int y);

private:
	std::vector<NeuralNet> nets;
};

#endif
