#include <iostream>
#include <memory>
#include <cstdlib>
#include <iomanip>
#include <algorithm>
#include <random>
#include <unordered_map>
#include <queue>
#include <set>
#include <fstream>
#include <sstream>
#include <iterator>

#include "Nsga2 Workshop.h"
#include "individual.cpp"

Nsga::Nsga() 
{
	numberOfJobs_ = numberOfMachines_ = itterations_ = sampleSize_ = numberOfProcesses_ = 0;
}

Nsga::Nsga(int numberOfJobs, int numberOfMachines, int itterations,int sampleSize, int numberOfProcesses, std::string useDefault) {
	numberOfJobs_ = numberOfJobs;
	numberOfMachines_ = numberOfMachines;
	itterations_ = itterations;
	sampleSize_ = sampleSize;
	numberOfProcesses_ = numberOfProcesses;
	useDefaultSample_ = useDefault;
}

void Nsga::run() 
{
	int itteration = 1;

	// STEP 1: Population initialization
	initalizePopulation();

	while (itteration <= itterations_)
	{
		// STEP 2: Determination of the objective function fitness value
		determineFitnessValue();
		if(itteration == 1)
			std::cout << population_[0]->getGenesAsString() << ";";

		// STEP 3: Fast non-dominated and crowding ranking
		nonDominatedSortingAndCrowdingDegree();

		// STEP 4: Competition selection
		competitionSelection();

		calculateLinearlyDecreasingProbability(itteration);

		if(itteration >= itterations_/3) {
			calculateElitistRetentionFactor(itteration);
		}

		// STEP 5: Crossover and mutation operation
		crossoverAndMutation();

		cleanupOldValues();

		// STEP 6: Elitist retention strategy
		elitistRetention(itteration);

		// Cleanup old values
		cleanupOldValues();

		// Genetic frequency plus one
		itteration++;
	}

	// STEP 7: Determination of the optimal solution
	outputOptimalSolution();
}

void Nsga::initalizePopulation()
{
	// Generate jobs
	if (useDefaultSample_.size() == 1) {
		if (std::stoi(useDefaultSample_) == 1) 
		{
			jobs_ = importDefaultSample();
		} 
		else 
		{
			jobs_ = generateJobs(numberOfJobs_, numberOfProcesses_, numberOfMachines_);
		}
	} else {
		jobs_ = importDefaultSample(useDefaultSample_);
	}

	// Displaying the generated jobs and their processes
	outputJobs(jobs_);

	// Initialize population
	for (int i=0; i<sampleSize_; i++)
	{
		IndividualPtr individual = std::make_shared<Individual>(jobs_, numberOfProcesses_, numberOfMachines_, i*sampleSize_);
		population_.push_back(individual);
	}
}

void Nsga::determineFitnessValue() 
{
	int individualNumber = 0;
	
	for(auto& individual : population_)
	{
		// calculate and set maximum completion time
		int maxCompletionTime = 0;
		// calculate and set total equipment load
		int totalEquipmentLoad = 0;

		std::map<int, int> machinesRuntime; // <machineId, machineLoad>
		std::map<int, int> jobRuntime;		// <jobId, jobCurrentTime>
		std::vector<int> occurrenceVector(*max_element(individual->processes_.begin(), individual->processes_.end()) + 1, 0);

		for(int gene = 0; gene < numberOfProcesses_; gene++)
		{
			const int jobIndex = individual->processes_[gene] - 1;
			occurrenceVector[jobIndex]++;
			const int processIndex = occurrenceVector[jobIndex] - 1;
			const int machineIndex = individual->machines_[gene] -1;

			// Get process duration for the machine
			int currentWorkpieceTime = jobs_[jobIndex].processes[processIndex].machineDurations[machineIndex];

			auto jobTime = jobRuntime.find(jobIndex);
			if (jobTime == jobRuntime.end())
			{
				jobRuntime[jobIndex] = 0;
			}

			// seaching if the corresponding machine is already running
			auto machineTime = machinesRuntime.find(machineIndex);
			if (machineTime != machinesRuntime.end())
			{
				if (jobRuntime[jobIndex] > machinesRuntime[machineIndex])
				{
					jobRuntime[jobIndex] += currentWorkpieceTime;
					machinesRuntime[machineIndex] = jobRuntime[jobIndex];
				}
				else
				{
					machinesRuntime[machineIndex] += currentWorkpieceTime;
					jobRuntime[jobIndex] = machinesRuntime[machineIndex];
				}
			}
			else
			{
				jobRuntime[jobIndex] += currentWorkpieceTime;
				machinesRuntime.insert({machineIndex, jobRuntime[jobIndex]});
			}
		}
		for(auto it : machinesRuntime)
		{
			if (it.second > maxCompletionTime)
			{
				maxCompletionTime = it.second;
			}
			totalEquipmentLoad+= it.second;
		}
		population_[individualNumber]->maxCompletionTime_ = maxCompletionTime;
		population_[individualNumber]->totalEquipmentLoad_ = totalEquipmentLoad;
		individualNumber++;
	}
}

