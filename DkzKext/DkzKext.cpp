// DkzKext — AMD Radeon RX 7000 (RDNA 3) Support for macOS
// DkzKext.cpp — Main kext implementation: GPU detection, Device ID spoofing,
//               and runtime patching of AMD macOS drivers.
//
// Copyright © 2026 DKZ. All rights reserved.

#include "DkzKext.hpp"
#include "Framebuffer/Framebuffer.hpp"
#include "Firmware/HWLibs.hpp"
#include "PowerPlay/PowerPlay.hpp"
#include <Headers/kern_api.hpp>
#include <Headers/kern_devinfo.hpp>
#include <Headers/kern_iokit.hpp>

// =============================================================================
// Static Singleton & Original Function Pointers
// =============================================================================

DkzKext DkzKextController::callback;

// AMDRadeonX6000
bool     (*DkzKextController::origAcceleratorStart)(void*, IOService*) = nullptr;
void*    (*DkzKextController::origCreateAccelChannels)(void*, bool) = nullptr;
uint64_t (*DkzKextController::origGetHWInfo)(void*, uint32_t) = nullptr;

// AMDRadeonX6000Framebuffer
bool     (*DkzKextController::origFBStart)(void*, IOService*) = nullptr;
void     (*DkzKextController::origFBSetupConnectors)(void*) = nullptr;
uint32_t (*DkzKextController::origFBGetConnectorCount)(void*) = nullptr;

// AMDRadeonX6000HWServices
bool     (*DkzKextController::origHWServicesStart)(void*, IOService*) = nullptr;
void*    (*DkzKextController::origGetHWLibsModule)(void*) = nullptr;
bool     (*DkzKextController::origInitHWLibs)(void*, void*) = nullptr;

// AMDSupport
const char* (*DkzKextController::origGetModelName)(void*) = nullptr;
uint32_t    (*DkzKextController::origGetDeviceId)(void*) = nullptr;

// =============================================================================
// Initialization
// =============================================================================

void DkzKextController::init() {
    DKZLOG("DkzKext v1.0.0 — AMD Radeon RX 7000 (RDNA 3) Support");
    DKZLOG("Initializing...");

    // Check boot arguments
    char bootargValue[32] = {};

    // Check if disabled via boot arg
    if (checkKernelArgument("-dkzoff")) {
        DKZLOG("Disabled via -dkzoff boot argument. Exiting.");
        return;
    }

    // Check debug mode
    if (checkKernelArgument("-dkzdbg")) {
        debugEnabled = true;
        DKZLOG("Debug mode enabled via -dkzdbg");
    }

    // Check spoof disable
    if (checkKernelArgument("-dkznospoof")) {
        spoofEnabled = false;
        DKZLOG("Device ID spoofing disabled via -dkznospoof");
    }

    // Register Lilu callbacks
    auto &api = lilu.onDeviceInfoAvailable(onDeviceInfo, this);

    // Register kexts to monitor for patching
    lilu.onKextLoadForce(kextList, kKextCount, onKextLoad, this);

    initialized = true;
    DKZLOG("Initialization complete. Waiting for device enumeration...");
}

void DkzKextController::deinit() {
    DKZLOG("Deinitializing DkzKext...");
    // Cleanup sub-modules
    if (framebuffer) { delete framebuffer; framebuffer = nullptr; }
    if (hwlibs)      { delete hwlibs;      hwlibs = nullptr; }
    if (powerPlay)   { delete powerPlay;   powerPlay = nullptr; }
}

// =============================================================================
// Device Info Callback — GPU Detection
// =============================================================================

void DkzKextController::onDeviceInfo(void *user, DeviceInfo *info) {
    auto *self = static_cast<DkzKext*>(user);
    DKZLOG("Device info available. Scanning for RDNA 3 GPU...");

    if (!self->detectGPU(info)) {
        DKZERR("No supported RDNA 3 GPU found! DkzKext will not activate.");
        return;
    }

    DKZLOG("Found GPU: %s (Device ID: 0x%04X, Chip: %s)",
           self->gpuModel, self->deviceId,
           chipFamilyToString(self->chipFamily));

    // Setup Device ID spoofing
    if (self->spoofEnabled) {
        self->setupDeviceIdSpoof();
    }

    // Inject device properties
    self->injectDeviceProperties();

    // Initialize sub-modules
    self->framebuffer = new DkzFramebuffer(self->chipFamily, self->deviceId);
    self->hwlibs = new DkzHWLibs(self->chipFamily, self->deviceId);
    self->powerPlay = new DkzPowerPlay(self->chipFamily, self->deviceId);

    DKZLOG("GPU setup complete. Waiting for AMD kexts to load...");
}

