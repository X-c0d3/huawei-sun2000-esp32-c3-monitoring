Read data from Huawei-SUN2000 Solar Inverter via Modbus RS486 with ESP32-C3

```
--- TFT_eSPI basic setup --


#define ILI9488_DRIVER


// --------------------------------------------------------------------------
#define TFT_MISO 2  // SDO(MISO) ให้ว่างไว้ (ห้ามต่อสาย)  (แต่จำเป็นต้องมีการประกาศไว้ใน code)
#define TFT_MOSI 7
#define TFT_SCLK 6
#define TFT_CS 5
#define TFT_DC 1
#define TFT_RST 0
#define TFT_BL 4  // ขา LED หรือ BL สามารถต่อเข้ากับขา 3.3V ของ MCU ก็ได้เช่นกัน ถ้าหากไม่ได้มีการ control LED backlight

// ถ้าต้องการใช้ toch screen ต้องต่อขาพวนี้ด้วย
#define TOUCH_CS 3    // T_CS
#define TOUCH_SCK 6   // T_CLK แชร์กับ TFT_SCLK
#define TOUCH_MOSI 7  // T_DIN แชร์กับ TFT_MOSI
#define TOUCH_MISO 2  // T_DO
#define TOUCH_IRQ -1  // T_IRQ ให้ว่างไว้ (ห้ามต่อสาย)

// ห้ามใช้ GPIO8 เพราะ Build-in LED จองแล้ว
// ห้ามใช้ GPIO9 เพราะ Reset button จองแล้ว
// --------------------------------------------------------------------------


#define LOAD_GLCD   // Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
#define LOAD_FONT2  // Font 2. Small 16 pixel high font, needs ~3534 bytes in FLASH, 96 characters
#define LOAD_FONT4  // Font 4. Medium 26 pixel high font, needs ~5848 bytes in FLASH, 96 characters
#define LOAD_FONT6  // Font 6. Large 48 pixel font, needs ~2666 bytes in FLASH, only characters 1234567890:-.apm
#define LOAD_FONT7  // Font 7. 7 segment 48 pixel font, needs ~2438 bytes in FLASH, only characters 1234567890:-.
#define LOAD_FONT8  // Font 8. Large 75 pixel font needs ~3256 bytes in FLASH, only characters 1234567890:-.

#define LOAD_GFXFF  // FreeFonts. Include access to the 48 Adafruit_GFX free fonts FF1 to FF48 and custom fonts

#define SMOOTH_FONT


#define SPI_FREQUENCY 40000000

// Optional reduced SPI frequency for reading TFT
#define SPI_READ_FREQUENCY 20000000

// The XPT2046 requires a lower SPI clock rate of 2.5MHz so we define that here:
#define SPI_TOUCH_FREQUENCY 2500000

```

<div style="display: inline-flex; align-items: center;">
  <!-- Video Thumbnail -->
  <a href="https://www.youtube.com/watch?v=PvaOxBSYhOM" target="_blank" style="display: inline-block;">
    <img src="https://github.com/X-c0d3/huawei-sun2000-esp32-c3-monitoring/blob/main/doc/Cover.jpg" style="width: 100%; display: block;">
  </a>

  <!-- Play Button -->
  <a href="https://www.youtube.com/watch?v=PvaOxBSYhOM" target="_blank" style="display: inline-block;">
    <img src="https://upload.wikimedia.org/wikipedia/commons/b/b8/YouTube_play_button_icon_%282013%E2%80%932017%29.svg" 
         style="width: 50px; height: auto; margin-left: 5px;">
  </a>
</div>

<p float="center">
<img src="https://github.com/X-c0d3/huawei-sun2000-esp32-c3-monitoring/blob/main/doc/IMG_1025.jpg"  width="ุ600">
<img src="https://github.com/X-c0d3/huawei-sun2000-esp32-c3-monitoring/blob/main/doc/IMG_1026.jpg"  width="ุ600">
<img src="https://github.com/X-c0d3/huawei-sun2000-esp32-c3-monitoring/blob/main/doc/IMG_1027.jpg"  width="ุ600">
<img src="https://github.com/X-c0d3/huawei-sun2000-esp32-c3-monitoring/blob/main/doc/IMG_1036.jpg"  width="ุ600">
<img src="https://github.com/X-c0d3/huawei-sun2000-esp32-c3-monitoring/blob/main/doc/IMG_1037.jpg"  width="ุ600">
</p>