void Nsga::nonDominatedSortingAndCrowdingDegree()
{
	// Non-dominated values
	std::vector<IndividualPtr> firstFront;
	for (std::size_t i=0; i < population_.size(); i++)
	{
		for (int j=0; j < population_.size(); j++)
		{
			// check if point 1 < point 2
			if(population_[i]->dominates(population_[j]))
			{
				population_[i]->dominatedPoints_.push_back(j);
			} else if (population_[j]->dominates(population_[i]))
			{
				population_[i]->dominationCount_+=1;
			}
		}

		if (population_[i]->dominationCount_ == 0)
		{
			population_[i]->frontLevel_ = 0;
			firstFront.push_back(population_[i]);
		}
	}

	fronts_.push_back(firstFront);

	int i = 0;
	while(fronts_[i].size() > 0) 
	{
		std::vector<IndividualPtr> nextFront;
		for(auto individual : fronts_[i]) {
			for (auto j : individual->dominatedPoints_) 
			{
				population_[j]->dominationCount_--;
				if (population_[j]->dominationCount_ == 0) 
				{
					population_[j]->frontLevel_ = i+1;
					nextFront.push_back(population_[j]);
				}
			}
		}
		i++;
		fronts_.push_back(nextFront);
	}

	for (std::vector<IndividualPtr> front : fronts_) 
	{
		if (front.size() > 0) 
		{
			int solutions_number = front.size();
			// calculate crowding distance based on objective 1
			std::sort(front.begin(), front.end(),
				[](const IndividualPtr &a, const IndividualPtr &b) -> bool
				{
					return a->maxCompletionTime_ < b->maxCompletionTime_;
				});
			front[0]->crowdingDistance_ = front[solutions_number - 1]->crowdingDistance_ = std::numeric_limits<double>::infinity();

			std::vector<int> durationsInFront;
			for (auto individual : front) 
			{
				durationsInFront.push_back(individual->maxCompletionTime_);
			}
			int scale = std::max_element(durationsInFront.begin(), durationsInFront.end()) - std::min_element(durationsInFront.begin(), durationsInFront.end()); 
			if(scale == 0) scale = 1;
			for (int i = 1; i < solutions_number - 1; i++) 
			{
				front[i]->crowdingDistance_ += static_cast<double>(front[i+1]->maxCompletionTime_ - front[i-1]->maxCompletionTime_) / static_cast<double>(scale);
			}

			// calculate crowding distance based on objective 2
			std::sort(front.begin(), front.end(),
				[](const IndividualPtr &a, const IndividualPtr &b) -> bool
				{
					return a->totalEquipmentLoad_ < b->totalEquipmentLoad_;
				});
			front[0]->crowdingDistance_ = front[solutions_number - 1]->crowdingDistance_ = std::numeric_limits<double>::infinity();

			std::vector<int> totalLoadInFront;
			for (auto individual : front) 
			{
				totalLoadInFront.push_back(individual->totalEquipmentLoad_);
			}
			scale = std::max_element(totalLoadInFront.begin(), totalLoadInFront.end()) - std::min_element(totalLoadInFront.begin(), totalLoadInFront.end()); 
			if(scale == 0) scale = 1;
			for (int i = 1; i < solutions_number - 1; i++) 
			{
				front[i]->crowdingDistance_ += static_cast<double>(front[i+1]->totalEquipmentLoad_ - front[i-1]->totalEquipmentLoad_) / static_cast<double>(scale);
			}
		}
	}

	std::sort(population_.begin(), population_.end(),
		[](const IndividualPtr &a, const IndividualPtr &b) -> bool
		{
			if (a->frontLevel_ == b->frontLevel_)
				return a->crowdingDistance_ > b->crowdingDistance_;
			else
				return a->frontLevel_ < b->frontLevel_;
		});

	fronts_.clear();
}

