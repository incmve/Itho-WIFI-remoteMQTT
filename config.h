/*
 * Change to your setup.
 */

// WiFi
#define CONFIG_WIFI_SSID "SSID"
#define CONFIG_WIFI_PASS "password"
#define CONFIG_WIFI_NAME "ESPhostname"

// MQTT
#define CONFIG_MQTT_HOST "MQTT broker"
#define CONFIG_MQTT_USER "username"
#define CONFIG_MQTT_PASS "password"
#define CONFIG_MQTT_CLIENT_ID "MQTTClientID" // Must be unique on the MQTT network

// MQTT Topics
#define CONFIG_MQTT_TOPIC_TEMP "itho/temperature"
#define CONFIG_MQTT_TOPIC_HUMID "itho/humidity"
#define CONFIG_MQTT_TOPIC_RESPONSE "itho/repsonse"
#define CONFIG_MQTT_TOPIC_COMMAND "itho/command"

// DHT
// Define the DHT type in the code file.
#define CONFIG_DHT_ENABLE 0 // 0 = disable and 1 = enable
#define CONFIG_DHT_PIN 2 
#define CONFIG_DHT_TYPE DHT22
#define CONFIG_DHT_SAMPLE_DELAY 60000 // Milliseconds between readings
