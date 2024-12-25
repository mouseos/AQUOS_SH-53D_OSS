// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2018 MediaTek Inc.
 *
 */

#include <linux/ctype.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/leds.h>
#include <linux/leds_pwm.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>
#include <linux/sched.h>
#include <linux/sched/clock.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/workqueue.h>

#include "leds-mtk-pwm.h"


#define CONFIG_LEDS_BRIGHTNESS_CHANGED
/****************************************************************************
 * variables
 ***************************************************************************/
#undef pr_fmt
#define pr_fmt(fmt) KBUILD_MODNAME " %s(%d) :" fmt, __func__, __LINE__

struct mtk_leds_info;

static int led_level_set(struct led_classdev *led_cdev,
					 enum led_brightness brightness);

struct led_debug_info {
	unsigned long long current_t;
	unsigned long long last_t;
	char buffer[4096];
	int count;
};

struct led_desp {
	int index;
	char name[16];
};

struct leds_desp_info {
	int lens;
	struct led_desp *leds[0];
};

struct led_pwm_info {
	struct pwm_device *pwm;
	struct led_pwm config;
	unsigned long long duty;
};

struct mtk_led_data {
	struct led_desp desp;
	struct led_conf_info	conf;
	int last_level;
	int brightness;
	struct led_pwm_info info;
	struct mtk_leds_info	*parent;
	struct led_debug_info debug;
};

struct mtk_leds_info {
	struct device *dev;
	int			nums;
	struct mtk_led_data leds[0];
};

static DEFINE_MUTEX(leds_mutex);
struct leds_desp_info *leds_info;
static BLOCKING_NOTIFIER_HEAD(mtk_leds_chain_head);

int mtk_leds_register_notifier(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&mtk_leds_chain_head, nb);
}
EXPORT_SYMBOL_GPL(mtk_leds_register_notifier);

int mtk_leds_unregister_notifier(struct notifier_block *nb)

{
	return blocking_notifier_chain_unregister(&mtk_leds_chain_head, nb);
}
EXPORT_SYMBOL_GPL(mtk_leds_unregister_notifier);

int mtk_leds_call_notifier(unsigned long action, void *data)
{
	return blocking_notifier_call_chain(&mtk_leds_chain_head, action, data);
}
EXPORT_SYMBOL_GPL(mtk_leds_call_notifier);


static int call_notifier(int event, struct mtk_led_data *led_dat)
{
	int err;

	err = mtk_leds_call_notifier(event, &led_dat->conf);
	if (err)
		pr_info("notifier_call_chain error\n");
	return err;
}

/****************************************************************************
 * DEBUG MACROS
 ***************************************************************************/

static void led_debug_log(struct mtk_led_data *s_led,
		int level, int mappingLevel)
{
	unsigned long cur_time_mod = 0;
	unsigned long long cur_time_display = 0;
	int ret = 0;

	s_led->debug.current_t = sched_clock();
	cur_time_display = s_led->debug.current_t;
	do_div(cur_time_display, 1000000);
	cur_time_mod = do_div(cur_time_display, 1000);

	ret = snprintf(s_led->debug.buffer + strlen(s_led->debug.buffer),
		4095 - strlen(s_led->debug.buffer),
		"T:%lld.%ld,L:%d L:%d map:%d    ",
		cur_time_display, cur_time_mod,
		s_led->conf.cdev.brightness, level, mappingLevel);

	s_led->debug.count++;

	if (ret < 0 || ret >= 4096) {
		pr_info("print log error!");
		s_led->debug.count = 5;
	}

	if (level == 0 || s_led->debug.count >= 5 ||
		(s_led->debug.current_t - s_led->debug.last_t) > 1000000000) {
		pr_info("%s", s_led->debug.buffer);
		s_led->debug.count = 0;
		s_led->debug.buffer[strlen("[Light] Set directly ") +
			strlen(s_led->conf.cdev.name)] = '\0';
	}

	s_led->debug.last_t = sched_clock();
}


static int getLedDespIndex(char *name)
{
	int i = 0;

	while (i < leds_info->lens) {
		if (!strcmp(name, leds_info->leds[i]->name))
			return i;
		i++;
	}
	return -1;
}


/****************************************************************************
 * driver functions
 ***************************************************************************/

