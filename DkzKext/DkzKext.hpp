// DkzKext — AMD Radeon RX 7000 (RDNA 3) Support for macOS
// DkzKext.hpp — Main kext class header
//
// Copyright © 2026 DKZ. All rights reserved.

#ifndef DKZKEXT_HPP
#define DKZKEXT_HPP

#include <Headers/kern_api.hpp>
#include <Headers/kern_patcher.hpp>
#include <Headers/kern_devinfo.hpp>
#include <Headers/kern_iokit.hpp>
#include <IOKit/pci/IOPCIDevice.h>
#include "NaVi3x.hpp"
#include "PatcherPlus.hpp"

// Forward declarations
class DkzFramebuffer;
class DkzHWLibs;
class DkzPowerPlay;

// =============================================================================
// DkzKext — Main Controller Class
// =============================================================================

class DkzKextController {
public:
    // Singleton callback instance
    static DkzKextController callback;

    /**
     * Initialize the kext — register Lilu callbacks.
     * Called from kern_start.cpp on plugin load.
     */
    void init();

    /**
     * De-initialize and cleanup.
     */
    void deinit();

    // =========================================================================
    // State Accessors
    // =========================================================================

    ChipFamily     getChipFamily()       const { return chipFamily; }
    uint16_t       getDeviceId()         const { return deviceId; }
    uint16_t       getSpoofDeviceId()    const { return spoofDeviceId; }
    const char*    getGPUModel()         const { return gpuModel; }
    bool           isSpoofEnabled()      const { return spoofEnabled; }
    IOPCIDevice*   getGPUDevice()        const { return gpuDevice; }

private:
    // =========================================================================
    // Lilu Callbacks
    // =========================================================================

    /**
     * Called when device info is available (PCI enumeration complete).
     * Used to detect the RDNA 3 GPU and read its Device ID.
     */
    static void onDeviceInfo(void *user, DeviceInfo *info);

    /**
     * Called when kexts are loaded — this is where we apply patches.
     * @param patcher  Kernel patcher reference
     * @param index    Kext index in the load list
     * @param address  Base address of loaded kext
     * @param size     Size of loaded kext
     */
    static void onKextLoad(void *user, KernelPatcher &patcher, size_t index,
                           mach_vm_address_t address, size_t size);

    // =========================================================================
    // GPU Detection & Setup
    // =========================================================================

    /**
     * Detect the RDNA 3 GPU from PCI device tree.
     * @param info  Device info from Lilu
     * @return true if an RDNA 3 GPU was found
     */
    bool detectGPU(DeviceInfo *info);

    /**
     * Setup Device ID spoofing — write the RDNA 2 Device ID
     * to the IORegistry so AMD drivers will claim the device.
     */
    void setupDeviceIdSpoof();

    /**
     * Inject device properties into the IORegistry.
     * Sets model name, compatible strings, etc.
     */
    void injectDeviceProperties();

    // =========================================================================
    // Kext Patching
    // =========================================================================

    /**
     * Apply patches to AMDRadeonX6000.kext
     * Main accelerator driver — Device ID matching and initialization.
     */
    void patchX6000(KernelPatcher &patcher, size_t index,
                    mach_vm_address_t address, size_t size);

    /**
     * Apply patches to AMDRadeonX6000Framebuffer.kext
     * Display controller — framebuffer, connectors, display output.
     */
    void patchX6000Framebuffer(KernelPatcher &patcher, size_t index,
                                mach_vm_address_t address, size_t size);

    /**
     * Apply patches to AMDRadeonX6000HWServices.kext
     * Hardware services — HWLibs loading, firmware init.
     */
    void patchX6000HWServices(KernelPatcher &patcher, size_t index,
                               mach_vm_address_t address, size_t size);

    /**
     * Apply patches to AMDSupport.kext
     * Support framework — model detection, capabilities.
     */
    void patchAMDSupport(KernelPatcher &patcher, size_t index,
                          mach_vm_address_t address, size_t size);

