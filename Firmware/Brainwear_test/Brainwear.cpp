/**
 Created by jpipe on 2/26/2021.
 This code is based in the OpenBCI Firmware v3.1.2
 Created by Conor Russomanno, Luke Travis, and Joel Murphy. Summer 2013.
**/

#include "Brainwear.h"
#include <SPI.h>


//Constructor
Brainwear::Brainwear(){
    //SPI BUS for ADS1299
    SPI0 = new SPIClass(VSPI);

    //Bools
    channelDataAvailable = false;
    verbosity = false;
    endMultiCharCmdTimer(); // this initializes and resets the variables
    streaming = false;
    serial_stream = true;

    //Nums
    currentChannelSetting = 0;
    lastSampleTime = 0;
    numberOfIncomingSettingsProcessedChannel = 0;
    numberOfIncomingSettingsProcessedLeadOff = 0;
    sampleCounter = 0;

    //enums
    curSampleRate = SAMPLE_RATE_250;
    curTxMode = DATA_RAW;
};

//////////////////////////////////////////////
/////////// Start up functions////////////////
//////////////////////////////////////////////

/**
 * @description: This function sets the board up
*/
boolean Brainwear::begin()
{
    beginBoard();
    beginSerial(BAUD_RATE);
    beginSPI();
    delay(10);

    //Soft reset
    boardReset();

    return true;
}

/**
 * @description: Attach the interrupt to the falling edge of DRDY signal
*/
void Brainwear::beginADSInterrupt(void)
{
    // Startup for interrupt
    if(verbosity)
    {
        Serial.println("Interrupt setup");
    }
    attachInterrupt(digitalPinToInterrupt(DRDY), ADS_DRDY_Service, FALLING);
}

/**
 * @description: This function tuns on the built-in LED on the Feather board
*/
void Brainwear::beginBoard(void)
{
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
}

/**
 * @description: This function starts the serial port at the determined baud rate
*/
void Brainwear::beginSerial(uint32_t baudRate)
{
    Serial.begin(baudRate);
    if(verbosity)
    {
        Serial.println("Serial connection started ...");
    }
}

/**
 * @description: This function starts the SPI communication with the board
*/
void Brainwear::beginSPI(void) {
// start the SPI library:
    SPI0->begin();
    SPI0->beginTransaction(SPISettings(1500000, MSBFIRST, SPI_MODE1));

    if (verbosity) {
        Serial.println("SPI connection started ...");
    }
}

/**
 * @description: Soft reset of the board
*/
void Brainwear::boardReset(void)
{
    initialize(); //Initializes ADS board
    delay(500);
    configureLeadOffDetection(LOFF_MAG_6NA, LOFF_FREQ_31p2HZ);

    Serial.println("BrainWear board");
    Serial.print("On Board ADS1299 Device ID: ");
    printHex(getDeviceID());
    Serial.println();
    Serial.println("Firmware: v1.0");

    if (verbosity)
    {
        Serial.println("On Board ADS1299 Device ");
        readRegisters();
    }
    sendEOT();
    delay(5);
    beginADSInterrupt();
}

/**
 * @description: Initialization of the board pins for SPI communication
*/

void Brainwear::initialize(void)
{
    // initalize the  data ready and chip select pins:
    pinMode(DRDY, INPUT);
    pinMode(CS, OUTPUT);
    pinMode(START_PIN, OUTPUT);

    //start the ADS1299
    initialize_ads();

}

/**
 * @description: Full initialization of the ADS1299 including power up sequence, setting channels to default
 * , and internal registers
*/
void Brainwear::initialize_ads(void)
{
    delay(50); // recommended power up sequence requiers >Tpor (~32mS)
    resetADS();
    delay(10);
    WREG(CONFIG1,(DEFAULT_CONFIG1 | curSampleRate)); // tell on-board ADS to output its clk, set the data rate to 250SPS
    delay(40);

    // DEFAULT CHANNEL SETTINGS FOR ADS
    defaultChannelSettings[POWER_DOWN] = NO;                  // on = NO, off = YES
    defaultChannelSettings[GAIN_SET] = ADS_GAIN24;            // Gain setting
    defaultChannelSettings[INPUT_TYPE_SET] = ADSINPUT_NORMAL; // input muxer setting
    defaultChannelSettings[BIAS_SET] = YES;                   // add this channel to bias generation
    defaultChannelSettings[SRB2_SET] = NO;                   // Don't connect this P side to SRB2
    defaultChannelSettings[SRB1_SET] = YES;                    // Connect N side to SRB1

    for (int i = 0; i < ADS_NUM_CHANNELS; i++)
    {
        for (int j = 0; j < NUMBER_OF_CHANNEL_SETTINGS; j++)
        {
            channelSettings[i][j] = defaultChannelSettings[j]; // assign default settings
        }
        useInBias[i] = true; // keeping track of Bias Generation
    }
    boardUseSRB1 = true;

    writeChannelSettings(); // write settings to the ADS

    WREG(CONFIG3, 0b11101100); // pg.48, Enable internal reference buff, internal bias ref signal, bias buffer enabled
    delay(1);

    for (int i = 0; i < ADS_NUM_CHANNELS; i++)
    { // turn off the impedance measure signal
        leadOffSettings[i][PCHAN] = OFF;
        leadOffSettings[i][NCHAN] = OFF;
    }
    changeChannelLeadOffDetect(); // write settings to all ADS

    firstDataPacket = true;
    streaming = false;
}

/**
 * @description: Reset sequence of the ADS1299
*/
void Brainwear::resetADS(void)
{
    int startChan, stopChan;
    startChan = 1;
    stopChan = 8;
    RESET(); // send RESET command to default all registers
    SDATAC(); // exit Read Data Continuous mode to communicate with ADS
    turnLED(); //Debug for GPIO
    delay(100);
    for (int chan = startChan; chan <= stopChan; chan++)
    {
        deactivateChannel(chan);
    }
}

/**
 * @description: Starting routine for the ADS1299
*/
void Brainwear::startADS(void)
{
    sampleCounter = 0;
    firstDataPacket = true;
    digitalWrite(START_PIN, HIGH); //High to start conversion
    delay(10);
    START(); // start the data acquisition
    delay(1);
    RDATAC(); // enter Read Data Continuous mode
    delay(1);
    isRunning = true;
}

/**
 * @description: Stopping routine for the ADS1299
*/
void Brainwear::stopADS(void)
{
    STOP(); // stop the data acquisition
    delay(1);
    SDATAC(); // stop Read Data Continuous mode to communicate with ADS
    delay(1);
    isRunning = false;
}

/**
 * @description: Read all the configuration registers of the ADS1299 and send information via serial port
*/
void Brainwear::readRegisters(void)
{
    boolean tempVerbosity = verbosity;
    verbosity = true;
    Serial.println("----------------------------------------------");
    Serial.println("-----------------Registers--------------------");
    Serial.println("----------------------------------------------");
    RREG(0x00, 0x17);
    Serial.println("----------------------------------------------");
    sendEOT();
    verbosity = tempVerbosity;
}

//////////////////////////////////////////////
/////////// Channel functions////////////////
//////////////////////////////////////////////