static void __led_pwm_set(struct led_pwm_info *led_info)
{
	int new_duty = led_info->duty;

	if (new_duty > 0 && new_duty < 269){
		new_duty = 269;
	}

	mutex_lock(&leds_mutex);

	pwm_config(led_info->pwm, new_duty, led_info->config.pwm_period_ns);
	if (new_duty == 0)
		pwm_disable(led_info->pwm);
	else
		pwm_enable(led_info->pwm);

	mutex_unlock(&leds_mutex);
}

static const char *proj;

typedef struct {
    int brightness;
    int last_brightness;
} brightness_level_table_t;

brightness_level_table_t brightness_level[256] = {
    { 255, 251},{ 254, 250},{ 253, 250},{ 252, 249},{ 251, 249},{ 250, 248},{ 249, 248},
	{ 248, 247},{ 247, 247},{ 246, 246},{ 245, 246},{ 244, 245},{ 243, 245},{ 242, 244},
	{ 241, 244},{ 240, 243},{ 239, 242},{ 238, 242},{ 237, 241},{ 236, 241},{ 235, 240},
	{ 234, 240},{ 233, 239},{ 232, 239},{ 231, 238},{ 230, 238},{ 229, 237},{ 228, 237},
	{ 227, 236},{ 226, 236},{ 225, 235},{ 224, 234},{ 223, 233},{ 222, 232},{ 221, 231},
	{ 220, 230},{ 219, 228},{ 218, 227},{ 217, 226},{ 216, 225},{ 215, 224},{ 214, 223},
	{ 213, 222},{ 212, 221},{ 211, 220},{ 210, 219},{ 209, 218},{ 208, 216},{ 207, 215},
	{ 206, 214},{ 205, 213},{ 204, 212},{ 203, 211},{ 202, 210},{ 201, 209},{ 200, 208},
	{ 199, 207},{ 198, 206},{ 197, 204},{ 196, 203},{ 195, 202},{ 194, 201},{ 193, 200},
	{ 192, 199},{ 191, 198},{ 190, 197},{ 189, 196},{ 188, 195},{ 187, 195},{ 186, 194},
	{ 185, 193},{ 184, 192},{ 183, 191},{ 182, 190},{ 181, 189},{ 180, 188},{ 179, 187},
	{ 178, 187},{ 177, 186},{ 176, 185},{ 175, 184},{ 174, 183},{ 173, 182},{ 172, 181},
	{ 171, 180},{ 170, 179},{ 169, 178},{ 168, 178},{ 167, 177},{ 166, 176},{ 165, 175},
	{ 164, 174},{ 163, 173},{ 162, 172},{ 161, 172},{ 160, 171},{ 159, 170},{ 158, 169},
	{ 157, 168},{ 156, 167},{ 155, 167},{ 154, 166},{ 153, 165},{ 152, 164},{ 151, 163},
	{ 150, 162},{ 149, 162},{ 148, 161},{ 147, 160},{ 146, 159},{ 145, 158},{ 144, 157},
	{ 143, 157},{ 142, 156},{ 141, 155},{ 140, 154},{ 139, 153},{ 138, 152},{ 137, 152},
	{ 136, 151},{ 135, 150},{ 134, 149},{ 133, 148},{ 132, 147},{ 131, 146},{ 130, 146},
	{ 129, 145},{ 128, 144},{ 127, 143},{ 126, 142},{ 125, 141},{ 124, 141},{ 123, 140},
	{ 122, 139},{ 121, 138},{ 120, 137},{ 119, 136},{ 118, 136},{ 117, 135},{ 116, 134},
	{ 115, 133},{ 114, 132},{ 113, 131},{ 112, 131},{ 111, 130},{ 110, 129},{ 109, 128},
	{ 108, 127},{ 107, 126},{ 106, 126},{ 105, 125},{ 104, 124},{ 103, 123},{ 102, 122},
	{ 101, 122},{ 100, 121},{ 99, 120},{ 98, 119},{ 97, 118},{ 96, 118},{ 95, 117},{ 94, 116},
	{ 93, 115},{ 92, 114},{ 91, 114},{ 90, 113},{ 89, 112},{ 88, 111},{ 87, 110},{ 86, 109},
	{ 85, 108},{ 84, 107},{ 83, 107},{ 82, 106},{ 81, 105},{ 80, 104},{ 79, 103},{ 78, 102},
	{ 77, 101},{ 76, 100},{ 75, 99},{ 74, 98},{ 73, 97},{ 72, 96},{ 71, 96},{ 70, 95},
	{ 69, 94},{ 68, 93},{ 67, 92},{ 66, 91},{ 65, 90},{ 64, 88},{ 63, 87},{ 62, 85},{ 61, 84},
	{ 60, 82},{ 59, 81},{ 58, 79},{ 57, 78},{ 56, 76},{ 55, 75},{ 54, 73},{ 53, 71},{ 52, 69},
	{ 51, 67},{ 50, 65},{ 49, 63},{ 48, 61},{ 47, 59},{ 46, 57},{ 45, 56},{ 44, 54},{ 43, 52},
	{ 42, 50},{ 41, 48},{ 40, 46},{ 39, 44},{ 38, 42},{ 37, 40},{ 36, 38},{ 35, 36},{ 34, 35},
	{ 33, 34},{ 32, 33},{ 31, 32},{ 30, 31},{ 29, 30},{ 28, 28},{ 27, 27},{ 26, 26},{ 25, 25},
	{ 24, 24},{ 23, 23},{ 22, 22},{ 21, 21},{ 20, 20},{ 19, 19},{ 18, 18},{ 17, 17},{ 16, 16},
	{ 15, 15},{ 14, 14},{ 13, 13},{ 12, 12},{ 11, 11},{ 10, 10},{ 9, 9},{ 8, 8},{ 7, 7},
	{ 6, 6},{ 5, 5},{ 4, 4},{ 3, 3},{ 2, 2},{ 1, 1},{ 0, 0}
};

