# Check if BOLOS SDK isn't defined
ifeq ($(BOLOS_SDK),)

# Display error
$(error Environment variable BOLOS_SDK is not set)
endif

include $(BOLOS_SDK)/Makefile.target

########################################
#        Mandatory configuration       #
########################################

VARIANT_PARAM = CURRENCY
VARIANT_VALUES = mimblewimble_coin mimblewimble_coin_floonet

# Check if currency isn't defined
ifndef CURRENCY
	# Set currency to MimbleWimble Coin
	CURRENCY = mimblewimble_coin
endif

# Application name
ifeq ($(CURRENCY),mimblewimble_coin)

	# Application name
	APPNAME = "MimbleWimble Coin"

	# 44'/593' path on secp256k1 curve
	PATH_APP_LOAD_PARAMS = "44'/593'"

	# Defines
	DEFINES += CURRENCY_BIP44_COIN_TYPE=593
	DEFINES += CURRENCY_MQS_VERSION=\{1,69\}
	DEFINES += CURRENCY_NAME=\"MimbleWimble\\x20\\x43oin\"
	DEFINES += CURRENCY_ABBREVIATION=\"MWC\"

# Otherwise check if currency is MimbleWimble Coin floonet
else ifeq ($(CURRENCY),mimblewimble_coin_floonet)

	# Application name
	APPNAME = "MimbleWimble Coin Floonet"

	# Testnet path
	PATH_APP_LOAD_PARAMS = "44'/1'"

	# Defines
	DEFINES += CURRENCY_BIP44_COIN_TYPE=1
	DEFINES += CURRENCY_MQS_VERSION=\{1,121\}
	DEFINES += CURRENCY_NAME=\"MimbleWimble\\x20\\x43oin\\x20\\x46loonet\"
	DEFINES += CURRENCY_ABBREVIATION=\"Floonet\\x20MWC\"

# Otherwise
else
# Display error
$(error Unsupported CURRENCY - use mimblewimble_coin or mimblewimble_coin_floonet)
endif

# Common to main and testnet
CURVE_APP_LOAD_PARAMS = secp256k1

# Application parameters
# APP_LOAD_PARAMS += $(COMMON_LOAD_PARAMS)

# Application version
APPVERSION_M = 7
APPVERSION_N = 6
APPVERSION_P = 0
APPVERSION = "$(APPVERSION_M).$(APPVERSION_N).$(APPVERSION_P)"

# Application source files
APP_SOURCE_PATH += src
# Needed fo Blake2
INCLUDES_PATH += $(BOLOS_SDK)/lib_cxng/src
APP_SOURCE_FILES += $(BOLOS_SDK)/lib_cxng/src/cx_blake2b.c
APP_SOURCE_FILES += $(BOLOS_SDK)/lib_cxng/src/cx_ram.c

ICON_NANOX = "icons/nanox_app.gif"
ICON_NANOSP = "icons/nanosplus_app.gif"
ICON_STAX = "icons/stax_app.png"
ICON_FLEX = "icons/flex_app.png"
ICON_APEX_P = "icons/apex_p_app.png"

########################################
# Application communication interfaces #
########################################
ENABLE_BLUETOOTH = 1
#ENABLE_NFC = 1
#ENABLE_NBGL_FOR_NANO_DEVICES = 1

########################################
#         NBGL custom features         #
########################################
ENABLE_NBGL_QRCODE = 1
#ENABLE_NBGL_KEYBOARD = 1
#ENABLE_NBGL_KEYPAD = 1

########################################
#          Features disablers          #
########################################
# These advanced settings allow to disable some feature that are by
# default enabled in the SDK `Makefile.standard_app`.
DISABLE_STANDARD_APP_FILES = 1
#DISABLE_DEFAULT_IO_SEPROXY_BUFFER_SIZE = 1 # To allow custom size declaration
#DISABLE_STANDARD_APP_DEFINES = 1 # Will set all the following disablers
#DISABLE_STANDARD_SNPRINTF = 1
#DISABLE_STANDARD_USB = 1
#DISABLE_STANDARD_WEBUSB = 1
#DISABLE_DEBUG_LEDGER_ASSERT = 1
#DISABLE_DEBUG_THROW = 1

include $(BOLOS_SDK)/Makefile.standard_app

# Emulator flags
EMULATOR_FLAGS = --model `echo $(lastword $(subst _, ,$(TARGET_NAME))) | tr 2 P | tr A-Z a-z` --seed "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about"

# Check if target version is defined
ifneq ($(TARGET_VERSION),)

	# SDK emulator flag
	EMULATOR_FLAGS += --sdk $(subst $(eval) ,.,$(wordlist 1,2,$(subst ., ,$(TARGET_VERSION))))
endif

# Run command
run: all

	# Run application in emulator
	SPECULOS_APPNAME=$(APPNAME):$(APPVERSION) $(BOLOS_EMU)/speculos.py bin/app.elf $(EMULATOR_FLAGS)

# Functional tests
functional_tests: all

	# Run functional tests
	node tests/functional_tests/main.js $(CURRENCY)
