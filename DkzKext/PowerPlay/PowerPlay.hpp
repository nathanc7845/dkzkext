// DkzKext — AMD Radeon RX 7000 (RDNA 3) Support for macOS
// PowerPlay.hpp — Power management controller header
//
// Copyright © 2026 DKZ. All rights reserved.

#ifndef DKZKEXT_POWERPLAY_HPP
#define DKZKEXT_POWERPLAY_HPP

#include <Headers/kern_patcher.hpp>
#include "../NaVi3x.hpp"
#include "../../Headers/ATOMBIOS.hpp"

// =============================================================================
// DkzPowerPlay — Power Management Controller
//
// Handles GPU power management including:
// - Clock frequency management (GFXCLK, MCLK, SOCCLK)
// - Voltage regulation
// - Fan control
// - Temperature monitoring
// - Power limit management
//
// RDNA 3 uses SMU 13.0 for power management, which differs significantly
// from the SMU 11.0/12.0 used by RDNA 2. This module provides safe defaults
// and stubs for incompatible PM features.
// =============================================================================

class DkzPowerPlay {
public:
    DkzPowerPlay(ChipFamily family, uint16_t deviceId);
    ~DkzPowerPlay() = default;

    /**
     * Apply power management patches.
     */
    void applyPatches(KernelPatcher &patcher, size_t index,
                      mach_vm_address_t address, size_t size);

    /**
     * Get default PowerPlay table for the GPU.
     * Returns a safe default PPTable that won't push the GPU
     * beyond known-safe operating parameters.
     */
    const PPTable_RDNA3* getDefaultPPTable() const;

    /**
     * Get the TDP (Thermal Design Power) in watts.
     */
    uint16_t getTDP() const;

    /**
     * Get maximum GPU clock in MHz.
     */
    uint32_t getMaxGfxClock() const;

    /**
     * Get maximum memory clock in MHz.
     */
    uint32_t getMaxMemClock() const;

private:
    /**
     * Build default PowerPlay table for the detected GPU.
     */
    void buildDefaultPPTable();

    ChipFamily     chipFamily;
    uint16_t       deviceId;
    PPTable_RDNA3  defaultPPTable;
    bool           ppTableBuilt = false;
};

#endif // DKZKEXT_POWERPLAY_HPP
