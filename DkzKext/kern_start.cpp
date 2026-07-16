// DkzKext — AMD Radeon RX 7000 (RDNA 3) Support for macOS
// kern_start.cpp — Plugin entry point for Lilu framework
//
// Copyright © 2026 DKZ. All rights reserved.

#include <Headers/plugin_start.hpp>
#include <Headers/kern_api.hpp>
#include "DkzKext.hpp"

// =============================================================================
// Plugin Metadata
// =============================================================================

static const char *kextVersion = "1.0.0";

// Boot arguments to control DkzKext behavior
static const char *bootargOff     = "-dkzoff";      // Disable DkzKext entirely
static const char *bootargDebug   = "-dkzdbg";      // Enable debug logging
static const char *bootargBeta    = "-dkzbeta";      // Enable beta features
static const char *bootargNoSpoof = "-dkznospoof";   // Disable Device ID spoofing
static const char *bootargDump    = "-dkzdump";      // Dump GPU register state

// =============================================================================
// Plugin Definition
// =============================================================================

PluginConfiguration ADDPR(config) {
    xStringify(PRODUCT_NAME),           // Plugin name
    parseModuleVersion(kextVersion),    // Version
    LiluAPI::AllowNormal |
    LiluAPI::AllowInstallerRecovery |
    LiluAPI::AllowSafeMode,             // Permissions
    bootargOff,                         // Boot arg to disable
    arrsize(bootargOff),
    bootargDebug,                       // Boot arg for debug
    arrsize(bootargDebug),
    nullptr,                            // No minimum kernel version
    nullptr,                            // No maximum kernel version
    KernelVersion::Ventura,             // Min: macOS 13.0 Ventura
    KernelVersion::Tahoe,               // Max: macOS 26.x Tahoe
    []() {
        DkzKext::callback.init();
    }
};