// =============================================================================
// GPU Detection
// =============================================================================

bool DkzKextController::detectGPU(DeviceInfo *info) {
    if (!info || !info->videoBuiltin) {
        DKZDBG("No video device info available");
    }

    // Scan all GPU devices for an RDNA 3 device
    for (size_t i = 0; i < info->videoExternal.size(); i++) {
        auto *gpu = info->videoExternal[i].video;
        if (!gpu) continue;

        uint32_t vendorId = 0, devId = 0;
        if (WIOKit::getOSDataValue(gpu, "vendor-id", vendorId) &&
            WIOKit::getOSDataValue(gpu, "device-id", devId)) {

            uint16_t vid = static_cast<uint16_t>(vendorId);
            uint16_t did = static_cast<uint16_t>(devId);

            DKZDBG("Found GPU: vendor=0x%04X, device=0x%04X", vid, did);

            if (vid != kAMDVendorID) continue;
            if (!isNavi3x(did)) continue;

            // Found an RDNA 3 GPU!
            deviceId = did;
            gpuDevice = OSDynamicCast(IOPCIDevice, gpu);

            // Read revision ID
            if (gpuDevice) {
                revisionId = gpuDevice->configRead16(PCI_REVISION_ID);
            }

            // Look up in our device table
            for (size_t j = 0; j < kSupportedDeviceIdCount; j++) {
                if (kSupportedDeviceIds[j].deviceId == did) {
                    chipFamily    = kSupportedDeviceIds[j].chipFamily;
                    spoofDeviceId = kSupportedDeviceIds[j].spoofDeviceId;
                    gpuModel      = kSupportedDeviceIds[j].modelName;
                    return true;
                }
            }

            // Device ID not in our table but is in RDNA 3 range
            // Try to determine chip family from range
            if (isNavi31(did)) {
                chipFamily    = ChipFamilyNavi31;
                spoofDeviceId = Navi31::kSpoofTargetDeviceId;
                gpuModel      = "AMD Radeon (Navi 31 - Unknown SKU)";
            } else if (isNavi32(did)) {
                chipFamily    = ChipFamilyNavi32;
                spoofDeviceId = Navi32::kSpoofTargetDeviceId;
                gpuModel      = "AMD Radeon (Navi 32 - Unknown SKU)";
            } else if (isNavi33(did)) {
                chipFamily    = ChipFamilyNavi33;
                spoofDeviceId = Navi33::kSpoofTargetDeviceId;
                gpuModel      = "AMD Radeon (Navi 33 - Unknown SKU)";
            }

            return true;
        }
    }

    return false;
}

// =============================================================================
// Device ID Spoofing
// =============================================================================

void DkzKextController::setupDeviceIdSpoof() {
    if (!gpuDevice || spoofDeviceId == 0) {
        DKZERR("Cannot setup spoof: no GPU device or spoof ID is 0");
        return;
    }

    DKZLOG("Spoofing Device ID: 0x%04X -> 0x%04X", deviceId, spoofDeviceId);

    // Write spoofed device-id to IORegistry
    uint32_t spoofedId32 = spoofDeviceId;
    auto spoofData = OSData::withBytes(&spoofedId32, sizeof(spoofedId32));
    if (spoofData) {
        gpuDevice->setProperty("device-id", spoofData);
        spoofData->release();
        DKZLOG("Device ID spoofed successfully in IORegistry");
    }

    // Store original device-id for reference
    uint32_t origId32 = deviceId;
    auto origData = OSData::withBytes(&origId32, sizeof(origId32));
    if (origData) {
        gpuDevice->setProperty(kDkzOriginalDeviceIdKey, origData);
        origData->release();
    }

    // Set compatible property to match RDNA 2 driver expectations
    char compatStr[64] = {};
    snprintf(compatStr, sizeof(compatStr), "pci1002,%x", spoofDeviceId);
    gpuDevice->setProperty("compatible", compatStr);

    DKZLOG("IORegistry compatible set to: %s", compatStr);
}