void Nsga::competitionSelection() 
{
	std::random_device rd;
	// Get a high-resolution time point as part of the seed
    auto now = std::chrono::high_resolution_clock::now();

	std::mt19937 gen(rd());

	std::vector<int> selectedParents;

	int retry = 0;

	while (selectedParents.size() < population_.size()) 
	{
		// Combine multiple sources of entropy for the seed
    	std::size_t seed = rd() ^ now.time_since_epoch().count() ^ selectedParents.size();
		gen.seed(seed);

		int firstIndividual = gen() % population_.size();
		int secondIndividual = gen() % population_.size();

		if (retry < 50 && *population_[firstIndividual] == *population_[secondIndividual]) 
		{
			retry++;
			continue;
		}

		retry = 0;

		if (population_[firstIndividual]->frontLevel_ == population_[secondIndividual]->frontLevel_) 
		{
			if (population_[firstIndividual]->crowdingDistance_ > population_[secondIndividual]->crowdingDistance_) 
			{
				selectedParents.push_back(firstIndividual);
			} 
			else 
			{
				selectedParents.push_back(secondIndividual);
			}
			continue;
		}

		if (population_[firstIndividual]->frontLevel_ < population_[secondIndividual]->frontLevel_) 
		{
			selectedParents.push_back(firstIndividual);
		} else selectedParents.push_back(secondIndividual);
	}

	selectedParents_ = unique_pairs(selectedParents);
}

void Nsga::crossoverAndMutation() 
{
	std::srand(unsigned(std::time(0)));
	std::vector<int> machineMask;

	std::random_device rd;
	// Get a high-resolution time point as part of the seed
    auto now = std::chrono::high_resolution_clock::now();

	// Combine multiple sources of entropy for the seed
    std::size_t seed = rd() ^ now.time_since_epoch().count() ^ static_cast<std::size_t>(itterations_);
	std::mt19937 gen(seed);
	std::uniform_real_distribution<> dis(0, 1);

	for (int i = 0; i < numberOfProcesses_ / 2; i++)
	{
		machineMask.push_back(1);
	}

	for (int j = numberOfProcesses_ / 2; j < numberOfProcesses_; j++)
	{
		machineMask.push_back(0);
	}
	std::shuffle(machineMask.begin(), machineMask.end(), gen);

	for(const auto& parents : selectedParents_) 
	{
		IndividualPtr child1 = std::make_shared<Individual>(population_[parents.first]);
		IndividualPtr child2 = std::make_shared<Individual>(population_[parents.second]);

		double randomValue = dis(gen);
		if (randomValue <= currentCrossoverProbability_) {

			double r = ((double)rand() / (RAND_MAX));
			if (r >= 0.5) { // machine-base crossover

				std::vector<int> occurrenceVector(*max_element(child1->processes_.begin(), child1->processes_.end()) + 1, 0);

				for (int iter = 0; iter < machineMask.size(); iter++)
				{
					int processGene1 = population_[parents.first]->processes_[iter];
					occurrenceVector[processGene1]++;
					if (machineMask[iter] == 1) 
					{
						int count = 0; // Counter for the occurrences of job 
						int jobIndex2 = -1;

						for (int i = 0; i < population_[parents.second]->processes_.size(); ++i) 
						{
							if (population_[parents.second]->processes_[i] == processGene1) 
							{
								count++; // Increment the occurrence count
								if (count == occurrenceVector[processGene1]) 
								{
									jobIndex2 = i; // Return the index when Oth occurrence is found
								}
							}
						}
						if (jobIndex2 != -1) 
						{
							std::swap(child1->machines_[iter], child2->machines_[jobIndex2]);
						}
						else std::cout << "ERROR: machineBasedCrossover FAILED" << std::endl;
					}
				}
			} 
			else 
			{ // process-based crossover
				std::vector<int> firstGroup, secondGroup;
				splitJobs(firstGroup, secondGroup);

				int i = 0, j = 0;
				while (i < numberOfProcesses_) {
					int processGene1 = population_[parents.first]->processes_[i];
					int machineGene1 = population_[parents.first]->machines_[i];

					if (std::count(firstGroup.begin(), firstGroup.end(), processGene1)) 
					{		
						child1->processes_[i] = processGene1;
						child1->machines_[i] = machineGene1;
						i++;
					}
					else 
					{
						bool found = false;
						while (!found && j < numberOfProcesses_) 
						{
							int processGene2 = population_[parents.second]->processes_[j];
							int machineGene2 = population_[parents.second]->machines_[j];
							if (std::count(secondGroup.begin(), secondGroup.end(), processGene2)) 
							{
								child1->processes_[i] = processGene2;
								child1->machines_[i] = machineGene2;
								i++; j++;
								found = true;
							}
							else 
							{
								j++;
							}
						}
					}
				}


				i = 0; j = 0;
				while (i < numberOfProcesses_) 
				{
					int processGene2 = population_[parents.second]->processes_[i];
					int machineGene2 = population_[parents.second]->machines_[i];

					if (std::count(secondGroup.begin(), secondGroup.end(), processGene2)) 
					{
						child2->processes_[i] = processGene2;
						child2->machines_[i] = machineGene2;
						i++;
					}
					else 
					{
						bool found = false;
						while (!found && j < numberOfProcesses_) {
							int processGene1 = population_[parents.first]->processes_[j];
							int machineGene1 = population_[parents.first]->machines_[j];
							if (std::count(firstGroup.begin(), firstGroup.end(), processGene1)) 
							{
								child2->processes_[i] = processGene1;
								child2->machines_[i] = machineGene1;
								i++; j++;
								found = true;
							}
							else j++;
						}
					}
				}
			}

			double randomMutationDraw = ((double)rand() / (RAND_MAX));

			if (randomMutationDraw <= currentMutationProbability_) {
				child1->mutate(jobs_, numberOfMachines_);
				child2->mutate(jobs_, numberOfMachines_);
			}
		}
		child1->isChild = true;
		child2->isChild = true;

		newPopulation_.push_back(child1);
		newPopulation_.push_back(child2);
	}
}

