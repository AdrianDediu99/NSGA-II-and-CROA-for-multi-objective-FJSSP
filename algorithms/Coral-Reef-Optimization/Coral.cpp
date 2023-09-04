#include <iostream>
#include <random>
#include <algorithm>
#include <string>
#include <ctime>
#include <chrono>

#include "Coral.h"
#include "Job.cpp"

Coral::Coral() {}

Coral::Coral(std::shared_ptr<Coral> coral)
{
	processes_ = coral->processes_;
	machines_ = coral->machines_;

	maxCompletionTime_ = 0;
	totalEquipmentLoad_ = 0;
}

Coral::Coral(std::vector<Job> jobs, int numProcesses, int numMachines, int seedEntropy) 
{
	// Iterate through the jobs and their processes
	for (int jobIndex = 0; jobIndex < jobs.size(); jobIndex++) {
		for (int processIndex = 0; processIndex < jobs[jobIndex].processes.size(); processIndex++) 
		{
			processes_.push_back(jobIndex + 1);
		}
	}

	for (int i = 0; i < numProcesses; i++) 
	{
		machines_.push_back(std::rand() % numMachines + 1); // Random machine ID
	}

	// Shuffle the processes using a random engine
    std::random_device rd;

    // Get a high-resolution time point as part of the seed
    auto now = std::chrono::high_resolution_clock::now();

	// Combine multiple sources of entropy for the seed
    std::size_t seed = rd() ^ now.time_since_epoch().count() ^
                       static_cast<std::size_t>(seedEntropy);

    // Initialize the random engine with the combined seed
    std::mt19937 gen(seed);

	std::shuffle(processes_.begin(), processes_.end(), gen);
    std::shuffle(machines_.begin(), machines_.end(), gen);

	// initialize with 0 maximum completion time
	maxCompletionTime_ = 0;
	// initialize with 0 total equipment load
	totalEquipmentLoad_ = 0;
}

std::string Coral::getGenesAsString()
{
	std::stringstream result;
	std::vector<int> occurrenceVector(*max_element(processes_.begin(), processes_.end()) + 1, 0);

	for (auto process : processes_)
	{
		occurrenceVector[process]++;
		result << std::to_string(process) << "," << std::to_string(occurrenceVector[process]) << " ";
	}
	
	for (auto machine : machines_)
	{
		result << std::to_string(machine) << " ";
	}

	result << std::to_string(maxCompletionTime_) << " ";
	result << std::to_string(totalEquipmentLoad_) << " ";

	std::string output(result.str());
	output.pop_back();

	return output;
}

void Coral::mutate(const int& numberOfMachines)
{
	std::random_device rd;
	// Get a high-resolution time point as part of the seed
    auto now = std::chrono::high_resolution_clock::now();

	// Combine multiple sources of entropy for the seed
    std::size_t seed = rd() ^ now.time_since_epoch().count() ^ static_cast<std::size_t>(numberOfMachines);
	std::mt19937 gen(seed);

	int randomMachineIndex = gen() % processes_.size(); // m[3] = 2
	int mutatedMachineId = machines_[randomMachineIndex];

	while (mutatedMachineId == machines_[randomMachineIndex]) 
	{
		mutatedMachineId = gen() % numberOfMachines + 1;
	}
	
	machines_[randomMachineIndex] = mutatedMachineId;
}

bool Coral::dominates(const std::shared_ptr<Coral>& coral)
{
	if((maxCompletionTime_ <= coral->maxCompletionTime_ &&
		totalEquipmentLoad_ <= coral->totalEquipmentLoad_) &&
		(maxCompletionTime_ < coral->maxCompletionTime_ ||
		totalEquipmentLoad_ < coral->totalEquipmentLoad_))
	{
		return true;
	}
	
	return false;
}

std::ostream& operator<<(std::ostream& os, const Coral& coral) 
{
    os << "Coral Info:" << std::endl;
    os << "Processes: ";
    for (const int& process : coral.processes_) 
	{
        os << process << " ";
    }
    os << std::endl;

    os << "Machines: ";
    for (const int& machine : coral.machines_) 
	{
        os << machine << " ";
    }
    os << std::endl;

    os << "Max Completion Time: " << coral.maxCompletionTime_ << std::endl;
    os << "Total Equipment Load: " << coral.totalEquipmentLoad_ << std::endl;

    return os;
}
