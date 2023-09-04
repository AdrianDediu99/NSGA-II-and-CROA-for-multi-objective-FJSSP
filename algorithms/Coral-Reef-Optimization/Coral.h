#pragma once
#include <string>
#include <memory>
#include <utility>


#include "Job.h"

class Coral
{
public:
	Coral();
	Coral(std::shared_ptr<Coral> coral);
	Coral(std::vector<Job> jobs, int numProcesses, int numMachines, int seedEntropy);
	std::string getGenesAsString();
	void mutate(const int& numberOfMachines);
	bool dominates(const std::shared_ptr<Coral>& coral);

	std::vector<int> processes_; // <jobId>
	std::vector<int> machines_;
	int maxCompletionTime_ = 0;
	int totalEquipmentLoad_ = 0;

	int  dominationCount_ = 0;
	std::vector<std::pair<int,int>> dominatedPoints_;

	int frontLevel_ = 0;
	float crowdingDistance_ = 0;

	friend std::ostream& operator<<(std::ostream& os, const Coral& coral);
};

using CoralPtr = std::shared_ptr<Coral>;
