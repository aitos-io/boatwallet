# Source and Objects

SOURCES = $(wildcard *.c)
OBJECTS_DIR = $(BUILD_DIR)/demo
OBJECTS = $(patsubst %.c,$(OBJECTS_DIR)/%.o,$(SOURCES))


all: $(OBJECTS_DIR) $(OBJECTS)
	$(CC) $(CFLAGS) $(LINK_FLAGS) -o $(BUILD_DIR)/boatdemo $(OBJECTS) \
		$(LIB_DIR)/libboatwallet.a \
		$(THIRD_LIBS) \
		$(LIB_DIR)/libhwdep.a \
		$(STD_LIBS)

$(OBJECTS_DIR):
	mkdir -p $(OBJECTS_DIR)

$(OBJECTS_DIR)/%.o:%.c
	$(CC) -c $(CFLAGS) $< -o $@


clean:
	-rm -f $(OBJECTS)
	-rm -f $(BUILD_DIR)/boatdemo