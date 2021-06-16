//
// Created by jpipe on 6/15/2021.
//

#ifndef SOFTWARE_MMG_H
#define SOFTWARE_MMG_H



#include <Wire.h>
#include "ADS1X15.h" // https://github.com/soligen2010/Adafruit_ADS1X15

#define MMG_CHANNELS 4

class MMG {
public:
    MMG(uint8_t);

    //ENUMS
    typedef enum TX_MODE{ //How to send data
        DATA_RAW,
        DATA_ASCII
    };

    void begin(adsGain_t , adsSPS_t);
    void sendMMGData(boolean);
    void setCurTxMode(TX_MODE);
    void updateMMGData(void);

    Adafruit_ADS1015 *MMG_ads;

    short MMGData[MMG_CHANNELS];

    // ENUM
    TX_MODE curTxMode;

private:
    void sendMMGDataSerial_Raw(void);
    void sendMMGDataSerial_Ascii(void);

};
#endif //SOFTWARE_MMG_H