#ifndef __ARTRON_DS1338_H__
#define __ARTRON_DS1338_H__

#include "Arduino.h"
#include "Wire.h"
#include <time.h>

class Artron_DS1338 {
    private:
        TwoWire *wire = NULL;

        uint8_t BCDtoDEC(uint8_t n) ;
        uint8_t DECtoBCD(uint8_t n) ;

    public:
        Artron_DS1338(TwoWire *bus = &Wire);
        
        bool begin() ;
        bool read(struct tm *timeinfo) ;
        bool write(struct tm *timeinfo) ;

};

#endif

