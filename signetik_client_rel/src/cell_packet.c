/*============================================================================*
 *         Copyright © 2019-2020 Signetik, LLC -- All Rights Reserved         *
 *----------------------------------------------------------------------------*
 *                                                              Signetik, LLC *
 *                                                           www.signetik.com *
 *                                      SPDX-License-Identifier: BSD-4-Clause *
 *============================================================================*/

#include "cell_packet.h"
#include <zephyr.h>

K_SEM_DEFINE(sem_data, 1, 1);

#if(0)
cell_packet_def_t sigcfg_packet_def [5];

const cell_packet_def_t sig_gps_packet_def = 
{
	.packet_name	= "gps_tracker",
	.product_type	= "HSS",
	.num_fields		= 6,
	.field[0].name	= "lat",
	.field[0].ftype	= CBOR_FLOAT_REVERSE,
	.field[0].data	= (void *) &gps_packet.lat,
	.field[1].name	= "lon",
	.field[1].ftype	= CBOR_FLOAT_REVERSE,
	.field[1].data	= (void *) &gps_packet.lon,
	.field[2].name	= "alt_m",
	.field[2].ftype	= CBOR_FLOAT_REVERSE,
	.field[2].data	= (void *) &gps_packet.alt_m,
	.field[3].name	= "HDOP",
	.field[3].ftype	= CBOR_FLOAT_REVERSE,
	.field[3].data	= (void *) &gps_packet.HDOP,
	.field[4].name	= "num_sat",
	.field[4].ftype	= CBOR_INT8,
	.field[4].data	= (void *) &gps_packet.num_sat,
	.field[5].name	= "distance_m",
	.field[5].ftype	= CBOR_INT32,
	.field[5].data	= (void *) &gps_packet.distance_m,
};
#endif

#define MAX_RECORDS 24
struct tha_packet_storage_s 
{
	uint32_t ts_s[MAX_RECORDS];
	uint32_t ts_ms[MAX_RECORDS];
	int32_t x[MAX_RECORDS];
	int32_t y[MAX_RECORDS];
	int32_t z[MAX_RECORDS];
	int32_t temperature[MAX_RECORDS];
	int32_t humidity[MAX_RECORDS];
};

struct tha_packet_storage_s tha_packet = { 0 };

int sig_tha_set_data( struct tha_packet_s *data, int index )
{
	if (index >= MAX_RECORDS)
		return -1;
    if (k_sem_take(&sem_data, 10000) != 0)
    {
        printk("Packet data in use, not setting.\n");
        return -1;
    }
	tha_packet.ts_s[index] = data->ts_s;
	tha_packet.ts_ms[index] = data->ts_ms;
    tha_packet.x[index] = data->x;
    tha_packet.y[index] = data->y;
    tha_packet.z[index] = data->z;
	tha_packet.temperature[index] = data->temperature;
	tha_packet.humidity[index] = data->humidity;
    k_sem_give(&sem_data);

	return 0;
}


const cell_packet_def_t sig_tha_packet_def =
{
    .packet_name	= "THA_Data",
    .product_type	= "SIGTHA",
	.ts_s           = tha_packet.ts_s,
	.ts_ms          = tha_packet.ts_ms,
    .num_fields     = 5,
    .field[0].name	= "x_rms",
    .field[0].ftype	= CBOR_INT32,
    .field[0].data	= (void *) tha_packet.x,
    .field[1].name	= "y_rms",
    .field[1].ftype	= CBOR_INT32,
    .field[1].data	= (void *) tha_packet.y,
    .field[2].name	= "z_rms",
    .field[2].ftype	= CBOR_INT32,
    .field[2].data	= (void *) tha_packet.z,
    .field[3].name	= "temp_c",
    .field[3].ftype	= CBOR_INT32,
    .field[3].data	= (void *) tha_packet.temperature,	
    .field[4].name  = "humidity",
    .field[4].ftype	= CBOR_INT32,
    .field[4].data	= (void *) tha_packet.humidity,
};
