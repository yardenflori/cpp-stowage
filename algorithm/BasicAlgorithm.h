//
// Created by Oz Zafar on 03/05/2020.
//

#ifndef CPP_STOWAGE_BASICALGORITHM_H
#define CPP_STOWAGE_BASICALGORITHM_H

#include "../interfaces/AbstractAlgorithm.h"
#include "../common/objects/Ship.h"
#include "../common/objects/Route.h"
#include "../interfaces/WeightBalanceCalculator.h"
#include "../common/utils/Error.h"
#include <string>
#include "../common/objects/ContainersPosition.h"
#include "../common/utils/IO.h"
#include "../common/utils/Errors.h"
#include <fstream>
#include <iostream>

using std::string;

class BasicAlgorithm : public AbstractAlgorithm {

protected:

    WeightBalanceCalculator calculator;
    Ship ship;
    Route route;
    map<string, vector<int>> portToIndexesInRoute;

public:

    // class functions
    int readContainerAwaitingAtPortFile(const string &path,vector<Container>& waitingContainers, vector<Container>& badContainers);
    void writeOperation(const std::string &filename, AbstractAlgorithm::Action op, const string &containerId, int floor, int x, int y);  // writes single operation to output file
    void writeOperation(const std::string &filename, AbstractAlgorithm::Action op, const string &containerId, int floor, int x, int y,int floorNew, int xNew, int yNew);  // writes single operation to output file
    Ship getShip();
    Route getShipRoute();

    // override functions
    int readShipPlan(const std::string& full_path_and_file_name) override;
    int readShipRoute(const std::string& full_path_and_file_name) override;
    int setWeightBalanceCalculator(WeightBalanceCalculator& calculator) override;
    int getInstructionsForCargo(const std::string& input_full_path_and_file_name,const std::string& output_full_path_and_file_name) override = 0 ;


};

#endif //CPP_STOWAGE_BASICALGORITHM_H
