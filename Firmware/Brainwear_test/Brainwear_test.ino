/* Arduino file for using the Brainwear board to acquire EEG signals with the ADS1299 from TI
 Signals from FSR and piezoelectric sensors are measured with the ADS1015 from TI
 This code is based in the OpenBCI Firmware v3.1.2
 Created by Conor Russomanno, Luke Travis, and Joel Murphy. Summer 2013.
*/

// Libraries for SD card
#include <mySD.h> //https://github.com/nhatuan84/esp32-micro-sdcard
#include "SPI.h"
#include <EEPROM.h>

// This library contains the firmware to interface the Brainwear board
#include "Brainwear.h"
// THis library contains firmware to interface the ADS1015 as Mechanomyography sensors
#include "MMG.h"

boolean multimode = true;

boolean SDfileOpen = false; // Set true by SD_Card.ino on successful file open
boolean addAuxtoSD = false; // Add AUX data when writing on the SD card

Brainwear  EEG;             // Declare an instance of the Brainwear module
MMG        MMG1(ADS1x15_1); // Declare an instance of the MMG module, used for FSR
MMG        MMG2(ADS1x15_2); // Declare an instance of the MMG module, used for Piezos

// ENUMS
typedef enum TX_MODE{ //How to send data
    DATA_RAW,   // Compatible with OpenBCI data visualization
    DATA_ASCII  // Compatible with Arduino serial plotter
};

TX_MODE curTxMode;

void setup() {
    curTxMode = DATA_RAW;   // Start sending in a mode compatible with OpenBCI
    delay(50);              //Gives time to the system to initalize
    EEPROM.begin(2);        // Start the EEPROM to keep track of the files written in the SD card
    EEG.begin();            // Start the Brainwear board
    MMG1.begin(GAIN_TWO,ADS1015_DR_3300SPS);        // FSR 2x gain   +/- 2.048V  1 bit = 1mV
    MMG2.begin(GAIN_SIXTEEN,ADS1015_DR_3300SPS);    // Piezo 16x gain  +/- 0.256V  1 bit = 0.125mV
    setCurTxMode(curTxMode);
}

void loop(){
    if (EEG.streaming) {
        if(EEG.channelDataAvailable)
        {
            // Read from the Brainwear, store data, set channelDataAvailable flag to false
            EEG.updateChannelData();

            // If multimode is active, update data from MMG sensors
            if(multimode) {
                MMG1.updateMMGData();
                MMG2.updateMMGData();
                addAuxtoSD = true;
            }

            // If SD was activated, store data in SD card
            if(SDfileOpen){
                writeDataToSDcard(EEG.sampleCounter);
            }

            // Send data to the serial port
            sendData();
        }
    }

    // Check the serial ports for new data
    if (hasDataSerial()){
        char newChar = getCharSerial();

        // Send command to the board
        boardProcessChar(newChar);

        // Send command to the SD library
        sdProcessChar(newChar);

        // Send command to the Brainwear library
        EEG.processChar(newChar);
    }
    EEG.loop();

}

//////////////////////////////////////////////
///////////Data manipulation functions////////
//////////////////////////////////////////////

/**
 * @description: Changes the transmission mode in the Brainwear and MMG boards
 */
void setCurTxMode(TX_MODE TxMode){
    if (curTxMode == DATA_RAW){
        EEG.setCurTxMode(EEG.DATA_RAW);
        MMG1.setCurTxMode(MMG1.DATA_RAW);
        MMG2.setCurTxMode(MMG2.DATA_RAW);
    }
    if (curTxMode == DATA_ASCII){
        EEG.setCurTxMode(EEG.DATA_ASCII);
        MMG1.setCurTxMode(MMG1.DATA_ASCII);
        MMG2.setCurTxMode(MMG2.DATA_ASCII);
    }
}

/**
 * @description: Sends data to serial port
 */
void sendData(void){
    if (curTxMode == DATA_RAW){
        Serial.write(ADS_BOP); // 1 byte
        Serial.write(EEG.sampleCounter); // 1 byte
        EEG.sendChannelData(); //compatible with OpenBCI data visualize (24 bytes or less (12 for 4 channels)
        if(multimode) {
            MMG1.sendMMGData(EEG.serial_stream); // (8 bytes)
            MMG2.sendMMGData(EEG.serial_stream); // (8 bytes)
        }
        Serial.write((uint8_t)(ADS_EOP)); //(1 byte)
    }
    if (curTxMode == DATA_ASCII){
        EEG.sendChannelData(); //compatible with Arduino serial plotter
        if(multimode){
            MMG1.sendMMGData(EEG.serial_stream);
            MMG2.sendMMGData(EEG.serial_stream);
        }
        Serial.println();
    }
}

/**
 * @description: Process the command sent via serial port
 */
char boardProcessChar(char character) {

    switch (character) {
        case ADS_TX_RAW:
            curTxMode = DATA_RAW;
            setCurTxMode(curTxMode);
            Serial.println("Transmission mode changed to Data_raw");
            break;
        case ADS_TX_ASCII:
            curTxMode = DATA_ASCII;
            setCurTxMode(curTxMode);
            Serial.println("Transmission mode changed to Data_ascii");
            break;
        case ADS_MULTMODE_ON:
            multimode = true;
            Serial.println("Multimode activated");
            break;
        case ADS_MULTMODE_OFF:
            multimode = false;
            Serial.println("Multimode deactivated");
            break;
        default:
            break;
    }
    return character;
}

//////////////////////////////////////////////
/////////// Serial port functions/////////////
//////////////////////////////////////////////

boolean hasDataSerial(void)
{
    if (Serial.available())
    {
        return true;
    }
    else
    {
        return false;
    }
}

/**
* @description Called if `hasDataSerial` is true, returns a char from `Serial`
* @returns {char} - A char from the serial port
*/
char getCharSerial(void)
{
    return Serial.read();
}

//////////////////////////////////////////////
/////////// Interrupt functions///////////////
//////////////////////////////////////////////
void IRAM_ATTR ADS_DRDY_Service()
{
    EEG.channelDataAvailable = true;
}