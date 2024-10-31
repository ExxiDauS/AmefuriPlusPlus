import express from "express";
import bodyParser from "body-parser";
import mqtt from "mqtt";

import path from "path";
import { fileURLToPath } from "url";

const PORT = process.env.PORT || 3000;

const app = express();


app.use(bodyParser.json());
app.use(bodyParser.urlencoded({ extended: true }));

const brokerUrl = "mqtt://mqtt-dashboard.com:1883"; // Example public broker
const mqttClient = mqtt.connect(brokerUrl);

let currentMessage = "";

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

// Handle connection to the MQTT broker
mqttClient.on("connect", () => {
  console.log("Connected to MQTT broker");
  mqttClient.subscribe("Amefuri/plus", (err) => {
    if (!err) {
      console.log("Subscribed to topic: Amefuri/plus");
    }
  });
});

// Listen for messages on the subscribed topic
mqttClient.on("message", (topic, message) => {
  currentMessage = message.toString();
  console.log(`Received message on topic ${topic}: ${currentMessage}`);
});

// Serve static files and HTML page
app.use(express.static("public"));

app.get("/events", (req, res) => {
  // Set headers for SSE
  res.setHeader("Content-Type", "text/event-stream");
  res.setHeader("Cache-Control", "no-cache");
  res.setHeader("Connection", "keep-alive");

  // Send a message when a new message is received
  const sendMessage = () => {
    res.write(`data: ${currentMessage}\n\n`);
  };

  // Emit the current message when the client connects
  sendMessage();

  // Forward messages to the client when new ones arrive
  mqttClient.on("message", sendMessage);

  // Clean up when the connection is closed
  req.on("close", () => {
    mqttClient.off("message", sendMessage);
    res.end();
  });
});

// Basic route to serve the HTML page
app.get("/", (req, res) => {
  res.sendFile(path.join(__dirname, "index.html"));
});

// Start the server
app.listen(PORT, () => {
  console.log(`Express server is running on http://localhost:${PORT}`);
});



