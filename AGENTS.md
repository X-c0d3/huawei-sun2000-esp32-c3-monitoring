# AGENTS.md — huawei-sun2000-esp32

บริบทและกฎสำหรับ AI (และคน) ที่จะแก้โค้ดในโปรเจกต์นี้ อ่านให้จบก่อนแก้ไฟล์ใน `src/`

## ภาพรวม

ESP32-C3 อ่านค่าจาก Huawei SUN2000 ผ่าน Modbus แล้ว:
- แสดงผลบนจอ TFT
- ส่ง JSON ขึ้น MQTT (`<MQTT_TOPIC_BASE>/state`) และ Socket.IO
- Home Assistant (HA) อ่าน attribute `deviceState` ไปทำ template sensor → `utility_meter` → Bill Energy Card

| เครื่อง | การเชื่อมต่อ | `USE_MODBUS_TCP` |
|---|---|---|
| SUN2000-10K-LC0 | RS485 ตรง (Modbus-RTU), slave 1, 9600 8N1 | `false` |
| SUN2000-10KTL-M1 | มี Smart Dongle → Modbus-TCP ไปที่ IP ของ dongle | `true` |

ไฟล์หลัก:
- `src/HuaweiSun2000Client.h`: อ่าน register, แปลงค่า, ตรวจความถูกต้อง (`getDeviceInfo()`)
- `src/main.cpp`: วน poll, ข้ามรอบที่ `valid == false`, คำนวณ `dailyGridImport`, วาดจอ, publish
- `src/models/InverterData.h`: โครงสร้างข้อมูลและ `toJson()` (**ตัวกำหนด payload**)
- `src/broadCastToClients.h`: ประกอบ JSON และส่ง MQTT/Socket.IO
- `src/settings.h`: config (มีรหัส WiFi/MQTT จริง **ห้าม commit ทั้งไฟล์**)

## กฎที่ห้ามละเมิด

1. **ห้ามเปลี่ยน response object** ห้าม rename, ลบ, เปลี่ยนชนิด หรือเปลี่ยนความหมายของ field ใน `InverterData::toJson()` และ `buildDeviceStatePayload()` เพราะ HA template, `utility_meter` และ Bill Energy Card ผูกกับชื่อ field เหล่านี้ทั้งหมด
   - ตัวอย่างที่ถือว่าเปลี่ยนความหมาย: ใส่ `abs()` ให้ `grid_power` (เครื่องหมายบอกทิศทางซื้อ/ขาย), สลับ `gridImportEnergy` กับ `gridExportEnergy`
2. **ห้ามส่งข้อมูลที่เพี้ยนออกไป** ถ้าค่าไม่น่าเชื่อถือให้ `throw std::runtime_error(...)` ภายใน `getDeviceInfo()` แล้ว `data.valid` จะเป็น `false` และ `main.cpp` จะข้ามทั้งรอบ (ไม่ publish, ไม่วาดจอ) ห้ามส่งค่า 0 หรือค่า default แทน
3. **ห้ามใส่การเช็ค sentinel แบบเหมารวมทุก register** register INT16 ที่ได้ `0xFFFF` คือ -1 ซึ่งเป็นค่าที่ถูกต้อง (เช่น power factor 37117, อุณหภูมิ 32087, กระแส PV) และ PV3 (32020/32021) บนรุ่นที่ไม่มี MPPT ตัวที่ 3 อาจคืนค่าแปลก ๆ ถ้าเช็คเหมารวม ข้อมูลจะถูกทิ้งทุกรอบ ให้เช็คเฉพาะ register ที่รู้ความหมายชัดเจน
4. ห้ามเปิด Modbus-TCP connection ค้างไว้ตลอดเวลา (ดูหัวข้อ Modbus-TCP)

## Register พลังงานสะสม (มักสับสน)

อ้างอิง `doc/Modbus Interface Definitions.pdf`

