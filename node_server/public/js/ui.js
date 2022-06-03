//@ts-check

// References to all important User Interface controls and widgets
const UI = {
    /**@type {HTMLImageElement} */
    cameraImage: null,
    imageAnalysis:{
        /**@type {HTMLElement} */
        avg: null,
        /**@type {HTMLElement} */
        dev: null,
        /**@type {HTMLElement} */
        cropSize: null,
        /**@type {HTMLElement} */
        avgColor: null,
    },
};

function initUIElements(){
    UI.cameraImage = document.querySelector('#camera_image');
    UI.imageAnalysis.avg = document.querySelector('#avg');
    UI.imageAnalysis.dev = document.querySelector('#dev');
    UI.imageAnalysis.cropSize = document.querySelector('#cropSize');
    UI.imageAnalysis.avgColor = document.querySelector('#avgColor');
	UI.imageAnalysis.h = document.querySelector('#h');
	UI.imageAnalysis.s = document.querySelector('#s');
	UI.imageAnalysis.l = document.querySelector('#l');
}

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
