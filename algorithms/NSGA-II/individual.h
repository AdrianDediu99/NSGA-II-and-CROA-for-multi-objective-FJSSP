#pragma once
#include <string>
#include <memory>
#include <utility>

#include "Job.h"

struct Individual
{
public:
	Individual();
	Individual(std::shared_ptr<Individual> indidual);
	Individual(std::vector<Job> jobs, int numProcesses, int numMachines, int seedEntropy);
	std::string getGenesAsString();
	void mutate(const int& numberOfMachines);
	bool dominates(const std::shared_ptr<Individual>& indidual);

	std::vector<int> processes_; // <jobId>
	std::vector<int> machines_;
	int maxCompletionTime_ = 0;
	int totalEquipmentLoad_ = 0;

	int  dominationCount_ = 0;
	std::vector<int> dominatedPoints_;

	int frontLevel_ = 0;
	float crowdingDistance_ = 0;

	bool isChild = false;
};

using IndividualPtr = std::shared_ptr<Individual>;
