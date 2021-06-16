/*
* File to interface the SD
*/

//Taking into account four channels for the ADS1299
#define BLOCK_1MIN  910
#define BLOCK_5MIN  4550
#define BLOCK_15MIN  13650
#define BLOCK_30MIN  27300
#define BLOCK_1HR  54600
#define BLOCK_2HR  109200
#define BLOCK_4HR  218400


#define OVER_DIM 20 // make room for up to 20 write-time overruns

boolean cardInit = false;
boolean fileIsOpen = false;

SdFile openfile;  // want to put this before setup...
Sd2Card card;// SPI needs to be init'd before here
SdVolume volume;
SdFile root;
uint32_t bgnBlock, endBlock; // file extent bookends
uint8_t* pCache;      // array to store the block of data before saving it on the SD card
uint32_t MICROS_PER_BLOCK = 2000; // block write longer than this will get flaged
uint32_t BLOCK_COUNT;
boolean openvol;

char currentFileName[]="ADS_SD00.txt";
byte fileTens, fileOnes;  // enumerate succesive files on card and store number in EEPROM
File file;

int byteCounter = 0;    // used to hold position in cache
int blockCounter;       // count up to BLOCK_COUNT with this

struct {
    uint32_t block;   // holds block number that over-ran
    uint32_t micro;  // holds the length of this of over-run
} over[OVER_DIM];
uint32_t overruns;      // count the number of overruns
uint32_t maxWriteTime;  // keep track of longest write time
uint32_t minWriteTime;  // and shortest write time
uint32_t t;        // used to measure total file write time


const char elapsedTime[] PROGMEM = {"\n%Total time mS:\n"};  // 17
const char minTime[] PROGMEM = {  "%min Write time uS:\n"};  // 20
const char maxTime[] PROGMEM = {  "%max Write time uS:\n"};  // 20
const char overNum[] PROGMEM = {  "%Over:\n"};               //  7
const char blockTime[] PROGMEM = {  "%block, uS\n"};         // 11
const char startStamp[] PROGMEM = {  "%START AT\n"};    // used to stamp SD record when started by PC
const char stopStamp[] PROGMEM = {  "%STOP AT\n"};      // used to stamp SD record when stopped by PC

/**
 * @description Process the command sent via serial port
 */
char sdProcessChar(char character) {

    switch (character) {
        case ADS_INIT_SD:
            SDfileOpen = setupSDcard(character);
            break;
        case ADS_SD_1MIN:
            SDfileOpen = setupSDcard(character);
            break;
        case ADS_SD_5MIN:
            SDfileOpen = setupSDcard(character);
            break;
        case ADS_SD_15MIN:
            SDfileOpen = setupSDcard(character);
            break;
        case ADS_SD_30MIN:
            SDfileOpen = setupSDcard(character);
            break;
        case ADS_SD_1HR:
            SDfileOpen = setupSDcard(character);
            break;
        case ADS_SD_2HR:
            SDfileOpen = setupSDcard(character);
            break;
        case ADS_SD_4HR:
            SDfileOpen = setupSDcard(character);
            break;

        case ADS_RST_SDCOUNT: // Reset counter in EEPROM for files
            resetFileCounter();
        break;

        case ADS_CLOSE_SDFILE: // close the file, if it's open
            if(EEG.streaming){
                EEG.streamStop();
            }
            if(SDfileOpen){
                SDfileOpen = closeSDfile();
            }
            break;

        case ADS_STREAM_STOP:
            if(SDfileOpen) {
                stampSD(OFF);
            }
            break;

        case ADS_STREAM_START:
            if(SDfileOpen) {
                stampSD(ON);
                t = millis();
            }
            break;
        default:
            break;
    }
    return character;
}

/**
 * @description Initializes the SD card to record files
 */
