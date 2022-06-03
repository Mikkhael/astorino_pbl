//@ts-check

initUIElements();

////////////////// MQTT //////////////////////////

/**
 * @param {string} topic 
 * @param {{
 *  asString: () => string,
 *  asBytes: () => Uint8Array
 * }} payload 
 */
let tmp;

function handleMqttMessage(topic, payload){
    if(topic == "test"){
        console.log("Received test message:", payload.asString());
    }else if(topic == "img/jpeg"){
        console.log("Received new image");
        updateCameraImage(new Blob([payload.asBytes()]));
    }else if(topic == "analysis"){
        const data = JSON.parse(payload.asString());
		tmp = data;
        updateImageAnalysis(data);
    }else{
        console.log(`Unhandled topic:`, topic);
    }
    //TODO add new messages to handle
}

// Start the MQTT client
MQTT_setMessageHandler(handleMqttMessage);
const mqttClient = MQTT_createClientAndConnect();