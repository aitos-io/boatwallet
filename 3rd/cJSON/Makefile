# Source and Objects

SOURCES = $(wildcard *.c)
OBJECTS_DIR = $(BUILD_DIR)/cJSON
OBJECTS = $(patsubst %.c,$(OBJECTS_DIR)/%.o,$(SOURCES))


all: $(OBJECTS_DIR) $(LIB_DIR)/libcJSON.a

$(OBJECTS_DIR):
	mkdir -p $(OBJECTS_DIR)


$(LIB_DIR)/libcJSON.a: $(OBJECTS)
	$(AR) r $(LIB_DIR)/libcJSON.a $(OBJECTS)

$(OBJECTS_DIR)/%.o:%.c
	$(CC) -c $(CFLAGS) $< -o $@


clean:	
	-rm -f $(OBJECTS)
	-rm -f $(LIB_DIR)/libcJSON.a



