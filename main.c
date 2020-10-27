//--------------------------------------------------------------------
// Name:            Chris Coulston
// Date:            Fall 2020
// Purp:            inLab09
//
// Assisted:        The entire class of EENG 383
// Assisted by:     Microchips 18F26K22 Tech Docs 
//-
//- Academic Integrity Statement: I certify that, while others may have
//- assisted me in brain storming, debugging and validating this program,
//- the program itself is my own work. I understand that submitting code
//- which is the work of other individuals is a violation of the course
//- Academic Integrity Policy and may result in a zero credit for the
//- assignment, or course failure and a report to the Academic Dishonesty
//- Board. I also understand that if I knowingly give my original work to
//- another individual that it could also result in a zero credit for the
//- assignment, or course failure and a report to the Academic Dishonesty
//- Board.
//------------------------------------------------------------------------
#include "mcc_generated_files/mcc.h"
#include "sdCard.h"
#pragma warning disable 520     // warning: (520) function "xyz" is never called  3
#pragma warning disable 1498    // fputc.c:16:: warning: (1498) pointer (unknown)
#define SINE_WAVE_ARRAY_LENGTH 26

void myTMR0ISR(void);

#define BLOCK_SIZE          512
//#define RATE                1600
#define MAX_NUM_BLOCKS      4
#define ZERO 0

uint16_t sampleRate = 1600;

uint8_t beginSampling = false;
uint8_t blueFull = false;
uint8_t redFull = false;

// Large arrays need to be defined as global even though you may only need to 
// use them in main.  This quirk will be important in the next two assignments.
uint8_t blueBuffer[BLOCK_SIZE];
uint8_t redBuffer[BLOCK_SIZE];
const uint8_t   sin[SINE_WAVE_ARRAY_LENGTH] = {128,	159,	187,	213,	233,	248,	255,	255,	248,	233,	213,	187,	159,	128,	97,	69,	43,	23,	8,	1,	1,	8,	23,	43,	69,	97};
//----------------------------------------------20
// Main "function"
//----------------------------------------------

