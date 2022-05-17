//@ts-check

////////////////// User Interfacee //////////////////

/**@type {HTMLImageElement} */
const cameraImage = document.querySelector("#camera_image"); // Reference to the HTMLImage object

const avgElement = document.querySelector("#avg");
const devElement = document.querySelector("#dev");
const cropSizeElement = document.querySelector("#cropSize");
/**@type {HTMLElement} */
const avgColorElement = document.querySelector("#avgColor");

let currentImageDataUrl = null; // Last bounded URL to the image data Blob
/**@param {Blob} newImageData */
function replaceImage(newImageData){
    if(currentImageDataUrl){ // Check if image is already bound to a Blob
        URL.revokeObjectURL(currentImageDataUrl); // Free the memory from the old Blob
    }
    currentImageDataUrl = URL.createObjectURL(newImageData) // Bind new Blob to an URL
    cameraImage.src = currentImageDataUrl // Set the image URL to the new Blob
}

function updateImageAnalysis(data){
    avgElement.innerHTML = data.avg;
    devElement.innerHTML = data.dev;
    cropSizeElement.innerHTML = data.cropSize;
    avgColorElement.style.backgroundColor = `rgb(${Math.floor(data.avg[0])}, ${Math.floor(data.avg[1])}, ${Math.floor(data.avg[2])})`
}

////////////////// MQTT //////////////////////////

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
        replaceImage(new Blob([payload.asBytes()]));
    }else if(topic == "analysis"){
        const data = JSON.parse(payload.asString());
        updateImageAnalysis(data);
    }else{
        console.log(`Unhandled topic:`, topic);
    }
    //TODO add new messages to handle
}

// Start the MQTT client
MQTT_setMessageHandler(handleMqttMessage);
const mqttClient = MQTT_createClientAndConnect();