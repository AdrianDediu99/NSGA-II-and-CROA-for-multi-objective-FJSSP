#pragma once

#include <vector>
#include <map>

class Job 
{
public:
    struct Process 
    {
        std::map<int, int> machineDurations;

        Process(const std::map<int, int>& durations) : machineDurations(durations) {}
    };

    std::vector<Process> processes;

    Job();
    Job(const std::vector<Process>& processes);
};
