LOCAL_DIR := $(GET_LOCAL_DIR)

OBJS += \
	$(LOCAL_DIR)/lk2nd-device.o \
	$(LOCAL_DIR)/lk2nd-fdt.o \
	$(LOCAL_DIR)/lk2nd-motorola.o \
	$(LOCAL_DIR)/lk2nd-rproc.o \
	$(LOCAL_DIR)/lk2nd-smd-rpm.o \
	$(LOCAL_DIR)/target_keys_lk2nd.o

ifneq ($(GPIO_I2C_BUS_COUNT),)
MODULES += lk2nd/regmap
OBJS += $(LOCAL_DIR)/lk2nd-samsung.o
endif

ifeq ($(TARGET),msm8916)
MODULES += lk2nd/smb1360
endif

ifneq ($(SMP_SPIN_TABLE_BASE),)
MODULES += lk2nd/smp
DEFINES += SMP_SPIN_TABLE_BASE=$(SMP_SPIN_TABLE_BASE)
endif

ifneq ($(LK1ST_DTB),)
LK1ST_DTB_PATH := dts/$(TARGET)/$(LK1ST_DTB).dtb
$(BUILDDIR)/$(LOCAL_DIR)/lk2nd-device.o: $(BUILDDIR)/$(LK1ST_DTB_PATH)
CFLAGS += -DLK1ST_DTB=\"$(LK1ST_DTB_PATH)\"
endif

ifneq ($(LK1ST_COMPATIBLE),)
CFLAGS += -DLK1ST_COMPATIBLE=\"$(LK1ST_COMPATIBLE)\"
endif

ifneq ($(LK1ST_PANEL),)
# Filter out original panel implementation
OBJS := $(filter-out target/$(TARGET)/oem_panel.o, $(OBJS))
MODULES += lk2nd/panel
CFLAGS += -DLK1ST_PANEL=$(LK1ST_PANEL)
else ifneq ($(LK2ND_DISPLAY),)
INCLUDES += -I$(LOCAL_DIR)/include
include $(LOCAL_DIR)/util/rules.mk
MODULES += lk2nd/display
endif