/**
 * @description: Deactivate a determined channel N
*/
void Brainwear::deactivateChannel(byte N)
{
    byte startChan, endChan, setting;
    startChan = 0;
    endChan = 8;
    SDATAC();
    delay(1); // exit Read Data Continuous mode to communicate with ADS
    N = constrain(N - 1, startChan, endChan - 1); //subtracts 1 so that we're counting from 0, not 1

    setting = RREG(CH1SET + (N - startChan)); // get the current channel settings
    delay(1);
    bitSet(setting,7); // set bit7 to shut down channel (pg. 50)
    bitClear(setting,3); // clear bit3 to disclude from SRB2 if used
    bitSet(setting,0);   //1
    bitClear(setting,1); //0
    bitClear(setting,2); //0

    // Write the new settings to the channel
    WREG(CH1SET + (N - startChan), setting);
    delay(1);  // write the new value to disable the channel

    //remove the channel from the bias generation...
    setting = RREG(BIAS_SENSP);
    delay(1);                         //get the current bias settings
    bitClear(setting, N - startChan); //clear this channel's bit to remove from bias generation
    WREG(BIAS_SENSP, setting);
    delay(1); //send the modified byte back to the ADS

    setting = RREG(BIAS_SENSN);
    delay(1);                         //get the current bias settings
    bitClear(setting, N - startChan); //clear this channel's bit to remove from bias generation
    WREG(BIAS_SENSN, setting);
    delay(1); //send the modified byte back to the ADS

    leadOffSettings[N][0] = leadOffSettings[N][1] = NO;
    changeChannelLeadOffDetect(N + 1);

}

/**
 * @description: Activate a determined channel N
*/
void Brainwear::activateChannel(byte N)
{
    byte setting, startChan, endChan;
    startChan = 0;
    endChan = 8;

    N = constrain(N - 1, startChan, endChan - 1); // 0-7

    SDATAC(); // exit Read Data Continuous mode to communicate with ADS
    setting = 0x00;
    setting |= channelSettings[N][GAIN_SET];       // gain
    setting |= channelSettings[N][INPUT_TYPE_SET]; // input code
    if (channelSettings[N][SRB2_SET] == YES)
    {
        bitSet(setting, 3);
    } // close this SRB2 switch
    WREG(CH1SET + (N - startChan), setting);
    // add or remove from inclusion in BIAS generation
    if (useInBias[N])
    {
        channelSettings[N][BIAS_SET] = YES;
    }
    else
    {
        channelSettings[N][BIAS_SET] = NO;
    }
    setting = RREG(BIAS_SENSP); //get the current P bias settings

    if (channelSettings[N][BIAS_SET] == YES)
    {
        bitSet(setting, N - startChan); //set this channel's bit to add it to the bias generation
        useInBias[N] = true;
    }
    else
    {
        bitClear(setting, N - startChan); // clear this channel's bit to remove from bias generation
        useInBias[N] = false;
    }
    WREG(BIAS_SENSP, setting);
    delay(1);                             //send the modified byte back to the ADS

    setting = RREG(BIAS_SENSN); //get the current N bias settings
    if (channelSettings[N][BIAS_SET] == YES)
    {
        bitSet(setting, N - startChan); //set this channel's bit to add it to the bias generation
    }
    else
    {
        bitClear(setting, N - startChan); // clear this channel's bit to remove from bias generation
    }
    WREG(BIAS_SENSN, setting);
    delay(1); //send the modified byte back to the ADS

    setting = 0x00;
    if (boardUseSRB1 == true)
        setting = 0x20;
    WREG(MISC1, setting); // close all SRB1 swtiches
}

/**
 * @description: Configure the Leadoff detection
*/
void Brainwear::configureLeadOffDetection(byte amplitudeCode, byte freqCode)
{
    amplitudeCode &= 0b00001100; //only these two bits should be used
    freqCode &= 0b00000011;      //only these two bits should be used

    byte setting, targetSS;
    setting = RREG(LOFF); //get the current bias settings
    //reconfigure the byte to get what we want
    setting &= 0b11110000;    //clear out the last four bits
    setting |= amplitudeCode; //set the amplitude
    setting |= freqCode;      //set the frequency
    //send the config byte back to the hardware
    WREG(LOFF, setting);
    delay(1); //send the modified byte back to the ADS
}

/**
 * @description: change the lead off detect settings for all channels
*/
void Brainwear::changeChannelLeadOffDetect(void)
{
    byte startChan, endChan, setting;
    startChan = 0;
    endChan = ADS_NUM_CHANNELS;

    SDATAC();
    delay(1); // exit Read Data Continuous mode to communicate with ADS

    byte P_setting = RREG(LOFF_SENSP);
    byte N_setting = RREG(LOFF_SENSN);

    for (int i = startChan; i < endChan; i++)
    {
        if (leadOffSettings[i][PCHAN] == ON)
        {
            bitSet(P_setting, i - startChan);
        }
        else
        {
            bitClear(P_setting, i - startChan);
        }
        if (leadOffSettings[i][NCHAN] == ON)
        {
            bitSet(N_setting, i - startChan);
        }
        else
        {
            bitClear(N_setting, i - startChan);
        }
        WREG(LOFF_SENSP, P_setting);
        WREG(LOFF_SENSN, N_setting);
    }

}

/**
 * @description: change the lead off detect settings for specified channel
*/
void Brainwear::changeChannelLeadOffDetect(byte N)
{
    byte startChan, endChan, setting;
    startChan = 0;
    endChan = ADS_NUM_CHANNELS;

    N = constrain(N - 1, startChan, endChan - 1);
    SDATAC();
    delay(1); // exit Read Data Continuous mode to communicate with ADS

    byte P_setting = RREG(LOFF_SENSP);
    byte N_setting = RREG(LOFF_SENSN);

    if (leadOffSettings[N][PCHAN] == ON)
    {
        bitSet(P_setting, N - startChan);
    }
    else
    {
        bitClear(P_setting, N - startChan);
    }
    if (leadOffSettings[N][NCHAN] == ON)
    {
        bitSet(N_setting, N - startChan);
    }
    else
    {
        bitClear(N_setting, N - startChan);
    }
    WREG(LOFF_SENSP, P_setting);
    WREG(LOFF_SENSN, N_setting);
}


/**
 * @description: write settings for ALL 8 channels for the Brainwear board
 * channel settings: powerDown, gain, inputType, SRB2, SRB1
*/
void Brainwear::writeChannelSettings()
{
    boolean use_SRB1 = false;
    byte startChan, endChan, setting;
    startChan = 0;
    endChan = ADS_NUM_CHANNELS;

    SDATAC();
    delay(1); // exit Read Data Continuous mode to communicate with ADS

    for (byte i = startChan; i < endChan; i++)
    { // write 8 channel settings
        setting = 0x00;
        if (channelSettings[i][POWER_DOWN] == YES)
        {
            setting |= 0x80;
        }
        setting |= channelSettings[i][GAIN_SET];       // gain
        setting |= channelSettings[i][INPUT_TYPE_SET]; // input code
        if (channelSettings[i][SRB2_SET] == YES)
        {
            setting |= 0x08;   // close this SRB2 switch
        }
        WREG(CH1SET + (i - startChan), setting); // write this channel's register settings

        // add or remove this channel from inclusion in BIAS generation
        setting = RREG(BIAS_SENSP); //get the current P bias settings
        if (channelSettings[i][BIAS_SET] == YES)
        {
            bitSet(setting, i - startChan);
            useInBias[i] = true; //add this channel to the bias generation
        }
        else
        {
            bitClear(setting, i - startChan);
            useInBias[i] = false; //remove this channel from bias generation
        }
        WREG(BIAS_SENSP, setting);
        delay(1); //send the modified byte back to the ADS

        setting = RREG(BIAS_SENSN); //get the current N bias settings
        if (channelSettings[i][BIAS_SET] == YES)
        {
            bitSet(setting, i - startChan); //set this channel's bit to add it to the bias generation
        }
        else
        {
            bitClear(setting, i - startChan); // clear this channel's bit to remove from bias generation
        }
        WREG(BIAS_SENSN, setting);
        delay(1); //send the modified byte back to the ADS

        if (channelSettings[i][SRB1_SET] == YES)
        {
            use_SRB1 = true; // if any of the channel setting closes SRB1, it is closed for all
        }
    } // end of CHnSET and BIAS settings
    // Configuration for referential montage
    if (use_SRB1)
    {
        for (int i = startChan; i < endChan; i++)
        {
            channelSettings[i][SRB1_SET] = YES;
        }
        WREG(MISC1, 0x20); // close SRB1 switch
        boardUseSRB1 = true;
    }
    else
    {
        for (int i = startChan; i < endChan; i++)
        {
            channelSettings[i][SRB1_SET] = NO;
        }
        WREG(MISC1, 0x00); // open SRB1 switch
        boardUseSRB1 = false;
    }
}

