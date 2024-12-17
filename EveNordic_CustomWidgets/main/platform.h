/*
 * Copyright (c) Riverdi Sp. z o.o. sp. k. <contact@riverdi.com>
 * Copyright (c) Skalski Embedded Technologies <contact@lukasz-skalski.com>
 */

#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>

/******************************************************************************/

/* type definitions for 'riverdi-eve-lib' library */
#define TRUE		(1)
#define FALSE		(0)

typedef char           bool_t;
typedef char           char8_t;
typedef unsigned char  uchar8_t;
typedef signed char    schar8_t;
typedef float          float_t;

/* predefined Riverdi modules */
#include "modules.h"

/* 'riverdi-eve-lib' */
#include "Gpu_Hal.h"
#include "Gpu.h"
#include "CoPro_Cmds.h"
#include "Hal_Utils.h"

/******************************************************************************/

#define SPI_MODE      0
#define SPI_BITS      8
#define SPI_SPEED_HZ	20000000

/* Define GPIO pins - adjust these based on your board configuration */
typedef enum {
    GPIO_MISO = 14,  // P1.14
    GPIO_MOSI = 13,  // P1.13
    GPIO_SCLK = 15,  // P1.15
    GPIO_CS   = 12,  // P1.12
    GPIO_PD   = 10,  // P1.10
    GPIO_INT  = 11   // P1.11
} gpio_name;

typedef enum {
    GPIO_HIGH = 1,
    GPIO_LOW  = 0
} gpio_val;

/******************************************************************************/

bool_t
platform_init                 (Gpu_HalInit_t*);

void
platform_sleep_ms             (uint32_t);

bool_t
platform_spi_init             (Gpu_Hal_Context_t*);

void
platform_spi_deinit           (Gpu_Hal_Context_t*);

uchar8_t
platform_spi_send_recv_byte   (Gpu_Hal_Context_t*,
                               uchar8_t,
                               uint32_t);

uint16_t
platform_spi_send_data        (Gpu_Hal_Context_t*,
                               uchar8_t*,
                               uint16_t,
                               uint32_t);

void
platform_spi_recv_data        (Gpu_Hal_Context_t*,
                               uchar8_t*,
                               uint16_t,
                               uint32_t);

bool_t
platform_gpio_init            (Gpu_Hal_Context_t*,
                               gpio_name);

bool_t
platform_gpio_value           (Gpu_Hal_Context_t*,
                               gpio_name,
                               gpio_val);

/******************************************************************************/

#endif /*_PLATFORM_H_*/