brightness_level_table_t brightness_level_SX4[256] = {
    { 255, 250},{ 254, 250},{ 253, 249},{ 252, 249},{ 251, 248},{ 250, 248},{ 249, 247},
	{ 248, 247},{ 247, 247},{ 246, 246},{ 245, 246},{ 244, 245},{ 243, 245},{ 242, 244},
	{ 241, 244},{ 240, 243},{ 239, 243},{ 238, 243},{ 237, 242},{ 236, 242},{ 235, 241},
	{ 234, 241},{ 233, 240},{ 232, 240},{ 231, 240},{ 230, 239},{ 229, 239},{ 228, 238},
	{ 227, 238},{ 226, 237},{ 225, 237},{ 224, 236},{ 223, 235},{ 222, 234},{ 221, 233},
	{ 220, 231},{ 219, 230},{ 218, 229},{ 217, 228},{ 216, 227},{ 215, 226},{ 214, 225},
	{ 213, 224},{ 212, 222},{ 211, 221},{ 210, 220},{ 209, 219},{ 208, 218},{ 207, 217},
	{ 206, 216},{ 205, 215},{ 204, 213},{ 203, 212},{ 202, 211},{ 201, 210},{ 200, 209},
	{ 199, 208},{ 198, 207},{ 197, 206},{ 196, 204},{ 195, 203},{ 194, 202},{ 193, 201},
	{ 192, 200},{ 191, 199},{ 190, 198},{ 189, 197},{ 188, 196},{ 187, 196},{ 186, 195},
	{ 185, 194},{ 184, 193},{ 183, 192},{ 182, 191},{ 181, 190},{ 180, 189},{ 179, 188},
	{ 178, 188},{ 177, 187},{ 176, 186},{ 175, 185},{ 174, 184},{ 173, 183},{ 172, 182},
	{ 171, 181},{ 170, 180},{ 169, 179},{ 168, 179},{ 167, 178},{ 166, 177},{ 165, 176},
	{ 164, 175},{ 163, 174},{ 162, 173},{ 161, 172},{ 160, 172},{ 159, 171},{ 158, 170},
	{ 157, 169},{ 156, 168},{ 155, 167},{ 154, 166},{ 153, 165},{ 152, 165},{ 151, 164},
	{ 150, 163},{ 149, 162},{ 148, 161},{ 147, 160},{ 146, 159},{ 145, 159},{ 144, 158},
	{ 143, 157},{ 142, 156},{ 141, 155},{ 140, 154},{ 139, 153},{ 138, 152},{ 137, 152},
	{ 136, 151},{ 135, 150},{ 134, 149},{ 133, 148},{ 132, 147},{ 131, 146},{ 130, 146},
	{ 129, 145},{ 128, 144},{ 127, 143},{ 126, 142},{ 125, 141},{ 124, 141},{ 123, 140},
	{ 122, 139},{ 121, 138},{ 120, 137},{ 119, 136},{ 118, 136},{ 117, 135},{ 116, 134},
	{ 115, 133},{ 114, 132},{ 113, 131},{ 112, 131},{ 111, 130},{ 110, 129},{ 109, 128},
	{ 108, 127},{ 107, 126},{ 106, 126},{ 105, 125},{ 104, 124},{ 103, 123},{ 102, 122},
	{ 101, 122},{ 100, 121},{ 99, 120},{ 98, 119},{ 97, 118},{ 96, 118},{ 95, 117},{ 94, 116},
	{ 93, 115},{ 92, 114},{ 91, 114},{ 90, 113},{ 89, 112},{ 88, 111},{ 87, 110},{ 86, 109},
	{ 85, 108},{ 84, 107},{ 83, 107},{ 82, 106},{ 81, 105},{ 80, 104},{ 79, 103},{ 78, 102},
	{ 77, 101},{ 76, 100},{ 75, 99},{ 74, 98},{ 73, 97},{ 72, 96},{ 71, 96},{ 70, 95},
	{ 69, 94},{ 68, 93},{ 67, 92},{ 66, 91},{ 65, 90},{ 64, 89},{ 63, 88},{ 62, 87},{ 61, 86},
	{ 60, 85},{ 59, 84},{ 58, 83},{ 57, 82},{ 56, 81},{ 55, 80},{ 54, 79},{ 53, 78},{ 52, 77},
	{ 51, 76},{ 50, 75},{ 49, 74},{ 48, 72},{ 47, 71},{ 46, 70},{ 45, 68},{ 44, 67},{ 43, 66},
	{ 42, 64},{ 41, 63},{ 40, 62},{ 39, 60},{ 38, 59},{ 37, 58},{ 36, 56},{ 35, 55},{ 34, 54},
	{ 33, 52},{ 32, 51},{ 31, 49},{ 30, 48},{ 29, 46},{ 28, 44},{ 27, 43},{ 26, 41},{ 25, 39},
	{ 24, 38},{ 23, 36},{ 22, 35},{ 21, 33},{ 20, 32},{ 19, 30},{ 18, 29},{ 17, 27},{ 16, 26},
	{ 15, 24},{ 14, 23},{ 13, 21},{ 12, 20},{ 11, 18},{ 10, 17},{ 9, 15},{ 8, 13},{ 7, 11},
	{ 6, 9},{ 5, 7},{ 4, 6},{ 3, 4},{ 2, 3},{ 1, 1},{ 0, 0}
};

