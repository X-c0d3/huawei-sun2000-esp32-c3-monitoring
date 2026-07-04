/*
  Standalone test: read a few registers from a Huawei SUN2000 inverter
  via the Smart Dongle's Modbus-TCP interface (no ESP32 / RS485 needed).

  Requires: "Dongle Parameter Settings > Modbus TCP > Enabled (Unrestricted)"
  to be turned on via the FusionSolar/SUN2000 app first.

  Run: node Test_HuaweiSun2000Client.js
*/

'use strict';

const net = require('net');

const HOST = '192.168.1.165';
const PORT = 502; // try 6607 if 502 doesn't respond
const UNIT_ID = 1; // must match the inverter's RS485_1 Com address
const TIMEOUT_MS = 3000;

let transactionId = 0;

function readHoldingRegisters(socket, address, quantity) {
    return new Promise((resolve, reject) => {
        transactionId = (transactionId + 1) & 0xffff;

        const request = Buffer.alloc(12);
        request.writeUInt16BE(transactionId, 0); // Transaction ID
        request.writeUInt16BE(0x0000, 2); // Protocol ID
        request.writeUInt16BE(0x0006, 4); // Length: unitId + function code + address + quantity
        request.writeUInt8(UNIT_ID, 6);
        request.writeUInt8(0x03, 7); // Function code: Read Holding Registers
        request.writeUInt16BE(address, 8);
        request.writeUInt16BE(quantity, 10);

        let buffer = Buffer.alloc(0);
        const timer = setTimeout(() => {
            socket.removeListener('data', onData);
            reject(new Error(`Timeout reading register ${address}`));
        }, TIMEOUT_MS);

        function onData(chunk) {
            buffer = Buffer.concat([buffer, chunk]);
            if (buffer.length < 9) return; // MBAP(7) + function code(1) + byte count(1)

            const functionCode = buffer[7];
            if (functionCode & 0x80) {
                clearTimeout(timer);
                socket.removeListener('data', onData);
                reject(new Error(`Modbus exception 0x${buffer[8].toString(16)} at register ${address}`));
                return;
            }

            const byteCount = buffer[8];
            if (buffer.length < 9 + byteCount) return;

            clearTimeout(timer);
            socket.removeListener('data', onData);

            const registers = [];
            for (let i = 0; i < quantity; i++) {
                registers.push(buffer.readUInt16BE(9 + i * 2));
            }
            resolve(registers);
        }

        socket.on('data', onData);
        socket.write(request);
    });
}

function registersToString(registers) {
    let str = '';
    for (const word of registers) {
        const hi = (word >> 8) & 0xff;
        const lo = word & 0xff;
        if (hi === 0) break;
        str += String.fromCharCode(hi);
        if (lo === 0) break;
        str += String.fromCharCode(lo);
    }
    return str.trim();
}

function registersToValue(registers, gain, signed) {
    let raw = 0;
    for (const word of registers) {
        raw = raw * 0x10000 + word;
    }
    if (signed) {
        const max = Math.pow(2, registers.length * 16);
        if (raw >= max / 2) raw -= max;
    }
    return raw / gain;
}

async function main() {
    const socket = new net.Socket();

    await new Promise((resolve, reject) => {
        socket.setTimeout(TIMEOUT_MS);
        socket.once('error', reject);
        socket.once('timeout', () => reject(new Error('Connection timeout')));
        socket.connect(PORT, HOST, () => {
            socket.removeListener('error', reject);
            resolve();
        });
    });
    console.log(`Connected to Smart Dongle ${HOST}:${PORT}`);

    try {
        const model = registersToString(await readHoldingRegisters(socket, 30000, 15));
        console.log(`Model: ${model}`);

        const activePower = registersToValue(await readHoldingRegisters(socket, 32080, 2), 1000, true);
        console.log(`Active Power: ${activePower} kW`);

        const gridVolt = registersToValue(await readHoldingRegisters(socket, 37101, 2), 10, false);
        console.log(`Grid Voltage: ${gridVolt} V`);
    } catch (err) {
        console.error('Read failed:', err.message);
    } finally {
        socket.destroy();
    }
}

main().catch((err) => {
    console.error('Connection failed:', err.message);
    process.exit(1);
});