/**
 * @description: write settings for a SPECIFIC channel N
*/
void Brainwear::writeChannelSettings(byte N)
{
    byte startChan, endChan, setting;
    startChan = 0;
    endChan = 8;

    N = constrain(N - 1, startChan, endChan - 1); //subtracts 1 so that we're counting from 0, not 1
    SDATAC();
    delay(1); // exit Read Data Continuous mode to communicate with ADS

    // write corresponding channel settings
    setting = 0x00;
    if (channelSettings[N][POWER_DOWN] == YES)
    {
        setting |= 0x80;
    }
    setting |= channelSettings[N][GAIN_SET];       // gain
    setting |= channelSettings[N][INPUT_TYPE_SET]; // input code
    if (channelSettings[N][SRB2_SET] == YES)
    {
        setting |= 0x08;   // close this SRB2 switch
    }
    WREG(CH1SET + (N - startChan), setting); // write this channel's register settings

    // add or remove this channel from inclusion in BIAS generation
    setting = RREG(BIAS_SENSP); //get the current P bias settings
    if (channelSettings[N][BIAS_SET] == YES)
    {
        bitSet(setting, N - startChan);
        useInBias[N] = true; //add this channel to the bias generation
    }
    else
    {
        bitClear(setting, N - startChan);
        useInBias[N] = false; //remove this channel from bias generation
    }
    WREG(BIAS_SENSP, setting);
    delay(1); //send the modified byte back to the ADS

    setting = RREG(BIAS_SENSN); //get the current N bias settings
    if (channelSettings[N][BIAS_SET] == YES)
    {
        bitSet(setting, N - startChan); //set this channel's bit to add it to the bias generation
    }
    else
    {
        bitClear(setting, N - startChan); // clear this channel's bit to remove from bias generation
    }
    WREG(BIAS_SENSN, setting);
    delay(1); //send the modified byte back to the ADS
}

/**
 * @description: Sets all channels to the default configuration
*/
void Brainwear::setChannelsToDefault(void)
{
    for (int i = 0; i < ADS_NUM_CHANNELS; i++)
    {
        for (int j = 0; j < 6; j++)
        {
            channelSettings[i][j] = defaultChannelSettings[j];
        }
        useInBias[i] = true; // keeping track of Bias Generation
    }
    boardUseSRB1 = true;

    writeChannelSettings(); // write settings to on-board ADS

    for (int i = 0; i < ADS_NUM_CHANNELS; i++)
    { // turn off the impedance measure signal
        leadOffSettings[i][PCHAN] = OFF;
        leadOffSettings[i][NCHAN] = OFF;
    }
    changeChannelLeadOffDetect(); // write settings to all ADS

    WREG(MISC1, 0x20); // close SRB1 switch on-board
}

/**
* @description Writes the default channel settings over the serial port
*/
void Brainwear::reportDefaultChannelSettings(void)
{
    char buf[7];
    buf[0] = getDefaultChannelSettingForSettingAscii(POWER_DOWN);     // on = NO, off = YES
    buf[1] = getDefaultChannelSettingForSettingAscii(GAIN_SET);       // Gain setting
    buf[2] = getDefaultChannelSettingForSettingAscii(INPUT_TYPE_SET); // input muxer setting
    buf[3] = getDefaultChannelSettingForSettingAscii(BIAS_SET);       // add this channel to bias generation
    buf[4] = getDefaultChannelSettingForSettingAscii(SRB2_SET);       // connect this P side to SRB2
    buf[5] = getDefaultChannelSettingForSettingAscii(SRB1_SET);       // don't use SRB1
    Serial.print((const char *)buf);
    sendEOT();
}

/**
* @description Used to set the channelSettings array to default settings
* @param `setting` - [byte] - The byte you need a setting for....
* @returns - [char] - Retuns the proper ascii char for the input setting, defaults to '0'
*/
char Brainwear::getDefaultChannelSettingForSettingAscii(byte setting)
{
    return getDefaultChannelSettingForSetting(setting) + '0';
}

/**
* @description Used to set the channelSettings array to default settings
* @param `setting` - [byte] - The byte you need a setting for....
* @returns - [byte] - Retuns the proper byte for the input setting, defualts to 0
*/
byte Brainwear::getDefaultChannelSettingForSetting(byte setting)
{
    switch (setting)
    {
        case POWER_DOWN:
            return defaultChannelSettings[POWER_DOWN];
        case GAIN_SET:
            return defaultChannelSettings[GAIN_SET];
        case INPUT_TYPE_SET:
            return defaultChannelSettings[INPUT_TYPE_SET];
        case BIAS_SET:
            return defaultChannelSettings[BIAS_SET];
        case SRB2_SET:
            return defaultChannelSettings[SRB2_SET];
        case SRB1_SET:
            return defaultChannelSettings[SRB1_SET];
        default:
            return NO;
    }
}

/**
 * @description: CHanges the type of input for all the channels
*/
void Brainwear::changeInputType(byte inputCode)
{
    for (int i = 0; i < ADS_NUM_CHANNELS; i++)
    {
        channelSettings[i][INPUT_TYPE_SET] = inputCode;
    }
    writeChannelSettings();
}


//////////////////////////////////////////////
/////////// Data transmission/////////////////
//////////////////////////////////////////////
/**
 * @description: Start streaming data with the ADS1299
*/
void Brainwear::streamStart(void)
{
    streaming = true;
    startADS();
    if (verbosity)
    {
        Serial.println("ADS1299 Started");
    }
}

/**
 * @description: Stop streaming data with the ADS1299
*/
void Brainwear::streamStop(void)
{
    streaming = false;
    stopADS();
    if (verbosity)
    {
        Serial.println("ADS1299 Stopped");
    }
}

/**
 * @description: Update the data when DRDY PIN IS ASSERTED. NEW ADS DATA AVAILABLE!
*/
void Brainwear::updateChannelData(void)
{
    // this needs to be reset, or else it will constantly flag us
    channelDataAvailable = false;

    lastSampleTime = millis();
    boolean downsample = true;

    updateBoardData(downsample);
}

/**
 * @description: Debugging function for data acquired in the channels of the ADS1299
 */
void Brainwear::updateData(void){
    // this needs to be reset, or else it will constantly flag us
    channelDataAvailable = false;

    lastSampleTime = millis();
    boolean downsample = true;

    byte inByte;
    digitalWrite(CS, LOW); //  open SPI
    for (int i = 0; i < 15; i++)
    {
        inByte = SPI0->transfer(0x00); //  read status register (1100 + LOFF_STATP + LOFF_STATN + GPIO[7:4])
        boardData[i] = inByte;
    }
    digitalWrite(CS, HIGH); // close SPI

    for (int i = 0; i < 15; i++) {
        Serial.print(boardData[i], HEX);
        Serial.print(" ");
    }
    Serial.println();

}
/**
 * @description: Read the data sent from the Brainwear board and stores it
 */
