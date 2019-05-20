# Source and Objects

SOURCES = $(wildcard *.c)
OBJECTS_DIR = $(BUILD_DIR)/hwdep
OBJECTS = $(patsubst %.c,$(OBJECTS_DIR)/%.o,$(SOURCES))


all: $(OBJECTS_DIR) $(LIB_DIR)/libhwdep.a

$(OBJECTS_DIR):
	mkdir -p $(OBJECTS_DIR)


$(LIB_DIR)/libhwdep.a: $(OBJECTS)
	$(AR) r $(LIB_DIR)/libhwdep.a $(OBJECTS)

$(OBJECTS_DIR)/%.o:%.c
	$(CC) -c $(CFLAGS) $< -o $@


clean:
	-rm -f $(OBJECTS)
	-rm -f $(LIB_DIR)/libhwdep.a

