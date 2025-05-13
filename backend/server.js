const express = require('express');
const bodyParser = require('body-parser');
const uploadRoutes = require('./routes/upload');

// Initialize Express app
const app = express();
const PORT = process.env.PORT || 5000;

// Middleware
app.use(bodyParser.json()); // To parse JSON requests

// Routes
app.use(uploadRoutes); // Use the upload routes

// Start the server
app.listen(PORT, () => {
  console.log(`Server running on port ${PORT}`);
});
