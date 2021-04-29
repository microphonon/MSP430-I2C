/*I2C demo with MSP430FR2355 Launchpad as SLAVE. Read single control byte from master.
 If the control byte is 0x01 blink green LED, otherwise blink red LED. The LED duration is set by
the Timer_B with ACLK set to VLO. Polling is done by master which also supplies the SCL clock.
P1.2 UCB0SDA
P1.3 UCB0SCL
This is the SLAVE code.
*/
#include <msp430.h> 
#include <stdio.h>
#include <stdint.h>
#define ON 5000
volatile uint8_t RxData;

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   //Stop watchdog timer
    PM5CTL0 &= ~LOCKLPM5; //For brownout reset
    P1DIR |= BIT0; //Red LED on Launchpad
    P6DIR |= BIT6; //Green LED on Launchpad
    P1SEL0 |= BIT2 + BIT3; //Set I2C pins; P1.2 UCB0SDA; P1.3 UCB0SCL

    CSCTL4 |= SELA__VLOCLK;  //Set ACLK to VLO

    //Setup I2C
    UCB0CTLW0 = UCSWRST;                      // Software reset enabled
    UCB0CTLW0 |= UCMODE_3 + UCSYNC;           // I2C mode, sync mode (Do not set clock in slave mode)
    UCB0I2COA0 = 0x77 | UCOAEN;               // Slave address is 0x77; enable it
    UCB0CTLW0 &= ~UCSWRST;                    // Clear reset register

    //Enable the Timer B interrupt, MC_1 to count up to TB0CCR0, set to ACLK (VLO) and divide it by 4
    TB0CTL |= MC_1 + TBSSEL_1;
    TB0EX0 |= TBIDEX_3;
    P1OUT &= ~BIT0; //Turn off LEDs
    P6OUT &= ~BIT6;

    _BIS_SR(GIE); //Enable global interrupts.
    RxData = 0;

    while(1){
        //This loop is timed by the master on the I2C bus
        UCB0IE |= UCRXIE0; //Enable receive interrupt
        LPM4; //Wait in low-power mode for master interrupt
        UCB0IE &= ~UCRXIE0; //Wakeup. Disable I2C receive interrupt

        if (RxData == 0x01) P6OUT |= BIT6; //Green LED
        else P1OUT |= BIT0; //Red LED

        TB0CCTL0 |= CCIE; //Enable timer interrupt
        TB0CCR0 = ON; //LED illumination time
        TB0CTL |= TBCLR; //Clear the timer counter
        LPM3; //Wait for timer in low power mode
        TB0CCTL0 &= ~CCIE; //Disable timer interrupt
        P1OUT &= ~BIT0; //Turn off LEDs
        P6OUT &= ~BIT6;
        }
}

#pragma vector=TIMER0_B0_VECTOR //This vector name is in header file
 __interrupt void Timer_B (void) //The name of this interrupt (Timer_B) is arbitrary
{
    LPM3_EXIT;
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
    case USCI_I2C_UCRXIFG0:                 // Vector 24: RXIFG0
                            RxData = UCB0RXBUF;
                            LPM4_EXIT;
                            break;
    case USCI_I2C_UCTXIFG0: break;             // Vector 26: TXIFG0
    case USCI_I2C_UCBCNTIFG: break;         // Vector 28: BCNTIFG
    case USCI_I2C_UCCLTOIFG: break;         // Vector 30: clock low timeout
    case USCI_I2C_UCBIT9IFG: break;         // Vector 32: 9th bit
    default: break;
  }
}


