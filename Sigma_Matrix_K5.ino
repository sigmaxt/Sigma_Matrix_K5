#include "Settings.h"
#include "Secret.h"
#include "Layout.h"
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <PubSubClient.h>         // https://github.com/knolleary/pubsubclient
#include <Keypad.h>               // https://github.com/Chris--A/Keypad
#include <BleKeyboard.h>          // https://github.com/T-vK/ESP32-BLE-Keyboard
#include <Adafruit_SSD1306.h>     // https://github.com/adafruit/Adafruit_SSD1306 https://github.com/adafruit/Adafruit-GFX-Library
#include <SchedTask.h>            // https://github.com/Nospampls/SchedTask
#include "AiEsp32RotaryEncoder.h" // https://github.com/igorantolic/ai-esp32-rotary-encoder

//#include <ezButton.h>             // https://github.com/ArduinoGetStarted/button
int i;
const byte ledPin = 2;
int Shift = 0;
int Mode = 0;
int ModeMax = 3;
int dispSleepT = 5000;
int keyReltime = 50;
bool MQTT = true;
String Shifted;
String Unshifted;
String msg; // Serial message for key PRESSED HOLD RELASED IDLE
unsigned long loopCount;
unsigned long startTime;
char KeyPressed;
const byte ROWS = 5; // four rows
const byte COLS = 5; // three columns
char keys[ROWS][COLS] = {
    {'1', '2', '3', 'A', 'M'},
    {'4', '5', '6', 'B', 'L'},
    {'7', '8', '9', 'C', 'K'},
    {'*', '0', '#', 'D', 'J'},
    {'E', 'F', 'G', 'H', 'I'}};
String fullKey;
byte rowPins[ROWS] = {33, 25, 26, 27, 13}; // connect to the row pinouts of the Keypad1
byte colPins[COLS] = {19, 18, 17, 16, 4};  // connect to the column pinouts of the Keypad1

void DispOff();
SchedTask taskDispOff(NEVER, ONESHOT, DispOff);

AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(ROTARY_ENCODER_A_PIN, ROTARY_ENCODER_B_PIN, ROTARY_ENCODER_BUTTON_PIN, -1, ROTARY_ENCODER_STEPS);

Keypad Keypad1 = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

BleKeyboard bleKeyboard("Sigma_Matrix_K5", "Sigma", 100);

WiFiClient espClientK5;

PubSubClient mqttclient(espClientK5);

void IRAM_ATTR readEncoderISR()
{
        rotaryEncoder.readEncoder_ISR();
}

void wificn() // Connect to WiFi
{
        if (WiFi.status() != WL_CONNECTED)
        {
                WiFi.begin(ssid, password);
                Serial.print("Connecting to ");
                Serial.print(ssid);
                Serial.println(" ...");

                int i = 0;
                while (WiFi.status() != WL_CONNECTED)
                {
                        delay(1000);
                        Serial.print(++i);
                        Serial.print(' ');
                }
                Serial.println('\n');
                Serial.println("Connection established!");
                Serial.print("IP address:\t");
                Serial.println(WiFi.localIP());
        }
}

void mqttcn() // Connect to MQTT
{
        mqttclient.setServer(mqttServer, mqttPort);
        mqttclient.setCallback(mqttcallback);

        while (!mqttclient.connected())
        {
                Serial.println("Connecting to MQTT...");

                if (mqttclient.connect("Keypad Client", mqttUser, mqttPassword))
                {
                        mqttclient.subscribe("esp32/output");
                        Serial.println("connected");
                        mqttclient.loop();
                }
                else
                {
                        Serial.print("failed with state ");
                        Serial.print(mqttclient.state());

                        delay(2000);
                }
                mqttclient.publish("MatrixR/LWT", "1");
        }
}

void mqttcallback(char *topic, byte *message, unsigned int length)
{
        Serial.print("Message arrived on topic: ");
        Serial.print(topic);
        Serial.print(". Message: ");
        String messageTemp;

        for (int c = 0; c < length; c++)
        {
                Serial.print((char)message[c]);
                messageTemp += (char)message[c];
        }
        Serial.println();

        // Feel free to add more if statements to control more GPIOs with MQTT

        // If a message is received on the topic esp32/output, you check if the message is either "on" or "off".
        // Changes the output state according to the message
        if (String(topic) == "esp32/output")
        {
                Serial.print("Changing output to ");
                if (messageTemp == "on")
                {
                        Serial.println("on");
                }
                else if (messageTemp == "off")
                {
                        Serial.println("off");
                }
                else if (messageTemp == "tog")
                        Serial.println("Tog");
        }
}

