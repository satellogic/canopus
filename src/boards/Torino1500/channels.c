#include <canopus/types.h>
#include <canopus/drivers/channel.h>
#include <canopus/board/channels.h>
#include <canopus/drivers/tms570/uart.h>
#include <canopus/drivers/tms570/spi.h>
#include <canopus/drivers/tms570/i2c.h>
#include <canopus/drivers/memory/channel_link_driver.h>

#define UART_DEFAULT_TIMEOUT 	100
#define SPI_DEFAULT_TIMEOUT		100
#define I2C_DEFAULT_TIMEOUT		100

#define EPS_ADDRESS						0x2b
#define INA209_3V3_ADDRESS 				0x40
#define INA209_5V_ADDRESS 				0x41
#define INA209_12V_ADDRESS 				0x45

DECLARE_CHANNEL_UART(_chan_extuart, sciREG, SCI_TX_INT|SCI_RX_INT, 115200, UART_DEFAULT_TIMEOUT);
DECLARE_CHANNEL_UART(_chan_fpgauart, scilinREG, SCI_TX_INT|SCI_RX_INT, 9600, UART_DEFAULT_TIMEOUT);

DEFINE_CHANNEL_MEMORY_FRAMES(_ch_discard, NULL, NULL);


DEFINE_CHANNEL_SPI(_ch_fpga_ctrl, spiREG3, /*CS*/ 5, 16 /*word bits*/, SPI_FMT_0, /*HOLD*/ 0, /*WDel*/ 1, SPI_DEFAULT_TIMEOUT);

const channel_t *const ch_fpga_ctrl = &_ch_fpga_ctrl;
const channel_t *const ch_fpga_spi   = &_ch_discard;

// POWER

DEFINE_CHANNEL_I2C(_ch_eps, EPS_ADDRESS, 100, I2C_DEFAULT_TIMEOUT);
DEFINE_CHANNEL_I2C(_ch_ina_pd3v3, INA209_3V3_ADDRESS, 100, I2C_DEFAULT_TIMEOUT);
DEFINE_CHANNEL_I2C(_ch_ina_pd5v,  INA209_5V_ADDRESS, 100, I2C_DEFAULT_TIMEOUT);
DEFINE_CHANNEL_I2C(_ch_ina_pd12v, INA209_12V_ADDRESS, 100, I2C_DEFAULT_TIMEOUT);

const channel_t *const ch_eps = &_ch_eps;
const channel_t *const ch_ina_pd3v3 = &_ch_ina_pd3v3;
const channel_t *const ch_ina_pd5v  = &_ch_ina_pd5v;
const channel_t *const ch_ina_pd12v = &_ch_ina_pd12v;

// CDH
const channel_t *const ch_umbilical_in = &_chan_extuart;
const channel_t *const ch_umbilical_out = &_chan_extuart;
const channel_t *const ch_lithium = &_chan_fpgauart;

// AOCS
DEFINE_CHANNEL_SPI(_ch_adis, spiREG1, /*CS*/ 0, 16 /*word bits*/, SPI_FMT_0, /*HOLD*/ 0, /*WDel*/ 1, SPI_DEFAULT_TIMEOUT);
const channel_t *const ch_adis  = &_ch_adis;

// PAYLOAD
// POWER|PULLUP|AUX|MOSI|CLK|MISO|CS
const channel_t *const ch_svip_power  = &_ch_discard;
const channel_t *const ch_octopus_adc   = &_ch_discard;

// THERMAL
