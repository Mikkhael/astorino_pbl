//@ts-check
// Include all the libraries
const express = require("express"); // HTTP server
const WebSocket = require("ws"); // WebSocket (asynchronous client-server communication)
const fs = require("fs"); // FileSystem library, for testing
const mqtt = require("mqtt"); // MQTT library
const Modbus = require("./modbus.js") // Modbus Module
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
    mb_addr: "192.168.0.102",
    mb_port: 502,
    mb_keepalive: 5000,
    mb_reconnectTimeout: 1000,
    mb_updateInterval: 1000,
  };
}

////////////////////////// ModBus ////////////////////////

const mbclient = new Modbus(
  Config.mb_addr,
  Config.mb_port,
  Config.mb_keepalive,
  Config.mb_reconnectTimeout
);

mbclient.getCameraColor = function(){
  return detectedcolor;
}

mbclient.onAssemblyCompleted = function(requestId, request){
    console.log("[MBUS]", `Completed Request Id: ${requestId}, Colors: ${request.bottomColor} | ${request.topColor}`);
}
mbclient.onAssemblyRequestSend = function(requestId, request, err){
    console.log("[MBUS]", `Sent Request Id: ${requestId}, Colors: ${request.bottomColor} | ${request.topColor}`);
    if(err){
        console.error("[MBUS] Assembly ", err);
    }
}
mbclient.onColorUpdateSend = function(finishedId, color, err){
    if(err){
        console.error("[MBUS] Color Request", err);
    }
};
mbclient.onPollError = function(err){
    console.error(err);
};

mbclient.connect();
const mbclientLoopInterval = setInterval(mbclient.loop.bind(mbclient), Config.mb_updateInterval);



////////////////////////// MQTT ////////////////////////

// Array of topics to subscribe to
const mqtt_topics = [
  `img/jpeg`,
  'assemblyRequest',
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

// Size of the cropped area of the image to analyse przewidziana wielkość przesyłangeo obrazu
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
    const [avg, dev, hsl] = analyseImage(payload, cropSize);
    broadcastImageAnalysis(avg, dev, cropSize, hsl[0], hsl[1], hsl[2], detectedcolor);

  }else if(topic == "assemblyRequest"){
    const request = JSON.parse(payload.toString());
    request.bottomColor = +request.bottomColor || 0;
    request.topColor = +request.topColor || 0;
    console.log(`Received new Assembly Request for colors:`, request);
    mbclient.enqueueRequest(request.bottomColor, request.topColor);

  }else{
    console.error(`Unhandled topic: `, topic);
  }
}

function broadcastState(){
  const payload = {plcConnected: mbclient.isConnected, plcState: mbclient.plcState};
  const payload_str = JSON.stringify(payload); // Convert the object to a string
  mqttClient.publish("serverstate", payload_str); // Publish the message
}
let boradcastStateInterval = setInterval(broadcastState, 500);

function broadcastImageAnalysis(avg, dev, cropSize, h, s, l, col){
  const payload = { // Create an object with image analysis results
    avg, dev, cropSize, h, s, l, col
  };
  const payload_str = JSON.stringify(payload); // Convert the object to a string
  mqttClient.publish("analysis", payload_str); // Publish the message
}

// Canvas for image processing
const Width = 320;
const Height = 240;
const cameraImageCanvas = canvas.createCanvas(Width, Height);
const ctx = cameraImageCanvas.getContext("2d");

let detectedcolor = 3; // Depended of h value, 0-blue,1-green,2-violet

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
  matchcolor(avg);
  let hsl = rgbToHsl(avg);
  return [avg, dev, hsl, detectedcolor];
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

   // const rangehighR;
   function matchcolor(avg)
   {
	   //218,200,171.25


	    let rangehighR=250;
		let rangelowR=190;
		let rangehighG=230;
		let rangelowG=170;
		let rangehighB=190;
		let rangelowB=150;

		const avgR = avg[0];
		const avgG = avg[1];
		const avgB = avg[2];

		if(avgR <= rangehighR  && avgR >= rangelowR)
		{
			
			
			if(avgG <= rangehighG  && avgG >= rangelowG)
			{
				if(avgB <= rangehighG  && avgB >= rangelowB)
				{
				//console.log("Wykryty kolor: pomarańczowy ");		
				}
			}
		}
   }


	function rgbToHsl(avg) {
	  let r=avg[0];
	  let g=avg[1];
	  let b=avg[2];
	  
	  
	  r /= 255, g /= 255, b /= 255;

	  var max = Math.max(r, g, b), min = Math.min(r, g, b);
	  var h = 0, s = 0, l = (max + min) / 2;

	  if (max == min) {
		h = s = 0; // achromatic
	  } else {
		var d = max - min;
		s = l > 0.5 ? d / (2 - max - min) : d / (max + min);

		switch (max) {
		  case r: h = (g - b) / d + (g < b ? 6 : 0); break;
		  case g: h = (b - r) / d + 2; break;
		  case b: h = (r - g) / d + 4; break;
		}

		h /= 6;
	  }
	  console.log("Wartość h:" +h);
	  console.log("Wartość s:" +s);
	  console.log("Wartość l:" +l);
	  
	  if(h >= 0.545 && h <= 0.610)
	  {
		  console.log("To jest niebieski");
		  detectedcolor = 0;
    }
	  else if(h >= 0.275 && h <= 0.380)
	  {
		  console.log("To jest zielony");
      detectedcolor = 1;
	  }
	  else if(h >= 0.675 && h <= 0.725)
	  {
		  console.log("To jest fioletowy");
      detectedcolor = 2;
	  }
    else{
      detectedcolor = 3;
    }
	  
		  
	  
	  return [ h, s, l , detectedcolor];
	}