static int led_level_pwm_set(struct mtk_led_data *led_dat,
				int brightness)
{
	unsigned int max;
	unsigned long long duty;
	int i = 0;

	if (!strcmp(proj, "SX4")){
		pr_notice("led_level_pwm_set proj property %s", proj);
		for(i = 0; i <= 255; i++)
	    {
	         if( brightness == brightness_level_SX4[i].brightness){
				brightness = brightness_level_SX4[i].last_brightness;
				pr_notice("led_level_pwm_set %s brightness_level_SX4[%d].brightness:%d  new brightness state:%d!\n", proj, i, brightness_level_SX4[i].brightness, brightness);
	            break;
	        }
	    }
	}
	else if (!strcmp(proj, "SX3")){
		pr_notice("led_level_pwm_set proj property %s", proj);
		for(i = 0; i <= 255; i++)
	    {
	         if( brightness == brightness_level[i].brightness){
				brightness = brightness_level[i].last_brightness;
				pr_notice("led_level_pwm_set %s brightness_level[%d].brightness:%d  new brightness state:%d!\n", proj, i, brightness_level[i].brightness, brightness);
	            break;
	        }
	    }

	}


	brightness = min(brightness, led_dat->conf.max_level);
	if (brightness == led_dat->conf.level)
		return 0;

	led_dat->conf.level = brightness;
	max = (1 << led_dat->conf.trans_bits) - 1;
	duty = led_dat->info.config.pwm_period_ns;
	duty *= brightness;
	do_div(duty, max);

	if (led_dat->info.config.active_low)
		duty = led_dat->info.config.pwm_period_ns - duty;

	led_dat->info.duty = duty;

	__led_pwm_set(&led_dat->info);

	return 0;
}

int mt_leds_brightness_set(char *name, int level)
{
	struct mtk_led_data *led_dat;
	int index;

	index = getLedDespIndex(name);
	if (index < 0) {
		pr_notice("can not find leds by led_desp %s", name);
		return -1;
	}
	led_dat = container_of(leds_info->leds[index],
		struct mtk_led_data, desp);
	led_level_pwm_set(led_dat, level);
	led_dat->conf.level = level;

	return 0;
}
EXPORT_SYMBOL(mt_leds_brightness_set);