void Brainwear::updateBoardData(boolean downsample)
{
    byte inByte;
    int byteCounter = 0;

    if (!firstDataPacket && downsample)
    {
        for (int i = 0; i < ADS_NUM_CHANNELS; i++)
        {                                                      // shift and average the byte arrays
            lastBoardChannelDataInt[i] = boardChannelDataInt[i]; // remember the last samples
        }
    }

    digitalWrite(CS, LOW); //  open SPI
    for (int i = 0; i < 3; i++)
    {
        inByte = SPI0->transfer(0x00); //  read status register (1100 + LOFF_STATP + LOFF_STATN + GPIO[7:4])
        boardStat = (boardStat << 8) | inByte;
    }

    for (int i = 0; i < ADS_CHANNELS_BOARD; i++)
    {
        for (int j = 0; j < ADS_BYTES_PER_CHAN; j++)
        { //  read 24 bits of channel data in 8 3 byte chunks
            inByte = SPI0->transfer(0x00);
            boardChannelDataRaw[byteCounter] = inByte; // raw data goes here
            byteCounter++;
            boardChannelDataInt[i] = (boardChannelDataInt[i] << 8) | inByte; // int data goes here
        }
    }
    digitalWrite(CS, HIGH); // close SPI
    // need to convert 24bit to 32bit
    for (int i = 0; i < ADS_CHANNELS_BOARD; i++)
    { // convert 3 byte 2's compliment to 4 byte 2's compliment
        if (bitRead(boardChannelDataInt[i], 23) == 1)
        {
            boardChannelDataInt[i] |= 0xFF000000;
        }
        else
        {
            boardChannelDataInt[i] &= 0x00FFFFFF;
        }
    }
    if (!firstDataPacket && downsample)
    {
        byteCounter = 0;
        for (int i = 0; i < ADS_CHANNELS_BOARD; i++)
        { // take the average of this and the last sample
            meanBoardChannelDataInt[i] = (lastBoardChannelDataInt[i] + boardChannelDataInt[i]) / 2;
        }
        for (int i = 0; i < ADS_CHANNELS_BOARD; i++)
        { // place the average values in the meanRaw array
            for (int b = 2; b >= 0; b--)
            {
                meanBoardDataRaw[byteCounter] = (meanBoardChannelDataInt[i] >> (b * 8)) & 0xFF;
                byteCounter++;
            }
        }
    }
    if (firstDataPacket == true)
    {
        firstDataPacket = false;
    }
}

/**
* @description: Simple method to send the EOT over serial...
* @author: AJ Keller (@pushtheworldllc)
*/
void Brainwear::sendEOT(void)
{
    Serial.print("$$$");
}

//////////////////////////////////////////////
/////////////// Test Signals /////////////////
//////////////////////////////////////////////
/**
 * @description: Set all the channels to the specific test condition
 */
void Brainwear::activateAllChannelsToTestCondition(byte testInputCode, byte amplitudeCode, byte freqCode)
{
    boolean wasStreaming = streaming;

    // Stop streaming if you are currently streaming
    if (streaming)
    {
        streamStop();
    }

    //set the test signal to the desired state
    configureInternalTestSignal(amplitudeCode, freqCode);
    //change input type settings for all channels
    changeInputType(testInputCode);

    // Restart stream if need be
    if (wasStreaming)
    {
        streamStart();
    }
    else
    {
        Serial.println("Configured internal");
        sendEOT();
    }
}

/**
 * @description: Configure the test signals that can be internally generated by the ADS1299
 */
void Brainwear::configureInternalTestSignal(byte amplitudeCode, byte freqCode)
{
    byte setting;

    if (amplitudeCode == ADSTESTSIG_NOCHANGE)
        amplitudeCode = (RREG(CONFIG2) & (0b00000100));
    if (freqCode == ADSTESTSIG_NOCHANGE)
        freqCode = (RREG(CONFIG2) & (0b00000011));
    freqCode &= 0b00000011;                               //only the last two bits are used
    amplitudeCode &= 0b00000100;                          //only this bit is used
    setting = 0b11010000 | freqCode | amplitudeCode; //compose the code. INT_CAL = 1 (Test signals generated internally)
    WREG(CONFIG2, setting);
    delay(1);
}

// Configurations observed in CONFIG2 register
/**
 * @description: testSignal to GND
 */
void Brainwear::testSignalGND(void)
{
    activateAllChannelsToTestCondition(ADSINPUT_SHORTED, ADSTESTSIG_NOCHANGE, ADSTESTSIG_NOCHANGE);
}

/**
 * @description: testSignal to slow pulse of 1 * (VREFP - VREFN) / 2.4 mV
 */
void Brainwear::testSignalSlowPulse1(void)
{
    activateAllChannelsToTestCondition(ADSINPUT_TESTSIG, ADSTESTSIG_AMP_1X, ADSTESTSIG_PULSE_SLOW);
}

/**
 * @description: testSignal to slow pulse of 2 * (VREFP - VREFN) / 2.4 mV
 */
void Brainwear::testSignalSlowPulse2(void)
{
    activateAllChannelsToTestCondition(ADSINPUT_TESTSIG, ADSTESTSIG_AMP_2X, ADSTESTSIG_PULSE_SLOW);
}

/**
 * @description: testSignal to fast pulse of 1 * (VREFP - VREFN) / 2.4 mV
 */
void Brainwear::testSignalFastPulse1(void)
{
    activateAllChannelsToTestCondition(ADSINPUT_TESTSIG, ADSTESTSIG_AMP_1X, ADSTESTSIG_PULSE_FAST);
}

/**
 * @description: testSignal to fast pulse of 2 * (VREFP - VREFN) / 2.4 mV
 */
void Brainwear::testSignalFastPulse2(void)
{
    activateAllChannelsToTestCondition(ADSINPUT_TESTSIG, ADSTESTSIG_AMP_2X, ADSTESTSIG_PULSE_FAST);
}

/**
 * @description: testSignal to DC value
 */
void Brainwear::testSignalDC(void)
{
    activateAllChannelsToTestCondition(ADSINPUT_TESTSIG, ADSTESTSIG_AMP_2X, ADSTESTSIG_DCSIG);
}

/**
 * @description: testSignal to normal data acquisition
 */
void Brainwear::normalInputSignal(void)
{
    activateAllChannelsToTestCondition(ADSINPUT_NORMAL, ADSTESTSIG_NOCHANGE, ADSTESTSIG_NOCHANGE);
}

//////////////////////////////////////////////
///////Serial communication functions/////////
//////////////////////////////////////////////

/**
* @description: Check multicommand instructions
 * To be called at some point in every loop function
*/
void Brainwear::loop(void)
{
    if (isMultiCharCmd)
    {
        checkMultiCharCmdTimer();
    }
}

/**
* @description Process one char at a time from serial port. This is the main
*  command processor for the system. Considered mission critical for
*  normal operation.
* @param `character` {char} - The character to process.
* @return {boolean} - `true` if the command was recognized, `false` if not
*/

