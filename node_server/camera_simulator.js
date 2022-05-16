//@ts-check
const canvas = require("canvas");
const mqtt = require("mqtt");
const fs = require("fs");

// Connect to MQTT broker
const mqttClient = mqtt.connect("mqtt://localhost:1883", {
    username: "user",
    password: "password"
});
mqttClient.on("error", (err) => console.log("MQTT", err));
mqttClient.on("connect", () => console.log("MQTT Connected."));

// Setup drawing canvas
const WIDTH = 320;
const HEIGHT = 240;
const camera_screen = canvas.createCanvas(WIDTH, HEIGHT);
const ctx = camera_screen.getContext("2d");
ctx.clearRect(0,0, WIDTH, HEIGHT);

function drawRandomRectangle(){
    // Randomize rectangle position, size and color
    const x = Math.random() * WIDTH;
    const y = Math.random() * HEIGHT;
    const w = Math.random() * (WIDTH - x);
    const h = Math.random() * (HEIGHT - y);
    const color = `rgb(${Math.floor(Math.random() * 255)}, ${Math.floor(Math.random() * 255)}, ${Math.floor(Math.random() * 255)})`;

    // Draw the rectangle
    ctx.fillStyle = color;
    ctx.fillRect(x, y, w, h);
}

function sendJpeg(){
    if(!mqttClient.connected){ // Check if MQTT is connected
        console.error("MQTT disconnected. Cannot send Jpeg");
        return false;
    }
    const data = camera_screen.toBuffer("image/jpeg");
    mqttClient.publish("img/jpeg", data);
    return true;
}

const sendInterval = 5000; // Sending interval (in milliseconds)
setInterval(() => { // Perform the image sending repeatidly
    drawRandomRectangle();
    sendJpeg();
    console.log("Image sent.");
    // console.log('<img src="' + camera_screen.toDataURL() + '" />');
}, sendInterval);