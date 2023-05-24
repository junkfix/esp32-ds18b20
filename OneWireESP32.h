#pragma once

#include <Arduino.h>
#include "driver/rmt.h"

#define OW_DURATION_RESET   480
#define OW_DURATION_SLOT    75
#define OW_DURATION_1_LOW   6
#define OW_DURATION_1_HIGH  (OW_DURATION_SLOT - OW_DURATION_1_LOW)
#define OW_DURATION_0_LOW   65
#define OW_DURATION_0_HIGH  (OW_DURATION_SLOT - OW_DURATION_0_LOW)
#define OW_DURATION_SAMPLE  (15 - 2)
#define OW_DURATION_RX_IDLE (OW_DURATION_SLOT + 2)


class OneWire32 {
	private:
		uint8_t LastDeviceFlag;
		uint8_t LastDiscrepancy;
		uint8_t LastFamilyDiscrepancy;
		unsigned char ROM_NO[8];
		uint8_t owDefaultPower = 0;
		gpio_num_t owpin;
		gpio_num_t owgpio = GPIO_NUM_NC;
		rmt_channel_t owtx;
		rmt_channel_t owrx;
		RingbufHandle_t owbuf;
		rmt_item32_t readSlot(void);
		rmt_item32_t writeSlot(uint8_t val);
		bool begin();
		void flush(void);
		bool attach(gpio_num_t gpio_num);
	public:
		OneWire32(uint8_t gpio_num, uint8_t tx = 0, uint8_t rx = 1);
		bool reset();
		void select(const uint8_t *addr);
		void skip();
		uint8_t read();
		bool read_bits(gpio_num_t gpio_num, uint8_t *data, uint8_t num);
		void write(const uint8_t data, uint8_t power = 0);
		bool write_bits(gpio_num_t gpio_num, uint8_t data, uint8_t num, uint8_t power);
		bool search(uint8_t *addr);
		uint8_t crc8(const uint8_t *addr, int len);
};
