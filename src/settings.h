#ifndef CONFIGS_H
#define CONFIGS_H

// config parameters
#define DEVICE_NAME "Huawei SUN2000-10K-LC0"
#define WIFI_SSID "MyWIFI_SSID"
#define WIFI_PASSWORD "my_wifi_password"

// Socket.IO config
#define SOCKETIO_HOST "192.168.137.102"  // SocketIO host
#define SOCKETIO_PORT 4000               // SocketIO port
#define SOCKETIO_CHANNEL "ESP"
#define ENABLE_SOCKETIO false

// MQTT config (Home Assistant / Mosquitto broker)
#define MQTT_BROKER_HOST "192.168.137.162"
#define MQTT_BROKER_PORT 1883
#define MQTT_USERNAME "mqtt"
#define MQTT_PASSWORD "solar@1234"
#define MQTT_TOPIC_BASE "home/esp32/Huawei_SUN2000_10K_LC0"
#define ENABLE_MQTT true

// Line config
#define LINE_TOKEN "__YOUR_LINE_TOKEN___"
#define DEFAULT_BAUD_RATE 115200
#define API_TIMEOUT 3000
#define ELECTRICITY_PRICE 5.5

#define ENABLE_DEBUG_MODE false
#define ENABLE_LINE_NOTIFY true

#define FIRMWARE_VERSION "1.0.5"
#define FIRMWARE_LASTUPDATE "2026-01-03"

#endif