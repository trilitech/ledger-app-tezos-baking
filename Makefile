ifeq ($(BOLOS_SDK),)
$(error Environment variable BOLOS_SDK is not set)
endif
include $(BOLOS_SDK)/Makefile.defines

ifeq ($(APP),)
APP=tezos_wallet
endif

ifeq ($(APP),tezos_baking)
APPNAME = "Tezos Baking"
else ifeq ($(APP),tezos_wallet)
APPNAME = "Tezos Wallet"
endif

ifeq ($(TARGET_NAME), TARGET_NANOS)
APP_LOAD_FLAGS=--appFlags 0x800  # APPLICATION_FLAG_LIBRARY
else
APP_LOAD_FLAGS=--appFlags 0xa40  # APPLICATION_FLAG_LIBRARY + APPLICATION_FLAG_BOLOS_SETTINGS + BLE SUPPORT
endif
APP_LOAD_PARAMS=$(APP_LOAD_FLAGS) --curve ed25519 --curve secp256k1 --curve secp256r1 --path "44'/1729'" $(COMMON_LOAD_PARAMS)

GIT_DESCRIBE ?= $(shell git describe --tags --abbrev=8 --always --long --dirty 2>/dev/null)

VERSION_TAG ?= $(shell echo "$(GIT_DESCRIBE)" | cut -f1 -d-)
APPVERSION_M=2
APPVERSION_N=4
APPVERSION_P=1
APPVERSION=$(APPVERSION_M).$(APPVERSION_N).$(APPVERSION_P)

# Only warn about version tags if specified/inferred
ifeq ($(VERSION_TAG),)
  $(warning VERSION_TAG not checked)
else
  ifneq (v$(APPVERSION), $(VERSION_TAG))
    $(warning Version-Tag Mismatch: v$(APPVERSION) version and $(VERSION_TAG) tag)
  endif
endif

COMMIT ?= $(shell echo "$(GIT_DESCRIBE)" | awk -F'-g' '{print $$2}' | sed 's/-dirty/*/')
ifeq ($(COMMIT),)
  $(warning COMMIT not specified and could not be determined with git from "$(GIT_DESCRIBE)")
else
  $(info COMMIT=$(COMMIT))
endif

ifeq ($(TARGET_NAME),TARGET_NANOS)
ICONNAME=icons/nano-s-tezos.gif
else ifeq ($(TARGET_NAME),TARGET_STAX)
ICONNAME=icons/stax_tezos.gif
else
ICONNAME=icons/nano-x-tezos.gif
endif

################
# Default rule #
################
all: show-app default


.PHONY: show-app
show-app:
	@echo ">>>>> Building $(APP) at commit $(COMMIT)"


############
# Platform #
############

DEFINES   += OS_IO_SEPROXYHAL
DEFINES   += HAVE_SPRINTF
DEFINES   += HAVE_IO_USB HAVE_L4_USBLIB IO_USB_MAX_ENDPOINTS=6 IO_HID_EP_LENGTH=64 HAVE_USB_APDU
DEFINES   += HAVE_LEGACY_PID
DEFINES   += VERSION=\"$(APPVERSION)\" APPVERSION_M=$(APPVERSION_M)
DEFINES   += COMMIT=\"$(COMMIT)\" APPVERSION_N=$(APPVERSION_N) APPVERSION_P=$(APPVERSION_P)
# DEFINES   += _Static_assert\(...\)=

ifeq ($(TARGET_NAME),$(filter $(TARGET_NAME),TARGET_NANOX TARGET_STAX))
DEFINES   += HAVE_BLE BLE_COMMAND_TIMEOUT_MS=2000
DEFINES   += HAVE_BLE_APDU # basic ledger apdu transport over BLE

SDK_SOURCE_PATH  += lib_blewbxx lib_blewbxx_impl
endif

ifeq ($(TARGET_NAME),TARGET_NANOS)
DEFINES   += IO_SEPROXYHAL_BUFFER_SIZE_B=128
else
DEFINES   += IO_SEPROXYHAL_BUFFER_SIZE_B=300
endif

ifeq ($(TARGET_NAME),TARGET_STAX)
    DEFINES += NBGL_QRCODE
    SDK_SOURCE_PATH += qrcode
