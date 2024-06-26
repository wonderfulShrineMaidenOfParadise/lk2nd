/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Copyright (c) 2013-2017,2019,2021 The Linux Foundation. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef __TARGET_H
#define __TARGET_H
#include <qmp_phy.h>

#define TARGET_MAX_CMDLNBUF 64

/* Serial Number Length. */
#define SERIAL_NUMBER_LEN   13

/* Enum for target VB version detection */
enum
{
	VB_L = 1,
	VB_M = 2,
	VB_AVB2 = 4,
};

/* Target helper functions exposed to USB driver */
typedef struct {
	void (*mux_config) (void);
	void (*phy_reset) (void);
	void (*phy_init) (void);
	void (*clock_init) (void);
	uint8_t vbus_override;
} target_usb_iface_t;

/* super early platform initialization, before almost everything */
void target_early_init(void);

/* later init, after the kernel has come up */
void target_init(void);

/* get memory address for fastboot image loading */
void *target_get_scratch_address(void);

/* get the max allowed flash size */
unsigned target_get_max_flash_size(void);

/* if target is using eMMC bootup */
#if BOOT_CONFIG_SUPPORT && !UFS_SUPPORT
int target_is_emmc_boot(void);
#else
static inline int target_is_emmc_boot(void)
{
#if _EMMC_BOOT
    return 1;
#else
    return 0;
#endif
}
#endif

uint8_t target_uart_gsbi(void);
unsigned* target_atag_mem(unsigned* ptr);
void target_battery_charging_enable(unsigned enable, unsigned disconnect);
unsigned target_pause_for_battery_charge(void);
unsigned target_baseband(void);
void target_serialno(unsigned char *buf);
void target_fastboot_init(void);
void target_load_ssd_keystore(void);
void *target_mmc_device(void);

#if USER_FORCE_RESET_SUPPORT
uint32_t is_user_force_reset(void);
#else
static inline uint32_t is_user_force_reset(void) { return 0; }
#endif

bool target_display_panel_node(char *pbuf, uint16_t buf_size);
void target_display_init(const char *panel_name);
void target_display_shutdown(void);

uint32_t target_get_boot_device(void);

const char * target_usb_controller(void);
void target_usb_phy_reset(void);
void target_usb_phy_sec_reset(void);
void target_usb_phy_mux_configure(void);
target_usb_iface_t * target_usb30_init(void);
bool target_is_cdp_qvga(void);
uint32_t target_hw_interposer(void);
uint32_t target_override_pll(void);
uint32_t target_ddr_cfg_val(void);
void target_usb_init(void);
void target_usb_stop(void);
uint32_t target_get_hlos_subtype(void);
void shutdown_device(void);
uint32_t target_is_pwrkey_pon_reason(void);
bool target_warm_boot(void);
int _emmc_recovery_init(void);
void ulpi_write(unsigned val, unsigned reg);
void target_crypto_init_params(void);
int target_cont_splash_screen(void);
void pmic_reset_configure(uint8_t reset_type);
bool is_display_disabled(void);
struct qmp_reg *target_get_qmp_settings(void);
int target_get_qmp_regsize(void);
uint32_t target_ddr_cfg_reg(void);

int target_volume_up(void);
uint32_t target_volume_down(void);

bool target_is_pmi_enabled(void);
#if PON_VIB_SUPPORT
struct qpnp_hap;
void get_vibration_type(struct qpnp_hap *config);
#endif

#if CHECK_BAT_VOLTAGE
void update_battery_status(void);
#endif

uint32_t target_get_battery_voltage(void);
bool target_battery_soc_ok(void);
bool target_battery_is_present(void);
uint32_t target_get_pmic(void);
int target_update_cmdline(char *cmdline);

struct mmc_device *target_get_sd_mmc(void);

static inline bool target_is_ssd_enabled(void)
{
#ifdef SSD_ENABLE
	return 1;
#else
	return 0;
#endif
}

static inline bool target_use_signed_kernel(void)
{
#if _SIGNED_KERNEL
	return 1;
#else
	return 0;
#endif
}

static inline bool target_build_variant_user(void)
{
#if USER_BUILD_VARIANT
	return true;
#else
	return false;
#endif
}

static inline bool target_dynamic_partition_supported(void)
{
#if DYNAMIC_PARTITION_SUPPORT
	return true;
#else
	return false;
#endif
}

static inline bool target_virtual_ab_supported(void)
{
#if VIRTUAL_AB_OTA
	return true;
#else
	return false;
#endif
}

static inline bool is_target_support_dtbo(void)
{
#if TARGET_DTBO_NOT_SUPPORTED
	return false;
#else
	return true;
#endif
}

#if VERIFIED_BOOT || VERIFIED_BOOT_2
int target_get_vb_version(void);
#else
static inline int target_get_vb_version(void) { return -1; }
#endif

static inline bool target_uses_system_as_root(void)
{
#if TARGET_USE_SYSTEM_AS_ROOT_IMAGE
	if (target_get_vb_version() >= VB_M)
		return true;
#endif
	return false;
}
#endif