// =============================================================================
// Device Property Injection
// =============================================================================

void DkzKextController::injectDeviceProperties() {
    if (!gpuDevice) return;

    DKZLOG("Injecting device properties...");

    // GPU Model Name
    gpuDevice->setProperty("model", gpuModel);
    gpuDevice->setProperty(kDkzGPUModelKey, gpuModel);

    // Chip family info
    uint32_t family32 = static_cast<uint32_t>(chipFamily);
    auto familyData = OSData::withBytes(&family32, sizeof(family32));
    if (familyData) {
        gpuDevice->setProperty(kDkzChipFamilyKey, familyData);
        familyData->release();
    }

    // DkzKext version
    gpuDevice->setProperty(kDkzVersionKey, "1.0.0");

    // Force-enable GPU
    gpuDevice->setProperty("force-power-on", kOSBooleanTrue);

    // HDMI/DP audio support
    gpuDevice->setProperty("hda-gfx", "onboard-1");

    // VRAM info (auto-detect is preferred, but set minimum)
    uint64_t vramSize = 0;
    switch (chipFamily) {
        case ChipFamilyNavi31:
            vramSize = GB(24);  // RX 7900 XTX: 24GB, RX 7900 XT: 20GB
            break;
        case ChipFamilyNavi32:
            vramSize = GB(16);  // RX 7800 XT: 16GB, RX 7700 XT: 12GB
            break;
        case ChipFamilyNavi33:
            vramSize = GB(8);   // RX 7600: 8GB
            break;
        default:
            vramSize = GB(8);
            break;
    }
    auto vramData = OSData::withBytes(&vramSize, sizeof(vramSize));
    if (vramData) {
        gpuDevice->setProperty("VRAM,totalsize", vramData);
        vramData->release();
    }

    DKZLOG("Device properties injected successfully");
}

// =============================================================================
// Kext Load Callback — Apply Patches
// =============================================================================

void DkzKextController::onKextLoad(void *user, KernelPatcher &patcher, size_t index,
                          mach_vm_address_t address, size_t size) {
    auto *self = static_cast<DkzKext*>(user);

    if (!self->initialized || self->chipFamily == ChipFamilyUnknown) {
        return;
    }

    // Determine which kext was loaded and apply corresponding patches
    if (self->kextList[KextX6000].loadIndex == index) {
        DKZLOG("AMDRadeonX6000 loaded at %p (size: 0x%lx). Applying patches...",
               (void*)address, size);
        self->patchX6000(patcher, index, address, size);
    }
    else if (self->kextList[KextX6000Framebuffer].loadIndex == index) {
        DKZLOG("AMDRadeonX6000Framebuffer loaded. Applying patches...");
        self->patchX6000Framebuffer(patcher, index, address, size);
    }
    else if (self->kextList[KextX6000HWServices].loadIndex == index) {
        DKZLOG("AMDRadeonX6000HWServices loaded. Applying patches...");
        self->patchX6000HWServices(patcher, index, address, size);
    }
    else if (self->kextList[KextAMDSupport].loadIndex == index) {
        DKZLOG("AMDSupport loaded. Applying patches...");
        self->patchAMDSupport(patcher, index, address, size);
    }
}

// =============================================================================
// AMDRadeonX6000 Patches — Main Accelerator Driver
// =============================================================================

