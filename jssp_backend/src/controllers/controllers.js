import { spawn } from 'child_process';

export async function uploadDataset(req, res) {
    if (!req.file) {
        return res.status(400).send('No file uploaded.');
    }

    res.status(200).send('File uploaded successfully.');
}

export async function runScheduling(req, res) {
    const { algorithm, numberOfJobs, numberOfMachines, generations, populationSize, numberOfProcesses, defaultSample } = req.body;

    // Path to C++ executable
    const cppExecutablePath = `${process.env.EXE_PATH}${algorithm}.exe`;

    // Input data to pass to the C++ executable
    const argumentsArray = [numberOfJobs, numberOfMachines, numberOfProcesses, populationSize, generations, defaultSample];

    let options = {
        cwd: `${process.env.EXE_PATH}`
    };

    // Spawn the C++ executable as a separate process
    const childProcess = spawn(cppExecutablePath, argumentsArray, options);

    // Listen for output from the C++ executable
    let outputData = '';
    childProcess.stdout.on('data', data => {
        outputData += data;
    });

    // Handle process completion
    childProcess.on('close', code => {
        if (code === 0) {
            // Process completed successfully
            res.status(200).json({ message: outputData });
        } else {
            // Process encountered an error
            console.error('C++ process encountered an error. CODE: ' + code + " , OUTPUT: " + outputData);
            res.status(500).json({ error: 'Internal Server Error' });
        }
    });
}