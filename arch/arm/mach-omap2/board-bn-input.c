/*
 * Button and Button LED support for Nook HDs.
 *
 * Copyright (C) 2011 Texas Instruments
 *
 * Author: Dan Murphy <dmurphy@ti.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/gpio_keys.h>
#include <linux/input.h>
#ifdef CONFIG_INPUT_KXTJ9
#include <linux/input/kxtj9.h>

#include <asm/system_info.h>

#define KXTJ9_GPIO_IRQ			152
#endif

#if defined(CONFIG_TOUCHSCREEN_FT5X06) || defined(CONFIG_TOUCHSCREEN_FT5X06_MODULE)
#include <linux/input/ft5x06_ts.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/i2c.h>

#include <asm/mach-types.h>

#define TOUCHPANEL_GPIO_IRQ     37
#define TOUCHPANEL_GPIO_RESET   39
#endif

#include <plat/omap4-keypad.h>

#include "board-bn-hd.h"
#include "mux.h"

static int bn_keymap[] = {
	KEY(0, 0, KEY_VOLUMEUP),
	KEY(1, 0, KEY_VOLUMEDOWN),
};

static struct omap_device_pad keypad_pads[] = {
	{	.name   = "kpd_col0.kpd_col0",
		.enable = OMAP_WAKEUP_EN | OMAP_MUX_MODE0,
	},
	{	.name   = "kpd_row0.kpd_row0",
		.enable = OMAP_PULL_ENA | OMAP_PULL_UP |
				OMAP_WAKEUP_EN | OMAP_MUX_MODE0 |
				OMAP_INPUT_EN,
	},
	{	.name   = "kpd_row1.kpd_row1",
		.enable = OMAP_PULL_ENA | OMAP_PULL_UP |
				OMAP_WAKEUP_EN | OMAP_MUX_MODE0 |
				OMAP_INPUT_EN,
	},
};

static struct matrix_keymap_data bn_keymap_data = {
	.keymap			= bn_keymap,
	.keymap_size	= ARRAY_SIZE(bn_keymap),
};

static struct omap4_keypad_platform_data bn_keypad_data = {
	.keymap_data	= &bn_keymap_data,
	.rows			= 2,
	.cols			= 1,
};

static struct omap_board_data keypad_data = {
	.id				= 1,
	.pads			= keypad_pads,
	.pads_cnt		= ARRAY_SIZE(keypad_pads),
};

enum {
	HOME_KEY_INDEX = 0,
	HALL_SENSOR_INDEX,
};

/* GPIO_KEY for Tablet */
static struct gpio_keys_button bn_gpio_keys_buttons[] = {
	[HOME_KEY_INDEX] = {
		.code			= KEY_HOME,
		.gpio			= 32,
		.desc			= "SW1",
		.active_low		= 1,
		.wakeup			= 1,
	},
	[HALL_SENSOR_INDEX] = {
		.code			= SW_LID,
		.type			= EV_SW,
		.gpio			= 31,
		.desc			= "HALL",
		.active_low		= 1,
		.wakeup			= 1,
		.debounce_interval = 5,
	},
};

static void gpio_key_buttons_mux_init(void)
{
	/* Hall sensor */
	omap_mux_init_gpio(31, OMAP_PIN_INPUT |
			OMAP_PIN_OFF_WAKEUPENABLE);
}

static struct gpio_keys_platform_data bn_gpio_keys = {
	.buttons	= bn_gpio_keys_buttons,
	.nbuttons	= ARRAY_SIZE(bn_gpio_keys_buttons),
	.rep		= 0,
};

static struct platform_device bn_gpio_keys_device = {
	.name		= "gpio-keys",
	.id			= -1,
	.dev = {
		.platform_data = &bn_gpio_keys,
	},
};

static struct platform_device *bn_devices[] __initdata = {
	&bn_gpio_keys_device,
};

int __init bn_button_init(void)
{
	int status;

	gpio_key_buttons_mux_init();

	platform_add_devices(bn_devices, ARRAY_SIZE(bn_devices));

	status = omap4_keyboard_init(&bn_keypad_data, &keypad_data);
	if (status)
		pr_err("Keypad initialization failed: %d\n", status);

	return 0;
}

#ifdef CONFIG_INPUT_KXTJ9
struct kxtj9_platform_data kxtj9_platform_data = {
	.min_interval   = 1,
	.init_interval  = 200,

	.res_12bit	= RES_12BIT,
	.g_range	= KXTJ9_G_8G,

	/* Map the axes from the sensor to the device */
	.axis_map_x	= machine_is_omap_ovation() ? 1 : 0,
	.axis_map_y	= machine_is_omap_ovation() ? 0 : 1,
	.axis_map_z	= 2,
	.negate_x	= machine_is_omap_hummingbird() ? 1 : 0,
	.negate_y	= 0,
	.negate_z	= 0,

