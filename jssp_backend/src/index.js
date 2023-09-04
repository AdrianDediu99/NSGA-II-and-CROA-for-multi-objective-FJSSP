import express, { json } from 'express';
import cors from 'cors';
import routes from './routes/routes.js';
import dotenv from 'dotenv'

dotenv.config();

const app = express();
app.use(json());
app.use(cors({
  credentials: true,
  origin: 'http://localhost:3000'
}));

app.use(function(req, res, next) {
  res.header("Access-Control-Allow-Origin", "*");
  res.header("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
  next();
});

// Routes
app.use('/api/', routes);

// Start the Server
app.listen(8000, () => {
  console.log('Server running on port ' + 8000);
});