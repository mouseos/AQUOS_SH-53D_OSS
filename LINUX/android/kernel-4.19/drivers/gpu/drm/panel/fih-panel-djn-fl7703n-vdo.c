/*
 * Copyright (c) 2015 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/backlight.h>
#include <drm/drmP.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_panel.h>

#include <linux/gpio/consumer.h>
#include <linux/regulator/consumer.h>

#include <video/mipi_display.h>
#include <video/of_videomode.h>
#include <video/videomode.h>

#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/of_graph.h>
#include <linux/platform_device.h>

#define CONFIG_MTK_PANEL_EXT
#if defined(CONFIG_MTK_PANEL_EXT)
#include "../mediatek/mtk_panel_ext.h"
#include "../mediatek/mtk_log.h"
#include "../mediatek/mtk_drm_graphics_base.h"
#endif

#ifdef CONFIG_MTK_ROUND_CORNER_SUPPORT
#include "../mediatek/mtk_corner_pattern/mtk_data_hw_roundedpattern.h"
#endif

#include <linux/proc_fs.h>
#include <linux/poll.h>
#define FIH_PROC_DIR_LCM0 "AllHWList/LCM0"
#define FIH_PROC_PATH_BRIDGE_PING "ping"

extern wait_queue_head_t log_wait;
int bl_onoff = 1;
int old_bl_onoff = 0;
static __poll_t fih_ping_poll(struct file *file, poll_table *wait)
{
	pr_debug("[Kernel/LCM]%s, %d: <-- START\n", __func__, __LINE__);

	poll_wait(file, &log_wait, wait);

	if (old_bl_onoff != bl_onoff)
	{
		pr_debug("[Kernel/LCM]%s, %d: old_bl_onoff(%d) != bl_onoff(%d), return POLLPRI\n", __func__, __LINE__, old_bl_onoff, bl_onoff);
		old_bl_onoff = bl_onoff;
		return POLLPRI;
	}
	else
	{
		//pr_err("[HL]%s, %d: old_bl_onoff == bl_onoff, return 0\n", __func__, __LINE__);
		return 0;
	}
}

static ssize_t fih_ping_write_proc(struct file *file, const char __user *buffer,
                    size_t count, loff_t *offp)
{
	pr_debug("[Kernel/LCM]%s, %d\n", __func__, __LINE__);

	return 1;
}

static int fih_ping_read_proc(struct seq_file *m, void *v)
{
  pr_debug("[Kernel/LCM]%s, %d: bl_onoff = %d <-- START\n", __func__, __LINE__, bl_onoff);

  seq_printf(m, "%d\n", bl_onoff);

  pr_debug("[Kernel/LCM]%s, %d: <-- END\n", __func__, __LINE__);

  return 0;
}

static int fih_ping_proc_open(struct inode *inode, struct file *file)
{
  return single_open(file, fih_ping_read_proc, NULL);
}

static struct file_operations ping_file_ops = {
  .owner          = THIS_MODULE,
  .open		= fih_ping_proc_open,
  .read		= seq_read,
  .write		= fih_ping_write_proc,
  .llseek		= seq_lseek,
  .release	= seq_release,
  .poll		=	fih_ping_poll,
};

/* ----------------------------------------------------------------- */
/* AW37501 Bias Power Implementations */
/* ----------------------------------------------------------------- */
#define I2C_BIAS_ID_NAME  "aw37501"

#if defined(CONFIG_MTK_LEGACY)
#err
#define AW37501_I2C_BUSNUM            7
#define AW37501_ADDR                  0x3E
static struct i2c_board_info aw37501_board_info __initdata = {
	I2C_BOARD_INFO(I2C_BIAS_ID_NAME, AW37501_ADDR)
};
#else
static const struct of_device_id lcm_of_match_bias[] = {
		{.compatible = "mediatek,i2c_lcd_bias"},
		{},
};
MODULE_DEVICE_TABLE(of, lcm_of_match_bias);
#endif

/*static struct i2c_client *aw37501_i2c_client;*/
static struct i2c_client *aw37501_i2c_client;

static int aw37501_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int aw37501_remove(struct i2c_client *client);

struct aw37501_dev {
	struct i2c_client *client;
};

static const struct i2c_device_id aw37501_id[] = {
	{I2C_BIAS_ID_NAME, 0},
	{}
};

static struct i2c_driver aw37501_iic_driver = {
	.id_table = aw37501_id,
	.probe = aw37501_probe,
	.remove = aw37501_remove,
	.driver = {
		   .owner = THIS_MODULE,
		   .name = I2C_BIAS_ID_NAME,
#if !defined(CONFIG_MTK_LEGACY)
			.of_match_table = lcm_of_match_bias,
#endif
		   },
};

static int aw37501_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	pr_err("[Kernel/LCM] %s enter, info==>name=%s addr=0x%x\n", __func__, client->name, client->addr);
	aw37501_i2c_client = client;
	return 0;
}

static int aw37501_remove(struct i2c_client *client)
{
	pr_err("[Kernel/LCM] %s enter\n", __func__);
	aw37501_i2c_client = NULL;
	i2c_unregister_device(client);
	return 0;
}

/*static int aw37501_write_bytes(unsigned char addr, unsigned char value)*/
int aw37501_write_bytes(unsigned char addr, unsigned char value)
{
	int ret = 0;
	struct i2c_client *client = aw37501_i2c_client;
	char write_data[2] = { 0 };

	write_data[0] = addr;
	write_data[1] = value;
	ret = i2c_master_send(client, write_data, 2);
	if (ret < 0)
		pr_err("[Kernel/LCM] AW37501 write data fail !!\n");
	return ret;
}