	.int_ctrl_init	= KXTJ9_IEN,
	.int_flags    	= IRQF_TRIGGER_FALLING | IRQF_DISABLED,

#if 0 // AM: old pdata that may be implemented in the future
	.shift_adj	= SHIFT_ADJ_2G,
	.data_odr_init		= ODR12_5F,
	.ctrl_reg1_init		= KXTF9_G_8G | RES_12BIT | TDTE | WUFE | TPE,
	.tilt_timer_init	= 0x03,
	.engine_odr_init	= OTP12_5 | OWUF50 | OTDT400,
	.wuf_timer_init		= 0x16,
	.wuf_thresh_init	= 0x28,
	.tdt_timer_init		= 0x78,
	.tdt_h_thresh_init	= 0xFF,
	.tdt_l_thresh_init	= 0x14,
	.tdt_tap_timer_init	= 0x53,
	.tdt_total_timer_init	= 0x24,
	.tdt_latency_timer_init	= 0x10,
	.tdt_window_timer_init	= 0xA0,

	.gpio			= KXTJ9_GPIO_IRQ,
#endif
};

static struct i2c_board_info __initdata kxtj9_i2c_boardinfo = {
	I2C_BOARD_INFO("kxtj9", 0xe),
	.platform_data = &kxtj9_platform_data,
};

int __init bn_accel_init(void)
{
	int ret;
	ret = gpio_request(KXTJ9_GPIO_IRQ, "kxtj9_irq");

	if (ret)
		return ret;

	printk(KERN_INFO "%s: Registering KXTJ9 accelerometer\n", __func__);

	gpio_direction_input(KXTJ9_GPIO_IRQ);

#ifdef CONFIG_MACH_OMAP_OVATION
	if (system_rev < OVATION_EVT1A) {
		printk(KERN_INFO "kxtj9 i2c address = 0xf \n");
		kxtj9_i2c_boardinfo.addr = 0xf;
	}
#endif
	kxtj9_i2c_boardinfo.irq = gpio_to_irq(KXTJ9_GPIO_IRQ);
	i2c_register_board_info(1, &kxtj9_i2c_boardinfo, 1);

	return 0;
}
#else
inline int __init bn_accel_init(void) { return 0; }
#endif

#if defined(CONFIG_TOUCHSCREEN_FT5X06) || defined(CONFIG_TOUCHSCREEN_FT5X06_MODULE)
static struct ft5x06_ts_platform_data ft5x06_platform_data = {
	.irqflags			= IRQF_TRIGGER_FALLING,
	.irq_gpio			= TOUCHPANEL_GPIO_IRQ,
	.irq_gpio_flags		= GPIOF_IN,
	.reset_gpio			= TOUCHPANEL_GPIO_RESET,
	.reset_gpio_flags	= GPIOF_OUT_INIT_LOW,
	.x_max				= machine_is_omap_ovation() ? 1920 : 900,
	.y_max				= machine_is_omap_ovation() ? 1280 : 1440,
	.flags				= machine_is_omap_ovation() ?
						  REVERSE_X_FLAG | REVERSE_Y_FLAG : 0,
	.ignore_id_check	= true,
#if 0 // AM: old pdata left here mostly for reference
	.power_init			= touch_power_init,
	.power_on			= touch_power_on,
	.max_tx_lines		= machine_is_omap_ovation() ? 38 : 32,
	.max_rx_lines		= machine_is_omap_ovation() ? 26 : 20,
	.maxx				= machine_is_omap_ovation() ? 1280 : 900,
	.maxy				= machine_is_omap_ovation() ? 1280 : 1440,
	.use_st				= FT_USE_ST,
	.use_mt				= FT_USE_MT,
	.use_trk_id			= FT_USE_TRACKING_ID,
	.use_sleep			= FT_USE_SLEEP,
	.use_gestures		= 1,
	.request_resources	= bn_touch_request_resources,
	.release_resources	= bn_touch_release_resources,
	.power_on			= bn_touch_power_on,
	.power_off			= bn_touch_power_off,
#endif
};

static struct i2c_board_info __initdata ft5x06_i2c_boardinfo = {
	I2C_BOARD_INFO("ft5x06_ts", 0x70 >> 1),
	.platform_data = &ft5x06_platform_data,
};

int __init bn_touch_init(void)
{
	printk(KERN_INFO "%s: Registering touch controller device\n", __func__);

	ft5x06_i2c_boardinfo.irq = gpio_to_irq(TOUCHPANEL_GPIO_IRQ);
	i2c_register_board_info(3, &ft5x06_i2c_boardinfo, 1);

	return 0;
}
#else
inline int __init bn_touch_init(void) { return 0; }
#endif
