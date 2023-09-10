#include <iostream>
#include <memory>
#include <cstdlib>
#include <iomanip>
#include <algorithm>
#include <random>
#include <utility>
#include <fstream>
#include <sstream>
#include <iterator>

#include "CRO.h"
#include "Coral.h"
#include "Coral.cpp"

CRO::CRO() 
{
	numberOfJobs_ = numberOfMachines_ = numberOfProcesses_ = reefSize_ = generations_ = 0;
}

CRO::CRO(int numberOfJobs, int numberOfMachines, int numberOfProcesses, int reefSize, int generations, std::string useDefault) 
{
	numberOfJobs_ = numberOfJobs;
	numberOfMachines_ = numberOfMachines;
	numberOfProcesses_ = numberOfProcesses;
	reefSize_ = reefSize;
	generations_ = generations;
	useDefaultSample_ = useDefault;
}

void CRO::run() 
{
	int generation = 1;

	initializePopulation();

	while (generation < generations_) 
	{

		// Begin sexual reproduction
		sexualReproduction();

		// Determination of the objective function fitness value
		determineFitnessValue();

		// Begin larvae settling
		larvaSettling(waterLarvae_.size());

		// Calculate domination counts
		calculateDominationCounts();
		
		// Begin asexual reproduction
		asexualReproduction();

		// RE-Determination of the objective function fitness value
		determineFitnessValue();

		// RE-Calculate domination counts
		calculateDominationCounts();

		// Extreme Depredation Operator
		extremeDepredation();

		// RE-Calculate domination counts
		calculateDominationCounts();

		// Begin depredation
		depredation();

		// Cleanup old values
		cleanupOldValues();

		generation++;

	}

	outputOptimalSolution();
}

void CRO::initializePopulation() 
{
	// Generate jobs
	if (useDefaultSample_.size() == 1) {
		if (std::stoi(useDefaultSample_) == 1) 
		{
			jobs_ = importDefaultSample();
		} 
		else 
		{
			jobs_ = generateJobs(numberOfJobs_, numberOfMachines_, numberOfProcesses_);
		}
	} else {
		jobs_ = importDefaultSample(useDefaultSample_);
	}

	outputJobs(jobs_);

	// Initialize population

	// Initialize a random binary mask matrix
	int totalElements = reefSize_ * reefSize_;
	int occupationCount = (totalElements * occupationRate_) / 100;

 	std::vector<std::vector<int>> binaryMaskMatrix(reefSize_, std::vector<int>(reefSize_, 0));

	for (int i = 0; i < occupationCount; i++) 
	{
        int row = std::rand() % reefSize_;
        int col = std::rand() % reefSize_;

        // Check if the element is already 1, if not, set it to 1
        if (binaryMaskMatrix[row][col] != 1) {
            binaryMaskMatrix[row][col] = 1;
        } else {
            i--; // Try again to fill a different element
        }
    }
	
	reef_.resize(reefSize_, std::vector<CoralPtr>(reefSize_));

	// Fill the reef with corals
    for (int i = 0; i < reefSize_; i++) 
	{
        for (int j = 0; j < reefSize_; j++) 
		{
			CoralPtr coral = std::make_shared<Coral>(jobs_, numberOfProcesses_, numberOfMachines_, i*j-i);
			if (binaryMaskMatrix[i][j] == 1) 
			{
            	reef_[i][j] = coral;
				currentPopulation_.push_back(std::make_pair(i, j));
			}
        }
    }
}

