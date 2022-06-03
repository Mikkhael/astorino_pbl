const Modbus = require("./modbus.js");
const readline = require("readline");
const terminal = readline.createInterface(process.stdin, process.stdout);

const mbclient = new Modbus("192.168.0.102", 502, 5000, 1000);

mbclient.onAssemblyCompleted = function(requestId, request){
    console.log("[MBUS]", `Completed Request Id: ${requestId}, Colors: ${request.bottomColor} | ${request.topColor}`);
}

mbclient.onAssemblyRequestSend = function(requestId, request, err){
    console.log("[MBUS]", `Sent Request Id: ${requestId}, Colors: ${request.bottomColor} | ${request.topColor}`);
    if(err){
        console.error("[MBUS] Assembly ", err);
    }
}

mbclient.onColorUpdateSend = function(finishedId, color, err){
    if(err){
        console.error("[MBUS] Color Request", err);
    }
};

mbclient.onPollError = function(err){
    console.error(err);
};

mbclient.connect();



mbclient.enqueueRequest(1, 1);
mbclient.enqueueRequest(1, 1);



function loop(){
    mbclient.loop();
}

setInterval(loop, 1000);