static int __init aw37501_iic_init(void)
{
	pr_err("[Kernel/LCM] %s enter\n", __func__);
#if defined(CONFIG_MTK_LEGACY)
	i2c_register_board_info(aw37501_I2C_BUSNUM, &aw37501_board_info, 1);
#endif
	i2c_add_driver(&aw37501_iic_driver);
	pr_err("[Kernel/LCM] %s success\n", __func__);
	return 0;
}

static void __exit aw37501_iic_exit(void)
{
	pr_err("[Kernel/LCM] %s enter\n", __func__);
	i2c_del_driver(&aw37501_iic_driver);
}

module_init(aw37501_iic_init);
module_exit(aw37501_iic_exit);

MODULE_AUTHOR("FIH");
MODULE_DESCRIPTION("MTK AW37501 I2C Driver");
MODULE_LICENSE("GPL");

/* ----------------------------------------------------------------- */
/* AW99703 Backlight Driver Implementations */
/* ----------------------------------------------------------------- */
#define I2C_BACKLIGHT_ID_NAME  "aw99703"

#if defined(CONFIG_MTK_LEGACY)
#err
#define AW99703_I2C_BUSNUM    7
#define AW99703_ADDR          0x36
static struct i2c_board_info aw99703_board_info __initdata = {
	I2C_BOARD_INFO(I2C_BACKLIGHT_ID_NAME, AW99703_ADDR)
};
#else
static const struct of_device_id lcm_of_match_backlight[] = {
		{.compatible = "mediatek,i2c_lcd_backlight"},
		{},
};
MODULE_DEVICE_TABLE(of, lcm_of_match_backlight);
#endif

static struct i2c_client *aw99703_i2c_client;

static int aw99703_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int aw99703_remove(struct i2c_client *client);

struct aw99703_dev {
	struct i2c_client *client;
};

static const struct i2c_device_id aw99703_id[] = {
	{I2C_BACKLIGHT_ID_NAME, 0},
	{}
};

static struct i2c_driver aw99703_iic_driver = {
	.id_table = aw99703_id,
	.probe = aw99703_probe,
	.remove = aw99703_remove,
	.driver = {
		   .owner = THIS_MODULE,
		   .name = I2C_BACKLIGHT_ID_NAME,
#if !defined(CONFIG_MTK_LEGACY)
			.of_match_table = lcm_of_match_backlight,
#endif
		   },
};

static int aw99703_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	pr_err("[Kernel/LCM] %s enter, info==>name=%s addr=0x%x\n", __func__, client->name, client->addr);
	aw99703_i2c_client = client;
	return 0;
}

static int aw99703_remove(struct i2c_client *client)
{
	pr_err("[Kernel/LCM] %s enter\n", __func__);
	aw99703_i2c_client = NULL;
	i2c_unregister_device(client);
	return 0;
}

static int aw99703_write_bytes(unsigned char addr, unsigned char value)
{
	int ret = 0;
	struct i2c_client *client = aw99703_i2c_client;
	char write_data[2] = { 0 };

	write_data[0] = addr;
	write_data[1] = value;
	ret = i2c_master_send(client, write_data, 2);
	if (ret < 0)
		pr_err("[Kernel/LCM] AW99703 write data fail !!\n");
	return ret;
}

void aw99703_init_reg(void)
{
	aw99703_write_bytes(0x02, 0x01); // MODE
	aw99703_write_bytes(0x06, 0x07); // Brightness Register LSBs (3bits)
	aw99703_write_bytes(0x07, 0xFF); // Brightness Register MSBs (8bits)
	aw99703_write_bytes(0x08, 0x93); // PWM Control Register
	aw99703_write_bytes(0x03, 0xB7); // LEDCUR Enable
}

static int __init aw99703_iic_init(void)
{
	pr_err("[Kernel/LCM] %s enter\n", __func__);
#if defined(CONFIG_MTK_LEGACY)
	i2c_register_board_info(AW99703_I2C_BUSNUM, &aw99703_board_info, 1);
#endif
	i2c_add_driver(&aw99703_iic_driver);
	pr_err("[Kernel/LCM] %s success\n", __func__);
	return 0;
}

static void __exit aw99703_iic_exit(void)
{
	pr_err("[Kernel/LCM] %s enter\n", __func__);
	i2c_del_driver(&aw99703_iic_driver);
}

module_init(aw99703_iic_init);
module_exit(aw99703_iic_exit);

MODULE_AUTHOR("FIH");
MODULE_DESCRIPTION("AW99703 I2C Driver");
/* ----------------------------------------------------------------- */
/* VDDIO */
/* ----------------------------------------------------------------- */
#if 0
static struct regulator *lcm_vtp;
/* get LDO supply */
static int lcm_get_vtp_supply(struct device* dev)
{
	int ret;
	struct regulator *lcm_vtp_ldo;
	pr_err("[Kernel/LCM] %s enter\n", __func__);

	lcm_vtp_ldo = regulator_get(NULL, "vtp");

	if (IS_ERR(lcm_vtp_ldo)) {
		ret = PTR_ERR(lcm_vtp_ldo);
		pr_err("[Kernel/LCM] failed to get lcm_vtp\n");
		return ret;
	}
	pr_err("[Kernel/LCM] lcm get supply ok.\n");

	/* get current voltage settings */
	ret = regulator_get_voltage(lcm_vtp_ldo);
	pr_err("[Kernel/LCM] lcm_vtp voltage = %d in LK stage\n", ret);

	//Enable ldo2, touch  may disable this regulator
	ret = regulator_enable(lcm_vtp_ldo);
	if (!ret) {
		pr_err("[Kernel/LCM] lcm_vtp enable successfully.");
	} else {
		pr_err("[Kernel/LCM] failed to enable lcm_vtp: %d", ret);
	}

	lcm_vtp = lcm_vtp_ldo;

	return ret;
}

