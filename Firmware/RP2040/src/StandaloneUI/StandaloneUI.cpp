#include "StandaloneUI/StandaloneUI.h"
#include "StandaloneUI/MenuLogic.h"
#include "StandaloneUI/OledSH1107.h"
#include "Bluepad32/Bluepad32.h"
#include "Board/board_api.h"
#include "UserSettings/UserSettings.h"
#include "UserSettings/NVSTool.h"
#include <hardware/gpio.h>

namespace standalone_ui {
namespace {
constexpr unsigned NEXT_PIN=15, SELECT_PIN=17;
constexpr uint32_t DEBOUNCE_MS=35;
constexpr const char* LAST_OUTPUT_KEY="oled_last";
MenuLogic menu;
OledSH1107 oled;
bool started_in_webapp=false;
bool redraw=true;
bool last_connected=false;
uint32_t last_frame=0, message_expiry=0;
const char* message_text=nullptr;

// Physical buttons are noisy for a few milliseconds when pushed.
// Debouncing keeps a single push from being mistaken for multiple pushes.
struct Button {
    unsigned pin;
    bool physical_down=false, stable_down=false;
    uint32_t changed_at=0;
    void begin() {
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_IN);
        gpio_pull_up(pin); // Pressed button pulls the pin down to 0 volts.
    }
    bool was_pressed(uint32_t now) {
        bool down=!gpio_get(pin);
        if (down!=physical_down) { physical_down=down; changed_at=now; }
        if (down!=stable_down && now-changed_at>=DEBOUNCE_MS) {
            stable_down=down;
            return down; // Ignore release; only trigger on press.
        }
        return false;
    }
};
Button next{NEXT_PIN}, select{SELECT_PIN};

void notice(const char* text) {
    message_text=text;
    message_expiry=board_api::ms_since_boot()+1800;
    redraw=true;
}

void draw(bool connected) {
    if(!oled.present())return;
    oled.clear();
    oled.text(0,0,"OGX MINI");
    oled.line(10);
    oled.text(0,15,connected?"BT:ON":"BT:WAIT");
    oled.text(0,27,"OUTPUT");
    oled.text(0,37,menu.output_name());
    oled.line(48);
    if(menu.page()==Page::Output) {
        // Selecting OUTPUT lets Button A cycle modes; B returns to menu.
        oled.text(0,55,"MODE:");
        oled.text(0,68,menu.output_name());
        oled.text(0,85,"A:NEXT");
        oled.text(0,96,"B:OK");
        oled.text(0,114,"SAVE LATER");
    } else {
        const char* choices[]={"PAIR","OUTPUT","WEB APP","SAVE"};
        for(unsigned i=0;i<4;++i) {
            const int row=53+11*static_cast<int>(i);
            oled.text(0,row,menu.choice()==i?">":" ");
            oled.text(7,row,choices[i]);
        }
        oled.line(101);
        oled.text(0,111,message_text ? message_text :
                     (menu.changed() ? "*UNSAVED" : "A:N B:OK"));
    }
    last_frame=board_api::ms_since_boot();
    redraw=false;
}
}

void initialize(DeviceDriverType saved_mode) {
    next.begin();
    select.begin();
    started_in_webapp = saved_mode==DeviceDriverType::WEBAPP;
    uint8_t last=static_cast<uint8_t>(saved_mode);
    if(started_in_webapp) {
        // WebApp is a temporary output. Restore the last real console choice
        // so the physical Save button can exit USB configuration mode.
        last=static_cast<uint8_t>(DeviceDriverType::XBOXOG);
        uint8_t value=last;
        if(NVSTool::get_instance().read(LAST_OUTPUT_KEY,&value,1) &&
           UserSettings::get_instance().is_valid_driver(
                static_cast<DeviceDriverType>(value)) &&
           value!=static_cast<uint8_t>(DeviceDriverType::WEBAPP)) {
            last=value;
        }
    }
    menu.begin(last);
    oled.begin();
    draw(false);
}

void poll() {
    const uint32_t now=board_api::ms_since_boot();
    if(next.was_pressed(now)) {
        menu.next();
        message_text=nullptr;
        redraw=true;
    }
    if(select.was_pressed(now)) {
        redraw=true;
        switch(menu.select()) {
            case Action::Pair:
                // Send a request, rather than touching Bluetooth on this core.
                bluepad32::request_pairing();
                notice("PAIR NOW");
                break;
            case Action::WebApp:
                // OGX-Mini saves and reboots to its built-in USB config mode.
                // Remember the console type in the same safe flash transaction.
                UserSettings::get_instance().enter_webapp_remembering(
                    static_cast<DeviceDriverType>(menu.output()));
                break;
            case Action::Save:
                // Saving changes the USB descriptor, so OGX-Mini must reboot.
                // In WebApp mode, Save is also the way back to console mode.
                if(menu.changed() || started_in_webapp)
                    UserSettings::get_instance().store_driver_type(
                        static_cast<DeviceDriverType>(menu.output()));
                else notice("SAVED");
                break;
            case Action::None:
                // Output selection confirmed; Save is a separate menu action.
                break;
        }
    }
    bool connected=bluepad32::controller_connected();
    if(connected!=last_connected) {
        last_connected=connected;
        redraw=true;
    }
    if(message_text && static_cast<int32_t>(message_expiry-now)<=0) {
        message_text=nullptr;
        redraw=true;
    }
    if(redraw && now-last_frame>=50) draw(connected);
    oled.send_one_page(); // Updates only one small strip per USB loop.
}
} // namespace standalone_ui
