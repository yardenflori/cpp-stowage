//
// Created by Yarden on 03/05/2020.
//


#include "Simulation.h"
#include "../common/utils/Registrar.h"
#include <functional>
#include <memory>
#include "../interfaces/WeightBalanceCalculator.h"
#include "../algorithm/_206039984_a.h"

Simulation::Simulation(const string &travelsPath, const string &algorithmPath, const string &outputPath): travelsPath(travelsPath), algorithmPath(algorithmPath), outputPath(outputPath)
{
#ifndef RUNNING_ON_NOVA
    Registrar::getInstance().factoryVec.emplace_back([](){return std::make_unique<_206039984_a>();});
    Registrar::getInstance().addName("_206039984_a");
    for(unsigned long long i = 0 ; i < Registrar::getInstance().factoryVec.size(); i++)
    {
        algorithmsResults[Registrar::getInstance().names[i]] = AlgorithmResults(Registrar::getInstance().names[i]);
    }
#endif

    if(algorithmPath == "")
    {
        //load algorithms from cwd
    } else
    {
        //load algorithms from algorithmPath
    }

    if(outputPath!="")
    {
        this->outputPath = outputPath;
        isOutputPathSupplied = true;
    }
    else
    {
        this->outputPath = std::filesystem::current_path().string();
    }

    std::filesystem::create_directory(outputPath);

    // TODO -------- do it in loop ---------After going to nova
    //Registrar::getInstance().loadSO("<path of x>");
    // TODO ------------- end ---------------


}

int Simulation::simulateAllTravelsWithAllAlgorithms()
{
    if (checkTravelsPath(travelsPath)!=0)
    {
        return 1;
    }

    vector <string> travelNames;

    for(auto& travel: std::filesystem::directory_iterator(travelsPath))
    {
        string pathOfCurrentTravel = travel.path().string();
        travelNames.push_back(travel.path().stem().string());
        simulateOneTravelWithAllAlgorithms(pathOfCurrentTravel);
    }

    if(!isOutputPathSupplied)
    {
        outputPath = travelsPath;
    }

    //TODO: sort algorithmResults
    string resultOutputPath = outputPath + "/simulation.results.csv"; //TODO: remove csv;
    IO::writeResultsOfsimulation(resultOutputPath, travelNames, algorithmsResults);

    return 0;
}

void Simulation::simulateOneTravelWithAllAlgorithms(const string &travelPath) {

    cout << "Started simulate " << travelPath << endl;
    Errors errors;

    const std::filesystem::path path = travelPath;
    if(is_directory(path) == 0) //travelPath isn't a directory
    {
        cout << "Travel error: " << travelPath << " is not a directory" << endl;
        return;
    }

    if(isOutputPathSupplied == false)
    {
        this->outputPath = travelPath;
    }

    std::ofstream file { travelPath + "/empty_containers_awaiting_at_port_file.txt" };

    Registrar &registrar = Registrar::getInstance();
    for(size_t i = 0 ; i < registrar.factoryVec.size() ; i++ )
    {
        string a = path.stem().string();
        if(errors.hasTravelError() == 0)
        {
            unique_ptr<AbstractAlgorithm> algorithm = registrar.factoryVec[i]();
            errors = simulateOneTravelWithOneAlgorithm(travelPath, std::move(algorithm), registrar.names[i]);
        }
        else
        {
            //TODO: write to error file that travel error occurred
            algorithmsResults[registrar.names[i]].addTravelResult(path.stem().string(), -1);
        }
    }

    cout << "Finished simulate " << travelPath << endl;
}

