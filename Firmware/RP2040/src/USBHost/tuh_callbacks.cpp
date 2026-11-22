#include <cstdint>

#include "tusb.h"
#include "host/usbh.h"
#include "class/hid/hid_host.h"

#include "USBHost/HostDriver/XInput/tuh_xinput/tuh_xinput.h"
#include "USBHost/HostManager.h"
#include "OGXMini/OGXMini.h"
#include "Board/board_api.h"
#include "USBHost/HostDriver/XInput/EightBitDo3Mode.h"

#if defined(CONFIG_OGXM_BOARD_RP2040_ZERO)
// The 8BitDo receiver deliberately disconnects/re-enumerates when its
// controller pairs. Unlike an ordinary wired pad, this must not cause
// the entire OGX-Mini to reboot while the receiver is changing PID.
static uint8_t diag_receiver_addr = 0;

static bool is_known_receiver(uint8_t dev_addr) {
    uint16_t vid = 0, pid = 0;
    return tuh_vid_pid_get(dev_addr, &vid, &pid) &&
           EightBitDo3Mode::is_receiver_id(vid, pid);
}

// This fires after USB enumeration. Keep receiver identity visible EVEN
// when an XInput gamepad driver was mounted: green otherwise conceals
// whether the dongle was 200F (waiting) or 201B (controller present).
void tuh_mount_cb(uint8_t dev_addr) {
    uint16_t vid = 0, pid = 0;
    tuh_vid_pid_get(dev_addr, &vid, &pid);
    if (EightBitDo3Mode::is_receiver_id(vid, pid)) {
        diag_receiver_addr = dev_addr;
        if (pid == EightBitDo3Mode::PID_WAITING) {
            board_api::set_rgb(255, 128, 0); // Orange: waiting PID 200F
        } else {
            board_api::set_rgb(128, 0, 255); // Purple: paired PID 201B
        }
    } else if (!HostManager::get_instance().any_mounted()) {
        board_api::set_rgb(255, 255, 255); // White: other enumerated device
    }
}

void tuh_umount_cb(uint8_t dev_addr) {
    if (!HostManager::get_instance().any_mounted()) {
        board_api::set_rgb(0, 0, 255); // Blue: unmounted, host still running
    }
    if (diag_receiver_addr == dev_addr) {
        diag_receiver_addr = 0;
    }
}
#endif

usbh_class_driver_t const* usbh_app_driver_get_cb(uint8_t* driver_count) {
    *driver_count = 1;
    return tuh_xinput::class_driver();
}

//HID

void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len) {
    uint16_t vid, pid;
    tuh_vid_pid_get(dev_addr, &vid, &pid);

    HostManager& host_manager = HostManager::get_instance();

    if (host_manager.setup_driver(HostManager::get_type({ vid, pid }), dev_addr, instance, desc_report, desc_len)) {
        OGXMini::host_mounted(true);
    }
}

void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
    HostManager& host_manager = HostManager::get_instance();
    host_manager.deinit_driver(HostManager::DriverClass::HID, dev_addr, instance);

    if (!host_manager.any_mounted()) {
#if defined(CONFIG_OGXM_BOARD_RP2040_ZERO)
        // Do not reboot on the receiver's transient 200F -> 201B remount.
        // This is limited to the documented 8BitDo receiver IDs.
        if (diag_receiver_addr != dev_addr && !is_known_receiver(dev_addr)) {
            OGXMini::host_mounted(false);
        }
#else
        OGXMini::host_mounted(false);
#endif
    }
}

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len) {
    HostManager::get_instance().process_report(dev_addr, instance, report, len);
}

//XINPUT

void tuh_xinput::mount_cb(uint8_t dev_addr, uint8_t instance, const tuh_xinput::Interface* interface) {
    HostManager& host_manager = HostManager::get_instance();
    HostDriverType host_type = HostManager::get_type(interface->dev_type);

    if (host_manager.setup_driver(host_type, dev_addr, instance)) {
        OGXMini::host_mounted(true, host_type);
    }
}

void tuh_xinput::unmount_cb(uint8_t dev_addr, uint8_t instance, const tuh_xinput::Interface* interface) {
    HostManager& host_manager = HostManager::get_instance();
    host_manager.deinit_driver(HostManager::DriverClass::XINPUT, dev_addr, instance);

    if (!host_manager.any_mounted()) {
#if defined(CONFIG_OGXM_BOARD_RP2040_ZERO)
        // Do not reboot on the receiver's transient 200F -> 201B remount.
        // This is limited to the documented 8BitDo receiver IDs.
        if (diag_receiver_addr != dev_addr && !is_known_receiver(dev_addr)) {
            OGXMini::host_mounted(false);
        }
#else
        OGXMini::host_mounted(false);
#endif
    }
}

void tuh_xinput::report_received_cb(uint8_t dev_addr, uint8_t instance, const uint8_t* report, uint16_t len) {
    HostManager::get_instance().process_report(dev_addr, instance, report, len);
}

void tuh_xinput::xbox360w_connect_cb(uint8_t dev_addr, uint8_t instance) {
    uint8_t idx = HostManager::get_instance().get_gamepad_idx(  HostManager::DriverClass::XINPUT, 
                                                                dev_addr, instance);
    OGXMini::wireless_connected(true, idx);
    HostManager::get_instance().connect_cb(dev_addr, instance);
}

void tuh_xinput::xbox360w_disconnect_cb(uint8_t dev_addr, uint8_t instance) {
    uint8_t idx = HostManager::get_instance().get_gamepad_idx(  HostManager::DriverClass::XINPUT, 
                                                                dev_addr, instance);
    OGXMini::wireless_connected(false, idx);
    HostManager::get_instance().disconnect_cb(dev_addr, instance);
}