void httpReq(String serverPath)
{
        Serial.println("httpReq");
        // Your Domain name with URL path or IP address with path
        HTTPClient http;
        http.begin(serverPath.c_str());

        // Send HTTP GET request
        int httpResponseCode = http.GET();

        if (httpResponseCode > 0)
        {
                Serial.print("HTTP Response code: ");
                Serial.println(httpResponseCode);
                String payload = http.getString();
                Serial.println(payload);
        }
        else
        {
                Serial.print("Error code: ");
                Serial.println(httpResponseCode);
        }
}

void macro() // Mode 0
{
        switch (KeyPressed)
        {
        case '1': //        Upload Sketch | Shift 1: Compile Sketch  | Shift 2: Comment Selecttion| Shift 3: Uncomment Selection
        {
                switch (Shift)
                {
                case 0:
                {
                        bleKeyboard.press(KEY_LEFT_CTRL);
                        bleKeyboard.press(KEY_LEFT_ALT);
                        bleKeyboard.press('u');
                        delay(keyReltime);
                        bleKeyboard.releaseAll();
                        break;
                }
                case 1:
                {
                        bleKeyboard.press(KEY_LEFT_CTRL);
                        bleKeyboard.press(KEY_LEFT_ALT);
                        bleKeyboard.press('r');
                        delay(keyReltime);
                        bleKeyboard.releaseAll();
                        break;
                }
                case 2:
                {
                        bleKeyboard.press(KEY_LEFT_CTRL);
                        bleKeyboard.press('k');
                        bleKeyboard.press('c');
                        delay(keyReltime);
                        bleKeyboard.releaseAll();
                        break;
                }
                case 3:
                {
                        bleKeyboard.press(KEY_LEFT_CTRL);
                        bleKeyboard.press('k');
                        bleKeyboard.press('u');
                        delay(keyReltime);
                        bleKeyboard.releaseAll();
                        break;
                }
                }
        }
        break;
        case '2': //        Collapse Code in vscode | Shift 1: Expand Code in vscode | Shift 2:  Open | Shift 3: Open
        {
                switch (Shift)
                {
                case 0:
                {
                        bleKeyboard.press(KEY_LEFT_CTRL);
                        bleKeyboard.press('k');
                        bleKeyboard.press('0');
                        delay(keyReltime);
                        bleKeyboard.releaseAll();
                        break;
                }
                case 1:
                {
                        bleKeyboard.press(KEY_LEFT_CTRL);
                        bleKeyboard.press('k');
                        bleKeyboard.press('j');
                        delay(keyReltime);
                        bleKeyboard.releaseAll();
                        break;
                }
                case 2:
                {

                        break;
                }
                case 3:
                {

                        break;
                }
                }
        }
        break;
        case '3': //        Open | Shift 1: Open | Shift 2:  Open | Shift 3: Open
        {
                switch (Shift)
                {
                case 0:
                {

                        break;
                }
                case 1:
                {

                        break;
                }
                case 2:
                {

                        break;
                }
                case 3:
                {

                        break;
                }
                }
        }
        break;
        case 'A': //        Bulb-01 Toggle | Shift 1: Bulb-02 Toggle | Shift 2:  Bulb-03 | Shift 3: Outlet-03
        {
                switch (Shift)
                {
                case 0:
                {
                        httpReq("http://192.168.1.151/control?cmd=event,ToggleMCP");
                        break;
                }
                case 1:
                {
                        httpReq("http://192.168.1.152/control?cmd=event,ToggleMCP");
                        break;
                }
                case 2:
                {
                        httpReq("http://192.168.1.153/control?cmd=event,ToggleMCP");
                        break;
                }
                case 3:
                {
                        httpReq("http://192.168.1.143/control?cmd=event,Toggle");
                        break;
                }
                }
        }
        break;
        case 'M': //        Light | Shift 1: Home | Shift 2: Open | Shift 3: Leaving
        {
                switch (Shift)
                {
                case 0:
                {
                        httpReq("http://192.168.1.151/control?cmd=event,ToggleMCP");
                        break;
                }
                case 1:
                {

                        mqttclient.publish("/routines", "1");
                        break;
                }
                case 2:
                {
                        break;
                }
                case 3:
                {
                        mqttclient.publish("/routines", "0");
                        break;
                }
                }
        }
        break;
        case '4': //        Kodi Stop | Shift 1: Open | Shift 2: Open | Shift 3: Open
        {
                switch (Shift)
                {
                case 0:
                {
                        mqttclient.publish("/Kodi/CMD", "Stop");
                        break;
                }
                case 1:
                {

                        break;
                }
                case 2:
                {

                        break;
                }
                case 3:
                {

                        break;
                }
                }
        }
        break;
        case '5': //        Kodi Play/Pause  | Shift 1: Kodi Zoom | Shift 2:  | Shift 3:
        {
                switch (Shift)
                {
                        {
                        case 0:
                        {
                                mqttclient.publish("/Kodi/CMD", "Pause");
                                break;
                        }
                        case 1:
                        {
                                mqttclient.publish("/Kodi/CMD", "Z");
                                break;
                        }
                        case 2:
                        {

                                break;
                        }
                        case 3:
                        {

                                break;
                        }
                        }
                }
        }
        break;
        case '6': //        Open | Shift 1: Open | Shift 2:  Open | Shift 3: Open
        {
                switch (Shift)
                {
                        {
                        case 0:
                        {

                                break;
                        }
                        case 1:
                        {

                                break;
                        }
                        case 2:
                        {

                                break;
                        }
                        case 3:
                        {

                                break;
                        }
                        }
                }
        }
        break;
        case 'B': //        Over H 1/0 | Shift 1: Over H 10%  | Shift 2: Outlet-06 | Shift 3: Open
        {
                switch (Shift)
                {
                        {
                        case 0:
                        {
                                mqttclient.publish("/MatrixR/CMD", "12");
                                break;
                        }
                        case 1:
                        {
                                mqttclient.publish("/MatrixR/CMD", "13");
                                break;
                        }
                        case 2:
                        {
                                httpReq("http://192.168.1.143/control?cmd=event,Toggle");
                                break;
                        }
                        case 3:
                        {

                                break;
                        }
                        }
                }
        }
        break;
        case 'L': //        Copy | Paste | Shift 2 Open | Cut
        {
                switch (Shift)
                {
                case 0:
                {
                        bleKeyboard.press(KEY_LEFT_CTRL);
                        bleKeyboard.press('c');
                        delay(keyReltime);
                        bleKeyboard.releaseAll();
                        break;
                }
                case 1:
                {

                        bleKeyboard.press(KEY_LEFT_CTRL);
                        bleKeyboard.press('v');
                        delay(keyReltime);
                        bleKeyboard.releaseAll();
                        break;
                }
                case 2:
                {

                        break;
                }
                case 3:
                {
                        bleKeyboard.press(KEY_LEFT_CTRL);
                        bleKeyboard.press('x');
                        delay(keyReltime);
                        bleKeyboard.releaseAll();
                        break;
                }
                }
        }
        break;
        case '7': //        Kodi_BACKSPACE | Shift 1: Open | Shift 2: Open | Shift 3: Monitor Off
        {
                switch (Shift)
                {
                        {
                        case 0:
                        {
                                mqttclient.publish("/Kodi/CMD", "Back");
                                break;
                        }
                        case 1:
                        {
                                bleKeyboard.write(KEY_MEDIA_VOLUME_UP);
                                // delay(keyReltime);
                                // bleKeyboard.releaseAll();
                                break;
                        }
                        case 2:
                        {
                                bleKeyboard.write(KEY_MEDIA_VOLUME_DOWN);
                                // delay(keyReltime);
                                // bleKeyboard.releaseAll();
                                break;
                        }
                        case 3:
                        {
                                httpReq("http://192.168.1.122:8655/monitor?mode=off");
                                break;
                        }
                        }
                }
        }
        break;
        case '8': //        Kodi_UP_ARROW | Shift 1: +30 | Shift 2: Open | Shift 3: Hibernate
        {
                switch (Shift)
                {
                        {
                        case 0:
                        {
                                mqttclient.publish("/Kodi/CMD", "Up");
                                break;
                        }
                        case 1:
                        {
                                mqttclient.publish("/Kodi/CMD", "bFwd");
                                break;
                        }
                        case 2:
                        {

                                break;
                        }
                        case 3:
                        {
                                httpReq("http://192.168.1.122:8655/pc?mode=hibernate");
                                break;
                        }
                        }
                }
        }
        break;
        case '9': //        Kodi Select | Shift 1: Kodi Context | Shift 2: Open | Shift 3: Open
        {
                switch (Shift)
                {
                        {
                        case 0:
                        {
                                mqttclient.publish("/Kodi/CMD", "Select");
                                break;
                        }
                        case 1:
                        {
                                mqttclient.publish("/Kodi/CMD", "context");
                                break;
                        }
                        case 2:
                        {

                                break;
                        }
                        case 3:
                        {

                                break;
                        }
                        }
                }
        }
        break;
        case 'C': //        Outlet-08 Toggle | Shift 1: Open | Shift 2: Open | Shift 3: Open
        {
                switch (Shift)
                {
                        {
                        case 0:
                        {
                                httpReq("http://192.168.1.148/control?cmd=event,Toggle");
                                break;
                        }
                        case 1:
                        {

                                break;
                        }
                        case 2:
                        {

                                break;
                        }
                        case 3:
                        {

                                break;
                        }
                        }
                }
        }
        break;
        case 'K': //        Redo | Shift 1: Open | Shift 2: Open | Shift 3: Open
        {
                switch (Shift)
                {
                case 0:
                {
                        bleKeyboard.press(KEY_LEFT_CTRL);
                        bleKeyboard.press(KEY_LEFT_SHIFT);
                        bleKeyboard.press('z');
                        delay(keyReltime);
                        bleKeyboard.releaseAll();
                        break;
                }
                case 1:
                {

                        break;
                }
                case 2:
                {

                        break;
                }
                case 3:
                {

                        break;
                }
                }
        }
        break;
        case '*': //        Kodi_LEFT_ARROW | Shift 1: -10 | Shift 2: Open | Shift 3: Open
        {
                switch (Shift)
                {
                        {
                        case 0:
                        {
                                mqttclient.publish("/Kodi/CMD", "Left");
                                break;
                        }
                        case 1:
                        {
                                mqttclient.publish("/Kodi/CMD", "Rew");
                                break;
                        }
                        case 2:
                        {

                                break;
                        }
                        case 3:
                        {

                                break;
                        }
                        }
                }
        }
        break;
        case '0': //        Kodi_DOWN_ARROW | Shift 1: -30 | Shift 2: Open | Shift 3: Open
        {
                switch (Shift)
                {
                        {
                        case 0:
                        {
                                mqttclient.publish("/Kodi/CMD", "Down");
                                break;
                        }
                        case 1:
                        {
                                mqttclient.publish("/Kodi/CMD", "bRew");
                                break;
                        }
                        case 2:
                        {

                                break;
                        }
                        case 3:
                        {

                                break;
                        }
                        }
                }
        }
        break;
        case '#': //        Kodi_RIGHT_ARROW | Shift 1: +10 | Shift 2: OPEN | Shift 3: OPEN
        {
                switch (Shift)
                {
                        {
                        case 0:
                        {
                                mqttclient.publish("/Kodi/CMD", "Right");
                                break;
                        }
                        case 1:
                        {
                                mqttclient.publish("/Kodi/CMD", "Fwd");
                                break;
                        }
                        case 2:
                        {

                                break;
                        }
                        case 3:
                        {

                                break;
                        }
                        }
                }
        }
        break;
        case 'D': //        Outlet-05 Toggle| Shift 1: Open  | Shift 2: Open | Shift 3: Open
        {

                switch (Shift)
                {
                case 0:
                {
                        httpReq("http://192.168.1.145/control?cmd=event,Toggle");
                        break;
                }
                case 1:
                {
                        break;
                }
                case 2:
                {
                        break;
                }
                case 3:
                {
                        break;
                }
                }
        }
        break;
        case 'J': //        Undo | Shift 1:  | Shift 2:  | Shift 3:
        {
                switch (Shift)
                {
                case 0:
                {
                        bleKeyboard.press(KEY_LEFT_CTRL);
                        bleKeyboard.press('z');
                        delay(keyReltime);
                        bleKeyboard.releaseAll();
                        break;
                }
                case 1:
                {

                        break;
                }
                case 2:
                {

                        break;
                }
                case 3:
                {

                        break;
                }
                }
        }
        break;
        }
}

