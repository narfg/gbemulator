#include "/tmp/rom_header.h"

#include "src/cpu.h"
#include "src/dummydisplay.h"
// #include "src/oleddisplay.h"
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
  // OledDisplay oled_display;
  DummyDisplay oled_display;
  /* oled_display.init(16, 16);
  oled_display.drawPixel(0, 0, 1);
  oled_display.update();
  sleep(5000); */

  // RomLoader rl("blah");
  uint8_t* ram = new uint8_t[65536];
  Serial.print("Before cpu: ");
  Serial.println(ESP.getFreeHeap());
  CPU cpu(ram, &oled_display);
  Serial.print("Before setROM: ");
  Serial.println(ESP.getFreeHeap());
  cpu.setROM(header_rom);
  cpu.run();
  

    // XOR A to get defined flags
    /*cpu.executeInstruction(0xAF, false, 0x42, 0x42);
    REQUIRE(cpu.getA() == 0);
    REQUIRE(cpu.getFlagZ() == 1);
    REQUIRE(cpu.getFlagN() == 0);
    REQUIRE(cpu.getFlagH() == 0);
    REQUIRE(cpu.getFlagC() == 0);

    // Set A to 0x30
    cpu.executeInstruction(0x3E, false, 0x42, 0x30);
    REQUIRE(cpu.getA() == 0x30);
    REQUIRE(cpu.getFlagZ() == 1);
    REQUIRE(cpu.getFlagN() == 0);
    REQUIRE(cpu.getFlagH() == 0);
    REQUIRE(cpu.getFlagC() == 0);

    // OR A
    cpu.executeInstruction(0xB7, false, 0x42, 0x42);
    REQUIRE(cpu.getA() == 0x30);
    REQUIRE(cpu.getFlagZ() == 0);
    REQUIRE(cpu.getFlagN() == 0);
    REQUIRE(cpu.getFlagH() == 0);
    REQUIRE(cpu.getFlagC() == 0);*/
}

void loop() {
  // put your main code here, to run repeatedly:
  // REQUIRE(true);
  // REQUIRE(false);
}