void Nsga::elitistRetention(int iteration) 
{
	for (const auto& parents : selectedParents_) 
	{
		IndividualPtr parent1 = std::make_shared<Individual>(population_[parents.first]);
		IndividualPtr parent2 = std::make_shared<Individual>(population_[parents.second]);

		newPopulation_.push_back(parent1);
		newPopulation_.push_back(parent2);
	}

	population_.clear();

	for (const auto individual : newPopulation_) 
	{
		IndividualPtr newIndividual = std::make_shared<Individual>(individual);
		population_.push_back(newIndividual);
	}

	selectedParents_.clear();
	newPopulation_.clear();

	determineFitnessValue();
	nonDominatedSortingAndCrowdingDegree();

	std::vector<IndividualPtr> newPopulation;

	int currentParents = 0;
	for (auto& individual : population_) 
	{
		if (newPopulation.size() == sampleSize_) break;
		if (!individual->isChild && currentParents >= static_cast<double>(currentElitistRetentionFactor_ * sampleSize_)) continue;
		if (!individual->isChild) currentParents++;
		IndividualPtr newIndividual = std::make_shared<Individual>(individual);
		newPopulation.push_back(newIndividual);
	}

	population_.clear();

	for(auto& individual : newPopulation) 
	{
		IndividualPtr newIndividual = std::make_shared<Individual>(individual);
		population_.push_back(newIndividual);
	}

	determineFitnessValue();
	nonDominatedSortingAndCrowdingDegree();
}

void Nsga::cleanupOldValues()
{
	for (auto iter : population_)
	{
		iter->maxCompletionTime_ = 0;
		iter->totalEquipmentLoad_ = 0;
		iter->dominationCount_ = 0;
		for (auto iter2 : iter->dominatedPoints_)
		{
			iter->dominatedPoints_.pop_back();
		}
		iter->frontLevel_ = 0;
		iter->crowdingDistance_ = 0.0;
	}
}

