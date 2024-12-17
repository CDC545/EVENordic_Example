/*
 * Copyright (c) Riverdi Sp. z o.o. sp. k. <contact@riverdi.com>
 * Copyright (c) Skalski Embedded Technologies <contact@lukasz-skalski.com>
 */

#include "platform.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(riverdi_platform, CONFIG_LOG_DEFAULT_LEVEL);

/* SPI device structure */
static const struct device *spi_dev;
static const struct device *gpio_dev;
static struct spi_config spi_cfg;

/*
 * platform_init()
 */
bool_t
platform_init(Gpu_HalInit_t *halinit)
{
    /* Get SPI device */
    spi_dev = DEVICE_DT_GET(DT_NODELABEL(spi2));
    if (!device_is_ready(spi_dev)) {
        return FALSE;
    }

    /* Get GPIO device */
    gpio_dev = DEVICE_DT_GET(DT_NODELABEL(gpio1));
    if (!device_is_ready(gpio_dev)) {
        return FALSE;
    }

    return TRUE;
}

/*
 * platform_sleep_ms()
 */
void
platform_sleep_ms(uint32_t ms)
{
    k_msleep(ms);
}

/*
 * platform_spi_init()
 */
bool_t
platform_spi_init(Gpu_Hal_Context_t *host)
{
    spi_cfg.frequency = SPI_SPEED_HZ;
    spi_cfg.operation = SPI_OP_MODE_MASTER | 
                       SPI_WORD_SET(8) |
                       SPI_TRANSFER_MSB;
    return TRUE;
}

/*
 * platform_spi_deinit()
 */
void
platform_spi_deinit(Gpu_Hal_Context_t *host)
{
    return;
}

/*
 * platform_spi_send_recv_byte()
 */
uchar8_t
platform_spi_send_recv_byte(Gpu_Hal_Context_t *host,
                           uchar8_t data,
                           uint32_t opt)
{
    uint8_t rx_data;
    struct spi_buf tx_buf = {
        .buf = &data,
        .len = 1
    };
    struct spi_buf rx_buf = {
        .buf = &rx_data,
        .len = 1
    };
    struct spi_buf_set tx_bufs = {
        .buffers = &tx_buf,
        .count = 1
    };
    struct spi_buf_set rx_bufs = {
        .buffers = &rx_buf,
        .count = 1
    };

    spi_transceive(spi_dev, &spi_cfg, &tx_bufs, &rx_bufs);
    return rx_data;
}

/*
 * platform_spi_send_data()
 */
uint16_t
platform_spi_send_data(Gpu_Hal_Context_t *host,
                      uchar8_t *data,
                      uint16_t size,
                      uint32_t opt)
{
    struct spi_buf tx_buf = {
        .buf = data,
        .len = size
    };
    struct spi_buf_set tx_bufs = {
        .buffers = &tx_buf,
        .count = 1
    };

    spi_write(spi_dev, &spi_cfg, &tx_bufs);
    return size;
}

/*
 * platform_spi_recv_data()
 */
void
platform_spi_recv_data(Gpu_Hal_Context_t *host,
                      uchar8_t *data,
                      uint16_t size,
                      uint32_t opt)
{
    struct spi_buf rx_buf = {
        .buf = data,
        .len = size
    };
    struct spi_buf_set rx_bufs = {
        .buffers = &rx_buf,
        .count = 1
    };

    spi_read(spi_dev, &spi_cfg, &rx_bufs);
}

/*
 * platform_gpio_init()
 */
bool_t
platform_gpio_init(Gpu_Hal_Context_t *host,
                  gpio_name ngpio)
{
    printk("GPIO init: pin %d\n", ngpio);
    gpio_pin_configure(gpio_dev, ngpio, GPIO_OUTPUT);
    return TRUE;
}

/*
 * platform_gpio_value()
 */
bool_t
platform_gpio_value(Gpu_Hal_Context_t *host,
                   gpio_name ngpio,
                   gpio_val vgpio)
{
   // printk("GPIO set: pin %d = %d\n", ngpio, vgpio);
    gpio_pin_set(gpio_dev, ngpio, vgpio);
    return TRUE;
}
