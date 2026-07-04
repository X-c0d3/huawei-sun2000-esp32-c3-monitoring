/*
  Huawei SUN2000 inverter reader via the Smart Dongle's Modbus-TCP interface.

  Requires: "Dongle Parameter Settings > Modbus TCP > Enabled (Unrestricted)"
  to be turned on via the FusionSolar/SUN2000 app first.

  Notes from field debugging:
  - The dongle's Modbus stack needs a moment to "wake up" after TCP connect;
    the first request after connecting frequently times out. Retry logic
    handles this.
  - Register addressing is 0-based / raw protocol address (e.g. 30000 means
    exactly 30000, NOT the Modicon "40001-style" convention some tools use).
  - Keep the TCP connection open and poll periodically instead of
    reconnecting every time, to avoid re-triggering the cold-start delay.

  Run: node HuaweiSun2000Client.js
*/

'use strict';

const net = require('net');

const HOST = '192.168.1.168';
const PORT = 502; // 6607 did not respond on this dongle; 502 is confirmed open
const UNIT_ID = 1; // must match the inverter's RS485_1 Com address
const TIMEOUT_MS = 3000;
const POLL_INTERVAL_MS = 5000;
const MAX_RETRIES = 3;
const RETRY_DELAY_MS = 500;

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

// Wraps readHoldingRegisters with retry logic to absorb the dongle's
// cold-start delay (first request after idle/connect often times out).
async function readWithRetry(socket, address, quantity, retries = MAX_RETRIES) {
    for (let attempt = 1; attempt <= retries; attempt++) {
        try {
            return await readHoldingRegisters(socket, address, quantity);
        } catch (err) {
            console.log(`  Attempt ${attempt}/${retries} failed for register ${address}: ${err.message}`);
            if (attempt === retries) throw err;
            await new Promise((r) => setTimeout(r, RETRY_DELAY_MS));
        }
    }
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

function connect() {
    return new Promise((resolve, reject) => {
        const socket = new net.Socket();
        socket.setTimeout(TIMEOUT_MS);
        socket.once('error', reject);
        socket.once('timeout', () => reject(new Error('Connection timeout')));
        socket.connect(PORT, HOST, () => {
            socket.removeListener('error', reject);
            resolve(socket);
        });
    });
}

async function pollOnce(socket) {
    const model = registersToString(await readWithRetry(socket, 30000, 15));
    console.log(`Model: ${model}`);

    const activePower = registersToValue(await readWithRetry(socket, 32080, 2), 1000, true);
    console.log(`Active Power: ${activePower} kW`);

    const gridVolt = registersToValue(await readWithRetry(socket, 37101, 2), 10, false);
    console.log(`Grid Voltage: ${gridVolt} V`);
}

async function main() {
    let socket = await connect();
    console.log(`Connected to Smart Dongle ${HOST}:${PORT}`);

    // Handle unexpected disconnects so the poll loop can reconnect instead of crashing.
    socket.on('close', () => {
        console.log('Socket closed unexpectedly.');
    });
    socket.on('error', (err) => {
        console.log(`Socket error: ${err.message}`);
    });

    // eslint-disable-next-line no-constant-condition
    while (true) {
        console.log(`\n--- Poll at ${new Date().toISOString()} ---`);
        try {
            if (socket.destroyed) {
                console.log('Reconnecting...');
                socket = await connect();
                console.log('Reconnected.');
            }
            await pollOnce(socket);
        } catch (err) {
            console.error('Poll failed:', err.message);
            // If the socket is in a bad state, force a reconnect on the next loop.
            if (socket && !socket.destroyed) {
                socket.destroy();
            }
        }
        await new Promise((r) => setTimeout(r, POLL_INTERVAL_MS));
    }
}

process.on('SIGINT', () => {
    console.log('\nShutting down...');
    process.exit(0);
});

main().catch((err) => {
    console.error('Fatal error:', err.message);
    process.exit(1);
});