void Nsga::outputOptimalSolution() 
{
	determineFitnessValue();
	nonDominatedSortingAndCrowdingDegree();

	// Optimal solutions sorted by total front level and crowding distance
	std::sort(population_.begin(), population_.end(),
		[](const IndividualPtr &a, const IndividualPtr &b) -> bool
		{
			if (a->frontLevel_ == b->frontLevel_)
				return a->crowdingDistance_ > b->crowdingDistance_;
			else
				return a->frontLevel_ < b->frontLevel_;
		});

	std::cout << population_[0]->getGenesAsString();
}

void Nsga::calculateLinearlyDecreasingProbability(int iteration) 
{
	double progress = static_cast<double>(iteration) / itterations_;
	currentCrossoverProbability_ = maxCrossoverProbability + progress * (minCrossoverProbability - maxCrossoverProbability);
	currentMutationProbability_ = maxMutationProbability + progress * (minMutationProbability - maxMutationProbability);
}

void Nsga::calculateElitistRetentionFactor(int iteration) 
{
    if (iteration <= itterations_ / 3) {
        currentElitistRetentionFactor_ = 0.4;
    } else if (iteration >= itterations_) {
        currentElitistRetentionFactor_ = 0.1;
    } else {
        double slope = (0.1 - 0.4) / (itterations_ - itterations_ / 3);
        double intercept = 0.4 - slope * (itterations_ / 3);
        currentElitistRetentionFactor_ = slope * iteration + intercept;
    }
}

// utility methods

std::vector<Job> Nsga::importDefaultSample(std::string fileName)
{
	std::vector<Job> jobs = std::vector<Job>(numberOfJobs_);

	std::ifstream file(fileName);
	std::string content;

	if (!file.is_open()) 
	{
        content = "1,1:12,5,18,10,16,23,18,12,21,13;1,2:15,12,5,16,7,18,21,17,16,9;1,3:17,4,12,11,9,14,11,10,25,10;2,1:19,14,5,17,16,13,10,15,14,6;2,2:18,14,24,11,16,19,20,9,22,7;2,3:13,16,10,15,18,16,17,5,15,16;2,4:8,7,12,6,5,10,22,8,8,17;3,1:16,13,18,6,14,7,20,12,19,5;3,2:11,10,9,16,11,8,5,12,10,5;4,1:21,17,21,16,20,5,18,8,19,17;4,2:5,24,12,20,17,18,20,22,21,14;4,3:6,7,5,5,7,6,16,9,17,10;5,1:23,14,12,5,15,11,13,14,5,16;5,2:15,5,22,12,16,8,13,18,8,13;5,3:12,10,11,14,15,25,16,13,15,15;6,1:5,15,6,17,20,16,14,10,5,19;6,2:14,13,12,5,15,7,11,14,17,13;7,1:18,21,15,12,9,24,7,5,20,7;7,2:17,14,15,17,19,20,15,12,16,15;7,3:8,10,9,8,7,12,14,7,8,9;7,4:15,20,18,23,5,16,10,16,6,21;7,5:12,25,16,8,15,9,18,17,20,5;7,6:10,8,7,7,7,8,9,17,6,8;8,1:17,20,8,23,19,19,11,15,16,5;8,2:5,18,15,20,16,22,19,17,13,14;8,3:24,7,26,24,25,24,9,18,10,20;8,4:5,22,16,18,13,7,19,8,20,21;9,1:5,7,7,11,8,11,10,23,8,18;9,2:24,25,7,22,12,18,5,20,17,21;9,3:15,9,13,13,14,10,12,11,16,10;10,1:20,21,18,11,19,18,17,8,22,19;10,2:15,14,8,15,10,16,13,15,16,12;10,3:11,15,8,12,10,13,23,8,9,9";
    } 
	else 
	{
		if (fileName.compare("dataset.txt") == 0) 
		{
			content = std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		} 
		else 
		{
			std::stringstream result;

			int number_total_jobs, number_total_machines, number_max_operations;
			std::string line;
			std::getline(file, line);
			std::istringstream headerStream(line);
			headerStream >> number_total_jobs >> number_total_machines >> number_max_operations;

			// Set Job Id to 1 to initiate dataset load
			int currentJob = 1;

			while (std::getline(file, line)) 
			{
				if (currentJob > number_total_jobs) 
				{
					break;
				}


				std::istringstream lineStream(line);
				int number_operations;
				lineStream >> number_operations;

				for (int id_operation = 0; id_operation < number_operations; ++id_operation) 
				{
					result << currentJob << "," << id_operation+1 << ":";
					int machinesNumber;
					lineStream >> machinesNumber;
					std::map<int,int> machinesMap;
					for (int i = 1; i <= number_total_machines; i++) 
					{
						machinesMap.insert(std::make_pair(i,100));
					}
					for (int i = 0; i < machinesNumber; i++) 
					{
						int machine, time;
						lineStream >> machine >> time;
						machinesMap[machine] = time;
					}

					for (int i = 1; i <= number_total_machines; i++) 
					{
						if (i == number_total_machines) 
						{
							result << machinesMap[i] << ";";
							break;
						}
						result << machinesMap[i] << ",";
					}
				}
				currentJob++;
			}

			content = result.str();
		}
	}

	size_t firstPeriodPos = content.find('.');
    std::string jobsStr = content.substr(0, firstPeriodPos);
    std::istringstream ssJobs(jobsStr);
    std::string tokenJobs;

	int currentJobIndex = 0;
    while (std::getline(ssJobs, tokenJobs, ';')) 
	{
        size_t colonPos = tokenJobs.find(':');
        std::string operation = tokenJobs.substr(0, colonPos);
        std::string durations = tokenJobs.substr(colonPos + 1);

        std::istringstream durationsStream(durations);
        std::vector<std::string> durationsArray(std::istream_iterator<std::string>{durationsStream}, 
                                                std::istream_iterator<std::string>());

		size_t colonPos2 = operation.find(',');
        std::string jobId = operation.substr(0, colonPos2);

		std::stringstream ssDurations(durations);
		std::string tokenDurations;

		std::map<int, int> processDurations;
		int machineIndex = 0;
		while (std::getline(ssDurations, tokenDurations, ',')) 
		{
			processDurations[machineIndex] = std::stoi(tokenDurations);
			machineIndex++;
		}

		if (std::stoi(jobId) != (currentJobIndex+1)) 
		{
			currentJobIndex++;
			
		}
		jobs[currentJobIndex].processes.push_back(Job::Process(processDurations));
    }

	file.close();
	return jobs;
}

