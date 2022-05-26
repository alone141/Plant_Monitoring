#include <msp430.h> 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define ONE_WIRE_PIN BIT3
#define ONE_WIRE_IN P2IN
#define ONE_WIRE_OUT P2OUT
#define ONE_WIRE_DIR P2DIR

float get_temp(void);
void reset_12B20(void);
void send_12B20(char data);
unsigned int read_12B20(void);
void configureClocks();
void delay_ms(unsigned int ms);
void delay_us(unsigned int us);
volatile float soil_temp = 0;

volatile int temp[50];
volatile int diff[50];
volatile unsigned int i=0;
volatile unsigned int j=0;
char th_char[5];
char tl_char[5];
char hh_char[5];
char hl_char[5];
char soilh_char[5];
char soilt_char[5];
volatile int hh = 0;
volatile int hl = 0;
volatile int th = 0;
volatile int tl = 0;
volatile int check = 0;
volatile int checksum = 0;
volatile int dataok;
char temperature[] = "Temperature is: ";
char dot[] = ".";
char celcius[] = " degrees Celcius  ";
char humidity[] = "Humidity is: ";
char percent[] = " %\r\n";
void ser_output(char *str);

volatile unsigned int soil_humidity = 0;

void configureClocks();
void UARTInit(void);
void DHT11init();
void ADCinit(void);

void main(void)
{
    configureClocks();      //Initialize clock
    UARTInit();             //Initialize UART
    DHT11init();            //Initialize DHT11
    ADCinit();              //Initialize ADC
    _enable_interrupts();   //Enable interrupts

    while (1){
        //Split and categorize DHT11 readings
        if (i>=40){
            for (j = 1; j <= 8; j++){
                if (diff[j] >= 110)
                    hh |= (0x01 << (8-j));
                }
            for (j = 9; j <= 16; j++){
                if (diff[j] >= 110)
                    hl |= (0x01 << (16-j));
            }
            for (j = 17; j <= 24; j++){
                if (diff[j] >= 110)
                    th |= (0x01 << (24-j));
            }
            for (j = 25; j <= 32; j++){
                if (diff[j] >= 110)
                    tl |= (0x01 << (32-j));
            }
            for (j = 33; j<=40; j++){
                if (diff[j] >= 110)
                    checksum |= (0x01 << (40-j));
            }
            check=hh+hl+th+tl;
            if (check == checksum)
                dataok = 1;
            else
                dataok = 0;


            ltoa(th,th_char,10); //Air Temp to string
            ltoa(tl,tl_char,10);
            ltoa(hh,hh_char,10); //Air Humidity to string
            ltoa(hl,hl_char,10);


            __delay_cycles(1000000);
            //WDTCTL = WDT_MRST_0_064;
        }

        ADC10CTL0 |= ENC + ADC10SC;             // Starting ADC
        soil_humidity = ADC10MEM/1023;

        soil_temp = get_temp();
        ltoa(soil_humidity,soilh_char,10);  // Soil humidity to string
        ltoa(soil_temp,soilt_char,10); // Soil temp to string

        ser_output(" ");
        ser_output(th_char);
        ser_output(" ");
        ser_output(hh_char);
        ser_output(" ");
        ser_output(soilt_char);
        ser_output(" ");
        ser_output(soilh_char);
        ser_output("\n");


    }
}
#pragma vector = TIMER1_A1_VECTOR
__interrupt void Timer_A1(void){
        temp[i] = TA1CCR2;
        i += 1;
        TA1CCTL2 &= ~CCIFG ;
        if (i>=2) diff[i-1]=temp[i-1]-temp[i-2];
}
void ser_output(char *str){
    while(*str != 0){
        while (!(IFG2&UCA0TXIFG));
        UCA0TXBUF = *str++;
    }
}
void configureClocks()
{
     WDTCTL = WDTPW + WDTHOLD;          // Stop WDT
     BCSCTL1 = CALBC1_1MHZ;
     DCOCTL = CALDCO_1MHZ;

 }

//Uart Ayarlari
void UARTInit(void){
    P1SEL = BIT1|BIT2;
    P1SEL2 = BIT1|BIT2;
    UCA0CTL1 |= UCSWRST+UCSSEL_2;
    UCA0BR0 = 52;
    UCA0BR1 = 0;
    UCA0MCTL = UCBRS_0;
    UCA0CTL1 &= ~UCSWRST;
}
void DHT11init(){
    __delay_cycles(2000000); //We need to have a delay according to DHT11 datasheet
       P2DIR |= BIT4;
       P2OUT &= ~BIT4;
       __delay_cycles(20000);
       P2OUT |= BIT4;
       __delay_cycles(20);
       P2DIR &= ~BIT4;
       P2SEL |= BIT4;
       TA1CTL = TASSEL_2|MC_2 ;
       TA1CCTL2 = CAP | CCIE | CCIS_0 | CM_2 | SCS ; //Capture mode timer
}

void ADCinit(void)
{
  /* Configure ADC  Channel */
  ADC10CTL1 = INCH_5 + ADC10DIV_3 ;         // Channel 5 ADC10CLK/4
  ADC10CTL0 = SREF_0 + ADC10SHT_3 + ADC10ON;  //Referance voltage
  ADC10AE0 |= BIT5;                         //P1.5 ADC
}


void delay_us(unsigned int us)
{
    while (us)
    {
        __delay_cycles(1); // 1 for 1 Mhz set 16 for 16 MHz
        us--;
    }
}

void delay_ms(unsigned int ms)
{
    while (ms)
    {
        __delay_cycles(1000);
        ms--;
    }
}

float get_temp(void)
{
    unsigned int temp;
    reset_12B20();
    send_12B20(0xcc);   //send CCH,Skip ROM command
    send_12B20(0x44);
    delay_us(100);

    reset_12B20();
    send_12B20(0xcc);   //send CCH,Skip ROM command
    send_12B20(0xbe);

    temp = read_12B20();
    return((float)temp/8.0);
}

void reset_12B20(void)
{
    ONE_WIRE_DIR |=ONE_WIRE_PIN;
    ONE_WIRE_OUT &= ~ONE_WIRE_PIN;
    __delay_cycles(500);
    ONE_WIRE_OUT |=ONE_WIRE_PIN;
    ONE_WIRE_DIR &= ~ONE_WIRE_PIN;
    __delay_cycles(500);
}

void send_12B20(char data)
{
    char i;

    for(i=8;i>0;i--)
    {
        ONE_WIRE_DIR |=ONE_WIRE_PIN;
        ONE_WIRE_OUT &= ~ONE_WIRE_PIN;
        __delay_cycles(2);
        if(data & 0x01)
        {
            ONE_WIRE_OUT |= ONE_WIRE_PIN;
        }
        __delay_cycles(60);
        ONE_WIRE_OUT |= ONE_WIRE_PIN;
        ONE_WIRE_DIR &= ~ONE_WIRE_PIN;
        data >>=1;
    }
}

unsigned int read_12B20()
{
    char i;
    unsigned int data=0;

    for(i=16;i>0;i--)
    {
        ONE_WIRE_DIR |= ONE_WIRE_PIN;
        ONE_WIRE_OUT &= ~ONE_WIRE_PIN;
        __delay_cycles(2);
        ONE_WIRE_OUT |=ONE_WIRE_PIN;
        ONE_WIRE_DIR &= ~ONE_WIRE_PIN;
        __delay_cycles(8);
        if(ONE_WIRE_IN & ONE_WIRE_PIN)
        {
            data |=0x8000;
        }
        data>>=1;
        __delay_cycles(120);
    }
    return(data);
}

