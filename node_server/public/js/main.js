//@ts-check

////////////////// User Interfacee //////////////////

/**@type {HTMLImageElement} */
const cameraImage = document.querySelector("#camera_image"); // Reference to the HTMLImage object

let currentImageDataUrl = null; // Last bounded URL to the image data Blob
/**@param {Blob} newImageData */
function replaceImage(newImageData){
    if(currentImageDataUrl){ // Check if image is already bound to a Blob
        URL.revokeObjectURL(currentImageDataUrl); // Free the memory from the old Blob
    }
    currentImageDataUrl = URL.createObjectURL(newImageData) // Bind new Blob to an URL
    cameraImage.src = currentImageDataUrl // Set the image URL to the new Blob
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
    }else{
        console.log(`Unhandled topic:`, topic);
    }
    //TODO add new messages to handle
}

// Start the MQTT client
MQTT_setMessageHandler(handleMqttMessage);
const mqttClient = MQTT_createClientAndConnect();