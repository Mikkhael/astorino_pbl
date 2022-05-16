//@ts-check
// Include all the libraries
const express = require("express"); // HTTP server
const WebSocket = require("ws"); // WebSocket (asynchronous client-server communication)
const fs = require("fs"); // FileSystem library, for testing
const mqtt = require("mqtt"); // MQTT library


// Config
const mqtt_user = "user";
const mqtt_pass = "password";
const mqtt_addr = "localhost";
const mqtt_port = 1883;

const http_port = 80;



////////////////////////// MQTT ////////////////////////

// Array of topics to subscribe to
const mqtt_topics = [
  `img/jpeg`,
  'topic_testowy'
];

// Creating a connection
const mqtt_brokerurl = `mqtt://${mqtt_addr}:${mqtt_port}`; // Make the URL to connect to
const mqttClient = mqtt.connect(mqtt_brokerurl, { // Connect with specified username and password
  username: mqtt_user,
  password: mqtt_pass,
});

// MQTT events
mqttClient.on('connect', () => {
  console.log(`[MQTT] Connected to ${mqtt_brokerurl}`)
  // Subscribing to topics
  mqttClient.subscribe(mqtt_topics, (err) => {
    if (err) { // Check if error occured
      console.error(`[MQTT] Faild to subscribe: `, err);
      return;
    }
    console.log(`[MQTT] Subscribed successfully.`);
  })
});
mqttClient.on("close", () => {
  console.log(`[MQTT] Disconnected.`);
});
mqttClient.on("error", (err) => {
  console.error("[MQTT]", err);
});
mqttClient.on('message', (topic, payload) => {
  console.log('[MQTT] Received Message from topic: ', topic);
  handleMqttMessage(topic, payload);
});

////////////////////////// HTTP ////////////////////////
const app = express(); // Creating the HTTP server
app.use('/', express.static('public')); // Serving the contents of "public" as a website

// Starting the HTTP server
const http_server = app.listen(http_port, () => {
  console.log(`[HTTP] Listening on port ${http_port}`);
});

////////////////////////// WebSockets ////////////////////////
const wss = new WebSocket.Server({ // Starting the WebSockets server
  server: http_server, // Bound HTTP server
  clientTracking: true, // Enable client tracking, to have a list of all connected clients (wss.clients)
});

// WebSockets Events
wss.on("connection", (ws, req) => { // New client (ws) connected to the server (wss)
  console.log(`[WSS] Connection from: `, req.socket.remoteAddress);
  ws.on("close", () =>{
    console.log(`[WSS] Disconnection from: ${req.socket.remoteAddress}`);
  });
  ws.on("error", (err) => {
    console.error(`[WSS]`, err);
  });
  ws.on("message", (data, isBinary) => {
    console.log(`Received ${isBinary ? "binary": "text"} message: `, data);
    // Here handle the incoming messages...
  });
});
wss.on("error", (err) => {
  console.error(`[WSS] Server`, err);
});

////////////////////////// Functions ////////////////////////

/**
 * @param {string} topic 
 * @param {Buffer} payload 
 */
function handleMqttMessage(topic, payload){

  if(topic == "topic_testowy"){
    console.log("Recived a test message: ", payload.toString());
  }else if(topic == "img/jpeg"){
    console.log("Received image.");
    broadcastNewImage(payload);
  }else{
    console.error(`Unhandled topic: `, topic);
  }

}

/**
 * @param {Buffer} data 
 */
function broadcastNewImage(data){
  for(let ws of wss.clients){ // Iterate over all elements of array "wss.clients" (in each iteration, "ws" is the next element in the array "ws.clients")

    // Create a message object to send
    const message = {
      type: "img", // The "type" property will allow the client to differenciate between... different types of messages :)
      data: data.toString("binary") // Since we cannot mix Buffers (byte arrays) and strings in WebSocket message, we convert to a binary string 
    };
    const message_str = JSON.stringify(message); // Convert the message object to a JSON string, easy for JavaScript to parse back into the message object, when the client recives it
    ws.send(message_str); // Send the message to the client

  }
}