void CRO::sexualReproduction() 
{
	// Shuffle the processes using a random engine
    std::random_device rd;

    // Get a high-resolution time point as part of the seed
    auto now = std::chrono::high_resolution_clock::now();

	// Combine multiple sources of entropy for the seed
    std::size_t seed = rd() ^ now.time_since_epoch().count();

    // Initialize the random engine with the combined seed
    std::mt19937 gen(seed);

	int broadcastCount = (currentPopulation_.size() * reproductionFactor_) / 100;

	broadcastCount = (broadcastCount%2==0?broadcastCount:broadcastCount-1);

	std::shuffle(currentPopulation_.begin(), currentPopulation_.end(), gen);

	for (auto i = 0; i < broadcastCount; i+=2) 
	{
		int coralRow1 = currentPopulation_[i].first;
		int coralCol1 = currentPopulation_[i].second;
		int coralRow2 = currentPopulation_[i+1].first;
		int coralCol2 = currentPopulation_[i+1].second;
		broadcastSpawning(reef_[coralRow1][coralCol1], reef_[coralRow2][coralCol2]);
	}

	for (auto i = broadcastCount; i < currentPopulation_.size(); i++) 
	{
		int coralRow = currentPopulation_[i].first;
		int coralCol = currentPopulation_[i].second;
		broodingMutation(reef_[coralRow][coralCol]);
	}
}

