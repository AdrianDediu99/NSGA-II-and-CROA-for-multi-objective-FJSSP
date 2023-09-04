import { Router } from 'express';
import { runScheduling } from '../controllers/controllers.js';

const routes = Router();

routes.post('/run-scheduling', runScheduling);

export default routes;