boolean Brainwear::processChar(char character)
{
    if (checkMultiCharCmdTimer())
    { // we are in a multi char command
        switch (getMultiCharCommand())
        {
            case MULTI_CHAR_CMD_PROCESSING_INCOMING_SETTINGS_CHANNEL:
                processIncomingChannelSettings(character);
                break;
            case MULTI_CHAR_CMD_PROCESSING_INCOMING_SETTINGS_LEADOFF:
                processIncomingLeadOffSettings(character);
                break;
            case MULTI_CHAR_CMD_SETTINGS_SAMPLE_RATE:
                processIncomingSampleRate(character);
                break;
            default:
                break;
        }
    }
    else
    { // Normal...
        switch (character)
        {
            //TURN CHANNELS ON/OFF COMMANDS
            case ADS_CHANNEL_OFF_1:
                streamSafeChannelDeactivate(1);
                break;
            case ADS_CHANNEL_OFF_2:
                streamSafeChannelDeactivate(2);
                break;
            case ADS_CHANNEL_OFF_3:
                streamSafeChannelDeactivate(3);
                break;
            case ADS_CHANNEL_OFF_4:
                streamSafeChannelDeactivate(4);
                break;
            case ADS_CHANNEL_OFF_5:
                streamSafeChannelDeactivate(5);
                break;
            case ADS_CHANNEL_OFF_6:
                streamSafeChannelDeactivate(6);
                break;
            case ADS_CHANNEL_OFF_7:
                streamSafeChannelDeactivate(7);
                break;
            case ADS_CHANNEL_OFF_8:
                streamSafeChannelDeactivate(8);
                break;

            case ADS_CHANNEL_ON_1:
                streamSafeChannelActivate(1);
                break;
            case ADS_CHANNEL_ON_2:
                streamSafeChannelActivate(2);
                break;
            case ADS_CHANNEL_ON_3:
                streamSafeChannelActivate(3);
                break;
            case ADS_CHANNEL_ON_4:
                streamSafeChannelActivate(4);
                break;
            case ADS_CHANNEL_ON_5:
                streamSafeChannelActivate(5);
                break;
            case ADS_CHANNEL_ON_6:
                streamSafeChannelActivate(6);
                break;
            case ADS_CHANNEL_ON_7:
                streamSafeChannelActivate(7);
                break;
            case ADS_CHANNEL_ON_8:
                streamSafeChannelActivate(8);
                break;

                // TEST SIGNAL CONTROL COMMANDS
            case ADS_TEST_SIGNAL_CONNECT_TO_GROUND:
                testSignalGND();
                break;
            case ADS_TEST_SIGNAL_CONNECT_TO_PULSE_1X_SLOW:
                testSignalSlowPulse1();
                break;
            case ADS_TEST_SIGNAL_CONNECT_TO_PULSE_1X_FAST:
                testSignalFastPulse1();
                break;
            case ADS_TEST_SIGNAL_CONNECT_TO_DC:
                testSignalDC();
                break;
            case ADS_TEST_SIGNAL_CONNECT_TO_PULSE_2X_SLOW:
                testSignalSlowPulse2();
                break;
            case ADS_TEST_SIGNAL_CONNECT_TO_PULSE_2X_FAST:
                testSignalFastPulse2();
                break;
            case ADS_NORMAL_INPUT:
                normalInputSignal();
                break;

                // CHANNEL SETTING COMMANDS
            case ADS_CHANNEL_CMD_SET: // This is a multi char command with a timeout
                startMultiCharCmdTimer(MULTI_CHAR_CMD_PROCESSING_INCOMING_SETTINGS_CHANNEL);
                numberOfIncomingSettingsProcessedChannel = 1;
                break;

                // LEAD OFF IMPEDANCE DETECTION COMMANDS
            case ADS_CHANNEL_IMPEDANCE_SET:
                startMultiCharCmdTimer(MULTI_CHAR_CMD_PROCESSING_INCOMING_SETTINGS_LEADOFF);
                numberOfIncomingSettingsProcessedLeadOff = 1;
                break;

            case ADS_CHANNEL_DEFAULT_ALL_SET: // reset all channel settings to default
                if (!streaming)
                {
                    Serial.print("updating channel settings to default");
                    sendEOT();
                }
                streamSafeSetAllChannelsToDefault();
                break;
            case ADS_CHANNEL_DEFAULT_ALL_REPORT: // report the default settings
                reportDefaultChannelSettings();
                break;

                // STREAM DATA AND FILTER COMMANDS
            case ADS_STREAM_START: // stream data
                streamStart(); // turn on the fire hose
                break;
            case ADS_STREAM_STOP: // stop streaming data
                streamStop();
                break;

                //  INITIALIZE AND VERIFY
            case ADS_MISC_SOFT_RESET:
                boardReset(); // initialize ADS and read device IDs
                break;

                //  QUERY THE ADSREGITSTERS
            case ADS_MISC_QUERY_REGISTER_SETTINGS:
                if (!streaming)
                {
                    readRegisters(); // print the ADS register values
                }
                break;

                // Sample rate set
            case ADS_SAMPLE_RATE_SET:
                startMultiCharCmdTimer(MULTI_CHAR_CMD_SETTINGS_SAMPLE_RATE);
                break;

            case ADS_TURN_ON_LED:
                turnOnLED();
                break;

            case ADS_TURN_OFF_LED:
                turnOffLED();
                break;

            case ADS_GET_VERSION:
                Serial.print("Brainwear v1.0");
                sendEOT();
                break;

            case ADS_ACTIVATE_SERIAL_STREAM:
                serial_stream = true;
                Serial.print("Stream via serial port activated");
                sendEOT();
                break;

            case ADS_DEACTIVATE_SERIAL_STREAM:
                serial_stream = false;
                Serial.print("Stream via serial port deactivated");
                sendEOT();
                break;
            case ADS_NUMBER_CHANNELS:
                Serial.print(ADS_CHANNELS_BOARD);
                sendEOT();
                break;

            default:
                return false;
        }
    }
    return true;
}

/**
 * @description Check for valid on multi char commands
 * @param None
 * @returns {boolean} true if a multi char commands is active and the timer is running, otherwise False
 */
boolean Brainwear::checkMultiCharCmdTimer(void)
{
    if (isMultiCharCmd)
    {
        if (millis() < multiCharCmdTimeout)
            return true;
        else
        { // the timer has timed out - reset the multi char timeout
            endMultiCharCmdTimer();
            Serial.print("Timeout processing multi byte");
            Serial.print(" Please send all the message");
            sendEOT();
        }
    }
    return false;
}

/**
 * @description Start the timer on multi char commands
 * @param cmd {char} the command received on the serial stream. See enum MULTI_CHAR_COMMAND
 * @returns void
 */
void Brainwear::startMultiCharCmdTimer(char cmd)
{
    isMultiCharCmd = true;
    multiCharCommand = cmd;
    multiCharCmdTimeout = millis() + MULTI_CHAR_COMMAND_TIMEOUT_MS;
}

/**
 * @description End the timer on multi char commands
 * @param None
 * @returns void
 */
void Brainwear::endMultiCharCmdTimer(void)
{
    isMultiCharCmd = false;
    multiCharCommand = MULTI_CHAR_CMD_NONE;
}

/**
 * @description Gets the active multi char command
 * @param None
 * @returns {char} multiCharCommand
 */
char Brainwear::getMultiCharCommand(void)
{
    return multiCharCommand;
}

/**
* @description When a 'x' is found on the serial port, we jump to this function
*                  where we continue to read from the serial port and read the
*                  remaining 7 bytes.
*/
void Brainwear::processIncomingChannelSettings(char character)
{

    if (character == ADS_CHANNEL_CMD_LATCH && numberOfIncomingSettingsProcessedChannel < ADS_NUMBER_OF_BYTES_SETTINGS_CHANNEL - 1)
    {
        // We failed somehow and should just abort
        numberOfIncomingSettingsProcessedChannel = 0;

        // put flag back down
        endMultiCharCmdTimer();

        if (!streaming)
        {
            Serial.print("Failure: ");
            Serial.print("too few chars");
            sendEOT();
        }
        return;
    }
    switch (numberOfIncomingSettingsProcessedChannel)
    {
        case 1: // channel number
            currentChannelSetting = getChannelCommandForAsciiChar(character);
            break;
        case 2: // POWER_DOWN
            optionalArgBuffer7[0] = getNumberForAsciiChar(character);
            break;
        case 3: // GAIN_SET
            optionalArgBuffer7[1] = getGainForAsciiChar(character);
            break;
        case 4: // INPUT_TYPE_SET
            optionalArgBuffer7[2] = getNumberForAsciiChar(character);
            break;
        case 5: // BIAS_SET
            optionalArgBuffer7[3] = getNumberForAsciiChar(character);
            break;
        case 6: // SRB2_SET
            optionalArgBuffer7[4] = getNumberForAsciiChar(character);
            break;
        case 7: // SRB1_SET
            optionalArgBuffer7[5] = getNumberForAsciiChar(character);
            break;
        case 8: // 'X' latch
            if (character != ADS_CHANNEL_CMD_LATCH)
            {
                if (!streaming)
                {
                    Serial.print("Failure: ");
                    Serial.print("too few chars");
                    sendEOT();
                }
                // We failed somehow and should just abort
                numberOfIncomingSettingsProcessedChannel = 0;

                // put flag back down
                endMultiCharCmdTimer();
            }
            break;
        default: // should have exited
            if (!streaming)
            {
                Serial.print("Failure: ");
                Serial.print("Err: too many chars");
                sendEOT();
            }
            // We failed somehow and should just abort
            numberOfIncomingSettingsProcessedChannel = 0;

            // put flag back down
            endMultiCharCmdTimer();
            return;
    }

    // increment the number of bytes processed
    numberOfIncomingSettingsProcessedChannel++;

    if (numberOfIncomingSettingsProcessedChannel == (ADS_NUMBER_OF_BYTES_SETTINGS_CHANNEL))
    {
        // We are done processing channel settings...
        if (!streaming)
        {
            char buf[2];
            Serial.print("Success: ");
            Serial.print("Channel set for ");
            Serial.print(itoa(currentChannelSetting + 1, buf, 10));
            sendEOT();
        }

        channelSettings[currentChannelSetting][POWER_DOWN] = optionalArgBuffer7[0];
        channelSettings[currentChannelSetting][GAIN_SET] = optionalArgBuffer7[1];
        channelSettings[currentChannelSetting][INPUT_TYPE_SET] = optionalArgBuffer7[2];
        channelSettings[currentChannelSetting][BIAS_SET] = optionalArgBuffer7[3];
        channelSettings[currentChannelSetting][SRB2_SET] = optionalArgBuffer7[4];
        channelSettings[currentChannelSetting][SRB1_SET] = optionalArgBuffer7[5];

        // Set channel settings
        streamSafeChannelSettingsForChannel(currentChannelSetting + 1, channelSettings[currentChannelSetting][POWER_DOWN], channelSettings[currentChannelSetting][GAIN_SET], channelSettings[currentChannelSetting][INPUT_TYPE_SET], channelSettings[currentChannelSetting][BIAS_SET], channelSettings[currentChannelSetting][SRB2_SET], channelSettings[currentChannelSetting][SRB1_SET]);

        // Reset
        numberOfIncomingSettingsProcessedChannel = 0;

        // put flag back down
        endMultiCharCmdTimer();
    }
}