Errors Simulation::simulateOneTravelWithOneAlgorithm(const string &travelPath, unique_ptr<AbstractAlgorithm> algorithm, const string &algorithmName) {

    std::cout <<"===========================================================" << std::endl;
    std::cout <<"started simulate " << algorithmName << " on " << travelPath << std::endl;
    std::cout <<"===========================================================" << std::endl;

    Errors errors;
    int totalOperations = 0;
    Route route;
    vector<string> ports;
    Ship ship;
    CraneManagement craneManagement;
    CraneManagement::CraneManagementAnswer craneManagementAnswer;
    WeightBalanceCalculator weightBalanceCalculator;
    std::map<string,int> indexOfVisitAtPort;
    std::map<string,int> totalNumbersOfVisitingPort;
    const std::filesystem::path pathOfTravel = travelPath;
    const string travelName = pathOfTravel.stem().string();
    const std::filesystem::path pathOfOutputFilesForAlgorithmAndTravel = outputPath + "/output_of_" + algorithmName + "_" + travelName; //TODO: replace algorithm name
    string outputPathOfErrorsFile = pathOfOutputFilesForAlgorithmAndTravel.string() + "/errors_of_" + algorithmName + "_" + travelName + ".csv";
    std::filesystem::create_directory(pathOfOutputFilesForAlgorithmAndTravel);

    errors.addError(algorithm->setWeightBalanceCalculator(weightBalanceCalculator));

    if(errors.hasTravelError())
    {
        IO::writeErrorsOfTravelAndAlgorithm(errors, outputPathOfErrorsFile);
        algorithmsResults[algorithmName].addTravelResult(travelName, -1);
        return errors;
    }

    cout << "Reading route" << endl;
    errors.addError(IO::readShipRoute(travelPath + "/route.txt", route));
    if(errors.hasTravelError())
    {
        IO::writeErrorsOfTravelAndAlgorithm(errors, outputPathOfErrorsFile);
        algorithmsResults[algorithmName].addTravelResult(travelName, -1);
        return errors;
    }
    ports = route.getPorts();
    for(string port : ports)
    {
        totalNumbersOfVisitingPort[port]++;
    }
    errors.addError(algorithm->readShipRoute(travelPath + "/route.txt"));
    if(errors.hasTravelError())
    {
        IO::writeErrorsOfTravelAndAlgorithm(errors, outputPathOfErrorsFile);
        algorithmsResults[algorithmName].addTravelResult(travelName, -1);
        return errors;
    }

    cout << "Reading ship plan" << endl;
    errors.addError(IO::readShipPlan(travelPath + "/ship_plan.txt", ship.getShipPlan()));
    if(errors.hasTravelError())
    {
        IO::writeErrorsOfTravelAndAlgorithm(errors, outputPathOfErrorsFile);
        algorithmsResults[algorithmName].addTravelResult(travelName, -1);
        return errors;
    }
    errors.addError(algorithm->readShipPlan(travelPath + "/ship_plan.txt"));
    if(errors.hasTravelError())
    {
        IO::writeErrorsOfTravelAndAlgorithm(errors, outputPathOfErrorsFile);
        algorithmsResults[algorithmName].addTravelResult(travelName, -1);
        return errors;
    }

    for (size_t i = 0; i < ports.size(); i++)
    {
        vector<Container> containers;
        vector<Container> badContainers;
        std::cout << "----------------" <<"Ship Arrived to port " << ports[i] << "----------------" << std::endl;
        const std::filesystem::path pathOfContainersAwaitingAtPortFile = travelPath + "/" + ports[i] + "_" + std::to_string(indexOfVisitAtPort[ports[i]]+1) + ".cargo_data.txt"; //TODO: remove .txt in linux

        if(std::filesystem::exists(pathOfContainersAwaitingAtPortFile) == 0)
        {
            std::cout << "No containers awaiting at port input file for " << ports[i] << " for visiting number " << indexOfVisitAtPort[ports[i]] + 1 << std::endl;
            errors.addError(algorithm->getInstructionsForCargo(travelPath + "/" + "empty_containers_awaiting_at_port_file.txt", pathOfOutputFilesForAlgorithmAndTravel.string() + "/" + ports[i] + "_" + algorithmName + "_" + std::to_string(indexOfVisitAtPort[ports[i]]+1) + ".txt"));
            if(errors.hasTravelError())
            {
                IO::writeErrorsOfTravelAndAlgorithm(errors, outputPathOfErrorsFile);
                algorithmsResults[algorithmName].addTravelResult(travelName, -1);
                return errors;
            }
        }
        else
        {
            errors.addError(IO::readContainerAwaitingAtPortFile(pathOfContainersAwaitingAtPortFile.string(),ship,containers,badContainers));
            if(errors.hasTravelError())
            {
                IO::writeErrorsOfTravelAndAlgorithm(errors, outputPathOfErrorsFile);
                algorithmsResults[algorithmName].addTravelResult(travelName, -1);
                return errors;
            }
            errors.addError(algorithm->getInstructionsForCargo(pathOfContainersAwaitingAtPortFile.string(), pathOfOutputFilesForAlgorithmAndTravel.string() + "/" + ports[i] + "_" + algorithmName + "_" + std::to_string(indexOfVisitAtPort[ports[i]]+1) + ".txt"));
            if(errors.hasTravelError())
            {
                IO::writeErrorsOfTravelAndAlgorithm(errors, outputPathOfErrorsFile);
                algorithmsResults[algorithmName].addTravelResult(travelName, -1);
                return errors;
            }
        }

        craneManagementAnswer = craneManagement.readAndExecuteInstructions(ship, pathOfOutputFilesForAlgorithmAndTravel.string() + "/" + ports[i] + "_" + algorithmName + "_" + std::to_string(indexOfVisitAtPort[ports[i]]+1) + ".txt"); //TODO: send reference to parameter "errors" to the crane for updating errors?
        totalOperations += craneManagementAnswer.numOfOperations;
        checkForErrorsAfterPort(ship, ports[i], craneManagementAnswer, route,containers, errors);
        indexOfVisitAtPort[ports[i]]++;
        std::cout << "----------------" <<"Ship left port " << ports[i] << "----------------" << std::endl;
        route.incrementCurrentPort();

    }

    algorithmsResults[algorithmName].addTravelResult(travelName, totalOperations);
    IO::writeErrorsOfTravelAndAlgorithm(errors, outputPathOfErrorsFile);

    std::cout <<"===========================================================" << std::endl;
    std::cout <<"finished simulate " << algorithmName << " on " << travelPath << std::endl;
    std::cout <<"===========================================================" << std::endl;

    return errors;

}