    // =========================================================================
    // Hooked Functions
    // =========================================================================

    // AMDRadeonX6000 hooks
    static bool wrapAcceleratorStart(void *that, IOService *provider);
    static void *wrapCreateAccelChannels(void *that, bool
    
    
    
    );
    static uint64_t wrapGetHWInfo(void *that, uint32_t param1);

    // AMDRadeonX6000Framebuffer hooks
    static bool wrapFBStart(void *that, IOService *provider);
    static void wrapFBSetupConnectors(void *that);
    static uint32_t wrapFBGetConnectorCount(void *that);

    // AMDRadeonX6000HWServices hooks
    static bool wrapHWServicesStart(void *that, IOService *provider);
    static void *wrapGetHWLibsModule(void *that);
    static bool wrapInitHWLibs(void *that, void *param1);

    // AMDSupport hooks
    static const char *wrapGetModelName(void *that);
    static uint32_t wrapGetDeviceId(void *that);

    // =========================================================================
    // Original Function Pointers (saved by Lilu routing)
    // =========================================================================

    static bool     (*origAcceleratorStart)(void*, IOService*);
    static void*    (*origCreateAccelChannels)(void*, bool);
    static uint64_t (*origGetHWInfo)(void*, uint32_t);
    static bool     (*origFBStart)(void*, IOService*);
    static void     (*origFBSetupConnectors)(void*);
    static uint32_t (*origFBGetConnectorCount)(void*);
    static bool     (*origHWServicesStart)(void*, IOService*);
    static void*    (*origGetHWLibsModule)(void*);
    static bool     (*origInitHWLibs)(void*, void*);
    static const char* (*origGetModelName)(void*);
    static uint32_t (*origGetDeviceId)(void*);

    // =========================================================================
    // Kext Info for Lilu
    // =========================================================================

    static constexpr size_t kKextCount = 4;

    // Indices into kextList
    enum KextIndex : size_t {
        KextX6000           = 0,
        KextX6000Framebuffer = 1,
        KextX6000HWServices = 2,
        KextAMDSupport      = 3,
    };

    static const char *kextPathX6000[1];
    static const char *kextPathX6000FB[1];
    static const char *kextPathX6000HW[1];
    static const char *kextPathAMDSupport[1];

    KernelPatcher::KextInfo kextList[kKextCount] = {
        {"com.apple.kext.AMDRadeonX6000",
         kextPathX6000, arrsize(kextPathX6000),
         {}, {}, KernelPatcher::KextInfo::Unloaded},

        {"com.apple.kext.AMDRadeonX6000Framebuffer",
         kextPathX6000FB, arrsize(kextPathX6000FB),
         {}, {}, KernelPatcher::KextInfo::Unloaded},

        {"com.apple.kext.AMDRadeonX6000HWServices",
         kextPathX6000HW, arrsize(kextPathX6000HW),
         {}, {}, KernelPatcher::KextInfo::Unloaded},

        {"com.apple.AMDSupport",
         kextPathAMDSupport, arrsize(kextPathAMDSupport),
         {}, {}, KernelPatcher::KextInfo::Unloaded},
    };

    // =========================================================================
    // Instance State
    // =========================================================================

    bool           initialized     = false;
    bool           spoofEnabled    = true;
    bool           debugEnabled    = false;

    ChipFamily     chipFamily      = ChipFamilyUnknown;
    uint16_t       deviceId        = 0;
    uint16_t       spoofDeviceId   = 0;
    uint16_t       revisionId      = 0;
    const char    *gpuModel        = "Unknown AMD GPU";

    IOPCIDevice   *gpuDevice       = nullptr;

    // Sub-modules
    DkzFramebuffer *framebuffer    = nullptr;
    DkzHWLibs      *hwlibs         = nullptr;
    DkzPowerPlay   *powerPlay      = nullptr;
};

#endif // DKZKEXT_HPP
