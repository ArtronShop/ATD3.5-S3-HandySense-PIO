#ifndef __ARTRON_DS1338_H__
#define __ARTRON_DS1338_H__

#include "Arduino.h"
#include "Wire.h"
#include <time.h>

enum RTC_Type {
    UNKNOW,
    DS1338,
    MCP79411,
    PCF8563
};

class ArtronShop_RTC {
    private:
        TwoWire *wire = NULL;
        RTC_Type type;
        int devAddr = 0x00;

        uint8_t BCDtoDEC(uint8_t n) ;
        uint8_t DECtoBCD(uint8_t n) ;

        bool CheckI2CDevice(int addr) ;

    public:
        ArtronShop_RTC(TwoWire *bus = &Wire, RTC_Type type = UNKNOW);
        
        bool begin() ;
        bool read(struct tm *timeinfo) ;
        bool write(struct tm *timeinfo) ;

};

#endif