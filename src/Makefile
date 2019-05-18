# Boatwallet Library Objects

OBJECTS = $(wildcard $(BUILD_DIR)/wallet/*.o) \
          $(wildcard $(BUILD_DIR)/utilities/*.o) \
          $(wildcard $(BUILD_DIR)/web3/*.o) \
          $(wildcard $(BUILD_DIR)/rpc/*.o)

all: boatwallet_objs
	$(AR) r $(LIB_DIR)/libboatwallet.a $(OBJECTS)


boatwallet_objs:
	for dir in $(SRC_DIR)/*; do \
		[ -d $$dir ] && make -C $$dir all; \
	done



clean:	
	-rm $(LIB_DIR)/libboatwallet.a
	for dir in $(SRC_DIR)/*; do \
		[ -d $$dir ] && make -C $$dir clean; \
	done


