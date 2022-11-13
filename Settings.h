#define I2C_SDA 21
#define I2C_SCL 22
#define ROTARY_ENCODER_A_PIN 34      // CLK
#define ROTARY_ENCODER_B_PIN 35      // DT
#define ROTARY_ENCODER_BUTTON_PIN 32 // SW
#define ROTARY_ENCODER_STEPS 4

#define SCREEN_WIDTH 128    // OLED display width, in pixels
#define SCREEN_HEIGHT 64    // OLED display height, in pixels
#define OLED_RESET -1       // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

char KeyMQTT[50];
const char sendKey = '*'; // Keypress to initialize send to MQTT