else
    DEFINES += HAVE_BAGL HAVE_UX_FLOW
    ifneq ($(TARGET_NAME),TARGET_NANOS)
        DEFINES   += HAVE_GLO096
        DEFINES   += BAGL_WIDTH=128 BAGL_HEIGHT=64
        DEFINES   += HAVE_BAGL_ELLIPSIS # long label truncation feature
        DEFINES   += HAVE_BAGL_FONT_OPEN_SANS_REGULAR_11PX
        DEFINES   += HAVE_BAGL_FONT_OPEN_SANS_EXTRABOLD_11PX
        DEFINES   += HAVE_BAGL_FONT_OPEN_SANS_LIGHT_16PX
    endif
endif

DEFINES   += UNUSED\(x\)=\(void\)x

# Enabling debug PRINTF
DEBUG ?= 0
ifneq ($(DEBUG),0)

        DEFINES += TEZOS_DEBUG

        ifeq ($(TARGET_NAME),TARGET_NANOS)
                DEFINES   += HAVE_PRINTF PRINTF=screen_printf
        else
                DEFINES   += HAVE_PRINTF PRINTF=mcu_usb_printf
        endif
else
        DEFINES   += PRINTF\(...\)=
endif



##############
# Compiler #
##############
ifneq ($(BOLOS_ENV),)
$(info BOLOS_ENV=$(BOLOS_ENV))
CLANGPATH := $(BOLOS_ENV)/clang-arm-fropi/bin/
GCCPATH := $(BOLOS_ENV)/gcc-arm-none-eabi-5_3-2016q1/bin/
CFLAGS += -idirafter $(BOLOS_ENV)/gcc-arm-none-eabi-5_3-2016q1/arm-none-eabi/include
else
$(info BOLOS_ENV is not set: falling back to CLANGPATH and GCCPATH)
endif
ifeq ($(CLANGPATH),)
$(info CLANGPATH is not set: clang will be used from PATH)
endif
ifeq ($(GCCPATH),)
$(info GCCPATH is not set: arm-none-eabi-* will be used from PATH)
endif

CC       := $(CLANGPATH)clang

ifeq ($(APP),tezos_wallet)
CFLAGS   += -O3 -Os -Wall -Wextra -Wno-incompatible-pointer-types-discards-qualifiers
else ifeq ($(APP),tezos_baking)
CFLAGS   += -DBAKING_APP -O3 -Os -Wall -Wextra -Wno-incompatible-pointer-types-discards-qualifiers
else
ifeq ($(filter clean,$(MAKECMDGOALS)),)
$(error Unsupported APP - use tezos_wallet, tezos_baking)
endif
endif

AS     := $(GCCPATH)arm-none-eabi-gcc

LD       := $(GCCPATH)arm-none-eabi-gcc
LDFLAGS  += -O3 -Os
LDLIBS   += -lm -lgcc -lc

# import rules to compile glyphs(/pone)
include $(BOLOS_SDK)/Makefile.glyphs

### computed variables
APP_SOURCE_PATH  += src
SDK_SOURCE_PATH  += lib_stusb lib_stusb_impl

ifneq ($(TARGET_NAME),TARGET_STAX)
SDK_SOURCE_PATH += lib_ux
endif

### U2F support (wallet app only)
ifeq ($(APP), tezos_wallet)
SDK_SOURCE_PATH  += lib_u2f lib_stusb_impl

DEFINES   += USB_SEGMENT_SIZE=64

DEFINES   += U2F_PROXY_MAGIC=\"XTZ\"
DEFINES   += HAVE_IO_U2F HAVE_U2F
endif

load: all
	python3 -m ledgerblue.loadApp $(APP_LOAD_PARAMS)

delete:
	python3 -m ledgerblue.deleteApp $(COMMON_DELETE_PARAMS)

# import generic rules from the sdk
include $(BOLOS_SDK)/Makefile.rules

listvariants:
	@echo VARIANTS APP tezos_wallet tezos_baking

# Define DEP_DIR to keep compatibility with old SDK
ifeq ($(DEP_DIR),)
	DEP_DIR := dep
endif

# Generate delegates from baker list
src/delegates.h: tools/gen-delegates.sh tools/BakersRegistryCoreUnfilteredData.json
	bash ./tools/gen-delegates.sh ./tools/BakersRegistryCoreUnfilteredData.json
$(DEP_DIR)/to_string.d: src/delegates.h