#ifndef CONFIG_MTK_AAL_SUPPORT
static int led_pwm_disable(struct led_pwm_info *led_info)
{

	mutex_lock(&leds_mutex);

	pwm_config(led_info->pwm, 0, led_info->config.pwm_period_ns);
	pwm_disable(led_info->pwm);

	mutex_unlock(&leds_mutex);

	return 0;
}
#endif


static int led_level_set(struct led_classdev *led_cdev,
					  enum led_brightness brightness)
{
	int trans_level = 0;

	struct led_conf_info *led_conf =
		container_of(led_cdev, struct led_conf_info, cdev);
	struct mtk_led_data *led_dat =
		container_of(led_conf, struct mtk_led_data, conf);

	if (led_dat->brightness == brightness)
		return 0;

	led_dat->brightness = brightness;

	trans_level = (
		(((1 << led_dat->conf.trans_bits) - 1) * brightness
		+ (((1 << led_dat->conf.led_bits) - 1) / 2))
		/ ((1 << led_dat->conf.led_bits) - 1));

	led_debug_log(led_dat, brightness, trans_level);

#ifdef CONFIG_LEDS_BRIGHTNESS_CHANGED
	call_notifier(1, led_dat);
#endif
#ifndef CONFIG_MTK_AAL_SUPPORT
	led_level_pwm_set(led_dat, trans_level);
	led_dat->last_level = trans_level;
#endif
	return 0;

}


/****************************************************************************
 * add API for temperature control
 ***************************************************************************/

int mt_leds_max_brightness_set(char *name, int percent, bool enable)
{
	struct mtk_led_data *led_dat;
	int max_l = 0, index = -1, limit_l = 0, cur_l = 0;

	index = getLedDespIndex(name);
	if (index < 0) {
		pr_notice("can not find leds by led_desp %s", name);
		return -1;
	}
	led_dat = container_of(leds_info->leds[index],
		struct mtk_led_data, desp);

	max_l = (1 << led_dat->conf.trans_bits) - 1;
	limit_l = (percent * max_l) / 100;
	pr_info("before: name: %s, percent : %d, limit_l : %d, enable: %d",
		leds_info->leds[index]->name, percent, limit_l, enable);
	if (enable) {
		led_dat->conf.max_level = limit_l;
		cur_l = min(led_dat->brightness, limit_l);
	} else if (!enable) {
		led_dat->conf.max_level = max_l;
		cur_l = led_dat->brightness;
	}
#ifdef CONFIG_LEDS_BRIGHTNESS_CHANGED
	call_notifier(3, led_dat);
#endif

	if (led_dat->conf.cdev.brightness != 0)
		led_level_pwm_set(led_dat, cur_l);

	pr_info("after: name: %s, cur_l : %d, max_level : %d",
		led_dat->conf.cdev.name, cur_l, led_dat->conf.max_level);
	return 0;

}
EXPORT_SYMBOL(mt_leds_max_brightness_set);

static int led_data_init(struct device *dev, struct mtk_led_data *s_led)
{
	int ret;

	s_led->conf.cdev.default_trigger = s_led->info.config.default_trigger;
	s_led->conf.cdev.max_brightness = s_led->info.config.max_brightness;
	s_led->conf.cdev.flags = LED_CORE_SUSPENDRESUME;
	s_led->conf.cdev.brightness_set_blocking = led_level_set;
	s_led->brightness = 0;
	s_led->conf.level = 0;
	s_led->last_level = 0;
	ret = devm_led_classdev_register(dev, &(s_led->conf.cdev));
	if (ret < 0) {
		pr_notice("led class register fail!");
		return ret;
	}
	pr_info("%s devm_led_classdev_register ok! ", s_led->conf.cdev.name);

	ret = snprintf(s_led->debug.buffer + strlen(s_led->debug.buffer),
		4095 - strlen(s_led->debug.buffer),
		"[Light] Set %s directly ", s_led->conf.cdev.name);

	if (ret < 0 || ret >= 4096)
		pr_info("print log init error!");

	return 0;
}

static int led_pwm_config_add(struct device *dev,
		struct mtk_led_data *s_led, struct device_node *leds_np)
{
	struct pwm_args pargs;
	int ret = 0;

