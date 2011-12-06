/* 
 +----------------------------------------------------------------------+
 | UAlbertaBot                                                          |
 +----------------------------------------------------------------------+
 | University of Alberta - AIIDE 2011 StarCraft Competition             |
 +----------------------------------------------------------------------+
 |                                                                      |
 +----------------------------------------------------------------------+
 | Authors: David Churchill <dave.churchill@gmail.com>                  |
 |          Jason Lorenz                                                |
 |          Sterling Oersten                                            |
 +----------------------------------------------------------------------+
*/

#include "Common.h"
#include "UAlbertaBotModule.h"

BWAPI::AIModule * __NewAIModule()
{
	return new UAlbertaBotModule();
}

UAlbertaBotModule::UAlbertaBotModule()  {}
UAlbertaBotModule::~UAlbertaBotModule() {}

void UAlbertaBotModule::onStart()
{
	BWAPI::Broodwar->setLocalSpeed(0);

	BWAPI::Broodwar->enableFlag(BWAPI::Flag::UserInput);
	BWAPI::Broodwar->enableFlag(BWAPI::Flag::CompleteMapInformation);

	BWTA::readMap();
	BWTA::analyze();
}

void UAlbertaBotModule::onEnd(bool isWinner)
{
	ProductionManager::getInstance()->onEnd();

	//append average final score to file
	std::fstream WLFile;
	WLFile.open("C:\\NN_weights\\WL.txt", std::fstream::app);
	
	assert(WLFile.is_open());

	WLFile << isWinner;
	WLFile <<"\n";

	WLFile.close();
}

void UAlbertaBotModule::onFrame()
{
	if (BWAPI::Broodwar->isReplay()) return;

	// draw game time
	if (DRAW_UALBERTABOT_DEBUG) BWAPI::Broodwar->drawTextScreen(5, 2, "Frame: %7d\nTime: %4dm %3ds", BWAPI::Broodwar->getFrameCount(), BWAPI::Broodwar->getFrameCount()/(24*60), (BWAPI::Broodwar->getFrameCount()/24)%60);

	// draw position of mouse cursor
	//int mouseX = BWAPI::Broodwar->getMousePosition().x() + BWAPI::Broodwar->getScreenPosition().x();
	//int mouseY = BWAPI::Broodwar->getMousePosition().y() + BWAPI::Broodwar->getScreenPosition().y();
	//if (DRAW_UALBERTABOT_DEBUG) BWAPI::Broodwar->drawTextMap(mouseX + 20, mouseY, " %d %d", mouseX, mouseY);

	gameCommander.update();
}

void UAlbertaBotModule::onUnitDestroy(BWAPI::Unit * unit)
{
	gameCommander.onUnitDestroy(unit);
}

void UAlbertaBotModule::onUnitMorph(BWAPI::Unit * unit)
{
	gameCommander.onUnitMorph(unit);
}

void UAlbertaBotModule::onSendText(std::string text) 
{ 
	//BWAPI::Broodwar->sendText(text.c_str());
	BWAPI::Broodwar->setLocalSpeed(atoi(text.c_str()));
	//gameCommander.onSendText(text); 
}

void UAlbertaBotModule::onUnitCreate(BWAPI::Unit * unit)
{ 
	gameCommander.onUnitCreate(unit); 
}

void UAlbertaBotModule::onUnitShow(BWAPI::Unit * unit)
{ 
	gameCommander.onUnitShow(unit); 
}

void UAlbertaBotModule::onUnitHide(BWAPI::Unit * unit)
{ 
	gameCommander.onUnitHide(unit); 
}

void UAlbertaBotModule::onUnitRenegade(BWAPI::Unit * unit)
{ 
	gameCommander.onUnitRenegade(unit); 
}

void UAlbertaBotModule::moveSelectedUnit(int x, int y)
{
	BOOST_FOREACH(BWAPI::Unit * unit, BWAPI::Broodwar->getSelectedUnits())
	{
		unit->move(BWAPI::Position(unit->getPosition().x()+x, unit->getPosition().y()+y));
	}
}

void UAlbertaBotModule::parseMap() {

	BWAPI::Broodwar->printf("Parsing Map Information");
	std::ofstream mapFile;
	std::string file = "c:\\scmaps\\" + BWAPI::Broodwar->mapName() + ".txt";
	mapFile.open(file.c_str());

	mapFile << BWAPI::Broodwar->mapWidth()*4 << "\n";
	mapFile << BWAPI::Broodwar->mapHeight()*4 << "\n";

	for (int j=0; j<BWAPI::Broodwar->mapHeight()*4; j++) {
		for (int i=0; i<BWAPI::Broodwar->mapWidth()*4; i++) {

			if (BWAPI::Broodwar->isWalkable(i,j)) {
				mapFile << "0";
			} else {
				mapFile << "1";
			}
		}

		mapFile << "\n";
	}
	
	
	BWAPI::Broodwar->printf(file.c_str());

	mapFile.close();
}
