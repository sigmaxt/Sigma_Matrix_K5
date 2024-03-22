#include "Settings.h"
#include "Secret.h"
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <PubSubClient.h> // https://github.com/knolleary/pubsubclient
#include <Keypad.h>       // https://github.com/Chris--A/Keypad
#include <BleKeyboard.h>  // https://github.com/T-vK/ESP32-BLE-Keyboard
// #include <BLEMidi.h>          // https://github.com/max22-/ESP32-BLE-MIDI
#include <Adafruit_SSD1306.h> // https://github.com/adafruit/Adafruit_SSD1306 https://github.com/adafruit/Adafruit-GFX-Library
#include <SchedTask.h>        // https://github.com/Nospampls/SchedTask
// #include "SPIFFS.h"
// #include <ezButton.h>      // https://github.com/ArduinoGetStarted/button

int i;
// const byte ledPin = 2;
int Lock = 0;
int Shift = 0;
int dispSleepT = 5000;
int keyReltime = 50;
bool MQTT = true;
int set = 0;
int setmax = 2;
int Mode = 0;
int ModeMax = 5;
String Mode_Lables[6] = {"Macro", "Single Key", "key Code", "PC Keypad", "Service", "ListNav"};
String Shifted;
String Unshifted;
unsigned long loopCount;
unsigned long startTime;
char KeyPressed;
int Kcode;
const byte ROWS = 11;
const byte COLS = 6;

char keys[ROWS][COLS] = {
    {'1', '2', '3', 'A', 'M', 'p'},
    {'4', '5', '6', 'B', 'L', 'q'},
    {'7', '8', '9', 'C', 'K', 'r'},
    {'-', '0', '=', 'D', 'J', 's'},
    {'E', 'F', 'G', 'H', 'I', 't'},
    {'N', 'O', 'P', 'Q', 'R', 'u'},
    {'S', 'T', 'U', 'V', 'W', 'v'},
    {'X', 'Y', 'Z', '!', '@', 'w'},
    {'a', 'b', 'c', 'd', 'e', 'x'},
    {'f', 'g', 'h', 'i', 'j', 'y'},
    {'k', 'l', 'm', 'n', 'o', 'z'}};

String fullKey;
byte rowPins[ROWS] = {33, 25, 26, 27, 13, 23, 14, 12, 32, 5, 15}; // connect to the row pinouts of the Keypad1
byte colPins[COLS] = {19, 18, 17, 16, 4, 0};                      // connect to the column pinouts of the Keypad1

void DispOff();
SchedTask taskDispOff(NEVER, ONESHOT, DispOff);

static uint8_t prevNextCode = 0;
static uint16_t store = 0;

Keypad Keypad1 = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
BleKeyboard bleKeyboard("Sigma_Matrix_K5", "Sigma", 100);
WiFiClient espClientK5;
PubSubClient mqttclient(espClientK5);

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
        if (MQTT)
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
                Serial.println(messageTemp);
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

