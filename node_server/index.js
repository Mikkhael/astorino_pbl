//@ts-check
// Include all the libraries
const express = require("express"); // HTTP server
const WebSocket = require("ws"); // WebSocket (asynchronous client-server communication)
const fs = require("fs"); // FileSystem library, for testing
const mqtt = require("mqtt"); // MQTT library
const canvas = require("canvas");


// Config

let Config = {};
try{
  Config = require("./config.js");
}catch(err){
  console.error("You are missing config.js file. Reffer to README.txt");
  // Default config:
  Config = {
    mqtt_user: "user",
    mqtt_pass: "password",
    mqtt_addr: "localhost",
    mqtt_port: 1883,
    http_port: 80,
  };
}



////////////////////////// MQTT ////////////////////////

// Array of topics to subscribe to
const mqtt_topics = [
  `img/jpeg`,
  'topic_testowy'
];

// Creating a connection
const mqtt_brokerurl = `mqtt://${Config.mqtt_addr}:${Config.mqtt_port}`; // Make the URL to connect to
const mqttClient = mqtt.connect(mqtt_brokerurl, { // Connect with specified username and password
  username: Config.mqtt_user,
  password: Config.mqtt_pass,
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
const http_server = app.listen(Config.http_port, () => {
  console.log(`[HTTP] Listening on port ${Config.http_port}`);
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

// Size of the cropped area of the image to analyse
const cropSize = 8;

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
    const [avg, dev] = analyseImage(payload, cropSize);
    broadcastImageAnalysis(avg, dev, cropSize);

  }else{
    console.error(`Unhandled topic: `, topic);
  }
}

function broadcastImageAnalysis(avg, dev, cropSize){
  const payload = { // Create an object with image analysis results
    avg, dev, cropSize
  };
  const payload_str = JSON.stringify(payload); // Convert the object to a string
  mqttClient.publish("analysis", payload_str); // Publish the message
}

// Canvas for image processing
const Width = 320;
const Height = 240;
const cameraImageCanvas = canvas.createCanvas(Width, Height);
const ctx = cameraImageCanvas.getContext("2d");


/**
 * @param {Buffer} jpegBytes 
 * @param {number} cropSize
 */
function analyseImage(jpegBytes, cropSize){
  // Convert jpegBytes to an Image
  const img = new canvas.Image();
  img.src = jpegBytes;
  // Draw the image on canvas and extract pixels
  ctx.drawImage(img, 0,0, Width, Height);
  const pixels = ctx.getImageData(Width/2-cropSize, Height/2-cropSize, cropSize*2, cropSize*2).data; // Creatting a cropped out square at the center
  // Analyse pixel data
  const avg = [0,0,0]; // Average pixel color of the cropped section
  const dev = [0,0,0]; // Standard deviation of pixel colors
  for(let pixel_i=0; pixel_i < pixels.length; pixel_i += 4){ // Iterate over all pixels (RGBA)
    for(let channel=0; channel<3; channel++){ // Iterate over all channels
      avg[channel] += pixels[pixel_i+channel];
      dev[channel] += pixels[pixel_i+channel]**2;
    }
  }
  for(let channel=0; channel<3; channel++){ // Calculate final values
    avg[channel] /= pixels.length / 4;
    dev[channel] /= pixels.length / 4;
    dev[channel] -= avg[channel]**2;
  }
  return [avg, dev];
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