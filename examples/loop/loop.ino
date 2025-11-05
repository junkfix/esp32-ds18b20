// https://github.com/junkfix/esp32-ds18b20

#include "OneWireESP32.h"
const uint8_t MaxDevs = 2;
OneWire32::Result tempErr[MaxDevs];
bool tempReady[MaxDevs];
float currTemp[MaxDevs];
uint64_t addr[MaxDevs];

//uint64_t addr[] = {
//	0x183c01f09506f428,
//	0xf33c01e07683de28,
//};

void tempTask(void *pvParameters){
	OneWire32 ds(13); //gpio pin

	//to find addresses
	uint8_t devices = ds.search(addr, MaxDevs);
	for (uint8_t i = 0; i < devices; i += 1) {
		Serial.printf("%d: 0x%llx,\n", i, addr[i]);
		//char buf[20]; snprintf( buf, 20, "0x%llx,", addr[i] ); Serial.println(buf);
	}
	//end

	for(;;){
		ds.request();
		vTaskDelay(750 / portTICK_PERIOD_MS);
		for(byte i = 0; i < MaxDevs; i++){
			tempErr[i] = ds.getTemp(addr[i], currTemp[i]);
			tempReady[i] = true;
		}
		vTaskDelay(3000 / portTICK_PERIOD_MS); //sleep for 3 sec
	}
} // tempTask

void setup() {
	delay(1000);
	Serial.begin(115200);
	xTaskCreatePinnedToCore(tempTask, "tempTask", 2048,  NULL,  1,  NULL, 0);
}


void loop() {

	for(byte i = 0; i < MaxDevs; i++){
		if(tempReady[i]){
			tempReady[i] = false;
			if(tempErr[i]){
				const char *errt[] = {"", "CRC", "BAD","DC","DRV"};
				Serial.print(i); Serial.print(": "); Serial.println(errt[tempErr[i]]);
			}else{
				Serial.print(i); Serial.print(": "); Serial.println(currTemp[i]);
			}
		}
	}

  
}
