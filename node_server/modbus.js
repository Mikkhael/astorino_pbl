const jsmodbus = require("jsmodbus");
const net = require("net");

const ModbusPLCAddresses = {
    Assemble: {
        RequestID:      556,
        FinishedID:     557,
        ColorBottom:    558,
        ColorTop:       559,
    },
    Color: {
        RequestID:      560,
        FinishedID:     561,
        Value:          562,
    },
    Status: {
        Value:          563,
    }
}
Object.freeze(ModbusPLCAddresses);

class Modbus extends jsmodbus.client.TCP{
    /**
     * @param {string} address 
     * @param {number} port
     * @param {number} keepalive
     * @param {number} reconnectTimeout
     */
    constructor(address, port, keepalive, reconnectTimeout){
        const socket = new net.Socket();
        super(socket);
        this.address = address;
        this.port = port;
        this.keepalive = keepalive;
        this.reconnectTimeout = reconnectTimeout;
        this.isConnected = false;

        this.socket.on("close", () => {
            console.log("[MBUS]", `Closed`);
            this.isConnected = false;
            this.loopCompleted = false;
            if(reconnectTimeout >= 0){
                setTimeout(this.connect.bind(this), reconnectTimeout);
            }
        });

        this.socket.on("error", (err) => {
            if(err.code == 'EHOSTUNREACH'){
                console.error("[MBUS]", "Host unreachable.");
                return;
            }
            console.error("[MBUS]", err);
        });

        this.socket.on("connect", () => {
            console.log("[MBUS]", `Connected`);
            this.isConnected = true;

            this.colorState = "Done";
            this.assemblyState = "Done";

            this.lastAssemblyRequestID = -1;
            this.polledAssemblyFinishedID = -1;
            this.polledColorRequestID = -1;
            this.polledColorFinishedID = -1;

            this.loopCompleted = true;
            this.assemblyToComplete = {};
        });

        this.assemblyQueue = [];
        this.assemblyToComplete = {};
        this.loopPromise = Promise.resolve();
        this.loopCompleted = false;

        this.getCameraColor = function() {return ;};
        this.onAssemblyRequestSend = function(requestId, request, err){console.log("Ass", requestId, request, err);};
        this.onAssemblyCompleted = function(requestId, request){console.log("AssC", requestId, request);}
        this.onColorUpdateSend = function(finishedId, color, err){console.log("Col", finishedId, color, err);};
        this.onPollError = function(err){console.log("Err", err)};

        this.plcState = 0;
    }

    /**
     * 
     * @param {number} bottomColor 
     * @param {number} topColor 
     */
    enqueueRequest(bottomColor, topColor){
        this.assemblyQueue.push({bottomColor, topColor});
    }

    connect(){
        this.socket.setKeepAlive(true, this.keepalive);
        this.socket.connect({
            host: this.address,
            port: this.port,
        });
        // Fix for memory leak in node v10.x
        let listeners = this.socket.listeners("connect");
        for(let i=2; i<listeners.length; i++){
            this.socket.removeListener("connect", listeners[i]);
        }
    }
    
    poll(){
        if(!this.isConnected){
            this.onPollError(new Error("Disconnected."));
            return Promise.resolve();
        }
        return Promise.all([
            this.readHoldingRegisters(ModbusPLCAddresses.Color.RequestID, 1),
            this.readHoldingRegisters(ModbusPLCAddresses.Color.FinishedID, 1),
            this.readHoldingRegisters(ModbusPLCAddresses.Assemble.FinishedID, 1),
            this.readHoldingRegisters(ModbusPLCAddresses.Status.Value, 1)
        ])
        .then((values) => {
            this.polledColorRequestID     = values[0].response.body.values[0];
            this.polledColorFinishedID    = values[1].response.body.values[0];
            this.polledAssemblyFinishedID = values[2].response.body.values[0];
            this.plcState                 = values[3].response.body.values[0];
            if(this.lastAssemblyRequestID < 0)
                this.lastAssemblyRequestID = this.polledAssemblyFinishedID;
            const completedRequest = this.assemblyToComplete[this.polledAssemblyFinishedID];
            if(completedRequest){
                this.onAssemblyCompleted(this.polledAssemblyFinishedID, completedRequest);
                delete this.assemblyToComplete[this.polledAssemblyFinishedID];
            }
            //console.log("Poll completed", this.polledColorRequestID, this.polledColorFinishedID, this.polledAssemblyFinishedID, this.lastAssemblyRequestID);
        })
        .catch((err) => {
            this.onPollError(err);
        });
    }

    loop(){
        if(!this.loopCompleted){
            return;
        }
        this.loopCompleted = false;
        this.loopPromise = this.poll().then(() => {

            if(this.colorState == "Done" && this.polledColorRequestID != this.polledColorFinishedID){
                const color = this.getCameraColor();
                let newFinishedID = -1;
                Promise.resolve().then(() => {
                    this.colorState = "Setting Color";
                    return this.writeSingleRegister(ModbusPLCAddresses.Color.Value, color)
                    .then(() => {
                        this.colorState = "Setting FinishedID";
                        newFinishedID = this.polledColorRequestID;
                        return this.writeSingleRegister(ModbusPLCAddresses.Color.FinishedID, newFinishedID)
                    })
                    .then(() => {
                        this.onColorUpdateSend(newFinishedID, color);
                    })
                    .catch((err) => {
                        this.onColorUpdateSend(newFinishedID, color, err);
                    })
                    .finally(() => {
                        this.colorState = "Done";
                    });
                });
            }
            if(this.assemblyState == "Done" && this.polledAssemblyFinishedID == this.lastAssemblyRequestID && this.assemblyQueue.length > 0){
                const request = this.assemblyQueue[0];
                Promise.resolve().then(() => {
                    this.assemblyState = "Setting Colors";
                    return Promise.all([
                        this.writeSingleRegister(ModbusPLCAddresses.Assemble.ColorBottom, request.bottomColor),
                        this.writeSingleRegister(ModbusPLCAddresses.Assemble.ColorTop,    request.topColor),
                    ])
                    .then(() => {
                        this.assemblyState = "Setting RequestID";
                        return this.writeSingleRegister(ModbusPLCAddresses.Assemble.RequestID, this.lastAssemblyRequestID+1)
                    })
                    .then(() => {
                        this.assemblyQueue.shift();
                        this.lastAssemblyRequestID += 1;
                        this.assemblyToComplete[this.lastAssemblyRequestID] = request;
                        this.onAssemblyRequestSend(this.lastAssemblyRequestID, request );
                    })
                    .catch((err) => {
                        this.onAssemblyRequestSend(this.lastAssemblyRequestID+1, request, err);
                    })
                    .finally(() => {
                        this.assemblyState = "Done";
                    });
                })
            }
        })
        .finally(() => {
            this.loopCompleted = true;
        })
    }

};

module.exports = Modbus;