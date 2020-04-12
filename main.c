/* MAIN.C file
 * 
 * Copyright (c) 2002-2005 STMicroelectronics
 */
#include "STM8S003K3.h"
#include "delay.h"

#define stop_cntr		TIM4_CR1 &= ~(1<<0)		//alias for enabling the timmer
#define start_cntr 	TIM4_CR1 |= (1<<0)		//alias for disabling the timmer

void timer4_init(void);
void gpio_init(void);
void trigger(void);
void counter_init(void);
void UART1_init(void);
void UART1_TX(int );
void get_val(void);
void UART1_TX_string(unsigned char *);
unsigned int time_var = 0, time_cal = 0, ref_height = 0 ;
unsigned int height = 0, avg_dist = 0 ;
int flag = 1;
int dist = 0;
int i;

/*
Intitiates the timer4 (8 bit timer) as counter with internal
clock at 1 MHZ for calculating the pulse time
*/
void timer4_init(void)
{
	TIM4_CR1 = 0b00000100;
	TIM4_SR = 0x00;
	TIM4_PSCR = 0x04;
	TIM4_ARR = 0xff;
	TIM4_IER = 0x01;
}

/*
initialises the function of GPIO pins required 
for the operation
*/
void gpio_init(void)
{
	//pb1 output
	PB_DDR |= (1<<1);
	PB_CR1 |= (1<<1);
	PB_CR2 &= ~(1<<1);
	
	//pb5 output
	PB_DDR |= (1<<5);
	PB_CR1 |= (1<<5);
	PB_CR2 &= ~(1<<5);
	
	//pb6 output
	PB_DDR |= (1<<6);
	PB_CR1 |= (1<<6);
	PB_CR2 &= ~(1<<6);
	
	//pb7 output
	PB_DDR |= (1<<7);
	PB_CR1 |= (1<<7);
	PB_CR2 &= ~(1<<7);
	
	//pb0 output
	PB_DDR |= (1<<0);
	PB_CR1 |= (1<<0);
	PB_CR2 &= ~(1<<0);
	
	//pb3 output
	PB_DDR |= (1<<3);
	PB_CR1 |= (1<<3);
	PB_CR2 &= ~(1<<3);
	
	/*pb2 input ecco pin with enabled interrupt with rising 
	falling edge*/
	PB_DDR &= ~(1<<2);
	PB_CR1 &= ~(1<<2);
	PB_CR2 |= (1<<2);
	EXTI_CR1 = 0x0c;
	
	//pd5 output for uart tx
	PD_DDR |= (1<<5);
	PD_CR1 |= (1<<5);
	PD_CR2 &= ~(1<<5);
	
	/*//pb4 input for ref and height
	PB_DDR &= ~(1<<4);
	PB_CR1 |= (1<<4);
	PB_CR2 &= ~(1<<4);*/
	
	PB_ODR |= (1<<0);		//pin 0 vcc
	PB_ODR &= ~(1<<3);	//pin 3 gnd 
	PB_ODR &= ~(1<<5);	//pin 5 gnd	
	PB_ODR &= ~(1<<6);	//pin 6 gnd
	PB_ODR |= (1<<7);		//pin 7 vcc
}

/*
Interupt Service Routine(ISR)
A interrupt is generated when a rising or falling edge is
received at pin 2 of port B
*/
@far @interrupt void inout(void)
{
	/*
	if the counter is empty that is a rising edge is detected 
	and counter is initialised
	*/
	if(TIM4_CNTR  == 0)
	{
		start_cntr;	
		return;
	}
	/*
	When not empty therefore a falling edge is detected and
	and the distance is calculated and stored in dist
	*/
	else
	{
		stop_cntr;
		time_cal = (time_var * 255) + TIM4_CNTR;
		dist = (time_cal/2) * 0.354;
		delay_microsec(500000);
		flag = 0;
		return;
	}
	
}

/*
Interrupt Service Routine for Timer that is generated 
whenever the timer reaches to maximum value a variable
time_var is incremented by on and is used for calculation of
time
*/
@far @interrupt void time(void)
{
	counter_init();
	time_var++; 
}

//Initialises the counter
void counter_init()
{
	TIM4_SR &= ~(1<<0);
	TIM4_CNTR = 0x00;
}

/*Sends a high pulse on trigger pin of the ultrasonic sensor 
for 10 microseconds*/ 
void trigger()
{
	
	PB_ODR &= ~(1<<1);
	delay_microsec(5);
	PB_ODR |= (1<<1); 
	delay_microsec(10);
	PB_ODR &= ~(1<<1);
}

//Initialises uart1 to transmit
void UART1_init()
{
	UART1_SR = 0x00;
	UART1_BRR1 = 0x68;		//baud rate 9600
	UART1_BRR2 = 0x03;
	UART1_CR1 = 0x00;
	UART1_CR2 = 0x08;
	UART1_CR3 = 0x00;
}

/* This function triggers the sensor and returns 
the calculated distance in cm*/
void get_val()
{
	flag = 1;
	time_var = 0;
	dist = 0;
	counter_init();
	trigger();
	_asm("rim\n");
	while(flag);
	_asm("sim\n");
}

/* integer data is converted to ascii 
value and then transmitted using uart*/

void UART1_TX(int data)
{
	int  temp = 10;
	int i = 1;
	char num[5];
	while(data/10)
	{
		num[i++] = data % 10;
	}
	num[i] = '\0';
	while(i != 0)
	{
		UART1_DR = (num[i] + 48) ;
		while(!(UART1_SR & (1<<6)));
		i--;
	}
}

//This function is used to send the strings using uart
void UART1_TX_string(unsigned char *data)
{
	while(*data!='\0')
	{
		UART1_DR = *data;
		while(!(UART1_SR & (1<<6)));
		data++;
	}
}

main()
{
	CLK_CKDIVR = 0x01;	// CPU clock at 8MHz
	gpio_init();
	timer2_init(); 			//for the function of delay
	timer4_init();			//for calculating the pulse time
	UART1_init();
	//callibration
	for(i = 0; i <5 ; i++)
	{
		get_val();
		delay_microsec(10000);
	}
	
	//calculating the initial refrence height
	for(i = 0; i<10 ;i++)
	{
		get_val();
		delay_microsec(5000);
		avg_dist = avg_dist + dist;
	}
	ref_height = avg_dist / 10;
	
	//Measured height is trasmitted using uart and can be received on phone
	while(1)
	{
		avg_dist  = height = 0;
		for(i = 0; i <10 ; i++)
		{
			get_val();
			delay_microsec(5000);
			avg_dist = avg_dist + dist; 
		}
		avg_dist = avg_dist / 10;
		height = ref_height - avg_dist;
		if(height < 0)
		{
			height = 0;
		}
		UART1_TX_string("REF. Height: ");
		UART1_TX(ref_height);
		UART1_DR = '\n';
		UART1_TX_string("Height: ");
		UART1_TX(height);
	}
}