void CRO::broadcastSpawning(const CoralPtr& parent1, const CoralPtr& parent2) 
{
	std::srand(unsigned(std::time(0)));
	std::vector<int> firstGroup, secondGroup;

	splitJobs(firstGroup, secondGroup);

	int i = 0, j = 0;
	CoralPtr child = std::make_shared<Coral>();

	while (i < numberOfProcesses_) 
	{
		int processGene1 = parent1->processes_[i];
		int machineGene1 = parent1->machines_[i];

		if (std::count(firstGroup.begin(), firstGroup.end(), processGene1)) 
		{		
			child->processes_.push_back(processGene1);
			child->machines_.push_back(machineGene1);
			i++;
		}
		else 
		{
			bool found = false;
			while (!found && j < numberOfProcesses_) {
				int processGene2 = parent2->processes_[j];
				int machineGene2 = parent2->machines_[j];
				if (std::count(secondGroup.begin(), secondGroup.end(), processGene2)) 
				{
					child->processes_.push_back(processGene2);
					child->machines_.push_back(machineGene2);
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

	waterLarvae_.push_back(child);
}

void CRO::broodingMutation(const CoralPtr& coral) 
{
	CoralPtr child = std::make_shared<Coral>(coral);
	child->mutate(jobs_, numberOfMachines_);

	waterLarvae_.push_back(child);
}

void CRO::determineFitnessValue() 
{	
	// Determine Fitness for corals in reef
	for (int i = 0; i < reefSize_; i++) 
	{
		int coralNumber = 0;
		for(auto& coral : reef_[i])
		{
			if (!coral) 
			{
				coralNumber++;
				continue;
			}
			// calculate and set maximum completion time
			int maxCompletionTime = 0;
			// calculate and set total equipment load
			int totalEquipmentLoad = 0;

			std::map<int, int> machinesRuntime; // <machineId, machineLoad>
			std::map<int, int> jobRuntime;		// <jobId, jobCurrentTime>
			std::vector<int> occurrenceVector(*max_element(coral->processes_.begin(), coral->processes_.end()) + 1, 0);

			for(int gene = 0; gene < numberOfProcesses_; gene++)
			{
				const int jobIndex = coral->processes_[gene] - 1;
				occurrenceVector[jobIndex]++;
				const int processIndex = occurrenceVector[jobIndex] - 1;
				const int machineIndex = coral->machines_[gene] -1;

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
			reef_[i][coralNumber]->maxCompletionTime_ = maxCompletionTime;
			reef_[i][coralNumber]->totalEquipmentLoad_ = totalEquipmentLoad;
			coralNumber++;
		}
	}

	// Determine Fitness for larvae in water
	for (int i = 0; i < waterLarvae_.size(); i++) 
	{
		// calculate and set maximum completion time
		int maxCompletionTime = 0;
		// calculate and set total equipment load
		int totalEquipmentLoad = 0;

		std::map<int, int> machinesRuntime; // <machineId, machineLoad>
		std::map<int, int> jobRuntime;		// <jobId, jobCurrentTime>
		std::vector<int> occurrenceVector(*max_element(waterLarvae_[i]->processes_.begin(), waterLarvae_[i]->processes_.end()) + 1, 0);

		for(int gene = 0; gene < numberOfProcesses_; gene++)
		{
			const int jobIndex = waterLarvae_[i]->processes_[gene] - 1;
			occurrenceVector[jobIndex]++;
			const int processIndex = occurrenceVector[jobIndex] - 1;
			const int machineIndex = waterLarvae_[i]->machines_[gene] -1;

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
		waterLarvae_[i]->maxCompletionTime_ = maxCompletionTime;
		waterLarvae_[i]->totalEquipmentLoad_ = totalEquipmentLoad;
	}
}

void CRO::calculateDominationCounts() 
{
	// Cleanup old domination counts
	for (int i = 0; i < reefSize_; i++) 
	{
		for(auto& coral : reef_[i])
		{
			if (!coral) 
			{
				continue;
			}
			coral->dominationCount_ = 0;
		}
	}

    for (int row1 = 0; row1 < reefSize_; ++row1) 
	{
        for (int col1 = 0; col1 < reefSize_; ++col1) 
		{
            if (!reef_[row1][col1]) 
			{
				continue;  // Skip if spot not populated
			}
            // Starting (row2, col2) from (row1, col1) to avoid duplicate pairs
            for (int row2 = row1; row2 < reefSize_; ++row2) 
			{
                for (int col2 = 0; col2 < reefSize_; ++col2) 
				{
                    // Skip duplicate elements, self-pairs, and empty slots
                    if ((row2 == row1 && col2 <= col1) || !reef_[row2][col2]) continue;
					
					// check if coral 1 dominates coral 2
					if (reef_[row1][col1]->dominates(reef_[row2][col2]))
					{
						reef_[row2][col2]->dominationCount_+=1;
					}

					// check if coral 2 dominates coral 1
					if (reef_[row2][col2]->dominates(reef_[row1][col1]))
					{
						reef_[row1][col1]->dominationCount_+=1;
					}
                }
            }
        }
    }
}

void CRO::larvaSettling(int allowedLarvaeInReef) 
{
	std::random_device rd;
	// Get a high-resolution time point as part of the seed
    auto now = std::chrono::high_resolution_clock::now();

	// Combine multiple sources of entropy for the seed
    std::size_t seed = rd() ^ now.time_since_epoch().count();
	std::mt19937 gen(seed);

	int numberOfRetries = 3;
	for (auto i = 0; i < allowedLarvaeInReef; i++) 
	{
		if (numberOfRetries == 0) 
		{
			numberOfRetries = 3;
			continue;
		}

		int row = gen() % reefSize_;
		int col = gen() % reefSize_;

		if (!reef_[row][col])
		{
			reef_[row][col] = waterLarvae_[i];
			currentPopulation_.push_back(std::make_pair(row, col));
			numberOfRetries = 3;
		} 
		else if (waterLarvae_[i]->dominates(reef_[row][col])) 
		{
			reef_[row][col] = waterLarvae_[i];
		} 
		else 
		{
			i--; numberOfRetries--;
		}
	}

	// Removing the larvae from water
	waterLarvae_.clear();
}

void CRO::extremeDepredation() {
	int maxDuplicatesAllowed = 3;
	
	std::vector<std::pair<int,int>> objectives;
	for (int row = 0; row < reefSize_; row++) 
	{
        for (int col = 0; col < reefSize_; col++) 
		{
			if (reef_[row][col]) 
			{
				objectives.push_back(std::make_pair(reef_[row][col]->maxCompletionTime_, reef_[row][col]->totalEquipmentLoad_));
			}
        }
    }

	for (int row = 0; row < reefSize_; row++) 
	{
        for (int col = 0; col < reefSize_; col++) 
		{
			if (reef_[row][col]) 
			{
				int completionTime = reef_[row][col]->maxCompletionTime_;
				int equipmentLoad = reef_[row][col]->totalEquipmentLoad_;
				int count = std::count_if(objectives.begin(), objectives.end(),[&](const std::pair<int, int> pair) 
				{
					return (pair.first == completionTime && pair.second == equipmentLoad);
				});

				if(count > maxDuplicatesAllowed) {
					// std::cout << "TOO MANY" << std::endl;
					reef_[row][col] = nullptr;
					auto it1 = std::find(
						objectives.begin(), 
						objectives.end(), 
						std::make_pair(completionTime, equipmentLoad));
					if (it1 != objectives.end()) 
					{
						objectives.erase(it1);
					}

					auto it2 = std::find(
						currentPopulation_.begin(), 
						currentPopulation_.end(), 
						std::make_pair(row, col));
					if (it2 != currentPopulation_.end()) 
					{
						currentPopulation_.erase(it2);
					}
				}
			}
        }
    }
	// printPopulation();
}

void CRO::asexualReproduction() 
{
	for (int i = 0; i < reefSize_; i++) 
	{
		for(auto coral : reef_[i])
		{
			if (!coral) 
			{
				continue;
			}
			waterLarvae_.push_back(coral);
		}
	}

	// Sort water larva by domination count
	std::sort(waterLarvae_.begin(), waterLarvae_.end(),[](const CoralPtr &a, const CoralPtr &b) 
	{
    	return a->dominationCount_ < b->dominationCount_;
	});

	int buddingCount = (waterLarvae_.size() * buddingFactor_) / 100;

	larvaSettling(buddingCount);
}

void CRO::depredation() 
{
	std::vector<std::pair<std::pair<int,int>,int>> coralDominationCounts;
	for (int row = 0; row < reefSize_; row++) 
	{
        for (int col = 0; col < reefSize_; col++) 
		{
			if (reef_[row][col]) 
			{
				coralDominationCounts.push_back(std::make_pair(std::make_pair(row, col), reef_[row][col]->dominationCount_));
			}
        }
    }
	// Sort corals by domination count
	std::sort(coralDominationCounts.begin(), coralDominationCounts.end(),[](const auto &a, const auto &b) 
	{
    	return a.second > b.second;
	});

	int depredationCount = (coralDominationCounts.size() * depredationFactor_) / 100;

	for (int i = 0; i < depredationCount; i++) 
	{
		reef_[coralDominationCounts[i].first.first][coralDominationCounts[i].first.second] = nullptr;
		auto it = std::find(
			currentPopulation_.begin(), 
			currentPopulation_.end(), 
			std::make_pair(coralDominationCounts[i].first.first, coralDominationCounts[i].first.second));
		if (it != currentPopulation_.end()) 
		{
			currentPopulation_.erase(it);
		}
	}
}

void CRO::cleanupOldValues()
{
	for (int i = 0; i < reefSize_; i++) 
	{
		for(auto& coral : reef_[i])
		{
			if (!coral) 
			{
				continue;
			}
			coral->maxCompletionTime_ = 0;
			coral->totalEquipmentLoad_ = 0;
			coral->dominationCount_ = 0;

			coral->frontLevel_ = 0;
			coral->crowdingDistance_ = 0.0;
		}
	}
}

void CRO::outputOptimalSolution() 
{
	determineFitnessValue();
	calculateDominationCounts();

	std::vector<CoralPtr> solutions;
	for (int i = 0; i < reefSize_; i++) 
	{
		for(auto& coral : reef_[i])
		{
			if (!coral) {
				continue;
			}
			solutions.push_back(coral);
		}
	}

	// Sort corals asc by domination count
	std::sort(solutions.begin(), solutions.end(),[](const CoralPtr &a, const CoralPtr &b) 
	{
    	return a->dominationCount_ < b->dominationCount_;
	});

	std::cout << solutions[solutions.size()-1]->getGenesAsString() << ";";
	std::cout << solutions[0]->getGenesAsString();
}

std::vector<Job> CRO::importDefaultSample(std::string fileName)
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
		if (fileName.compare("dataset.txt") == 0) {
			content = std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		} else {
			std::stringstream result;

			int number_total_jobs, number_total_machines, number_max_operations;
			std::string line;
			std::getline(file, line);
			std::istringstream headerStream(line);
			headerStream >> number_total_jobs >> number_total_machines >> number_max_operations;

			// Set Job Id to 1 to initiate dataset load
			int currentJob = 1;

			while (std::getline(file, line)) {
				if (currentJob > number_total_jobs) {
					break;
				}


				std::istringstream lineStream(line);
				int number_operations;
				lineStream >> number_operations;

				for (int id_operation = 0; id_operation < number_operations; ++id_operation) {
					result << currentJob << "," << id_operation+1 << ":";
					int machinesNumber;
					lineStream >> machinesNumber;
					std::map<int,int> machinesMap;
					for (int i = 1; i <= number_total_machines; i++) {
						machinesMap.insert(std::make_pair(i,100));
					}
					for (int i = 0; i < machinesNumber; i++) {
						int machine, time;
						lineStream >> machine >> time;
						machinesMap[machine] = time;
					}

					for (int i = 1; i <= number_total_machines; i++) {
						if (i == number_total_machines) {
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

std::vector<Job> CRO::generateJobs(int numberOfJobs, int numberOfMachines, int numberOfProcesses) 
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

		for (int machineIndex = 0; machineIndex < numberOfMachines; ++machineIndex) 
		{
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

void CRO::splitJobs(std::vector<int>& firstGroup, std::vector<int>& secondGroup) 
{
	// Original vector of integers -> testing
	std::vector<int> originalVector(jobs_.size());
	std::iota(originalVector.begin(), originalVector.end(), 1);

	// Random number generator
	std::random_device rd;
	std::mt19937 gen(rd());

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

void CRO::printJobs(std::vector<Job> jobs)
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

void CRO::outputJobs(std::vector<Job> jobs)
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
						if(j + 1 == jobs[i].processes.size())
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

void CRO::printPopulation() 
{
	for (int i = 0; i < reefSize_; i++) 
	{
        for (int j = 0; j < reefSize_; j++) 
		{
			if (reef_[i][j]) 
			{
				std::cout << *reef_[i][j] << std::endl;
			}
        }
    }
}

void CRO::printPopulationGrid() 
{
	for (int i = 0; i < reefSize_; i++) 
	{
        for (int j = 0; j < reefSize_; j++) 
		{
			if (reef_[i][j]) 
			{
				std::cout << "1 ";
			} else std::cout << "0 ";
        }
		std::cout << std::endl;
    }
}

int main(int argc, char* argv[])
{
	bool command_line_args = true;

	int numberOfJobs, numberOfMachines, numberOfProcesses;
	int reefSize;
	int generations;
	std::string useDefault;

	if (command_line_args) 
	{
		// Check if there are at least two arguments (executable name + 5 user arguments)
		if (argc >= 7) {

			numberOfJobs = std::stoi(argv[1]);

			numberOfMachines = std::stoi(argv[2]);

			numberOfProcesses = std::stoi(argv[3]);

			reefSize = std::stoi(argv[4]);

			generations = std::stoi(argv[5]);
			
			// Sample data
			useDefault = std::string(argv[6]);
		}
		else 
		{
			// Print an error message if there are not enough arguments
			std::cout << "Usage: " << argv[0] << " <numberOfJobs>" << " <numberOfMachines>" << " <numberOfProcesses>" << " <reefSize>" << " <generations>" << " <useDefault>. Only " << argc << " args provided." << std::endl;
			return 0;
		}
	}
	else 
	{
		std::cout << "Enter number of jobs: ";
		std::cin >> numberOfJobs;

		std::cout << "Enter number of machines: ";
		std::cin >> numberOfMachines;
		
		std::cout << "Input number of processes: ";
		std::cin >> numberOfProcesses;

		std::cout << "Input reefSize: ";
		std::cin >> reefSize;

		std::cout << "Input number of generations: ";
		std::cin >> generations;
	}

	std::unique_ptr<CRO> workshop = std::make_unique<CRO>(numberOfJobs, numberOfMachines, numberOfProcesses, reefSize, generations, useDefault);
	workshop->run();
	return 0;
}