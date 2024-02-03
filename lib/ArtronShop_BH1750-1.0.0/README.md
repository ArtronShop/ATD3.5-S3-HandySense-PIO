# ArtronShop_BH1750

Arduino library of BH1750 Ambient Light Sensor

## Examples

```C++
#include <Arduino.h>
#include <Wire.h>
#include <ArtronShop_BH1750.h>

ArtronShop_BH1750 bh1750(0x5C, &Wire); // Non Jump ADDR: 0x23, Jump ADDR: 0x5C

void setup() {
  Serial.begin(115200);

  Wire.begin();
  while (!bh1750.begin()) {
    Serial.println("BH1750 not found !");
    delay(1000);
  }
}

void loop() {
  Serial.print("Light: ");
  Serial.print(bh1750.light());
  Serial.print(" lx");
  Serial.println();
  delay(1000);
}
```