	if (leds_np != NULL)
		s_led->info.pwm = devm_of_pwm_get(dev, leds_np,
			s_led->info.config.name);
	else
		s_led->info.pwm = devm_pwm_get(dev, s_led->info.config.name);
	if (IS_ERR(s_led->info.pwm)) {
		ret = PTR_ERR(s_led->info.pwm);
		if (ret != -EPROBE_DEFER) {
			pr_notice("unable to request PWM for %s, err_code: %d\n",
				s_led->info.config.name, ret);
			goto err;
		}
	}

	pwm_apply_args(s_led->info.pwm);
	pwm_get_args(s_led->info.pwm, &pargs);

	s_led->info.config.pwm_period_ns = pargs.period;
	if (!s_led->info.config.pwm_period_ns && (pargs.period > 0))
		s_led->info.config.pwm_period_ns = pargs.period;

	pr_info("set led pwm OK! info.config.pwm_period_ns = %d!",
		s_led->info.config.pwm_period_ns);
	return ret;

 err:
	pr_notice("add pwm failed!\n");
	ret = -ENOMEM;
	return ret;

}

static int mtk_leds_parse_dt(struct device *dev,
		struct mtk_leds_info *m_leds)
{
	struct device_node *child;
	struct mtk_led_data *s_led;
	int ret = 0, num = 0, level = 0;
	const char *state;

	if (!dev->of_node) {
		pr_notice("Error load dts: node not exist!\n");
		return ret;
	}

	for_each_available_child_of_node(dev->of_node, child) {

		s_led = &(m_leds->leds[num]);
		ret = of_property_read_string(child, "label",
			&(s_led->conf.cdev.name));
		if (ret) {
			pr_info("Fail to read label property");
			goto out_led_dt;
		}
		ret = of_property_read_string(child, "pwm-names",
			&(s_led->info.config.name));
		if (ret) {
			pr_info("Fail to read pwm-names property");
			goto out_led_dt;
		}
		ret = of_property_read_string(child, "default-trigger",
			&(s_led->info.config.default_trigger));
		if (ret) {
			pr_info("Fail to read default-trigger property");
			s_led->info.config.default_trigger = NULL;
		}
		ret = of_property_read_u8(child, "active-low",
			&(s_led->info.config.active_low));
		if (ret)
			pr_info("Fail to read active-low property\n");
		ret = of_property_read_u32(child,
			"led-bits", &(s_led->conf.led_bits));
		if (ret) {
			pr_info("No led-bits, use default value 8");
			s_led->conf.led_bits = 8;
		}
		ret = of_property_read_u32(child,
			"max-brightness", &(s_led->info.config.max_brightness));
		if (ret) {
			pr_info("No max-brightness, use default value 255");
			s_led->info.config.max_brightness =
				(1 << s_led->conf.led_bits) - 1;
		}
		ret = of_property_read_u32(child,
			"trans-bits", &(s_led->conf.trans_bits));
		if (ret) {
			pr_info("No trans-bits, use default value 8");
			s_led->conf.trans_bits = 8;
		}
		s_led->conf.max_level = (1 << s_led->conf.trans_bits) - 1;
		ret = of_property_read_string(child, "default-state", &state);
		if (!ret) {
			if (!strcmp(state, "half"))
				level = s_led->info.config.max_brightness / 2;
			else if (!strcmp(state, "on"))
				level = s_led->info.config.max_brightness;
			else
				level = s_led->conf.level = 0;
		} else {
			level = s_led->info.config.max_brightness;
		}

		ret = of_property_read_string(child, "proj", &proj);
		if (ret) {
			pr_notice("Fail to read proj property");
		}else{
			pr_notice("proj property %s", proj);
		}

		pr_info("parse %s(%d) leds dt: %s, %s, %d, %d, %d\n",
			s_led->conf.cdev.name, num, s_led->info.config.name,
			s_led->info.config.default_trigger,
			s_led->info.config.active_low,
			s_led->info.config.max_brightness,
			s_led->conf.led_bits);
		s_led->desp.index = num;
		strncpy(s_led->desp.name, s_led->conf.cdev.name,
			strlen(s_led->conf.cdev.name));
		s_led->desp.index = num;
		leds_info->leds[num] = &s_led->desp;
		s_led->conf.cdev.brightness = level;
		ret = led_data_init(dev, s_led);
		if (ret)
			goto out_led_dt;
		led_pwm_config_add(dev, s_led, child);
		led_level_set(&s_led->conf.cdev, level);
		num++;
	}
	m_leds->nums = num;
	pr_info("load dts ok!");
	return ret;
out_led_dt:
	pr_notice("Error load dts node!\n");
	of_node_put(child);
	return ret;
}


