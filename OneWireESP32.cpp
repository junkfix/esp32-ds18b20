/*
Adaptation of Paul Stoffregen's One wire library to the NodeMcu
Ported to ESP32 RMT peripheral for low-level signal generation by Arnim Laeuger.
https://github.com/nodemcu/nodemcu-firmware/blob/b8ae6ca6f809f735616238ce879a47e0ff1e9bc4/components/platform/onewire.c
The latest version of this library may be found at:
http://www.pjrc.com/teensy/td_libs_OneWire.html
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
Much of the code was inspired by Derek Yerger's code, though I don't think much of that remains.  In any event that was..     (copyleft) 2006 by Derek Yerger - Free to distribute freely.
The CRC code was excerpted and inspired by the Dallas Semiconductor sample code bearing this copyright.
Copyright (C) 2000 Dallas Semiconductor Corporation, All Rights Reserved.
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL DALLAS SEMICONDUCTOR BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
Except as contained in this notice, the name of Dallas Semiconductor shall not be used except as stated in the Dallas Semiconductor Branding Policy.
*/
#include <Arduino.h>
#include "OneWireESP32.h"


OneWire32::OneWire32(uint8_t gpio_num, uint8_t tx, uint8_t rx){
	owpin = static_cast<gpio_num_t>(gpio_num);
	owtx = static_cast<rmt_channel_t>(tx);
	owrx = static_cast<rmt_channel_t>(rx);
	begin();
}


bool OneWire32::begin(){
	rmt_config_t rmt_tx;
	rmt_tx.channel = owtx;
	rmt_tx.gpio_num = owpin;
	rmt_tx.mem_block_num = 1;
	rmt_tx.clk_div = 80;
	rmt_tx.tx_config.loop_en = false;
	rmt_tx.tx_config.carrier_en = false;
	rmt_tx.tx_config.idle_level = (rmt_idle_level_t)1;
	rmt_tx.tx_config.idle_output_en = true;
	rmt_tx.rmt_mode = RMT_MODE_TX;
	if(rmt_config(&rmt_tx) == ESP_OK){
		if(rmt_driver_install(rmt_tx.channel, 0, ESP_INTR_FLAG_LOWMED | ESP_INTR_FLAG_IRAM | ESP_INTR_FLAG_SHARED) == ESP_OK){
			rmt_set_source_clk(rmt_tx.channel, RMT_BASECLK_APB);
			rmt_config_t rmt_rx;
			rmt_rx.channel = owrx;
			rmt_rx.gpio_num = owpin;
			rmt_rx.clk_div = 80;
			rmt_rx.mem_block_num = 1;
			rmt_rx.rmt_mode = RMT_MODE_RX;
			rmt_rx.rx_config.filter_en = true;
			rmt_rx.rx_config.filter_ticks_thresh = 30;
			rmt_rx.rx_config.idle_threshold = OW_DURATION_RX_IDLE;
			if(rmt_config(&rmt_rx) == ESP_OK){
				if(rmt_driver_install(rmt_rx.channel, 512, ESP_INTR_FLAG_LOWMED | ESP_INTR_FLAG_IRAM | ESP_INTR_FLAG_SHARED) == ESP_OK){
					rmt_set_source_clk(rmt_rx.channel, RMT_BASECLK_APB);
					rmt_get_ringbuf_handle(owrx, &owbuf);
					return true;
				}else{
					return false;
				}
			}else{
				return false;
			}
			rmt_driver_uninstall(rmt_tx.channel);
		}else{
			return false;
		}
	}else{
		return false;
	}
	return false;
} // begin


bool OneWire32::reset(){
	rmt_item32_t tx_items[1];
	bool _presence = false;
	int res = true;
	gpio_num_t gpio_num = owpin;
	if(attach(gpio_num) != true){
		return false;
	}
	GPIO.pin[gpio_num].pad_driver = 1;
	tx_items[0].duration0 = OW_DURATION_RESET;
	tx_items[0].level0 = 0;
	tx_items[0].duration1 = 0;
	tx_items[0].level1 = 1;
	uint16_t old_rx_thresh;
	rmt_get_rx_idle_thresh(owrx, &old_rx_thresh);
	rmt_set_rx_idle_thresh(owrx, OW_DURATION_RESET + 60);
	flush();
	rmt_rx_start(owrx, true);
	if(rmt_write_items(owtx, tx_items, 1, true) == ESP_OK){
		size_t rx_size;
		rmt_item32_t *rx_items = (rmt_item32_t *)xRingbufferReceive(owbuf, &rx_size, 100 / portTICK_PERIOD_MS);
		if(rx_items){
			if(rx_size >= 1 * sizeof( rmt_item32_t)){
				if((rx_items[0].level0 == 0) && (rx_items[0].duration0 >= OW_DURATION_RESET - 2)){
					if((rx_items[0].level1 == 1) && (rx_items[0].duration1 > 0)){
						if(rx_items[1].level0 == 0){
							_presence = true;
						}
					}
				}
			}
			vRingbufferReturnItem(owbuf, (void *)rx_items);
		}else{
			res = false;
		}
	}else{
		res = false;
	}
	rmt_rx_stop(owrx);
	rmt_set_rx_idle_thresh(owrx, old_rx_thresh);
	(void)res;
	return _presence;
} // reset


