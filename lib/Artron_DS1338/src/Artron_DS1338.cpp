#include "Artron_DS1338.h"

#define DS1338_ADDR 0x68

Artron_DS1338::Artron_DS1338(TwoWire *bus) {
    this->wire = bus;
}

bool Artron_DS1338::begin() {
    this->wire->beginTransmission(DS1338_ADDR);
    this->wire->write(0x07); // Start at address 0x07
    this->wire->write(0); // Write 0 to Oscillator Stop Flag for start
    if (this->wire->endTransmission() != 0) {
        return false;
    }

    return true;
}

bool Artron_DS1338::read(struct tm* timeinfo) {
    if (!timeinfo) {
        return false;
    }

    this->wire->beginTransmission(DS1338_ADDR);
    this->wire->write(0); // Start at address 0
    if (this->wire->endTransmission() != 0) {
        return false;
    }

    size_t len = this->wire->requestFrom(DS1338_ADDR, 7);
    if (len != 7) {
        return false;
    }

    uint8_t buff[7];
    this->wire->readBytes(buff, 7);

    timeinfo->tm_sec = BCDtoDEC(buff[0] & 0x7F);
    timeinfo->tm_min = BCDtoDEC(buff[1] & 0x7F);
    timeinfo->tm_hour = BCDtoDEC(buff[2] & 0x3F);
    timeinfo->tm_wday = BCDtoDEC(buff[3] & 0x07);
    timeinfo->tm_mday = BCDtoDEC(buff[4] & 0x3F);
    timeinfo->tm_mon = BCDtoDEC(buff[5] & 0x1F);
    timeinfo->tm_year = BCDtoDEC(buff[6]) + 2000 - 1900;

    return true;
}

bool Artron_DS1338::write(struct tm* timeinfo) {
    uint8_t buff[7];
    buff[0] = DECtoBCD(timeinfo->tm_sec) & 0x7F;
    buff[1] = DECtoBCD(timeinfo->tm_min) & 0x7F;
    buff[2] = DECtoBCD(timeinfo->tm_hour) & 0x3F;
    buff[3] = DECtoBCD(timeinfo->tm_wday) & 0x07;
    buff[4] = DECtoBCD(timeinfo->tm_mday) & 0x3F;
    buff[5] = DECtoBCD(timeinfo->tm_mon) & 0x1F;
    buff[6] = DECtoBCD((timeinfo->tm_year + 1900) % 100);

    this->wire->beginTransmission(DS1338_ADDR);
    this->wire->write(0); // Start at address 0
    this->wire->write(buff, 7);
    if (this->wire->endTransmission() != 0) {
        return false;
    }

    return true;
}

uint8_t Artron_DS1338::BCDtoDEC(uint8_t n) {
    return ((n >> 4)  * 10) + (n & 0x0F);
}

uint8_t Artron_DS1338::DECtoBCD(uint8_t n) {
    return (((n / 10) << 4) & 0xF0) | ((uint8_t)(n % 10) & 0x0F);
}
