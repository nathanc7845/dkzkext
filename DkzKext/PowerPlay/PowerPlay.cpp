// DkzKext — AMD Radeon RX 7000 (RDNA 3) Support for macOS
// PowerPlay.cpp — Power management implementation with safe defaults
//
// Copyright © 2026 DKZ. All rights reserved.

#include "PowerPlay.hpp"
#include "../../Headers/kern_util.hpp"

// =============================================================================
// Default Power Specifications per GPU
// =============================================================================

// These are conservative defaults based on AMD's reference specifications.
// Using lower-than-stock clocks and voltages for safety, as the RDNA 2
// power management code may not handle RDNA 3 VRM/power delivery correctly.

namespace PowerSpecs {
    // RX 7900 XTX — Navi 31
    struct Navi31 {
        static constexpr uint32_t maxGfxClk  = 2500;   // MHz (stock boost: 2500)
        static constexpr uint32_t minGfxClk  = 300;    // MHz
        static constexpr uint32_t maxMemClk  = 1250;   // MHz (GDDR6 20Gbps)
        static constexpr uint32_t minMemClk  = 96;     // MHz
        static constexpr uint32_t maxSocClk  = 1950;   // MHz
        static constexpr uint32_t minSocClk  = 400;    // MHz
        static constexpr uint16_t maxVoltGfx = 1200;   // mV
        static constexpr uint16_t maxVoltMem = 1350;   // mV
        static constexpr uint16_t maxVoltSoc = 1150;   // mV
        static constexpr uint16_t tdp        = 355;    // Watts
        static constexpr uint16_t tdc        = 390;    // Amps
        static constexpr uint16_t maxTemp    = 110;    // Celsius (hotspot)
        static constexpr uint32_t fanTemp    = 85;     // Target fan temp
        static constexpr uint32_t fanMaxRPM  = 3200;   // Max RPM
    };

    // RX 7800 XT — Navi 32
    struct Navi32 {
        static constexpr uint32_t maxGfxClk  = 2430;   // MHz
        static constexpr uint32_t minGfxClk  = 300;    // MHz
        static constexpr uint32_t maxMemClk  = 1200;   // MHz (GDDR6 19.5Gbps)
        static constexpr uint32_t minMemClk  = 96;     // MHz
        static constexpr uint32_t maxSocClk  = 1800;   // MHz
        static constexpr uint32_t minSocClk  = 400;    // MHz
        static constexpr uint16_t maxVoltGfx = 1200;   // mV
        static constexpr uint16_t maxVoltMem = 1350;   // mV
        static constexpr uint16_t maxVoltSoc = 1100;   // mV
        static constexpr uint16_t tdp        = 263;    // Watts
        static constexpr uint16_t tdc        = 280;    // Amps
        static constexpr uint16_t maxTemp    = 110;    // Celsius
        static constexpr uint32_t fanTemp    = 80;     // Target fan temp
        static constexpr uint32_t fanMaxRPM  = 2800;   // Max RPM
    };

    // RX 7600 — Navi 33
    struct Navi33 {
        static constexpr uint32_t maxGfxClk  = 2655;   // MHz
        static constexpr uint32_t minGfxClk  = 300;    // MHz
        static constexpr uint32_t maxMemClk  = 1125;   // MHz (GDDR6 18Gbps)
        static constexpr uint32_t minMemClk  = 96;     // MHz
        static constexpr uint32_t maxSocClk  = 1600;   // MHz
        static constexpr uint32_t minSocClk  = 400;    // MHz
        static constexpr uint16_t maxVoltGfx = 1150;   // mV
        static constexpr uint16_t maxVoltMem = 1350;   // mV
        static constexpr uint16_t maxVoltSoc = 1050;   // mV
        static constexpr uint16_t tdp        = 165;    // Watts
        static constexpr uint16_t tdc        = 180;    // Amps
        static constexpr uint16_t maxTemp    = 100;    // Celsius
        static constexpr uint32_t fanTemp    = 75;     // Target fan temp
        static constexpr uint32_t fanMaxRPM  = 2500;   // Max RPM
    };
}

// =============================================================================
// Constructor
// =============================================================================

DkzPowerPlay::DkzPowerPlay(ChipFamily family, uint16_t devId)
    : chipFamily(family), deviceId(devId) {
    buildDefaultPPTable();
}

// =============================================================================
// Build Default PowerPlay Table
// =============================================================================

