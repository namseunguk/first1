#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

void UART_Transmit(unsigned char data);
unsigned char Uart_Receive(void);
void Uart1_Trans_Num(int num);

volatile int g_mode = 0;
volatile int adc[8] = {0,};
volatile int normalization[8] = {0,};
volatile int adc_max[8] = {0,};
volatile int adc_min[8] = {1024,};
volatile int line[8] = {0,};//0은 흰색 1은 검은색이다!!
volatile int threshold = 40;


ISR(INT0_vect) {
	g_mode = (g_mode+1)%2;
}

ISR(INT1_vect) {
	g_mode = (g_mode+1)%2;
}

int main(void) {
	
	int i = 0;
	
	DDRA = 0xFF;//LED
	DDRB = 0xFF;//PWM
	DDRD = 0x08;//UART TX는 출력 UART RX와 외부 인터럽트는 입력
	DDRE = 0x0F;//MOTOR DIRECTION //모터 하나당 10 또는 01이 들어감 서로 반대 방향,, 00,11은 미작동
	PORTE = 0b00001010;
	DDRF = 0x00;//ADC는 당근빳따 입력
	
	PORTA = 0xFF;
	
	EICRA = (1<<ISC11) | (1<<ISC01);
	EIMSK = (1<<INT0) | (1<<INT1);
	
	//20kHz 주파수로 맹그는 FAST PWM
	TCCR1A = (1<<COM1A1) | (1<<COM1B1) | (1<<WGM11);
	TCCR1B = (1<<WGM13) | (1<<WGM12) | (1<<CS00);//PRESCALE(분주비) = 1
	ICR1 = 730;
	OCR1B = 730;
	
	UCSR1A = 0x00;
	UCSR1B = (1<<TXEN1);
	UCSR1C = (1<<UCSZ11) | (1<<UCSZ10);
	
	UBRR1L = 8;//통신속도 115200bps
	
	ADMUX = 0x00;
	ADCSRA = (1<<ADEN) | (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);
	
	sei();
	while(1){
		for(i = 0; i < 8; i++) {
			ADMUX = i;
			ADCSRA |= (1<<ADSC);
			while(!(ADCSRA & (1<<ADIF)));
			adc[i] = ADC;
		}
		
		if(g_mode == 1) {//정규화 모드
			PORTA = 0x00;
			OCR1A = 0;
			OCR1B = 0;
			for(i = 0; i < 8; i++) {
				if(adc[i] < adc_min[i]) adc_min[i] = adc[i];
				else if(adc[i] > adc_max[i]) adc_max[i] = adc[i];
				UART_Transmit(i+48);
				UART_Transmit(' ');
				Uart1_Trans_Num(adc[i]);
				UART_Transmit(' ');
			}
			UART_Transmit('\n');
			UART_Transmit('\r');
		}
		
		else {//배틀로봇 구동 모드
			PORTA=0xFF;
			OCR1A = 730;
			OCR1B = 730;
			PORTE=0b00001010;
			for(i = 0; i < 8; i++) {
				normalization[i] = (double)(adc[i] - adc_min[i])/(adc_max[i] - adc_min[i]) * 100;
				if(normalization[i] < threshold) line[i] = 1;b
				else line[i] = 0;
				UART_Transmit(i+48);
				UART_Transmit(' ');
				Uart1_Trans_Num(normalization[i]);
				UART_Transmit(' ');
				Uart1_Trans_Num(line[i]);
				UART_Transmit(' ');
			}
			UART_Transmit('\n');
			UART_Transmit('\r');
			
			if(line[0] == 1) {
				OCR1A = 750;
				OCR1B = 750;
				PORTE = 0b00000101;
				_delay_ms(400);
				OCR1A = 740;
				OCR1B = 740;
				PORTE = 0b00001001;
				_delay_ms(350);
				OCR1A = 730;
				OCR1B = 730;
				PORTE = 0b00001010;
			}
			else if((line[1] == 1)) {
				OCR1A = 740;
				OCR1B = 740;
				PORTE = 0b00000101;
				_delay_ms(300);
				OCR1A = 750;
				OCR1B = 750;
				PORTE = 0b00001001;
				_delay_ms(390);
				OCR1A = 730;
				OCR1B = 730;
				PORTE = 0b00001010;
			}
			
			else if((line[7] == 1)) {
				OCR1A = 740;
				OCR1B = 740;
				PORTE = 0b00000101;
				_delay_ms(300);
				OCR1A = 750;
				OCR1B = 750;
				PORTE = 0b00000110;
				_delay_ms(390);
				OCR1A = 730;
				OCR1B = 730;
				PORTE = 0b00001010;
			}
			
			
			
			
		}
		
	}
	return 0;
}

void UART_Transmit(unsigned char data)
{
	while (!( UCSR1A & (1<<UDRE1)));
	UDR1 = data;
}

unsigned char Uart_Receive(void) {
	while(!(UCSR1A & (1<<RXC1)));
	return UDR1;//받은 데이터를 리턴
}

void Uart1_Trans_Num(int num) {
	//UART_Trasnmit은 문자를 보내기 때문에 숫자를 그대로 쓰면 엉뚱한 값이 송신된다.
	//따라서 아스키코드 표에 맞게 송신해 주기 위하여 +48을 해주면 해당 숫자에 대응하는 아스키 코드로 전송한다.
	if(num < 0) {//음수를 입력받을 경우에는 '-'를 출력한 뒤 양수로 변환하고 계산한다.
		UART_Transmit('-');
		num *= -1;
	}
	num = (num%10000);//나머지 연산으로 다섯 번째 자리수를 삭제.
	UART_Transmit(num/1000 +48);//네 번째 자리수 송신 나누기 연산으로 네 번째 자리 수만 취한다.
	num = (num%1000);//나머지 연산으로 네 번째 자리수를 삭제.
	UART_Transmit(num/100 +48);//두 번째 자리수를 송신.
	num = num%100;//나머지 연산으로 세 번째 자리수를 삭제.
	UART_Transmit(num/10 +48);//두 번째 자리수를 송신.
	num = num%10;//나머지 연산으로 두 번째 자리수를 삭제.
	UART_Transmit(num +48);//첫 번째 자리수를 송신.
}