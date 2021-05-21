/*I2C demo program to exchange 10 bytes of data between master and slave on I2C. Master is an MSP430FR5969 Launchpad.
  Slave is a MSP430FR2355 Launchpad. Master toggles the 3rd byte as 0x03 and 0x07 and sends to slave
  on alternate cycles. The master immediately requests slave to send the data back. If master receives 3rd byte
  as 0x03, flash green LED; otherwise flash red. Slave is polled at rate set by the VLO timer of master on SMCLK.
  Master/slave both run on timed loops but are asynchronous. Receive and transmission interrupts on I2C.
  Use low power modes LPM0 and LPM3. This is the MASTER code.
    P1.6  UCB0SDA with 10k pullup
    P1.7  UCB0SCL with 10k pullup
  */
#include <msp430.h> 
#include <stdio.h>
#include <stdint.h>

# define PERIOD 20000 //Samping period. 10000 count is approximately 1 second; maximum is 65535
# define BLINK 500
volatile uint8_t RxCount, TxCount, RxData[10];
volatile uint8_t *PRxData, *PRxCount, *PTxData;   // Pointer to RX data
volatile uint8_t TxData[]={1,2,3,4,5,6,7,8,9,10};
volatile uint8_t Control_Byte = 0x01;

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
    TA0CCTL0 |= CCIE;
    TA0CTL |= MC_1 + TASSEL_1;

    // Configure the eUSCI_B0 module for I2C at 100 kHz
    UCB0CTLW0 |= UCSWRST;
    UCB0CTLW0 |=  UCSSEL__SMCLK + UCMST + UCSYNC + UCMODE_3; //Select SMCLK, master, receiver, synchronous, I2C
    UCB0BRW = 10;  //Divide SMCLK by 10 to get ~100 kHz
    UCB0I2CSA = 0x77; // FR2355 address
    UCB0CTLW0 &= ~UCSWRST; // Clear reset

    UCB0IE |= UCTXIE0 + UCRXIE0; //Enable I2C transmission and receive interrupts
    __enable_interrupt(); //Enable global interrupts.

    while(1)
    {
        TA0CCR0 = PERIOD; //Looping period with VLO
        LPM3;       //Wait in low power mode
        //Timeout
        Control_Byte ^= BIT0;
        PTxData = (uint8_t *)TxData; //Set pointer to start of TX array
        //Toggle the 3rd byte
        if (Control_Byte == 0x01) *(PTxData + 2) = 0x07;
        else *(PTxData + 2) = 0x03;

        //PTxData = (uint8_t *)TxData; //Reset pointer
        TxCount = 10; //Send all 10 bytes to slave
        UCB0CTLW0 |= UCTR + UCTXSTT; // Set to transmit and start
        LPM0;    // Remain in LPM0 until all data transmitted
        while (UCB0CTLW0 & UCTXSTP);  // Ensure stop condition got sent

        UCB0CTLW0 &= ~UCTR; //Set as receiver
        PRxData = (uint8_t *)RxData;    // Point to start of RX array
        RxCount = 10; //Read entire data register from slave
        UCB0CTLW0 |= UCTXSTT; //Start read
        LPM0; //Wait for I2C
        PRxData = (uint8_t *)RxData; //point to start of RX array

        //Test 3rd byte of received data. Blink one of the LEDs
        if(*(PRxData+2) == 0x03)  P1OUT |= BIT0; //Green
      //  if(RxCount == 0x00)  P1OUT ^= BIT0; //Green
        else P4OUT |= BIT6; //Red
        TA0CCR0 = BLINK;
        LPM3;
        P1OUT &= ~BIT0;
        P4OUT &= ~BIT6;
    }
}
#pragma vector=TIMER0_A0_VECTOR
 __interrupt void TIMER_A0 (void)
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
        case 8:   break;        // Vector 8: STPIFG
        case 10: break;         // Vector 10: RXIFG3
        case 12: break;         // Vector 12: TXIFG3
        case 14: break;         // Vector 14: RXIFG2
        case 16: break;         // Vector 16: TXIFG2
        case 18: break;         // Vector 18: RXIFG1
        case 20: break;         // Vector 20: TXIFG1
        case 22:                // Vector 22: RXIFG0
                        RxCount--;        // Decrement RX byte counter
                        if (RxCount) //Execute the following if counter not zero
                            {
                                *PRxData++ = UCB0RXBUF; // Move RX data to address PRxData
                                if (RxCount == 1)     // Only one byte left?
                                UCB0CTLW0 |= UCTXSTP;    // Generate I2C stop condition BEFORE last read
                            }
                        else
                            {
                                *PRxData = UCB0RXBUF;   // Move final RX data to PRxData(0)
                                LPM0_EXIT;             // Exit active CPU

                            }
                        break;

        case 24:                // Vector 24: TXIFG0
                        if (TxCount)      // Check if TX byte counter not empty
                            {
                                UCB0TXBUF = *PTxData++; // Load TX buffer
                                TxCount--;            // Decrement TX byte counter
                            }
                        else
                            {
                                UCB0CTL1 |= UCTXSTP; // I2C stop condition
                                UCB0IFG &= ~UCTXIFG0;  // Clear USCI_B0 TX int flag
                                LPM0_EXIT;      // Exit LPM0
                            }
                        break;
        case 26: break;        // Vector 26: BCNTIFG
        case 28: break;         // Vector 28: clock low timeout
        case 30: break;         // Vector 30: 9th bit
        default: break;
    }
}



