 /*I2C demo with MSP430FR2355 Launchpad as SLAVE. Receive unspecified number control bytes (RxData < 11) from master.
  These bytes overwrite corresponding bytes in the array TxData. The entire 10 bytes of TxData are sent back
  to master when requested. Slave will automatically switch to transmit and immediately send
  TxData when a read request is made by master. Slave runs on a timed cycle in LPM3 and waits for timer or I2C interrupts.
  If 3rd byte is 0x03, flash green LED; otherwise flash red. Master and slave run in asynchronous timed loops.
  Slave loop is timed only by LED blink rate. This is the SLAVE code.
    P1.2  UCB0SDA with 10k pullup
    P1.3  UCB0SCL with 10k pullup
 */
 #include <msp430.h>
 #include <stdio.h>
 #include <stdint.h>
 #define BLINK 2525
 volatile uint8_t TxData[]={1,2,3,4,5,6,7,8,9,10}, TxCount;
 volatile uint8_t RxData[]={1,2,3,4,5,6,7,8,9,10}, RxCount;
 volatile uint8_t *PTxData,*PRxData;   // Pointers to data arrays
 volatile uint8_t i;

 int main(void)
 {
     WDTCTL = WDTPW | WDTHOLD;   //Stop watchdog timer
     PM5CTL0 &= ~LOCKLPM5; //Unlock GPIO
     P1DIR |= BIT0; //Red LED on Launchpad
     P6DIR |= BIT6; //Green LED on Launchpad
     P1SEL0 |= BIT2 + BIT3; //Set I2C pins; P1.2 UCB0SDA; P1.3 UCB0SCL
     P1OUT &= ~BIT0; //Turn off LEDs
     P6OUT &= ~BIT6;

     CSCTL4 |= SELA__VLOCLK;  //Set ACLK to VLO
     //Setup Timer B for flashing LEDs
     TB0CTL |= MC_1 + TBSSEL_1;
     TB0EX0 |= TBIDEX_3;
     TB0CCTL0 |= CCIE; //Enable timer interrupt
     //Setup I2C
     UCB0CTLW0 = UCSWRST;                      // Software reset enabled
     UCB0CTLW0 |= UCMODE_3 + UCSYNC;           // I2C mode, sync mode (Do not set clock in slave mode)
     UCB0I2COA0 = 0x77 | UCOAEN;               // Slave address is 0x77; enable it
     UCB0CTLW0 &= ~UCSWRST;                    // Clear reset register

     UCB0IE |= UCRXIE0 + UCTXIE0;              // Enable receive and transmit I2C interrupts
     __enable_interrupt(); //Enable global interrupts.

     RxCount = 0; // Received byte count

     while(1){
         TxCount = 10;
         PRxData = (uint8_t *)RxData;    // Point PRxData to start of RxData
         PTxData = (uint8_t *)TxData;    // Point TRxData to start of TxData
         //Execute the following if data received. Update TxData with new bytes. Slave will send TxData when requested.
         if(RxCount != 0){
             PRxData = (uint8_t *)RxData;
             PTxData = (uint8_t *)TxData;
             for (i=0;i<RxCount;i++)*PTxData++ = *PRxData++; //Overwrite TxData
             TxCount = 10;
             RxCount = 0;
             PRxData = (uint8_t *)RxData; //Reset pointers
             PTxData = (uint8_t *)TxData;
         }
        //Simple test of the TX data; look at 3rd byte
         if(*(PTxData+2)==0x03) P6OUT |= BIT6; //Green
         else P1OUT |= BIT0; //Red
         //Flash one of the LEDs
         TB0CCR0 = BLINK;
         TB0CTL |= TBCLR; //Clear the timer counter
         LPM3; //Wait for timeout in low power mode
         P6OUT &= ~BIT6;
         P1OUT &= ~BIT0;
         TB0CCR0 = BLINK;
         TB0CTL |= TBCLR;
         LPM3;
         }
 }

#pragma vector=TIMER0_B0_VECTOR //This vector name is in header file
 __interrupt void Timer_B (void)
{
    LPM3_EXIT;
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
     case USCI_I2C_UCSTPIFG: break;          // Vector 8: STPIFG
     case USCI_I2C_UCRXIFG3: break;          // Vector 10: RXIFG3
     case USCI_I2C_UCTXIFG3: break;          // Vector 14: TXIFG3
     case USCI_I2C_UCRXIFG2: break;          // Vector 16: RXIFG2
     case USCI_I2C_UCTXIFG2: break;          // Vector 18: TXIFG2
     case USCI_I2C_UCRXIFG1: break;          // Vector 20: RXIFG1
     case USCI_I2C_UCTXIFG1: break;          // Vector 22: TXIFG1
     case USCI_I2C_UCRXIFG0:                 // Vector 24: RXIFG0
                             RxCount++;
                             *PRxData++ = UCB0RXBUF;
                             break;
     case USCI_I2C_UCTXIFG0:                 // Vector 26: TXIFG0
                             if (TxCount)      // Check TX byte counter not empty
                             {
                                 UCB0TXBUF = *PTxData++; // Load TX buffer
                                 TxCount--;            // Decrement TX byte counter
                             }
                             else
                             {
                                 UCB0CTL1 |= UCTXSTP;        // Set I2C stop condition
                                 UCB0IFG &= ~UCTXIFG0;        // Clear USCI_B0 TX int flag
                             }
                             break;
     case USCI_I2C_UCBCNTIFG: break;            // Vector 28: BCNTIFG
     case USCI_I2C_UCCLTOIFG: break;         // Vector 30: clock low timeout
     case USCI_I2C_UCBIT9IFG: break;         // Vector 32: 9th bit
     default: break;
   }
 }


