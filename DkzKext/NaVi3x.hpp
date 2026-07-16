// DkzKext — AMD Radeon RX 7000 (RDNA 3) Support for macOS
// NaVi3x.hpp — Hardware definitions, Device IDs, and register offsets
//
// Copyright © 2026 DKZ. All rights reserved.
// This file contains hardware-specific constants for RDNA 3 (GFX11) GPUs.

#ifndef DKZKEXT_NAVI3X_HPP
#define DKZKEXT_NAVI3X_HPP

#include <Headers/kern_util.hpp>

// =============================================================================
// PCI Vendor ID
// =============================================================================
static constexpr uint16_t kAMDVendorID = 0x1002;

// =============================================================================
// RDNA 3 Chip Families
// =============================================================================
enum ChipFamily : uint32_t {
    ChipFamilyUnknown = 0,
    // RDNA 3
    ChipFamilyNavi31   = 1,   // gfx1100 — RX 7900 XTX / 7900 XT / 7900 GRE
    ChipFamilyNavi32   = 2,   // gfx1101 — RX 7800 XT / 7700 XT
    ChipFamilyNavi33   = 3,   // gfx1102 — RX 7600 / 7600 XT
    // RDNA 2 (spoof targets)
    ChipFamilyNavi21   = 10,  // gfx1030 — RX 6900 XT / 6800 XT / 6800
    ChipFamilyNavi22   = 11,  // gfx1031 — RX 6700 XT
    ChipFamilyNavi23   = 12,  // gfx1032 — RX 6600 XT / 6600
};

// =============================================================================
// RDNA 3 Device IDs — Navi 31 (gfx1100)
// =============================================================================
namespace Navi31 {
    static constexpr uint16_t kDeviceIdRangeStart = 0x7440;
    static constexpr uint16_t kDeviceIdRangeEnd   = 0x745F;

    // Known specific Device IDs
    static constexpr uint16_t kDeviceId7440 = 0x7440;  // Navi 31 variant
    static constexpr uint16_t kDeviceId744C = 0x744C;  // RX 7900 XTX / RX 7900 XT

    static constexpr uint16_t kDeviceIds[] = {
        kDeviceId7440,
        kDeviceId744C,
    };
    static constexpr size_t kDeviceIdCount = arrsize(kDeviceIds);

    // Spoof target: Navi 21 (RX 6900 XT)
    static constexpr uint16_t kSpoofTargetDeviceId = 0x73BF;
}

// =============================================================================
// RDNA 3 Device IDs — Navi 32 (gfx1101)
// =============================================================================
namespace Navi32 {
    static constexpr uint16_t kDeviceIdRangeStart = 0x7460;
    static constexpr uint16_t kDeviceIdRangeEnd   = 0x747F;

    // Known specific Device IDs
    static constexpr uint16_t kDeviceId7460 = 0x7460;  // Navi 32 variant
    static constexpr uint16_t kDeviceId747E = 0x747E;  // RX 7800 XT / RX 7700 XT

    static constexpr uint16_t kDeviceIds[] = {
        kDeviceId7460,
        kDeviceId747E,
    };
    static constexpr size_t kDeviceIdCount = arrsize(kDeviceIds);

    // Spoof target: Navi 22 (RX 6700 XT)
    static constexpr uint16_t kSpoofTargetDeviceId = 0x73DF;
}

// =============================================================================
// RDNA 3 Device IDs — Navi 33 (gfx1102)
// =============================================================================
namespace Navi33 {
    static constexpr uint16_t kDeviceIdRangeStart = 0x7480;
    static constexpr uint16_t kDeviceIdRangeEnd   = 0x749F;

    // Known specific Device IDs
    static constexpr uint16_t kDeviceId7480 = 0x7480;  // RX 7600
    static constexpr uint16_t kDeviceId7481 = 0x7481;  // RX 7600 XT

    static constexpr uint16_t kDeviceIds[] = {
        kDeviceId7480,
        kDeviceId7481,
    };
    static constexpr size_t kDeviceIdCount = arrsize(kDeviceIds);

    // Spoof target: Navi 23 (RX 6600 XT)
    static constexpr uint16_t kSpoofTargetDeviceId = 0x73FF;
}

// =============================================================================
// Combined Device ID table for all RDNA 3 GPUs
// =============================================================================
struct DeviceIdEntry {
    uint16_t   deviceId;
    ChipFamily chipFamily;
    uint16_t   spoofDeviceId;
    const char *modelName;
};