void singleKey() //  Mode 1
{
        char Key1 = Keypad1.key[i].kchar;
        fullKey = Key1;
        fullKey.toCharArray(KeyMQTT, fullKey.length() + 1);
        if (Key1 != NULL)
        {
                switch (Shift)
                {
                case 0:
                {
                        if (Key1 != 'E' && Key1 != 'F' && Key1 != 'G' && Key1 != 'H' && Key1 != 'I')
                        {
                                mqttclient.publish("/MatrixR/KEY", KeyMQTT);
                                break;
                        }
                        else
                                break;
                }
                case 1:
                {
                        mqttclient.publish("/MatrixR/KEY1", KeyMQTT);
                        break;
                }
                case 2:
                {
                        mqttclient.publish("/MatrixR/KEY2", KeyMQTT);
                        break;
                }
                case 3:
                {
                        mqttclient.publish("/MatrixR/KEY3", KeyMQTT);
                        break;
                }
                }

                display.clearDisplay();
                display.setCursor(0, 0);
                display.setTextSize(2);
                display.setTextColor(SSD1306_WHITE);
                display.print("Sent:");
                display.print(Key1);
                display.println(" To:");
                display.println("");
                display.println("/MatrixR  /KEY");
                display.display();
                taskDispOff.setNext(dispSleepT);
        }
}

