//
// Created by jpipe on 2/26/2021.
//

#ifndef SOFTWARE_BRAINWEAR_DEFINITIONS_H
#define SOFTWARE_BRAINWEAR_DEFINITIONS_H

// Pin connections Huzzah 32 feather
#define CS        4
#define DRDY      39
#define START_PIN 36

#define SD_SCL     15
#define SD_MISO    14
#define SD_MOSI    32
#define SD_SS      33

// Baud rates
#define BAUD_RATE 115200

// File transmissions
#define ADS_BOP 0xA0 // Beginning of stream packet
#define ADS_EOP 0xC0 // Beginning of stream packet

//Address od ADS1X15
#define ADS1x15_1  0x49  // Used for FSR
#define ADS1x15_2  0x4A  // Used for piezos

//SPI Command Definitions (pg. 35)
#define _WAKEUP   0b00000010     // Wake-up from standby mode
#define _STANDBY  0b00000100   // Enter Standby mode
#define _RESET    0b00000110   // Reset the device
#define _START    0b00001000   // Start and restart (synchronize) conversions
#define _STOP     0b00001010   // Stop conversion
#define _RDATAC   0b00010000   // Enable Read Data Continuous mode (default mode at power-up)
#define _SDATAC   0b00010001   // Stop Read Data Continuous mode
#define _RDATA    0b00010010   // Read data by command; supports multiple read back

#define _RREG 0x20 // (also = 00100000) is the first opcode that the address must be added to for RREG communication
#define _WREG 0x40 // 01000000 in binary (Datasheet, pg. 35)

//ADS1299 Register Addressess
#define ID_REG      0x00  // this register contains ADS_ID
#define CONFIG1     0x01
#define CONFIG2     0x02
#define CONFIG3     0x03
#define LOFF        0x04
#define CH1SET      0x05
#define CH2SET      0x06
#define CH3SET      0x07
#define CH4SET      0x08
#define CH5SET      0x09
#define CH6SET      0x0A
#define CH7SET      0x0B
#define CH8SET      0x0C
#define BIAS_SENSP  0x0D
#define BIAS_SENSN  0x0E
#define LOFF_SENSP  0x0F
#define LOFF_SENSN  0x10
#define LOFF_FLIP   0x11
#define LOFF_STATP  0x12
#define LOFF_STATN  0x13
#define GPIO        0x14
#define MISC1       0x15
#define MISC2       0x16
#define CONFIG4     0x17

//Channel Settings
#define POWER_DOWN      (0)
#define GAIN_SET        (1)
#define INPUT_TYPE_SET  (2)
#define BIAS_SET        (3)
#define SRB2_SET        (4)
#define SRB1_SET        (5)
#define YES      	    (0x01)
#define NO      	    (0x00)

//gainCode choices
#define ADS_GAIN01 (0b00000000)	// 0x00
#define ADS_GAIN02 (0b00010000)	// 0x10
#define ADS_GAIN04 (0b00100000)	// 0x20
#define ADS_GAIN06 (0b00110000)	// 0x30
#define ADS_GAIN08 (0b01000000)	// 0x40
#define ADS_GAIN12 (0b01010000)	// 0x50
#define ADS_GAIN24 (0b01100000)	// 0x60
#define ADS_NOGAIN (0b01110000) // 0x70

//inputType choices
#define ADSINPUT_NORMAL     (0b00000000)
#define ADSINPUT_SHORTED    (0b00000001)
#define ADSINPUT_BIAS_MEAS  (0b00000010)
#define ADSINPUT_MVDD       (0b00000011)
#define ADSINPUT_TEMP       (0b00000100)
#define ADSINPUT_TESTSIG    (0b00000101)
#define ADSINPUT_BIAS_DRP   (0b00000110)
#define ADSINPUT_BIAL_DRN   (0b00000111)

//Number of channels
#define ADS_NUM_CHANNELS         8
#define ADS_CHANNELS_BOARD       4
#define ADS_BYTES_PER_CHAN       3
#define ADS_BYTES_PER_ADS_SAMPLE 24

//Number of additional channels for ADC Data (ADS1115)
#define MMG_CHANNELS     4
#define MMG_BOARDS       2