static int lcm_vtp_supply_enable(void)
{
	int ret;
	unsigned int volt;
	pr_err("[Kernel/LCM] %s enter\n", __func__);

	if (lcm_vtp == NULL)
		return 0;

	ret = regulator_set_voltage(lcm_vtp, 1800000, 1800000);
	if (ret != 0) {
		pr_err("[Kernel/LCM] failed to set lcm_vtp: %d\n", ret);
	}
	volt = regulator_get_voltage(lcm_vtp);
	if (volt == 1800000)
		pr_err("[Kernel/LCM] check voltage=1.8V pass!\n");
	else
		pr_err("[Kernel/LCM] check voltage=1.8V fail! (voltage: %d)\n", volt);

	ret = regulator_enable(lcm_vtp);
	if (ret != 0) {
		pr_err("[Kernel/LCM] failed to enable lcm_vtp: %d\n", ret);
		return ret;
	}

	return ret;
}

static int lcm_vtp_supply_disable(void)
{
	int ret = 0;
	unsigned int isenable;
	pr_err("[Kernel/LCM] %s enter\n", __func__);

	if (lcm_vtp == NULL)
		return 0;

	/* disable regulator */
	isenable = regulator_is_enabled(lcm_vtp);
	pr_err("[Kernel/LCM] LCM query regulator enable status[0x%x]\n", isenable);

	if (isenable) {
		ret = regulator_disable(lcm_vtp);
		if (ret != 0) {
			pr_err("[Kernel/LCM] LCM failed to disable lcm_vtp: %d\n", ret);
			return ret;
		}
		/* verify */
		isenable = regulator_is_enabled(lcm_vtp);
		if (!isenable)
			pr_err("[Kernel/LCM] LCM regulator disable pass\n");
	}

	return ret;
}
#endif

/* ----------------------------------------------------------------- */
/* LCM */
/* ----------------------------------------------------------------- */
struct lcm {
	struct device *dev;
	struct drm_panel panel;
	struct backlight_device *backlight;
	struct gpio_desc *reset_gpio, *reset_gpio_fps;
	struct gpio_desc *bias_pos, *bias_neg;

	bool prepared;
	bool enabled;

	int error;

	struct gpio_desc *lcm_enable_gpio;
	struct gpio_desc *pm_enable_gpio;
};

#define lcm_dcs_write_seq(ctx, seq...) \
({\
	const u8 d[] = { seq };\
	BUILD_BUG_ON_MSG(ARRAY_SIZE(d) > 64, "DCS sequence too big for stack");\
	lcm_dcs_write(ctx, d, ARRAY_SIZE(d));\
})

#define lcm_dcs_write_seq_static(ctx, seq...) \
({\
	static const u8 d[] = { seq };\
	lcm_dcs_write(ctx, d, ARRAY_SIZE(d));\
})

static inline struct lcm *panel_to_lcm(struct drm_panel *panel)
{
	return container_of(panel, struct lcm, panel);
}

static void lcm_dcs_write(struct lcm *ctx, const void *data, size_t len)
{
	struct mipi_dsi_device *dsi = to_mipi_dsi_device(ctx->dev);
	ssize_t ret;
	char *addr;

	if (ctx->error < 0)
		return;

	addr = (char *)data;
	if ((int)*addr < 0xB0)
		ret = mipi_dsi_dcs_write_buffer(dsi, data, len);
	else
		ret = mipi_dsi_generic_write(dsi, data, len);
	if (ret < 0) {
		dev_err(ctx->dev, "error %zd writing seq: %ph\n", ret, data);
		ctx->error = ret;
	}
}

#ifdef PANEL_SUPPORT_READBACK
static int lcm_dcs_read(struct lcm *ctx, u8 cmd, void *data, size_t len)
{
	struct mipi_dsi_device *dsi = to_mipi_dsi_device(ctx->dev);
	ssize_t ret;

	if (ctx->error < 0)
		return 0;

	ret = mipi_dsi_dcs_read(dsi, cmd, data, len);
	if (ret < 0) {
		dev_err(ctx->dev, "error %d reading dcs seq:(%#x)\n", ret, cmd);
		ctx->error = ret;
	}

	return ret;
}

static void lcm_panel_get_data(struct lcm *ctx)
{
	u8 buffer[3] = {0};
	static int ret;

	if (ret == 0) {
		ret = lcm_dcs_read(ctx,  0x0A, buffer, 1);
		dev_info(ctx->dev, "return %d data(0x%08x) to dsi engine\n",
			 ret, buffer[0] | (buffer[1] << 8));
	}
}
#endif

