/*============================================================================*
 *         Copyright © 2019-2020 Signetik, LLC -- All Rights Reserved         *
 *----------------------------------------------------------------------------*
 *                                                              Signetik, LLC *
 *                                                           www.signetik.com *
 *                                      SPDX-License-Identifier: BSD-4-Clause *
 *============================================================================*/

#define DEVICE_ID 0xC0000003
extern uint8_t * COAP_buff_ptr;
extern uint32_t COAP_buff_len;
extern s64_t base_time;

_Bool cellular_ready(void);
void LTE_thread(void);
void LTE_send(int rn, int rc);
int LTE_wait_complete(void);
