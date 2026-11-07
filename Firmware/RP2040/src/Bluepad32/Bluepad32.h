#pragma once

#include <cstdint>
#include <array>

#include "Gamepad/Gamepad.h"
#include "Board/Config.h"

/*  NOTE: Everything bluepad32/uni needs to be wrapped
    and kept away from tinyusb due to naming conflicts */

namespace bluepad32 {
#ifdef CONFIG_OGXM_STANDALONE_UI
    // The screen asks for pairing; the Bluetooth processor core actually does it.
    void request_pairing();
    bool controller_connected();
#endif
    void run_task(Gamepad(&gamepads)[MAX_GAMEPADS]);
} 