#ifndef __HANDYSENSE_SHT45__
#define __HANDYSENSE_SHT45__

#include <Arduino.h>
#include <Wire.h>

class ArtronShop_SHT45 {
    private:
        uint8_t _addr;
        TwoWire *_wire = NULL;
        float _t, _h;

    public:
        ArtronShop_SHT45(TwoWire *wire = &Wire, uint8_t addr = 0x44) ;
        bool begin() ;
        bool measure() ;
        float temperature() ;
        float humidity() ;

};

// External use
int handySenseSHT45TempRead(float*) ;
int handySenseSHT45HumiRead(float*) ;

#endif