/**
* @description When a 'z' is found on the serial port, we jump to this function
*                  where we continue to read from the serial port and read the
*                  remaining 4 bytes.
* @param `character` - {char} - The character you want to process...
*/
void Brainwear::processIncomingLeadOffSettings(char character)
{

    if (character == ADS_CHANNEL_IMPEDANCE_LATCH && numberOfIncomingSettingsProcessedLeadOff < ADS_NUMBER_OF_BYTES_SETTINGS_LEAD_OFF - 1)
    {
        // We failed somehow and should just abort
        // reset numberOfIncomingSettingsProcessedLeadOff
        numberOfIncomingSettingsProcessedLeadOff = 0;

        // put flag back down
        endMultiCharCmdTimer();

        if (!streaming)
        {
            Serial.print("Failure: ");
            Serial.print("Err: too many chars");
            sendEOT();
        }
        return;
    }
    switch (numberOfIncomingSettingsProcessedLeadOff)
    {
        case 1: // channel number
            currentChannelSetting = getChannelCommandForAsciiChar(character);
            break;
        case 2: // pchannel setting
            optionalArgBuffer7[0] = getNumberForAsciiChar(character);
            break;
        case 3: // nchannel setting
            optionalArgBuffer7[1] = getNumberForAsciiChar(character);
            break;
        case 4: // 'Z' latch
            if (character != ADS_CHANNEL_IMPEDANCE_LATCH)
            {
                if (!streaming)
                {
                    Serial.print("Failure: ");
                    Serial.print("Err: 5th char not Z");
                    sendEOT();
                }
                // We failed somehow and should just abort
                // reset numberOfIncomingSettingsProcessedLeadOff
                numberOfIncomingSettingsProcessedLeadOff = 0;

                // put flag back down
                endMultiCharCmdTimer();
            }
            break;
        default: // should have exited
            if (!streaming)
            {
                Serial.print("Failure: ");
                Serial.print("Err: too many chars");
                sendEOT();
            }
            // We failed somehow and should just abort
            // reset numberOfIncomingSettingsProcessedLeadOff
            numberOfIncomingSettingsProcessedLeadOff = 0;

            // put flag back down
            endMultiCharCmdTimer();
            return;
    }

    // increment the number of bytes processed
    numberOfIncomingSettingsProcessedLeadOff++;

    if (numberOfIncomingSettingsProcessedLeadOff == (ADS_NUMBER_OF_BYTES_SETTINGS_LEAD_OFF))
    {
        // We are done processing lead off settings...

        if (!streaming)
        {
            char buf[3];
            Serial.print("Success: ");
            Serial.print("Lead off set for ");
            Serial.print(itoa(currentChannelSetting + 1, buf, 10));
            sendEOT();
        }

        leadOffSettings[currentChannelSetting][PCHAN] = optionalArgBuffer7[0];
        leadOffSettings[currentChannelSetting][NCHAN] = optionalArgBuffer7[1];

        // Set lead off settings
        streamSafeLeadOffSetForChannel(currentChannelSetting + 1, leadOffSettings[currentChannelSetting][PCHAN], leadOffSettings[currentChannelSetting][NCHAN]);

        // reset numberOfIncomingSettingsProcessedLeadOff
        numberOfIncomingSettingsProcessedLeadOff = 0;

        // put flag back down
        endMultiCharCmdTimer();
    }
}

/**
* @description changes the sample rate with the multicommand option
*/
void Brainwear::processIncomingSampleRate(char c)
{
    if (c == ADS_SAMPLE_RATE_SET)
    {
        Serial.print("Success: ");
        Serial.print("Sample rate is ");
        Serial.print(getSampleRate());
        Serial.print("Hz");
        sendEOT();
    }
    else if (isDigit(c))
    {
        uint8_t digit = c - '0';
        if (digit <= SAMPLE_RATE_250)
        {
            streamSafeSetSampleRate((SAMPLE_RATE)digit);
            if (!streaming)
            {
                Serial.print("Success: ");
                Serial.print("Sample rate is ");
                Serial.print(getSampleRate());
                Serial.println("Hz");
                sendEOT();
            }
        }
        else
        {
            if (!streaming)
            {
                Serial.print("Failure: ");
                Serial.println("sample value out of bounds");
                sendEOT();
            }
        }
    }
    else
    {
        if (!streaming)
        {
            Serial.print("Failure: ");
            Serial.println("invalid sample value");
            sendEOT();

        }
    }
    endMultiCharCmdTimer();
}

/**
* @description Converts ascii character to byte value for channel setting bytes
* @param `asciiChar` - [char] - The ascii character to convert
* @return [char] - Byte number value of ascii character, defaults to 0
* @author AJ Keller (@pushtheworldllc)
*/
char Brainwear::getChannelCommandForAsciiChar(char asciiChar)
{
    switch (asciiChar)
    {
        case ADS_CHANNEL_CMD_CHANNEL_1:
            return 0x00;
        case ADS_CHANNEL_CMD_CHANNEL_2:
            return 0x01;
        case ADS_CHANNEL_CMD_CHANNEL_3:
            return 0x02;
        case ADS_CHANNEL_CMD_CHANNEL_4:
            return 0x03;
        case ADS_CHANNEL_CMD_CHANNEL_5:
            return 0x04;
        case ADS_CHANNEL_CMD_CHANNEL_6:
            return 0x05;
        case ADS_CHANNEL_CMD_CHANNEL_7:
            return 0x06;
        case ADS_CHANNEL_CMD_CHANNEL_8:
            return 0x07;
        default:
            return 0x00;
    }
}

/**
* @description Converts ascii character to get gain from channel settings
* @param `asciiChar` - [char] - The ascii character to convert
* @return [char] - Byte number value of acsii character, defaults to 0
* @author AJ Keller (@pushtheworldllc)
*/
char Brainwear::getNumberForAsciiChar(char asciiChar)
{
    if (asciiChar < '0' || asciiChar > '9')
    {
        asciiChar = '0';
    }

    // Convert ascii char to number
    asciiChar -= '0';

    return asciiChar;
}

