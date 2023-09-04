import React, { useState } from 'react'
import './style.css'

const Toolbar = ({ 
    algorithm,
    setAlgorithm,
    runScheduling, 
    reset,
    numberOfJobs, 
    setNumberOfJobs,
    numberOfMachines,
    setNumberOfMachines,
    generations,
    setGenerations,
    populationSize,
    setPopulationSize,
    numberOfProcesses,
    setNumberOfProcesses,
    completionTime,
    equipmentLoad,
    setDefaultSample,
    loading
 }) => {

    const [isDefaultSelected, setIsDefaultSelected] = useState(true);
    const [isRandomSelected, setIsRandomSelected] = useState(false);

    const handleDefaultSelector = () => {
        setIsDefaultSelected(true);
        setIsRandomSelected(false);
        setDefaultSample(1);
    }

    const handleRandomSelector = () => {
        setIsDefaultSelected(false);
        setIsRandomSelected(true);
        setDefaultSample(0);
    }

  return (
    <div className='toolbar-container'>
        <div className='toolbar-header'>
            <div className='toolbar-title'>
                Job-Shop Scheduling
            </div>
        </div>
        <div className='problem-configuration-panel'>
            <div className='problem-input'>
                <div>
                    Select the algorithm:
                </div>
                <input
                    type="radio"
                    name="algorithm"
                    value="nsga"
                    checked={algorithm === 'nsga'}
                    onChange={() => setAlgorithm('nsga')}
                    />
                    <b>NSGA-II</b>
                <br />
                <input
                    type="radio"
                    name="algorithm"
                    value="cro"
                    checked={algorithm === 'cro'}
                    onChange={() => setAlgorithm('cro')}
                    />
                    <b>MO-CROA</b>
            </div>
            <div className='instance-configurator'>
                <div className='instance-type-container'>
                    <div className={isDefaultSelected ? 'instance-selector selected' : 'instance-selector'} onClick={handleDefaultSelector}>
                        Default sample
                    </div>
                    <div className={isRandomSelected ? 'instance-selector selected' : 'instance-selector'} onClick={handleRandomSelector}>
                        Random instance
                    </div>
                </div>
                <form>
                    <div className='problem-input'>
                        <label htmlFor="numberOfJobs">Number of Jobs</label><br/>
                        <input disabled={isDefaultSelected} type="text" id="numberOfJobs" name="numberOfJobs" required value={numberOfJobs} onChange={(e) => setNumberOfJobs(e.target.value)}/>
                    </div>

                    <div className='problem-input'>
                        <label htmlFor="numberOfMachines">Number of Machines</label><br/>
                        <input disabled={isDefaultSelected} type="text" id="numberOfMachines" name="numberOfMachines" required value={numberOfMachines} onChange={(e) => setNumberOfMachines(e.target.value)}/>
                    </div>

                    <div className='problem-input'>
                        <label htmlFor="numberOfProcesses">Number of Processes</label><br/>
                        <input disabled={isDefaultSelected} type="text" id="numberOfProcesses" name="numberOfProcesses" required value={numberOfProcesses} onChange={(e) => setNumberOfProcesses(e.target.value)}/>
                    </div>

                    <div className='problem-input'>
                        <label htmlFor="generations">Number of Generations</label><br/>
                        <input type="text" id="generations" name="generations" required value={generations} onChange={(e) => setGenerations(e.target.value)}/>
                    </div>

                    <div className='problem-input'>
                        <label htmlFor="populationSize">{algorithm === 'nsga' ? 'Population Size' : 'Reef size'}</label><br/>
                        <input type="text" id="populationSize" name="populationSize" required value={populationSize} onChange={(e) => setPopulationSize(e.target.value)}/>
                    </div>
                </form>
            </div>
            <div className='toolbar-run-button'>
                <button onClick={runScheduling}>Run Scheduling</button>
                <button onClick={reset}>Reset</button>
            </div>
        </div>
        {
        loading ?
        <div className='toolbar-results'>
            Running Scheduling Optimization... <br/>
            Please wait
        </div>
        :
        completionTime !== 0 ?
            <div className='toolbar-results'>
                Completion time: {completionTime} minutes <br/>
                Total equipment load: {equipmentLoad}
            </div>
            :
            <></>
        }
    </div>
  )
}

export default Toolbar