void OneWire32::select(const uint8_t *addr){
	write_bits(owpin, 0x55, 8, owDefaultPower);
	for(int i = 0; i < 8; i++){
		write_bits(owpin, addr[i], 8, owDefaultPower);
	}
}


void OneWire32::skip(){
	write_bits(owpin, 0xCC, 8, owDefaultPower);
}


void OneWire32::flush(void){
	void *p;
	size_t s;
	while((p = xRingbufferReceive(owbuf, &s, 0))){
		vRingbufferReturnItem(owbuf, p);
	}
}


bool OneWire32::attach(gpio_num_t gpio_num){
	if(owtx < 0 || owrx < 0){
		return false;
	}
	if(gpio_num != owgpio){
		if(gpio_num < 32){
			GPIO.enable_w1ts = (0x1 << gpio_num);
		}else{
			GPIO.enable1_w1ts.data = (0x1 << (gpio_num - 32));
		}
		if(owgpio >= 0){
			gpio_matrix_out(owgpio, SIG_GPIO_OUT_IDX, 0, 0);
		}
		rmt_set_gpio(owrx, RMT_MODE_RX, gpio_num, false);
		rmt_set_gpio(owtx, RMT_MODE_TX, gpio_num, false);
		PIN_INPUT_ENABLE(GPIO_PIN_MUX_REG[gpio_num]);

		GPIO.pin[gpio_num].pad_driver = 1;
		owgpio = gpio_num;
	}
	return true;
} // attach


void OneWire32::write(const uint8_t data, uint8_t power){
	owDefaultPower = power;
	write_bits(owpin, data, 8, owDefaultPower);
}


bool OneWire32::write_bits(gpio_num_t gpio_num, uint8_t data, uint8_t num, uint8_t power){
	rmt_item32_t tx_items[num + 1];
	if(num > 8){return false;}
	if(attach(gpio_num) != true){return false;}
	if(power){
		GPIO.pin[gpio_num].pad_driver = 0;
	}else{
		GPIO.pin[gpio_num].pad_driver = 1;
	}
	for(int i = 0; i < num; i++){
		tx_items[i] = writeSlot(data & 0x01);
		data >>= 1;
	}
	tx_items[num].level0 = 1;
	tx_items[num].duration0 = 0;
	if(rmt_write_items(owtx, tx_items, num + 1, true) == ESP_OK){
		return true;
	}else{
		return false;
	}
} // write_bits


rmt_item32_t OneWire32::writeSlot(uint8_t val){
	rmt_item32_t item;
	item.level0 = 0;
	item.level1 = 1;
	if(val){
		item.duration0 = OW_DURATION_1_LOW;
		item.duration1 = OW_DURATION_1_HIGH;
	}else{
		item.duration0 = OW_DURATION_0_LOW;
		item.duration1 = OW_DURATION_0_HIGH;
	}
	return item;
}


uint8_t OneWire32::read(){
	uint8_t ret = 0;
	if(read_bits(owpin, &ret, 8) != true){
		return 0;
	}
	return ret;
}


bool OneWire32::read_bits(gpio_num_t gpio_num, uint8_t *data, uint8_t num){
	rmt_item32_t tx_items[num + 1];
	uint8_t read_data = 0;
	int ret = true;
	if(num > 8){
		return false;
	}
	if(attach(gpio_num) != true){
		return false;
	}
	GPIO.pin[gpio_num].pad_driver = 1;
	for(int i = 0; i < num; i++){
		tx_items[i] = readSlot();
	}
	tx_items[num].level0 = 1;
	tx_items[num].duration0 = 0;
	flush();
	rmt_rx_start(owrx, true);
	if(rmt_write_items(owtx, tx_items, num + 1, true) == ESP_OK){
		size_t rx_size;
		rmt_item32_t *rx_items = (rmt_item32_t *)xRingbufferReceive(owbuf, &rx_size, portMAX_DELAY);

		if(rx_items){
			if(rx_size >= num * sizeof( rmt_item32_t)){
				for(int i = 0; i < num; i++){
					read_data >>= 1;
					if(rx_items[i].level1 == 1){
						if((rx_items[i].level0 == 0) && (rx_items[i].duration0 < OW_DURATION_SAMPLE)){
							read_data |= 0x80;
						}
					}
				}
				read_data >>= 8 - num;
			}
			vRingbufferReturnItem(owbuf, (void *)rx_items);
		}else{
			ret = false;
		}
	}else{
		ret = false;
	}
	rmt_rx_stop(owrx);
	*data = read_data;
	return ret;
} // read_bits


