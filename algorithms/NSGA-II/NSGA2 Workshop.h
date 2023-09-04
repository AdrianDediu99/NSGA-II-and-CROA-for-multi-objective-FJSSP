#pragma once
#include <vector>
#include <map>
#include "Job.h"
#include "individual.h"

class Nsga {
public:
	Nsga();
	Nsga(int numberOfJobs, int numberOfMachines, int itterations, int sampleSize, int numberOfProcesses, int useDefault = 0);

	void run();

private:
	void initalizePopulation();
	void determineFitnessValue();
	void calculateNonDominatedValuesAndCrowdingDegree();
	void competitionSelection();
	void crossoverAndMutation();
	void elitistRetention();
	void cleanupOldValues();
	void outputOptimalSolution();

	void calculateLinearlyDecreasingProbability(int iteration);
	void calculateFrontLevel();
	void calculateCrowdingDistance();
	void calculateFrontCrowdingDistance();

	// sorting methods
	void sortNonDominated();
	void sortPopulationByMaxCompletionTime();
	void sortPopulationByTotalEquipmentLoad();

	// utility methods
	std::vector<Job> importDefaultSample();
	std::vector<Job> generateJobs(int numberOfJobs, int numberOfProcesses, int numberOfMachines);
	void splitJobs(std::vector<int>& firstGroup, std::vector<int>& secondGroup);
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


	double maxCrossoverProbability = 0.8;
	double minCrossoverProbability = 0.4;

	double maxMutationProbability = 0.1;
	double minMutationProbability = 0.02;
	
	std::vector<Job> jobs_;
	std::vector<IndividualPtr> population_;
	std::vector<std::pair<int,int>> selectedParents_;
	std::vector<IndividualPtr> newPopulation_;

	std::vector<int> front1_;
	std::vector<int> front2_;
	std::vector<int> front3_;
	std::vector<int> front4_;

	int useDefaultSample_ = 0;
};