/****************************************************************************
 * driver functions
 ***************************************************************************/

static int mtk_leds_probe(struct platform_device *pdev)
{

	struct device *dev = &pdev->dev;
	struct mtk_leds_info *m_leds;
	int ret, nums;

	pr_info("probe begain +++");

	nums = of_get_child_count(dev->of_node);
	pr_info("Load dts node nums: %d", nums);
	m_leds = devm_kzalloc(dev, (sizeof(struct mtk_leds_info) +
		(sizeof(struct mtk_led_data) * (nums))), GFP_KERNEL);
	if (!m_leds)
		goto err;

	leds_info = devm_kzalloc(dev, (sizeof(struct leds_desp_info) +
		sizeof(struct led_desp *) * (nums)),
		GFP_KERNEL);
	leds_info->lens = nums;
	if (!leds_info) {
		ret = -ENOMEM;
		goto err;
	}

	platform_set_drvdata(pdev, m_leds);
	m_leds->dev = dev;
	ret = mtk_leds_parse_dt(&(pdev->dev), m_leds);
	if (ret) {
		pr_notice("Failed to parse devicetree!\n");
		goto err;
	}

	pr_info("probe end ---");
	return 0;
 err:
	pr_notice("Failed to probe!\n");
	ret = -ENOMEM;
	return ret;
}

static int mtk_leds_remove(struct platform_device *pdev)
{
	int i;
	struct mtk_leds_info *m_leds = dev_get_platdata(&pdev->dev);

	if (!m_leds)
		return 0;
	for (i = 0; i < m_leds->nums; i++) {
		if (!m_leds->leds[i].parent)
			continue;
		led_classdev_unregister(&m_leds->leds[i].conf.cdev);
		m_leds->leds[i].parent = NULL;
	}
	kfree(m_leds);
	m_leds = NULL;

	return 0;
}

static void mtk_leds_shutdown(struct platform_device *pdev)
{
	int i;
	struct mtk_leds_info *m_leds = dev_get_platdata(&pdev->dev);

	pr_info("Turn off backlight\n");

	for (i = 0; m_leds && i < m_leds->nums; i++) {
		if (!&(m_leds->leds[i]))
			continue;
#ifdef CONFIG_LEDS_BRIGHTNESS_CHANGED
		call_notifier(2, &(m_leds->leds[i]));
#ifdef CONFIG_MTK_AAL_SUPPORT
		continue;
#else
		led_pwm_disable(&(m_leds->leds[i].info));
#endif
#else
		led_pwm_disable(&(m_leds->leds[i].info));
#endif
	}

}

static const struct of_device_id of_mtk_pwm_leds_match[] = {
	{ .compatible = "mediatek,disp-pwm-leds", },
	{},
};
MODULE_DEVICE_TABLE(of, of_mtk_pwm_leds_match);

static struct platform_driver mtk_pwm_leds_driver = {
	.driver = {
		   .name = "mtk-pwm-leds",
		   .owner = THIS_MODULE,
		   .of_match_table = of_mtk_pwm_leds_match,
		   },
	.probe = mtk_leds_probe,
	.remove = mtk_leds_remove,
	.shutdown = mtk_leds_shutdown,

};

static int __init mtk_leds_init(void)
{
	int ret;

	pr_info("Leds init\n");
	ret = platform_driver_register(&mtk_pwm_leds_driver);

	if (ret) {
		pr_info("driver register error: %d\n", ret);
		return ret;
	}

	return ret;
}

static void __exit mtk_leds_exit(void)
{
	platform_driver_unregister(&mtk_pwm_leds_driver);
}

/* delay leds init, for (1)display has delayed to use clock upstream.
 * (2)to fix repeat switch battary and power supply caused BL KE issue,
 * battary calling bl .shutdown whitch need to call disp_pwm and display
 * function and they not yet probe.
 */
late_initcall(mtk_leds_init);
module_exit(mtk_leds_exit);

MODULE_AUTHOR("Mediatek Corporation");
MODULE_DESCRIPTION("MTK Disp PWM Backlight Driver");
MODULE_LICENSE("GPL");


