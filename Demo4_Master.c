/*I2C demo program to send multiple bytes of data from from master to slave. Master is an MSP430FR5969 Launchpad;
  Slave is a MSP430FR2355 Launchpad. If the master sends 4 data bytes, toggle green LED;
  otherwise toggle red LED. Slave is polled at rate set by the VLO timer. I2C is on SMCLK.
  Use low power modes LPM0 and LPM3. Use the stop and transmission interrupts on I2C. This is the MASTER code.
  */
#include <msp430.h> 
#include <stdio.h>
#include <stdint.h>
/*
P1.6  UCB0SDA with 10k pullup
P1.7  UCB0SCL with 10k pullup
 */
# define PERIOD 20000 //Samping period. 10000 count is approximately 1 second; maximum is 65535
volatile uint8_t Control_Byte, Data[] = {1,2,3,4}; //Dummy data array, only interested in size
volatile uint8_t *PTxData;   // Pointer to TX data

void main(void) {

    WDTCTL = WDTPW | WDTHOLD;   //Stop watchdog timer

    PM5CTL0 &= ~LOCKLPM5; //Unlocks GPIO pins at power-up
    P1DIR |= BIT0 + BIT1 + BIT2 + BIT3 + BIT4 + BIT5;
    P1SEL1 |= BIT6 + BIT7; //Setup I2C on UCB0
    P1OUT &= ~BIT0; //green LED off
    P4DIR |= BIT0 + BIT1 + BIT2 + BIT3 + BIT4 + BIT5 + BIT6 + BIT7;
    P4OUT &= ~BIT6; //red LED off

    CSCTL0 = CSKEY; //Password to unlock the clock registers
    //Default frequency ~ 10 kHz
    CSCTL2 |= SELA__VLOCLK;  //Set ACLK to VLO
    CSCTL0_H = 0xFF; //Re-lock the clock registers

    //Enable the timer interrupt, MC_1 to count up to TA0CCR0, Timer A set to ACLK (VLO)
    TA0CCTL0 = CCIE;
    TA0CTL |= MC_1 + TASSEL_1;

    // Configure the eUSCI_B0 module for I2C at 100 kHz
    UCB0CTLW0 |= UCSWRST;
    UCB0CTLW0 |=  UCSSEL__SMCLK + UCMST + UCSYNC + UCTR + UCMODE_3; //Select SMCLK, master, transmitter, synchronous, I2C
    UCB0CTLW1 |=  UCASTP_2; //Automatic stop generation when byte counter is hit
    UCB0BRW = 10;  //Divide SMCLK by 10 to get ~100 kHz
    UCB0I2CSA = 0x77; // FR2355 slave address
    UCB0CTLW0 &= ~UCSWRST; // Clear reset

    UCB0IE |= UCTXIE0 + UCSTPIE; //Enable I2C transmission and stop interrupts
    _BIS_SR(GIE); //Enable global interrupts

    while(1)
    {
        TA0CCR0 = PERIOD; //Looping period with VLO
        LPM3;       //Wait in low power mode
        //Timeout
        Control_Byte ^= BIT0;
        if (Control_Byte==0x01){
            UCB0TBCNT = 0x01; //Send 1 byte. Do not set UCSWRST bit. This is contrary to data sheet!
            P4OUT |= BIT6; //Turn on red LED on master
        }
        else {
            UCB0TBCNT = 0x04; //Send 4 bytes. Do not set UCSWRST bit. This is contrary to data sheet!
            P1OUT |= BIT0; //Turn on green LED on master
        }
        PTxData = (uint8_t *)Data; //Dummy data array
        UCB0CTL1 |= UCTXSTT;         // Set start bit
        LPM0; //Wait for I2C operations in LPM0
        while (UCB0CTL1 & UCTXSTP); // Ensure stop condition sent
        PTxData = 0; //Reset pointer
        P1OUT &= ~BIT0; //LEDs off
        P4OUT &= ~BIT6; 
    }
}
#pragma vector=TIMER0_A0_VECTOR
 __interrupt void timerfoo (void)
{
    LPM3_EXIT;
}

#pragma vector = USCI_B0_VECTOR
__interrupt void USCI_B0_ISR(void)
{
    switch(__even_in_range(UCB0IV,30))
    {
        case 0: break;         // Vector 0: No interrupts
        case 2: break;         // Vector 2: ALIFG
        case 4: break;         // Vector 4: NACKIFG
        case 6: break;         // Vector 6: STTIFG
        case 8:                 // Vector 8: STPIFG
            LPM0_EXIT;          // Exit LPM0
            break;
        case 10: break;         // Vector 10: RXIFG3
        case 12: break;         // Vector 12: TXIFG3
        case 14: break;         // Vector 14: RXIFG2
        case 16: break;         // Vector 16: TXIFG2
        case 18: break;         // Vector 18: RXIFG1
        case 20: break;         // Vector 20: TXIFG1
        case 22: break;         // Vector 22: RXIFG0
        case 24:                // Vector 24: TXIFG0
           UCB0TXBUF = *PTxData++; // Load TX buffer
            break;
        case 26: break;        // Vector 26: BCNTIFG
        case 28: break;         // Vector 28: clock low timeout
        case 30: break;         // Vector 30: 9th bit
        default: break;
    }
}



