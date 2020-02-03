/*============================================================================*
 *         Copyright © 2019-2020 Signetik, LLC -- All Rights Reserved         *
 *----------------------------------------------------------------------------*
 *                                                              Signetik, LLC *
 *                                                           www.signetik.com *
 *                                      SPDX-License-Identifier: BSD-4-Clause *
 *============================================================================*/

#define LTE_STACKSIZE 4096
#define LTE_PRIORITY 2

#define MAIN_STACKSIZE 1024
#define MAIN_PRIORITY 5

#ifdef CONFIG_LIS3DH
#define ACCEL_STACKSIZE 1024
#define ACCEL_PRIORITY 7
#endif //CONFIG_LIS3DH

#ifdef CONFIG_HTU21D
#define TEMP_HUMIDITY_STACKSIZE 1024
#define TEMP_HUMIDITY_PRIORITY 7
#endif //CONFIG_HTU21D

#define THREAD_START_DELAY 500 //ms


