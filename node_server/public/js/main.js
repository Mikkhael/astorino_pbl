//@ts-check
initUIElements();

////////////////// MQTT //////////////////////////

let tmp;

/**
 * @param {string} topic 
 * @param {{
 *  asString: () => string,
 *  asBytes: () => Uint8Array
 * }} payload 
 */
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



function sendNewAssemblyRequest(bottomColor, topColor){
    console.log(`Sending assembly request for ${bottomColor} | ${topColor}`);
    const payload = {bottomColor, topColor};
    mqttClient.send("assemblyRequest", JSON.stringify(payload));
}

function performNewAssemblyRequest(){
    const request = getNewAssemblyRequest();
    sendNewAssemblyRequest(request.bottomColor, request.topColor);
}
