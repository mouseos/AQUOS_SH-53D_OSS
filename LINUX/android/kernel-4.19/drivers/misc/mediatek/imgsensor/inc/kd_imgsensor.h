/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef _KD_IMGSENSOR_H
#define _KD_IMGSENSOR_H

#include <linux/ioctl.h>

#ifndef ASSERT
#define ASSERT(expr)        WARN_ON(!(expr))
#endif

#define IMGSENSORMAGIC 'i'
/* IOCTRL(inode * ,file * ,cmd ,arg ) */
/* S means "set through a ptr" */
/* T means "tell by a arg value" */
/* G means "get by a ptr" */
/* Q means "get by return a value" */
/* X means "switch G and S atomically" */
/* H means "switch T and Q atomically" */

/******************************************************************************
 *
 ******************************************************************************/

/* sensorOpen */
#define KDIMGSENSORIOC_T_OPEN \
	_IO(IMGSENSORMAGIC, 0)
/* sensorGetInfo */
#define KDIMGSENSORIOC_X_GET_CONFIG_INFO \
	_IOWR(IMGSENSORMAGIC, 5, struct IMGSENSOR_GET_CONFIG_INFO_STRUCT)

#define KDIMGSENSORIOC_X_GETINFO \
	_IOWR(IMGSENSORMAGIC, 5, struct ACDK_SENSOR_GETINFO_STRUCT)
/* sensorGetResolution */
#define KDIMGSENSORIOC_X_GETRESOLUTION \
	_IOWR(IMGSENSORMAGIC, 10, struct ACDK_SENSOR_RESOLUTION_INFO_STRUCT)
/* For kernel 64-bit */
#define KDIMGSENSORIOC_X_GETRESOLUTION2 \
	_IOWR(IMGSENSORMAGIC, 10, struct ACDK_SENSOR_PRESOLUTION_STRUCT)
/* sensorFeatureControl */
#define KDIMGSENSORIOC_X_FEATURECONCTROL \
	_IOWR(IMGSENSORMAGIC, 15, struct ACDK_SENSOR_FEATURECONTROL_STRUCT)
/* sensorControl */
#define KDIMGSENSORIOC_X_CONTROL \
	_IOWR(IMGSENSORMAGIC, 20, struct ACDK_SENSOR_CONTROL_STRUCT)
/* sensorClose */
#define KDIMGSENSORIOC_T_CLOSE \
	_IO(IMGSENSORMAGIC, 25)
/* sensorSearch */
#define KDIMGSENSORIOC_T_CHECK_IS_ALIVE \
	_IO(IMGSENSORMAGIC, 30)
/* set sensor driver */
#define KDIMGSENSORIOC_X_SET_DRIVER \
	_IOWR(IMGSENSORMAGIC, 35, struct SENSOR_DRIVER_INDEX_STRUCT)
/* get socket postion */
#define KDIMGSENSORIOC_X_GET_SOCKET_POS \
	_IOWR(IMGSENSORMAGIC, 40, u32)
/* set I2C bus */
#define KDIMGSENSORIOC_X_SET_I2CBUS \
	_IOWR(IMGSENSORMAGIC, 45, u32)
/* set I2C bus */
#define KDIMGSENSORIOC_X_RELEASE_I2C_TRIGGER_LOCK \
	_IO(IMGSENSORMAGIC, 50)
/* Set Shutter Gain Wait Done */
#define KDIMGSENSORIOC_X_SET_SHUTTER_GAIN_WAIT_DONE \
	_IOWR(IMGSENSORMAGIC, 55, u32)
/* set mclk */
#define KDIMGSENSORIOC_X_SET_MCLK_PLL \
	_IOWR(IMGSENSORMAGIC, 60, struct ACDK_SENSOR_MCLK_STRUCT)
#define KDIMGSENSORIOC_X_GETINFO2 \
	_IOWR(IMGSENSORMAGIC, 65, struct IMAGESENSOR_GETINFO_STRUCT)
/* set open/close sensor index */
#define KDIMGSENSORIOC_X_SET_CURRENT_SENSOR \
	_IOWR(IMGSENSORMAGIC, 70, u32)
/* set GPIO */
#define KDIMGSENSORIOC_X_SET_GPIO \
	_IOWR(IMGSENSORMAGIC, 75, struct IMGSENSOR_GPIO_STRUCT)
/* Get ISP CLK */
#define KDIMGSENSORIOC_X_GET_ISP_CLK \
	_IOWR(IMGSENSORMAGIC, 80, u32)
