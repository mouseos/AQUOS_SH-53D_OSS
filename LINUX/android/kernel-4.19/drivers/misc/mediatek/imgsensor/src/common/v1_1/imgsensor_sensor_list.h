/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __KD_SENSORLIST_H__
#define __KD_SENSORLIST_H__

#include "kd_camera_typedef.h"
#include "imgsensor_sensor.h"

struct IMGSENSOR_INIT_FUNC_LIST {
	MUINT32   id;
	MUINT8    name[32];
	MUINT32 (*init)(struct SENSOR_FUNCTION_STRUCT **pfFunc);
};

/*SX3*/
#if defined(S5K3L6_MIPI_RAW)
UINT32 S5K3L6_MIPI_RAW_SensorInit(struct SENSOR_FUNCTION_STRUCT **pfFunc);
#endif

#if defined(HI556_MIPI_RAW)
UINT32 HI556_MIPI_RAW_SensorInit(struct SENSOR_FUNCTION_STRUCT **pfFunc);
#endif

/* SX4 */
#if defined(S5KJNSSQ33_MIPI_RAW)
UINT32 S5KJNSSQ33_MIPI_RAW_SensorInit(struct SENSOR_FUNCTION_STRUCT **pfFunc);
#endif

#if defined(S5K4H7_MIPI_RAW)
UINT32 S5K4H7_MIPI_RAW_SensorInit(struct SENSOR_FUNCTION_STRUCT **pfFunc);
#endif

#if defined(GC02M1B_MIPI_MONO)
UINT32 GC02M1B_MIPI_MONO_SensorInit(struct SENSOR_FUNCTION_STRUCT **pfFunc);
#endif

extern struct IMGSENSOR_SENSOR_LIST gimgsensor_sensor_list[];

#endif

