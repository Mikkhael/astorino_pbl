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
            if(reconnectTimeout >= 0){
                setTimeout(this.connect.bind(this), reconnectTimeout);
            }
        });

        this.socket.on("error", (err) => {
            console.error("[MBUS]", err);
        });

        this.socket.on("connect", () => {
            console.log("[MBUS]", `Connected`);
            this.isConnected = true;
        });


        this.lastAssemblyRequestID = 0;
        this.lastColorRequestID = 0;
    }

    connect(){
        this.socket.setKeepAlive(true, this.keepalive);
        this.socket.connect({
            host: this.address,
            port: this.port,
        })
    }
    
    /**
     * @param {number} colorBottom 
     * @param {number} colorTop 
     */
    sendAssemblyRequest(colorBottom, colorTop){
        if(!this.isConnected){
            return Promise.reject("Disconnected");
        }
        return Promise.resolve().then( () => {
            return this.readHoldingRegisters(ModbusPLCAddresses.Assemble.FinishedID, 1);
        }).then((value) => {
            //console.log(value.response);
            if(this.lastAssemblyRequestID === undefined){
                this.lastAssemblyRequestID = value.response.body.values[0] + 1;
                return;
            }
            if(value.response.body.values[0] === this.lastAssemblyRequestID ){
                this.lastAssemblyRequestID = (this.lastAssemblyRequestID + 1);
                return;
            }
            throw "NotFinished";
        }).then(() => {
            return this.writeSingleRegister(ModbusPLCAddresses.Assemble.ColorBottom, colorBottom);
        }).then(() => {
            return this.writeSingleRegister(ModbusPLCAddresses.Assemble.ColorTop, colorTop);
        }).then(() => {
            return this.writeSingleRegister(ModbusPLCAddresses.Assemble.RequestID, this.lastAssemblyRequestID);
        });
    }

    pollForColorUpdate(){
        if(!this.isConnected){
            return Promise.reject("Disconnected");
        }
        return Promise.resolve().then( () => {
            return this.readHoldingRegisters(ModbusPLCAddresses.Color.RequestID, 1);
        }).then((value) => {
            if(this.lastColorRequestID != value.response.body.values[0]){
                this.lastColorRequestID = value.response.body.values[0];
                if(this.color === undefined) this.color = 0;
                this.color = (this.color + 1) % 3;
                return Promise.resolve().then(() => {
                    return this.writeSingleRegister(ModbusPLCAddresses.Color.Value, this.color);
                }).then(() => {
                    return this.writeSingleRegister(ModbusPLCAddresses.Color.FinishedID, this.lastColorRequestID);
                }).then(() => {
                    return true;
                });
            }else{
                return false;
            }
        })
    }

};

module.exports = Modbus;