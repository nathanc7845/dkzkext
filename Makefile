# DkzKext Makefile
# AMD Radeon RX 7000 (RDNA 3) Support for macOS
#
# Usage:
#   make debug       - Build debug version
#   make release     - Build release version
#   make clean       - Clean build artifacts
#   make package     - Create distributable .zip
#
# Required environment variables:
#   LILU_PATH         - Path to Lilu source/headers
#   MACKERNELSDK_PATH - Path to MacKernelSDK

# =============================================================================
# Configuration
# =============================================================================

PRODUCT_NAME    = DkzKext
BUNDLE_ID       = com.dkz.DkzKext
VERSION         = 1.0.0

# Paths (override with environment variables)
LILU_PATH       ?= ../Lilu
MACKERNELSDK_PATH ?= ../MacKernelSDK

# Build output
BUILD_DIR       = build
DEBUG_DIR       = $(BUILD_DIR)/Debug
RELEASE_DIR     = $(BUILD_DIR)/Release
KEXT_NAME       = $(PRODUCT_NAME).kext

# Compiler
CXX             = clang++
KEXT_SDK        = -isysroot $(shell xcrun --sdk macosx --show-sdk-path)

# =============================================================================
# Flags
# =============================================================================

COMMON_CXXFLAGS = \
    -arch x86_64 \
    -std=c++17 \
    -fno-rtti \
    -fno-exceptions \
    -mkernel \
    -nostdlib \
    -DKERNEL \
    -DKERNEL_PRIVATE \
    -DDRIVER_PRIVATE \
    -DAPPLE \
    -DNeXT \
    -DPRODUCT_NAME=$(PRODUCT_NAME) \
    -I$(LILU_PATH)/Lilu \
    -I$(MACKERNELSDK_PATH)/Headers \
    -I./Headers \
    -I./DkzKext \
    $(KEXT_SDK)

DEBUG_CXXFLAGS   = $(COMMON_CXXFLAGS) -DDEBUG -O0 -g
RELEASE_CXXFLAGS = $(COMMON_CXXFLAGS) -O2 -DNDEBUG

LDFLAGS = \
    -arch x86_64 \
    -mkernel \
    -nostdlib \
    -Xlinker -kext \
    -Xlinker -no_deduplicate \
    $(KEXT_SDK)

# =============================================================================
# Source Files
# =============================================================================

SRCS = \
    DkzKext/kern_start.cpp \
    DkzKext/DkzKext.cpp \
    DkzKext/Framebuffer/Framebuffer.cpp \
    DkzKext/Firmware/HWLibs.cpp \
    DkzKext/PowerPlay/PowerPlay.cpp

HEADERS = \
    DkzKext/DkzKext.hpp \
    DkzKext/NaVi3x.hpp \
    DkzKext/PatcherPlus.hpp \
    DkzKext/Framebuffer/Framebuffer.hpp \
    DkzKext/Firmware/HWLibs.hpp \
    DkzKext/PowerPlay/PowerPlay.hpp \
    Headers/kern_util.hpp \
    Headers/ATOMBIOS.hpp

# Object files
DEBUG_OBJS   = $(SRCS:%.cpp=$(DEBUG_DIR)/%.o)
RELEASE_OBJS = $(SRCS:%.cpp=$(RELEASE_DIR)/%.o)

# =============================================================================
# Targets
# =============================================================================

.PHONY: all debug release clean package help

all: release

help:
	@echo "DkzKext Build System"
	@echo "===================="
	@echo ""
	@echo "Targets:"
	@echo "  debug     - Build debug version (with logging)"
	@echo "  release   - Build release version (optimized)"
	@echo "  clean     - Remove all build artifacts"
	@echo "  package   - Create distributable .zip"
	@echo ""
	@echo "Environment Variables:"
	@echo "  LILU_PATH         = $(LILU_PATH)"
	@echo "  MACKERNELSDK_PATH = $(MACKERNELSDK_PATH)"

# --- Debug Build ---
debug: $(DEBUG_DIR)/$(KEXT_NAME)
	@echo "✅ Debug build complete: $(DEBUG_DIR)/$(KEXT_NAME)"

$(DEBUG_DIR)/$(KEXT_NAME): $(DEBUG_OBJS)
	@mkdir -p $(DEBUG_DIR)/$(KEXT_NAME)/Contents/MacOS
	$(CXX) $(LDFLAGS) -o $(DEBUG_DIR)/$(KEXT_NAME)/Contents/MacOS/$(PRODUCT_NAME) $(DEBUG_OBJS)
	@cp DkzKext/Info.plist $(DEBUG_DIR)/$(KEXT_NAME)/Contents/Info.plist
	@echo "Built debug kext"

$(DEBUG_DIR)/%.o: %.cpp $(HEADERS)
	@mkdir -p $(dir $@)
	$(CXX) $(DEBUG_CXXFLAGS) -c $< -o $@

# --- Release Build ---
release: $(RELEASE_DIR)/$(KEXT_NAME)
	@echo "✅ Release build complete: $(RELEASE_DIR)/$(KEXT_NAME)"

$(RELEASE_DIR)/$(KEXT_NAME): $(RELEASE_OBJS)
	@mkdir -p $(RELEASE_DIR)/$(KEXT_NAME)/Contents/MacOS
	$(CXX) $(LDFLAGS) -o $(RELEASE_DIR)/$(KEXT_NAME)/Contents/MacOS/$(PRODUCT_NAME) $(RELEASE_OBJS)
	@cp DkzKext/Info.plist $(RELEASE_DIR)/$(KEXT_NAME)/Contents/Info.plist
	@echo "Built release kext"

$(RELEASE_DIR)/%.o: %.cpp $(HEADERS)
	@mkdir -p $(dir $@)
	$(CXX) $(RELEASE_CXXFLAGS) -c $< -o $@

# --- Clean ---
clean:
	@rm -rf $(BUILD_DIR)
	@echo "🧹 Build directory cleaned"

# --- Package ---
package: release
	@mkdir -p $(BUILD_DIR)/package
	@cp -R $(RELEASE_DIR)/$(KEXT_NAME) $(BUILD_DIR)/package/
	@cp README.md $(BUILD_DIR)/package/
	@cd $(BUILD_DIR)/package && zip -r ../$(PRODUCT_NAME)-v$(VERSION).zip .
	@echo "📦 Package created: $(BUILD_DIR)/$(PRODUCT_NAME)-v$(VERSION).zip"