rmt_item32_t OneWire32::readSlot(void){
	rmt_item32_t item;
	item.level0 = 0;
	item.duration0 = OW_DURATION_1_LOW;
	item.level1 = 1;
	item.duration1 = OW_DURATION_1_HIGH;
	return item;
}


bool OneWire32::search(uint8_t *addr){
	uint8_t id_bit_number;
	uint8_t last_zero, rom_byte_number, search_result;
	uint8_t id_bit, cmp_id_bit;
	unsigned char rom_byte_mask, search_direction;
	id_bit_number = 1;
	last_zero = 0;
	rom_byte_number = 0;
	rom_byte_mask = 1;
	search_result = 0;
	if(!LastDeviceFlag){
		if(reset() != true){
			LastDiscrepancy = 0;
			LastDeviceFlag = false;
			LastFamilyDiscrepancy = 0;
			return false;
		}
		write_bits(owpin, 0xF0, 8, owDefaultPower);
		do {
			if(read_bits(owpin, &id_bit, 1) != true){
				break;
			}
			if(read_bits(owpin, &cmp_id_bit, 1) != true){
				break;
			}
			if((id_bit == 1) && (cmp_id_bit == 1)){
				break;
			}else{
				if(id_bit != cmp_id_bit){
					search_direction = id_bit;
				}else{
					if(id_bit_number < LastDiscrepancy){
						search_direction = ((ROM_NO[rom_byte_number] & rom_byte_mask) > 0);
					}else{
						search_direction = (id_bit_number == LastDiscrepancy);
					}
					if(search_direction == 0){
						last_zero = id_bit_number;
						if(last_zero < 9){
							LastFamilyDiscrepancy = last_zero;
						}
					}
				}
				if(search_direction == 1){
					ROM_NO[rom_byte_number] |= rom_byte_mask;
				}else{
					ROM_NO[rom_byte_number] &= ~rom_byte_mask;
				}
				write_bits(owpin, search_direction, 1, owDefaultPower);
				id_bit_number++;
				rom_byte_mask <<= 1;
				if(rom_byte_mask == 0){
					rom_byte_number++;
					rom_byte_mask = 1;
				}
			}
		} while(rom_byte_number < 8);

		if(!(id_bit_number < 65)){
			LastDiscrepancy = last_zero;
			if(LastDiscrepancy == 0){
				LastDeviceFlag = true;
			}
			search_result = true;
		}
	}

	if(!search_result || !ROM_NO[0]){
		LastDiscrepancy = 0;
		LastDeviceFlag = false;
		LastFamilyDiscrepancy = 0;
		for(int i = 7;; i--){
			ROM_NO[i] = 0;
			if(i == 0)break;
		}
		search_result = false;
	}else{
		for(rom_byte_number = 0; rom_byte_number < 8; rom_byte_number++){
			addr[rom_byte_number] = ROM_NO[rom_byte_number];
		}
	}
	return search_result;
} // search


uint8_t OneWire32::crc8(const uint8_t *addr, int len){
	const uint8_t crc_table[] = {0, 94, 188, 226, 97, 63, 221, 131, 194, 156, 126, 32, 163, 253, 31, 65, 157, 195, 33, 127, 252, 162, 64, 30, 95, 1, 227, 189, 62, 96, 130, 220, 35, 125, 159, 193, 66, 28, 254, 160, 225, 191, 93, 3, 128, 222, 60, 98, 190, 224, 2, 92, 223, 129, 99, 61, 124, 34, 192, 158, 29, 67, 161, 255, 70, 24, 250, 164, 39, 121, 155, 197, 132, 218, 56, 102, 229, 187, 89, 7, 219, 133, 103, 57, 186, 228, 6, 88, 25, 71, 165, 251, 120, 38, 196, 154, 101, 59, 217, 135, 4, 90, 184, 230, 167, 249, 27, 69, 198, 152, 122, 36, 248, 166, 68, 26, 153, 199, 37, 123, 58, 100, 134, 216, 91, 5, 231, 185, 140, 210, 48, 110, 237, 179, 81, 15, 78, 16, 242, 172, 47, 113, 147, 205, 17, 79, 173, 243, 112, 46, 204, 146, 211, 141, 111, 49, 178, 236, 14, 80, 175, 241, 19, 77, 206, 144, 114, 44, 109, 51, 209, 143, 12, 82, 176, 238, 50, 108, 142, 208, 83, 13, 239, 177, 240, 174, 76, 18, 145, 207, 45, 115, 202, 148, 118, 40, 171, 245, 23, 73, 8, 86, 180, 234, 105, 55, 213, 139, 87, 9, 235, 181, 54, 104, 138, 212, 149, 203, 41, 119, 244, 170, 72, 22, 233, 183, 85, 11, 136, 214, 52, 106, 43, 117, 151, 201, 74, 20, 246, 168, 116, 42, 200, 150, 21, 75, 169, 247, 182, 232, 10, 84, 215, 137, 107, 53};
	uint8_t ret = 0x00;
	while(len-- > 0){
		ret = crc_table[ret ^ *addr++];
	}
	return ret;
}