static void lcm_panel_init(struct lcm *ctx)
{
	pr_err("[Kernel/LCM] %s enter\n", __func__);

	lcm_dcs_write_seq_static(ctx, 0xB9, 0xF1, 0x12, 0x87); // Set EXTC
	lcm_dcs_write_seq_static(ctx, 0xB2, 0x7C, 0x13, 0x78); //Set RSO
	lcm_dcs_write_seq_static(ctx, 0xB3, 0x10, 0x10, 0x28, 0x28, 0x03, 0xFF, 0x00, 0x00, 0x00, 0x00); // SET RGB
	lcm_dcs_write_seq_static(ctx, 0xB4, 0x80); //Set Panel Inversion Zig-zag inversion type D
	lcm_dcs_write_seq_static(ctx, 0xB5, 0x0F, 0x0F); //Set BGP
	lcm_dcs_write_seq_static(ctx, 0xB6, 0x53, 0x53); // Set VCOM
	lcm_dcs_write_seq_static(ctx, 0xB8, 0x76); // Set ECP
	lcm_dcs_write_seq_static(ctx, 0xBA, 0x33, 0x81, 0x05, 0xF9, 0x0E, 0x0E, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0x25, 0x00, 0x90, 0x0A, 0x00, 0x00, 0x01, 0x4F, 0x01, 0x00, 0x00, 0x37); // Set DSI 
	lcm_dcs_write_seq_static(ctx, 0xBC, 0x47); // Set VDC
	lcm_dcs_write_seq_static(ctx, 0xBF, 0x02, 0x11, 0x00); // Set PCR
	lcm_dcs_write_seq_static(ctx, 0xC0, 0x73, 0x73, 0x50, 0x50, 0x20, 0x00, 0x12, 0x63, 0x00); // Set SCR
	lcm_dcs_write_seq_static(ctx, 0xC1, 0x73, 0x80, 0x32, 0x32, 0x77, 0xF1, 0x77, 0x77, 0xCC, 0xCC, 0xFF, 0xFF, 0x11, 0x11, 0x00, 0x00, 0x32); // Set POWER
	lcm_dcs_write_seq_static(ctx, 0xC7, 0xB0, 0x00, 0x0A, 0x00, 0x00, 0x02, 0x00, 0x00, 0xED, 0xC7, 0x00, 0xA5);
	lcm_dcs_write_seq_static(ctx, 0xC8, 0x10, 0x40, 0x1E, 0x03); // Set SCR
	lcm_dcs_write_seq_static(ctx, 0xCC, 0x0B); // Set Panel
	lcm_dcs_write_seq_static(ctx, 0xE0, 0x00, 0x14, 0x1B, 0x2D, 0x30, 0x3F, 0x54, 0x3F, 0x09, 0x0E, 0x0D, 0x11, 0x12, 0x10, 0x12, 0x11, 0x19, 0x00, 0x14, 0x1B, 0x2D, 0x30, 0x3F, 0x54, 0x3F, 0x09, 0x0E, 0x0D, 0x11, 0x12, 0x10, 0x12, 0x11, 0x19); // Set Gamma2.2
	lcm_dcs_write_seq_static(ctx, 0xE1, 0x11, 0x11, 0x91, 0x00, 0x00, 0x00, 0x00); // SET
	lcm_dcs_write_seq_static(ctx, 0xE3, 0x03, 0x03, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x84, 0xC0, 0x10); // SET EQ
	lcm_dcs_write_seq_static(ctx, 0xE9, 0xC8, 0x10, 0x05, 0x00, 0x00, 0x80, 0x81, 0x12, 0x31, 0x23, 0x2F, 0x83, 0x80, 0x81, 0x27, 0x0A, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0xBA, 0x64, 0x20, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x08, 0x88, 0xBA, 0x75, 0x31, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x18, 0x88, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00); // SET GIP
	lcm_dcs_write_seq_static(ctx, 0xEA, 0x97, 0x0C, 0x82, 0x02, 0x0D, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x57, 0x13, 0x81, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0xBA, 0x88, 0x46, 0x02, 0x80, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0xBA, 0x88, 0x23, 0x30, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00); // SET GIP2
	lcm_dcs_write_seq_static(ctx, 0xEF, 0xFF, 0xFF, 0x01); //

	//Sleep-Out
	lcm_dcs_write_seq_static(ctx, 0x11);
	msleep(120);
	//Display On
	lcm_dcs_write_seq_static(ctx, 0x29);
	msleep(20);
}

static int lcm_disable(struct drm_panel *panel)
{
	struct lcm *ctx = panel_to_lcm(panel);
	pr_err("[Kernel/LCM] %s enter\n", __func__);

	if (!ctx->enabled)
		return 0;

	if (ctx->backlight) {
		ctx->backlight->props.power = FB_BLANK_POWERDOWN;
		backlight_update_status(ctx->backlight);
	}

	ctx->pm_enable_gpio =
		devm_gpiod_get(ctx->dev, "pm-enable", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->pm_enable_gpio)) {
		dev_err(ctx->dev, "%s: cannot get pm_enable_gpio %ld\n",
			__func__, PTR_ERR(ctx->pm_enable_gpio));
	} else {
		gpiod_set_value(ctx->pm_enable_gpio, 0);
		devm_gpiod_put(ctx->dev, ctx->pm_enable_gpio);
	}

	ctx->enabled = false;
	bl_onoff = 0;
	pr_err("[Kernel/LCM]  bl_onoff, %d \n", bl_onoff);

	return 0;
}

