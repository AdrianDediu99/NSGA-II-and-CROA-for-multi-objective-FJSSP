#pragma once
#include <vector>
#include <map>
#include "Job.h"
#include "individual.h"

class Nsga {
public:
	Nsga();
	Nsga(int numberOfJobs, int numberOfMachines, int itterations, int sampleSize, int numberOfProcesses, std::string useDefault);

	void run();

private:
	void initalizePopulation();
	void determineFitnessValue();
	void nonDominatedSortingAndCrowdingDegree();
	void competitionSelection();
	void crossoverAndMutation();
	void elitistRetention(int interation);
	void cleanupOldValues();
	void outputOptimalSolution();

	void calculateLinearlyDecreasingProbability(int iteration);
	void calculateElitistRetentionFactor(int iteration);
	void selectionTournament(std::vector<int> &tournamnetIndexVector,
						   	 std::vector<int> &winnerIndexVector,
							 int iter);

	// utility methods
	std::vector<Job> importDefaultSample(std::string fileName = "dataset.txt");
	std::vector<Job> generateJobs(int numberOfJobs, int numberOfProcesses, int numberOfMachines);
	void splitJobs(std::vector<int>& firstGroup, std::vector<int>& secondGroup);
	void minimizeAdjacentDuplicates(std::vector<int>& nums);
	std::vector<std::pair<int, int>> unique_pairs(const std::vector<int>& vec);
	void printPopulation();
	void printJobs(std::vector<Job> jobs);
	void outputJobs(std::vector<Job> jobs);
	void printDominationValues();

	int numberOfJobs_, numberOfMachines_;
	int itterations_;
	int sampleSize_;
	int numberOfProcesses_;
	const int maxDuration_ = 10;
	const float maxCrowdingDistance_ = 10000;
	double currentCrossoverProbability_ = 0.8;
	double currentMutationProbability_ = 0.1;
	double currentElitistRetentionFactor_ = 0.4;


	double maxCrossoverProbability = 0.8;
	double minCrossoverProbability = 0.4;

	double maxMutationProbability = 0.1;
	double minMutationProbability = 0.02;
	
	double maxElitistRetentionFactor = 0.4;
	double minElitistRetentionFactor = 0.1;

	std::vector<Job> jobs_;
	std::vector<IndividualPtr> population_;
	std::vector<std::pair<int,int>> selectedParents_;
	std::vector<IndividualPtr> newPopulation_;

	std::vector<std::vector<IndividualPtr>> fronts_;

	std::string useDefaultSample_ = "0";
};
