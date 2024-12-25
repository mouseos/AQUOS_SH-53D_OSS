// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2016 MediaTek Inc.
 */

#include "kd_imgsensor.h"


#include "imgsensor_hw.h"
#include "imgsensor_cfg_table.h"

/* Legacy design */
struct IMGSENSOR_HW_POWER_SEQ sensor_power_sequence[] = {
/*SX3*/
#if defined(S5K3L6_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5K3L6_MIPI_RAW,
		{
			{RST, Vol_Low, 1},
			{DOVDD, Vol_1800, 1},

			{AVDD, Vol_Low, 1},
			{AVDD, Vol_High, 1},

			{DVDD, Vol_Low, 1},
			{DVDD, Vol_High, 1},

			{AFVDD, Vol_Low, 5},
			{AFVDD, Vol_High, 5},

			{SensorMCLK, Vol_High, 1},
			{RST, Vol_High, 10}
		},
	},
#endif

#if defined(HI556_MIPI_RAW)
	{
		SENSOR_DRVNAME_HI556_MIPI_RAW,
		{
			{RST, Vol_Low, 1},
			{DOVDD, Vol_Low, 1},
			{DOVDD, Vol_High, 1},

			{AVDD, Vol_Low, 1},
			{AVDD, Vol_High, 1},

			{DVDD, Vol_Low, 2},
			{DVDD, Vol_High, 2},
			{SensorMCLK, Vol_High, 2},
			{RST, Vol_High, 1},
		},
	},
#endif

/* SX4 */
#if defined(S5KJNSSQ33_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5KJNSSQ33_MIPI_RAW,
		{
			{RST, Vol_Low, 1},
			{DOVDD, Vol_1800, 1},
			{DVDD, Vol_Low, 1},
			{DVDD, Vol_High, 1},
			{AVDD, Vol_Low, 5},
			{AVDD, Vol_High, 5},
			{AFVDD, Vol_Low, 5},
			{AFVDD, Vol_High, 5},
			{SensorMCLK, Vol_High, 1},
			{RST, Vol_High, 10}
		},
	},
#endif


#if defined(S5K4H7_MIPI_RAW)
	{

		SENSOR_DRVNAME_S5K4H7_MIPI_RAW,
		{
			{RST, Vol_Low, 1},

			{DOVDD, Vol_Low, 1},
			{DOVDD, Vol_High, 1},

			{AVDD, Vol_Low, 1},
			{AVDD, Vol_High, 1},

			{DVDD, Vol_Low, 1},
			{DVDD, Vol_High, 10},

			{SensorMCLK, Vol_High, 1},

			{RST, Vol_High, 10},

		},
	},

#endif
#if defined(GC02M1B_MIPI_MONO)
		{
			SENSOR_DRVNAME_GC02M1B_MIPI_MONO,
			{
				{RST, Vol_Low, 1},
				
				{DOVDD, Vol_Low, 1},
				{DOVDD, Vol_High, 1},
				
				{DVDD, Vol_Low, 1},
				{DVDD, Vol_High, 1},
				
				{AVDD, Vol_Low, 1},
				{AVDD, Vol_High, 1},
				{SensorMCLK, Vol_High, 1},
				{RST, Vol_High, 10}
			},
		},
#endif

	/* add new sensor before this line */
	{NULL,},
};