static int lcm_unprepare(struct drm_panel *panel)
{
	struct lcm *ctx = panel_to_lcm(panel);
	pr_err("[Kernel/LCM] %s enter\n", __func__);

	if (!ctx->prepared)
		return 0;

	lcm_dcs_write_seq_static(ctx, 0x28);
	msleep(20);
	lcm_dcs_write_seq_static(ctx, 0x10);
	msleep(82);

	ctx->error = 0;
	ctx->prepared = false;

	ctx->reset_gpio =
		devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio)) {
		dev_err(ctx->dev, "%s: cannot get reset_gpio %ld\n",
			__func__, PTR_ERR(ctx->reset_gpio));
		return PTR_ERR(ctx->reset_gpio);
	}
	gpiod_set_value(ctx->reset_gpio, 0);
	devm_gpiod_put(ctx->dev, ctx->reset_gpio);

	ctx->reset_gpio_fps =
		devm_gpiod_get(ctx->dev, "resetfps", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio_fps)) {
		dev_err(ctx->dev, "%s: cannot get reset_gpio_fps %ld\n",
			__func__, PTR_ERR(ctx->reset_gpio_fps));
		return PTR_ERR(ctx->reset_gpio_fps);
	}
	gpiod_set_value(ctx->reset_gpio_fps, 0);
	devm_gpiod_put(ctx->dev, ctx->reset_gpio_fps);

	ctx->bias_neg = devm_gpiod_get_index(ctx->dev,
		"bias", 1, GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->bias_neg)) {
		dev_err(ctx->dev, "%s: cannot get bias_neg %ld\n",
			__func__, PTR_ERR(ctx->bias_neg));
		return PTR_ERR(ctx->bias_neg);
	}
	gpiod_set_value(ctx->bias_neg, 0);
	devm_gpiod_put(ctx->dev, ctx->bias_neg);
	udelay(2000);

	ctx->bias_pos = devm_gpiod_get_index(ctx->dev,
		"bias", 0, GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->bias_pos)) {
		dev_err(ctx->dev, "%s: cannot get bias_pos %ld\n",
			__func__, PTR_ERR(ctx->bias_pos));
		return PTR_ERR(ctx->bias_pos);
	}
	gpiod_set_value(ctx->bias_pos, 0);
	devm_gpiod_put(ctx->dev, ctx->bias_pos);
	udelay(5000);

	ctx->lcm_enable_gpio =
		devm_gpiod_get(ctx->dev, "lcm-enable", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->lcm_enable_gpio)) {
		dev_err(ctx->dev, "%s: cannot get lcm_enable_gpio %ld\n",
			__func__, PTR_ERR(ctx->lcm_enable_gpio));
		return PTR_ERR(ctx->lcm_enable_gpio);
	}
	gpiod_set_value(ctx->lcm_enable_gpio, 0);
	devm_gpiod_put(ctx->dev, ctx->lcm_enable_gpio);
#if 0
	lcm_vtp_supply_disable();
#endif

	return 0;
}

static int lcm_prepare(struct drm_panel *panel)
{
	struct lcm *ctx = panel_to_lcm(panel);
	int ret;
	pr_err("[Kernel/LCM] %s enter\n", __func__);

	if (ctx->prepared)
		return 0;

	ctx->lcm_enable_gpio =
		devm_gpiod_get(ctx->dev, "lcm-enable", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->lcm_enable_gpio)) {
		dev_err(ctx->dev, "%s: cannot get lcm_enable_gpio %ld\n",
			__func__, PTR_ERR(ctx->lcm_enable_gpio));
		return PTR_ERR(ctx->lcm_enable_gpio);
	}
	gpiod_set_value(ctx->lcm_enable_gpio, 1);
	devm_gpiod_put(ctx->dev, ctx->lcm_enable_gpio);
	msleep(1);

	ctx->reset_gpio =
		devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio)) {
		dev_err(ctx->dev, "%s: cannot get reset_gpio %ld\n",
			__func__, PTR_ERR(ctx->reset_gpio));
		return PTR_ERR(ctx->reset_gpio);
	}
	gpiod_set_value(ctx->reset_gpio, 1);
	devm_gpiod_put(ctx->dev, ctx->reset_gpio);

	ctx->reset_gpio_fps =
		devm_gpiod_get(ctx->dev, "resetfps", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio_fps)) {
		dev_err(ctx->dev, "%s: cannot get reset_gpio_fps %ld\n",
			__func__, PTR_ERR(ctx->reset_gpio_fps));
		return PTR_ERR(ctx->reset_gpio_fps);
	}
	gpiod_set_value(ctx->reset_gpio_fps, 1);
	devm_gpiod_put(ctx->dev, ctx->reset_gpio_fps);
	msleep(1);

	ctx->reset_gpio =
		devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_LOW);
	if (IS_ERR(ctx->reset_gpio)) {
		dev_err(ctx->dev, "%s: cannot get reset_gpio %ld\n",
			__func__, PTR_ERR(ctx->reset_gpio));
		return PTR_ERR(ctx->reset_gpio);
	}
	gpiod_set_value(ctx->reset_gpio, 0);
	devm_gpiod_put(ctx->dev, ctx->reset_gpio);

	ctx->reset_gpio_fps =
		devm_gpiod_get(ctx->dev, "resetfps", GPIOD_OUT_LOW);
	if (IS_ERR(ctx->reset_gpio_fps)) {
		dev_err(ctx->dev, "%s: cannot get reset_gpio_fps %ld\n",
			__func__, PTR_ERR(ctx->reset_gpio_fps));
		return PTR_ERR(ctx->reset_gpio_fps);
	}
	gpiod_set_value(ctx->reset_gpio_fps, 0);
	devm_gpiod_put(ctx->dev, ctx->reset_gpio_fps);
	udelay(10);

	ctx->reset_gpio =
		devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio)) {
		dev_err(ctx->dev, "%s: cannot get reset_gpio %ld\n",
			__func__, PTR_ERR(ctx->reset_gpio));
		return PTR_ERR(ctx->reset_gpio);
	}
	gpiod_set_value(ctx->reset_gpio, 1);
	devm_gpiod_put(ctx->dev, ctx->reset_gpio);

	ctx->reset_gpio_fps =
		devm_gpiod_get(ctx->dev, "resetfps", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio_fps)) {
		dev_err(ctx->dev, "%s: cannot get reset_gpio_fps %ld\n",
			__func__, PTR_ERR(ctx->reset_gpio_fps));
		return PTR_ERR(ctx->reset_gpio_fps);
	}
	gpiod_set_value(ctx->reset_gpio_fps, 1);
	devm_gpiod_put(ctx->dev, ctx->reset_gpio_fps);

	msleep(10);

	// VPOS
	ctx->bias_pos = devm_gpiod_get_index(ctx->dev,
		"bias", 0, GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->bias_pos)) {
		dev_err(ctx->dev, "%s: cannot get bias_pos %ld\n",
			__func__, PTR_ERR(ctx->bias_pos));
		return PTR_ERR(ctx->bias_pos);
	}
	gpiod_set_value(ctx->bias_pos, 1);
	devm_gpiod_put(ctx->dev, ctx->bias_pos);
	udelay(3000);

	// VNEG
	ctx->bias_neg = devm_gpiod_get_index(ctx->dev,
		"bias", 1, GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->bias_neg)) {
		dev_err(ctx->dev, "%s: cannot get bias_neg %ld\n",
			__func__, PTR_ERR(ctx->bias_neg));
		return PTR_ERR(ctx->bias_neg);
	}
	gpiod_set_value(ctx->bias_neg, 1);
	devm_gpiod_put(ctx->dev, ctx->bias_neg);
	udelay(10000);

	// Set VPOS voltage to 5.5v
	ret = aw37501_write_bytes(0x00, 0x0f);
	if (ret < 0) {
		pr_err("[Kernel/LCM] panel AW37501 VPOS i2c write error\n");
	}

	// Set VNEG voltage to -5.5v
	ret = aw37501_write_bytes(0x01, 0x0f);
	if (ret < 0) {
		pr_err("[Kernel/LCM] panel AW37501 VNEG i2c write error\n");
	}

	msleep(5);

	lcm_panel_init(ctx);

	ret = ctx->error;
	if (ret < 0)
		lcm_unprepare(panel);

	ctx->prepared = true;

