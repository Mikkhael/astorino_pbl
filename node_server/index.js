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

const CropSize = 8;

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
    const [avg, div] = analyseImage(payload, CropSize);
    updateCurrentlySeenColor(avg, div, CropSize);

  }else{
    console.error(`Unhandled topic: `, topic);
  }
}


function updateCurrentlySeenColor(avg, div, cropSize){
  const payload = {
    avg, div, cropSize
  };
  const payload_str = JSON.stringify(payload);
  mqttClient.publish("analysis", payload_str);
}

const Width = 320;
const Height = 240;
const cameraImageCanvas = canvas.createCanvas(Width, Height);
const ctx = cameraImageCanvas.getContext("2d");


/**
 * @param {Buffer} jpegBytes 
 */
function analyseImage(jpegBytes, CropSize){
  const img = new canvas.Image();
  img.src = jpegBytes;
  ctx.drawImage(img, 0,0, Width, Height);
  //const buf = cameraImageCanvas.toBuffer();
  //fs.writeFileSync("test.jpg", buf);
  const pixels = ctx.getImageData(Width/2-CropSize, Height/2-CropSize, CropSize*2, CropSize*2);
  const avg = [0,0,0];
  for(let i=0; i<pixels.data.length; i+=4){
    for(let j=0; j<3; j++){
      avg[j] += pixels.data[i+j];
    }
  }
  for(let j=0; j<3; j++){
    avg[j] /= (pixels.data.length / 4);
  }
  const div = [0,0,0];
  for(let i=0; i<pixels.data.length; i+=4){
    for(let j=0; j<3; j++){
      div[j] += (pixels.data[i+j] - avg[j])**2;
    }
  }
  for(let j=0; j<3; j++){
    div[j] /= (pixels.data.length / 4);
    div[j] = Math.sqrt(div[j]);
  }
  return [avg, div];
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