#pragma once
#include <cstdint>
namespace standalone_ui {
// A very small driver for the 1.3-inch SH1107 OLED.
// The module already plugs into Pico WH headers: no wires are necessary.
class OledSH1107 {
public:
    bool begin();             // Find and start the display.
    bool present() const { return present_; }
    void clear();             // Start drawing a fresh screen.
    void text(int x,int y,const char* letters);
    void line(int y);
    void send_one_page();     // Updates 8 rows so gamepad USB stays responsive.
private:
    void pixel(int x,int y);
    void command(uint8_t value);
    void data(const uint8_t* bytes,unsigned length);
    bool present_=false, i2c_=false;
    uint8_t address_=0x3c;
    uint8_t pixels_[64*128/8]{};
    uint16_t dirty_pages_=0xffff;
};
}