std::vector<Job> Nsga::generateJobs(int numberOfJobs, int numberOfProcesses, int numberOfMachines)
{
	std::vector<Job> jobs;

	std::random_device rd;
	// Get a high-resolution time point as part of the seed
    auto now = std::chrono::high_resolution_clock::now();

	// Combine multiple sources of entropy for the seed
    std::size_t seed = rd() ^ now.time_since_epoch().count() ^
                       static_cast<std::size_t>(numberOfJobs);
	std::mt19937 gen(seed);
	std::uniform_int_distribution<> dis(0, numberOfJobs - 1);

	for (int i = 0; i < numberOfJobs; i++) 
	{
		std::vector<Job::Process> jobProcesses;

		std::map<int, int> processDurations;

		for (int machineIndex = 0; machineIndex < numberOfMachines; ++machineIndex) {
			processDurations[machineIndex] = std::max(static_cast<int>(gen() % maxDuration_) + 1, 1); // Random duration for each machine
		}
		jobProcesses.push_back(Job::Process(processDurations));
		jobs.push_back(Job(jobProcesses));

		numberOfProcesses--;
	}

	while (numberOfProcesses > 0) 
	{
		int randomJob = dis(gen);

		std::map<int, int> processDurations;

		for (int machineIndex = 0; machineIndex < numberOfMachines; ++machineIndex) 
		{
			processDurations[machineIndex] = std::max(static_cast<int>(gen() % maxDuration_) + 1, 1); // Random duration for each machine
		}

		jobs[randomJob].processes.push_back(Job::Process(processDurations));

		numberOfProcesses--;
	}

	return jobs;
}