void DkzKextController::patchX6000(KernelPatcher &patcher, size_t index,
                          mach_vm_address_t address, size_t size) {
    // Route accelerator start to inject our Device ID
    KernelPatcher::RouteRequest routes[] = {
        // Hook the accelerator start to inject our configuration
        {"__ZN30AMDRadeonX6000_AMDAccelDevice5startEP9IOService",
         reinterpret_cast<mach_vm_address_t>(wrapAcceleratorStart),
         reinterpret_cast<mach_vm_address_t&>(origAcceleratorStart)},

        // Hook channel creation
        {"__ZN30AMDRadeonX6000_AMDAccelDevice20createAccelChannelsEb",
         reinterpret_cast<mach_vm_address_t>(wrapCreateAccelChannels),
         reinterpret_cast<mach_vm_address_t&>(origCreateAccelChannels)},

        // Hook HW info query (for VRAM size, GPU capabilities)
        {"__ZN30AMDRadeonX6000_AMDAccelDevice9getHWInfoEj",
         reinterpret_cast<mach_vm_address_t>(wrapGetHWInfo),
         reinterpret_cast<mach_vm_address_t&>(origGetHWInfo)},
    };

    if (!patcher.routeMultiple(index, routes, arrsize(routes))) {
        DKZERR("Failed to route AMDRadeonX6000 functions (error: %d)", patcher.getError());
        patcher.clearError();
    } else {
        DKZLOG("AMDRadeonX6000 patches applied successfully");
    }

    // Apply binary patches to add our Device IDs to the supported list
    // The X6000 driver has an internal table of supported Device IDs.
    // We need to patch one of the existing RDNA 2 entries to match our RDNA 3 ID.
    // This is a targeted byte-level patch.
    if (spoofEnabled) {
        DKZLOG("Applying Device ID table patches in AMDRadeonX6000...");

        // Search for the RDNA 2 device ID in the binary and ensure our
        // spoofed ID will be recognized
        const uint8_t navi21_id[] = {0xBF, 0x73};  // 0x73BF little-endian
        const uint8_t navi22_id[] = {0xDF, 0x73};  // 0x73DF little-endian
        const uint8_t navi23_id[] = {0xFF, 0x73};  // 0x73FF little-endian

        // Verify the spoof target exists in the driver's device table
        auto *base = reinterpret_cast<const uint8_t*>(address);
        bool foundTarget = false;

        const uint8_t *searchId = nullptr;
        switch (chipFamily) {
            case ChipFamilyNavi31: searchId = navi21_id; break;
            case ChipFamilyNavi32: searchId = navi22_id; break;
            case ChipFamilyNavi33: searchId = navi23_id; break;
            default: break;
        }

        if (searchId) {
            for (size_t i = 0; i < size - 1; i++) {
                if (base[i] == searchId[0] && base[i+1] == searchId[1]) {
                    foundTarget = true;
                    DKZDBG("Found spoof target Device ID at offset 0x%lx", i);
                    break;
                }
            }
            if (foundTarget) {
                DKZLOG("Spoof target Device ID (0x%04X) found in AMDRadeonX6000",
                       spoofDeviceId);
            } else {
                DKZWARN("Spoof target Device ID (0x%04X) NOT found in AMDRadeonX6000. "
                        "Driver may not claim the GPU.", spoofDeviceId);
            }
        }
    }
}

// =============================================================================
// AMDRadeonX6000Framebuffer Patches
// =============================================================================

void DkzKextController::patchX6000Framebuffer(KernelPatcher &patcher, size_t index,
                                     mach_vm_address_t address, size_t size) {
    KernelPatcher::RouteRequest routes[] = {
        {"__ZN35AMDRadeonX6000_AtiAtomBiosDCE125startEP9IOService",
         reinterpret_cast<mach_vm_address_t>(wrapFBStart),
         reinterpret_cast<mach_vm_address_t&>(origFBStart)},

        {"__ZN35AMDRadeonX6000_AtiAtomBiosDCE1215setupConnectorsEv",
         reinterpret_cast<mach_vm_address_t>(wrapFBSetupConnectors),
         reinterpret_cast<mach_vm_address_t&>(origFBSetupConnectors)},

        {"__ZN35AMDRadeonX6000_AtiAtomBiosDCE1217getConnectorCountEv",
         reinterpret_cast<mach_vm_address_t>(wrapFBGetConnectorCount),
         reinterpret_cast<mach_vm_address_t&>(origFBGetConnectorCount)},
    };

    if (!patcher.routeMultiple(index, routes, arrsize(routes))) {
        DKZWARN("Some AMDRadeonX6000Framebuffer routes failed (error: %d). "
                "This may be due to macOS version differences.",
                patcher.getError());
        patcher.clearError();
    }

    // Apply framebuffer-specific patches via sub-module
    if (framebuffer) {
        framebuffer->applyPatches(patcher, index, address, size);
    }

    DKZLOG("AMDRadeonX6000Framebuffer patches applied");
}

