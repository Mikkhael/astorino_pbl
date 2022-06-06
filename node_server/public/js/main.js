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
    }else if(topic == "robotstate"){
        console.log(payload.asString().length);
        const robotstate = parseMQTT_robotstate(payload.asBytes());
        console.log(robotstate.GFar1);
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


const Mqtt_robotstate_labels_uint8 = [
    "OMotorOn", "CycleStart", "Reset", "OHold", "CycleStop", "MotorOff", "Zeroing", "Interrupt",
    "Send", "Cmd1", "Cmd2",
    "Cycle", "Repeat", "Teach", "MotorOn", "ESTOP",  "Ready", "Error", "Hold", "Home", "Zeroed",
    "Idle", "Ack",  "Grab",
    "GGrab", "GFar1",
    "QueueFull", "QueueEmpty"
];
const Mqtt_robotstate_labels_uint16 = [
    "ExecutedCmds", "ExecutedDebugCmds"
];

/**
 * @param {Uint8Array} payload 
 */
function parseMQTT_robotstate(payload){
    let res = {};
    const payload_buf = payload.buffer;
    const view = new DataView(payload_buf, payload.byteOffset);
    for(let i = 0; i<Mqtt_robotstate_labels_uint8.length; i++){
        const val = view.getUint8(i);
        if(val == 0) continue;
        res[Mqtt_robotstate_labels_uint8[i]] = (val == 2);
    }
    for(let i = 0; i<Mqtt_robotstate_labels_uint16.length; i++){
        res[Mqtt_robotstate_labels_uint16[i]] = view.getUint16(i + Mqtt_robotstate_labels_uint8.length, true);
    }
    return res;
}