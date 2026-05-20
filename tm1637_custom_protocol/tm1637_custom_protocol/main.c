#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
// 오픈 드레인 방식으로 가자.

#define CLK_PIN 2
#define DIO_PIN 3
#define LED_PIN 5

unsigned char flag = 0;

const unsigned char digit_table[] = {
	0x3F, // 0
	0x06, // 1
	0x5B, // 2
	0x4F, // 3
	0x66, // 4
	0x6D, // 5
	0x7D, // 6
	0x07, // 7
	0x7F, // 8
	0x6F  // 9
};
// const 전역 변수라서 Flash 메모리 코드 영역에 저장됨.
// 그래서 Sram 용량을 아낄 수 있음.

int main(void){
	PORTD |= (1 << CLK_PIN);
	PORTD &= ~(1 << DIO_PIN); // 내부 풀업 비활성화.
	PORTB &= ~(1 << LED_PIN);
	DDRD |= (1 << CLK_PIN);
	DDRD &= ~(1 << DIO_PIN); // DIO 핀을 입력 모드로.
	DDRB |= (1 << LED_PIN);
	_delay_us(5); // 안정화 시간
	
	// start DIO high -> low로
	DDRD |= (1 << DIO_PIN); // DIO 핀을 출력 모드로. PORTD의 DIO 핀이 0이니깐 0V를 출력하게 됨.
	_delay_us(5); // TM1637이 start를 인지하고 준비할 시간을 줌.
	
	
	unsigned char cmd1 = 0x40; // 0b0100 0000
	
	for (unsigned char i = 0; i < 8; i++){ // 메모리 최적화를 위해 unsigned char로 함.
		PORTD &= ~(1 << CLK_PIN);
		// 해당 명령어가 1주기가 걸리든 2주기가 걸리든 다음 명령어 실행 때(주기가 끝나자마자 or 다음 주기 시작 edge)
		// 즉, 마지막 클럭 주기가 끝나는 시점부터 다음 주기의 rising edge까지 실제 값이 업데이트 되는거임.
		// 그래서 아래 문장인 if문이 시작하고 5ns 뒤에 MCU의 CLK 핀엔 0V가 걸릴거임.
		// 그래서 아직 if문이 연산되는 중이라서 dio가 먼저 바뀔일은 없음.
		// 그래서 delay는 안해도 됨.
		
		if((cmd1 >> i) & 0x01){
			DDRD &= ~(1 << DIO_PIN);
		}
		else{
			DDRD |= (1 << DIO_PIN);
		}
		_delay_us(5);
		// 외부 풀업 rc 지연 시간이 안정적으로 3us가 필요함.
		// 최소 low or high 최소 유지 시간은 충족해야 함.
		// 만약 DDRD의 DIO pin이 입력 모드가 되었다고 가정해보자. 그러면 외부풀업저항에 의해 서서히 5V가 될거임. 명령어가 끝나자마자.
		// 근데 delay가 없다고 하면 clk핀을 1로 바꾸는 코드가 실행됨.
		// RC 지연 계산법에 의해 3us가 필요한데 전압이 5V로 되기까지 그래서 clk를 1로 만드는 아래 코드가 실행 중일때도 서서히 오르고 있음.
		// 근데 clk을 1로 만드는 코드는 끝나고 5ns 후 바로 MCU가 해당 핀을 5V로 올림.
		// 왜냐면 clk선은 mcu가 vcc를 연결해 직접 밀어내서 외부 풀업 저항에 의해 자동으로 전압이 올라가는 dio 핀과는 속도가 다름.
		// 그리고 최소 clk low level 유지시간이 있음. 그래서 이러한 이유들로 인해 delay를 넣어준거임.
		
		PORTD |= (1 << CLK_PIN);
		// clk high로.
		_delay_us(5);
		// TM1637이 데이터를 업로드 하고 처리할 시간이 필요하므로 delay함.
		// 즉, 내부 트렌지스터가 데이터 소화 시간을 벌어줄려고 delay함.
	}
	// ACK 입력 받기.
	PORTD &= ~(1 << CLK_PIN); // ACK는 주기 시작 falling edge 때부터 바로 작동함.
	DDRD &= ~(1 << DIO_PIN); // 입력 모드로 바꿔 파악함.
	// CLK이 먼저 falling edge로 바뀌어 TM1637이 이제 GND랑 연결 되어서 해당 전선을 0V로 전압을 내릴려고 할거임.
	// 근데 위의 for문에서 DIO 값이 만약 출력 모드로 끝났을 경우 0V를 잡고 있을텐데 그렇게 되면 MCU랑 TM1637이 동시에 핀 제어권을 갖는거임.
	// 그치만 여기서 오픈 드레인 모드를 한 이유가 들어나는데 둘 다 0V이기 때문에 둘 다 DIO 전선을 0V로 누르려고 함.
	// 근데 그냥 양쪽 다 전하를 빼내려고 흡입하는 문을 열었을 뿐 어느 한 쪾이 5V 전하를 뿜어내고 있는게 아님.
	// 그냥 외부 풀업 저항을 통해 나오는 전하를 둘 다 흡수 할 뿐. 문젠 없음.
	// 만약 오픈 드레인을 안해서 DIO전선을 TM1637은 0V랑 연결되고 MCU랑은 5V랑 연결되었을 땐 즉, 한 쪽이 5V를 뿜어내고 다른 한 쪽이 0V로 강하게 잡아당길 때
	// 그땐 MCU -> TM1637로 엄청난 과전류(사이에 저항이 없으니깐)가 발생해 칩이 타버릴 수 있음.
	// 그래서 오픈 드레인 방식을 체택한거임.
	if (!((PIND >> DIO_PIN) & 0x01)){ // PIN을 바로 읽어도 상관 없는게 clk가 low가 되자마자 tm은 바로 gnd를 연결시킴.
		flag += 1; // 그래서 dio 핀을 작동시키는 명령어가 실행되고 5ns 뒤에 dio전선은 이미 0V가 되어 버림.
	}// 외부 신호가 들어오고 2주기 뒤에 pindx에 업데이트 되니깐 if문일땐 이미 pindx엔 tm이 전압을 낮춘 0V가 들어온 상태임.
	// 그래서 최종적으로 pindx 업데이트를 위한 delay는 생략해도 됨.
	_delay_us(5); // low level 최소 유지 시간을 위해
	
	PORTD |= (1 << CLK_PIN);
	_delay_us(5);
	
	// 10 주기
	PORTD &= ~(1 << CLK_PIN);
	DDRD |= (1 << DIO_PIN);
	// clk과 dio 사이에 delay는 아두이노 나노가 16MHz라 충분히 실행되고 실행되어서 안 넣음.
	// 즉, 두 핀 모두 외부 풀업 저항에 의존하는 게 아니라 MCU 내부 GND 스위치를 꽉 닫아서 직접 0V로 압착시키니깐 속도가 빨라 걱정할 필요가 없음.
	// 만약 초고속 MCU였다면 이런거 고려해서 delay 줘야 함.
	// 또한 만약 2개의 명령어 중 첫 번째 명령어가 내부 풀업을 사용하는 거라면 delay 주는게 안전함.
	_delay_us(5);
	PORTD |= (1 << CLK_PIN);
	_delay_us(5);
	
	// 11 주기
	DDRD &= ~(1 << DIO_PIN);
	_delay_us(5);
	
	// command2 시작
	DDRD |= (1 << DIO_PIN);
	_delay_us(5); // TM1637이 start 신호를 받고 준비할 시간을 줌.
	
	// 또한 mcu가 gnd로 내리는거라 5ns밖에 안 걸려 delay 안해도 됨.
	unsigned char cmd2 = 0xC0; // 0b1100 0000
	for(unsigned char i = 0; i < 8; i++){
		PORTD &= ~(1 << CLK_PIN); // clk을 low로 내려 dio 값을 변경하고 미리 준비할 수 있도록 함.
		if((cmd2 >> i) & 0x01){
			DDRD &= ~(1 << DIO_PIN);
		}
		else{
			DDRD |= (1 << DIO_PIN);
		}
		_delay_us(5); // low level 최소 시간을 위해 기다림.
		
		PORTD |= (1 << CLK_PIN);
		_delay_us(5); // TM1637이 dio 값을 처리할 시간을 줌.
	}
	
	// ACK
	PORTD &= ~(1 << CLK_PIN); // TP1637이 이제 gnd를 연결해 dio의 전압을 내릴거임.
	DDRD &= ~(1 << DIO_PIN); // 입력 모드
	if(!((PIND >> DIO_PIN) & 0x01)){
		flag += 1;
	}
	_delay_us(5); // low level 최소 유지 시간을 위해
	
	
	PORTD |= (1 << CLK_PIN);
	_delay_us(5);
	
	// Data1
	unsigned char data = digit_table[0];
	for(unsigned char i = 0; i < 8; i++){
		PORTD &= ~(1 << CLK_PIN);
		if((data >> i) & 0x01){
			DDRD &= ~(1 << DIO_PIN);
		}
		else{
			DDRD |= (1 << DIO_PIN);
		}
		_delay_us(5);
		
		PORTD |= (1 << CLK_PIN);
		_delay_us(5);
	}
	
	// ACK
	PORTD &= ~(1 << CLK_PIN);
	DDRD &= ~(1 << DIO_PIN);
	if(!((PIND >> DIO_PIN) & 0x01)){
		flag += 1;
	}
	_delay_us(5); // low level 최소 유지 시간을 위해
	
	PORTD |= (1 << CLK_PIN);
	_delay_us(5);
	
	
	// Data2
	data = digit_table[1];
	for(unsigned char i = 0; i < 8; i++){
		PORTD &= ~(1 << CLK_PIN);
		if((data >> i) & 0x01){
			DDRD &= ~(1 << DIO_PIN);
		}
		else{
			DDRD |= (1 << DIO_PIN);
		}
		_delay_us(5);
		
		PORTD |= (1 << CLK_PIN);
		_delay_us(5);
	}
	
	// ACK
	PORTD &= ~(1 << CLK_PIN);
	DDRD &= ~(1 << DIO_PIN);
	if(!((PIND >> DIO_PIN) & 0x01)){
		flag += 1;
	}
	_delay_us(5); // low level 최소 유지 시간을 위해
	
	PORTD |= (1 << CLK_PIN);
	_delay_us(5);
	
	// Data3
	data = digit_table[2];
	for(unsigned char i = 0; i < 8; i++){
		PORTD &= ~(1 << CLK_PIN);
		if((data >> i) & 0x01){
			DDRD &= ~(1 << DIO_PIN);
		}
		else{
			DDRD |= (1 << DIO_PIN);
		}
		_delay_us(5);
		
		PORTD |= (1 << CLK_PIN);
		_delay_us(5);
	}
	
	// ACK
	PORTD &= ~(1 << CLK_PIN);
	DDRD &= ~(1 << DIO_PIN);
	if(!((PIND >> DIO_PIN) & 0x01)){
		flag += 1;
	}
	_delay_us(5); // low level 최소 유지 시간을 위해
	
	PORTD |= (1 << CLK_PIN);
	_delay_us(5);
	
	// Data4
	data = digit_table[3];
	for(unsigned char i = 0; i < 8; i++){
		PORTD &= ~(1 << CLK_PIN);
		if((data >> i) & 0x01){
			DDRD &= ~(1 << DIO_PIN);
		}
		else{
			DDRD |= (1 << DIO_PIN);
		}
		_delay_us(5);
		
		PORTD |= (1 << CLK_PIN);
		_delay_us(5);
	}
	
	// ACK
	PORTD &= ~(1 << CLK_PIN);
	DDRD &= ~(1 << DIO_PIN);
	if(!((PIND >> DIO_PIN) & 0x01)){
		flag += 1;
	}
	_delay_us(5); // low level 최소 유지 시간을 위해
	
	PORTD |= (1 << CLK_PIN);
	_delay_us(5);
	
	PORTD &= ~(1 << CLK_PIN);
	DDRD |= (1 << DIO_PIN); // 이걸 안해주면 mcu와 tm 둘 다 입력 모드이기 때문에 외부 풀업 저항으로 인해 dio 핀 전압이 5V가 됨.
	_delay_us(5);
	
	PORTD |= (1 << CLK_PIN);
	_delay_us(5);
	
	DDRD &= ~(1 << DIO_PIN);
	_delay_us(5);
	
	// command3
	DDRD |= (1 << DIO_PIN);
	_delay_us(5);
	
	unsigned char cmd3 = 0x8F; 
	for(unsigned char i = 0; i < 8; i++){
		PORTD &= ~(1 << CLK_PIN); // clk을 low로 내려 dio 값을 변경하고 미리 준비할 수 있도록 함.
		if((cmd3 >> i) & 0x01){
			DDRD &= ~(1 << DIO_PIN);
		}
		else{
			DDRD |= (1 << DIO_PIN);
		}
		_delay_us(5); // low level 최소 시간을 위해 기다림.
		
		PORTD |= (1 << CLK_PIN);
		_delay_us(5); // TM1637이 dio 값을 처리할 시간을 줌.
	}
	
	// ACK
	PORTD &= ~(1 << CLK_PIN); // TP1637이 이제 gnd를 연결해 dio의 전압을 내릴거임.
	DDRD &= ~(1 << DIO_PIN); // 입력 모드
	if(!((PIND >> DIO_PIN) & 0x01)){
		flag += 1;
	}
	_delay_us(5); // low level 최소 유지 시간을 위해
	
	PORTD |= (1 << CLK_PIN);
	_delay_us(5);
	
	PORTD &= ~(1 << CLK_PIN);
	DDRD |= (1 << DIO_PIN);
	_delay_us(5);
	
	PORTD |= (1 << CLK_PIN);
	_delay_us(5);
	
	DDRD &= ~(1 << DIO_PIN);
	_delay_us(5);
	
	_delay_ms(5000);
	if(flag == 7){
		PORTB |= (1 << LED_PIN);
	}
	while(1){
	}
}


