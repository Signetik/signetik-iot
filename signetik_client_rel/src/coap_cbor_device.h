/*============================================================================*
 *         Copyright © 2019-2020 Signetik, LLC -- All Rights Reserved         *
 *----------------------------------------------------------------------------*
 *                                                              Signetik, LLC *
 *                                                           www.signetik.com *
 *                                      SPDX-License-Identifier: BSD-4-Clause *
 *============================================================================*/

#ifndef _COAP_CBOR_DEVICE_H_
#define _COAP_CBOR_DEVICE_H_

#include <stdint.h>
#include <stdbool.h>
//#include <stddef.h>
#include "cell_packet.h"
#include "sigconfig.h"

#ifdef __cplusplus
extern "C" {
#endif
void create_coap_cbor_with_sigcfg(sigconfig_master_t * sigcfg, uint8_t group, uint32_t device_id, int record_number, int num_reports, uint8_t **coap_buf, uint32_t *coap_len);
void create_coap_cbor_with_packet_def(cell_packet_def_t * packet_def, uint32_t device_id, int record_number, int num_reports, uint8_t **coap_buf, uint32_t *coap_len);
void create_config_request(uint32_t device_id, uint8_t need_settings, uint8_t **coap_buf, uint32_t *coap_len);
int extract_binary_from_coap_cbor(uint8_t *coap, uint16_t coap_len, uint8_t *payload, uint16_t payload_len);

#ifdef __cplusplus
}
#endif

#endif
