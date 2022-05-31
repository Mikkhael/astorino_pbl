const Modbus = require("./modbus.js");
const readline = require("readline");
const terminal = readline.createInterface(process.stdin, process.stdout);

const mbclient = new Modbus("192.168.0.102", 502, 5000, 1000);

mbclient.connect();


function loop(){
    mbclient.sendAssemblyRequest(1,2)
    .then(() => {
        console.log("1 Done.");
    })
    .catch(err => {
        console.error(err);
    });
    
    mbclient.pollForColorUpdate()
    .then((value) => {
        console.log("2 Done.", value);
    })
    .catch(err => {
        console.error(err);
    });
}

setInterval(loop, 1000);