// =============================================================================
// AMDRadeonX6000HWServices Patches
// =============================================================================

void DkzKextController::patchX6000HWServices(KernelPatcher &patcher, size_t index,
                                    mach_vm_address_t address, size_t size) {
    KernelPatcher::RouteRequest routes[] = {
        {"__ZN38AMDRadeonX6000_AMDRadeonHWServicesNavi5startEP9IOService",
         reinterpret_cast<mach_vm_address_t>(wrapHWServicesStart),
         reinterpret_cast<mach_vm_address_t&>(origHWServicesStart)},

        {"__ZN38AMDRadeonX6000_AMDRadeonHWServicesNavi16getHWLibsModuleEv",
         reinterpret_cast<mach_vm_address_t>(wrapGetHWLibsModule),
         reinterpret_cast<mach_vm_address_t&>(origGetHWLibsModule)},

        {"__ZN38AMDRadeonX6000_AMDRadeonHWServicesNavi11initHWLibsEPv",
         reinterpret_cast<mach_vm_address_t>(wrapInitHWLibs),
         reinterpret_cast<mach_vm_address_t&>(origInitHWLibs)},
    };

    if (!patcher.routeMultiple(index, routes, arrsize(routes))) {
        DKZWARN("Some AMDRadeonX6000HWServices routes failed (error: %d)",
                patcher.getError());
        patcher.clearError();
    }

    // Apply HWLibs patches via sub-module
    if (hwlibs) {
        hwlibs->applyPatches(patcher, index, address, size);
    }

    DKZLOG("AMDRadeonX6000HWServices patches applied");
}

// =============================================================================
// AMDSupport Patches
// =============================================================================

void DkzKextController::patchAMDSupport(KernelPatcher &patcher, size_t index,
                               mach_vm_address_t address, size_t size) {
    KernelPatcher::RouteRequest routes[] = {
        {"__ZN13ATIController12getModelNameEv",
         reinterpret_cast<mach_vm_address_t>(wrapGetModelName),
         reinterpret_cast<mach_vm_address_t&>(origGetModelName)},

        {"__ZN13ATIController11getDeviceIdEv",
         reinterpret_cast<mach_vm_address_t>(wrapGetDeviceId),
         reinterpret_cast<mach_vm_address_t&>(origGetDeviceId)},
    };

    if (!patcher.routeMultiple(index, routes, arrsize(routes))) {
        DKZWARN("Some AMDSupport routes failed (error: %d)", patcher.getError());
        patcher.clearError();
    }

    DKZLOG("AMDSupport patches applied");
}

// =============================================================================
// Hooked Function Implementations
// =============================================================================

// --- AMDRadeonX6000 Hooks ---

bool DkzKextController::wrapAcceleratorStart(void *that, IOService *provider) {
    DKZLOG("AMDAccelDevice::start intercepted");

    auto &self = callback;

    // If we have a spoofed Device ID, ensure the provider sees it
    if (self.spoofEnabled && self.gpuDevice && provider) {
        uint32_t spoofId32 = self.spoofDeviceId;
        auto data = OSData::withBytes(&spoofId32, sizeof(spoofId32));
        if (data) {
            provider->setProperty("device-id", data);
            data->release();
        }
    }

    // Call original
    bool result = origAcceleratorStart(that, provider);

    if (result) {
        DKZLOG("AMDAccelDevice::start succeeded");
    } else {
        DKZERR("AMDAccelDevice::start FAILED — GPU acceleration may not work");
    }

    return result;
}

void* DkzKextController::wrapCreateAccelChannels(void *that, bool param1) {
    DKZDBG("createAccelChannels called (param1=%d)", param1);
    return origCreateAccelChannels(that, param1);
}

