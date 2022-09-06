//@ts-check
// References to all important User Interface controls and widgets
//@ts-nocheck
const UI = {
    imageAnalysis:{
        /**@type {HTMLImageElement} */ cameraImage: null,
        /**@type {HTMLElement} */      cropper: null,
        /**@type {HTMLElement} */ avg: null,
        /**@type {HTMLElement} */ dev: null,
        /**@type {HTMLElement} */ hsl: null,
        /**@type {HTMLElement} */ cropSize: null,
        /**@type {HTMLElement} */ avgColor: null,
        /**@type {HTMLElement} */ recognizedColor: null,
    },
    assemblyRequest: {
        /**@type {HTMLSelectElement} */ bottomColorSelect: null,
        /**@type {HTMLSelectElement} */ topColorSelect: null,
        /**@type {HTMLInputElement} */  customCommandInput: null,
    },
    dashboard:{
        /**@type {HTMLElement} */ mqttConnectedState: null,
        /**@type {HTMLElement} */ robotConnectedState: null,
        /**@type {HTMLElement} */ plcConnectedState: null,
        /**@type {HTMLElement} */ currentExecutingCmd: null,
        
        /**@type {HTMLElement} */ grabber: null,
        /**@type {HTMLElement} */ element: null,
        /**@type {HTMLElement} */ button:  null,
        /**@type {HTMLElement} */ robotAction:  null,
        /**@type {HTMLElement} */ robotActionText:  null,
        /**@type {HTMLElement} */ assemblyStatus:  null,
        /**@type {HTMLElement} */ assemblyStatusText:  null,
    },
    robotState: {
        /**@type {HTMLElement} */ executedCmds: null,
        /**@type {HTMLElement} */ executedDebugCmds: null,
    }
};
//@ts-check
/**
 * @param {string} selector 
 * @return {any}
 */
function querySelector(selector){
    let res = document.querySelector(selector);
    if(res === null){
        alert(`Cannot resolve querySelector for: '${selector}'`);
    }
    return res;
}

function initUIElements(){
    UI.imageAnalysis.cameraImage =  querySelector('#camera_image');

    UI.imageAnalysis.avg = querySelector('#avg');
    UI.imageAnalysis.dev = querySelector('#dev');
    UI.imageAnalysis.hsl = querySelector('#hsl');
    UI.imageAnalysis.cropSize = querySelector('#cropSize');
    UI.imageAnalysis.avgColor = querySelector('#avgColor');
    UI.imageAnalysis.cropper = querySelector("#cropper");
    UI.imageAnalysis.recognizedColor = querySelector("#recognizedColor");

    UI.assemblyRequest.bottomColorSelect = querySelector("#bottomColorSelect");
    UI.assemblyRequest.topColorSelect    = querySelector("#topColorSelect");
    UI.assemblyRequest.customCommandInput = querySelector("#customCommandValues");

    UI.robotState.executedCmds          = querySelector("#stateExecutedCmds");
    UI.robotState.executedDebugCmds     = querySelector("#stateExecutedDebugCmds");

    UI.dashboard.mqttConnectedState   = querySelector("#mqtt_connection_state .state");
    UI.dashboard.robotConnectedState  = querySelector("#robot_connection_state .state");
    UI.dashboard.plcConnectedState    = querySelector("#plc_connection_state .state");
    UI.dashboard.currentExecutingCmd   = querySelector("#stateCurrentCommand");

    UI.dashboard.grabber = querySelector("#grabberStatus");
    UI.dashboard.button = querySelector("#buttonStatus");
    UI.dashboard.element = querySelector("#elementStatus");
    UI.dashboard.robotAction = querySelector("#robotAction");
    UI.dashboard.robotActionText = querySelector("#robotActionText");
    UI.dashboard.assemblyStatus = querySelector("#assemblyStatus");
    UI.dashboard.assemblyStatusText = querySelector("#assemblyStatusText");
}

let lastRobotState = {};
let lastServerState = {};
let dashboardState = {
    mqtt_connected: false,
    robot_connected: false,
    plc_connected: false,

    grabberClosed: false,
    grabberClosedError: false,
    buttonPressed: false,
    elementDetected: false,
    isIdle: false,

    assembly_status: 0,
    /**@type {number?} */ current_cmd: null,
};

let currentImageDataUrl = null; // Last bounded URL to the image data Blob

