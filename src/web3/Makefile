# Source and Objects

SOURCES = $(wildcard *.c)
OBJECTS_DIR = $(BUILD_DIR)/web3
OBJECTS = $(patsubst %.c,$(OBJECTS_DIR)/%.o,$(SOURCES))


all: $(OBJECTS_DIR) $(OBJECTS)

$(OBJECTS_DIR):
	mkdir -p $(OBJECTS_DIR)

$(OBJECTS_DIR)/%.o:%.c
	$(CC) -c $(CFLAGS) $< -o $@


clean:
	-rm -f $(OBJECTS)