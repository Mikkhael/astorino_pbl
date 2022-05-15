const express = require("express");
const WebSocket = require("ws");
const fs = require("fs");
const mqtt = require("mqtt");

const clientId = `mqtt_${Math.random().toString(16).slice(3)}`;
const mqttClient = mqtt.connect("mqtt:/localhost:1883", {
  clientId,
  clean: true,
  connectTimeout: 4000,
  username: 'user',
  password: 'password',
  reconnectPeriod: 1000,
})

mqttClient.on('connect', () => {
  console.log('Connected MQTT')
  mqttClient.subscribe(['img/jpeg'], () => {
    console.log(`Subscribe to topic img/jpeg`)
  })
});


const app = express();
const port = 80;

app.use('/', express.static('.'));

const server = app.listen(port, () => {
  console.log(`Example app listening on port ${port}`);
})


const wss = new WebSocket.Server({
	server: server,
	clientTracking: true,
});

mqttClient.on('message', (topic, payload) => {
  console.log('Received Message:', topic);
  if(topic == 'img/jpeg'){
	for(let ws of wss.clients){
		ws.send(payload);
	}
  }
})

wss.on("connection", (ws) => {
	console.log(`Connected`);
});