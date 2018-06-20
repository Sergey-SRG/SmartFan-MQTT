#include "config.h"
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>
#ifdef USE_IRRECV
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>
#endif
#include <ArduinoJson.h>
#include <ArduinoOTA.h>

#define MQTT_MAX_PACKET_SIZE 384

typedef struct{
  bool on;
  bool oscillate;
  int speed;
  int wind;
} smartfan_msg_t;

#ifdef USE_IRRECV
IRrecv irrecv(RECV_PIN, CAPTURE_BUFFER_SIZE, TIMEOUT, true);
decode_results results;
unsigned long ir_lasttime = 0;
#endif

DynamicJsonBuffer jsonBuffer;

smartfan_msg_t msg;

void callback(char *topic, byte *payload, int length);

WiFiClient espClient;
PubSubClient client(MQTT_SERVER, MQTT_PORT, callback, espClient);

void callback(char *topic, byte *payload, int length){
  payload[length] = '\0';
  String str_payload = String((char *)payload);

  String publishing_topic = "";
  String publishing_payload = "";

  if (String(topic) == String(JSON_SET_TOPIC))
  {
    JsonObject &root = jsonBuffer.parseObject((char *)payload);

    if (!root.success())
    {
      return;
    }

    if (root.containsKey("power"))
    {
      msg.on = root["power"];
      publish_to_mqtt(ON_STATE_TOPIC, root["power"]);
    }

    if (root.containsKey("oscillate"))
    {
      msg.oscillate = root["oscillate"];
      publish_to_mqtt(OSCILLATE_STATE_TOPIC, root["oscillate"]);
    }

    if (root.containsKey("speed"))
    {
      String speed = String(root["speed"].asString());
      if (speed == "eco")
      {
        msg.speed = 3;
      }
      else if (speed == "low")
      {
        msg.speed = 0;
      }
      else if (speed == "medium")
      {
        msg.speed = 1;
      }
      else if (speed == "high")
      {
        msg.speed = 2;
      }
      publish_to_mqtt(SPEED_STATE_TOPIC, root["speed"]);
    }

    if (root.containsKey("wind"))
    {
      String wind = String(root["wind"].asString());
      if (wind == "normal")
      {
        msg.wind = 1;
      }
      else if (wind == "sleeping")
      {
        msg.wind = 2;
      }
      else if (wind == "natural")
      {
        msg.wind = 3;
      }
      publish_to_mqtt(WIND_STATE_TOPIC, root["wind"]);
    }
  }
  else if (String(topic) == String(ON_SET_TOPIC))
  {
    msg.on = (str_payload == "true");
    publishing_topic = ON_STATE_TOPIC;
    publishing_payload = (msg.on ? "true" : "false");
  }
  else if (String(topic) == String(OSCILLATE_SET_TOPIC))
  {
    msg.oscillate = (str_payload == "true");
    publishing_topic = OSCILLATE_STATE_TOPIC;
    publishing_payload = (msg.oscillate ? "true" : "false");
  }
  else if (String(topic) == String(SPEED_SET_TOPIC))
  {
    if (str_payload == "eco")
    {
      msg.speed = 3;
    }
    else if (str_payload == "low")
    {
      msg.speed = 0;
    }
    else if (str_payload == "medium")
    {
      msg.speed = 1;
    }
    else if (str_payload == "high")
    {
      msg.speed = 2;
    }
    publishing_topic = SPEED_STATE_TOPIC;
    publishing_payload = String(str_payload).c_str();
  }
  else if (String(topic) == String(WIND_SET_TOPIC))
  {
    if (str_payload == "normal")
    {
      msg.speed = 1;
    }
    else if (str_payload == "sleeping")
    {
      msg.speed = 2;
    }
    else if (str_payload == "natural")
    {
      msg.speed = 3;
    }
    publishing_topic = WIND_STATE_TOPIC;
    publishing_payload = String(str_payload).c_str();
  }
  else
  {
    publish_to_mqtt(LOG_TOPIC, "No topic matched!");
  }

  set_fan_state();

  if (publishing_topic != "" && publishing_payload != "")
  {
    publish_to_mqtt(publishing_topic.c_str(), publishing_payload.c_str());
  }

  JsonObject &publish_root = jsonBuffer.createObject();

  publish_root["power"] = msg.on;
  publish_root["oscillate"] = msg.oscillate;

  String human_speed = "";

  if (msg.speed == 3)
  {
    human_speed = "eco";
  }
  else if (msg.speed == 0)
  {
    human_speed = "low";
  }
  else if (msg.speed == 1)
  {
    human_speed = "medium";
  }
  else if (msg.speed == 2)
  {
    human_speed = "high";
  }
  publish_root["speed"] = human_speed;

  String human_wind = "";

  if (msg.wind == 1)
  {
    human_wind = "normal";
  }
  else if (msg.wind == 2)
  {
    human_wind = "sleeping";
  }
  else if (msg.wind == 3)
  {
    human_wind = "natural";
  }
  publish_root["wind"] = human_wind;

  char buf[256];
  publish_root.printTo(buf, sizeof(buf));

  publish_to_mqtt(JSON_STATE_TOPIC, buf);
}

