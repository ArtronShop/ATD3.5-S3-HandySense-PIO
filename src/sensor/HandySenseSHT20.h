#ifndef __HANDYSENSE_SHT20__
#define __HANDYSENSE_SHT20__

#include <Arduino.h>
#include <Wire.h>

#define SHT20_ADDR 0x40

class Artron_SHT20 {
    private:
        TwoWire *_wire;

        int write(uint8_t command, uint8_t *data = NULL, uint8_t len = 0) ;
        int read(uint8_t command, uint8_t *data = NULL, int len = 0, int stop = 100) ;

    public:
        Artron_SHT20(TwoWire*);

        bool begin() ;
        float readTemperature() ;
        float readHumidity() ;
    
};

// External use
int handySenseSHT20TempRead(float*) ;
int handySenseSHT20HumiRead(float*) ;

#endif
