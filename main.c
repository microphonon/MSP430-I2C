/* MSP430FR2355 as slave on I2C bus. Runs in a timed loop to blink green LED.
   Receipt of 3-byte sequence 0x00 0x01 0x02 from master, halts the loop, latches
   the red LED, and enters LPM4. The 3-byte sequence 0x04 0x05 0x06 restarts the
   loop and clears the red LED.
     P1.2 SDA on UCB0
     P1.3 SCL on UCB0
*/
#include <msp430.h>
#include <stdint.h>

volatile uint8_t RxData[3], RxCount;
volatile uint8_t *PRxData;   // Pointer to receive buffer


void main(void) {

    WDTCTL = WDTPW | WDTHOLD;   //Stop watchdog timer
    PM5CTL0 &= ~LOCKLPM5; //Unlock GPIO
    P1SEL0 |= BIT2 + BIT3; //Set I2C pins; P1.2 UCB0SDA; P1.3 UCB0SCL
    P1DIR |= BIT0 + BIT1 + BIT4 + BIT5 + BIT6 + BIT7; //Set these pins to outputs
    P6DIR |= BIT0 + BIT1 + BIT2 + BIT3 + BIT4 + BIT5 + BIT6 + BIT7; //Set these pins to outputs
    P1OUT &= ~BIT0; //LEDs off
    P6OUT &= ~BIT6;

    CSCTL4 = SELA__VLOCLK;  //Set ACLK to VLO at 10 kHz
    /* MC_1 to count up to TB0CCR0, set to ACLK (VLO) and divide it by 8.
    The measured frequency is 1.2 kHz NOT 1.25 kHz. */
    TB0CTL |= MC_1 + TBSSEL__ACLK + TBCLR;
    TB0EX0 |= TBIDEX_7;
    TB0CCTL0 = CCIE; //Enable the Timer B interrupt

    UCB0CTLW0 = UCSWRST;                      // Software reset enabled
    UCB0CTLW0 |= UCMODE_3 + UCSYNC;           // I2C mode, sync mode (Do not set clock in slave mode)
    UCB0I2COA0 = 0x77 | UCOAEN;               // Slave address is 0x77; enable it
    UCB0CTLW0 &= ~UCSWRST;                    // Clear reset register
    UCB0IE |= UCRXIE0;              // Enable receive I2C interrupt
       __enable_interrupt(); //Enable global interrupts.

     RxCount =0;

    //Main loop follows
    while(1)
    {
        TB0CCR0 = 1100; //Looping period
        TB0CTL |= TBCLR; //Clear the timer counter
        LPM0;      //Wait in low power mode for timeout
        P6OUT |= BIT6; //Flash green LED
        TB0CCR0 = 100;
        TB0CTL |= TBCLR;
        LPM0;
        P6OUT &= ~BIT6; //Flash LED
       if (RxCount == 0) ;
       else if (RxCount == 3)
        {
            PRxData = (uint8_t *)RxData;    // Point PRxData to start of RxData
            if ((*PRxData == 1) && (*PRxData+1 == 2) && (*PRxData+2 == 3))
            {
                //Stop loop
                RxCount=0;
                P1OUT|= BIT0; //Red LED on
                TB0CTL = MC_0; //Stop the timer
                LPM4;
                //Rx flag is raised. Need some time to read the I2C bus
                __delay_cycles(100);
                //Check for correct 3 byte string
                while(1)
                {
                        if (RxCount == 3)
                        {
                            PRxData = (uint8_t *)RxData; // Point PRxData to start of RxData
                            if ((*PRxData == 4) && (*PRxData+1 == 5) && (*PRxData+2 == 6)) //Re-start slave loop
                            {
                                RxCount=0;
                                P1OUT &= ~BIT0; //Clear red LED
                                TB0CTL |= MC_1 + TBSSEL__ACLK + TBCLR; //Restart timer
                                break;
                            }
                            else //Incorrect 3-byte command; go back to sleep
                            {
                                RxCount=0;
                                LPM4; //Wait in LPM4 for master data
                                __delay_cycles(100);
                            }
                        }
                        else //Need to receive 3 bytes to re-start loop
                        {
                            RxCount=0;
                            LPM4; //Wait in LPM4 for master data
                            __delay_cycles(100);
                    }
                }
            }
            else RxCount=0;
        }
        else RxCount=0; //Byte count |= 0 or |=3; reset count

    } //end of main loop
}

#pragma vector=TIMER0_B0_VECTOR //This vector name is in header file
 __interrupt void Timer_B (void)
{
    LPM0_EXIT;
}

 #pragma vector = USCI_B0_VECTOR
 __interrupt void USCIB0_ISR(void)
 {
   switch(__even_in_range(UCB0IV, USCI_I2C_UCBIT9IFG))
   {
     case USCI_NONE:         break;           // Vector 0: No interrupts
     case USCI_I2C_UCALIFG:  break;           // Vector 2: ALIFG
     case USCI_I2C_UCNACKIFG:break;         // Vector 4: NACKIFG
     case USCI_I2C_UCSTTIFG: break;          // Vector 6: STTIFG
     case USCI_I2C_UCSTPIFG: break;            // Vector 8: STPIFG
     case USCI_I2C_UCRXIFG3: break;          // Vector 10: RXIFG3
     case USCI_I2C_UCTXIFG3: break;          // Vector 14: TXIFG3
     case USCI_I2C_UCRXIFG2: break;          // Vector 16: RXIFG2
     case USCI_I2C_UCTXIFG2: break;          // Vector 18: TXIFG2
     case USCI_I2C_UCRXIFG1: break;          // Vector 20: RXIFG1
     case USCI_I2C_UCTXIFG1: break;          // Vector 22: TXIFG1
     case USCI_I2C_UCRXIFG0:                 // Vector 24: RXIFG0
     /* I2C data will load into the buffer when in low power modes.
      Exiting from LPM must take place inside the interrupt. */
                             if (RxCount==0) LPM4_EXIT;
                             RxCount++;
                             *PRxData++ = UCB0RXBUF;
                             break;
     case USCI_I2C_UCTXIFG0: break;            // Vector 26: TXIFG0
     case USCI_I2C_UCBCNTIFG: break;         // Vector 28: BCNTIFG
     case USCI_I2C_UCCLTOIFG: break;         // Vector 30: clock low timeout
     case USCI_I2C_UCBIT9IFG: break;         // Vector 32: 9th bit
     default: break;
   }
 }

