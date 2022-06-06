
const mqtt_user = "user";
const mqtt_pass = "password";
const mqtt_port = 8080;
const mqtt_id   = "mqttws_" + Math.floor(Math.random() * 10000000); // Generating a random mqtt client id

if(location.hostname.match(/^(?:\d{1,3}\.){3}\d{1,3}$/)){
    alert(`You cannot connect to MQTT over WebSockets with ipv4 address (${location.hostname}). Use ipv6 or a domain name.`);
}

const mqtt_url = `ws://${location.hostname}:${mqtt_port}/mqtt`; // Create a URL for MQTT over WebSockets


let mqtt_messageHandler = () => {};
function MQTT_setMessageHandler(handler){
    mqtt_messageHandler = handler;
} 

function MQTT_createClientAndConnect(){
    const mqttClient = new Paho.MQTT.Client(mqtt_url, mqtt_id);

    mqttClient.connect({
        userName: mqtt_user,
        password: mqtt_pass,
        onSuccess: () => {
            console.log("[MQTT] Connection Success");
            mqttClient.subscribe("img/jpeg");
            mqttClient.subscribe("test");
            mqttClient.subscribe("analysis");
            mqttClient.subscribe("robotstate");
        },
        onFailure: (err) => {
            console.error("[MQTT] Connection Failed", err);
        }
    });
    mqttClient.onConnectionLost = (err) => {
        console.error("[MQTT] Disconnected", err);
    }
    mqttClient.onMessageArrived = (message) => {
        mqtt_messageHandler(message.destinationName, {
            asString() {return message.payloadString;},
            asBytes() {return message.payloadBytes;},
        },
        message);
    }
    return mqttClient;
}
