// DkzKext — AMD Radeon RX 7000 (RDNA 3) Support for macOS
// PatcherPlus.hpp — Enhanced binary patching utilities for Lilu-based kext development
//
// Copyright © 2026 DKZ. All rights reserved.
// Provides helper classes and macros for applying binary patches to AMD kexts.

#ifndef DKZKEXT_PATCHERPLUS_HPP
#define DKZKEXT_PATCHERPLUS_HPP

#include <Headers/kern_api.hpp>
#include <Headers/kern_util.hpp>
#include <Headers/kern_patcher.hpp>
#include "../Headers/kern_util.hpp"

// =============================================================================
// Route Request Helper — Simplified function routing
// =============================================================================

/**
 * SolveRequestPlus — Extended solve request that supports fallback symbols.
 * Used to find function addresses in kexts even when symbols change between
 * macOS versions.
 */
struct SolveRequestPlus {
    const char   *symbol;           // Primary symbol name
    const char   *fallbackSymbol;   // Fallback symbol if primary not found
    void        **address;          // Output: resolved address
    bool          required;         // If true, fail if not found
    size_t        offset;           // Offset from symbol start

    SolveRequestPlus(const char *sym, void **addr, bool req = true)
        : symbol(sym), fallbackSymbol(nullptr), address(addr),
          required(req), offset(0) {}

    SolveRequestPlus(const char *sym, const char *fallback, void **addr, bool req = true)
        : symbol(sym), fallbackSymbol(fallback), address(addr),
          required(req), offset(0) {}

    SolveRequestPlus withOffset(size_t off) {
        this->offset = off;
        return *this;
    }
};

/**
 * RouteRequestPlus — Extended route request that supports original function
 * storage and conditional routing based on chip family.
 */
struct RouteRequestPlus {
    const char  *symbol;            // Symbol to hook
    const void  *replacement;       // Replacement function
    void        *original;          // Original function pointer storage
    bool         required;          // If true, fail if symbol not found
    uint32_t     chipFamilyMask;    // Bitmask: which chip families this applies to

    static constexpr uint32_t kAllChips = 0xFFFFFFFF;
    static constexpr uint32_t kNavi31   = (1 << 1);
    static constexpr uint32_t kNavi32   = (1 << 2);
    static constexpr uint32_t kNavi33   = (1 << 3);

    RouteRequestPlus(const char *sym, const void *repl, void *orig = nullptr,
                     bool req = true, uint32_t chipMask = kAllChips)
        : symbol(sym), replacement(repl), original(orig),
          required(req), chipFamilyMask(chipMask) {}

    bool appliesToChip(uint32_t chipFamily) const {
        return (chipFamilyMask & (1 << chipFamily)) != 0;
    }
};

// =============================================================================
// Binary Pattern Matching
// =============================================================================

struct BinaryPattern {
    const uint8_t *pattern;
    const uint8_t *mask;        // 0xFF = exact match, 0x00 = wildcard
    size_t         length;
    const char    *description;
};

/**
 * Find a binary pattern in a memory region.
 * @param haystack    Memory region to search
 * @param haystackLen Length of the memory region
 * @param pattern     Pattern to search for
 * @return Pointer to the pattern location, or nullptr if not found
 */
static inline const void* findPattern(const void *haystack, size_t haystackLen,
                                       const BinaryPattern &pattern) {
    if (!haystack || haystackLen < pattern.length) return nullptr;

    const uint8_t *data = static_cast<const uint8_t*>(haystack);
    for (size_t i = 0; i <= haystackLen - pattern.length; i++) {
        bool match = true;
        for (size_t j = 0; j < pattern.length; j++) {
            if (pattern.mask && pattern.mask[j] == 0x00) continue;
            if (data[i + j] != pattern.pattern[j]) {
                match = false;
                break;
            }
        }
        if (match) return &data[i];
    }
    return nullptr;
}

// =============================================================================
// Patcher Utility Functions
// =============================================================================

