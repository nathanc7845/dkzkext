// DkzKext — AMD Radeon RX 7000 (RDNA 3) Support for macOS
// Framebuffer.hpp — Display output controller: connector definitions and
//                   framebuffer patching for RDNA 3.
//
// Copyright © 2026 DKZ. All rights reserved.

#ifndef DKZKEXT_FRAMEBUFFER_HPP
#define DKZKEXT_FRAMEBUFFER_HPP

#include <Headers/kern_patcher.hpp>
#include <IOKit/IOService.h>
#include "../NaVi3x.hpp"

// =============================================================================
// DkzFramebuffer — Display Output Controller
// =============================================================================

class DkzFramebuffer {
public:
    DkzFramebuffer(ChipFamily family, uint16_t deviceId);
    ~DkzFramebuffer() = default;

    /**
     * Apply framebuffer-related binary patches to AMDRadeonX6000Framebuffer.kext.
     */
    void applyPatches(KernelPatcher &patcher, size_t index,
                      mach_vm_address_t address, size_t size);

    /**
     * Inject connector data into the IORegistry before framebuffer starts.
     * @param provider  The IOService provider (GPU device)
     */
    void injectConnectorData(IOService *provider);

    /**
     * Override connector setup after the framebuffer's own setup runs.
     * @param fbController  Pointer to the framebuffer controller instance
     */
    void overrideConnectors(void *fbController);

    /**
     * Get the number of display connectors for this GPU.
     */
    uint32_t getConnectorCount() const;

    /**
     * Get the framebuffer info for the current GPU.
     */
    const FramebufferInfo* getFramebufferInfo() const { return currentFB; }

private:
    /**
     * Build the framebuffer info table for the detected GPU.
     */
    void buildFramebufferInfo();

    /**
     * Get the default framebuffer info for a chip family.
     */
    static const FramebufferInfo* getDefaultFramebuffer(ChipFamily family);

    ChipFamily          chipFamily;
    uint16_t            deviceId;
    const FramebufferInfo *currentFB;

    // =========================================================================
    // Default Framebuffer Definitions
    // =========================================================================

    // Navi 31 (RX 7900 XTX / 7900 XT)
    // Typical: 2x DP 2.1 + 1x HDMI 2.1 + 1x USB-C
    static const FramebufferInfo kFBNavi31;

    // Navi 32 (RX 7800 XT / 7700 XT)
    // Typical: 2x DP 2.1 + 1x HDMI 2.1
    static const FramebufferInfo kFBNavi32;

    // Navi 33 (RX 7600)
    // Typical: 2x DP 2.1 + 1x HDMI 2.1
    static const FramebufferInfo kFBNavi33;
};

#endif // DKZKEXT_FRAMEBUFFER_HPP
