const express = require("express");
const app = express();
const cardRouter = require('./routes/card-auth.js')
const logsRouter = require('./routes/logs.js')
const connectDB = require('./db/connect');
require('dotenv').config();
const notFound = require('./middleware/not-found');
const errorHandlerMiddleware = require('./middleware/error-handler');

app.use(express.static('./public'));
app.use(express.json()); 
app.use("/api/cards", cardRouter);
app.use("/api/logs", logsRouter);


app.use(notFound);
app.use(errorHandlerMiddleware);
const port = process.env.PORT || 3000;



const start= async () => {
  try {
    await connectDB(process.env.MONGO_URI);
    app.listen(port, () =>
      console.log(`Server is listening on port ${port}...`)
    );
  } catch (error) {
    console.log(error);
  }
};

start();