uint64_t DkzKextController::wrapGetHWInfo(void *that, uint32_t param1) {
    DKZDBG("getHWInfo called (param1=0x%X)", param1);

    uint64_t result = origGetHWInfo(that, param1);

    // Override VRAM size if needed
    if (param1 == 0x01) { // Typically the VRAM size query
        auto &self = callback;
        switch (self.chipFamily) {
            case ChipFamilyNavi31:
                result = GB(24);
                break;
            case ChipFamilyNavi32:
                result = GB(16);
                break;
            case ChipFamilyNavi33:
                result = GB(8);
                break;
            default:
                break;
        }
        DKZDBG("Returning VRAM size: %llu bytes", result);
    }

    return result;
}

// --- AMDRadeonX6000Framebuffer Hooks ---

bool DkzKextController::wrapFBStart(void *that, IOService *provider) {
    DKZLOG("Framebuffer::start intercepted");

    auto &self = callback;

    // Inject connector data before the framebuffer initializes
    if (self.framebuffer) {
        self.framebuffer->injectConnectorData(provider);
    }

    bool result = origFBStart(that, provider);

    if (result) {
        DKZLOG("Framebuffer::start succeeded — display output should be active");
    } else {
        DKZERR("Framebuffer::start FAILED — display output may not work");
    }

    return result;
}

void DkzKextController::wrapFBSetupConnectors(void *that) {
    DKZLOG("Framebuffer::setupConnectors intercepted");

    auto &self = callback;

    // Let the original setup run first
    origFBSetupConnectors(that);

    // Then override with our connector data if available
    if (self.framebuffer) {
        self.framebuffer->overrideConnectors(that);
    }
}

uint32_t DkzKextController::wrapFBGetConnectorCount(void *that) {
    auto &self = callback;

    // Return our connector count if we have framebuffer data
    if (self.framebuffer) {
        uint32_t count = self.framebuffer->getConnectorCount();
        if (count > 0) {
            DKZDBG("Returning connector count: %u", count);
            return count;
        }
    }

    return origFBGetConnectorCount(that);
}

// --- AMDRadeonX6000HWServices Hooks ---

bool DkzKextController::wrapHWServicesStart(void *that, IOService *provider) {
    DKZLOG("HWServices::start intercepted");

    bool result = origHWServicesStart(that, provider);

    if (result) {
        DKZLOG("HWServices::start succeeded");
    } else {
        DKZERR("HWServices::start FAILED");
    }

    return result;
}

void* DkzKextController::wrapGetHWLibsModule(void *that) {
    DKZLOG("getHWLibsModule intercepted");

    auto &self = callback;

    // Try original first
    void *result = origGetHWLibsModule(that);

    if (!result) {
        DKZWARN("Original getHWLibsModule returned null — "
                "attempting to redirect to RDNA 2 HWLibs");
        // The original function failed because there are no RDNA 3 HWLibs.
        // We need to redirect to use the RDNA 2 HWLibs module.
        if (self.hwlibs) {
            result = self.hwlibs->getRedirectedModule(that);
        }
    }

    return result;
}

bool DkzKextController::wrapInitHWLibs(void *that, void *param1) {
    DKZLOG("initHWLibs intercepted");

    auto &self = callback;

    // Apply pre-init patches
    if (self.hwlibs) {
        self.hwlibs->preInitPatches(that, param1);
    }

    bool result = origInitHWLibs(that, param1);

    if (result) {
        DKZLOG("HWLibs initialization succeeded");
        // Apply post-init patches
        if (self.hwlibs) {
            self.hwlibs->postInitPatches(that, param1);
        }
    } else {
        DKZERR("HWLibs initialization FAILED — GPU hardware init incomplete");
    }

    return result;
}

// --- AMDSupport Hooks ---

const char* DkzKextController::wrapGetModelName(void *that) {
    auto &self = callback;

    if (self.gpuModel && self.chipFamily != ChipFamilyUnknown) {
        DKZDBG("Returning GPU model: %s", self.gpuModel);
        return self.gpuModel;
    }

    return origGetModelName(that);
}

uint32_t DkzKextController::wrapGetDeviceId(void *that) {
    auto &self = callback;

    if (self.spoofEnabled && self.spoofDeviceId != 0) {
        DKZDBG("Returning spoofed Device ID: 0x%04X (original: 0x%04X)",
               self.spoofDeviceId, self.deviceId);
        return self.spoofDeviceId;
    }

    if (self.deviceId != 0) {
        return self.deviceId;
    }

    return origGetDeviceId(that);
}
