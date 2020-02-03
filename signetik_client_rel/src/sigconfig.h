/*============================================================================*
 *         Copyright © 2019-2020 Signetik, LLC -- All Rights Reserved         *
 *----------------------------------------------------------------------------*
 *                                                              Signetik, LLC *
 *                                                           www.signetik.com *
 *                                      SPDX-License-Identifier: BSD-4-Clause *
 *============================================================================*/

#include "coap_cbor_device.h"

#ifndef SIGCONFIG_H_
#define SIGCONFIG_H_


enum sigcfg_protocol
{
	PROTO_ERROR = 0,
	MODBUS,
	I2C,
	SPI,	
};

typedef struct modbus_transaction_s // objects in get array (should just be 1 for now)
{
	uint8_t slave_addr;
	uint8_t opcode;
	uint16_t reg_addr;
	uint16_t num;
	uint8_t * data_received;
} modbus_action_t;

typedef struct field_s //field
{
	enum sigcfg_protocol protocol;
	enum cbor_data_type type;
	uint8_t field_name [16];
	uint8_t num_actions;
	uint8_t * data;
	modbus_action_t action [5];	
}field_group_t;

typedef struct packet_s //packet
{
	uint8_t report_name [16];
	uint32_t sample_interval_s;
	uint32_t next_sample_time;
	uint32_t report_interval_s;
	uint8_t num_fields;
	field_group_t field [5];	
} packet_group_t;

typedef struct sigconfig_master_s
{
	uint8_t num_groups;
	packet_group_t packet [5];	
}sigconfig_master_t;

//typedef struct uart_settings_s
//{
//	uint32_t baud;
//	enum usart_parity parity;
//	enum usart_stopbits stop_bits;	
//}uart_settings_t;

extern volatile bool need_settings;
extern uint8_t sigcfg_packet_space [64];
extern sigconfig_master_t sigcfg_master;


void sigconfig_task(void * p);




#endif /* SIGCONFIG_H_ */
