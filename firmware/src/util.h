#pragma once

#define MIN(a, b) ((a) < (b) ? (a) : (b))

int64_t convertRawToCurrent_uA(int64_t rawValue);

uint64_t get64bitMicros();