void Nsga::splitJobs(std::vector<int>& firstGroup, std::vector<int>& secondGroup) 
{
	// Original vector of integers -> testing
	std::vector<int> originalVector(jobs_.size());
	std::iota(originalVector.begin(), originalVector.end(), 1);

	std::random_device rd;
	// Get a high-resolution time point as part of the seed
    auto now = std::chrono::high_resolution_clock::now();

	// Combine multiple sources of entropy for the seed
    std::size_t seed = rd() ^ now.time_since_epoch().count();
	std::mt19937 gen(seed);

	// Ensure that each resulting vector has at least a third of the original size
	int minVectorSize = originalVector.size() / 3;

	// Generate random sizes for the two new vectors
	std::uniform_int_distribution<> distribution(minVectorSize, originalVector.size() - minVectorSize);
	int firstVectorSize = distribution(gen);
	int secondVectorSize = originalVector.size() - firstVectorSize;

	// Shuffle the original vector to make the splitting random
	std::shuffle(originalVector.begin(), originalVector.end(), gen);

	// Create the two new vectors and copy elements
	std::vector<int> firstVector(originalVector.begin(), originalVector.begin() + firstVectorSize);
	std::vector<int> secondVector(originalVector.begin() + firstVectorSize, originalVector.end());

	firstGroup = firstVector;
	secondGroup = secondVector;
}

void Nsga::minimizeAdjacentDuplicates(std::vector<int>& nums) 
{
    std::unordered_map<int, int> freqMap;

    // Calculate the frequency of each number
    for (const auto& num : nums) 
	{
        ++freqMap[num];
    }

    // Use priority queue to sort by frequency
    auto compare = [](const std::pair<int, int>& a, const std::pair<int, int>& b) 
	{
        return a.second < b.second;
    };
    std::priority_queue<std::pair<int, int>, std::vector<std::pair<int, int>>, decltype(compare)> pq(compare);

    for (const auto& entry : freqMap) 
	{
        int num = entry.first;
        int freq = entry.second;
        pq.push({num, freq});
    }

    int n = nums.size();
    int i = 0;

    // Try to place each number as far apart as possible
    while (!pq.empty()) 
	{
        auto top = pq.top();
        pq.pop();
        int num = top.first;
        int freq = top.second;

        for (int count = 0; count < freq; ++count) 
		{
            if (i >= n) {
                i = 1;  // Reset index to fill in the gaps
            }

            nums[i] = num;
            i += 2;  // Try to place the number far apart
        }
    }
}

std::vector<std::pair<int, int>> Nsga::unique_pairs(const std::vector<int>& vec) 
{
    // Create a map to store frequency of each number
    std::map<int, int> freq;

    // Populate the frequency map
    for (int num : vec) 
	{
        freq[num]++;
    }

    // Create a set for numbers left to be paired
    std::set<int> unpaired;

    for (const auto& entry : freq) 
	{
        int num = entry.first;
        int count = entry.second;
        unpaired.insert(num);
    }

    std::vector<std::pair<int, int>> pairs;
    std::set<std::pair<int, int>> pairedNumbers;

    // Create unique pairs by iterating over the unpaired set
    while (unpaired.size() > 1) 
	{
        auto it1 = unpaired.begin();
        auto it2 = std::next(it1);

        while (it2 != unpaired.end() && pairedNumbers.count({*it1, *it2})) 
		{
            ++it2;
        }

        // If no more unique pairs can be created, break
        if (it2 == unpaired.end()) 
		{
            break;
        }

        pairs.push_back({*it1, *it2});
        pairedNumbers.insert({*it1, *it2});
        pairedNumbers.insert({*it2, *it1});  // Since (a, b) and (b, a) are the same pair

        // Decrease the frequency of the paired numbers
        freq[*it1]--;
        freq[*it2]--;

        // Remove from unpaired set if their frequency becomes zero
        if (freq[*it1] == 0) unpaired.erase(it1);
        if (freq[*it2] == 0) unpaired.erase(it2);
    }

    // Handle remaining numbers
    for (const auto& entry : freq) 
	{
        int num = entry.first;
        int count = entry.second;
        for (int i = 0; i < count; ++i) 
		{
            unpaired.insert(num);
        }
    }

    // Form pairs from the leftover numbers
    while (unpaired.size() > 1) 
	{
        auto it1 = unpaired.begin();
        auto it2 = std::next(it1);

        pairs.push_back({*it1, *it2});

        // Remove numbers from the set
        unpaired.erase(it1);
        unpaired.erase(it2);
    }

    // Any leftover single numbers are paired with themselves
    for (int num : unpaired) 
	{
        pairs.push_back({num, num});
    }

    return pairs;
}