/**
* @description Converts ascii character to get gain from channel settings
* @param `asciiChar` - [char] - The ascii character to convert
* @return [char] - Byte number value of acsii character, defaults to 0
* @author AJ Keller (@pushtheworldllc)
*/
char Brainwear::getGainForAsciiChar(char asciiChar)
{
    char output = 0x00;

    if (asciiChar < '0' || asciiChar > '6')
    {
        asciiChar = '6'; // Default to 24
    }

    output = asciiChar - '0';

    return output << 4;
}

/**
* @description gets the current sample rate in the settings
*/
const char *Brainwear::getSampleRate(void)
{
    switch (curSampleRate)
    {
        case SAMPLE_RATE_16000:
            return "16000";
        case SAMPLE_RATE_8000:
            return "8000";
        case SAMPLE_RATE_4000:
            return "4000";
        case SAMPLE_RATE_2000:
            return "2000";
        case SAMPLE_RATE_1000:
            return "1000";
        case SAMPLE_RATE_500:
            return "500";
        case SAMPLE_RATE_250:
        default:
            return "250";
    }
}

/**
* @description Change the current sample rate and restarts the Brainwear module
*/
void Brainwear::setSampleRate(uint8_t newSampleRateCode)
{
    curSampleRate = (SAMPLE_RATE)newSampleRateCode;
    initialize_ads();
}

//////////////////////////////////////////////
//////////// Stream-safe options//////////////
//////////////////////////////////////////////

/**
* @description Used to set lead off for a channel, if running must stop and start after...
* @param see `.channelSettingsSetForChannel()` for parameters
* @author AJ Keller (@pushtheworldllc)
*/
void Brainwear::streamSafeChannelSettingsForChannel(byte channelNumber, byte powerDown, byte gain, byte inputType, byte bias, byte srb2, byte srb1)
{
    boolean wasStreaming = streaming;

    // Stop streaming if you are currently streaming
    if (streaming)
    {
        streamStop();
    }

    writeChannelSettings(channelNumber);

    // Restart stream if need be
    if (wasStreaming)
    {
        streamStart();
    }
}

/**
* @description Used to set lead off for a channel, if running must stop and start after...
* @param `channelNumber` - [byte] - The channel you want to change
* @param `pInput` - [byte] - Apply signal to P input, either ON (1) or OFF (0)
* @param `nInput` - [byte] - Apply signal to N input, either ON (1) or OFF (0)
* @author AJ Keller (@pushtheworldllc)
*/
void Brainwear::streamSafeLeadOffSetForChannel(byte channelNumber, byte pInput, byte nInput)
{
    boolean wasStreaming = streaming;

    // Stop streaming if you are currently streaming
    if (streaming)
    {
        streamStop();
    }

    changeChannelLeadOffDetect(channelNumber);

    // Restart stream if need be
    if (wasStreaming)
    {
        streamStart();
    }
}

/**
* @description Used to set the sample rate
* @param sr {SAMPLE_RATE} - The sample rate to set to.
* @author AJ Keller (@pushtheworldllc)
*/
void Brainwear::streamSafeSetSampleRate(SAMPLE_RATE sr)
{
    boolean wasStreaming = streaming;

    // Stop streaming if you are currently streaming
    if (streaming)
    {
        streamStop();
    }

    setSampleRate(sr);

    // Restart stream if need be
    if (wasStreaming)
    {
        streamStart();
    }
}

/**
* @description Used to deactivate a channel, if running must stop and start after...
* @param channelNumber int the channel you want to change
* @author AJ Keller (@pushtheworldllc)
*/
void Brainwear::streamSafeChannelDeactivate(byte channelNumber)
{
    boolean wasStreaming = streaming;
    char buf[3];

    // Stop streaming if you are currently streaming
    if (streaming)
    {
        streamStop();
    }

    // deactivate the channel
    deactivateChannel(channelNumber);
    Serial.print("Channel: ");
    Serial.print(itoa(channelNumber, buf, DEC));
    Serial.println(" deactivated.");


    // Restart stream if need be
    if (wasStreaming)
    {
        streamStart();
    }
}

/**
* @description Used to activate a channel, if running must stop and start after...
* @param channelNumber int the channel you want to change
* @author AJ Keller (@pushtheworldllc)
*/
void Brainwear::streamSafeChannelActivate(byte channelNumber)
{
    boolean wasStreaming = streaming;
    char buf[3];

    // Stop streaming if you are currently streaming
    if (streaming)
    {
        streamStop();
    }

    // Activate the channel
    activateChannel(channelNumber);
    Serial.print("Channel: ");
    Serial.print(itoa(channelNumber, buf, DEC));
    Serial.println(" activated.");

    // Restart stream if need be
    if (wasStreaming)
    {
        streamStart();
    }
}

/**
* @description Used to set all channels on Board (and Daisy) to the default
*                  channel settings if running must stop and start after...
* @author AJ Keller (@pushtheworldllc)
*/
void Brainwear::streamSafeSetAllChannelsToDefault(void)
{
    boolean wasStreaming = streaming;

    // Stop streaming if you are currently streaming
    if (streaming)
    {
        streamStop();
    }

    setChannelsToDefault();

    // Restart stream if need be
    if (wasStreaming)
    {
        streamStart();
    }
}

//////////////////////////////////////////////
///////Data transmission functions////////////
//////////////////////////////////////////////
/**
* @description Changes the current mode of data transmission in a safe way
*/
void Brainwear::setCurTxMode(TX_MODE TxMode){
    boolean wasStreaming = streaming;

    // Stop streaming if you are currently streaming
    if (streaming)
    {
        streamStop();
    }

    curTxMode = TxMode;

    // Restart stream if need be
    if (wasStreaming)
    {
        streamStart();
    }
}

//////////////////////////////////////////////
////////Send Channel Data functions///////////
//////////////////////////////////////////////
/**
 * @description Called from the .ino file as the main sender. Driven by board mode,
 *  sample number, and ultimately the current packet type.
 */
void Brainwear::sendChannelData()
{
    sendChannelData(curTxMode);
}

/**
* @description Writes data to serial port.
*
*  Adds stop byte. See `Brainwear_Definitions.h`
*/
void Brainwear::sendChannelData(TX_MODE curTxMode)
{
    if(serial_stream) {
        if (curTxMode == DATA_RAW) {sendChannelDataSerial_Raw();}
        if (curTxMode == DATA_ASCII) {sendChannelDataSerial_Ascii();}
    }
    sampleCounter++;
}

/**
* @description Writes channel data to serial port sending chunks of 8 bytes.
*/
void Brainwear::sendChannelDataSerial_Raw(void)
{
    ADS_writeChannelData();     // 24 bytes or less (12 for 4 channels)
}


/**
* @description Writes raw data of the channels to serial port.
*/
void Brainwear::ADS_writeChannelData(void)
{
    for (int i = 0; i < ADS_CHANNELS_BOARD*ADS_BYTES_PER_CHAN; i++)
    {
        Serial.write(boardChannelDataRaw[i]);
    }
}

/**
* @description Writes channel data to serial port sending whole data.
*/
void Brainwear::sendChannelDataSerial_Ascii(void)
{
    for (int i = 0; i < ADS_CHANNELS_BOARD; i++)
    {
        Serial.print(boardChannelDataRaw[i]);
        Serial.print(" ");
    }
}

//////////////////////////////////////////////
//////////// Other functions//////////////////
//////////////////////////////////////////////

// HW
/**
* @description Used as HW debug when turning on the board
*/
void Brainwear::turnLED(void)
{
    RREG(GPIO);
    WREG(GPIO,0x00);
    delay(100);
    RREG(GPIO);
}

/**
* @description Turn off LED to check commands and SPI communication
*/
void Brainwear::turnOffLED(void)
{
    boolean wasStreaming = streaming;
    if (streaming)
    {
        streamStop();
    }

    RREG(GPIO);
    SDATAC();
    WREG(GPIO,0x80);
    delay(100);
    RREG(GPIO);

    // Restart stream if need be
    if (wasStreaming)
    {
        streamStart();
    }

}

