#pragma once
#include <cstdint>
#include <cstddef>

namespace PS4LocalAuth {
bool initialize();
bool available();
void reset();
void receive(const uint8_t* buffer, uint16_t size);
uint16_t get(uint8_t id, uint8_t* buffer, uint16_t size);
void service();
void worker();
}