| Register | ชื่อในคู่มือ | ชนิด | ความหมาย | Field ใน payload | ค่า "ไม่มีข้อมูล" |
|---|---|---|---|---|---|
| 32106 | Accumulated energy yield | U32, gain 100, kWh | PV ผลิตสะสม | `accumulatedEnergy` | `0xFFFFFFFF` |
| 37119 | Positive active electricity | I32, gain 100, kWh | ไฟที่ **ขายเข้า** grid สะสม | `gridExportEnergy` | `0x7FFFFFFF` |
| 37121 | Reverse active power | I32, gain 100, kWh | ไฟที่ **ซื้อจาก** grid สะสม | `gridImportEnergy` | `0x7FFFFFFF` |
| 37113 | Active power (meter) | I32, gain 1000, kW | > 0 ขายเข้า grid, < 0 ซื้อจาก grid | `grid_power` | |
| 37100 | Meter status | U16 | 0 = offline, 1 = normal | `meterStatus` (`"Offline"`/`"Normal"`) | |

**"Positive" = ขาย (export) ไม่ใช่ซื้อ** เคยมีการแนะนำผิดในอดีตว่า 37119 คือยอดซื้อ ห้ามทำซ้ำ

## เหตุการณ์ไฟดับ ต.ค. 2026 และการแก้

### อาการ
หลังไฟดับวันที่ 25 และ 27 ต.ค. 2026 Bill Energy Card แสดงกริด 85,878,273 kWh และโหลด 171,777,794 kWh (ค่าไฟหลายร้อยล้านบาท)

### ต้นเหตุ
1. มิเตอร์ DTSU666 กินไฟจาก grid พอไฟดับมิเตอร์ดับตาม แต่ inverter ยังได้ไฟจากฝั่ง PV และตอบ Modbus ได้
2. การอ่านจึง **สำเร็จ** (ไม่ error) แต่ register พลังงานสะสมคืนค่า "ไม่มีข้อมูล" (`0x7FFFFFFF` ÷ 100 ≈ 21.47 ล้าน kWh หรือ `0xFFFFFFFF` ÷ 100 ≈ 42.95 ล้าน kWh) หรือคืนค่า 0
3. เดิมโค้ดกันได้แค่กรณีอ่านไม่สำเร็จ ค่าขยะจึงถูก publish ออกไป
4. ใน HA sensor เป็น `total_increasing` เมื่อค่ากระโดดขึ้น `utility_meter` จะนับส่วนต่างทั้งหมดเป็นยอดใช้ไฟ พอค่าลดกลับมาเป็นปกติ มันมองว่าเป็นการ reset มิเตอร์ จึงไม่หักคืน ยอดผิดจึงค้างถาวร

### การแก้ (ฝั่ง ESP32 อย่างเดียว ไม่แตะ payload)
ใน `src/HuaweiSun2000Client.h`:
- `parseEnergyRegister()` ใช้อ่าน 32106/37119/37121 ถ้าค่า raw เป็น `0xFFFFFFFF` หรือ `0x7FFFFFFF` จะ throw
- `validateEnergyCounters()` เรียกก่อนตั้ง `data.valid = true` จะ throw เมื่อ:
  - มิเตอร์ offline (register 37100 = 0)
  - `gridImportEnergy <= 0` หรือ `accumulatedEnergy <= 0` (ไม่เช็ค export เพราะระบบ zero-export อาจเป็น 0 จริง)
  - ค่าสะสมตัวใดลดลงจากค่าดีล่าสุด หรือเพิ่มเร็วกว่า `MAX_PLAUSIBLE_POWER_KW` (ใน `settings.h`) ตามเวลาที่ผ่านไป

ใน `src/main.cpp`: ไม่ตั้ง baseline ของ `dailyGridImport` จนกว่า NTP จะ sync เช็คด้วย `getLocalTime(&now, 0)` ของ ESP32 framework (คืน `false` จนกว่าเวลาจะถูกตั้ง) ห้าม hardcode ปีเพื่อเช็คเอง
- เหตุผล: `setupTimeZone()` ใน `src/utility.h` รอ NTP ด้วย `while (!time(nullptr))` แต่ `time()` คืนวินาทีนับจากบูตซึ่งไม่ใช่ 0 ลูปจึงไม่ได้รอจริง หลังไฟดับแล้วรีบูต รอบแรก ๆ นาฬิกาจะเป็นปี 1970

