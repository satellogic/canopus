#include <canopus/types.h>
#include <canopus/drivers/channel.h>
#include <canopus/board/channels.h>

#include <canopus/drivers/simusat/remote.h>

#define PORT_RADIO        10000

// ALL
const channel_t *const ch_fpga_ctrl  = &DECLARE_CHANNEL_REMOTE_PORTMAPPED("", "FpgaSimulator");
// const channel_t *const ch_fpga_spi  = &DECLARE_CHANNEL_REMOTE_PORTMAPPED("spi:nCS,30000,0", "FpgaSimulatorSpi");

// POWER
const channel_t *const ch_eps = &DECLARE_CHANNEL_REMOTE_PORTMAPPED("i2c:0x2b,100000", "EPS");	// EPSSimulator");
const channel_t *const ch_ina_pd3v3 = &DECLARE_CHANNEL_REMOTE_PORTMAPPED("i2c:0x40,100000", "I2C");
const channel_t *const ch_ina_pd5v  = &DECLARE_CHANNEL_REMOTE_PORTMAPPED("i2c:0x41,100000", "I2C");
const channel_t *const ch_ina_pd12v = &DECLARE_CHANNEL_REMOTE_PORTMAPPED("i2c:0x45,100000", "I2C");

// CDH
const channel_t *const ch_lithium = &DECLARE_CHANNEL_TCP_CLIENT("127.0.0.1", PORT_RADIO);

// AOCS
const channel_t *const ch_adis  = &DECLARE_CHANNEL_REMOTE_PORTMAPPED("spi:nCS,250000,3", "IMU"); // SPI");
xSemaphoreHandle xSemaphore_imu_dready;

// PAYLOAD
// POWER|PULLUP|AUX|MOSI|CLK|MISO|CS
const channel_t *const ch_svip_power  = &DECLARE_CHANNEL_REMOTE_PORTMAPPED("no gpio:i=0b0000000,o=0b1000000", "GPIO");
const channel_t *const ch_octopus_adc   = &DECLARE_CHANNEL_REMOTE_PORTMAPPED("adc:", "BusPirateBridge");

// THERMAL
