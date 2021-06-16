//
// Created by jpipe on 6/15/2021.
//

#include "MMG.h"

// Constructor
MMG::MMG(uint8_t i2cAddress){
    MMG_ads = new Adafruit_ADS1115(i2cAddress);
    curTxMode = DATA_RAW;
};

/**
 * @description: This function starts the ADS1015 module setting gain and Sample rate
*/
void MMG::begin(adsGain_t GAIN, adsSPS_t SAMPLE_RATE){
    MMG_ads->setGain(GAIN);        // 2x gain   +/- 2.048V  1 bit = 1mV (2x FSR, 16x piezo)
    MMG_ads->setSPS(SAMPLE_RATE); //3300 SPS -> Each channel takes around 630 us to be read
    MMG_ads->begin();
}

/**
 * @description: This function update the data acquired by the ADS1015
*/
void MMG::updateMMGData(void){
    for (int chan = 0; chan < MMG_CHANNELS; chan++){
        MMGData[chan] = MMG_ads->readADC_SingleEnded(chan);
    }
}

/**
* @description Writes data to serial port.
*/
void MMG::sendMMGData(boolean serial_stream)
{
    if(serial_stream) {
        if (curTxMode == DATA_RAW) {sendMMGDataSerial_Raw();}
        if (curTxMode == DATA_ASCII) {sendMMGDataSerial_Ascii();}
    }
}

/**
* @description Writes channel data to serial port sending chunks of 8 bytes.
*/
void MMG::sendMMGDataSerial_Raw(void)
{
    for (int i = 0; i < MMG_CHANNELS; i++)
    {
        Serial.write((uint8_t)highByte(MMGData[i]));
        Serial.write((uint8_t)lowByte(MMGData[i]));
    }
}

/**
* @description Writes channel data to serial port sending whole data in ascii format.
*/
void MMG::sendMMGDataSerial_Ascii(void)
{
    for (int i = 0; i < MMG_CHANNELS; i++)
    {
        Serial.print(MMGData[i]);
        Serial.print(" ");
    }
}

/**
* @description Changes the current transmission mode
*/
void MMG::setCurTxMode(TX_MODE TxMode){
    curTxMode = TxMode;
}