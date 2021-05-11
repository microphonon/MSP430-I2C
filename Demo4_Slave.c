/*I2C demo with MSP430FR2355 Launchpad as SLAVE. Read multiple control bytes from master.
If 4 control bytes are detected, toggle green LED, otherwise toggle red LED.
Data byte values are irrelevant; check only for the number of bytes sent. Toggle rate is set by
master. Slave is held in LPM4 with no clocks or timers running. Wakeup by reception of control bytes from master on I2C bus, which sets
receive interrupt. Escape from LPM4 by subsequent reception of stop bit from master, which sets UCSTPIFG interrupt.
This is the SLAVE code. Master code is Demo4_Master.c on FR5969 Launchpad.
P1.2 UCB0SDA
P1.3 UCB0SCL
*/
#include <msp430.h> 
#include <stdio.h>
#include <stdint.h>
volatile uint8_t RxData, RxCount; //Control bytes from master

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
    RxData = 0;

    while(1){
        RxCount=0;
        LPM4; //Wait for master interrupt in low power mode LPM4
        //Interrupt occurred. Check the received byte count
        if (RxCount == 0x04) P6OUT ^= BIT6; //Toggle green LED
        else P1OUT ^= BIT0; //Toggle red LED
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
                            RxData = UCB0RXBUF;
                            RxCount++;
                            break;
    case USCI_I2C_UCTXIFG0: break;             // Vector 26: TXIFG0
    case USCI_I2C_UCBCNTIFG: break;         // Vector 28: BCNTIFG
    case USCI_I2C_UCCLTOIFG: break;         // Vector 30: clock low timeout
    case USCI_I2C_UCBIT9IFG: break;         // Vector 32: 9th bit
    default: break;
  }
}