void setup(){
#ifdef USE_IRRECV
  irrecv.setUnknownThreshold(MIN_UNKNOWN_SIZE);
  irrecv.enableIRIn(); //Enable IR recv
#endif
  pinMode(LOW_PIN, OUTPUT);
  pinMode(MEDIUM_PIN, OUTPUT);
  pinMode(HIGH_PIN, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  //init pin
  digitalWrite(GREEN_LED, HIGH);
  digitalWrite(LOW_PIN, LOW);
  digitalWrite(MEDIUM_PIN, LOW);
  digitalWrite(HIGH_PIN, LOW);
  //end init pin
  msg.on = false;
  msg.oscillate = false;
  msg.speed = 0; // 0 = low, 1 = medium, 2 = high, 3 = eco
  msg.wind = 1;  // 1 = normal, 2 = sleeping, 3 = natural
  
  WiFi.mode(WIFI_STA); //set wifi mode STA
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD); //connect wifi
  
  //hostname for OTA
  char hostname[20];
  sprintf(hostname, "SmartFan-MQTT-%06x", ESP.getChipId()); 
  ArduinoOTA.setHostname(hostname);
  ArduinoOTA.begin();
}

void loop(){
  if (WiFi.status() == WL_CONNECTED)
  {
    if (!client.connected())
    {
      if (client.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS, LWT_TOPIC, 0, 1, "Offline"))
      {
        publish_to_mqtt(LWT_TOPIC, "Online");
        client.subscribe(JSON_SET_TOPIC);
        client.subscribe(ON_SET_TOPIC);
        client.subscribe(OSCILLATE_SET_TOPIC);
        client.subscribe(SPEED_SET_TOPIC);
        client.subscribe(WIND_SET_TOPIC);
      }
    }
    ArduinoOTA.handle();
  }
  else
  {
    delay(500);
  }
#ifdef USE_IRRECV
  if (irrecv.decode(&results))
  {
    unsigned long now = millis();
    if ((now - ir_lasttime > IR_TIME_AVOID_DUPLICATE) && (results.bits > 0))
    {
      ir_lasttime = now;
      publish_to_mqtt(IRRECEIVED_TOPIC, uint64ToString(results.value, 16).c_str());
      tone(BUZ_PIN, 2000, 100);
    }
    irrecv.resume();
  }
#endif
  client.loop();
}

void publish_to_mqtt(const char *topic, const char *payload){
  client.publish(topic, payload, true);
}

void set_fan_state(){
  digitalWrite(LOW_PIN, LOW);
  digitalWrite(MEDIUM_PIN, LOW);
  digitalWrite(HIGH_PIN, LOW);
  digitalWrite(GREEN_LED, HIGH);
  if (msg.on)
  {
    if (msg.oscillate)
    {
      if (msg.speed == 3)
      {
        // On, Oscillate, Eco
      }
      else if (msg.speed == 0)
      {
        // On, Oscillate, Low
        digitalWrite(LOW_PIN, HIGH);
        digitalWrite(GREEN_LED, LOW);
        tone(BUZ_PIN, 2000, 200);
      }
      else if (msg.speed == 1)
      {
        // On, Oscillate, Medium
        digitalWrite(MEDIUM_PIN, HIGH);
        digitalWrite(GREEN_LED, LOW);
        tone(BUZ_PIN, 2000, 200);
      }
      else if (msg.speed == 2)
      {
        // On, Oscillate, High
        digitalWrite(HIGH_PIN, HIGH);
        digitalWrite(GREEN_LED, LOW);
        tone(BUZ_PIN, 2000, 200);
      }
    }
    else
    {
      if (msg.speed == 3)
      {
        // On, Eco
      }
      else if (msg.speed == 0)
      {
        // On, Low
        digitalWrite(LOW_PIN, HIGH);
        digitalWrite(GREEN_LED, LOW);
        tone(BUZ_PIN, 2000, 200);
      }
      else if (msg.speed == 1)
      {
        // On, Medium
        digitalWrite(MEDIUM_PIN, HIGH);
        digitalWrite(GREEN_LED, LOW);
        tone(BUZ_PIN, 2000, 200);
      }
      else if (msg.speed == 2)
      {
        // On, High
        digitalWrite(HIGH_PIN, HIGH);
        digitalWrite(GREEN_LED, LOW);
        tone(BUZ_PIN, 2000, 200);
      }
    }
  }
  else
  {
    // Off
    if (msg.oscillate)
    {
      if (msg.speed == 3)
      {
        // Off, Oscillate, Eco
      }
      else if (msg.speed == 0)
      {
        // Off, Oscillate, Low
        tone(BUZ_PIN, 2000, 400);
      }
      else if (msg.speed == 1)
      {
        // Off, Oscillate, Medium
        tone(BUZ_PIN, 2000, 400);
      }
      else if (msg.speed == 2)
      {
        // Off, Oscillate, High
        tone(BUZ_PIN, 2000, 400);
      }
    }
    else
    {
      if (msg.speed == 3)
      {
        // Off, Eco
      }
      else if (msg.speed == 0)
      {
        // Off, Low
        tone(BUZ_PIN, 2000, 400);
      }
      else if (msg.speed == 1)
      {
        // Off, Medium
        tone(BUZ_PIN, 2000, 400);
      }
      else if (msg.speed == 2)
      {
        // Off, High
        tone(BUZ_PIN, 2000, 400);
      }
    }
  }
}