static constexpr DeviceIdEntry kSupportedDeviceIds[] = {
    // Navi 31
    {0x7440, ChipFamilyNavi31, 0x73BF, "AMD Radeon RX 7900 (Navi 31)"},
    {0x744C, ChipFamilyNavi31, 0x73BF, "AMD Radeon RX 7900 XTX/XT"},
    // Navi 32
    {0x7460, ChipFamilyNavi32, 0x73DF, "AMD Radeon RX 7800/7700 (Navi 32)"},
    {0x747E, ChipFamilyNavi32, 0x73DF, "AMD Radeon RX 7800 XT / RX 7700 XT"},
    // Navi 33
    {0x7480, ChipFamilyNavi33, 0x73FF, "AMD Radeon RX 7600"},
    {0x7481, ChipFamilyNavi33, 0x73FF, "AMD Radeon RX 7600 XT"},
};
static constexpr size_t kSupportedDeviceIdCount = arrsize(kSupportedDeviceIds);

// =============================================================================
// RDNA 2 Spoof Target Device IDs
// =============================================================================
namespace SpoofTargets {
    // Navi 21 — RX 6900 XT / 6800 XT
    static constexpr uint16_t kNavi21_73BF = 0x73BF;
    static constexpr uint16_t kNavi21_73A5 = 0x73A5;
    // Navi 22 — RX 6700 XT
    static constexpr uint16_t kNavi22_73DF = 0x73DF;
    // Navi 23 — RX 6600 XT
    static constexpr uint16_t kNavi23_73FF = 0x73FF;
    static constexpr uint16_t kNavi23_73E3 = 0x73E3;
}

// =============================================================================
// GFX IP Version Constants
// =============================================================================
enum GFXIPVersion : uint32_t {
    GFX_v10_3_0 = 0x0A0300,  // RDNA 2 — Navi 21
    GFX_v10_3_1 = 0x0A0301,  // RDNA 2 — Navi 22
    GFX_v10_3_2 = 0x0A0302,  // RDNA 2 — Navi 23
    GFX_v11_0_0 = 0x0B0000,  // RDNA 3 — Navi 31
    GFX_v11_0_1 = 0x0B0001,  // RDNA 3 — Navi 32
    GFX_v11_0_2 = 0x0B0002,  // RDNA 3 — Navi 33
};

// =============================================================================
// DCN (Display Core Next) Version Constants
// =============================================================================
enum DCNVersion : uint32_t {
    DCN_v3_2_0 = 0x030200,   // RDNA 3 — Navi 31
    DCN_v3_2_1 = 0x030201,   // RDNA 3 — Navi 32/33
    DCN_v3_0_1 = 0x030001,   // RDNA 2 — Navi 21 (spoof target)
};

// =============================================================================
// Hardware Register Offsets (GFX11)
// =============================================================================
namespace GFX11Regs {
    // GPU Identification
    static constexpr uint32_t mmGRBM_STATUS         = 0x2004;
    static constexpr uint32_t mmGRBM_STATUS2        = 0x2002;
    static constexpr uint32_t mmRLC_STAT            = 0x4E44;
    static constexpr uint32_t mmCP_STAT             = 0x2100;

    // Memory Controller
    static constexpr uint32_t mmMC_VM_FB_OFFSET     = 0x096B;
    static constexpr uint32_t mmMC_VM_FB_SIZE_OFFSET = 0x096C;

    // Display Controller
    static constexpr uint32_t mmDCHUBBUB_SDPIF_MMIO_CNTRL_0 = 0x049D;

    // Power Management
    static constexpr uint32_t mmMP1_SMN_C2PMSG_90   = 0x029A;
    static constexpr uint32_t mmMP1_SMN_C2PMSG_82   = 0x0292;
    static constexpr uint32_t mmMP1_SMN_C2PMSG_66   = 0x0282;

    // Clocks
    static constexpr uint32_t mmCG_CLKPIN_CNTL_2    = 0x008D;
    static constexpr uint32_t mmMPLL_SEQ_UCODE_1    = 0x0091;
}

// =============================================================================
// Connector Types for Display Output
// =============================================================================
enum ConnectorType : uint8_t {
    ConnectorDP    = 0x00,   // DisplayPort (including DP 2.1)
    ConnectorHDMI  = 0x01,   // HDMI (including HDMI 2.1)
    ConnectorDVI   = 0x02,   // DVI-D (legacy)
    ConnectorVGA   = 0x03,   // VGA (legacy, not used on RDNA 3)
    ConnectorUSBC  = 0x04,   // USB-C with DP Alt Mode
    ConnectorNone  = 0xFF,   // No connector
};

// =============================================================================
// Framebuffer Connector Entry
// =============================================================================
struct ConnectorEntry {
    ConnectorType type;
    uint8_t       portIndex;
    uint8_t       transmitterIndex;
    uint8_t       encoderIndex;
    uint8_t       senseId;
    uint8_t       priority;
    bool          hotplugSupported;
};

// =============================================================================
// Framebuffer Info
// =============================================================================
struct FramebufferInfo {
    ChipFamily     chipFamily;
    uint16_t       deviceId;
    uint32_t       connectorCount;
    ConnectorEntry connectors[6];  // Max 6 display outputs
    uint64_t       vramSize;       // VRAM size in bytes (0 = auto-detect)
};

#endif // DKZKEXT_NAVI3X_HPP
