import { spawn } from 'child_process';

export async function runScheduling(req, res) {
    const { algorithm, numberOfJobs, numberOfMachines, generations, populationSize, numberOfProcesses, defaultSample } = req.body;

    console.log("Selected algorithm: ", algorithm);

    console.log("ROOT PATH: ", process.env.EXE_PATH);

    // Path to your C++ executable
    const cppExecutablePath = `${process.env.EXE_PATH}${algorithm}.exe`;
    console.log("EXE PATH: ", cppExecutablePath);

    // Input data to pass to the C++ executable

    const argumentsArray = [numberOfJobs, numberOfMachines, numberOfProcesses, populationSize, generations, defaultSample];

    // Spawn the C++ executable as a separate process
    const childProcess = spawn(cppExecutablePath, argumentsArray);

    // Listen for output from the C++ executable
    let outputData = '';
    childProcess.stdout.on('data', data => {
        outputData += data;
    });

    // Handle process completion
    childProcess.on('close', code => {
        if (code === 0) {
            // Process completed successfully
            console.log('C++ process completed successfully.');
            console.log('Output:', outputData);
            res.status(200).json({ message: outputData });
        } else {
            // Process encountered an error
            console.error('C++ process encountered an error. CODE: ' + code + " , OUTPUT: " + outputData);
            res.status(500).json({ error: 'Internal Server Error' });
        }
    });
    
}