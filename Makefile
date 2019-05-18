
# Check gcc version

ifneq (,$(CC))
    GCCVERSION := $(shell $(CC) -v 2>&1)
else
    GCCVERSION := "NONE"
endif

ifneq (,$(findstring arm,$(GCCVERSION)))         # Target: arm-oe-linux-gnueabi
    TARGETTYPE = "ARM"
else ifneq (,$(findstring linux,$(GCCVERSION)))  # Target: x86_64-redhat-linux
    TARGETTYPE = "LINUX"
else ifneq (,$(findstring cygwin,$(GCCVERSION))) # Target: x86_64-pc-cygwin
    TARGETTYPE = "CYGWIN"
else
    TARGETTYPE = "NOTSUPPORT"                    # Not supported
endif


# Directories
BASE_DIR := $(CURDIR)
SRC_DIR := $(BASE_DIR)/src
LIB_DIR := $(BASE_DIR)/lib
BUILD_DIR := $(BASE_DIR)/build
THIRD_SRC_DIR := $(BASE_DIR)/3rd


# Environment Language
LANG = en_US  # zh_CN.UTF-8


# Common Flags
CC_INC = -I$(SRC_DIR) \
         -I$(THIRD_SRC_DIR)/ecdsa \
         -I$(THIRD_SRC_DIR)/cJSON \
         -I$(BASE_DIR)/demo

CSTD = -std=c99
OPTFLAGS = -g -Os 
CFLAGS_WARN = -Wall
DEFINE = -DUSE_KECCAK=1 #-DDEBUG_LOG



# Target-specific Flags
ifeq ($(TARGETTYPE), "ARM")
    TARGET_SPEC_CFLAGS = -mthumb
    CC_INC += -I$(THIRD_SRC_DIR)  # for $(THIRD_SRC_DIR)/curl
    THIRD_LIBS =  $(LIB_DIR)/libecdsa.a \
                  $(LIB_DIR)/libcJSON.a \
                  $(LIB_DIR)/libcurl.so
    STD_LIBS = -lcrypto
    LINK_FLAGS = -Wl,-Map,$(BUILD_DIR)/boatwallet.map   #-Wl,-L$(LIB_DIR)
else ifeq ($(TARGETTYPE), "LINUX")
    TARGET_SPEC_CFLAGS =
    THIRD_LIBS =  $(LIB_DIR)/libecdsa.a \
                  $(LIB_DIR)/libcJSON.a
    STD_LIBS = -lcurl -lcrypto
    LINK_FLAGS = -Wl,-Map,$(BUILD_DIR)/boatwallet.map
else ifeq ($(TARGETTYPE), "CYGWIN")
    TARGET_SPEC_CFLAGS =
    THIRD_LIBS =  $(LIB_DIR)/libecdsa.a \
                  $(LIB_DIR)/libcJSON.a
    STD_LIBS = -lcurl -lcrypto
    LINK_FLAGS = -Wl,-Map,$(BUILD_DIR)/boatwallet.map
else
    TARGET_SPEC_CFLAGS =
    THIRD_LIBS = 
    STD_LIBS = 
    LINK_FLAGS = 
endif

# Hardware Target
HW_TARGET = qc9x07
CC_INC += -I$(BASE_DIR)/hwdep/$(HW_TARGET)

# Combine CFLAGS
CFLAGS = $(TARGET_SPEC_CFLAGS) $(CC_INC) $(CSTD) $(OPTFLAGS) $(CFLAGS_WARN) $(DEFINE)



export BASE_DIR
export SRC_DIR
export LIB_DIR
export BUILD_DIR
export HW_TARGET
export THIRD_LIBS
export STD_LIBS
export CFLAGS
export LINK_FLAGS



#Targets
#make all: build libboatwallet.a as well as 3rd party libraries
#make boatwalletlib: Same as make all
#make demoapp: build demo executable
#make clean: clean libboatwallet.a and its objects
#make cleandemo: clean demo executable and objects
#make clean3rd: clean 3rd party libraries and objects
#make distclean: clean above 3 ones

all: createdir boatwalletlib hwdeplib thirdlibs demoapp

createdir:
	mkdir -p $(LIB_DIR)
	mkdir -p $(BUILD_DIR)


demoapp: boatwalletlib hwdeplib thirdlibs
	make -C $(BASE_DIR)/demo all

boatwalletlib:
	make -C $(BASE_DIR)/src all

hwdeplib:
	make -C $(BASE_DIR)/hwdep all

thirdlibs:
	for dir in $(THIRD_SRC_DIR)/*; do \
		[ -d $$dir ] && make -C $$dir all; \
	done



clean: cleanboatwallet cleanhwdep cleandemo
	-rm -f $(BUILD_DIR)/boatwallet.map
	-rm -f $(LIB_DIR)/libboatwallet.a
	for dir in $(SRC_DIR)/*; do \
		[ -d $$dir ] && make -C $$dir clean; \
	done

cleanboatwallet:
	make -C $(BASE_DIR)/src clean
	
cleanhwdep:
	make -C $(BASE_DIR)/hwdep clean

cleandemo:
	make -C $(BASE_DIR)/demo clean

clean3rd:
	for dir in $(THIRD_SRC_DIR)/*; do \
		[ -d $$dir ] && make -C $$dir clean; \
	done


distclean: clean clean3rd