void Nsga::printPopulation()
{
	int itteration = 0;

	std::cout << "\nCurrent population:\n\n";
	for (auto& individual : population_)
	{
		std::cout << "Individual " << std::setw(2) << ++itteration << ": " << individual->getGenesAsString()
				  << "  Front Level: " << std::setw(2) << individual->frontLevel_
				  << "  Crowding Distance: " << std::setw(10) << individual->crowdingDistance_ 
				  << "	Domination Count: " << std::setw(10) << individual->dominationCount_ << std::endl;
	}
	std::cout << "\n";
}

void Nsga::printJobs(std::vector<Job> jobs)
{
	for (int i = 0; i < jobs.size(); ++i) 
	{
		std::cout << "Job " << i + 1 << ":\n";
		for (int j = 0; j < jobs[i].processes.size(); ++j) 
		{
			std::cout << "  Process " << j + 1 << ":\n";
			for (const auto& durationPair : jobs[i].processes[j].machineDurations) 
			{
				std::cout << "    Machine " << durationPair.first << ": " << durationPair.second << " minutes\n";
			}
		}
		std::cout << std::endl;
	}
}

void Nsga::outputJobs(std::vector<Job> jobs)
{
	for (int i = 0; i < jobs.size(); ++i) 
	{
		for (int j = 0; j < jobs[i].processes.size(); ++j) 
		{
			std::cout << i + 1 << "," << j + 1 << ":";
			for (const auto& durationPair : jobs[i].processes[j].machineDurations) 
			{
				if (std::addressof(durationPair) == std::addressof(*(std::prev(jobs[i].processes[j].machineDurations.end())))) 
				{
					if (i + 1 != jobs.size()) 
					{
						std::cout << durationPair.second << ";";
					}
					else 
					{
						if (j + 1 == jobs[i].processes.size())
							std::cout << durationPair.second << ".";
						else 
						{
							std::cout << durationPair.second << ";";
						}
					}
				}
				else 
				{
					std::cout << durationPair.second << ",";
				}
			}
		}
	}
}

void Nsga::printDominationValues()
{
	int i = 0;
	std::cout << "\ndominationValues:\n";
	for(auto& iter : population_)
	{
		std::cout << "Individual " << std::setw(2) << i++
				  <<": domination_count: " << std::setw(2) << iter->dominationCount_ << " dominated_list: ";
		for (auto iter2 : iter->dominatedPoints_)
		{
			std::cout << iter2 << " ";
		}
		std::cout << std::endl;
	}
}

int main(int argc, char* argv[])
{
	bool command_line_args = true;

	int numberOfJobs, numberOfMachines, numberOfProcesses;
	int sampleSize;
	int itterations;
	std::string useDefault;

	if (command_line_args) 
	{
		// Check if there are at least two arguments (executable name + 5 user arguments)
		if (argc >= 7) 
		{

			numberOfJobs = std::stoi(argv[1]);

			numberOfMachines = std::stoi(argv[2]);

			numberOfProcesses = std::stoi(argv[3]);

			sampleSize = std::stoi(argv[4]);

			itterations = std::stoi(argv[5]);

			// Sample data

			useDefault = std::string(argv[6]);
		}
		else 
		{
			// Print an error message if there are not enough arguments
			std::cout << "Usage: " << argv[0] << " <numberOfJobs>" << " <numberOfMachines>" << " <numberOfProcesses>" << " <sampleSize>" << " <itterations>" << " <useDefault>. Only " << argc << " args provided." << std::endl;
			return 0;
		}
	}
	else 
	{
		std::cout << "Enter number of jobs (N): ";
		std::cin >> numberOfJobs;

		std::cout << "Enter number of machines: ";
		std::cin >> numberOfMachines;

		std::cout << "Input number of processes: ";
		std::cin >> numberOfProcesses;

		std::cout << "Input sampleSize: ";
		std::cin >> sampleSize;
		
		std::cout << "Input number of itterations: ";
		std::cin >> itterations;
	}

	std::unique_ptr<Nsga> workshop = std::make_unique<Nsga>(numberOfJobs, numberOfMachines, itterations, sampleSize, numberOfProcesses, useDefault);
	workshop->run();
	return 0;
}
