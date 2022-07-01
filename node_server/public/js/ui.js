//@ts-check
// References to all important User Interface controls and widgets
//@ts-nocheck
const UI = {
    /**@type {HTMLImageElement} */ cameraImage: null,
    imageAnalysis:{
        /**@type {HTMLElement} */ avg: null,
        /**@type {HTMLElement} */ dev: null,
        /**@type {HTMLElement} */ cropSize: null,
        /**@type {HTMLElement} */ avgColor: null,
        /**@type {HTMLElement} */ h: null,
        /**@type {HTMLElement} */ l: null,
        /**@type {HTMLElement} */ s: null,
    },
    assemblyRequest: {
        /**@type {HTMLSelectElement} */ bottomColorSelect: null,
        /**@type {HTMLSelectElement} */ topColorSelect: null,
    },
    dashboard:{
        /**@type {HTMLElement} */ mqttConnectedState: null,
        /**@type {HTMLElement} */ robotConnectedState: null,
        /**@type {HTMLElement} */ plcConnectedState: null,
        /**@type {HTMLElement} */ currentExecutingCmd: null,
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
    UI.cameraImage =  querySelector('#camera_image');

    UI.imageAnalysis.avg = querySelector('#avg');
    UI.imageAnalysis.dev = querySelector('#dev');
    UI.imageAnalysis.cropSize = querySelector('#cropSize');
    UI.imageAnalysis.avgColor = querySelector('#avgColor');
	UI.imageAnalysis.h = querySelector('#h');
	UI.imageAnalysis.s = querySelector('#s');
	UI.imageAnalysis.l = querySelector('#l');

    UI.assemblyRequest.bottomColorSelect = querySelector("#bottomColorSelect");
    UI.assemblyRequest.topColorSelect    = querySelector("#topColorSelect");

    UI.robotState.executedCmds          = querySelector("#stateExecutedCmds");
    UI.robotState.executedDebugCmds     = querySelector("#stateExecutedDebugCmds");

    UI.dashboard.mqttConnectedState   = querySelector("#mqtt_connection_state .state");
    UI.dashboard.robotConnectedState  = querySelector("#robot_connection_state .state");
    UI.dashboard.plcConnectedState    = querySelector("#plc_connection_state .state");
    UI.dashboard.currentExecutingCmd   = querySelector("#stateCurrentCommand");
}

let lastRobotState = {};
let lastServerState = {};
let dashboardState = {
    mqtt_connected: false,
    robot_connected: false,
    plc_connected: false,

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
    UI.cameraImage.src = currentImageDataUrl // Set the image URL to the new Blob
}

// Convert a 3-element float array to a CSS string representing an RGB color
function rgbColorFromArray(arr){
    return `rgb(${ arr.map(x => Math.floor(x)).join(',') })`;
}

function updateImageAnalysis(data){
    UI.imageAnalysis.avg.innerHTML = data.avg;
    UI.imageAnalysis.dev.innerHTML = data.dev;
    UI.imageAnalysis.cropSize.innerHTML = data.cropSize;
    UI.imageAnalysis.avgColor.style.backgroundColor = rgbColorFromArray(data.avg);
	UI.imageAnalysis.h.innerHTML = data.h;
	UI.imageAnalysis.l.innerHTML = data.l;
	UI.imageAnalysis.s.innerHTML = data.s;
}

function getNewAssemblyRequest(){
    const bottomColor = UI.assemblyRequest.bottomColorSelect.value;
    const topColor    = UI.assemblyRequest.topColorSelect.value;
    return {bottomColor, topColor};
}

function updateServerState(state = {}){
    Object.assign(lastServerState, state);
    if(state["plcConnected"]){
        dashboardState.plc_connected = state["plcConnected"];
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
        }
    }

    setConnected(UI.dashboard.mqttConnectedState, dashboardState.mqtt_connected);
    setConnected(UI.dashboard.robotConnectedState, dashboardState.robot_connected);
    setConnected(UI.dashboard.plcConnectedState, dashboardState.plc_connected);

    if(dashboardState.current_cmd == null){
        UI.dashboard.currentExecutingCmd.innerHTML = "None.";
    }else{
        UI.dashboard.currentExecutingCmd.innerHTML = "Command nr " + dashboardState.current_cmd;
    }
}