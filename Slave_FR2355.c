/*I2C demo program to read a single byte of data from from a slave. Master is an MSP430FR5969 Launchpad.
 * Slave is a MSP430FR2355 Launchpad. If the master reads data byte = 0x03, flash green LED.
 * Otherwise flash red LED. Corresponding LEDs also toggle on the slave.
 * Slave is held in LPM4 and polled at rate set by the VLO timer. I2C clock supplied by master. This is the SLAVE code.
 * */
#include <msp430.h> 
#include <stdio.h>
#include <stdint.h>
volatile uint8_t Data, NewData;

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   //Stop watchdog timer
    //Set pins
    PM5CTL0 &= ~LOCKLPM5; //For brownout reset
    P1DIR |= BIT0; //Red LED on Launchpad
    P6DIR |= BIT6; //Green LED on Launchpad
    P1OUT &= ~BIT0; //LEDs off at startup
    P6OUT &= ~BIT6;
    P1SEL0 |= BIT2 + BIT3; //Set I2C pins; P1.2 UCB0SDA; P1.3 UCB0SCL
    //Setup I2C
    UCB0CTLW0 = UCSWRST;                      // Software reset enabled
    UCB0CTLW0 |= UCMODE_3 + UCSYNC;           // I2C mode, sync mode (Do not set clock in slave mode)
    UCB0I2COA0 = 0x77 | UCOAEN;               // Slave address is 0x77; enable it
    UCB0CTLW0 &= ~UCSWRST;                    // Clear reset register

    _BIS_SR(GIE); //Enable global interrupts.
    /*If the Data variable is modified directly, it can generate an undesired transmission interrupt.
      Use NewData to write to Data only once per loop and for logical branching statements below.
      Byte 0x03 lights green LED, all else red. */
    NewData = 0x03;
    UCB0IE |=UCTXIE0; //Enable start interrupt
    while(1){
        LPM4; //Wait in low-power mode for master interrupt
        Data = NewData; //Master interrupt occurred. Send single data byte to master from slave
        LPM0; //Wait for stop bit in LPM0
        while (UCB0CTL1 & UCTXSTP); // Ensure stop condition sent
        //Toggle the data sent to master and LEDs on slave Launchpad
        if(NewData == 0x03){
            P6OUT |= BIT6;
            P1OUT &= ~BIT0;
            NewData = 0x04;
            }
        else{
            NewData = 0x03;
            P6OUT &= ~BIT6;
            P1OUT |= BIT0;
            }
        }
}

#pragma vector = USCI_B0_VECTOR
__interrupt void USCIB0_ISR(void)
{
  switch(__even_in_range(UCB0IV, USCI_I2C_UCBIT9IFG))
  {
    case USCI_NONE:        break;           // Vector 0: No interrupts
    case USCI_I2C_UCALIFG: break;           // Vector 2: ALIFG
    case USCI_I2C_UCNACKIFG: break;         // Vector 4: NACKIFG
    case USCI_I2C_UCSTTIFG: break;          // Vector 6: STTIFG
    case USCI_I2C_UCSTPIFG: break;          // Vector 8: STPIFG
    case USCI_I2C_UCRXIFG3: break;          // Vector 10: RXIFG3
    case USCI_I2C_UCTXIFG3: break;          // Vector 14: TXIFG3
    case USCI_I2C_UCRXIFG2: break;          // Vector 16: RXIFG2
    case USCI_I2C_UCTXIFG2: break;          // Vector 18: TXIFG2
    case USCI_I2C_UCRXIFG1: break;          // Vector 20: RXIFG1
    case USCI_I2C_UCTXIFG1: break;          // Vector 22: TXIFG1
    case USCI_I2C_UCRXIFG0: break;          // Vector 24: RXIFG0
    case USCI_I2C_UCTXIFG0:                 // Vector 26: TXIFG0
                            UCB0TXBUF = Data;
                            LPM4_EXIT;
                            break;
    case USCI_I2C_UCBCNTIFG: break;         // Vector 28: BCNTIFG
    case USCI_I2C_UCCLTOIFG: break;         // Vector 30: clock low timeout
    case USCI_I2C_UCBIT9IFG: break;         // Vector 32: 9th bit
    default: break;
  }
}