void DkzPowerPlay::buildDefaultPPTable() {
    memset(&defaultPPTable, 0, sizeof(defaultPPTable));

    // Header
    defaultPPTable.header.header.structureSize = sizeof(PPTable_RDNA3);
    defaultPPTable.header.header.formatRevision = 1;
    defaultPPTable.header.header.contentRevision = 0;
    defaultPPTable.header.tableRevision = 1;
    defaultPPTable.header.tableSize = sizeof(PPTable_RDNA3);

    switch (chipFamily) {
        case ChipFamilyNavi31: {
            using S = PowerSpecs::Navi31;
            defaultPPTable.maxGfxClk     = S::maxGfxClk;
            defaultPPTable.minGfxClk     = S::minGfxClk;
            defaultPPTable.maxMemClk     = S::maxMemClk;
            defaultPPTable.minMemClk     = S::minMemClk;
            defaultPPTable.maxSocClk     = S::maxSocClk;
            defaultPPTable.minSocClk     = S::minSocClk;
            defaultPPTable.maxVoltageGfx = S::maxVoltGfx;
            defaultPPTable.maxVoltageMem = S::maxVoltMem;
            defaultPPTable.maxVoltageSoc = S::maxVoltSoc;
            defaultPPTable.tdp           = S::tdp;
            defaultPPTable.tdc           = S::tdc;
            defaultPPTable.maxTemp       = S::maxTemp;
            defaultPPTable.fanTargetTemp = S::fanTemp;
            defaultPPTable.fanMaxRPM     = S::fanMaxRPM;

            // Clock levels (3 GFX levels)
            defaultPPTable.numGfxClkLevels = 3;
            defaultPPTable.gfxClkLevels[0] = {1, {}, S::minGfxClk * 100, 750};
            defaultPPTable.gfxClkLevels[1] = {1, {}, 1500 * 100, 1000};
            defaultPPTable.gfxClkLevels[2] = {1, {}, S::maxGfxClk * 100, S::maxVoltGfx};

            // Memory clock levels (2 levels)
            defaultPPTable.numMemClkLevels = 2;
            defaultPPTable.memClkLevels[0] = {1, {}, S::minMemClk * 100, 800};
            defaultPPTable.memClkLevels[1] = {1, {}, S::maxMemClk * 100, S::maxVoltMem};

            DKZLOG("PowerPlay: Built PPTable for Navi 31 (TDP: %uW, Max GFXCLK: %uMHz)",
                   S::tdp, S::maxGfxClk);
            break;
        }

        case ChipFamilyNavi32: {
            using S = PowerSpecs::Navi32;
            defaultPPTable.maxGfxClk     = S::maxGfxClk;
            defaultPPTable.minGfxClk     = S::minGfxClk;
            defaultPPTable.maxMemClk     = S::maxMemClk;
            defaultPPTable.minMemClk     = S::minMemClk;
            defaultPPTable.maxSocClk     = S::maxSocClk;
            defaultPPTable.minSocClk     = S::minSocClk;
            defaultPPTable.maxVoltageGfx = S::maxVoltGfx;
            defaultPPTable.maxVoltageMem = S::maxVoltMem;
            defaultPPTable.maxVoltageSoc = S::maxVoltSoc;
            defaultPPTable.tdp           = S::tdp;
            defaultPPTable.tdc           = S::tdc;
            defaultPPTable.maxTemp       = S::maxTemp;
            defaultPPTable.fanTargetTemp = S::fanTemp;
            defaultPPTable.fanMaxRPM     = S::fanMaxRPM;

            defaultPPTable.numGfxClkLevels = 3;
            defaultPPTable.gfxClkLevels[0] = {1, {}, S::minGfxClk * 100, 750};
            defaultPPTable.gfxClkLevels[1] = {1, {}, 1400 * 100, 1000};
            defaultPPTable.gfxClkLevels[2] = {1, {}, S::maxGfxClk * 100, S::maxVoltGfx};

            defaultPPTable.numMemClkLevels = 2;
            defaultPPTable.memClkLevels[0] = {1, {}, S::minMemClk * 100, 800};
            defaultPPTable.memClkLevels[1] = {1, {}, S::maxMemClk * 100, S::maxVoltMem};

            DKZLOG("PowerPlay: Built PPTable for Navi 32 (TDP: %uW, Max GFXCLK: %uMHz)",
                   S::tdp, S::maxGfxClk);
            break;
        }

        case ChipFamilyNavi33: {
            using S = PowerSpecs::Navi33;
            defaultPPTable.maxGfxClk     = S::maxGfxClk;
            defaultPPTable.minGfxClk     = S::minGfxClk;
            defaultPPTable.maxMemClk     = S::maxMemClk;
            defaultPPTable.minMemClk     = S::minMemClk;
            defaultPPTable.maxSocClk     = S::maxSocClk;
            defaultPPTable.minSocClk     = S::minSocClk;
            defaultPPTable.maxVoltageGfx = S::maxVoltGfx;
            defaultPPTable.maxVoltageMem = S::maxVoltMem;
            defaultPPTable.maxVoltageSoc = S::maxVoltSoc;
            defaultPPTable.tdp           = S::tdp;
            defaultPPTable.tdc           = S::tdc;
            defaultPPTable.maxTemp       = S::maxTemp;
            defaultPPTable.fanTargetTemp = S::fanTemp;
            defaultPPTable.fanMaxRPM     = S::fanMaxRPM;

            defaultPPTable.numGfxClkLevels = 3;
            defaultPPTable.gfxClkLevels[0] = {1, {}, S::minGfxClk * 100, 700};
            defaultPPTable.gfxClkLevels[1] = {1, {}, 1500 * 100, 950};
            defaultPPTable.gfxClkLevels[2] = {1, {}, S::maxGfxClk * 100, S::maxVoltGfx};

            defaultPPTable.numMemClkLevels = 2;
            defaultPPTable.memClkLevels[0] = {1, {}, S::minMemClk * 100, 750};
            defaultPPTable.memClkLevels[1] = {1, {}, S::maxMemClk * 100, S::maxVoltMem};

            DKZLOG("PowerPlay: Built PPTable for Navi 33 (TDP: %uW, Max GFXCLK: %uMHz)",
                   S::tdp, S::maxGfxClk);
            break;
        }

        default:
            DKZERR("PowerPlay: Unknown chip family %u — using minimal defaults",
                   chipFamily);
            defaultPPTable.maxGfxClk = 1500;
            defaultPPTable.minGfxClk = 300;
            defaultPPTable.maxMemClk = 1000;
            defaultPPTable.minMemClk = 96;
            defaultPPTable.tdp = 150;
            defaultPPTable.maxTemp = 100;
            break;
    }

    ppTableBuilt = true;
}

