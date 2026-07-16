// DkzKext — AMD Radeon RX 7000 (RDNA 3) Support for macOS
// ATOMBIOS.hpp — AtomBIOS firmware table structures
//
// Copyright © 2026 DKZ. All rights reserved.
// Based on AMD's public AtomBIOS documentation and Linux amdgpu driver headers.

#ifndef DKZKEXT_ATOMBIOS_HPP
#define DKZKEXT_ATOMBIOS_HPP

#include <stdint.h>

// =============================================================================
// AtomBIOS Common Header
// =============================================================================

#pragma pack(push, 1)

struct AtomCommonTableHeader {
    uint16_t structureSize;
    uint8_t  formatRevision;
    uint8_t  contentRevision;
};

// =============================================================================
// AtomBIOS ROM Header
// =============================================================================

struct AtomRomHeader {
    AtomCommonTableHeader header;
    uint32_t firmwareSignature;       // Should be 0x0000AABB
    uint16_t biosRuntimeSegmentAddr;
    uint16_t protectedModeInfoOffset;
    uint16_t configFilenameOffset;
    uint16_t crcBlockOffset;
    uint16_t biosBootupMessageOffset;
    uint16_t int10Offset;
    uint16_t pciInfoOffset;
    uint16_t pciBusDevInitCode;
    uint16_t ioBaseAddr;
    uint16_t subsystemVendorId;
    uint16_t subsystemId;
    uint16_t pciInfoTableOffset;
    uint16_t masterCommandTableOffset;
    uint16_t masterDataTableOffset;
    uint8_t  extendedFunctionCode;
    uint8_t  reserved;
};

// =============================================================================
// VRAM Info (simplified for RDNA 3)
// =============================================================================

struct AtomVramModuleV3 {
    AtomCommonTableHeader header;
    uint32_t channelMapConfig;
    uint16_t moduleSize;
    uint16_t mcRamCfg;
    uint16_t enableChannels;
    uint8_t  extMemoryId;
    uint8_t  memoryType;           // GDDR6 = 6, GDDR6X = 7
    uint8_t  channelNum;
    uint8_t  channelWidth;
    uint8_t  density;
    uint8_t  bankCol;
    uint8_t  misc;
    uint8_t  vrefi;
    uint16_t reserved;
    uint16_t memorySize;           // In MB
    uint8_t  memVendorRevId;
    uint8_t  refreshRateFactor;
    uint8_t  nvitId;
    uint8_t  bankGroupSwap;
    uint8_t  dqPadDriving;
    uint8_t  dqsPadDriving;
    uint8_t  addressPadDriving;
    uint8_t  clkPadDriving;
};

struct AtomVramInfoHeaderV3 {
    AtomCommonTableHeader header;
    uint16_t memAdjustTableOffset;
    uint16_t memClkPatchTableOffset;
    uint16_t perBytePresetOffset;
    uint16_t reserved[3];
    uint8_t  numVramModules;
    uint8_t  memoryClkPatchTableVersion;
    uint8_t  vramModuleVersion;
    uint8_t  mcPhyTileNum;
    // Followed by AtomVramModuleV3[]
};

// =============================================================================
// PowerPlay Table Structures (RDNA 3 / SMU 13.x)
// =============================================================================

struct PPTableHeader {
    AtomCommonTableHeader header;
    uint8_t  tableRevision;
    uint16_t tableSize;
    uint32_t goldenPPId;
    uint32_t goldenRevision;
    uint16_t formatId;
};

// SMU 13.0 Clock Entry
struct SmuClockEntry {
    uint8_t  enabled;
    uint8_t  padding[3];
    uint32_t freq;    // In 10KHz units
    uint32_t vol;     // In mV
};

// Simplified PowerPlay Table for RDNA 3
struct PPTable_RDNA3 {
    PPTableHeader       header;
    uint32_t            maxGfxClk;      // Max GFXCLK in MHz
    uint32_t            minGfxClk;      // Min GFXCLK in MHz
    uint32_t            maxMemClk;      // Max MCLK in MHz
    uint32_t            minMemClk;      // Min MCLK in MHz
    uint32_t            maxSocClk;      // Max SOCCLK in MHz
    uint32_t            minSocClk;      // Min SOCCLK in MHz
    uint16_t            maxVoltageGfx;  // mV
    uint16_t            maxVoltageMem;  // mV
    uint16_t            maxVoltageSoc;  // mV
    uint16_t            tdp;            // TDP in watts
    uint16_t            tdc;            // TDC in amps
    uint16_t            maxTemp;        // Max temp in Celsius
    uint32_t            fanTargetTemp;  // Target fan temp
    uint32_t            fanMaxRPM;      // Max fan RPM
    uint8_t             numGfxClkLevels;
    uint8_t             numMemClkLevels;
    uint8_t             numSocClkLevels;
    uint8_t             padding;
    SmuClockEntry       gfxClkLevels[8];
    SmuClockEntry       memClkLevels[4];
    SmuClockEntry       socClkLevels[8];
};

// =============================================================================
// Display Connector Objects (AtomBIOS Display Object Path)
// =============================================================================

struct AtomDisplayObjectPath {
    uint16_t displayObjId;
    uint16_t gpuObjId;
    uint16_t encoderObjId;
    uint16_t extEncoderObjId;
    uint16_t encoderRecordOffset;
    uint16_t extEncoderRecordOffset;
    uint16_t deviceTag;
    uint8_t  priority;
    uint8_t  reserved;
};

struct AtomObjectHeader {
    AtomCommonTableHeader header;
    uint16_t deviceSupport;
    uint16_t connectorObjectTableOffset;
    uint16_t routerObjectTableOffset;
    uint16_t encoderObjectTableOffset;
    uint16_t protectionObjectTableOffset;
    uint16_t displayPathTableOffset;
};

// =============================================================================
// Integrated System Info (useful for firmware version detection)
// =============================================================================

struct AtomIntegratedSystemInfo {
    AtomCommonTableHeader header;
    uint32_t bootUpEngineClock;
    uint32_t bootUpMemoryClock;
    uint32_t maxSystemMemoryClock;
    uint32_t minSystemMemoryClock;
    uint8_t  numberOfCyclesInPeriod;
    uint8_t  reserved1[3];
    uint32_t interNBVoltageLow;
    uint32_t interNBVoltageHigh;
    uint32_t reserved2[2];
    uint16_t bootUpNBVoltage;
    uint16_t reserved3;
};

#pragma pack(pop)

// =============================================================================
// Memory Type Constants
// =============================================================================

enum MemoryType : uint8_t {
    MEMORY_TYPE_GDDR5  = 5,
    MEMORY_TYPE_GDDR6  = 6,
    MEMORY_TYPE_GDDR6X = 7,  // Not used by AMD, but defined
    MEMORY_TYPE_HBM2   = 8,
    MEMORY_TYPE_HBM2E  = 9,
    MEMORY_TYPE_HBM3   = 10,
};

// =============================================================================
// AtomBIOS Signature Verification
// =============================================================================

static constexpr uint16_t kAtomBiosSignature    = 0xAA55;
static constexpr uint32_t kAtomRomMagic         = 0x0000AABB;
static constexpr char     kAtomBiosString[]     = "ATOM";

#endif // DKZKEXT_ATOMBIOS_HPP
