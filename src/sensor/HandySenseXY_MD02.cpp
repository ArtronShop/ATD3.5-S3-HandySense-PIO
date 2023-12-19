#include "HandySenseXY_MD02.h"
#include "PinConfigs.h"

static uint16_t CRC16(uint8_t *buf, int len) {
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

bool readXY_MD02(uint8_t start_addr, float* value) {
	Serial2.begin(9600, SERIAL_8N1, RS485_RX_PIN, RS485_TX_PIN); // Rx, Tx
	Serial2.setTimeout(200);

	uint8_t buff[] = {
		0x01, // Devices Address
		0x04, // Function code
		0x00, // Start Address HIGH
		start_addr, // 0x01, // Start Address LOW
		0x00, // Quantity HIGH
		0x01, // 0x02, // Quantity LOW
		0,	  // CRC LOW
		0	  // CRC HIGH
	};

	uint16_t crc = CRC16(&buff[0], 6);
	buff[6] = crc & 0xFF;
	buff[7] = (crc >> 8) & 0xFF;

	Serial2.write(buff, sizeof(buff));
	Serial2.flush(); // wait MODE_SEND completed

	if (Serial2.find("\x01\x04")) {
		while(!Serial2.available()) delay(1);

		uint8_t n = Serial2.read();
		// if (n != 4) {
		if (n != 2) {
			Serial.printf("Error data size : %d\n", n);
			return -20;
		}

		*value = ((uint16_t)(Serial2.read() << 8) | Serial2.read()) / 10.0;

		return 0;
	}
	
	Serial.println("ERROR Timeout");
	return -10;
}

// Real Program part
int handySenseXYMD02TempRead(float *value) {
	return readXY_MD02(0x01, value);
}

int handySenseXYMD02HumiRead(float *value) {
	return readXY_MD02(0x02, value);
}