//Useful numbers
#define NUMBER_OF_BOARD_SETTINGS     1
#define NUMBER_OF_CHANNEL_SETTINGS   6
#define NUMBER_OF_LEAD_OFF_SETTINGS  2

//Lead-off signal choices
#define LOFF_MAG_6NA        (0b00000000)
#define LOFF_MAG_24NA       (0b00000100)
#define LOFF_MAG_6UA        (0b00001000)
#define LOFF_MAG_24UA       (0b00001100)
#define LOFF_FREQ_DC        (0b00000000)
#define LOFF_FREQ_7p8HZ     (0b00000001)
#define LOFF_FREQ_31p2HZ    (0b00000010)
#define LOFF_FREQ_FS_4      (0b00000011)
#define PCHAN (0)
#define NCHAN (1)
#define OFF (0)
#define ON (1)

//Settings
#define DEFAULT_CONFIG1     (0b10010000)

//test signal choices CONFIG2...ADS1299 datasheet page 47
#define ADSTESTSIG_AMP_1X       (0b00000000)
#define ADSTESTSIG_AMP_2X       (0b00000100)
#define ADSTESTSIG_PULSE_SLOW   (0b00000000)
#define ADSTESTSIG_PULSE_FAST   (0b00000001)
#define ADSTESTSIG_DCSIG        (0b00000011)
#define ADSTESTSIG_NOCHANGE     (0b11111111)

//COMMANDS TO CONTROL BOARD
/** Time out for multi char commands **/
#define MULTI_CHAR_COMMAND_TIMEOUT_MS 1000

/** Channel Setting Commands */
#define ADS_NUMBER_OF_BYTES_SETTINGS_CHANNEL 9
#define ADS_CHANNEL_CMD_SET             'x'
#define ADS_CHANNEL_CMD_LATCH           'X'
//Choose channel
#define ADS_CHANNEL_CMD_CHANNEL_1       '1'
#define ADS_CHANNEL_CMD_CHANNEL_2       '2'
#define ADS_CHANNEL_CMD_CHANNEL_3       '3'
#define ADS_CHANNEL_CMD_CHANNEL_4       '4'
#define ADS_CHANNEL_CMD_CHANNEL_5       '5'
#define ADS_CHANNEL_CMD_CHANNEL_6       '6'
#define ADS_CHANNEL_CMD_CHANNEL_7       '7'
#define ADS_CHANNEL_CMD_CHANNEL_8       '8'
//Power-down
#define ADS_CHANNEL_CMD_POWER_ON        '0'
#define ADS_CHANNEL_CMD_POWER_OFF       '1'
// PGA gain
#define ADS_CHANNEL_CMD_GAIN_1          '0'
#define ADS_CHANNEL_CMD_GAIN_2          '1'
#define ADS_CHANNEL_CMD_GAIN_4          '2'
#define ADS_CHANNEL_CMD_GAIN_6          '3'
#define ADS_CHANNEL_CMD_GAIN_8          '4'
#define ADS_CHANNEL_CMD_GAIN_12         '5'
#define ADS_CHANNEL_CMD_GAIN_24         '6'
//Channel input
#define ADS_CHANNEL_CMD_ADC_Normal      '0'
#define ADS_CHANNEL_CMD_ADC_Shorted     '1'
#define ADS_CHANNEL_CMD_ADC_BiasMethod  '2'
#define ADS_CHANNEL_CMD_ADC_MVDD        '3'
#define ADS_CHANNEL_CMD_ADC_Temp        '4'
#define ADS_CHANNEL_CMD_ADC_TestSig     '5'
#define ADS_CHANNEL_CMD_ADC_BiasDRP     '6'
#define ADS_CHANNEL_CMD_ADC_BiasDRN     '7'
//Bias generation
#define ADS_CHANNEL_CMD_BIAS_REMOVE     '0'
#define ADS_CHANNEL_CMD_BIAS_INCLUDE    '1'
// SRB2 connection
#define ADS_CHANNEL_CMD_SRB2_DISCONNECT '0'
#define ADS_CHANNEL_CMD_SRB2_CONNECT    '1'
//SRB1 connection
#define ADS_CHANNEL_CMD_SRB1_DISCONNECT '0'
#define ADS_CHANNEL_CMD_SRB1_CONNECT    '1'

