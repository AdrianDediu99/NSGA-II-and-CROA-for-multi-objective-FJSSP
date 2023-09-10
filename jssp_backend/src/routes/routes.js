import { Router } from 'express';
import { runScheduling, uploadDataset } from '../controllers/controllers.js';
import multer from 'multer'

// Configure storage
const storage = multer.diskStorage({
    destination: (req, file, cb) => {
        cb(null, `${process.env.EXE_PATH}`)
    },
    filename: (req, file, cb) => {
        cb(null, file.originalname)
    }
});

const upload = multer({ storage: storage });

const routes = Router();

routes.post('/upload-dataset',  upload.single('file'), uploadDataset)
routes.post('/run-scheduling', runScheduling);

export default routes;