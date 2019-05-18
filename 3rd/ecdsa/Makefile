OPTFLAGS ?= -O3 

CFLAGS   =  $(OPTFLAGS) \
            -std=gnu99 \
            -W \
            -Wall \
            -Wextra \
            -Wimplicit-function-declaration \
            -Wredundant-decls \
            -Wstrict-prototypes \
            -Wundef \
            -Wshadow \
            -Wpointer-arith \
            -Wformat \
            -Wreturn-type \
            -Wsign-compare \
            -Wmultichar \
            -Wformat-nonliteral \
            -Winit-self \
            -Wuninitialized \
            -Wformat-security \
            -Werror

VALGRIND ?= 1

CFLAGS += -I.
CFLAGS += -DVALGRIND=$(VALGRIND)
CFLAGS += -DUSE_ETHEREUM=1
CFLAGS += -DUSE_GRAPHENE=1
CFLAGS += -DUSE_KECCAK=1
CFLAGS += -DUSE_MONERO=1
CFLAGS += -DUSE_NEM=1
CFLAGS += -DUSE_CARDANO=1
CFLAGS += $(shell pkg-config --cflags openssl)

# use platform depedent random mechanism
CFLAGS += -DRAND_PLATFORM_INDEPENDENT

# disable certain optimizations and features when small footprint is required
ifdef SMALL
CFLAGS += -DUSE_PRECOMPUTED_CP=0
endif


# Source and Objects

SOURCES = $(wildcard *.c)
OBJECTS_DIR = $(BUILD_DIR)/ecdsa
OBJECTS = $(patsubst %.c,$(OBJECTS_DIR)/%.o,$(SOURCES))

all: $(OBJECTS_DIR) $(LIB_DIR)/libecdsa.a

$(OBJECTS_DIR):
	mkdir -p $(OBJECTS_DIR)

$(LIB_DIR)/libecdsa.a: $(OBJECTS)
	$(AR) r $(LIB_DIR)/libecdsa.a $(OBJECTS)

$(OBJECTS_DIR)/%.o:%.c
	$(CC) -c $(CFLAGS) -DAES_128 -DAES_192 -fPIC $< -o $@


clean:	
	-rm -f $(OBJECTS)
	-rm -f $(LIB_DIR)/libecdsa.a



