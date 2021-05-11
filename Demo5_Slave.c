 /*I2C demo with MSP430FR2355 Launchpad as SLAVE. Read multiple control bytes from master.
 If last control byte sent is 0x04, toggle green LED, otherwise toggle red LED.
 Different size data byte arrays may be sent. Toggle rate is set by
 master. Slave is held in LPM4 with no clocks or timers running. Wakeup by reception of control bytes
 from master on I2C bus, which sets receive interrupt. Escape from LPM4 by reception of
 stop bit from master, which sets UCSTPIE interrupt. This is the SLAVE code.
 Master code is Demo4_Master.c on FR5969 Launchpad.
 */
 #include <msp430.h>
 #include <stdio.h>
 #include <stdint.h>
//RxData must be initialized to at least 1 element more than expected received array size
 volatile uint8_t RxData[]={0,0,0,0,0,0,0,0};
 volatile uint8_t *PRxData;   // Pointer to RxData

 int main(void)
 {
     WDTCTL = WDTPW | WDTHOLD;   //Stop watchdog timer
     PM5CTL0 &= ~LOCKLPM5; //For brownout reset
     P1DIR |= BIT0; //Red LED on Launchpad
     P6DIR |= BIT6; //Green LED on Launchpad
     P1SEL0 |= BIT2 + BIT3; //Set I2C pins; P1.2 UCB0SDA; P1.3 UCB0SCL
     P1OUT &= ~BIT0; //Turn off LEDs
     P6OUT &= ~BIT6;

     //Setup I2C
     UCB0CTLW0 = UCSWRST;                      // Software reset enabled
     UCB0CTLW0 |= UCMODE_3 + UCSYNC;           // I2C mode, sync mode (Do not set clock in slave mode)
     UCB0I2COA0 = 0x77 | UCOAEN;               // Slave address is 0x77; enable it
     UCB0CTLW0 &= ~UCSWRST;                    // Clear reset register
     UCB0IE |= UCRXIE0 + UCSTPIE;              // Enable receive and stop interrupts

     _BIS_SR(GIE); //Enable global interrupts.

     while(1){
         PRxData = (uint8_t *)RxData;    // Point PRxData to start of RX buffer
         LPM4; //Wait for master interrupt in low power mode LPM4
         //Interrupt occurred. Check the last received byte, which is one less than current pointer position
         if (*(--PRxData) == 0x04) P6OUT ^= BIT6; //Green LED
         else P1OUT ^= BIT0; //Red LED
         }
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
     case USCI_I2C_UCSTPIFG:                 // Vector 8: STPIFG
                             LPM4_EXIT;
                             break;
     case USCI_I2C_UCRXIFG3: break;          // Vector 10: RXIFG3
     case USCI_I2C_UCTXIFG3: break;          // Vector 14: TXIFG3
     case USCI_I2C_UCRXIFG2: break;          // Vector 16: RXIFG2
     case USCI_I2C_UCTXIFG2: break;          // Vector 18: TXIFG2
     case USCI_I2C_UCRXIFG1: break;          // Vector 20: RXIFG1
     case USCI_I2C_UCTXIFG1: break;          // Vector 22: TXIFG1
     case USCI_I2C_UCRXIFG0:                 // Vector 24: RXIFG0
                             *PRxData++ = UCB0RXBUF;
                             break;
     case USCI_I2C_UCTXIFG0: break;             // Vector 26: TXIFG0
     case USCI_I2C_UCBCNTIFG: break;         // Vector 28: BCNTIFG
     case USCI_I2C_UCCLTOIFG: break;         // Vector 30: clock low timeout
     case USCI_I2C_UCBIT9IFG: break;         // Vector 32: 9th bit
     default: break;
   }
 }


