
#include "OneWireESP32.h"

float currTemp[2] = {0.0, 0.0};


void tempTask(void *pvParameters){
	OneWire32 ds(13, 0, 1);// gpio pin, RMT channels for TX, RX, there are 8 (0-7) channels available on ESP32
	byte addr[2][8] = {
		{0x28, 0xF4, 0x6, 0x95, 0xF0, 0x1, 0x3C, 0x18},   //first
		{0x28, 0xde, 0x83, 0x76, 0xe0, 0x1, 0x3c, 0xf3}    //second
	};

	//search first time to get all the addresses
	byte srch[8];
	while( ds.search(srch) ){
		Serial.print("{");
		for(byte i = 0; i < 8; i++){
			Serial.print("0x"); Serial.print(srch[i], HEX);
			if(i == 7){Serial.println("}");}else{Serial.print(", ");}
		}
		Serial.println();
	}

	for(;;){
		ds.reset(); ds.skip();
		ds.write(0x44, 0); // start conversion, 1=with parasite power on at the end
		vTaskDelay(750 / portTICK_PERIOD_MS);
		for(byte i = 0; i < 2; i++){
			Serial.print(i); Serial.print(" = "); 
			if(ds.reset()){ //connected
				ds.select(addr[i]); ds.write(0xBE); // Read
				byte data[9]; uint16_t zero = 0;
				int16_t freshTemp = 0;
				for(byte j = 0; j < 9; j++){
					data[j] = ds.read();
					zero += data[j];
				}
				if(zero != 0x8f7 && zero){
					if(data[8] == ds.crc8(data, 8) ){ //CRC OK
						freshTemp = (data[1] << 8) | data[0];
						currTemp[i] = ((float)freshTemp / 16.0);
						Serial.println(currTemp[i]);
					}else{
						Serial.println("CRC Error");
					}
				}else{
					Serial.println("No Data");
				}
			}else{
				Serial.println("Disconnected");
			}
		}
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
} /* tempTask */


void setup() {
	delay(1000);
	Serial.begin(115200);

	xTaskCreatePinnedToCore(tempTask, "tempTask", 8192,  NULL,  1,  NULL, 0);
}

void loop() {
}
