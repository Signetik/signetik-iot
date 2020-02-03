/*============================================================================*
 *         Copyright © 2019-2020 Signetik, LLC -- All Rights Reserved         *
 *----------------------------------------------------------------------------*
 *                                                              Signetik, LLC *
 *                                                           www.signetik.com *
 *                                      SPDX-License-Identifier: BSD-4-Clause *
 *============================================================================*/

#include <stdint.h>

#ifndef CELL_PACKET_H_
#define CELL_PACKET_H_

enum cbor_data_type
{
	CBOR_ERROR = 0,
	CBOR_INT8,
	CBOR_INT16,
	CBOR_INT32,
	CBOR_RAW,
	CBOR_TEXT,
	CBOR_FLOAT,
	CBOR_FLOAT_REVERSE,
	CBOR_DOUBLE,
	CBOR_BOOL,
};

typedef struct packet_field_s
{
	uint8_t name[32];
	enum cbor_data_type ftype;
	void * data;
	
}cell_packet_field_t;

typedef const struct cell_packet_def_s
{
	uint32_t *ts_s;
	uint32_t *ts_ms;
	uint8_t packet_name[32];
	uint8_t product_type[32];
	uint8_t num_fields;
	cell_packet_field_t field [10];
		
}cell_packet_def_t;

struct tha_packet_s 
{
	uint32_t ts_s;
	uint32_t ts_ms;
	int32_t x;
	int32_t y;
	int32_t z;
	int32_t temperature;
	int32_t humidity;
};

int sig_tha_set_data( struct tha_packet_s *data, int index );

extern const cell_packet_def_t sig_gps_packet_def;
extern const cell_packet_def_t sig_tha_packet_def;
extern cell_packet_def_t sigcfg_packet_def [5];

#endif /* CELL_PACKET_H_ */
