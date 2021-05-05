/*I2C demo with MSP430FR2355 Launchpad as SLAVE. Read single control byte from master.
 If the control byte is 0x01 toggle green LED, otherwise toggle red LED. Toggle rate is set by
variable ON in Timer_B with ACLK set to VLO. Interrupt either by timeout or reception of control byte
from master on I2C bus. Slave is held in LPM3 to keep VLO clock running.
P1.2 UCB0SDA
P1.3 UCB0SCL
This is the SLAVE code. Master code is Demo2_Master.c on FR5969 Launchpad.
*/
#include <msp430.h> 
#include <stdio.h>
#include <stdint.h>
#define ON 2239 //Toggle period for LEDs. Can make this asynchronous from master.
volatile uint8_t RxData; //Control byte from master

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   //Stop watchdog timer
    PM5CTL0 &= ~LOCKLPM5; //For brownout reset
    P1DIR |= BIT0; //Red LED on Launchpad
    P6DIR |= BIT6; //Green LED on Launchpad
    P1SEL0 |= BIT2 + BIT3; //Set I2C pins; P1.2 UCB0SDA; P1.3 UCB0SCL
    P1OUT &= ~BIT0; //Turn off LEDs
    P6OUT &= ~BIT6;

    CSCTL4 |= SELA__VLOCLK;  //Set ACLK to VLO

    //Setup I2C
    UCB0CTLW0 = UCSWRST;                      // Software reset enabled
    UCB0CTLW0 |= UCMODE_3 + UCSYNC;           // I2C mode, sync mode (Do not set clock in slave mode)
    UCB0I2COA0 = 0x77 | UCOAEN;               // Slave address is 0x77; enable it
    UCB0CTLW0 &= ~UCSWRST;                    // Clear reset register
    UCB0IE |= UCRXIE0;                         //Enable I2C receive interrupt

    //Configure Timer B: MC_1 to count up to TB0CCR0 continuously; set to ACLK (VLO) and divide it by 4
    TB0CTL |= MC_1 + TBSSEL_1;
    TB0EX0 |= TBIDEX_3;
    TB0CCTL0 |= CCIE; //Enable timer interrupt

    _BIS_SR(GIE); //Enable global interrupts.
    RxData = 0;
   //Loop forever
    while(1){
        TB0CCR0 = ON; //LED illumination period
        TB0CTL |= TBCLR; //Clear the timer counter
        LPM3; //Wait for timer or master interrupt in low power mode LPM3

        /* if/else executes immediately after interrupt. If new control byte received,
        switch the toggled LED; otherwise continue to toggle same LED on and off with period set by ON. */
        if (RxData == 0x01) P6OUT ^= BIT6; //Toggle green LED
        else P1OUT ^= BIT0; //Toggle red LED
        }
}

#pragma vector=TIMER0_B0_VECTOR //This vector name is in header file
 __interrupt void Timer_B (void)
{
    LPM3_EXIT;
}

#pragma vector = USCI_B0_VECTOR //This vector name is in header file
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
    case USCI_I2C_UCRXIFG0:                 // Vector 24: RXIFG0
                            RxData = UCB0RXBUF;
                            LPM3_EXIT;
                            break;
    case USCI_I2C_UCTXIFG0: break;             // Vector 26: TXIFG0
    case USCI_I2C_UCBCNTIFG: break;         // Vector 28: BCNTIFG
    case USCI_I2C_UCCLTOIFG: break;         // Vector 30: clock low timeout
    case USCI_I2C_UCBIT9IFG: break;         // Vector 32: 9th bit
    default: break;
  }
}