boolean setupSDcard(char limit){

    if(!cardInit){
        if (!card.init(SPI_FULL_SPEED, SD_SS, SD_MOSI, SD_MISO, SD_SCL)) {
            if(!EEG.streaming) {
                Serial.println("initialization failed.! Things to check:");
                Serial.println("* is a card is inserted?");
                EEG.sendEOT();
                return fileIsOpen;
            }
        } else
        {
            if(!EEG.streaming) {
                Serial.println("Wiring is correct and a card is present.");
                EEG.sendEOT();
            }
            cardInit = true;
        }
        if (!volume.init(card)) { // Now we will try to open the 'volume'/'partition' - it should be FAT16 or FAT32
            if(!EEG.streaming) {
                Serial.println("Could not find FAT16/FAT32 partition. Make sure you've formatted the card");
                EEG.sendEOT();
            }
            return fileIsOpen;
        }
    }

    // use limit to determine file size
    switch(limit){
        case ADS_INIT_SD:
            BLOCK_COUNT = 512; break;
        case ADS_SD_1MIN:
            BLOCK_COUNT = BLOCK_1MIN; break;
        case ADS_SD_5MIN:
            BLOCK_COUNT = BLOCK_5MIN; break;
        case ADS_SD_15MIN:
            BLOCK_COUNT = BLOCK_15MIN; break;
        case ADS_SD_30MIN:
            BLOCK_COUNT = BLOCK_30MIN; break;
        case ADS_SD_1HR:
            BLOCK_COUNT = BLOCK_1HR; break;
        case ADS_SD_2HR:
            BLOCK_COUNT = BLOCK_2HR; break;
        case ADS_SD_4HR:
            BLOCK_COUNT = BLOCK_4HR; break;
        default:
            if(!EEG.streaming) {
                Serial.println("invalid BLOCK count");
                EEG.sendEOT();
            }
            return fileIsOpen;
    }
    incrementFileCounter();
    Serial.println(currentFileName);
    openvol = root.openRoot(volume);


    openfile.remove(root, currentFileName); // if the file is over-writing, let it!
    root.ls(LS_R | LS_DATE | LS_SIZE);

    if (!openfile.createContiguous(root, currentFileName, BLOCK_COUNT*512UL)) {
        if(!EEG.streaming) {
            Serial.println("created Contiguous fail");
        }
        cardInit = false;
    }
    if (!openfile.contiguousRange(&bgnBlock, &endBlock)) {
        if(!EEG.streaming) {
            Serial.println("get contiguousRange fail");
        }
        cardInit = false;
    }
    pCache = (uint8_t*)volume.cacheClear();
    if (!card.erase(bgnBlock, endBlock)){
        if(!EEG.streaming) {
            Serial.println("erase block fail");
        }
        cardInit = false;
    }
    if (!card.erase(bgnBlock, endBlock)){
        if(!EEG.streaming) {
            Serial.println("erase block fail");
        }
        cardInit = false;
    }
    if (!card.writeStart(bgnBlock, BLOCK_COUNT)){
        if(!EEG.streaming) {
            Serial.println("writeStart fail");
        }
        cardInit = false;
    } else{
        fileIsOpen = true;
        delay(1);
    }
    // initialize write-time overrun error counter and min/max wirte time benchmarks
    overruns = 0;
    maxWriteTime = 0;
    minWriteTime = 65000;
    byteCounter = 0;  // counter from 0 - 512
    blockCounter = 0; // counter from 0 - BLOCK_COUNT;
    if(fileIsOpen == true){  // send corresponding file name to controlling program
        if(!EEG.streaming) {
            Serial.print("Corresponding SD file ");
            Serial.println(currentFileName);
        }
    }
    if(!EEG.streaming) {
        EEG.sendEOT();
    }
    return fileIsOpen;
}

/**
 * @description Increments the filecounter to store the file in the SD card with a different identifier
 */
void incrementFileCounter(){
    fileTens = EEPROM.read(0);
    fileOnes = EEPROM.read(1);
//     if it's the first time writing to EEPROM, seed the file number to '00'
    if(fileTens == 0xFF | fileOnes == 0xFF){
        fileTens = fileOnes = '0';
    }
    fileOnes++;   // increment the file name
    if (fileOnes == ':'){fileOnes = 'A';}
    if (fileOnes > 'F'){
        fileOnes = '0';         // hexify
        fileTens++;
        if(fileTens == ':'){fileTens = 'A';}
        if(fileTens > 'F'){fileTens = '0';fileOnes = '1';}
    }
    EEPROM.write(0,fileTens);     // store current file number in eeprom
    EEPROM.write(1,fileOnes);
    EEPROM.commit();
    currentFileName[6] = fileTens;
    currentFileName[7] = fileOnes;
}

/**
 * @description Reset the counter that keeps track of the number of files stored in the SD (used for debug purposes)
 */
void resetFileCounter(){
    EEPROM.write(0,0xFF);
    EEPROM.write(1,0xFF);
    EEPROM.commit();
    Serial.println("File counter restarted");
    EEG.sendEOT();
}

/**
 * @description Write data to the SDcard
 */
void writeDataToSDcard(byte sampleNumber){
    boolean addComma = true;
    // convert 8 bit sampleCounter into HEX
    convertToHex(sampleNumber, 1, addComma);
    // convert 24 bit channelData into HEX
    for (int currentChannel = 0; currentChannel < ADS_CHANNELS_BOARD; currentChannel++){
        if (!addAuxtoSD && currentChannel == ADS_CHANNELS_BOARD-1) addComma = false;
        convertToHex(EEG.boardChannelDataInt[currentChannel], 5, addComma);
    }

    if(addAuxtoSD && multimode){
        // convert MMG into HEX
        for(int currentChannel = 0; currentChannel <  MMG_BOARDS*MMG_CHANNELS; currentChannel++){
            if(currentChannel < MMG_CHANNELS){
                convertToHex(MMG1.MMGData[currentChannel], 3, addComma);
            } else{
                if(currentChannel == (2*MMG_CHANNELS-1)) addComma = false;
                convertToHex(MMG2.MMGData[currentChannel], 3, addComma);
            }

        }
        addAuxtoSD = false;
    }
}

/**
 * @description CONVERT RAW BYTE DATA TO HEX FOR SD STORAGE
 * NumNibbles = number of bytes to convert -1
 */
