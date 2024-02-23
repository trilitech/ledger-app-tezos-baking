ifeq ($(BOLOS_SDK),)
$(error Environment variable BOLOS_SDK is not set)
endif
include $(BOLOS_SDK)/Makefile.defines

APPNAME = "Tezos Baking"


HAVE_APPLICATION_FLAG_LIBRARY = 1
ifneq ($(TARGET_NAME), TARGET_NANOS)
HAVE_APPLICATION_FLAG_GLOBAL_PIN = 1
HAVE_APPLICATION_FLAG_BOLOS_SETTINGS = 1
endif
CURVE_APP_LOAD_PARAMS = ed25519 secp256k1 secp256r1
PATH_APP_LOAD_PARAMS  = "44'/1729'"

GIT_DESCRIBE ?= $(shell git describe --tags --abbrev=8 --always --long --dirty 2>/dev/null)

VERSION_TAG ?= $(shell echo "$(GIT_DESCRIBE)" | cut -f1 -d-)
APPVERSION_M=2
APPVERSION_N=4
APPVERSION_P=7
APPVERSION=$(APPVERSION_M).$(APPVERSION_N).$(APPVERSION_P)

# Only warn about version tags if specified/inferred
ifeq ($(VERSION_TAG),)
  $(warning VERSION_TAG not checked)
else
  ifneq (v$(APPVERSION), $(VERSION_TAG))
    $(warning Version-Tag Mismatch: v$(APPVERSION) version and $(VERSION_TAG) tag)
  endif
endif

COMMIT ?= $(shell echo "$(GIT_DESCRIBE)" | awk -F'-g' '{print $2}' | sed 's/-dirty/*/')
ifeq ($(COMMIT),)
  $(warning COMMIT not specified and could not be determined with git from "$(GIT_DESCRIBE)")
else
  $(info COMMIT=$(COMMIT))
endif

ICON_NANOS  = icons/nano-s-tezos.gif
ICON_NANOX  = icons/nano-x-tezos.gif
ICON_NANOSP = $(ICON_NANOX)
ICON_STAX   = icons/stax_tezos.gif

################
# Default rule #
################
all: show-app default


.PHONY: show-app
show-app:
	@echo ">>>>> Building at commit $(COMMIT)"


############
# Platform #
############

DISABLE_STANDARD_APP_SYNC_RAPDU = 1
DISABLE_STANDARD_APP_FILES = 1
DISABLE_STANDARD_USB = 1
DEFINES   += HAVE_IO_USB HAVE_L4_USBLIB IO_USB_MAX_ENDPOINTS=6 IO_HID_EP_LENGTH=64 HAVE_USB_APDU
DEFINES   += HAVE_LEGACY_PID
DEFINES   += VERSION=\"$(APPVERSION)\" APPVERSION_M=$(APPVERSION_M)
DEFINES   += COMMIT=\"$(COMMIT)\" APPVERSION_N=$(APPVERSION_N) APPVERSION_P=$(APPVERSION_P)

ENABLE_BLUETOOTH = 1
ENABLE_NBGL_QRCODE = 1

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

CFLAGS   += -Wno-incompatible-pointer-types-discards-qualifiers

### computed variables
APP_SOURCE_PATH  += src
SDK_SOURCE_PATH  += lib_stusb lib_stusb_impl

VARIANT_PARAM  = APP
VARIANT_VALUES = tezos_baking

include $(BOLOS_SDK)/Makefile.standard_app

# Generate delegates from baker list
src/delegates.h: tools/gen-delegates.sh tools/BakersRegistryCoreUnfilteredData.json
	bash ./tools/gen-delegates.sh ./tools/BakersRegistryCoreUnfilteredData.json
$(DEP_DIR)/to_string.d $(DEP_DIR)/app/src/to_string.d: src/delegates.h
