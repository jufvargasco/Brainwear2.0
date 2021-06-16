//
// Created by jpipe on 2/26/2021.
//

#ifndef SOFTWARE_BRAINWEAR_H
#define SOFTWARE_BRAINWEAR_H

#include <Arduino.h>
#include "Brainwear_definitions.h"
#include "SPI.h"

void IRAM_ATTR ADS_DRDY_Service(void); //Interrupt service for ESP32

class Brainwear {
public:
    Brainwear();

    //ENUMS
    /**How to send data*/
    typedef enum TX_MODE{
        DATA_RAW,
        DATA_ASCII
    };

    /**Commands with multiple characters*/
    typedef enum MULTI_CHAR_COMMAND {
        MULTI_CHAR_CMD_NONE,
        MULTI_CHAR_CMD_PROCESSING_INCOMING_SETTINGS_CHANNEL,
        MULTI_CHAR_CMD_PROCESSING_INCOMING_SETTINGS_LEADOFF,
        MULTI_CHAR_CMD_SETTINGS_SAMPLE_RATE
    };

    /**Sample rate to send data*/
    typedef enum SAMPLE_RATE {
        SAMPLE_RATE_16000,
        SAMPLE_RATE_8000,
        SAMPLE_RATE_4000,
        SAMPLE_RATE_2000,
        SAMPLE_RATE_1000,
        SAMPLE_RATE_500,
        SAMPLE_RATE_250
    };

    // SPI to communicate with the ADS1299
    SPIClass *SPI0 = NULL;

    //Functions
    boolean begin();
    void ADS_writeChannelData(void);
    void activateAllChannelsToTestCondition(byte, byte, byte);
    void activateChannel(byte);    // enable the selected channel
    void beginBoard(void);
    void beginSerial(uint32_t);
    void beginSPI(void);
    void boardReset(void);
    void changeChannelLeadOffDetect(void);
    void changeChannelLeadOffDetect(byte);
    boolean checkMultiCharCmdTimer(void);
    void configureInternalTestSignal(byte, byte);
    void configureLeadOffDetection(byte, byte);
    void deactivateChannel(byte);
    void endMultiCharCmdTimer(void);
    char getChannelCommandForAsciiChar(char);
    byte getDefaultChannelSettingForSetting(byte);
    char getDefaultChannelSettingForSettingAscii(byte);
    char getGainForAsciiChar(char);
    char getMultiCharCommand(void);
    char getNumberForAsciiChar(char);
    const char* getSampleRate(void);
    void loop(void);
    void normalInputSignal(void);
    void printRegisterName(byte);
    void printHex(byte);
    boolean processChar(char);
    void processIncomingChannelSettings(char);
    void processIncomingLeadOffSettings(char);
    void processIncomingSampleRate(char);
    void readRegisters(void);
    void reportDefaultChannelSettings(void);
    void resetADS(void);
    void sendChannelData(void);
    void sendChannelData(TX_MODE);
    void sendEOT(void);
    void setChannelsToDefault(void);
    void setCurTxMode(TX_MODE);
    void setSampleRate(uint8_t);
    void startADS(void);
    void startMultiCharCmdTimer(char);
    void stopADS(void);
    void streamSafeChannelActivate(byte);
    void streamSafeChannelDeactivate(byte);
    void streamSafeChannelSettingsForChannel(byte, byte, byte, byte, byte, byte, byte);
    void streamSafeLeadOffSetForChannel(byte, byte, byte);
    void streamSafeSetAllChannelsToDefault(void);
    void streamSafeSetSampleRate(SAMPLE_RATE);
    void streamStart(void);
    void streamStop(void);
    void testSignalDC(void);
    void testSignalFastPulse1(void);
    void testSignalFastPulse2(void);
    void testSignalGND(void);
    void testSignalSlowPulse1(void);
    void testSignalSlowPulse2(void);
    void turnLED(void);
    void turnOffLED(void);
    void turnOnLED(void);
    void updateChannelData(void);   // retrieve data from ADS
    void updateBoardData(boolean);
    void updateData(void);
    void writeChannelSettings(void);
    void writeChannelSettings(byte);

    //Variables
    boolean verbosity;
    boolean streaming;  //Activate or deactivate stream of data
    boolean serial_stream;
    boolean firstDataPacket;
    boolean boardUseSRB1;             // used to keep track of if we are using SRB1
    boolean useInBias[ADS_NUM_CHANNELS];        // used to remember if we were included in Bias before channel power down
    volatile boolean channelDataAvailable;

    byte sampleCounter;
    byte boardChannelDataRaw[ADS_BYTES_PER_ADS_SAMPLE];    // array to hold raw channel data
    byte meanBoardDataRaw[ADS_BYTES_PER_ADS_SAMPLE];       // mean raw

    byte boardData[27];

    int boardChannelDataInt[ADS_BYTES_PER_ADS_SAMPLE];    // array used when reading channel data as ints
    int lastBoardChannelDataInt[ADS_BYTES_PER_ADS_SAMPLE]; //Keep the last values of the data
    int meanBoardChannelDataInt[ADS_BYTES_PER_ADS_SAMPLE];

    //Settings
    byte leadOffSettings[ADS_NUM_CHANNELS][NUMBER_OF_LEAD_OFF_SETTINGS];  // used to control on/off of impedance measure for P and N side of each channel
    byte channelSettings[ADS_NUM_CHANNELS][NUMBER_OF_CHANNEL_SETTINGS];
    byte defaultChannelSettings[NUMBER_OF_CHANNEL_SETTINGS];  // default channel settings


    unsigned long lastSampleTime;

    //ENUMS instances
    SAMPLE_RATE curSampleRate;
    TX_MODE curTxMode;

private:
    // Functions
    void beginADSInterrupt(void);
    void changeInputType(byte);
    byte getDeviceID(void);
    void initialize(void);
    void initialize_ads(void);
    void WAKEUP(void);
    void STANDBY(void);
    void RESET(void);
    void sendChannelDataSerial_Raw(void);
    void sendChannelDataSerial_Ascii(void);
    void START(void);
    void STOP(void);
    void RDATAC(void);
    void SDATAC(void);
    byte RREG(byte);
    void RREG(byte, byte);
    void WREG(byte, byte);

    int  boardStat; // used to hold the status register

    //Variables
    char currentChannelSetting;
    boolean isRunning;
    boolean isMultiCharCmd;  // A multi char command is in progress
    char multiCharCommand;  // The type of command
    unsigned long multiCharCmdTimeout;  // the timeout in millis of the current multi char command
    int numberOfIncomingSettingsProcessedChannel;
    int numberOfIncomingSettingsProcessedLeadOff;
    char optionalArgBuffer7[7];
};

#endif //SOFTWARE_BRAINWEAR_H