/** LeadOff Impedance Commands */
#define ADS_NUMBER_OF_BYTES_SETTINGS_LEAD_OFF 5
#define ADS_CHANNEL_IMPEDANCE_SET                     'z'
#define ADS_CHANNEL_IMPEDANCE_LATCH                   'Z'

#define ADS_CHANNEL_IMPEDANCE_TEST_SIGNAL_APPLIED     '1'
#define ADS_CHANNEL_IMPEDANCE_TEST_SIGNAL_APPLIED_NOT '0'

/** Set sample rate */
#define ADS_SAMPLE_RATE_SET '~'

/** Turning channels off */
#define ADS_CHANNEL_OFF_1 '1'
#define ADS_CHANNEL_OFF_2 '2'
#define ADS_CHANNEL_OFF_3 '3'
#define ADS_CHANNEL_OFF_4 '4'
#define ADS_CHANNEL_OFF_5 '5'
#define ADS_CHANNEL_OFF_6 '6'
#define ADS_CHANNEL_OFF_7 '7'
#define ADS_CHANNEL_OFF_8 '8'

/** Turn channels on */
#define ADS_CHANNEL_ON_1 'Q'
#define ADS_CHANNEL_ON_2 'W'
#define ADS_CHANNEL_ON_3 'E'
#define ADS_CHANNEL_ON_4 'R'
#define ADS_CHANNEL_ON_5 'T'
#define ADS_CHANNEL_ON_6 'Y'
#define ADS_CHANNEL_ON_7 'U'
#define ADS_CHANNEL_ON_8 'I'

/** Test Signal Control Commands
 * 1x - Voltage will be 1 * (VREFP - VREFN) / 2.4 mV
 * 2x - Voltage will be 2 * (VREFP - VREFN) / 2.4 mV
 */
#define ADS_TEST_SIGNAL_CONNECT_TO_DC            'p'
#define ADS_TEST_SIGNAL_CONNECT_TO_GROUND        '0'
#define ADS_TEST_SIGNAL_CONNECT_TO_PULSE_1X_FAST '='
#define ADS_TEST_SIGNAL_CONNECT_TO_PULSE_1X_SLOW '-'
#define ADS_TEST_SIGNAL_CONNECT_TO_PULSE_2X_FAST ']'
#define ADS_TEST_SIGNAL_CONNECT_TO_PULSE_2X_SLOW '['
#define ADS_NORMAL_INPUT                         'n'

/** Default Channel Settings */
#define ADS_CHANNEL_DEFAULT_ALL_SET 'd'
#define ADS_CHANNEL_DEFAULT_ALL_REPORT 'D'

/** Default Channel Settings */
#define ADS_NUMBER_CHANNELS    'C'

/** Stream Data Commands */
#define ADS_ACTIVATE_SERIAL_STREAM  't'
#define ADS_DEACTIVATE_SERIAL_STREAM  'y'
#define ADS_STREAM_START  'b'
#define ADS_STREAM_STOP   's'

/** Miscellaneous */
#define ADS_MISC_QUERY_REGISTER_SETTINGS '?'
#define ADS_MISC_SOFT_RESET              'v'
#define ADS_GET_VERSION                  'V'

/** Turn On/Off LED */
#define ADS_TURN_ON_LED  'l'
#define ADS_TURN_OFF_LED 'k'

/** SD Setting Commands */
#define ADS_INIT_SD         'a'
#define ADS_RST_SDCOUNT     'r'
#define ADS_CLOSE_SDFILE    'j'
#define ADS_SD_1MIN         'A'
#define ADS_SD_5MIN         'S'
#define ADS_SD_15MIN         'F'
#define ADS_SD_30MIN         'G'
#define ADS_SD_1HR         'H'
#define ADS_SD_2HR         'J'
#define ADS_SD_4HR         'K'

/** board Commands */
#define ADS_TX_RAW         '<'
#define ADS_TX_ASCII       '>'
#define ADS_MULTMODE_ON    'M'
#define ADS_MULTMODE_OFF   'N'


#endif //SOFTWARE_BRAINWEAR_DEFINITIONS_H