void Simulation::checkForErrorsAfterPort(Ship& ship, const string &port, CraneManagement::CraneManagementAnswer& answer,Route& route, vector<Container>& containers, Errors &errors) {

        // --------------- check all containers supposed to be unloaded were unloaded ---------------
        if (ship.portToContainers[port].size() > 0)
        {
            errors.addError(Error::CONTAINERS_SHOULD_BE_UNLOADED_SKIPPED_BY_THE_ALGORITHM);
            cout << "Not all of the containers with of this port destination were unloaded" << std::endl;
        }

    // --------------- check all containers supposed to be loaded were loaded or rejected ---------------
    bool inRejected, inLoaded;
        for (auto& container : containers){
            for (string& id : answer.changedContainers[Action::REJECT]){
                if (!id.compare(container.getId())){
                    inRejected = true;
                    break;
                }
            }
            for (string& id : answer.changedContainers[Action::LOAD]) {
                if (!id.compare(container.getId())) {
                    inLoaded = true;
                    break;
                }
            }
            if (!inRejected && !inLoaded){
                errors.addError(Error::CONTAINERS_ON_PORT_IGNORED_BY_THE_ALGORITHM);
                cout << "Not all of the containers of this port were loaded (or rejected)" << std::endl;
            }
        }

    // --------------- check all rejected containers were rejected from good reason ---------------
    if (answer.changedContainers.count(Action::REJECT) > 0)
        {
            for (auto& rejected : answer.changedContainers[Action::REJECT])
            {
                if (route.portInNextStops(ship.containerIdToDestination(rejected))) {
                    for (auto &loaded : answer.changedContainers[Action::LOAD]) {
                        if (route.nextStopForPort(ship.containerIdToDestination(rejected)) <
                            route.nextStopForPort(ship.containerIdToDestination(loaded))) {
                            cout << "Rejected container " + rejected + " with higher priority" << std::endl;
                            errors.addError(Error::CONTAINER_INCORRECTLY_REJECTED);
                        }
                    }
                } else {
                    // good ignore
                }
            }
        }

    // --------------- check if ship is empty in end of travel ---------------
    if (route.inLastStop()) {
        int amountOfContainers = ship.getAmountOfContainers() > 0;
        if (amountOfContainers > 0) {
            cout << "Ship arrived to last port in route but there are still " + std::to_string(amountOfContainers) +
                         " containers on the ship";
            errors.addError(Error::SHIP_HAS_CONTAINERS_AT_THE_END_OF_THE_ROUTE);
        }
    }
}

bool Simulation::checkIfCloserContainerWasLoaded(Ship &ship,  CraneManagement::CraneManagementAnswer &answer,
                                                 Route &route, Container &container)  {
    bool closerContainerExists = false;
    for (auto &loaded : answer.changedContainers[Action::LOAD]) {
        if (route.nextStopForPort(ship.containerIdToDestination(container.getId())) <
            route.nextStopForPort(ship.containerIdToDestination(loaded))) {
            closerContainerExists = true;
        }
    }
    return closerContainerExists;
}

int Simulation::checkTravelsPath(const string &travelsPathToCheck) {
    if (travelsPathToCheck == "")
    {
        string error = "Fatal error: missing travel_path argument!";
        cout << error << endl;
        IO::writeToFile(this->outputPath + "/simulation.errors.csv", error);
        return 1;
    }

    if(std::filesystem::exists(travelsPathToCheck) == 0)
    {
        string error = "Fatal error: travel_path argument isn't valid!";
        cout << error << endl;
        string a = std::filesystem::current_path().string();
        IO::writeToFile(this->outputPath + "/simulation.errors.csv", error);
        return 1;
    }

    if(this->isOutputPathSupplied == 0)
    {
        this->outputPath = travelsPath;
    }
    return 0;
}





