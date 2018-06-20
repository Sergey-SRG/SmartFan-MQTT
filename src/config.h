#define PROJECT                  "SmartFan" 
// Wifi
#define WIFI_SSID                "MyWiFi33"
#define WIFI_PASSWORD            "sergey1001wifi"

// MQTT. Leave username/password blank if none
#define MQTT_SERVER              "192.168.0.35"
#define MQTT_PORT                1883
#define MQTT_USER                "sergey"
#define MQTT_PASS                "sergey"
#define MQTT_CLIENT_ID           "SmartFan"

// Topics
#define LWT_TOPIC                "tele/SmartFan/LWT"
#define LOG_TOPIC                "tele/SmartFan/log"

#define JSON_STATE_TOPIC         "tele/SmartFan/json"
#define JSON_SET_TOPIC           "cmnd/SmartFan/json"

#define ON_STATE_TOPIC           "tele/SmartFan/POWER"
#define ON_SET_TOPIC             "cmnd/SmartFan/POWER"

#define OSCILLATE_STATE_TOPIC    "tele/SmartFan/oscillate"
#define OSCILLATE_SET_TOPIC      "cmnd/SmartFan/oscillate"

#define SPEED_STATE_TOPIC        "tele/SmartFan/speed"
#define SPEED_SET_TOPIC          "cmnd/SmartFan/speed"

#define WIND_STATE_TOPIC         "tele/SmartFan/wind"
#define WIND_SET_TOPIC           "cmnd/SmartFan/wind"

#define IRRECEIVED_TOPIC         "tele/SmartFan/irreceived"

// Pin configurations
#define BUT_PIN                  0
#define LOW_PIN                 12                    // 12=relay pin
#define MEDIUM_PIN              14                    // sensor pin
#define HIGH_PIN                 3                    // 3=RX
#define GREEN_LED               13                    // leg green
#define BUZ_PIN                  1                    // не распаян

#define USE_IRRECV
  #ifdef USE_IRRECV
    #define MIN_UNKNOWN_SIZE        24
    #define CAPTURE_BUFFER_SIZE     25
    #define TIMEOUT                 5U
    #define RECV_PIN                 1                // 1= TX
    #define IR_TIME_AVOID_DUPLICATE 500
  #endif //USE_IRRECV