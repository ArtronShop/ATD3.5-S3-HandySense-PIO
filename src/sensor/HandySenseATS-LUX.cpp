#include "HandySenseATS-LUX.h"

#define DE_RE_PIN 2
#define MODE_SEND HIGH
#define MODE_RECV LOW

uint16_t CRC16(uint8_t *buf, int len) {
	uint16_t crc = 0xFFFF;
	for (uint16_t pos = 0; pos < len; pos++) {
		crc ^= (uint16_t)buf[pos]; // XOR byte into least sig. byte of crc
		for (int i = 8; i != 0; i--) { // Loop over each bit
			if ((crc & 0x0001) != 0)
			{			   // If the LSB is set
				crc >>= 1; // Shift right and XOR 0xA001
				crc ^= 0xA001;
			}
			else
			{			   // Else LSB is not set
				crc >>= 1; // Just shift right
			}
		}
	}

	return crc;
}

bool readATS_LUX(float* value) {
	pinMode(RS485_DIR, OUTPUT);
	digitalWrite(RS485_DIR, MODE_RECV);

	Serial2.begin(4800, SERIAL_8N1, RS485_RX, RS485_TX); // Rx, Tx
	Serial2.setTimeout(200);

	uint8_t buff[] = {
		0x01, // Devices Address
		0x04, // Function code
		0x00, // Start Address HIGH
		0x01, // Start Address LOW
		0x00, // Quantity HIGH
		0x02, // Quantity LOW
		0,	  // CRC LOW
		0	  // CRC HIGH
	};

	uint16_t crc = CRC16(&buff[0], 6);
	buff[6] = crc & 0xFF;
	buff[7] = (crc >> 8) & 0xFF;

	digitalWrite(RS485_DIR, MODE_SEND);
	Serial2.write(buff, sizeof(buff));
	Serial2.flush(); // wait MODE_SEND completed
	digitalWrite(RS485_DIR, MODE_RECV);

	if (Serial2.find("\x01\x04")) {
		while(!Serial2.available()) delay(1);

		uint8_t n = Serial2.read();
		if (n != 4) {
			Serial.printf("Error data size : %d\n", n);
			return -20;
		}

		uint8_t data[4];
		size_t bytes = Serial2.readBytes(data, 4);
		if (bytes != 4) {
			Serial.printf("Error data size 2 : %d\n", n);
			return -30;
		}

		uint16_t low = ((uint16_t)(data[0]) << 8) | data[1];
		uint16_t high = ((uint16_t)(data[2]) << 8) | data[3];

		*value = (uint32_t)((high << 8) | low) / 1000.0f;
		Serial.printf("ATS-LUX : %.2f\n", *value);

		return 0;
	}
	
	Serial.println("ERROR Timeout");
	return -10;
}

// Real Program part
int handySenseRS485_ATS_LUX_LightRead(float *value) {
	return readATS_LUX(value);
}
