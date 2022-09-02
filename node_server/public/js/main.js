//@ts-check
initUIElements();

////////////////// MQTT //////////////////////////

let tmp;
let lastRobotStateMessageTime = -10000;

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
        lastRobotStateMessageTime = Date.now();
        const robotstate = parseMQTT_robotstate(payload.asBytes());
        updateRobotState(robotstate);
    }else if(topic == "serverstate"){
        const data = JSON.parse(payload.asString());
        updateServerState(data);
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
    let r = confirm(`Assemble element for ${request.bottomColorName} button with ${request.topColorName} top ?`);
    if(r){
        sendNewAssemblyRequest(request.bottomColor, request.topColor);
    }
}

function sendCustomCommandsRequest(commands){
    const payload = Uint8Array.from(commands);
    mqttClient.send('customCommand', payload);
}

function performCustomCommandsRequest(){
    const str = UI.assemblyRequest.customCommandInput.value;
    const commands = str.split(',').map(x => parseInt(x.trim())).filter(x => +x >= 0 && +x <= 255);
    if(commands.length == 0){
        alert("No commands has beed entered.");
    }else{
        let r = confirm(`Send commands: [${commands}]?`);
        if(r){
            sendCustomCommandsRequest(commands);
        }
    }
}


const Mqtt_robotstate_labels_bool = [
    "OMotorOn", "OCycleStart", "OReset", "OHold", "OCycleStop", "OMotorOff", "OZeroing", "OInterrupt",
    "OSend", "OCmd1", "OCmd2",
    "ICycle", "IRepeat", "ITeach", "IMotorOn", "IESTOP", "IReady", "IError", "IHold", "IHome", "IZeroed",
    "IIdle", "IAck", "IGrab",
    "GGrab", "GTestButt", "GFar1", "GTens", "GGrabbed",
    "QueueFull", "QueueEmpty", "Idle",
];
const Mqtt_robotstate_labels_uint16 = [
    "ExecutedCmds", "ExecutedDebugCmds", "CurrentExecutingCmd"
];

/**
 * @param {Uint8Array} payload 
 */
function parseMQTT_robotstate(payload){
    let res = {};
    const payload_buf = payload.buffer;
    const view = new DataView(payload_buf, payload.byteOffset);
    for(let i = 0; i<Mqtt_robotstate_labels_bool.length; i++){
        const val = view.getUint8(i);
        if(val == 0) continue;
        res[Mqtt_robotstate_labels_bool[i]] = (val != 2);
    }
    for(let i = 0; i<Mqtt_robotstate_labels_uint16.length; i++){
        //console.log(Mqtt_robotstate_labels_uint16[i],i*2 + Mqtt_robotstate_labels_bool.length);
        res[Mqtt_robotstate_labels_uint16[i]] = view.getUint16(2*i + Mqtt_robotstate_labels_bool.length, true);
    }
    //console.log(payload);
    res["IGrab"] = !res["IGrab"];
    return res;
}

function sendDebugRobotState(data = {}){
    const payload = new ArrayBuffer(Mqtt_robotstate_labels_bool.length + Mqtt_robotstate_labels_uint16.length * 2);
    const view = new DataView(payload);
    for(let i = 0; i<Mqtt_robotstate_labels_bool.length; i++){
        view.setUint8(i, data[Mqtt_robotstate_labels_bool[i]] ?? 1);
    }
    for(let i = 0; i<Mqtt_robotstate_labels_uint16.length; i++){
        view.setUint16(Mqtt_robotstate_labels_bool.length + i*2, data[Mqtt_robotstate_labels_uint16[i]] ?? 0, true);
    }
    mqttClient.send("robotstate", payload);
}

// Periodically update dashboard

function dashboardUpdateRoutine(){
    dashboardState.mqtt_connected = mqttClient.isConnected;
    dashboardState.robot_connected = ((Date.now() - lastRobotStateMessageTime) <= 2000);
    updateDashboard();
}
setInterval(dashboardUpdateRoutine, 1000);