#if defined(CONFIG_MTK_PANEL_EXT)
	mtk_panel_tch_rst(panel);
#endif
#ifdef PANEL_SUPPORT_READBACK
	lcm_panel_get_data(ctx);
#endif

	return ret;
}

static int lcm_enable(struct drm_panel *panel)
{
	struct lcm *ctx = panel_to_lcm(panel);
	pr_err("[Kernel/LCM] %s enter\n", __func__);

	if (ctx->enabled)
		return 0;

	if (ctx->backlight) {
		ctx->backlight->props.power = FB_BLANK_UNBLANK;
		backlight_update_status(ctx->backlight);
	}

	mdelay(100);

	ctx->pm_enable_gpio =
		devm_gpiod_get(ctx->dev, "pm-enable", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->pm_enable_gpio)) {
		dev_err(ctx->dev, "%s: cannot get pm_enable_gpio %ld\n",
			__func__, PTR_ERR(ctx->pm_enable_gpio));
	} else {
		gpiod_set_value(ctx->pm_enable_gpio, 1);
		devm_gpiod_put(ctx->dev, ctx->pm_enable_gpio);
	}
	aw99703_init_reg();

	ctx->enabled = true;

	bl_onoff = 1;
	pr_err("[Kernel/LCM]  bl_onoff, %d \n", bl_onoff);
	return 0;
}

#define HFP (60)
#define HSA (16)
#define HBP (48)
#define VFP (16)
#define VSA (4)
#define VBP (14)
#define VAC (1520)
#define HAC (720)
static u32 fake_heigh = 1520;
static u32 fake_width = 720;
#define PHYSICAL_WIDTH_UM  (62053)
#define PHYSICAL_HEIGHT_UM (130889)

static bool need_fake_resolution;

static struct drm_display_mode default_mode = {
	.clock = 78695,
	.hdisplay = HAC,
	.hsync_start = HAC + HFP,
	.hsync_end = HAC + HFP + HSA,
	.htotal = HAC + HFP + HSA + HBP,
	.vdisplay = VAC,
	.vsync_start = VAC + VFP,
	.vsync_end = VAC + VFP + VSA,
	.vtotal = VAC + VFP + VSA + VBP,
	.vrefresh = 60,
};

#if defined(CONFIG_MTK_PANEL_EXT)
static int panel_ext_reset(struct drm_panel *panel, int on)
{
	struct lcm *ctx = panel_to_lcm(panel);

	ctx->reset_gpio =
		devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio)) {
		dev_err(ctx->dev, "%s: cannot get reset_gpio %ld\n",
			__func__, PTR_ERR(ctx->reset_gpio));
		return PTR_ERR(ctx->reset_gpio);
	}
	gpiod_set_value(ctx->reset_gpio, on);
	devm_gpiod_put(ctx->dev, ctx->reset_gpio);

	ctx->reset_gpio_fps =
		devm_gpiod_get(ctx->dev, "resetfps", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio_fps)) {
		dev_err(ctx->dev, "%s: cannot get reset_gpio_fps %ld\n",
			__func__, PTR_ERR(ctx->reset_gpio_fps));
		return PTR_ERR(ctx->reset_gpio_fps);
	}
	gpiod_set_value(ctx->reset_gpio_fps, on);
	devm_gpiod_put(ctx->dev, ctx->reset_gpio_fps);

	return 0;
}