/**
* @description Turn on LED to check commands and SPI communication
*/
void Brainwear::turnOnLED(void)
{
    boolean wasStreaming = streaming;
    if (streaming)
    {
        streamStop();
    }

    RREG(GPIO);
    SDATAC();
    WREG(GPIO,0x00);
    delay(100);
    RREG(GPIO);

    // Restart stream if need be
    if (wasStreaming)
    {
        streamStart();
    }

}

/**
* @description Hello world for the ADS1299
*/
byte Brainwear::getDeviceID(void){
    byte _data = RREG(ID_REG);
    return _data;
}

//////////////////////////////////////////////
//////////// ADS1299 Commands ////////////////
//////////////////////////////////////////////

void Brainwear::WAKEUP(void)
{
    digitalWrite(CS, LOW);
    SPI0->transfer(_WAKEUP);
    digitalWrite(CS, HIGH);
    delayMicroseconds(3); //must wait 4 tCLK cycles before sending another command (Datasheet, pg. 40)
}

void Brainwear::STANDBY(void)
{// only allowed to send WAKEUP after sending STANDBY
    digitalWrite(CS, LOW);
    SPI0->transfer(_STANDBY);
    digitalWrite(CS, HIGH);
}

void Brainwear::RESET(void)
{
    digitalWrite(CS, LOW);
    SPI0->transfer(_RESET);
    delayMicroseconds(12); //must wait 18 tCLK cycles to execute this command (Datasheet, pg. 41)
    digitalWrite(CS, HIGH);
}

void Brainwear::START(void)
{
    digitalWrite(CS, LOW);
    SPI0->transfer(_START);
    digitalWrite(CS, HIGH);
}

void Brainwear::STOP(void)
{
    digitalWrite(CS, LOW);
    SPI0->transfer(_STOP);
    digitalWrite(CS, HIGH);
}

void Brainwear::RDATAC(void)
{
    digitalWrite(CS, LOW);
    SPI0->transfer(_RDATAC); // read data continuous
    digitalWrite(CS, HIGH);
    delayMicroseconds(3); // data retrieval SCLKs or the SDATAC command should wait at least 4 tCLK cycles (Datasheet, pg. 40)
}

void Brainwear::SDATAC(void)
{
    digitalWrite(CS, LOW);
    SPI0->transfer(_SDATAC); // read data continuous
    digitalWrite(CS, HIGH);
    delayMicroseconds(10); // data retrieval SCLKs or the SDATAC command should wait at least 4 tCLK cycles (Datasheet, pg. 40)
}

byte Brainwear::RREG(byte _address)
{                                 //  reads ONE register at _address
    byte opcode1 = _RREG + _address; //  RREG expects 001rrrrr where rrrrr = _address
    digitalWrite(CS, LOW);          //  open SPI
    SPI0->transfer(_SDATAC);
    SPI0->transfer(opcode1);          //  opcode1
    SPI0->transfer(0x00);             //  opcode2 Read only one register
    byte _data = SPI0->transfer(0x00);    //  returned byte
    digitalWrite(CS, HIGH);         //  close SPI
    if (verbosity)
    { //  verbosity output
        printRegisterName(_address);
        printHex(_address);
        Serial.print(", ");
        printHex(_data);
        Serial.print(", ");
        // Show in binary
        for(byte j = 0; j<8; j++)
        {
            char buf[3];
            Serial.print(itoa(bitRead(_data, 7 - j), buf, DEC));
            if(j!=7) Serial.print(", ");
        }
        Serial.println();
    }
    return _data; // return requested register value
}

void Brainwear::RREG(byte _address, byte _numRegistersMinusOne)
{
    byte opcode1 = _RREG + _address; //001rrrrr; _RREG = 00100000 and _address = rrrrr
    digitalWrite(CS, LOW); //Low to communicated
    SPI0->transfer(_SDATAC);
    SPI0->transfer(opcode1); //RREG
    SPI0->transfer(_numRegistersMinusOne); //opcode2
    for(byte i = 0; i <= _numRegistersMinusOne; i++){
        byte _data = SPI0->transfer(0x00); // returned byte should match default of register map unless previously edited manually (Datasheet, pg.39)
        if(verbosity)
        {
            printRegisterName(_address+i);
            printHex(_address+i);
            Serial.print(", ");
            printHex(_data);
            Serial.print(", ");
            // Show in binary
            for(byte j = 0; j<8; j++)
            {
                char buf[3];
                Serial.print(itoa(bitRead(_data, 7 - j), buf, DEC));
                if(j!=7) Serial.print(", ");
            }
            Serial.println();
        }

    }
    digitalWrite(CS, HIGH); //High to end communication
}

void Brainwear::WREG(byte _address, byte _value)
{
    byte opcode1 = _WREG + _address; //010rrrrr; _WREG = 01000000 and _address = rrrrr
    digitalWrite(CS, LOW);
    SPI0->transfer(_SDATAC);
    SPI0->transfer(opcode1);          //  opcode1
    SPI0->transfer(0x00);             //  opcode2 Read only one register
    SPI0->transfer(_value);          //  Value to write
    if (verbosity)
    { //  verbosity output
        Serial.print("Register ");
        printHex(_address);
        Serial.print(" modified.");
        Serial.println();
    }
    digitalWrite(CS, HIGH); //High to end communication
}

//////////////////////////////////////////////
/////////// Printing functions////////////////
//////////////////////////////////////////////
/**
* @description Prints the names of the register according to datasheet
*/
void Brainwear::printRegisterName(byte _address) {
    if(_address == ID_REG){
        Serial.print("ID: ");
    }
    else if(_address == CONFIG1){
        Serial.print("CONFIG1: ");
    }
    else if(_address == CONFIG2){
        Serial.print("CONFIG2: ");
    }
    else if(_address == CONFIG3){
        Serial.print("CONFIG3: ");
    }
    else if(_address == LOFF){
        Serial.print("LOFF: ");
    }
    else if(_address == CH1SET){
        Serial.print("CH1SET: ");
    }
    else if(_address == CH2SET){
        Serial.print("CH2SET: ");
    }
    else if(_address == CH3SET){
        Serial.print("CH3SET: ");
    }
    else if(_address == CH4SET){
        Serial.print("CH4SET: ");
    }
    else if(_address == CH5SET){
        Serial.print("CH5SET: ");
    }
    else if(_address == CH6SET){
        Serial.print("CH6SET: ");
    }
    else if(_address == CH7SET){
        Serial.print("CH7SET: ");
    }
    else if(_address == CH8SET){
        Serial.print("CH8SET: ");
    }
    else if(_address == BIAS_SENSP){
        Serial.print("BIAS_SENSP: ");
    }
    else if(_address == BIAS_SENSN){
        Serial.print("BIAS_SENSN: ");
    }
    else if(_address == LOFF_SENSP){
        Serial.print("LOFF_SENSP: ");
    }
    else if(_address == LOFF_SENSN){
        Serial.print("LOFF_SENSN: ");
    }
    else if(_address == LOFF_FLIP){
        Serial.print("LOFF_FLIP: ");
    }
    else if(_address == LOFF_STATP){
        Serial.print("LOFF_STATP: ");
    }
    else if(_address == LOFF_STATN){
        Serial.print("LOFF_STATN: ");
    }
    else if(_address == GPIO){
        Serial.print("GPIO: ");
    }
    else if(_address == MISC1){
        Serial.print("MISC1: ");
    }
    else if(_address == MISC2){
        Serial.print("MISC2: ");
    }
    else if(_address == CONFIG4){
        Serial.print("CONFIG4: ");
    }
}

/**
* @description Print value in HEX
*/
void Brainwear::printHex(byte _data)
{
    Serial.print("0x");
    if (_data < 0x10)
        Serial.print("0");
    char buf[4];
    Serial.print(itoa(_data, buf, HEX));
}

