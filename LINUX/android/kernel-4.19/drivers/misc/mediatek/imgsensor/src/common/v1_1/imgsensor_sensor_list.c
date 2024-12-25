// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include "kd_imgsensor.h"
#include "imgsensor_sensor_list.h"

/* Add Sensor Init function here
 * Note:
 * 1. Add by the resolution from ""large to small"", due to large sensor
 *    will be possible to be main sensor.
 *    This can avoid I2C error during searching sensor.
 * 2. This file should be the same as
 *    mediatek\custom\common\hal\imgsensor\src\sensorlist.cpp
 */
struct IMGSENSOR_SENSOR_LIST
	gimgsensor_sensor_list[MAX_NUM_OF_SUPPORT_SENSOR] = {

/*SX3*/
#if defined(S5K3L6_MIPI_RAW)
	{S5K3L6_SENSOR_ID,
	SENSOR_DRVNAME_S5K3L6_MIPI_RAW,
	S5K3L6_MIPI_RAW_SensorInit},
#endif

#if defined(HI556_MIPI_RAW)
	{HI556_SENSOR_ID,
	SENSOR_DRVNAME_HI556_MIPI_RAW,
	HI556_MIPI_RAW_SensorInit},
#endif

/*SX4*/
#if defined(S5KJNSSQ33_MIPI_RAW)
	{S5KJNSSQ33_SENSOR_ID,
	SENSOR_DRVNAME_S5KJNSSQ33_MIPI_RAW,
	S5KJNSSQ33_MIPI_RAW_SensorInit},
#endif

#if defined(S5K4H7_MIPI_RAW)
	{S5K4H7_SENSOR_ID,
	SENSOR_DRVNAME_S5K4H7_MIPI_RAW,
	S5K4H7_MIPI_RAW_SensorInit},
#endif

#if defined(GC02M1B_MIPI_MONO)
	{GC02M1B_SENSOR_ID,
	SENSOR_DRVNAME_GC02M1B_MIPI_MONO,
	GC02M1B_MIPI_MONO_SensorInit},
#endif

	/*  ADD sensor driver before this line */
	{0, {0}, NULL}, /* end of list */
};

/* e_add new sensor driver here */

