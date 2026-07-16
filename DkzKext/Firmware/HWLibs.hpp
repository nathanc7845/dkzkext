// DkzKext — AMD Radeon RX 7000 (RDNA 3) Support for macOS
// HWLibs.hpp — Hardware Libraries patching header
//
// Copyright © 2026 DKZ. All rights reserved.

#ifndef DKZKEXT_HWLIBS_HPP
#define DKZKEXT_HWLIBS_HPP

#include <Headers/kern_patcher.hpp>
#include "../NaVi3x.hpp"

// =============================================================================
// DkzHWLibs — Hardware Library Patches
//
// The AMD macOS drivers use internal "HWLibs" modules for GPU initialization,
// power management, and hardware abstraction. These are architecture-specific
// (e.g., there are separate HWLibs for Navi 21, Navi 22, Navi 23).
//
// Since no RDNA 3 HWLibs exist in macOS, we:
// 1. Redirect the HWLibs loader to use RDNA 2 modules
// 2. Patch the loaded RDNA 2 HWLibs to handle RDNA 3 register differences
// 3. Stub functions that would crash due to GFX11 vs GFX10.3 differences
// =============================================================================

class DkzHWLibs {
public:
    DkzHWLibs(ChipFamily family, uint16_t deviceId);
    ~DkzHWLibs() = default;

    /**
     * Apply HWLibs-related binary patches to AMDRadeonX6000HWServices.kext.
     */
    void applyPatches(KernelPatcher &patcher, size_t index,
                      mach_vm_address_t address, size_t size);

    /**
     * Get a redirected HWLibs module pointer.
     * Called when the original getHWLibsModule() fails (no RDNA 3 module).
     * Returns a pointer to the RDNA 2 HWLibs module instead.
     */
    void* getRedirectedModule(void *hwServices);

    /**
     * Apply patches before HWLibs initialization.
     * Sets up register mappings and stubs incompatible functions.
     */
    void preInitPatches(void *hwServices, void *initParam);

    /**
     * Apply patches after HWLibs initialization.
     * Fixes up any state that the RDNA 2 init got wrong for RDNA 3 hardware.
     */
    void postInitPatches(void *hwServices, void *initParam);

private:
    /**
     * Get the HWLibs module name for the spoof target.
     */
    const char* getSpoofHWLibsName() const;

    /**
     * Patch GFX IP version comparisons in the HWLibs.
     */
    void patchGFXIPChecks(KernelPatcher &patcher, size_t index,
                          mach_vm_address_t address, size_t size);

    /**
     * Patch SMU (System Management Unit) version checks.
     * RDNA 3 uses SMU 13.x, RDNA 2 uses SMU 11.x/12.x.
     */
    void patchSMUChecks(KernelPatcher &patcher, size_t index,
                        mach_vm_address_t address, size_t size);

    /**
     * Patch PSP (Platform Security Processor) firmware loading.
     */
    void patchPSPFirmware(KernelPatcher &patcher, size_t index,
                          mach_vm_address_t address, size_t size);

    ChipFamily chipFamily;
    uint16_t   deviceId;

    // RDNA 2 module name to redirect to
    const char *redirectModuleName = nullptr;

    // State tracking
    bool patchesApplied  = false;
    bool hwlibsRedirected = false;

    // =========================================================================
    // GFX Register Mapping Tables
    // Maps GFX11 (RDNA 3) register offsets to GFX10.3 (RDNA 2) equivalents
    // where the register semantics are compatible.
    // =========================================================================

    struct RegisterMapping {
        uint32_t gfx11Offset;    // RDNA 3 register offset
        uint32_t gfx10_3Offset;  // RDNA 2 register offset
        const char *name;        // Register name for debugging
        bool compatible;         // true if semantics are the same
    };

    static const RegisterMapping kRegisterMappings[];
    static const size_t kRegisterMappingCount;
};

#endif // DKZKEXT_HWLIBS_HPP
