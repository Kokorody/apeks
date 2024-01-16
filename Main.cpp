#include "includes.cpp"

int main() {
    //load config
    ConfigLoader* cl = new ConfigLoader();
    int prevRadarSize;
    bool newRadarSize;
    int prevRadarPos;
    int secCounter;

    //basic checks
    if (getuid()) { std::cout << "[ ERROR ] Run program using as root (sudo)\n" << std::flush; return -1; }
    if (mem::GetPID() == 0) { std::cout << "[ ERROR ] Game not found (using r5apex.exe). Open the game first!\n" << std::flush; return -1; }

    //create basic objects
    XDisplay* display = new XDisplay();
    Level* level = new Level();
    LocalPlayer* localPlayer = new LocalPlayer();
    std::vector<Player*>* humanPlayers = new std::vector<Player*>;
    std::vector<Player*>* dummyPlayers = new std::vector<Player*>;
    std::vector<Player*>* players = new std::vector<Player*>;

    //fill in slots for players, dummies and items
    for (int i = 0; i < 70; i++) humanPlayers->push_back(new Player(i, localPlayer));
    for (int i = 0; i < 15000; i++) dummyPlayers->push_back(new Player(i, localPlayer));

    //create features     
    NoRecoil* noRecoil = new NoRecoil(cl, display, level, localPlayer);
    AimBot* aimBot = new AimBot(cl, display, level, localPlayer, players);
    TriggerBot* triggerBot = new TriggerBot(cl, display, level, localPlayer, players);
    Sense* sense = new Sense(cl, display, level, localPlayer, players);
    Radar* radar = new Radar(cl, display, level, localPlayer, players);

    //begin main loop
    secCounter = -10;
    for (int counter = 0; ; counter = ((counter >= 1000) ? 0 : counter + 1)) {
        try {
            //record time so we know how long a single loop iteration takes
            long long startTime = util::currentEpochMillis();

            // will attempt to reload config if there have been any updates to it
            if (counter % 20 == 0) {
            prevRadarSize = cl->RADAR_SIZE;
            prevRadarPos = cl->RADAR_POSITION;
            cl->reloadFile();
            if (prevRadarSize != cl->RADAR_SIZE || prevRadarPos != cl->RADAR_POSITION) {
                //printf("Resizing and Moving Window to %d...\n", cl->RADAR_POSITION);
                radar->resizeWindow();
                radar->moveWindow();
            } 
	    }

            //read level and make sure it is playable
            level->readFromMemory();
            if (!level->playable) {
                //printf("[ INFO  ] Not in game! Sleeping 10 seconds...\n");
                secCounter = secCounter + 10;
                std::cout << "\r" 
			<<  "[ INFO  ] Not in game! Sleeping ("
			<< std::setw(3)
			<< secCounter
			<< " seconds)"
			<< std::flush;
                std::this_thread::sleep_for(std::chrono::milliseconds(10000));
                continue;
            }

            //read localPlayer and make sure he is valid
            localPlayer->readFromMemory();
            //if (!localPlayer->isValid()) throw std::invalid_argument("LocalPlayer invalid!");
            if (!localPlayer->isValid()) {
            	//printf("[ INFO  ] Local Player Invalid! Sleeping 10 seconds...\n");
            	secCounter = secCounter + 10;
            	std::cout << "\r" 
			<<  "[ INFO  ] Local Player Invalid! Sleeping ("
			<< std::setw(3)
			<< secCounter
			<< " seconds)"
			<< std::flush;
                std::this_thread::sleep_for(std::chrono::milliseconds(10000));
                continue;
            } 

            //read players
            players->clear();
            if (level->trainingArea)
                for (int i = 0; i < dummyPlayers->size(); i++) {
                    Player* p = dummyPlayers->at(i);
                    p->readFromMemory();
                    if (p->isValid()) players->push_back(p);
                }
            else
                for (int i = 0; i < humanPlayers->size(); i++) {
                    Player* p = humanPlayers->at(i);
                    p->readFromMemory();
                    if (p->isValid()) players->push_back(p);
                }

            //run features       
            noRecoil->controlWeapon(counter);
            triggerBot->shootAtEnemy();
            aimBot->aimAssist(counter);
            sense->modifyHighlights();
            sense->glowPlayers();
            radar->processEvents(counter);
            radar->repaint();

            //check how fast we completed all the processing and if we still have time left to sleep
            int processingTime = static_cast<int>(util::currentEpochMillis() - startTime);
            int goalSleepTime = 6; // 16.67ms=60HZ | 6.97ms=144HZ
            int timeLeftToSleep = std::max(0, goalSleepTime - processingTime);
            std::this_thread::sleep_for(std::chrono::milliseconds(timeLeftToSleep));

            //print loop info every now and then
            if (counter % 500 == 0)
//		printf("[ INFO  ] Loop[%04d] OK | ProcTime: %02dms | TimeToSleep: %02dms | MAP: %s |\n", 
//		counter, processingTime, timeLeftToSleep, level->name.c_str());
		std::cout << "\r" 
			<<  "[ INFO  ] Loop[" 
			<< std::setw(4)
			<< counter
			<< "] OK | ProcTime: "
			<< std::setw(2)
			<< processingTime
			<< "ms | TimeToSleep: "
			<< std::setw(2)
			<< timeLeftToSleep
			<< "ms | MAP: "
			<< level->name.c_str()
			<< "|"
			<< std::flush;
        }
        catch (std::invalid_argument& e) {
            printf("[ ERROR ] %s Trying again in 10sec! \n", e.what());
        }
        catch (...) {
            printf("[UNKNOWN] Trying again in 10sec! \n");
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
    }

}