/**
 * @param {Blob} newImageData 
 */
function updateCameraImage(newImageData){
    if(currentImageDataUrl){ // Check if image is already bound to a Blob
        URL.revokeObjectURL(currentImageDataUrl); // Free the memory from the old Blob
    }
    currentImageDataUrl = URL.createObjectURL(newImageData) // Bind new Blob to an URL
    UI.imageAnalysis.cameraImage.src = currentImageDataUrl // Set the image URL to the new Blob
}

// Convert a 3-element float array to a CSS string representing an RGB color
function rgbColorFromArray(arr){
    return `rgb(${ arr.map(x => Math.floor(x)).join(',') })`;
}

const colorNames = ["Blue", "Green", "Purple", "---"];
function updateImageAnalysis(data = {}){
    const hsl = [data.h, data.s, data.l];
    UI.imageAnalysis.avg.innerHTML = data.avg.map(x => x.toFixed(0));
    UI.imageAnalysis.dev.innerHTML = data.dev.map(x => x.toFixed(0));
    UI.imageAnalysis.hsl.innerHTML = hsl.map(x => x.toFixed(3));

    UI.imageAnalysis.avgColor.style.backgroundColor = `rgb(${data.avg})`;
    UI.imageAnalysis.avgColor.style.width = UI.imageAnalysis.cameraImage.clientWidth + 'px';

    UI.imageAnalysis.cropSize.innerHTML = data.cropSize;

    
    UI.imageAnalysis.recognizedColor.innerHTML = colorNames[data.col];

    UI.imageAnalysis.cropper.style.height = data.cropSize*2 + 'px';
    UI.imageAnalysis.cropper.style.width  = data.cropSize*2 + 'px';
    UI.imageAnalysis.cropper.style.left   = (UI.imageAnalysis.cameraImage.clientWidth/2  - data.cropSize) + 'px';
    UI.imageAnalysis.cropper.style.top    = (UI.imageAnalysis.cameraImage.clientHeight/2 - data.cropSize) + 'px';
    UI.imageAnalysis.cropper.style.display = 'block';
}

function getNewAssemblyRequest(){
    const bottomColor     = UI.assemblyRequest.bottomColorSelect.value;
    const topColor        = UI.assemblyRequest.topColorSelect.value;
    const bottomColorName = UI.assemblyRequest.bottomColorSelect.options[bottomColor].text;
    const topColorName    = UI.assemblyRequest.topColorSelect.options[topColor].text;
    return {bottomColor, topColor, bottomColorName, topColorName};
}

const PlcStateNames = {

    "0": "IDLE",
    "1": "GET BOTTOM",
    "2": "CHECK BOTTOM PRESENCE",
    "3": "PLACE BOTTOM",
    "4": "TURN BOTTOM TO COLOR CHECK",
    "5": "CHECK BOTTOM COLOR",
    "6": "TURN BOTTOM TO ROBOT",
    "7": "GET TOP",
    "8": "CHECK TOP PRESENCE",
    "9": "MOUNT TOP",
   "10": "TURN ASSEMBLED TO COLOR CHECK",
   "11": "CHECK TOP COLOR",
   "12": "TURN TO PRESS CHECK",
   "13": "PRESS CHECK",
   "14": "PRESS RELEASE",
   "15": "TURN ASSEMBLED TO ROBOT",
   "16": "OUTPUT ASSEMBLED",
   "17": "SIGNAL FINISH",
   
    "-1": "CRITICAL ERROR (OPERATOR REQUIRED)",
    "-7": "DISCARD INVALID COLOR BOTTOM",
   "-12": "TURN INVALID COLOR ASSEMBLED TO ROBOT",
   "-13": "DISCARD INVALID COLOR TOP",
   "-16": "DISCARD FAILED PRESS ASSEMBLED",
};

function updateServerState(state = {}){
    Object.assign(lastServerState, state);
    if(state["plcConnected"] != undefined){
        dashboardState.plc_connected = state["plcConnected"];
    }
    if(state["plcState"] != undefined){
        let s = state["plcState"];
        if(s == 0){
            UI.dashboard.assemblyStatus.setAttribute('type', 'idle');
        }
        else if(s > 0x8FFF){
            //console.log(s);
            s = -((0xFFFF-s)+1);
            //console.log(s);
            UI.dashboard.assemblyStatus.setAttribute('type', 'error');
        }
        else{
            UI.dashboard.assemblyStatus.setAttribute('type', 'working');
        }
        const text = PlcStateNames[s] ?? "";
        UI.dashboard.assemblyStatusText.innerHTML = `${text} (${s})`;
    }
}

