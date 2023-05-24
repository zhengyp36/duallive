.PHONY: all clean
.SECONDARY:

COMMON-objs += libspl/avl.o
COMMON-objs += libspl/list.o
COMMON-objs += libspl/kernel.o
COMMON-objs += xutils/cvector.o
COMMON-objs += xutils/shell.o
COMMON-objs += xutils/file_map.o
COMMON-objs += xutils/simple_sock.o
COMMON-objs += duallive/arbitrary_msg.o
COMMON-objs += duallive/arbitrary_disk.o

BIN += demo
demo-objs += $(COMMON-objs)
demo-objs += duallive/demo.o

BIN += arbitrary_server
arbitrary_server-objs += $(COMMON-objs)

BIN += arbitrary_cleanup
arbitrary_cleanup-objs += $(COMMON-objs)

BIN += arbitrary_proxy
arbitrary_proxy-objs += $(COMMON-objs)

BIN += duallive_manager
duallive_manager-objs += $(COMMON-objs)

TOP_DIR := $(shell pwd)
SRC_DIR := $(TOP_DIR)/src
RELEASE := $(TOP_DIR)/release
BUILD_DIR := $(TOP_DIR)/build

INCLUDES += -I$(TOP_DIR)/inc
INCLUDES += -I$(TOP_DIR)/inc/libspl
INCLUDES += -include sys/sysmacros.h

CFLAGS += -g -DDL_DEBUG
CFLAGS += -std=gnu99 -Wall $(INCLUDES)

all: $(patsubst %,$(RELEASE)/%,$(BIN))

clean:
	$(RM) -r $(RELEASE) $(BUILD_DIR)

$(BUILD_DIR)/src/%.o: $(SRC_DIR)/%.c
	@echo "CC $<"
	@mkdir -p $(shell dirname $@)
	@$(CC) $(CFLAGS) -c -o $@ $<

define LINK
$(1)-objs += tools/$(1).o
$$(RELEASE)/$(1): $$(patsubst %.o,$$(BUILD_DIR)/src/%.o,$$($(1)-objs))
	@echo "LD $$(shell pwd)/$$@"
	@mkdir -p $$(shell dirname $$@)
	@$$(CC) $$^ -o $$@
endef

$(foreach bin,$(BIN),$(eval $(call LINK,$(bin))))
