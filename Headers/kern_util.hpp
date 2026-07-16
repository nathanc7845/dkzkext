// DkzKext — AMD Radeon RX 7000 (RDNA 3) Support for macOS
// kern_util.hpp — Utility macros and helpers for kernel extension development
//
// Copyright © 2026 DKZ. All rights reserved.

#ifndef DKZKEXT_KERN_UTIL_HPP
#define DKZKEXT_KERN_UTIL_HPP

#include <libkern/libkern.h>
#include <mach/mach_types.h>
#include <IOKit/IOLib.h>

// =============================================================================
// Logging Macros
// =============================================================================

#define DKZKEXT_LOG_PREFIX "DkzKext: "

#ifdef DEBUG
#define DKZDBG(fmt, ...) \
    IOLog(DKZKEXT_LOG_PREFIX "[DEBUG] " fmt "\n", ##__VA_ARGS__)
#else
#define DKZDBG(fmt, ...) do {} while(0)
#endif

#define DKZLOG(fmt, ...) \
    IOLog(DKZKEXT_LOG_PREFIX fmt "\n", ##__VA_ARGS__)

#define DKZERR(fmt, ...) \
    IOLog(DKZKEXT_LOG_PREFIX "[ERROR] " fmt "\n", ##__VA_ARGS__)

#define DKZWARN(fmt, ...) \
    IOLog(DKZKEXT_LOG_PREFIX "[WARN] " fmt "\n", ##__VA_ARGS__)

// =============================================================================
// Utility Macros
// =============================================================================

// Get array size at compile time
#ifndef arrsize
#define arrsize(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

// Bit manipulation
#define BIT(n)              (1UL << (n))
#define BITS(hi, lo)        (((1UL << ((hi) - (lo) + 1)) - 1) << (lo))
#define GET_BITS(val, hi, lo) (((val) >> (lo)) & ((1UL << ((hi) - (lo) + 1)) - 1))
#define SET_BITS(val, hi, lo, bits) \
    (((val) & ~BITS(hi, lo)) | (((bits) & ((1UL << ((hi) - (lo) + 1)) - 1)) << (lo)))

// Alignment macros
#define ALIGN_UP(val, align)   (((val) + (align) - 1) & ~((align) - 1))
#define ALIGN_DOWN(val, align) ((val) & ~((align) - 1))
#define IS_ALIGNED(val, align) (((val) & ((align) - 1)) == 0)

// Memory size helpers
#define KB(n) ((n) * 1024ULL)
#define MB(n) ((n) * 1024ULL * 1024ULL)
#define GB(n) ((n) * 1024ULL * 1024ULL * 1024ULL)

// =============================================================================
// PCI Configuration Space Helpers
// =============================================================================

// PCI Config offsets
#define PCI_VENDOR_ID        0x00
#define PCI_DEVICE_ID        0x02
#define PCI_COMMAND          0x04
#define PCI_STATUS           0x06
#define PCI_REVISION_ID      0x08
#define PCI_CLASS_CODE       0x09
#define PCI_SUBCLASS         0x0A
#define PCI_BASE_CLASS       0x0B
#define PCI_SUBSYSTEM_VENDOR 0x2C
#define PCI_SUBSYSTEM_ID     0x2E
#define PCI_BAR0             0x10
#define PCI_BAR1             0x14
#define PCI_BAR2             0x18
#define PCI_BAR3             0x1C
#define PCI_BAR4             0x20
#define PCI_BAR5             0x24

// PCI Command Register bits
#define PCI_CMD_IO_SPACE     BIT(0)
#define PCI_CMD_MEMORY_SPACE BIT(1)
#define PCI_CMD_BUS_MASTER   BIT(2)

// =============================================================================
// IOKit Property Keys
// =============================================================================

#define kDkzDeviceIdKey          "dkz-device-id"
#define kDkzOriginalDeviceIdKey  "dkz-original-device-id"
#define kDkzSpoofedDeviceIdKey   "dkz-spoofed-device-id"
#define kDkzChipFamilyKey        "dkz-chip-family"
#define kDkzGPUModelKey          "dkz-gpu-model"
#define kDkzVersionKey           "dkz-version"

// =============================================================================
// Helper Functions
// =============================================================================

static inline const char* chipFamilyToString(uint32_t family) {
    switch (family) {
        case 1:  return "Navi 31 (gfx1100)";
        case 2:  return "Navi 32 (gfx1101)";
        case 3:  return "Navi 33 (gfx1102)";
        case 10: return "Navi 21 (gfx1030) [spoof]";
        case 11: return "Navi 22 (gfx1031) [spoof]";
        case 12: return "Navi 23 (gfx1032) [spoof]";
        default: return "Unknown";
    }
}

static inline bool isNavi3x(uint16_t deviceId) {
    return (deviceId >= 0x7440 && deviceId <= 0x749F);
}

static inline bool isNavi31(uint16_t deviceId) {
    return (deviceId >= 0x7440 && deviceId <= 0x745F);
}

static inline bool isNavi32(uint16_t deviceId) {
    return (deviceId >= 0x7460 && deviceId <= 0x747F);
}

static inline bool isNavi33(uint16_t deviceId) {
    return (deviceId >= 0x7480 && deviceId <= 0x749F);
}

#endif // DKZKEXT_KERN_UTIL_HPP