function updateRobotState(state = {}){
    Object.assign(lastRobotState, state);
    if(state["ExecutedCmds"] !== undefined){
        UI.robotState.executedCmds.innerHTML = state['ExecutedCmds'];
    }
    if(state["ExecutedDebugCmds"] !== undefined){
        UI.robotState.executedDebugCmds.innerHTML = state['ExecutedDebugCmds'];
    }
    if(state["CurrentExecutingCmd"] !== undefined){
        let val = state["CurrentExecutingCmd"];
        console.log(val);
        if(val == 255) val = null;
        dashboardState.current_cmd = val;
    }

    dashboardState.grabberClosed = lastRobotState["GGrabbed"];
    dashboardState.grabberClosedError = lastRobotState["GGrabbed"] != lastRobotState["GGrab"];
    dashboardState.elementDetected = lastRobotState["GFar1"];
    dashboardState.buttonPressed = lastRobotState["GTens"];
    dashboardState.isIdle = lastRobotState["Idle"];

    // quick test
    for(let [key, value] of Object.entries(state)){
        let element = document.querySelector(`.diode#state${key}`);
        if(element === null){
            continue;
        }
        element.setAttribute("diodevalue", value ? "1" : "0");
        element.parentElement.style.display = "flex";
    }
}

const RobotCommandNames = {
    "0": "Getting Bottom Piece from Storage 1",
    "1": "Getting Bottom Piece from Storage 2",
    "2": "Getting Bottom Piece from Storage 3",
    "3": "Getting Bottom Piece from Storage 4",
    "4": "Getting Bottom Piece from Storage 5",

    "5": "Getting Top Piece from Storage 1",
    "6": "Getting Top Piece from Storage 2",
    "7": "Getting Top Piece from Storage 3",
    "8": "Getting Top Piece from Storage 4",
    "9": "Getting Top Piece from Storage 5",

    "20": "Putting Bottom Piece on Turntable",
    "21": "Assembling the Button on Turntable",

    "10": "Moving Assembled Button to Exit",

    "15": "Discarding Bottom Piece",
    "16": "Disassembling and Discarding Top Piece",
    "17": "Discarding Assembled Button",

};

function updateDashboard()
{
    /**
     * @param {HTMLElement} element 
     * @param {boolean} state 
     */
    function setConnected(element, state){
        if(state){
            element.classList.add("connected");
            element.innerHTML = "CONNECTED";
        }else{
            element.classList.remove("connected");
            element.innerHTML = "DISCONNECTED";
        }
    }

    setConnected(UI.dashboard.mqttConnectedState, dashboardState.mqtt_connected);
    setConnected(UI.dashboard.robotConnectedState, dashboardState.robot_connected);
    setConnected(UI.dashboard.plcConnectedState, dashboardState.plc_connected);

    if(dashboardState.robot_connected){
        if(dashboardState.current_cmd == null){
            UI.dashboard.currentExecutingCmd.innerHTML = "None.";
            if(dashboardState.isIdle){
                UI.dashboard.robotAction.setAttribute("type", "idle");
                UI.dashboard.robotActionText.innerHTML = "IDLE";
            }else{
                UI.dashboard.robotAction.setAttribute("type", "sending");
                UI.dashboard.robotActionText.innerHTML = "Sending next command...";
            }
        }else{
            UI.dashboard.currentExecutingCmd.innerHTML = "Command nr " + dashboardState.current_cmd;
            UI.dashboard.robotAction.setAttribute("type", "working");
            const text = RobotCommandNames[dashboardState.current_cmd] || "";
            UI.dashboard.robotActionText.innerHTML = `${text} (${dashboardState.current_cmd})`;
        }

        UI.dashboard.grabber.innerHTML = dashboardState.grabberClosed ? "CLOSED" : "OPEN";
        UI.dashboard.element.innerHTML = dashboardState.elementDetected ? "DETECTED" : "NOT DETECTED";
        UI.dashboard.button.innerHTML  = dashboardState.buttonPressed ? "PRESSED" : "NOT PRESSED";
    }

}

