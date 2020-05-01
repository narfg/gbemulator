#include <memory>

#include "/tmp/rom_header.h"

#include "src/cpu.h"
#include "src/joypad.h"
// #include "src/dummydisplay.h"
// #include "src/oleddisplay.h"
// #include "src/ST7789display.h"
// #include "src/st7789display2.h"
#include "src/odroid_display.h"
#include "src/ppu.h"
#include "src/romloader.h"

void REQUIRE(bool condition) {
  if (!condition) {
    Serial.println("false");
  } else {
    Serial.println("true");
  }
}

void setup() {
  // Serial.begin(115200);

  // auto oled_display = std::make_unique<DummyDisplay>();
  // auto oled_display = std::make_unique<OledDisplay>();
  // auto oled_display = std::make_unique<ST7789Display>();
  Joypad joypad;

  uint8_t* ram = new uint8_t[65536];
  auto oled_display = std::make_unique<OdroidDisplay>(ram, &joypad);
  
  Serial.print("Before cpu: ");
  Serial.println(ESP.getFreeHeap());
  CPU cpu(ram, std::move(oled_display), &joypad);
  Serial.print("Before setROM: ");
  Serial.println(ESP.getFreeHeap());
  cpu.setROM(header_rom);
  cpu.run();
}

void loop() {
  // nothing to do here
}