void main(void) {

    uint8_t status;
    uint16_t i;
    uint32_t sdCardAddress = 0x00000000;
    uint16_t bufIdx = 0;
    uint8_t waveIdx = 0;
    uint32_t localAddress;
    char cmd, letter;

    letter = '0';

    SYSTEM_Initialize();
    CS_SetHigh();

    // Provide Baud rate generator time to stabilize before splash screen
    TMR0_WriteTimer(0x0000);
    INTCONbits.TMR0IF = 0;
    while (INTCONbits.TMR0IF == 0);

    TMR0_SetInterruptHandler(myTMR0ISR);

    INTERRUPT_GlobalInterruptEnable();
    INTERRUPT_PeripheralInterruptEnable();

    printf("inLab 09\r\n");
    printf("SD card testing\r\n");
    printf("Dev'21\r\n");
    printf("No configuration of development board\r\n> "); // print a nice command prompt    
    
    SPI2_Close();
    SPI2_Open(SPI2_DEFAULT);
    
    for (;;) {
        
        if (EUSART1_DataReady) { // wait for incoming data on USART
            cmd = EUSART1_Read();
            switch (cmd) { // and do what it tells you to do

                    //--------------------------------------------
                    // Reply with help menu
                    //--------------------------------------------
                case '?':
                    printf("\r\n-------------------------------------------------\r\n");
                    printf("SD card address:  ");
                    printf("%04x", sdCardAddress >> 16);
                    printf(":");
                    printf("%04x", sdCardAddress & 0X0000FFFF);
                    printf("\r\n");
                    printf("-------------------------------------------------\r\n");
                    printf("?: help menu\r\n");
                    printf("o: k\r\n");
                    printf("Z: Reset processor\r\n");
                    printf("z: Clear the terminal\r\n");
                    printf("-------------------------------------------------\r\n");
                    printf("i: Initialize SD card\r\n");
                    printf("-------------------------------------------------\r\n");
//                    printf("----------------SPI TEST-------------------------\r\n");
//                    printf("t: send a Test character over SPI\r\n");
                    
                    printf("a/A decrease/increase read address\r\n");
                    printf("r: read a block of %d bytes from SD card\r\n", BLOCK_SIZE);
                    printf("1: write a perfect 26 value sine wave to the SD card\r\n", BLOCK_SIZE);
                    printf("-------------------------------------------------\r\n");
                    printf("+/-: Increase/Decrease the sample rate by 10 us\r\n", BLOCK_SIZE);
                    printf("W: Write microphone => SD card at 1600 us\r\n", BLOCK_SIZE);
                    printf("s: spool memory to a csv file\r\n", BLOCK_SIZE);
                    printf("-------------------------------------------------\r\n");
                    break;

                    //--------------------------------------------
                    // Reply with "k", used for PC to PIC test
                    //--------------------------------------------
                case 'o':
                    printf("o:	ok\r\n");
                    break;

                    //--------------------------------------------
                    // Reset the processor after clearing the terminal
                    //--------------------------------------------                      
                case 'Z':
                    for (i = 0; i < 40; i++) printf("\n");
                    RESET();
                    break;

                    //--------------------------------------------
                    // Clear the terminal
                    //--------------------------------------------                      
                case 'z':
                    for (i = 0; i < 40; i++) printf("\n");
                    break;

                    //--------------------------------------------
                    // Clear the terminal
                    //--------------------------------------------             
                /*
                case 't':                    
                    printf("    Connect oscilloscope channel 1 to PIC header pin RB1 (vertical scale 2v/div)\r\n");
                    printf("    Connect oscilloscope channel 2 to PIC header pin RB3 (vertical scale 2v/div)\r\n");
                    printf("    Trigger on channel 1\r\n");
                    printf("    Set the horizontal scale to 500ns/div\r\n");
                    printf("    Hit any key when ready\r\n");
                    while (!EUSART1_DataReady);
                    (void) EUSART1_Read();

                    printf("sent: %02x  received: %02x\r\n", letter, SPI2_ExchangeByte(letter));
                    letter += 1;
                    break;
*/
                    //-------------------------------------------- 
                    // Init SD card to get it read to perform read/write
                    // Will hang in infinite loop on error.
                    //--------------------------------------------    
                case 'i':
                    SPI2_Close();
                    SPI2_Open(SPI2_DEFAULT);    // Reset the SPI channel for SD card communication
                    SDCARD_Initialize(true);
                    break;

                    //--------------------------------------------
                    // Increase or decrease block address
                    //--------------------------------------------                 
                case 'A':
                case 'a':
                    if (cmd == 'a') {
                        sdCardAddress -= BLOCK_SIZE;
                        if (sdCardAddress >= 0x04000000) {
                            printf("Underflowed to high address\r\n");
                            sdCardAddress = 0x04000000 - BLOCK_SIZE;
                        } else {
                            printf("Decreased address\r\n");
                        }
                    } else {
                        sdCardAddress += BLOCK_SIZE;
                        if (sdCardAddress >= 0x04000000) {
                            printf("Overflowed to low address\r\n");
                            sdCardAddress = 0x00000000;
                        } else {
                            printf("Increased address\r\n");
                        }
                    }

                    // 32-bit integers need printed as a pair of 16-bit integers
                    printf("SD card address:  ");
                    printf("%04x", sdCardAddress >> 16);
                    printf(":");
                    printf("%04x", sdCardAddress & 0X0000FFFF);
                    printf("\r\n");
                    break;

//                    //--------------------------------------------
//                    // w: write a block of BLOCK_SIZE bytes to SD card
//                    //--------------------------------------------
                case 'w':
                    for (i = 0; i < BLOCK_SIZE; i++) blueBuffer[i] = 255 - i;
                    WRITE_TIME_PIN_SetHigh();
                    SDCARD_WriteBlock(sdCardAddress, blueBuffer);
                    while ((status = SDCARD_PollWriteComplete()) == WRITE_NOT_COMPLETE);
                    WRITE_TIME_PIN_SetLow();
                    
                    printf("Write block of decremented 8-bit values:\r\n");
                    printf("    Address:    ");
                    printf("%04x", sdCardAddress >> 16);
                    printf(":");
                    printf("%04x", sdCardAddress & 0X0000FFFF);
                    printf("\r\n");
                    printf("    Status:     %02x\r\n", status);
                    break;

                    //--------------------------------------------
                    // r: read a block of BLOCK_SIZE bytes from SD card                
                    //--------------------------------------------
                case 'r':
                    READ_TIME_PIN_SetHigh();
                    SDCARD_ReadBlock(sdCardAddress, blueBuffer);
                    READ_TIME_PIN_SetLow();
                    printf("Read block: \r\n");
                    printf("    Address:    ");
                    printf("%04x", sdCardAddress >> 16);
                    printf(":");
                    printf("%04x", sdCardAddress & 0X0000FFFF);
                    printf("\r\n");
                    hexDumpBuffer(blueBuffer);
                    break;


                case '1':
                    waveIdx = 0;
                    localAddress = sdCardAddress;
                    printf("Writing sine wave to memory, starting with the current address. Press any key to stop.\r\n");
                    while (!EUSART1_DataReady){
                        
                    //First, construct the write buffer. 
                    for(bufIdx = 0;bufIdx < BLOCK_SIZE;waveIdx = (waveIdx + 1) % 26, bufIdx++){
                        blueBuffer[bufIdx] = sin[waveIdx];
                    }
                    //Then write the buffer
                    SDCARD_WriteBlock(localAddress, blueBuffer);
                    while ((status = SDCARD_PollWriteComplete()) == WRITE_NOT_COMPLETE);
                    localAddress += BLOCK_SIZE;
                    
                    }
                    //get the key that interrupted the write out of the buffer
                    (void) EUSART1_Read();
                    printf("Write stopped\r\n");
                    break;
                case '+':
                    sampleRate += 160;
                    printf("The sampling rate is %u us\r\n", sampleRate/16);
                    break;
                case '-':
                    sampleRate -= 160;
                    if(sampleRate < 320) sampleRate = 320;
                    printf("The sampling rate is %u us\r\n", sampleRate/16);
                    break;
                case 'W':
                    printf("Press any key to start recording audio and press any key to stop recording\r\n");
                    //key to start sampling
                    localAddress = sdCardAddress;
                    while(!EUSART1_DataReady);
                    (void) EUSART1_Read();//remove the start key from buffer might be unnecessary
                    beginSampling = true;     
                    //key to stop sampling
                    while(!EUSART1_DataReady){
                        if(blueFull == true){
                            WRITE_TIME_PIN_SetHigh();
                            SDCARD_WriteBlock(localAddress, blueBuffer);
                            while ((status = SDCARD_PollWriteComplete()) == WRITE_NOT_COMPLETE);
                            WRITE_TIME_PIN_SetLow();
                            localAddress += BLOCK_SIZE;
                            blueFull = false;
//                            printf("Just wrote blue buffer to SDcard\r\n");
                        }
                        if(redFull == true){
                            WRITE_TIME_PIN_SetHigh();
                            SDCARD_WriteBlock(localAddress, redBuffer);
                            while ((status = SDCARD_PollWriteComplete()) == WRITE_NOT_COMPLETE);
                            WRITE_TIME_PIN_SetLow();
                            localAddress += BLOCK_SIZE;
                            redFull = false;
//                            printf("Just wrote red buffer to SDcard\r\n");
                        }
                    }
                    (void) EUSART1_Read();
                    beginSampling = false;
                    
                    //*****************
                    //needs a status print statement somewhere to slow down the process
                    //*****************
                    
                    break;
                case 's':
                    //Gigantic string but it works. The whole thing is constant so no need to format it.
                    printf("\r\nYou may terminate spooling at anytime with a keypress.\r\nTo spool terminal contents into a file follow these instructions:\r\n\r\nRight mouse click on the upper left of the PuTTY window\r\nSelect:     Change settings...\r\nSelect:     Logging\r\nSelect:     Session logging: All session output\r\nLog file name:  Browse and provide a .csv extension to your file name\r\nSelect:     Apply\r\nPress any key to start");
                    
                    //The instructions say to be able to stop spooling between blocks, the rubric says to allow stops midway through a block.
                    //I'll follow the rubric
                    localAddress = sdCardAddress;
                    while(!EUSART1_DataReady){
                    SDCARD_ReadBlock(localAddress, blueBuffer);
                    for(int i = 0;i<BLOCK_SIZE;i++){
                        printf("%d\r\n", blueBuffer[i]);
                        if(EUSART1_DataReady){
                            //Data is still ready after breaking out so you leave the while loop too
                            break;
                        }
                    }
                    localAddress += BLOCK_SIZE;
                    
                    }
                    (void) EUSART1_Read();
                    //Another massive constant string
                    printf("Spooled 512 out of the 512 blocks. \r\nTo close the file follow these instructions: \r\n \r\nRight mouse click on the upper left of the PuTTY window \r\nSelect:     Change settings... \r\nSelect:     Logging \r\nSelect:     Session  logging: None \r\nSelect:     Apply");
                    break;
                    //--------------------------------------------
                    // If something unknown is hit, tell user
                    //--------------------------------------------
                default:
                    printf("Unknown key %c\r\n", cmd);
                    break;
            } // end switch

        } // end if
    } // end while 
} // end main