/* Get CSI CLK */
#define KDIMGSENSORIOC_X_GET_CSI_CLK \
	_IOWR(IMGSENSORMAGIC, 85, u32)

/* Get ISP CLK via MMDVFS*/
#define KDIMGSENSORIOC_DFS_UPDATE \
	_IOWR(IMGSENSORMAGIC, 90, unsigned int)
#define KDIMGSENSORIOC_GET_SUPPORTED_ISP_CLOCKS \
	_IOWR(IMGSENSORMAGIC, 95, struct IMAGESENSOR_GET_SUPPORTED_ISP_CLK)
#define KDIMGSENSORIOC_GET_CUR_ISP_CLOCK \
	_IOWR(IMGSENSORMAGIC, 100, unsigned int)

#ifdef CONFIG_COMPAT
#define COMPAT_KDIMGSENSORIOC_X_GET_CONFIG_INFO \
	_IOWR(IMGSENSORMAGIC, 5, struct COMPAT_IMGSENSOR_GET_CONFIG_INFO_STRUCT)

#define COMPAT_KDIMGSENSORIOC_X_GETINFO \
	_IOWR(IMGSENSORMAGIC, 5, struct COMPAT_ACDK_SENSOR_GETINFO_STRUCT)
#define COMPAT_KDIMGSENSORIOC_X_FEATURECONCTROL \
	_IOWR(IMGSENSORMAGIC, 15, \
		struct COMPAT_ACDK_SENSOR_FEATURECONTROL_STRUCT)
#define COMPAT_KDIMGSENSORIOC_X_CONTROL \
	_IOWR(IMGSENSORMAGIC, 20, struct COMPAT_ACDK_SENSOR_CONTROL_STRUCT)
#define COMPAT_KDIMGSENSORIOC_X_GETINFO2 \
	_IOWR(IMGSENSORMAGIC, 65, struct COMPAT_IMAGESENSOR_GETINFO_STRUCT)
#define COMPAT_KDIMGSENSORIOC_X_GETRESOLUTION2 \
	_IOWR(IMGSENSORMAGIC, 10, struct COMPAT_ACDK_SENSOR_PRESOLUTION_STRUCT)
#endif

/************************************************************************
 *
 ************************************************************************/
/* SENSOR CHIP VERSION */
/* SX3 */
#if defined(S5K3L6_MIPI_RAW)
#define S5K3L6_SENSOR_ID                        0x30C6
#endif

#if defined(HI556_MIPI_RAW)
#define HI556_SENSOR_ID                         0x0556
#endif

/* SX4 */
#if defined(S5KJNSSQ33_MIPI_RAW)
#define S5KJNSSQ33_SENSOR_ID                    0x38EE
#endif

#if defined(S5K4H7_MIPI_RAW)
#define S5K4H7_SENSOR_ID						0x487B
#endif

#if defined(GC02M1B_MIPI_MONO)
#define GC02M1B_SENSOR_ID                       0x02e0
#endif

/* CAMERA DRIVER NAME */
#define CAMERA_HW_DEVNAME                       "kd_camera_hw"

/* SENSOR DEVICE DRIVER NAME */
/* SX3 */
#if defined(S5K3L6_MIPI_RAW)
#define SENSOR_DRVNAME_S5K3L6_MIPI_RAW          "s5k3l6_mipi_raw"
#endif

#if defined(HI556_MIPI_RAW)
#define SENSOR_DRVNAME_HI556_MIPI_RAW           "hi556_mipi_raw"
#endif

/* SX4 */
#if defined(S5KJNSSQ33_MIPI_RAW)
#define SENSOR_DRVNAME_S5KJNSSQ33_MIPI_RAW      "s5kjnssq33_mipi_raw"
#endif

#if defined(S5K4H7_MIPI_RAW)
#define SENSOR_DRVNAME_S5K4H7_MIPI_RAW           "s5k4h7_mipi_raw"
#endif

#if defined(GC02M1B_MIPI_MONO)
#define SENSOR_DRVNAME_GC02M1B_MIPI_MONO        "gc02m1b_mipi_mono"
#endif
/******************************************************************************
 *
 ******************************************************************************/
void KD_IMGSENSOR_PROFILE_INIT(void);
void KD_IMGSENSOR_PROFILE(char *tag);
void KD_IMGSENSOR_PROFILE_INIT_I2C(void);
void KD_IMGSENSOR_PROFILE_I2C(char *tag, int trans_num);

#define mDELAY(ms)     mdelay(ms)
#define uDELAY(us)       udelay(us)
#endif              /* _KD_IMGSENSOR_H */