void keyCode() // Mode 2
{
        char Key1 = Keypad1.key[i].kchar;
        if (Key1 == sendKey)
        {
                fullKey.toCharArray(KeyMQTT, fullKey.length() + 1);
                mqttclient.publish("MatrixR/Code", KeyMQTT);
                display.clearDisplay();
                display.setCursor(0, 0);
                display.setTextSize(2);
                display.setTextColor(SSD1306_WHITE);
                display.println(fullKey);
                display.println("Sent To:");
                display.println("/MatrixR /Code");
                display.display();
                fullKey = "";
                taskDispOff.setNext(dispSleepT);
        }
        else if (Key1 != NULL)
        {
                fullKey += Key1;
                display.clearDisplay();
                display.setCursor(0, 0);
                display.setTextSize(2);
                display.setTextColor(SSD1306_WHITE);
                display.println("Code:");
                display.println("");
                display.println(fullKey);
                display.display();
                taskDispOff.setNext(-1);
        }
}

void pcKeypad() // Mode 3
{

        switch (KeyPressed)
        {
        case '1': //    KEY_NUM_1 | KEY_F13 | Open | Open
        {
                switch (Shift)
                {
                case 0:
                {
                        bleKeyboard.write(KEY_NUM_1);
                        break;
                }
                case 1:
                {
                        bleKeyboard.write(KEY_F13);
                        break;
                }
                case 2:
                {

                        break;
                }
                case 3:
                {

                        break;
                }
                }
        }
        break;
        case '2': //    KEY_NUM_2 | KEY_F14 | Open | Open
        {
                switch (Shift)
                {
                case 0:
                {
                        bleKeyboard.write(KEY_NUM_2);
                        break;
                }
                case 1:
                {
                        bleKeyboard.write(KEY_F14);
                        break;
                }
                case 2:
                {

                        break;
                }
                case 3:
                {

                        break;
                }
                }
        }
        break;
        case '3': //    KEY_NUM_3 | KEY_F15 | Open | Open
        {
                switch (Shift)
                {
                case 0:
                {
                        bleKeyboard.write(KEY_NUM_3);
                        break;
                }
                case 1:
                {
                        bleKeyboard.write(KEY_F15);
                        break;
                }
                case 2:
                {

                        break;
                }
                case 3:
                {

                        break;
                }
                }
        }
        break;
        case 'A': //   KEY_NUM_ASTERISK | Open | Open | Open
        {
                switch (Shift)
                {
                case 0:
                {
                        bleKeyboard.write(KEY_NUM_ASTERISK);
                        break;
                }
                case 1:
                {

                        break;
                }
                case 2:
                {

                        break;
                }
                case 3:
                {

                        break;
                }
                }
        }
        break;
        case 'M': //    " mm" | Open | Open | Open
        {
                switch (Shift)
                {
                case 0:
                {
                        bleKeyboard.print(" mm");
                        break;
                }
                case 1:
                {

                        break;
                }
                case 2:
                {

                        break;
                }
                case 3:
                {

                        break;
                }
                }
        }
        break;
        case '4': //    KEY_NUM_4 | KEY_F16 | Open | Open
        {
                switch (Shift)
                {
                case 0:
                {
                        bleKeyboard.write(KEY_NUM_4);
                        break;
                }
                case 1:
                {
                        bleKeyboard.write(KEY_F16);
                        break;
                }
                case 2:
                {

                        break;
                }
                case 3:
                {

                        break;
                }
                }
        }
        break;
        case '5': //    KEY_NUM_5 | KEY_F17 | Open | Open
        {
                switch (Shift)
                {
                case 0:
                {
                        bleKeyboard.write(KEY_NUM_5);
                        break;
                }
                case 1:
                {
                        bleKeyboard.write(KEY_F17);
                        break;
                }
                case 2:
                {

                        break;
                }
                case 3:
                {

                        break;
                }
                }
        }
        break;
        case '6': //    KEY_NUM_6 | KEY_F18 | Open | Open
        {
                switch (Shift)
                {
                case 0:
                {
                        bleKeyboard.write(KEY_NUM_6);
                        break;
                }
                case 1:
                {
                        bleKeyboard.write(KEY_F18);
                        break;
                }
                case 2:
                {

                        break;
                }
                case 3:
                {

                        break;
                }
                }
        }
        break;
        case 'B': //    KEY_NUM_MINUS | Open | Open | Open
        {
                switch (Shift)
                {
                case 0:
                {
                        bleKeyboard.write(KEY_NUM_MINUS);
                        break;
                }
                case 1:
                {

                        break;
                }
                case 2:
                {

                        break;
                }
                case 3:
                {

                        break;
                }
                }
        }
        break;
        case 'L': //    " inch" | Open | Open | Open
        {
                switch (Shift)
                {
                case 0:
                {
                        bleKeyboard.print(" inch");
                        break;
                }
                case 1:
                {

                        break;
                }
                case 2:
                {

                        break;
                }
                case 3:
                {

                        break;
                }
                }
        }
        break;
        case '7': //    KEY_NUM_7 | KEY_F19 | Open | Open
        {
                switch (Shift)
                {
                case 0:
                {
                        bleKeyboard.write(KEY_NUM_7);
                        break;
                }
                case 1:
                {
                        bleKeyboard.write(KEY_F19);
                        break;
                }
                case 2:
                {

                        break;
                }
                case 3:
                {

                        break;
                }
                }
        }
        break;
        case '8': //    KEY_NUM_8 | KEY_F20 | Open | Open
        {
                switch (Shift)
                {
                case 0:
                {
                        bleKeyboard.write(KEY_NUM_8);
                        break;
                }
                case 1:
                {
                        bleKeyboard.write(KEY_F20);
                        break;
                }
                case 2:
                {

                        break;
                }
                case 3:
                {

                        break;
                }
                }
        }
        break;
        case '9': //    KEY_NUM_9 | KEY_F21 | Open | Open
        {
                switch (Shift)
                {
                case 0:
                {
                        bleKeyboard.write(KEY_NUM_9);
                        break;
                }
                case 1:
                {
                        bleKeyboard.write(KEY_F21);
                        break;
                }
                case 2:
                {

                        break;
                }
                case 3:
                {

                        break;
                }
                }
        }
        break;
        case 'C': //    KEY_NUM_PLUS | Open | Open | Open
        {
                switch (Shift)
                {
                case 0:
                {
                        bleKeyboard.write(KEY_NUM_PLUS);
                        break;
                }
                case 1:
                {

                        break;
                }
                case 2:
                {

                        break;
                }
                case 3:
                {

                        break;
                }
                }
        }
        break;
        case 'K': //    KEY_TAB | Open | Open | KEY_LEFT_SHIFT + KEY_TAB
        {
                switch (Shift)
                {
                case 0:
                {
                        bleKeyboard.write(KEY_TAB);
                        break;
                }
                case 1:
                {

                        break;
                }
                case 2:
                {

                        break;
                }
                case 3:
                {
                        bleKeyboard.press(KEY_LEFT_SHIFT);
                        bleKeyboard.press(KEY_TAB);
                        delay(keyReltime);
                        bleKeyboard.releaseAll();
                        break;
                }
                }
        }
        break;
        case '*': //    KEY_NUM_PERIOD | KEY_F22 | Open | Open
        {
                switch (Shift)
                {
                case 0:
                {
                        bleKeyboard.write(KEY_NUM_PERIOD);
                        break;
                }
                case 1:
                {
                        bleKeyboard.write(KEY_F22);
                        break;
                }
                case 2:
                {

                        break;
                }
                case 3:
                {

                        break;
                }
                }
        }
        break;
        case '0': //    KEY_NUM_0 | KEY_F23 | Open | Open
        {
                switch (Shift)
                {
                case 0:
                {
                        bleKeyboard.write(KEY_NUM_0);
                        break;
                }
                case 1:
                {

                        bleKeyboard.write(KEY_F23);
                        break;
                }
                case 2:
                {

                        break;
                }
                case 3:
                {

                        break;
                }
                }
        }
        break;
        case '#': //    KEY_NUM_PERIOD | KEY_F24 | Open | Open
        {
                switch (Shift)
                {
                case 0:
                {
                        bleKeyboard.write(KEY_NUM_PERIOD);
                        break;
                }
                case 1:
                {
                        bleKeyboard.write(KEY_F24);
                        break;
                }
                case 2:
                {

                        break;
                }
                case 3:
                {

                        break;
                }
                }
        }
        break;
        case 'D': //    KEY_NUM_SLASH | Open | Open | Open
        {
                switch (Shift)
                {
                case 0:
                {
                        bleKeyboard.write(KEY_NUM_SLASH);
                        break;
                }
                case 1:
                {

                        break;
                }
                case 2:
                {

                        break;
                }
                case 3:
                {

                        break;
                }
                }
        }
        break;
        case 'J': //    KEY_NUM_ENTER | Open | Open | Open
        {
                switch (Shift)
                {
                case 0:
                {
                        bleKeyboard.write(KEY_NUM_ENTER);
                        break;
                }
                case 1:
                {

                        break;
                }
                case 2:
                {

                        break;
                }
                case 3:
                {

                        break;
                }
                }
        }
        break;
        case 'H': //    KEY_BACKSPACE | Open | Open | Open
        {
                switch (Shift)
                {
                case 0:
                {
                        bleKeyboard.write(KEY_BACKSPACE);
                        break;
                }
                case 1:
                {

                        break;
                }
                case 2:
                {

                        break;
                }
                case 3:
                {

                        break;
                }
                }
        }
        break;
        default:
        {
                break;
        }
        }
        // bleKeyboard.press(Key1);
        delay(keyReltime);
        bleKeyboard.releaseAll();
        display.clearDisplay();
        display.setCursor(0, 0);
        display.setTextSize(2);
        display.setTextColor(SSD1306_WHITE);
        display.print("Pressed: ");
        display.println(KeyPressed);
        display.display();
        taskDispOff.setNext(dispSleepT);
}

