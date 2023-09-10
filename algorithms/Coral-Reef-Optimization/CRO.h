#pragma once
#include <vector>
#include <map>
#include "Job.h"
#include "Coral.h"

class CRO {
public:
	CRO();
	CRO(int numberOfJobs, int numberOfMachines, int numberOfProcesses, int reefSize, int generations, std::string useDefault);
	void run();
private:
	void initializePopulation();
	void sexualReproduction();
	void broadcastSpawning(const CoralPtr& parent1, const CoralPtr& parent2);
	void broodingMutation(const CoralPtr& coral);
	void determineFitnessValue();
	void calculateDominationCounts();
	void larvaSettling(int allowedLarvaeInReef);
	void extremeDepredation();
	void asexualReproduction();
	void depredation();
	void cleanupOldValues();
	void outputOptimalSolution();

	// utility methods
	std::vector<Job> importDefaultSample(std::string fileName = "dataset.txt");
	std::vector<Job> generateJobs(int numberOfJobs, int numberOfMachines, int numberOfProcesses);
	void splitJobs(std::vector<int>& firstGroup, std::vector<int>& secondGroup);
	void printPopulation();
	void printPopulationGrid();
	void printJobs(std::vector<Job> jobs);
	void outputJobs(std::vector<Job> jobs);


	int numberOfJobs_, numberOfMachines_;
	int generations_;
	int reefSize_;
	int numberOfProcesses_;
	std::vector<std::pair<int, int>> currentPopulation_;;
	std::vector<CoralPtr> waterLarvae_;
	const int maxDuration_ = 10;

	int occupationRate_ = 60; // %
	int reproductionFactor_ = 70; // %
	int buddingFactor_ = 10; // %
	int depredationFactor_ = 20; // %
	int depredationProbability_ = 15; // %

	std::vector<Job> jobs_;
	std::vector<std::vector<CoralPtr>> reef_;


	std::string useDefaultSample_ = "0";
};