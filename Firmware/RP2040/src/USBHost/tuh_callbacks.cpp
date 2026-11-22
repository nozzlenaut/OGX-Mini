#include <cstdint>

#include "tusb.h"
#include "host/usbh.h"
#include "class/hid/hid_host.h"

#include "USBHost/HostDriver/XInput/tuh_xinput/tuh_xinput.h"
#include "USBHost/HostManager.h"
#include "OGXMini/OGXMini.h"
#include "USBHost/HostDriver/XInput/EightBitDo3Mode.h"

// The receiver may disappear with one PID and return as another.
// Keep this state until the entire USB device has unmounted; VID/PID lookup
// is not always valid while TinyUSB calls the interface unmount callback.
#if defined(CONFIG_OGXM_BOARD_RP2040_ZERO)
static uint8_t eightbitdo_receiver_addr = 0;
static bool is_receiver_unmount(uint8_t dev_addr) {
    uint16_t vid = 0, pid = 0;
    return eightbitdo_receiver_addr == dev_addr ||
           (tuh_vid_pid_get(dev_addr, &vid, &pid) &&
            EightBitDo3Mode::is_receiver_id(vid, pid));
}
void tuh_umount_cb(uint8_t dev_addr) {
    if (eightbitdo_receiver_addr == dev_addr) {
        eightbitdo_receiver_addr = 0;
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
        OGXMini::host_mounted(false);
    }
}

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len) {
    HostManager::get_instance().process_report(dev_addr, instance, report, len);
}

//XINPUT

void tuh_xinput::mount_cb(uint8_t dev_addr, uint8_t instance, const tuh_xinput::Interface* interface) {
#if defined(CONFIG_OGXM_BOARD_RP2040_ZERO)
    if (interface && interface->is_8bitdo_receiver) {
        eightbitdo_receiver_addr = dev_addr;
    }
#endif
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
        if (!is_receiver_unmount(dev_addr)) {
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