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
bool MQTT = true;
String Shifted;
String Unshifted;
String msg; // Serial message for key PRESSED HOLD RELASED IDLE
unsigned long loopCount;
unsigned long startTime;

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

String serverPath = "";

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

    for (int i = 0; i < length; i++)
    {
        Serial.print((char)message[i]);
        messageTemp += (char)message[i];
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

void mode()
{
    if (Keypad1.key[i].kchar == 'E' || Keypad1.key[i].kchar == 'F' || Keypad1.key[i].kchar == 'G' || Keypad1.key[i].kchar == 'H' || Keypad1.key[i].kchar == 'I')
    {
        return;
    }
    switch (Mode)
    {
    case 0: // Macro Mode
    {
        switch (Keypad1.key[i].kchar)
        {
        case '1': //        Shift: Compile Sketch | else: Upload Sketch
        {
            switch (Shift)
            {
            case 0:
            {
                bleKeyboard.press(KEY_LEFT_CTRL);
                bleKeyboard.press(KEY_LEFT_ALT);
                bleKeyboard.press('u');
                delay(100);
                bleKeyboard.releaseAll();
                break;
            }
            case 1:
            {
                bleKeyboard.press(KEY_LEFT_CTRL);
                bleKeyboard.press(KEY_LEFT_ALT);
                bleKeyboard.press('r');
                delay(100);
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
        case '2': //        Shift: Expand | else: Collapse Code in vscode
        {
            if (Shift == 1)
            {
                bleKeyboard.press(KEY_LEFT_CTRL);
                bleKeyboard.press('k');
                bleKeyboard.press('j');
                delay(100);
                bleKeyboard.releaseAll();
            }
            else
            {
                bleKeyboard.press(KEY_LEFT_CTRL);
                bleKeyboard.press('k');
                bleKeyboard.press('0');
                delay(100);
                bleKeyboard.releaseAll();
            }
        }
        break;
        case '3': //        Shift: Uncomment Selection | else: Comment Selecttion
        {
            if (Shift == 1)
            {
                bleKeyboard.press(KEY_LEFT_CTRL);
                bleKeyboard.press('k');
                bleKeyboard.press('u');
                delay(100);
                bleKeyboard.releaseAll();
            }
            else
            {
                bleKeyboard.press(KEY_LEFT_CTRL);
                bleKeyboard.press('k');
                bleKeyboard.press('c');
                delay(100);
                bleKeyboard.releaseAll();
            }
        }
        break;
        case 'A': //        Shift: Bulb-02 Toggle | else: Bulb-01 Toggle
        {
            if (Shift == 1)
            {
                serverPath = "http://192.168.1.152/control?cmd=event,ToggleMCP";
                httpReq(serverPath);
            }
            else
            {
                serverPath = "http://192.168.1.151/control?cmd=event,ToggleMCP";
                httpReq(serverPath);
            }
        }
        break;
        case '4': //        Shift: Open | else: Kodi Stop
        {
            if (Shift == 1)
            {
            }
            else
            {
                mqttclient.publish("/Kodi/CMD", "Stop");
            }
        }
        break;
        case '5': //        Shift: Kodi Zoom | else: Kodi Play/Pause
        {
            if (Shift == 1)
            {
                mqttclient.publish("/Kodi/CMD", "Z");
            }
            else
            {
                mqttclient.publish("/Kodi/CMD", "Pause");
            }
        }
        break;
        case '6': //        Shift: Leaving | else: Home
        {
            if (Shift == 1)
            {
                mqttclient.publish("/routines", "0");
            }
            else
            {
                mqttclient.publish("/routines", "1");
            }
        }
        break;
        case 'B': //        Shift: Over H 10% | else: Over H 1/0
        {
            if (Shift == 1)
            {
                mqttclient.publish("/MatrixR/CMD", "13");
            }
            else
            {
                mqttclient.publish("/MatrixR/CMD", "12");
            }
        }
        break;
        case '7': //        Shift: Open | else: Kodi_BACKSPACE
        {
            if (Shift == 1)
            {
            }
            else
            {
                mqttclient.publish("/Kodi/CMD", "Back");
            }
        }
        break;
        case '8': //        Shift: Seek | else: Kodi_UP_ARROW
        {
            if (Shift == 1)
            {
                mqttclient.publish("/Kodi/CMD", "bFwd");
            }
            else
            {
                mqttclient.publish("/Kodi/CMD", "Up");
            }
        }
        break;
        case '9': //        Shift: Kodi Context Menu | else: Kodi Select
        {
            if (Shift == 1)
            {
                mqttclient.publish("/Kodi/CMD", "context");
            }
            else
            {
                mqttclient.publish("/Kodi/CMD", "Select");
            }
        }
        break;
        case 'C': //        Shift: Outlet-03 Toggle | else: Outlet-08 Toggle
        {
            if (Shift == 1)
            {
                serverPath = "http://192.168.1.143/control?cmd=event,Toggle";
                httpReq(serverPath);
            }
            else
            {
                serverPath = "http://192.168.1.148/control?cmd=event,Toggle";
                httpReq(serverPath);
            }
        }
        break;
        case '*': //        Shift: Seek | else: Kodi_LEFT_ARROW
        {
            if (Shift == 1)
            {
                mqttclient.publish("/Kodi/CMD", "Rew");
            }
            else
            {
                mqttclient.publish("/Kodi/CMD", "Left");
            }
        }
        break;
        case '0': //        Shift: Seek | else: Kodi_DOWN_ARROW
        {
            if (Shift == 1)
            {
                mqttclient.publish("/Kodi/CMD", "bRew");
            }
            else
            {
                mqttclient.publish("/Kodi/CMD", "Down");
            }
        }
        break;
        case '#': //        Shift: Seek | else: Kodi_RIGHT_ARROW
        {
            if (Shift == 1)
            {
                mqttclient.publish("/Kodi/CMD", "Fwd");
            }
            else
            {
                mqttclient.publish("/Kodi/CMD", "Right");
            }
        }
        break;
        case 'D': //        Shift: Open | else: Outlet-05 Toggle
        {
            switch (Shift)
            {
            case 0:
            {
                Serial.println("D Shit 0");
                break;
            }
            case 1:
            {
                Serial.println("D Shit 1");
                break;
            }
            case 2:
            {
                Serial.println("D Shit 2");
                break;
            }
            case 3:
            {
                Serial.println("D Shit 3");
                break;
            }
            }
        }
        break;
        }

        Serial.println("-----------------");
        Serial.println("In Mode 0");
        Serial.print("kchar is:");
        Serial.println(Keypad1.key[i].kchar);
        Serial.println("-----------------");
        break;
    }

    case 1: //  Single KEY Mode
    {
        char Key1 = Keypad1.key[i].kchar;
        fullKey = Key1;
        fullKey.toCharArray(KeyMQTT, fullKey.length() + 1);
        if (Key1 != NULL)
        {
            mqttclient.publish("/MatrixR/KEY", KeyMQTT);
            Serial.print("Sent key:\t");
            Serial.println(Key1);

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
    break;

    case 2: // Keycode
    {
        char Key1 = Keypad1.key[i].kchar;
        if (Key1 == sendKey)
        {
            fullKey.toCharArray(KeyMQTT, fullKey.length() + 1);
            mqttclient.publish("MatrixR/Code", KeyMQTT);
            Serial.print("Code key:\t");
            Serial.println(fullKey);
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
            Serial.print("Current keys:\t");
            Serial.println(fullKey);
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
    break;

    case 3: // PC Keypad
    {

        char Key1 = Keypad1.key[i].kchar;
        if (Key1 != NULL)
            if (Key1 != 'E' && Key1 != 'F' && Key1 != 'G' && Key1 != 'H' && Key1 != 'I')
            {
                bleKeyboard.press(Key1);
                delay(100);
                bleKeyboard.releaseAll();
                display.clearDisplay();
                display.setCursor(0, 0);
                display.setTextSize(2);
                display.setTextColor(SSD1306_WHITE);
                display.print("Pressed: ");
                display.println(Key1);
                display.display();
                taskDispOff.setNext(dispSleepT);
            }
    }
    break;
    }
}

void UpdateOled(int Mode)
{
    Serial.println("UpdateOled");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);

    switch (Mode)
    {
    case 0:
        display.println("   Macro  ");
        display.println("");
        break;

    case 1:
        display.println("Single Key");
        display.println("");
        display.println("");
        display.println("");
        break;

    case 2:
        display.println("Key Code");
        display.println("");
        display.println("");
        display.println("");
        break;

    case 3:
        display.println("PC KeyPad");
        display.println("");
        display.println("");
        display.println("");
        break;
    }

    display.display();
    taskDispOff.setNext(dispSleepT);
}

void DispOff()
{
    Serial.println("DispOff");
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
        for (int i = 0; i < LIST_MAX; i++) // Scan the whole key list.
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
                        Mode = ++Mode;
                        if (Mode > ModeMax)
                        {
                            Mode = 0;
                        }
                        UpdateOled(Mode);
                        break;
                    }
                    }
                    break;
                }
                    // case HOLD:
                    //   msg = " HOLD. ";
                    //   break;

                case RELEASED:
                    msg = " RELEASED. ";
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
                    mode();
                    break;

                    // case IDLE:
                    //   msg = " IDLE.";
                    //   break;
                }
                Serial.println("-----------------");
                Serial.print("Key: ");
                Serial.print(Keypad1.key[i].kchar);
                Serial.println(msg);
                Serial.print("Shift: ");
                Serial.println(Shift);
                Serial.print("Mode: ");
                Serial.println(Mode);
                Serial.print("Keycode: ");
                Serial.println(Keypad1.key[i].kcode);
                Serial.println("-----------------");
            }
        }
    }

if (rotaryEncoder.encoderChanged())
{
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
// xxx