void CMD_List(int CMD)
{
        switch (CMD)
        {
        case 100: // Upload
        {
                bleKeyboard.press(KEY_LEFT_CTRL);
                bleKeyboard.press(KEY_LEFT_ALT);
                bleKeyboard.press('u');
                delay(keyReltime);
                bleKeyboard.releaseAll();
                break;
        }
        case 101: // Compile
        {
                bleKeyboard.press(KEY_LEFT_CTRL);
                bleKeyboard.press(KEY_LEFT_ALT);
                bleKeyboard.press('r');
                delay(keyReltime);
                bleKeyboard.releaseAll();
                break;
        }
        case 102: // Comment
        {
                bleKeyboard.press(KEY_LEFT_CTRL);
                bleKeyboard.press('k');
                bleKeyboard.press('c');
                delay(keyReltime);
                bleKeyboard.releaseAll();
                break;
        }
        case 103: // Uncomment
        {
                bleKeyboard.press(KEY_LEFT_CTRL);
                bleKeyboard.press('k');
                bleKeyboard.press('u');
                delay(keyReltime);
                bleKeyboard.releaseAll();
                break;
        }
        case 104: // Collapse
        {
                bleKeyboard.press(KEY_LEFT_CTRL);
                bleKeyboard.press('k');
                bleKeyboard.press('0');
                delay(keyReltime);
                bleKeyboard.releaseAll();
                break;
        }
        case 105: // Expand
        {
                bleKeyboard.press(KEY_LEFT_CTRL);
                bleKeyboard.press('k');
                bleKeyboard.press('j');
                delay(keyReltime);
                bleKeyboard.releaseAll();
                break;
        }
        case 106: // RootFolder
        {
                bleKeyboard.press(KEY_LEFT_CTRL);
                bleKeyboard.press(KEY_LEFT_GUI);
                delay(keyReltime);
                bleKeyboard.releaseAll();
                break;
        }
        case 107: // Bulb-01
        {
                httpReq("http://192.168.1.151/control?cmd=event,ToggleMCP");
                break;
        }
        case 108: // Bulb-02
        {
                httpReq("http://192.168.1.152/control?cmd=event,ToggleMCP");
                break;
        }
        case 109: // Bulb-03
        {
                httpReq("http://192.168.1.153/control?cmd=event,ToggleMCP");
                break;
        }
        case 110: // Open
        {

                break;
        }
        case 111: // Outlet-01
        {
                httpReq("http://192.168.1.141/control?cmd=event,Toggle");
                break;
        }
        case 112: // Outlet-02
        {
                httpReq("http://192.168.1.142/control?cmd=event,Toggle");
                break;
        }
        case 113: // Outlet-03
        {
                httpReq("http://192.168.1.143/control?cmd=event,Toggle");
                break;
        }
        case 114: // Outlet-04
        {
                httpReq("http://192.168.1.144/control?cmd=event,Toggle");
                break;
        }
        case 115: // Outlet-05
        {
                httpReq("http://192.168.1.145/control?cmd=event,Toggle");
                break;
        }
        case 116: // Outlet-06
        {
                httpReq("http://192.168.1.146/control?cmd=event,Toggle");
                break;
        }
        case 117: // Outlet-07
        {
                httpReq("http://192.168.1.147/control?cmd=event,Toggle");
                break;
        }
        case 118: // Outlet-08
        {
                httpReq("http://192.168.1.148/control?cmd=event,Toggle");
                break;
        }
        case 119: // Outlet-09
        {
                httpReq("http://192.168.1.149/control?cmd=event,Toggle");
                break;
        }
        case 120: // Outlet-10
        {
                httpReq("http://192.168.1.140/control?cmd=event,Toggle");
                break;
        }
        case 121: // Bitwarden
        {
                bleKeyboard.press(KEY_LEFT_CTRL);
                bleKeyboard.press(KEY_LEFT_SHIFT);
                bleKeyboard.press('l');
                delay(keyReltime);
                bleKeyboard.releaseAll();
                break;
        }
        case 122: // Open
        {
                break;
        }
        case 123: // CMD_Home
        {
                mqttclient.publish("/routines", "1");
                break;
        }
        case 124: // CMD_Leave
        {
                mqttclient.publish("/routines", "0");
                break;
        }
        case 125: // Kodi Stop
        {
                mqttclient.publish("/Kodi/CMD", "Stop");
                break;
        }
        case 126: // Kodi Pause
        {
                mqttclient.publish("/Kodi/CMD", "Pause");
                break;
        }
        case 127: // Kodi Zoom
        {
                mqttclient.publish("/Kodi/CMD", "Z");
                break;
        }
        case 128: // OverH1/0
        {
                mqttclient.publish("/MatrixR/CMD", "12");
                break;
        }
        case 129: // OverH10%
        {
                mqttclient.publish("/MatrixR/CMD", "13");
                break;
        }
        case 130: // Copy
        {
                bleKeyboard.press(KEY_LEFT_CTRL);
                bleKeyboard.press('c');
                delay(keyReltime);
                bleKeyboard.releaseAll();
                break;
        }
        case 131: // Paste
        {
                bleKeyboard.press(KEY_LEFT_CTRL);
                bleKeyboard.press('v');
                delay(keyReltime);
                bleKeyboard.releaseAll();
                break;
        }
        case 132: // Cut
        {
                bleKeyboard.press(KEY_LEFT_CTRL);
                bleKeyboard.press('x');
                delay(keyReltime);
                bleKeyboard.releaseAll();
                break;
        }
        case 133: // Kodi Back
        {
                mqttclient.publish("/Kodi/CMD", "Back");
                break;
        }
        case 134: // Mon Off
        {
                httpReq("http://192.168.1.122:8655/monitor?mode=off");
                break;
        }
        case 135: // Kodi Up
        {
                mqttclient.publish("/Kodi/CMD", "Up");
                break;
        }
        case 136: // Kodi+30
        {
                mqttclient.publish("/Kodi/CMD", "bFwd");
                break;
        }
        case 137: // Hibernate
        {
                httpReq("http://192.168.1.122:8655/pc?mode=hibernate");
                break;
        }
        case 138: // Open
        {
                break;
        }
        case 139: // Kodi Select
        {
                mqttclient.publish("/Kodi/CMD", "Select");
                break;
        }
        case 140: // Kodi Context
        {
                mqttclient.publish("/Kodi/CMD", "context");
                break;
        }
        case 141: // ReDo
        {
                bleKeyboard.press(KEY_LEFT_CTRL);
                bleKeyboard.press('y');
                delay(keyReltime);
                bleKeyboard.releaseAll();
                break;
        }
        case 142: // Kodi Left
        {
                mqttclient.publish("/Kodi/CMD", "Left");
                break;
        }
        case 143: // Kodi-10
        {
                mqttclient.publish("/Kodi/CMD", "Rew");
                break;
        }
        case 144: // Kodi Down
        {
                mqttclient.publish("/Kodi/CMD", "Down");
                break;
        }
        case 145: // Kodi-30
        {
                mqttclient.publish("/Kodi/CMD", "bRew");
                break;
        }
        case 146: // Kodi Right
        {
                mqttclient.publish("/Kodi/CMD", "Right");
                break;
        }
        case 147: // Kodi+10
        {
                mqttclient.publish("/Kodi/CMD", "Fwd");
                break;
        }
        case 148: // UnDo
        {
                bleKeyboard.press(KEY_LEFT_CTRL);
                bleKeyboard.press('z');
                delay(keyReltime);
                bleKeyboard.releaseAll();
                break;
        }
        case 149: // Open
        {
                bleKeyboard.write(KEY_NUM_1);
                break;
        }
        case 150: // Open
        {
                break;
        }
        case 151: // Open
        {
                break;
        }
        case 152: // Open
        {
                break;
        }
        case 153: // Open
        {
                break;
        }
        case 154: // Open
        {
                break;
        }
        case 155: // Open
        {
                break;
        }
        case 156: // Open
        {
                break;
        }
        case 157: // Open
        {
                break;
        }
        case 158: // Open
        {
                break;
        }
        case 159: // Open
        {
                break;
        }
        case 160: // Open
        {
                break;
        }
        case 161: // Open
        {
                break;
        }
        case 162: // Open
        {
                break;
        }
        case 163: // Open
        {
                break;
        }
        case 164: // KEY_NUM_1
        {
                bleKeyboard.write(KEY_NUM_1);
                break;
        }
        case 165: // KEY_NUM_2
        {
                bleKeyboard.write(KEY_NUM_2);
                break;
        }
        case 166: // KEY_NUM_3
        {
                bleKeyboard.write(KEY_NUM_3);
                break;
        }
        case 167: // KEY_NUM_4
        {
                bleKeyboard.write(KEY_NUM_4);
                break;
        }
        case 168: // KEY_NUM_5
        {
                bleKeyboard.write(KEY_NUM_5);
                break;
        }
        case 169: // KEY_NUM_6
        {
                bleKeyboard.write(KEY_NUM_6);
                break;
        }
        case 170: // KEY_NUM_7
        {
                bleKeyboard.write(KEY_NUM_7);
                break;
        }
        case 171: // KEY_NUM_8
        {
                bleKeyboard.write(KEY_NUM_8);
                break;
        }
        case 172: // KEY_NUM_9
        {
                bleKeyboard.write(KEY_NUM_9);
                break;
        }
        case 173: // KEY_NUM_0
        {
                bleKeyboard.write(KEY_NUM_0);
                break;
        }
        case 174: // KEY_NUM_ASTERISK
        {
                bleKeyboard.write(KEY_NUM_ASTERISK);
                break;
        }
        case 175: // KEY_NUM_MINUS
        {
                bleKeyboard.write(KEY_NUM_MINUS);
                break;
        }
        case 176: // KEY_NUM_PLUS
        {
                bleKeyboard.write(KEY_NUM_PLUS);
                break;
        }
        case 177: // KEY_NUM_PERIOD
        {
                bleKeyboard.write(KEY_NUM_PERIOD);
                break;
        }
        case 178: // KEY_NUM_SLASH
        {
                bleKeyboard.write(KEY_NUM_SLASH);
                break;
        }
        case 179: // KEY_NUM_ENTER
        {
                bleKeyboard.write(KEY_NUM_ENTER);
                break;
        }
        case 180: // Open
        {
                break;
        }
        case 181: // KEY_DELETE
        {
                bleKeyboard.write(KEY_DELETE);
                break;
        }
        case 182: // KEY_TAB
        {
                bleKeyboard.write(KEY_TAB);
                break;
        }
        case 183: // KEY_S_TAB
        {
                bleKeyboard.press(KEY_LEFT_SHIFT);
                bleKeyboard.press(KEY_TAB);
                delay(keyReltime);
                bleKeyboard.releaseAll();
                break;
        }
        case 184: // KEY_ESC
        {
                bleKeyboard.write(KEY_ESC);
                break;
        }
        case 185: // KEY_INSERT
        {
                bleKeyboard.write(KEY_INSERT);
                break;
        }
        case 186: // KEY_HOME
        {
                bleKeyboard.write(KEY_HOME);
                break;
        }
        case 187: // KEY_BACKSPACE
        {
                bleKeyboard.write(KEY_BACKSPACE);
                break;
        }
        case 188: // KEY_UP_ARROW
        {
                bleKeyboard.write(KEY_UP_ARROW);
                break;
        }
        case 189: // KEY_RETURN
        {
                bleKeyboard.write(KEY_RETURN);
                break;
        }
        case 190: // KEY_PAGE_UP
        {
                bleKeyboard.write(KEY_PAGE_UP);
                break;
        }
        case 191: // KEY_END
        {
                bleKeyboard.write(KEY_END);
                break;
        }
        case 192: // KEY_LEFT_ARROW
        {
                bleKeyboard.write(KEY_LEFT_ARROW);
                break;
        }
        case 193: // KEY_DOWN_ARROW
        {
                bleKeyboard.write(KEY_DOWN_ARROW);
                break;
        }
        case 194: // KEY_RIGHT_ARROW
        {
                bleKeyboard.write(KEY_RIGHT_ARROW);
                break;
        }
        case 195: // KEY_PAGE_DOWN
        {
                bleKeyboard.write(KEY_PAGE_DOWN);
                break;
        }
        case 196: // KEY_F13
        {
                bleKeyboard.write(KEY_F13);
                break;
        }
        case 197: // KEY_F14
        {
                bleKeyboard.write(KEY_F14);
                break;
        }
        case 198: // KEY_F15
        {
                bleKeyboard.write(KEY_F15);
                break;
        }
        case 199: // KEY_F16
        {
                bleKeyboard.write(KEY_F16);
                break;
        }
        case 200: // KEY_F17
        {
                bleKeyboard.write(KEY_F17);
                break;
        }
        case 201: // KEY_F18
        {
                bleKeyboard.write(KEY_F18);
                break;
        }
        case 202: // KEY_F19
        {
                bleKeyboard.write(KEY_F19);
                break;
        }
        case 203: // KEY_F20
        {
                bleKeyboard.write(KEY_F20);
                break;
        }
        case 204: // KEY_F21
        {
                bleKeyboard.write(KEY_F21);
                break;
        }
        case 205: // KEY_F22
        {
                bleKeyboard.write(KEY_F22);
                break;
        }
        case 206: // KEY_F23
        {
                bleKeyboard.write(KEY_F23);
                break;
        }
        case 207: // KEY_F24
        {
                bleKeyboard.write(KEY_F24);
                break;
        }
        case 208: // inch
        {
                bleKeyboard.print(" inch ");
                break;
        }
        case 209: // mm
        {
                bleKeyboard.print(" mm ");
                break;
        }
        break;
        case 210: // Shift 1
        {
                Shift = 1;
                // digitalWrite(ledPin, HIGH);
                UpdateOled(Mode);
                break;
        }
        case 211: // Shift 2
        {
                Shift = 2;
                // digitalWrite(ledPin, HIGH);
                UpdateOled(Mode);
                break;
        }
        case 212: // Shift 3
        {
                Shift = 3;
                // digitalWrite(ledPin, HIGH);
                UpdateOled(Mode);
                break;
        }
        case 213: // Keylock
        {
                if (Shift != 4)
                        Shift = 4;
                else
                        Shift = 0;
                // digitalWrite(ledPin, HIGH);
                UpdateOled(Mode);
                break;
        }
        case 214: // Shift  Lock
        {
                Lock = (!Lock);
                if (!Lock)
                {
                        Shift = 0;
                        Mode = 0;
                        // digitalWrite(ledPin, LOW);
                }
                UpdateOled(Mode);
                break;
        }
        case 215: // Mode/Set
        {
                set = ++set;
                if (set > setmax)
                {
                        set = 0;
                }
                UpdateOled(Mode);
                break;
        }
        case 216: // Space
        {
                bleKeyboard.print(" ");
                break;
        }
        case 217: // Select All
        {
                bleKeyboard.press(KEY_LEFT_CTRL);
                bleKeyboard.press('a');
                delay(keyReltime);
                bleKeyboard.releaseAll();
                break;
        }
        case 218: // Single Monitor
        {
                bleKeyboard.press(KEY_LEFT_GUI);
                bleKeyboard.press('p');
                delay(keyReltime);
                bleKeyboard.releaseAll();
                bleKeyboard.write(KEY_HOME);
                bleKeyboard.write(KEY_NUM_ENTER);
                bleKeyboard.write(KEY_ESC);
                break;
        }
        case 219: // Second Monitor
        {
                bleKeyboard.press(KEY_LEFT_GUI);
                bleKeyboard.press('p');
                delay(keyReltime);
                bleKeyboard.releaseAll();
                bleKeyboard.write(KEY_END);
                bleKeyboard.write(KEY_NUM_ENTER);
                bleKeyboard.write(KEY_ESC);
                break;
        }
        case 220: // Extend Monitor
        {
                bleKeyboard.press(KEY_LEFT_GUI);
                bleKeyboard.press('p');
                delay(keyReltime);
                bleKeyboard.releaseAll();
                bleKeyboard.write(KEY_HOME);
                bleKeyboard.write(KEY_DOWN_ARROW);
                bleKeyboard.write(KEY_DOWN_ARROW);
                bleKeyboard.write(KEY_NUM_ENTER);
                bleKeyboard.write(KEY_ESC);
                break;
        }
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

void ListNav()
{
}

void UpdateOled(int Mode)
{
        display.clearDisplay();
        display.setCursor(0, 0);
        display.setTextSize(2);
        display.setTextColor(SSD1306_WHITE);
        display.println(Mode_Lables[Mode]);
        display.print("CMD: ");
        display.println(SET[set][Kcode][Shift]);
        display.print("LK:");
        display.print(Lock);
        display.print(" ST:");
        display.println(set);
        display.print("SH:");
        display.print(Shift);
        display.print(" KC:");
        display.println(Kcode);
        display.display();
        taskDispOff.setNext(dispSleepT);
}

void DispOff()
{
        display.clearDisplay();
        display.display();
}

int8_t read_rotary()
{
        static int8_t rot_enc_table[] = {0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0};

        prevNextCode <<= 2;
        if (digitalRead(ROTARY_DATA))
                prevNextCode |= 0x02;
        if (digitalRead(ROTARY_CLK))
                prevNextCode |= 0x01;
        prevNextCode &= 0x0f;

        // If valid then store as 16 bit data.
        if (rot_enc_table[prevNextCode])
        {
                store <<= 4;
                store |= prevNextCode;
                if (store == 0xd42b)
                        return 1;
                if (store == 0xe817)
                        return -1;
                // if ((store & 0xff) == 0x2b)
                //         return -1;
                // if ((store & 0xff) == 0x17)
                //         return 1;
        }
        return 0;
}

void setup()
{
        Serial.begin(115200);
        loopCount = 0;
        startTime = millis();

        delay(10);
        Serial.println('\n');
        bleKeyboard.begin();
        wificn();
        mqttcn();

        pinMode(ROTARY_CLK, INPUT);
        pinMode(ROTARY_DATA, INPUT);
        pinMode(ROTARY_SW, INPUT);

        // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
        if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
        {
                Serial.println(F("SSD1306 allocation failed"));
                for (;;)
                        ; // Don't proceed, loop forever
        }

        // Show initial display buffer contents on the screen --
        // the library initializes this with an Adafruit splash screen.

        display.setRotation(0);
        display.display();
        UpdateOled(Mode);

        taskDispOff.setNext(dispSleepT);
        // pinMode(2, OUTPUT);
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
                Serial.println(Keypad1.key[i].kchar);
                for (int i = 0; i < LIST_MAX; i++) // Scan the whole key list.
                {
                        if (Keypad1.key[i].stateChanged) // Only find keys that have changed state.
                        {
                                switch (Keypad1.key[i].kstate) // Report active key state : IDLE, PRESSED, HOLD, or RELEASED
                                {
                                case PRESSED:
                                {
                                        switch (Keypad1.key[i].kchar)
                                        {
                                        default:
                                        {
                                                KeyPressed = Keypad1.key[i].kchar;
                                                Kcode = Keypad1.key[i].kcode;
                                                switch (Mode)
                                                {
                                                case 0: // Main
                                                {
                                                        CMD_List(SET[set][Kcode][Shift]);
                                                        UpdateOled(Mode);
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
                                                        CMD_List(SET[set][Kcode][4]);
                                                        UpdateOled(Mode);
                                                        // pcKeypad();
                                                        break;
                                                }
                                                case 4: // Service
                                                {
                                                        UpdateOled(Mode);
                                                        break;
                                                }
                                                case 5: // ListNav
                                                {
                                                        ListNav();
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
                                        Kcode = Keypad1.key[i].kcode;
                                        switch (Kcode)

                                        {
                                        case 11: // Shift 1
                                        case 17: // Shift 2
                                        case 23: // Shift 3
                                        {
                                                if (Lock)
                                                {
                                                        UpdateOled(Mode);
                                                        break;
                                                }
                                                else
                                                        Shift = 0;
                                                // digitalWrite(ledPin, LOW);
                                                UpdateOled(Mode);
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

        static int8_t c, val;

        if (val = read_rotary())
        {
                c += val;
                if (prevNextCode == 0x0b)
                {

                        switch (digitalRead(ROTARY_SW))
                        {
                        case 1:
                        {
                                switch (Shift)
                                {
                                case 0:
                                {
                                        Mode = --Mode;
                                        if (Mode < 0)
                                        {
                                                Mode = ModeMax;
                                        }
                                        UpdateOled(Mode);
                                        break;
                                }
                                case 1:
                                {
                                        Mode = --Mode;
                                        if (Mode < 0)
                                        {
                                                Mode = ModeMax;
                                        }
                                        UpdateOled(Mode);
                                        break;
                                }
                                case 2:
                                {
                                        Mode = --Mode;
                                        if (Mode < 0)
                                        {
                                                Mode = ModeMax;
                                        }
                                        UpdateOled(Mode);
                                        break;
                                }
                                case 3:
                                {
                                        Mode = --Mode;
                                        if (Mode < 0)
                                        {
                                                Mode = ModeMax;
                                        }
                                        UpdateOled(Mode);
                                        break;
                                }
                                }
                                break;
                        }
                        case 0:
                        {
                                switch (Shift)
                                {
                                case 0:
                                {

                                        bleKeyboard.write(KEY_MEDIA_VOLUME_DOWN);
                                        break;
                                }
                                case 1:
                                {
                                        bleKeyboard.write(KEY_MEDIA_VOLUME_UP);
                                        break;
                                }
                                case 2:
                                {
                                        bleKeyboard.write(KEY_MEDIA_VOLUME_DOWN);
                                        break;
                                }
                                case 3:
                                {
                                        bleKeyboard.write(KEY_MEDIA_VOLUME_DOWN);
                                        break;
                                }
                                }

                                break;
                        }
                        }
                }

                if (prevNextCode == 0x07)
                {
                        switch (digitalRead(ROTARY_SW))
                        {
                        case 1:
                        {
                                switch (Shift)
                                {
                                case 0:
                                {
                                        Mode = ++Mode;
                                        if (Mode > ModeMax)
                                        {
                                                Mode = 0;
                                        }
                                        UpdateOled(Mode);
                                        break;
                                }
                                case 1:
                                {
                                        Mode = ++Mode;
                                        if (Mode > ModeMax)
                                        {
                                                Mode = 0;
                                        }
                                        UpdateOled(Mode);
                                        break;
                                }
                                case 2:
                                {
                                        Mode = ++Mode;
                                        if (Mode > ModeMax)
                                        {
                                                Mode = 0;
                                        }
                                        UpdateOled(Mode);
                                        break;
                                }
                                case 3:
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
                        case 0:
                        {
                                switch (Shift)
                                {
                                case 0:
                                {

                                        bleKeyboard.write(KEY_MEDIA_VOLUME_UP);
                                        break;
                                }
                                case 1:
                                {
                                        bleKeyboard.write(KEY_MEDIA_VOLUME_DOWN);
                                        break;
                                }
                                case 2:
                                {
                                        bleKeyboard.write(KEY_MEDIA_VOLUME_UP);
                                        break;
                                }
                                case 3:
                                {
                                        bleKeyboard.write(KEY_MEDIA_VOLUME_UP);
                                        break;
                                }
                                }
                                break;
                        }
                        }
                }
        }

        if (WiFi.status() != WL_CONNECTED)
        {
                wificn();
        }

        if (MQTT)
                if (!mqttclient.loop())
                {
                        mqttcn();
                }

} // End loops