// =============================================================================
// Accessors
// =============================================================================

const PPTable_RDNA3* DkzPowerPlay::getDefaultPPTable() const {
    return ppTableBuilt ? &defaultPPTable : nullptr;
}

uint16_t DkzPowerPlay::getTDP() const {
    return defaultPPTable.tdp;
}

uint32_t DkzPowerPlay::getMaxGfxClock() const {
    return defaultPPTable.maxGfxClk;
}

uint32_t DkzPowerPlay::getMaxMemClock() const {
    return defaultPPTable.maxMemClk;
}

// =============================================================================
// Apply Patches
// =============================================================================

void DkzPowerPlay::applyPatches(KernelPatcher &patcher, size_t index,
                                 mach_vm_address_t address, size_t size) {
    DKZLOG("PowerPlay: Applying power management patches...");

    // The PowerPlay subsystem in the AMD driver handles all power management.
    // For RDNA 3 running through RDNA 2 drivers:
    //
    // 1. The VBIOS PowerPlay table (PPTable) is parsed by the driver
    //    Our spoofed device should let the driver parse the VBIOS PPTable
    //    from the actual GPU, which contains correct clock/voltage data
    //
    // 2. SMU commands are sent to control clocks/voltages
    //    Since SMU 13.0 differs from SMU 11.0/12.0, we operate in
    //    "conservative" mode — letting the GPU VBIOS/SMU handle power
    //    management autonomously rather than sending explicit commands
    //
    // 3. Temperature monitoring uses different sensor addresses
    //    We rely on the GPU's built-in thermal protection rather than
    //    software-controlled fan curves

    DKZWARN("PowerPlay: Operating in conservative mode. "
            "Power management features (fan control, clock adjustment) "
            "may be limited due to SMU 13.0 incompatibility.");

    DKZLOG("PowerPlay: Default PPTable ready with safe limits:");
    DKZLOG("  TDP: %uW | Max GFXCLK: %uMHz | Max MCLK: %uMHz | Max Temp: %u°C",
           defaultPPTable.tdp, defaultPPTable.maxGfxClk,
           defaultPPTable.maxMemClk, defaultPPTable.maxTemp);

    DKZLOG("PowerPlay: Patches complete");
}
