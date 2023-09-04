class Job {
    constructor(processes) {
        this.processes = processes;
    }
}

class Process {
    constructor(machineDurations) {
        var numberArray = [];
        for (var i = 0; i < machineDurations.length; i++)
            numberArray.push(parseInt(machineDurations[i]));
        this.machineDurations = numberArray;
    }
}

module.exports = { Job, Process };