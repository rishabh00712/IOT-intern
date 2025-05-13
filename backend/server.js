const express = require('express');
const http = require('http');
const WebSocket = require('ws');
const bodyParser = require('body-parser');
const uploadRoutes = require('./routes/upload'); // Import the routes

const app = express();
const server = http.createServer(app);
const wss = new WebSocket.Server({ server });

// Middleware
app.use(bodyParser.json());

// Store ESP32 clients
let esp32Clients = [];

wss.on('connection', (ws) => {
  console.log('ESP32 connected via WebSocket');
  esp32Clients.push(ws);

  ws.on('close', () => {
    esp32Clients = esp32Clients.filter((client) => client !== ws);
    console.log('ESP32 disconnected');
  });
});

// Broadcast firmware URL to all connected ESP32 devices
function notifyESP32Clients(firmwareData) {
  esp32Clients.forEach((client) => {
    if (client.readyState === WebSocket.OPEN) {
      client.send(JSON.stringify(firmwareData));
    }
  });
}

// Use the notifyESP32Clients in your upload routes by passing it as an argument
app.use('/api', uploadRoutes(notifyESP32Clients));  // POST /upload

// Start the server
const PORT = process.env.PORT || 5001;
server.listen(PORT, () => console.log(`Server running on port ${PORT}`));

