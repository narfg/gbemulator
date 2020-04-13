#include <memory>

#include "/tmp/rom_header.h"

#include "src/cpu.h"
// #include "src/dummydisplay.h"
#include "src/joypad.h"
#include "src/oleddisplay.h"
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
  Serial.begin(115200);
  Serial.print("Before display: ");
  Serial.println(ESP.getFreeHeap());

  // auto oled_display = std::make_unique<DummyDisplay>();
  auto oled_display = std::make_unique<OledDisplay>();
  Joypad joypad;

  uint8_t* ram = new uint8_t[65536];
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
