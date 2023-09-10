import { GanttChart } from 'ibm-gantt-chart-react';
import axios from 'axios'
import 'ibm-gantt-chart/dist/ibm-gantt-chart.css';
import './App.css';
import { useEffect, useState } from 'react';
import { Job, Process } from './models/Problem';
import Toolbar from './components/Toolbar/Toolbar';

function App() {
  const [algorithm, setAlgorithm] = useState('nsga');
  const [dataset, setDataset] = useState([]);
  const [solutions, setSolutions] = useState([]);
  const [chartData, setChartData] = useState([]);
  const [config, setConfig] = useState();

  const [numberOfJobs, setNumberOfJobs] = useState(10);
  const [numberOfMachines, setNumberOfMachines] = useState(10);
  const [generations, setGenerations] = useState(50);
  const [populationSize, setPopulationSize] = useState(200);
  const [numberOfProcesses, setNumberOfProcesses] = useState(33);

  const [completionTime, setCompletionTime] = useState(0);
  const [equipmentLoad, setEquipmentLoad] = useState(0);

  const [defaultSample, setDefaultSample] = useState(1);
  const [loading, setLoading] = useState(false);

  const updateCustomDataset = (file) => {
    const reader = new FileReader();

    reader.onload = (e) => {
      const content = e.target.result; // This contains the file content as a string
      let temp = content.split('\t');
      setNumberOfJobs(temp[0]);
      setNumberOfMachines(temp[1]);

      let temp2 = content.split('\n')

      let operationsNum = 0;
      for (let i = 1; i <= temp[0]; i++) {
        operationsNum += parseInt(temp2[i].split(' ')[0]);
      }
      setNumberOfProcesses(operationsNum);
    };

    reader.readAsText(file);
  };

  const fillDataset = (response) => {
    setCompletionTime(0);
    setEquipmentLoad(0);
    setDataset([]);
    setSolutions([]);
    setChartData([]);
    var jobsArray = [];

    let temp = response.split(' ');

    setCompletionTime(temp[temp.length-2]);
    setEquipmentLoad(temp[temp.length-1]);

    var jobs = response.split('.')[0];
    var individuals = response.split('.')[1];

    const tempJobs = jobs.split(';');

    tempJobs.forEach(element => {
      var operation = element.split(':')[0];
      var durations = element.split(':')[1];
      var durationsArray = durations.split(',');
      const process = new Process(durationsArray);
      var jobId = parseInt(operation.split(',')[0]);
      if (jobsArray.length < jobId) {
        jobsArray.push(new Job([process]));
      } else {
        jobsArray[jobId - 1].processes.push(process);
      }
    });

    setDataset(jobsArray);
    var individualsArray = [];

    const tempIndividuals = individuals.split(';');
    
    tempIndividuals.map(individual => {
      var processesArr = [];
      var machinesArr = [];
      var genes = individual.split(' ');
      for (let i = 0; i < numberOfProcesses; i++) {
        processesArr.push(genes[i]);
      }
      for (let i = numberOfProcesses; i < numberOfProcesses*2; i++) {
        machinesArr.push(genes[i]);
      }

      let obj = {
        processes: processesArr,
        machines: machinesArr,
      }

      individualsArray.push(obj);
    });

    setSolutions(individualsArray);
  }

  useEffect(() => {
    if (solutions.length != 0) {
      fillChartData();
    }
  },[solutions])

  const fillChartData = () => {
    let data = [];

    var machinesRuntime = new Map(); // <machineId, machineLoad>
		var jobRuntime = new Map();
    var durationsMap = new Map();
    for (let processIndex = 0; processIndex < numberOfProcesses; processIndex++) {
      let startTime = [];
      let endTime = [];
      let jobId = solutions[1].processes[processIndex].split(',')[0];
      let processId = solutions[1].processes[processIndex].split(',')[1];  
      let machineId = solutions[1].machines[processIndex];      
      
      let cwt = dataset[parseInt(jobId)-1].processes[parseInt(processId)-1].machineDurations[parseInt(machineId)-1];

      if (!jobRuntime.has(jobId)) {
        jobRuntime.set(jobId, 0);
      }

      if (machinesRuntime.has(machineId)) {
        if (parseInt(jobRuntime.get(jobId)) > parseInt(machinesRuntime.get(machineId))) {
          const { hours: startHour, minutes:startMinutes } = convertMinutesToHoursAndMinutes(parseInt(jobRuntime.get(jobId)));
          startTime = [startHour, startMinutes]
          
          let newRuntime = parseInt(jobRuntime.get(jobId)) + cwt;

          const { hours: endHour, minutes: endMinutes } = convertMinutesToHoursAndMinutes(newRuntime);
          endTime = [endHour, endMinutes];

          jobRuntime.set(jobId, newRuntime);
          machinesRuntime.set(machineId, newRuntime);
        } else {
          const { hours: startHour, minutes:startMinutes } = convertMinutesToHoursAndMinutes(parseInt(machinesRuntime.get(machineId)));
          startTime = [startHour, startMinutes]

          let newRuntime = parseInt(machinesRuntime.get(machineId)) + cwt;

          const { hours: endHour, minutes: endMinutes } = convertMinutesToHoursAndMinutes(newRuntime);
          endTime = [endHour, endMinutes];
          
          machinesRuntime.set(machineId, newRuntime);
          jobRuntime.set(jobId, newRuntime);
        }
      } else {
        let newRuntime = parseInt(jobRuntime.get(jobId)) + cwt;
        machinesRuntime.set(machineId, newRuntime);
        
        const { hours: startHour, minutes:startMinutes } = convertMinutesToHoursAndMinutes(parseInt(jobRuntime.get(jobId))); // maybe already int (TBC)
        startTime = [startHour, startMinutes];

        jobRuntime.set(jobId, newRuntime);

        const { hours: endHour, minutes:endMinutes } = convertMinutesToHoursAndMinutes(parseInt(jobRuntime.get(jobId))); // maybe already int (TBC)
        endTime = [endHour, endMinutes];
      }
      let activity = {
        id: jobId + processId,
        name: jobId + "," + processId + '=' + cwt,
        start: new Date(`11/21/1987 00:${startTime[0]}:${startTime[1]}`).getTime(),
        end: new Date(`11/21/1987 00:${endTime[0]}:${endTime[1]}`).getTime(),
      }
      
      if (durationsMap.has(parseInt(machineId))) {
        let existingActivities = durationsMap.get(parseInt(machineId));
        existingActivities.push(activity);
        durationsMap.set(parseInt(machineId), existingActivities);
      } else {
        durationsMap.set(parseInt(machineId), [activity]);
      }
    }
    
    const sortedArray = Array.from(durationsMap).sort((a, b) => a[0] - b[0]);
    var sortedDurationMap = new Map(sortedArray);

    sortedDurationMap.forEach((value, key) => {
      let task = {
        id: key.toString(),
        name: 'Machine ' + key.toString(),
        activities: value
      }
      data.push(task);
    })

    setChartData(data);
    
    const configuration = {
      data: {
        resources: {
          data: data,
          activities: 'activities',
          name: 'name',
          id: 'id',
        },
        activities: {
          start: 'start',
          end: 'end',
          name: 'name',
        },
      },
    }
    setConfig(configuration);
  }

  function convertMinutesToHoursAndMinutes(totalMinutes) {
    const hours = Math.floor(totalMinutes / 60);
    const minutes = totalMinutes % 60;
  
    // Add leading zeros if needed
    const formattedHours = hours.toString().padStart(2, '0');
    const formattedMinutes = minutes.toString().padStart(2, '0');
  
    return { hours: formattedHours, minutes: formattedMinutes };
  }
  
  const runScheduling = async () => {
    setLoading(true);
    await axios.post("http://localhost:8000/api/run-scheduling", {
      algorithm: algorithm,
      numberOfJobs: numberOfJobs,
      numberOfMachines: numberOfMachines,
      generations: generations,
      populationSize: populationSize,
      numberOfProcesses: numberOfProcesses,
      defaultSample: defaultSample
    }, { withCredentials: true })
      .then(response => {
        setLoading(false);
        if (response.status === 200) {
            fillDataset(response.data.message);
        }
      })
      .catch((error) => {
        setLoading(false);
      })
  }

  const reset = () => {
    setConfig({});
    setCompletionTime(0);
    setEquipmentLoad(0);
    setDataset([]);
    setSolutions([]);
    setChartData([]);
    setDefaultSample(1)
  }

  useEffect(() => {
    if (algorithm === 'nsga') {
      setPopulationSize(200);
    } else setPopulationSize(10);
  },[algorithm])

  useEffect(() => {
    if (defaultSample === 1) {
      setNumberOfJobs(10);
      setNumberOfMachines(10);
      setNumberOfProcesses(33);
      setGenerations(50);
      if (algorithm === 'nsga') {
        setPopulationSize(200);
      } else setPopulationSize(10);
    }
  },[defaultSample])

  return (
    <div className="App">
      <Toolbar algorithm={algorithm}
              setAlgorithm={setAlgorithm} 
              runScheduling={runScheduling} 
              reset={reset}
              numberOfJobs={numberOfJobs} 
              setNumberOfJobs={setNumberOfJobs}
              numberOfMachines={numberOfMachines}
              setNumberOfMachines={setNumberOfMachines}
              generations={generations}
              setGenerations={setGenerations}
              populationSize={populationSize}
              setPopulationSize={setPopulationSize}
              numberOfProcesses={numberOfProcesses}
              setNumberOfProcesses={setNumberOfProcesses}
              completionTime={completionTime}
              equipmentLoad={equipmentLoad}
              defaultSample={defaultSample}
              setDefaultSample={setDefaultSample}
              updateCustomDataset={updateCustomDataset}
              loading={loading}
      />
      <GanttChart config={config} style={{height: `${30 * chartData.length + 60}px`}} />
    </div>
  );
}

export default App;