### ข้อจำกัดที่รู้อยู่
- ระบบที่ไม่มีมิเตอร์ (37100 = 0 ตลอด) จะถูกข้ามทุกรอบ ทั้ง 2 เครื่องในโปรเจกต์นี้มีมิเตอร์
- "ค่าดีล่าสุด" เก็บใน RAM หลังรีบูตค่าแรกจะถูกเช็คแค่ sentinel, 0 และสถานะมิเตอร์
- `dailyGridImport` จะเริ่มนับใหม่จากตอนรีบูต (baseline อยู่ใน RAM)
- ยังไม่ได้ทดสอบกับไฟดับจริง เมื่อเกิดขึ้น Serial Monitor ควรแสดง `-- Skipped: invalid inverter read` พร้อมสาเหตุ

## ฝั่ง Home Assistant

Template sensor (ต้องเป็น `device_class: energy`, `state_class: total_increasing`, หน่วย kWh):

```yaml
# sensor.huawei_grid_import_energy  (ยอดซื้อสะสม)
{{ state_attr('sensor.huawei_inverter_data', 'deviceState').gridImportEnergy | float(0) }}

# sensor.huawei_load_energy  (โหลด = PV + ซื้อ − ขาย)
{% set d = state_attr('sensor.huawei_inverter_data', 'deviceState') %}
{{ (d.accumulatedEnergy | float(0) + d.gridImportEnergy | float(0) - d.gridExportEnergy | float(0)) | round(2) }}
```

- ควรเพิ่ม `availability:` ให้ template เพื่อให้เป็น `unavailable` แทนที่ `float(0)` จะแปลงเป็น 0 (กรณี ESP32/MQTT หลุดขณะที่ HA ยังทำงานบน UPS)
- `utility_meter` 4 ตัว (`grid_daily`, `grid_cycle`, `load_daily`, `load_cycle`) ตั้ง "No cycle" แล้วให้ automation เรียก `utility_meter.reset` ตาม `input_number.pea_cutoff_day` / `input_number.pea_cutoff_hour` (ตาม SETUP_GUIDE ของ Bill Energy Card)
- `utility_meter` ที่เพิ่งสร้างหรือเพิ่ง reset จะเป็น `unknown` จนกว่า source จะเปลี่ยนค่า ถือเป็นเรื่องปกติ
- ข้อมูลที่เพี้ยนไปแล้วต้องแก้เองใน Developer Tools > Statistics และใช้ `utility_meter.calibrate` โค้ดแก้ย้อนหลังให้ไม่ได้

## Modbus-TCP ผ่าน Smart Dongle (SDongleA-05)

- ต้องเปิดในแอป FusionSolar: Me > Device Commissioning > ต่อ WiFi ของ inverter (`SUN2000-<SN>`) > login `installer` > Settings > Communication configuration > Dongle parameter settings > Modbus-TCP ค่าเริ่มต้นคือปิด
- พอร์ตเป็น 502 หรือ 6607 ขึ้นกับเฟิร์มแวร์ของ dongle แอปไม่แสดงพอร์ต ให้เช็คด้วย `nc -zv <ip> 502`
- รับ client ได้ทีละ 1 ตัว การค้าง connection ไว้มีรายงานว่าทำให้ FusionSolar หยุด sync ให้ connect → อ่าน → ปิด และ poll ไม่ถี่เกินไป
- เฟิร์มแวร์ dongle บางรุ่นไม่ส่งต่อ request ไปที่ inverter (ได้แต่ register ของ dongle เอง) ทดสอบด้วย `node Test_HuaweiSun2000Client.js` ว่าอ่าน register 30000 ได้ชื่อรุ่นหรือไม่
- ถ้าต่อ ESP32 เข้า RS485 (COM pin 1 = A, pin 3 = B บน M1) ขณะที่มี Smart Dongle อยู่ จะชนกัน เพราะ Modbus-RTU มี master ได้ตัวเดียว