static int panel_ata_check(struct drm_panel *panel)
{
	struct lcm *ctx = panel_to_lcm(panel);
	struct mipi_dsi_device *dsi = to_mipi_dsi_device(ctx->dev);
	unsigned char data[3] = {0x00, 0x00, 0x00};
	unsigned char id[3] = {0x00, 0x00, 0x00};
	ssize_t ret;

	ret = mipi_dsi_dcs_read(dsi, 0x4, data, 3);
	if (ret < 0) {
		pr_err("%s error\n", __func__);
		return 0;
	}

	DDPINFO("ATA read data %x %x %x\n", data[0], data[1], data[2]);

	if (data[0] == id[0] &&
			data[1] == id[1] &&
			data[2] == id[2])
		return 1;

	DDPINFO("ATA expect read data is %x %x %x\n",
			id[0], id[1], id[2]);

	return 0;
}

static int lcm_setbacklight_cmdq(void *dsi, dcs_write_gce cb,
	void *handle, unsigned int level)
{
	char bl_tb0[] = {0x51, 0xFF};

	bl_tb0[1] = level;

	if (!cb)
		return -1;

	cb(dsi, handle, bl_tb0, ARRAY_SIZE(bl_tb0));

	return 0;
}

static int lcm_get_virtual_heigh(void)
{
	return VAC;
}

static int lcm_get_virtual_width(void)
{
	return HAC;
}

static struct mtk_panel_params ext_params = {
	.pll_clk = 247,
	.cust_esd_check = 0,
	.esd_check_enable = 1,
	.lcm_esd_check_table[0] = {
		.cmd = 0x0a,
		.count = 1,
		.para_list[0] = 0x9c,
	},
	.physical_width_um = PHYSICAL_WIDTH_UM,
	.physical_height_um = PHYSICAL_HEIGHT_UM,
	.phy_timcon = {
		.clk_hs_post = 36,
	},
};

static struct mtk_panel_funcs ext_funcs = {
	.reset = panel_ext_reset,
	.set_backlight_cmdq = lcm_setbacklight_cmdq,
	.ata_check = panel_ata_check,
	.get_virtual_heigh = lcm_get_virtual_heigh,
	.get_virtual_width = lcm_get_virtual_width,
};
#endif

struct panel_desc {
	const struct drm_display_mode *modes;
	unsigned int num_modes;

	unsigned int bpc;

	struct {
		unsigned int width;
		unsigned int height;
	} size;

	struct {
		unsigned int prepare;
		unsigned int enable;
		unsigned int disable;
		unsigned int unprepare;
	} delay;
};

static void change_drm_disp_mode_params(struct drm_display_mode *mode)
{
	if (fake_heigh > 0 && fake_heigh < VAC) {
		mode->vdisplay = fake_heigh;
		mode->vsync_start = fake_heigh + VFP;
		mode->vsync_end = fake_heigh + VFP + VSA;
		mode->vtotal = fake_heigh + VFP + VSA + VBP;
	}
	if (fake_width > 0 && fake_width < HAC) {
		mode->hdisplay = fake_width;
		mode->hsync_start = fake_width + HFP;
		mode->hsync_end = fake_width + HFP + HSA;
		mode->htotal = fake_width + HFP + HSA + HBP;
	}
}

static int lcm_get_modes(struct drm_panel *panel)
{
	struct drm_display_mode *mode;

	if (need_fake_resolution)
		change_drm_disp_mode_params(&default_mode);
	mode = drm_mode_duplicate(panel->drm, &default_mode);
	if (!mode) {
		dev_err(panel->drm->dev, "failed to add mode %ux%ux@%u\n",
			default_mode.hdisplay, default_mode.vdisplay,
			default_mode.vrefresh);
		return -ENOMEM;
	}

	drm_mode_set_name(mode);
	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	drm_mode_probed_add(panel->connector, mode);

	panel->connector->display_info.width_mm = PHYSICAL_WIDTH_UM/1000;
	panel->connector->display_info.height_mm = PHYSICAL_HEIGHT_UM/1000;

	return 1;
}

static const struct drm_panel_funcs lcm_drm_funcs = {
	.disable = lcm_disable,
	.unprepare = lcm_unprepare,
	.prepare = lcm_prepare,
	.enable = lcm_enable,
	.get_modes = lcm_get_modes,
};

static void check_is_need_fake_resolution(struct device *dev)
{
	unsigned int ret = 0;

	ret = of_property_read_u32(dev->of_node, "fake_heigh", &fake_heigh);
	if (ret)
		need_fake_resolution = false;
	ret = of_property_read_u32(dev->of_node, "fake_width", &fake_width);
	if (ret)
		need_fake_resolution = false;
	if (fake_heigh > 0 && fake_heigh < VAC)
		need_fake_resolution = true;
	if (fake_width > 0 && fake_width < HAC)
		need_fake_resolution = true;
}

