#include <Wire.h>
#include <ADS1X15.h>

Adafruit_ADS1115 ads(0X49);  /* Use this for the 16-bit version */
//Adafruit_ADS1015 ads;     /* Use this for the 12-bit version */

//Setup some variables used to show how long the ADC conversion takes place
unsigned long starttime;
unsigned long endtime ;
int16_t Raw;

void setup(void)
{
    Serial.begin(115200);
    Serial.println("Hello!");

    Serial.println("Getting single-ended readings from AIN0..3");
    Serial.println("ADC Range: +/- 6.144V (1 bit = 3mV/ADS1015, 0.1875mV/ADS1115)");

    // The ADC input range (or gain) can be changed via the following
    // functions, but be careful never to exceed VDD +0.3V max, or to
    // exceed the upper and lower limits if you adjust the input range!
    // Setting these values incorrectly may destroy your ADC!
    //                                                                ADS1015  ADS1115
    //                                                                -------  -------
//     ads.setGain(GAIN_TWOTHIRDS);  // 2/3x gain +/- 6.144V  1 bit = 3mV      0.1875mV (default)
     ads.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
//     ads.setGain(GAIN_TWO);        // 2x gain   +/- 2.048V  1 bit = 1mV      0.0625mV
    // ads.setGain(GAIN_FOUR);       // 4x gain   +/- 1.024V  1 bit = 0.5mV    0.03125mV
    // ads.setGain(GAIN_EIGHT);      // 8x gain   +/- 0.512V  1 bit = 0.25mV   0.015625mV
    // ads.setGain(GAIN_SIXTEEN);    // 16x gain  +/- 0.256V  1 bit = 0.125mV  0.0078125mV
    ads.setSPS(ADS1115_DR_860SPS);
    ads.begin();

}

void loop(void)
{
//    calcSPS(1000);
    int16_t adc0, adc1, adc2, adc3;
//
    starttime = micros(); //Record a start time for demonstration
    adc0 = ads.readADC_SingleEnded(0);
//    adc1 = ads.readADC_SingleEnded(1);
//    adc2 = ads.readADC_SingleEnded(2);
//    adc3 = ads.readADC_SingleEnded(3);
    endtime = micros();
//    Serial.print("Conversion complete: "); Serial.print(adc0); Serial.print(",  "); Serial.print(endtime - starttime);  Serial.print("us");
    Serial.print("Conversion complete: ");  Serial.print(endtime - starttime);  Serial.println("us");
    Serial.println("=======================");
}

void calcSPS(long period)
{
    int16_t adc0, adc1, adc2, adc3;
    unsigned long tempsInitial = millis();
    unsigned long tempsCurrent = tempsInitial;
    int  count = 0;

    while (tempsCurrent-tempsInitial<period)
    {
        adc0 = ads.readADC_SingleEnded(0);
        adc1 = ads.readADC_SingleEnded(1);
        adc2 = ads.readADC_SingleEnded(2);
        adc3 = ads.readADC_SingleEnded(3);
        tempsCurrent = millis();
        count++;
    }

    Serial.print("SPS=");
    Serial.println(count);
}