//----------------------------------------------
// As configured, we are hoping to get a toggle
// every 100us - this will require some work.
//
// You will be starting an ADC conversion here and
// storing the results (when you reenter) into a global
// variable and setting a flag, alerting main that 
// it can read a new value.
//
// !!!MAKE SURE THAT TMR0 has 0 TIMER PERIOD in MCC!!!!
//----------------------------------------------

typedef enum  {MIC_IDLE, MIC_ACQUIRE_BLUE, MIC_ACQUIRE_RED} myTMR0states_t;
myTMR0states_t timerState = MIC_IDLE;

uint16_t bufferIdx = 0;
void myTMR0ISR(void) {
    //Ensure that there is always something in the buffer. 
//    while(ADCON0bits.GO_NOT_DONE == 1);
    uint8_t micReading = ADRESH;
    //Always need something in the buffer
    //make CERTAIN that you give this as much time as you possibly can to work.
    //It should always be the second line of this ISR
    //I tried to put it at the end and it broke everything. 
    ADCON0bits.GO_NOT_DONE = 1; 
    //Each ISR will tell the ADC to take the sample now. By the next ISR, the sample should be through the ADC. 
    //This means that the data an ISR places into the buffer at a given interrupt is the data it got during the last interrupt.
    //I believe this doesn't actually cause problems?
    switch(timerState){
        
        
        case MIC_IDLE:
            if(beginSampling == true){
                timerState = MIC_ACQUIRE_BLUE;
                bufferIdx = 0; 
            }
            break;
        
        case MIC_ACQUIRE_BLUE:
            blueBuffer[bufferIdx] = micReading;
            bufferIdx += 1;
            if(bufferIdx >= BLOCK_SIZE){
                blueFull = true;
                timerState = MIC_ACQUIRE_RED;
                bufferIdx = 0;
            }
            if(beginSampling == false){
                timerState = MIC_IDLE;
            }
            break;
            
        case MIC_ACQUIRE_RED:
            redBuffer[bufferIdx] = micReading;
            bufferIdx += 1;
            if(bufferIdx >= BLOCK_SIZE){
                redFull = true;
                timerState = MIC_ACQUIRE_BLUE;
                bufferIdx = 0;
            }
            if(beginSampling == false){
                timerState = MIC_IDLE;
            }
            break;
        
    }
        
        
        //Need to add fudge value. I have no idea how to count the cycles.
        //We'll probably need to ask a TA.
        
        TMR0_WriteTimer(TMR0_ReadTimer() + (0x10000 - sampleRate));
        INTCONbits.TMR0IF = 0;
            

} // end myTMR0ISR


/* end of file */