static int lcm_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct lcm *ctx;
	struct device_node *backlight;
	int ret;
	struct device_node *dsi_node, *remote_node = NULL, *endpoint = NULL;
	pr_err("[Kernel/LCM] fl7703n_hdplus_vdo %s enter\n", __func__);

	struct proc_dir_entry *lcm0_dir;
	lcm0_dir = proc_mkdir (FIH_PROC_DIR_LCM0, NULL);

	pr_err("[Kernel/LCM] start to create proc/%s\n", FIH_PROC_PATH_BRIDGE_PING);
	if (proc_create(FIH_PROC_PATH_BRIDGE_PING, 0, lcm0_dir, &ping_file_ops) == NULL)
    {
        pr_err("[Kernel/LCM] fail to create /proc/%s\n", FIH_PROC_PATH_BRIDGE_PING);
    }
	dsi_node = of_get_parent(dev->of_node);
	if (dsi_node) {
		endpoint = of_graph_get_next_endpoint(dsi_node, NULL);
		if (endpoint) {
			remote_node = of_graph_get_remote_port_parent(endpoint);
			if (!remote_node) {
				pr_err("[Kernel/LCM] No panel connected,skip probe lcm\n");
				return -ENODEV;
			}
			pr_err("[Kernel/LCM] device node name:%s\n", remote_node->name);
		}
	}
	if (remote_node != dev->of_node) {
		pr_err("[Kernel/LCM] %s+ skip probe due to not current lcm\n", __func__);
		return -ENODEV;
	}

	ctx = devm_kzalloc(dev, sizeof(struct lcm), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	mipi_dsi_set_drvdata(dsi, ctx);

	ctx->dev = dev;
	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_SYNC_PULSE
			 | MIPI_DSI_MODE_LPM | MIPI_DSI_MODE_EOT_PACKET;

	backlight = of_parse_phandle(dev->of_node, "backlight", 0);
	if (backlight) {
		ctx->backlight = of_find_backlight_by_node(backlight);
		of_node_put(backlight);

		if (!ctx->backlight)
			return -EPROBE_DEFER;
	}

#if 0
	lcm_get_vtp_supply(dev);
#endif

	ctx->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio)) {
		dev_err(dev, "[Kernel/LCM] %s: cannot get reset-gpios %ld\n",
			__func__, PTR_ERR(ctx->reset_gpio));
		return PTR_ERR(ctx->reset_gpio);
	}
	devm_gpiod_put(dev, ctx->reset_gpio);

	ctx->reset_gpio_fps = devm_gpiod_get(dev, "resetfps", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio_fps)) {
		dev_err(dev, "[Kernel/LCM] %s: cannot get reset-gpios %ld\n",
			__func__, PTR_ERR(ctx->reset_gpio_fps));
		return PTR_ERR(ctx->reset_gpio_fps);
	}
	devm_gpiod_put(dev, ctx->reset_gpio_fps);

	ctx->bias_pos = devm_gpiod_get_index(dev, "bias", 0, GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->bias_pos)) {
		dev_err(dev, "[Kernel/LCM] %s: cannot get bias-pos 0 %ld\n",
			__func__, PTR_ERR(ctx->bias_pos));
		return PTR_ERR(ctx->bias_pos);
	}
	devm_gpiod_put(dev, ctx->bias_pos);

	ctx->bias_neg = devm_gpiod_get_index(dev, "bias", 1, GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->bias_neg)) {
		dev_err(dev, "[Kernel/LCM] %s: cannot get bias-neg 1 %ld\n",
			__func__, PTR_ERR(ctx->bias_neg));
		return PTR_ERR(ctx->bias_neg);
	}
	devm_gpiod_put(dev, ctx->bias_neg);

	ctx->pm_enable_gpio = devm_gpiod_get(dev, "pm-enable", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->pm_enable_gpio)) {
		dev_err(dev, "[Kernel/LCM] %s: cannot get pm-enable-gpios %ld\n",
			__func__, PTR_ERR(ctx->pm_enable_gpio));
	} else {
		devm_gpiod_put(dev, ctx->pm_enable_gpio);
	}

	ctx->prepared = true;
	ctx->enabled = true;

	drm_panel_init(&ctx->panel);
	ctx->panel.dev = dev;
	ctx->panel.funcs = &lcm_drm_funcs;

	ret = drm_panel_add(&ctx->panel);
	if (ret < 0)
		return ret;

	ret = mipi_dsi_attach(dsi);
	if (ret < 0)
		drm_panel_remove(&ctx->panel);

#if defined(CONFIG_MTK_PANEL_EXT)
	mtk_panel_tch_handle_reg(&ctx->panel);
	ret = mtk_panel_ext_create(dev, &ext_params, &ext_funcs, &ctx->panel);
	if (ret < 0)
		return ret;
#endif
	check_is_need_fake_resolution(dev);
	pr_err("[Kernel/LCM] %s success\n", __func__);

	return ret;
}

static int lcm_remove(struct mipi_dsi_device *dsi)
{
	struct lcm *ctx = mipi_dsi_get_drvdata(dsi);

	mipi_dsi_detach(dsi);
	drm_panel_remove(&ctx->panel);
    remove_proc_entry(FIH_PROC_PATH_BRIDGE_PING, NULL);
    pr_err("[Kernel/LCM] %s success\n", __func__);

	return 0;
}

static const struct of_device_id lcm_of_match[] = {
	{ .compatible = "djn,fl7703n_hdplus_vdo", },
	{ }
};

MODULE_DEVICE_TABLE(of, lcm_of_match);

static struct mipi_dsi_driver lcm_driver = {
	.probe = lcm_probe,
	.remove = lcm_remove,
	.driver = {
		.name = "panel-djn-fl7703n-vdo",
		.owner = THIS_MODULE,
		.of_match_table = lcm_of_match,
	},
};

module_mipi_dsi_driver(lcm_driver);

MODULE_AUTHOR("FIH");
MODULE_DESCRIPTION("DJN FL7703N VDO LCD Panel Driver");
MODULE_LICENSE("GPL v2");