class PatcherPlus {
public:
    /**
     * Solve multiple symbol requests with fallback support.
     * @param patcher    KernelPatcher reference
     * @param index      Kext index
     * @param requests   Array of SolveRequestPlus
     * @param count      Number of requests
     * @return true if all required symbols were resolved
     */
    static bool solveMultiple(KernelPatcher &patcher, size_t index,
                              SolveRequestPlus *requests, size_t count) {
        bool allResolved = true;
        for (size_t i = 0; i < count; i++) {
            auto &req = requests[i];
            auto addr = patcher.solveSymbol(index, req.symbol);
            if (!addr && req.fallbackSymbol) {
                addr = patcher.solveSymbol(index, req.fallbackSymbol);
            }
            if (addr) {
                *req.address = reinterpret_cast<void*>(addr + req.offset);
                DKZDBG("Resolved symbol: %s at %p (+0x%lx)",
                       req.symbol, addr, req.offset);
            } else {
                *req.address = nullptr;
                if (req.required) {
                    DKZERR("Failed to resolve required symbol: %s", req.symbol);
                    allResolved = false;
                } else {
                    DKZWARN("Optional symbol not found: %s", req.symbol);
                }
            }
        }
        return allResolved;
    }

    /**
     * Route multiple function calls with chip family filtering.
     * @param patcher    KernelPatcher reference
     * @param index      Kext index
     * @param requests   Array of RouteRequestPlus
     * @param count      Number of requests
     * @param chipFamily Current chip family
     * @return true if all required routes were applied
     */
    static bool routeMultiple(KernelPatcher &patcher, size_t index,
                              RouteRequestPlus *requests, size_t count,
                              uint32_t chipFamily) {
        bool allRouted = true;
        for (size_t i = 0; i < count; i++) {
            auto &req = requests[i];
            if (!req.appliesToChip(chipFamily)) {
                DKZDBG("Skipping route %s (not applicable to chip family %u)",
                       req.symbol, chipFamily);
                continue;
            }

            if (req.original) {
                KernelPatcher::RouteRequest route(
                    req.symbol,
                    req.replacement,
                    *static_cast<mach_vm_address_t*>(req.original)
                );
                if (!patcher.routeMultiple(index, &route, 1)) {
                    if (req.required) {
                        DKZERR("Failed to route required symbol: %s", req.symbol);
                        allRouted = false;
                    } else {
                        DKZWARN("Failed to route optional symbol: %s", req.symbol);
                    }
                }
            } else {
                KernelPatcher::RouteRequest route(
                    req.symbol,
                    req.replacement
                );
                if (!patcher.routeMultiple(index, &route, 1)) {
                    if (req.required) {
                        DKZERR("Failed to route required symbol: %s", req.symbol);
                        allRouted = false;
                    } else {
                        DKZWARN("Failed to route optional symbol: %s", req.symbol);
                    }
                } else {
                    DKZDBG("Successfully routed: %s", req.symbol);
                }
            }
        }
        return allRouted;
    }

    /**
     * Apply a binary patch at a specific address.
     * @param patcher   KernelPatcher reference
     * @param address   Address to patch
     * @param find      Original bytes
     * @param replace   Replacement bytes
     * @param size      Size of patch
     * @return true if patch was applied successfully
     */
    static bool applyBinaryPatch(KernelPatcher &patcher, mach_vm_address_t address,
                                  const uint8_t *find, const uint8_t *replace,
                                  size_t size) {
        // Verify current content matches expected
        auto *current = reinterpret_cast<const uint8_t*>(address);
        for (size_t i = 0; i < size; i++) {
            if (current[i] != find[i]) {
                DKZERR("Binary patch mismatch at offset %lu: expected 0x%02X, got 0x%02X",
                       i, find[i], current[i]);
                return false;
            }
        }

        // Apply the patch
        auto result = MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock);
        if (result != KERN_SUCCESS) {
            DKZERR("Failed to enable kernel writing: %d", result);
            return false;
        }

        auto *target = reinterpret_cast<uint8_t*>(address);
        for (size_t i = 0; i < size; i++) {
            target[i] = replace[i];
        }

        MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);
        DKZDBG("Binary patch applied at %p (%lu bytes)", (void*)address, size);
        return true;
    }
};

// =============================================================================
// Convenience Macros for Patch Definitions
// =============================================================================

// Define a static member function hook
#define DKZHOOK_STATIC(cls, name) \
    static void name(void *that)

// Define a member function hook with original storage
#define DKZHOOK_MEMBER(cls, name, ret, ...) \
    static ret hooked_##name(void *that, ##__VA_ARGS__); \
    static ret (*original_##name)(void *that, ##__VA_ARGS__)

// Create a route request
#define DKZROUTE(sym, hook, orig) \
    RouteRequestPlus(sym, reinterpret_cast<const void*>(hook), &orig)

// Create an optional route request
#define DKZROUTE_OPT(sym, hook, orig) \
    RouteRequestPlus(sym, reinterpret_cast<const void*>(hook), &orig, false)

#endif // DKZKEXT_PATCHERPLUS_HPP