void UpdateOled(int Mode)
{
        display.clearDisplay();
        display.setCursor(0, 0);
        display.setTextSize(2);
        display.setTextColor(SSD1306_WHITE);

        switch (Mode)

        {
        case 0:
                display.println("Macro  ");
                break;

        case 1:
                display.println("Single Key");
                break;
        case 2:

                display.println("Key Code");
                break;

        case 3:
                display.println("PC KeyPad");
                break;
        }
        display.print("Mode: ");
        display.println(Mode);
        display.print("Shift: ");
        display.println(Shift);
        display.display();
        taskDispOff.setNext(dispSleepT);
}

void DispOff()
{
        display.clearDisplay();
        display.display();
}

void setup()
{
        Serial.begin(115200);
        loopCount = 0;
        startTime = millis();
        msg = "";

        delay(10);
        Serial.println('\n');
        bleKeyboard.begin();
        wificn();
        mqttcn();

        // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
        if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
        {
                Serial.println(F("SSD1306 allocation failed"));
                for (;;)
                        ; // Don't proceed, loop forever
        }

        // Show initial display buffer contents on the screen --
        // the library initializes this with an Adafruit splash screen.

        display.setRotation(2);
        display.display();
        UpdateOled(Mode);

        rotaryEncoder.begin();
        rotaryEncoder.setup(readEncoderISR);
        rotaryEncoder.setBoundaries(0, 2, true); // minValue, maxValue, circleValues true|false (when max go to min and vice versa)
        // rotaryEncoder.setAcceleration(250);

        taskDispOff.setNext(dispSleepT);
        pinMode(2, OUTPUT);
}

