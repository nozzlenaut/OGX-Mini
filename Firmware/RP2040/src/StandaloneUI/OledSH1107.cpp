#include "StandaloneUI/OledSH1107.h"
#include <hardware/gpio.h>
#include <hardware/i2c.h>
#include <hardware/spi.h>
#include <pico/stdlib.h>
#include <cstring>
#include <initializer_list>

namespace standalone_ui {
namespace {
// The Waveshare / Seeed Pico-OLED-1.3 standard header pin assignments.
constexpr unsigned SDA=6, SCL=7, DC=8, CS=9, CLK=10, MOSI=11, RESET=12;

// Each letter is five columns of seven tiny black/white pixels.
// A small built-in font means no extra display software is needed.
struct Letter { char ch; uint8_t cols[5]; };
constexpr Letter LETTERS[]={
 {' ',{0,0,0,0,0}}, {'-',{8,8,8,8,8}}, {':',{0,0x36,0,0,0}},
 {'>',{8,0x14,0x22,0x41,0}}, {'*',{0x14,8,0x3e,8,0x14}},
 {'0',{0x3e,0x41,0x41,0x3e,0}}, {'1',{0,0x42,0x7f,0x40,0}},
 {'2',{0x62,0x51,0x49,0x46,0}}, {'3',{0x22,0x49,0x49,0x36,0}},
 {'4',{0x18,0x14,0x12,0x7f,0}}, {'5',{0x27,0x45,0x45,0x39,0}},
 {'6',{0x3e,0x49,0x49,0x32,0}}, {'7',{1,0x71,9,5,3}},
 {'8',{0x36,0x49,0x49,0x36,0}}, {'9',{0x26,0x49,0x49,0x3e,0}},
 {'A',{0x7e,0x11,0x11,0x7e,0}}, {'B',{0x7f,0x49,0x49,0x36,0}},
 {'C',{0x3e,0x41,0x41,0x22,0}}, {'D',{0x7f,0x41,0x41,0x3e,0}},
 {'E',{0x7f,0x49,0x49,0x41,0}}, {'F',{0x7f,9,9,1,0}},
 {'G',{0x3e,0x41,0x49,0x7a,0}}, {'H',{0x7f,8,8,0x7f,0}},
 {'I',{0x41,0x7f,0x41,0,0}}, {'J',{0x20,0x40,0x41,0x3f,0}},
 {'K',{0x7f,8,0x14,0x63,0}}, {'L',{0x7f,0x40,0x40,0x40,0}},
 {'M',{0x7f,2,4,2,0x7f}}, {'N',{0x7f,6,0x18,0x7f,0}},
 {'O',{0x3e,0x41,0x41,0x3e,0}}, {'P',{0x7f,9,9,6,0}},
 {'Q',{0x3e,0x41,0x61,0x7e,0}}, {'R',{0x7f,9,0x19,0x66,0}},
 {'S',{0x26,0x49,0x49,0x32,0}}, {'T',{1,1,0x7f,1,1}},
 {'U',{0x3f,0x40,0x40,0x3f,0}}, {'V',{0x1f,0x20,0x40,0x20,0x1f}},
 {'W',{0x7f,0x20,0x18,0x20,0x7f}}, {'X',{0x63,0x14,8,0x14,0x63}},
 {'Y',{7,8,0x70,8,7}}, {'Z',{0x61,0x51,0x49,0x45,0x43}}
};
const uint8_t* letter(char c) {
    for(const auto& glyph:LETTERS) if(glyph.ch==c) return glyph.cols;
    return LETTERS[0].cols; // Unknown symbols appear as blanks.
}
}

bool OledSH1107::begin() {
    // Reset the screen. Try its normal I2C bus before the optional SPI bus.
    gpio_init(RESET); gpio_set_dir(RESET,GPIO_OUT);
    gpio_put(RESET,1); sleep_ms(2);
    gpio_put(RESET,0); sleep_ms(10);
    gpio_put(RESET,1); sleep_ms(50);
    i2c_init(i2c1,400000);
    gpio_set_function(SDA,GPIO_FUNC_I2C);
    gpio_set_function(SCL,GPIO_FUNC_I2C);
    gpio_pull_up(SDA); gpio_pull_up(SCL);
    const uint8_t ping[]={0x00,0xae};
    for(uint8_t addr : {0x3c,0x3d,0x3e}) {
        if(i2c_write_blocking(i2c1,addr,ping,2,false)==2) {
            address_=addr; present_=true; i2c_=true; break;
        }
    }
    if(!present_) {
        // SPI panels cannot acknowledge a probe. This fallback assumes a
        // connected screen whose physical jumpers have been set to SPI.
        i2c_deinit(i2c1);
        spi_init(spi1,8000000);
        gpio_set_function(CLK,GPIO_FUNC_SPI);
        gpio_set_function(MOSI,GPIO_FUNC_SPI);
        gpio_init(DC); gpio_set_dir(DC,GPIO_OUT);
        gpio_init(CS); gpio_set_dir(CS,GPIO_OUT); gpio_put(CS,1);
        present_=true;
    }
    // SH1107 vendor initialization: 64 wide by 128 tall, normal polarity.
    const uint8_t setup[]={
        0xae,0xa8,0x7f,0xd5,0x50,0xc0,0xd3,0,0xdc,0,
        0x20,0x81,0x80,0xa0,0xa4,0xa6,0xad,0x80,0xd9,0x25,0xdb,0x30
    };
    for(uint8_t cmd:setup) command(cmd);
    clear();
    for(int i=0;i<16;++i) send_one_page();
    command(0xaf); // Turn on the screen after initializing.
    return present_;
}
void OledSH1107::command(uint8_t value) {
    if(i2c_) {
        uint8_t packet[2]={0x00,value}; // 0 means "this is a command".
        i2c_write_blocking(i2c1,address_,packet,2,false);
    } else {
        gpio_put(DC,0); gpio_put(CS,0);
        spi_write_blocking(spi1,&value,1);
        gpio_put(CS,1);
    }
}
void OledSH1107::data(const uint8_t* bytes,unsigned length) {
    if(i2c_) {
        for(unsigned off=0;off<length;off+=16) {
            const unsigned part=length-off>16?16:length-off;
            uint8_t packet[17]={0x40}; // 0x40 means "these are pixels".
            std::memcpy(packet+1,bytes+off,part);
            i2c_write_blocking(i2c1,address_,packet,part+1,false);
        }
    } else {
        gpio_put(DC,1); gpio_put(CS,0);
        spi_write_blocking(spi1,bytes,length);
        gpio_put(CS,1);
    }
}
void OledSH1107::clear() {
    std::memset(pixels_,0,sizeof(pixels_));
    dirty_pages_=0xffff; // Every section now needs refreshing.
}
void OledSH1107::pixel(int x,int y) {
    if(x<0||x>=64||y<0||y>=128)return;
    pixels_[y/8*64+x] |= static_cast<uint8_t>(1u<<(y%8));
    dirty_pages_|=static_cast<uint16_t>(1u<<(y/8));
}
void OledSH1107::text(int x,int y,const char* letters) {
    // Translate each character to a 5x7 pattern and draw its lit pixels.
    while(*letters&&x+5<64) {
        const uint8_t* columns=letter(*letters++);
        for(int xx=0;xx<5;++xx)
            for(int yy=0;yy<7;++yy)
                if((columns[xx]>>yy)&1) pixel(x+xx,y+yy);
        x+=6; // One blank pixel separates letters.
    }
}
void OledSH1107::line(int y) {
    for(int x=0;x<64;++x) pixel(x,y);
}
void OledSH1107::send_one_page() {
    if(!present_||!dirty_pages_)return;
    // Update only one 8-row strip per pass, to avoid interrupting game input.
    for(unsigned page=0;page<16;++page) {
        if(!(dirty_pages_&(1u<<page)))continue;
        dirty_pages_&=static_cast<uint16_t>(~(1u<<page));
        command(static_cast<uint8_t>(0xb0|page));
        command(0); command(0x10);
        data(pixels_+page*64,64);
        break;
    }
}
} // namespace standalone_ui
