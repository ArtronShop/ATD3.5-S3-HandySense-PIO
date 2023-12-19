#ifndef __HANDYSENSE_SHT30__
#define __HANDYSENSE_SHT30__

#include <Arduino.h>
#include <Wire.h>
#include <math.h>

class SHT3x
{
public:
    enum ValueIfError //Define, what to return in case of error: Zero or previous value
    {
        Zero,
        PrevValue
    };
    enum SHT3xMode
    {
        Single_HighRep_ClockStretch,
        Single_MediumRep_ClockStretch,
        Single_LowRep_ClockStretch,
        Single_HighRep_NoClockStretch,
        Single_MediumRep_NoClockStretch,
        Single_LowRep_NoClockStretch
    };
    enum SHT3xSensor
    {
        SHT30,
        SHT31,
        SHT35
    };
    enum TemperatureScale
    {
        Cel,
        Far,
        Kel
    };
    enum AbsHumidityScale
    {
        mmHg,
        Torr, //same as mm Hg
        Pa,
        Bar,
        At,  //Techical atmosphere
        Atm, //Standart atmosphere
        mH2O,
        psi,
    };
    struct CalibrationPoints
    {
        float First;
        float Second;
    };
    struct CalibrationFactors
    {
        CalibrationFactors() : Factor(1.), Shift(0.) {}
        float Factor;
        float Shift;
    };

    SHT3x(int Address = 0x44,
          ValueIfError Value = Zero,
          uint8_t HardResetPin = 255,
          SHT3xSensor SensorType = SHT30,
          SHT3xMode Mode = Single_HighRep_ClockStretch);

    void Begin(int SDA = 0,
               int SCL = 0);
    void UpdateData(int SDA = 0);

    float GetTemperature(TemperatureScale Degree = Cel);
    float GetRelHumidity();
    float GetAbsHumidity(AbsHumidityScale Scale = Torr);
    float GetTempTolerance(TemperatureScale Degree = Cel, SHT3xSensor SensorType = SHT30);
    float GetRelHumTolerance(SHT3xSensor SensorType = SHT30);
    float GetAbsHumTolerance(AbsHumidityScale Scale = Torr, SHT3xSensor SensorType = SHT30);

    uint8_t GetError();

    void SetMode(SHT3xMode Mode = Single_HighRep_ClockStretch);
    void SetTemperatureCalibrationFactors(CalibrationFactors TemperatureCalibration);
    void SetRelHumidityCalibrationFactors(CalibrationFactors RelHumidityCalibration);
    void SetTemperatureCalibrationPoints(CalibrationPoints SensorValues, CalibrationPoints Reference);
    void SetRelHumidityCalibrationPoints(CalibrationPoints SensorValues, CalibrationPoints Reference);

    void SoftReset(int SDA = 0);
    void HardReset();

    void HeaterOn(int SDA = 0);
    void HeaterOff(int SDA = 0);

    void SetAddress(uint8_t NewAddress);
    void SetUpdateInterval(uint32_t UpdateIntervalMillisec);
    void SetTimeout(uint32_t TimeoutMillisec);

private:
    float _TemperatureCeil;
    float _RelHumidity;
    bool _OperationEnabled = false;
    uint8_t _HardResetPin;
    ValueIfError _ValueIfError;
    uint8_t _MeasLSB; //Data read command, Most Significant Byte
    uint8_t _MeasMSB; //Data read command, Most Significant Byte
    uint8_t _Address;
    SHT3xSensor _SensorType;
    uint32_t _UpdateIntervalMillisec = 500;
    uint32_t _LastUpdateMillisec = 0;
    uint32_t _TimeoutMillisec = 100;
    void SendCommand(uint8_t MSB, uint8_t LSB, int SDA = 0);
    bool CRC8(uint8_t MSB, uint8_t LSB, uint8_t CRC);
    float ReturnValueIfError(float Value);
    void ToReturnIfError(ValueIfError Value);
    CalibrationFactors _TemperatureCalibration;
    CalibrationFactors _RelHumidityCalibration;

    /* 
			* 	Factors for poly for calculating absolute humidity (in Torr):
			*	P = (RelativeHumidity /100%) * sum(_AbsHumPoly[i]*T^i)
			*	where P is absolute humidity (Torr/mm Hg),
			*	T is Temperature(Kelvins degree) / 1000,
			* 	^ means power.
			*	For more data, check the NIST chemistry webbook:
			*	http://webbook.nist.gov/cgi/cbook.cgi?ID=C7732185&Units=SI&Mask=4&Type=ANTOINE&Plot=on#ANTOINE
		*/
    float _AbsHumPoly[6] = {-157.004, 3158.0474, -25482.532, 103180.197, -209805.497, 171539.883};

    enum Errors
    {
        noError = 0,
        Timeout = 1,
        DataCorrupted = 2,
        WrongAddress = 3,
        //I2C errors
        TooMuchData = 4,
        AddressNACK = 5,
        DataNACK = 6,
        OtherI2CError = 7,
        UnexpectedError = 8
    } _Error;
    void I2CError(uint8_t I2Canswer);
};

// External use
int handySenseSHT30TempRead(float *);
int handySenseSHT30HumiRead(float *);

#endif