void loop()
{
        SchedBase::dispatcher();
        loopCount++;
        if ((millis() - startTime) > 5000)
        {
                Serial.print("Average loops per second = ");
                Serial.println(loopCount / 5);
                startTime = millis();
                loopCount = 0;
        }

        // Fills Keypad1.key[ ] array with up-to 10 active keys.
        // Returns true if there are ANY active keys.

        if (Keypad1.getKeys())
        {
                for (int i = 0; i < 2; i++) // Scan the whole key list.
                {
                        if (Keypad1.key[i].stateChanged) // Only find keys that have changed state.
                        {
                                switch (Keypad1.key[i].kstate) // Report active key state : IDLE, PRESSED, HOLD, or RELEASED
                                {
                                case PRESSED:
                                {
                                        msg = " PRESSED. ";
                                        switch (Keypad1.key[i].kchar)
                                        {
                                        case 'E':
                                        {
                                                Shift = 1;
                                                digitalWrite(ledPin, HIGH);
                                                break;
                                        }
                                        case 'F':
                                        {
                                                Shift = 2;
                                                digitalWrite(ledPin, HIGH);
                                                break;
                                        }
                                        case 'G':
                                        {
                                                Shift = 3;
                                                digitalWrite(ledPin, HIGH);
                                                break;
                                        }
                                        case 'I':
                                        {

                                                switch (Shift)
                                                {
                                                case 0:
                                                {
                                                        Mode = 0;
                                                        break;
                                                }
                                                case 1:
                                                {
                                                        Mode = 1;
                                                        break;
                                                }
                                                case 2:
                                                {
                                                        Mode = 2;
                                                        break;
                                                }
                                                case 3:
                                                {
                                                        Mode = 3;
                                                        break;
                                                }
                                                }

                                                // Mode = ++Mode;
                                                // if (Mode > ModeMax)
                                                // {
                                                //         Mode = 0;
                                                // }
                                                UpdateOled(Mode);
                                                break;
                                        }
                                        default:
                                        {
                                                KeyPressed = Keypad1.key[i].kchar;
                                                switch (Mode)
                                                {
                                                case 0: // macro
                                                {
                                                        macro();
                                                        break;
                                                }
                                                case 1: // Single Key
                                                {
                                                        singleKey();
                                                        break;
                                                }
                                                case 2: // key Code
                                                {
                                                        keyCode();
                                                        break;
                                                }
                                                case 3: // PC Keypad
                                                {

                                                        KeyPressed = Keypad1.key[i].kchar;
                                                        pcKeypad();
                                                        break;
                                                }
                                                }
                                                break;
                                        }
                                        }
                                        break;
                                }

                                case RELEASED:
                                {
                                        switch (Keypad1.key[i].kchar)
                                        {
                                        case 'E':
                                        {
                                                Shift = 0;
                                                digitalWrite(ledPin, LOW);
                                                break;
                                        }
                                        case 'F':
                                        {
                                                Shift = 0;
                                                digitalWrite(ledPin, LOW);
                                                break;
                                        }
                                        case 'G':
                                        {
                                                Shift = 0;
                                                digitalWrite(ledPin, LOW);
                                                break;
                                        }
                                        case 'I':
                                        {
                                                break;
                                        }
                                        }
                                }
                                break;
                                }
                        }
                }
        }

        // https://www.best-microcontroller-projects.com/rotary-encoder.html
        if (rotaryEncoder.encoderChanged())
        {
                Mode = ++Mode;
                if (Mode > ModeMax)
                {
                        Mode = 0;
                }
                UpdateOled(Mode);
                Serial.println(rotaryEncoder.readEncoder());
        }

        if (rotaryEncoder.isEncoderButtonClicked())
        {

                Serial.println("button pressed");
        }

        if (WiFi.status() != WL_CONNECTED)
        {
                Serial.println("Going to wificn");
                wificn();
        }

        if (MQTT)
                if (!mqttclient.loop())
                {
                        mqttcn();
                }

} // End loops