void convertToHex(long rawData, int numNibbles, boolean useComma){
    for (int currentNibble = numNibbles; currentNibble >= 0; currentNibble--){
        byte nibble = (rawData >> currentNibble*4) & 0x0F;
        if (nibble > 9){
            nibble += 55;  // convert to ASCII A-F
        }
        else{
            nibble += 48;  // convert to ASCII 0-9
        }
        pCache[byteCounter] = nibble;
        byteCounter++;
        if(byteCounter == 512){
            writeCache();
        }
    }
    if(useComma == true){
        pCache[byteCounter] = ',';
    }else{
        pCache[byteCounter] = '\n';
    }
    byteCounter++;
    if(byteCounter == 512){
        writeCache();
    }
}// end of byteToHex converter

/**
 * @description counts the number of blocks written
 */
void writeCache(){
    if(blockCounter > BLOCK_COUNT) return;
    uint32_t tw = micros();  // start block write timer
    if(!card.writeData(pCache)){
        if (!EEG.streaming) {
            Serial.println("block write fail");
            EEG.sendEOT();
        }
    }// write the block
    tw = micros() - tw;      // stop block write timer
    if (tw > maxWriteTime) maxWriteTime = tw;  // check for max write time
    if (tw < minWriteTime) minWriteTime = tw;  // check for min write time
    if (tw > MICROS_PER_BLOCK) {      // check for overrun
        if (overruns < OVER_DIM) {
            over[overruns].block = blockCounter;
            over[overruns].micro = tw;
        }
        overruns++;
    }
    byteCounter = 0; // reset 512 byte counter for next block
    blockCounter++;    // increment BLOCK counter

    if(blockCounter == BLOCK_COUNT-1){
        t = millis() - t;
        EEG.streamStop();
        writeFooter();
    }

    if(blockCounter == BLOCK_COUNT){
        closeSDfile();
        BLOCK_COUNT = 0;
    }  // we did it!

}

/**
 * @description Close the SD file
 */
boolean closeSDfile(){
    if(fileIsOpen){
        card.writeStop();
        openfile.close();
        fileIsOpen = false;
        if(!EEG.streaming){ // verbosity. this also gets insterted as footer in openFile
            Serial.print("Total Elapsed Time: ");Serial.print(t);Serial.println(" mS"); //delay(10);
            Serial.print("Max write time: "); Serial.print(maxWriteTime); Serial.println(" uS"); //delay(10);
            Serial.print("Min write time: ");Serial.print(minWriteTime); Serial.println(" uS"); //delay(10);
            Serial.print("Overruns: "); Serial.print(overruns); Serial.println(); //delay(10);
            if (overruns) {
                uint8_t n = overruns > OVER_DIM ? OVER_DIM : overruns;
                Serial.println("fileBlock,micros");
                for (uint8_t i = 0; i < n; i++) {
                    Serial.print(over[i].block); Serial.print(','); Serial.println(over[i].micro);
                }
            }
            EEG.sendEOT();
        }
    }else{
        if(!EEG.streaming) {
            Serial.println("No open file to close");
            EEG.sendEOT();
        }
    }
    return fileIsOpen;
}

/**
 * @description Initial stamp to the SD
 */
void stampSD(boolean state){
    unsigned long time = millis();
    if(state){
        for(int i=0; i<10; i++){
            pCache[byteCounter] = pgm_read_byte_near(startStamp+i);
            byteCounter++;
            if(byteCounter == 512){
                writeCache();
            }
        }
    }
    else{
        for(int i=0; i<9; i++){
            pCache[byteCounter] = pgm_read_byte_near(stopStamp+i);
            byteCounter++;
            if(byteCounter == 512){
                writeCache();
            }
        }
    }
    convertToHex(time, 7, false);
}

/**
 * @description Footer of the SD file
 */
void writeFooter(){
    for(int i=0; i<17; i++){
        pCache[byteCounter] = pgm_read_byte_near(elapsedTime+i);
        byteCounter++;
    }
    convertToHex(t, 7, false);

    for(int i=0; i<20; i++){
        pCache[byteCounter] = pgm_read_byte_near(minTime+i);
        byteCounter++;
    }
    convertToHex(minWriteTime, 7, false);

    for(int i=0; i<20; i++){
        pCache[byteCounter] = pgm_read_byte_near(maxTime+i);
        byteCounter++;
    }
    convertToHex(maxWriteTime, 7, false);

    for(int i=0; i<7; i++){
        pCache[byteCounter] = pgm_read_byte_near(overNum+i);
        byteCounter++;
    }
    convertToHex(overruns, 7, false);
    for(int i=0; i<11; i++){
        pCache[byteCounter] = pgm_read_byte_near(blockTime+i);
        byteCounter++;
    }
    if (overruns) {
        uint8_t n = overruns > OVER_DIM ? OVER_DIM : overruns;
        for (uint8_t i = 0; i < n; i++) {
            convertToHex(over[i].block, 7, true);
            convertToHex(over[i].micro, 7, false);
        }
    }
    for(int i=byteCounter; i<512; i++){
        pCache[i] = NULL;
    }
    writeCache();
}