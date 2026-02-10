#include "include/utils.h"
#include "include/peripherals/mini_uart.h"
#include "include/peripherals/gpio.h"

void uart_send ( char c )
{
	while(1) {
		if(get32(AUX_MU_LSR_REG)&0x20) 
			break;
	}
	put32(AUX_MU_IO_REG,c);
}

char uart_recv ( void )
{
	while(1) {
		if(get32(AUX_MU_LSR_REG)&0x01) 
			break;
	}
	return(get32(AUX_MU_IO_REG)&0xFF);
}

void uart_send_string(char* str)
{
	for (int i = 0; str[i] != '\0'; i ++) {
		uart_send((char)str[i]);
	}
}

void uart_init ( void )
{
	unsigned int selector;

	selector = get32(GPFSEL1);
	selector &= ~(7<<12);                   // clean gpio14
	selector |= 2<<12;                      // set alt5 for gpio14
	selector &= ~(7<<15);                   // clean gpio15
	selector |= 2<<15;                      // set alt5 for gpio15
	put32(GPFSEL1,selector);

	put32(GPPUD,0);
	delay(150);
	put32(GPPUDCLK0,(1<<14)|(1<<15));
	delay(150);
	put32(GPPUDCLK0,0);

	put32(AUX_ENABLES,1);                   //Enable mini uart (this also enables access to its registers)
	put32(AUX_MU_CNTL_REG,0);               //Disable auto flow control and disable receiver and transmitter (for now)
	put32(AUX_MU_IER_REG,0);                //Disable receive and transmit interrupts
	put32(AUX_MU_LCR_REG,3);                //Enable 8 bit mode
	put32(AUX_MU_MCR_REG,0);                //Set RTS line to be always high
	put32(AUX_MU_BAUD_REG,270);             //Set baud rate to 115200

	put32(AUX_MU_CNTL_REG,3);               //Finally, enable transmitter and receiver
}

/*
	여기서 우리는 2가지의 functions을 사용한다.
	1. put32
	2. get32
	이 두 함수는 매우 간단하다. 32-bit register로부터 특정 값을 읽고 쓴다.
	이 함수들은 utils.c에 선언되어 있으며 utils.S에 선언되어 있다.

	GPIO alternative function selection

	먼저, GPIO pins를 activate해야 한다. 대부분의 pin은 각기 다른 devices에 의해 사용 가능하다.
	따라서 어떠한 특정 pin을 사용할 때, 우리는 그 pin의 alternative function을 선택해야 한다.

	alternative function이란 무엇인가?
	Raspberry Pi의 많은 GPIO pin은 한 가지 용도만 있는 게 아니라,
	1. 그냥 GPIO 입력/출력으로도 사용하고
	2. UART. SPI, I2C 같은 주변장치 signal line으로도 사용될 수 있다.
	그래서 SoC 안에 pin 하나를 어느 장치의 signal line에 연결할지 선택하는 multiplexer(MUX)가 있다.
	이 선택값이 바로 ALT0 ~ ALT5이다. 이 숫자는 각각의 pin에 부여될 수 있는 0부터 5까지의 숫자로 
	어떠한 devilce가 그 pin에 연결되었는지를 나타낸다. 

	즉, GPIO pin의 alternative function을 설정함으로써 우리는
	그 pin이 단순한 GPIO 입력/출력으로 사용될지,
	아니면 특정 주변장치의 signal line으로 사용될지를 결정 할 수 있다.

	available GPIO alternative functions (ALT0~ALT5):

	Legend: "-" = blank / none
	+------+-----+------------+--------------+----------+------------------+------------+--------+
	| GPIO |Pull | ALT0       | ALT1         | ALT2     | ALT3             | ALT4       | ALT5   |
	+------+-----+------------+--------------+----------+------------------+------------+--------+
	|GPIO0 |High | SDA0       | SA5          |<reserved>| -                | -          | -      |
	|GPIO1 |High | SCL0       | SA4          |<reserved>| -                | -          | -      |
	|GPIO2 |High | SDA1       | SA3          |<reserved>| -                | -          | -      |
	|GPIO3 |High | SCL1       | SA2          |<reserved>| -                | -          | -      |
	|GPIO4 |High | GPCLK0     | SA1          |<reserved>| -                | -          |ARM_TDI |
	|GPIO5 |High | GPCLK1     | SA0          |<reserved>| -                | -          |ARM_TDO |
	|GPIO6 |High | GPCLK2     | SOE_N / SE   |<reserved>| -                | -          |ARM_RTCK|
	|GPIO7 |High | SPI0_CE1_N | SWE_N / SRW_N|<reserved>| -                | -          | -      |
	|GPIO8 |High | SPI0_CE0_N | SD0          |<reserved>| -                | -          | -      |
	|GPIO9 |Low  | SPI0_MISO  | SD1          |<reserved>| -                | -          | -      |
	|GPIO10|Low  | SPI0_MOSI  | SD2          |<reserved>| -                | -          | -      |
	|GPIO11|Low  | SPI0_SCLK  | SD3          |<reserved>| -                | -          | -      |
	|GPIO12|Low  | PWM0       | SD4          |<reserved>| -                | -          |ARM_TMS |
	|GPIO13|Low  | PWM1       | SD5          |<reserved>| -                | -          |ARM_TCK |
	|GPIO14|Low  | TXD0       | SD6          |<reserved>| -                | -          |TXD1    |
	|GPIO15|Low  | RXD0       | SD7          |<reserved>| -                | -          |RXD1    |
	|GPIO16|Low  | <reserved> | SD8          |<reserved>| CTS0             |SPI1_CE2_N  |CTS1    |
	|GPIO17|Low  | <reserved> | SD9          |<reserved>| RTS0             |SPI1_CE1_N  |RTS1    |
	|GPIO18|Low  | PCM_CLK    | SD10         |<reserved>| BSCSL SDA / MOSI |SPI1_CE0_N  |PWM0    |
	|GPIO19|Low  | PCM_FS     | SD11         |<reserved>| BSCSL SCL / SCLK |SPI1_MISO   |PWM1    |
	|GPIO20|Low  | PCM_DIN    | SD12         |<reserved>| BSCSL / MISO     |SPI1_MOSI   |GPCLK0  |
	|GPIO21|Low  | PCM_DOUT   | SD13         |<reserved>| BSCSL / CE_N     |SPI1_SCLK   |GPCLK1  |
	|GPIO22|Low  | <reserved> | SD14         |<reserved>| SD1_CLK          |ARM_TRST    | -      |
	|GPIO23|Low  | <reserved> | SD15         |<reserved>| SD1_CMD          |ARM_RTCK    | -      |
	|GPIO24|Low  | <reserved> | SD16         |<reserved>| SD1_DAT0         |ARM_TDO     | -      |
	|GPIO25|Low  | <reserved> | SD17         |<reserved>| SD1_DAT1         |ARM_TCK     | -      |
	|GPIO26|Low  | <reserved> | <reserved>   |<reserved>| SD1_DAT2         |ARM_TDI     | -      |
	|GPIO27|Low  | <reserved> | <reserved>   |<reserved>| SD1_DAT3         |ARM_TMS     | -      |
	|GPIO28|-    | SDA0       | SA5          | PCM_CLK  |<reserved>        | -          | -      |
	|GPIO29|-    | SCL0       | SA4          | PCM_FS   |<reserved>        | -          | -      |
	|GPIO30|Low  | <reserved> | SA3          | PCM_DIN  | CTS0             | -          |CTS1    |
	|GPIO31|Low  | <reserved> | SA2          | PCM_DOUT | RTS0             | -          |RTS1    |
	|GPIO32|Low  | GPCLK0     | SA1          |<reserved>| TXD0             | -          |TXD1    |
	|GPIO33|Low  | <reserved> | SA0          |<reserved>| RXD0             | -          |RXD1    |
	|GPIO34|High | GPCLK0     | SOE_N / SE   |<reserved>|<reserved>        | -          | -      |
	|GPIO35|High | SPI0_CE1_N | SWE_N / SRW_N| -        |<reserved>        | -          | -      |
	|GPIO36|High | SPI0_CE0_N | SD0          | TXD0     |<reserved>        | -          | -      |
	|GPIO37|Low  | SPI0_MISO  | SD1          | RXD0     |<reserved>        | -          | -      |
	|GPIO38|Low  | SPI0_MOSI  | SD2          | RTS0     |<reserved>        | -          | -      |
	|GPIO39|Low  | SPI0_SCLK  | SD3          | CTS0     |<reserved>        | -          | -      |
	|GPIO40|Low  | PWM0       | SD4          | -        |<reserved>        |SPI2_MISO   |TXD1    |
	+------+-----+------------+--------------+----------+------------------+------------+--------+

	여기서 봐야할 것이 있다.
	pins 14과 15는 각각 TXD1 and RXD1 alternative functions available이다.
	즉, 만약 pins 14과 15의 alternative function # 5를 선택하면 우리는 mini UART를 사용할 수 있다.
	pin 14은 Mini UART transmit data pin으로 사용되고 pin 15는 Mini UART receive data pin으로 사용된다.

	GPIO 기능 선택은 GPFSELn(GPIO Function Select) 레지스터들로 한다.
	- GPFSEL0: GPIO 0~9
	- GPFSEL1: GPIO 10~19
	- GPFSEL2: GPIO 20~29
		… 이런 구조.

	14/15는 10~19 범위에 포함됨.
	그래서 이 pin들의 alternative function을 설정하려면 GPFSEL1을 건드려야 한다.

	이 register의 모든 bit가
	의미하는 내용은 다음과 같다:

	GPIO Function Select Register Bitfields (example: FSEL10~FSEL19)
	+--------+-----------+--------------------------------------------------------------+------+-------+
	| Bit(s) | Field     | Description                                                  | Type | Reset |
	+--------+-----------+--------------------------------------------------------------+------+-------+
	| 31-30  | ---       | Reserved                                                     |  R   |   0   |
	| 29-27  | FSEL19    | FSEL19 - Function Select 19                                  | R/W  |   0   |
	|        |           |   000 = GPIO Pin 19 is an input                              |      |       |
	|        |           |   001 = GPIO Pin 19 is an output                             |      |       |
	|        |           |   100 = GPIO Pin 19 takes alternate function 0               |      |       |
	|        |           |   101 = GPIO Pin 19 takes alternate function 1               |      |       |
	|        |           |   110 = GPIO Pin 19 takes alternate function 2               |      |       |
	|        |           |   111 = GPIO Pin 19 takes alternate function 3               |      |       |
	|        |           |   011 = GPIO Pin 19 takes alternate function 4               |      |       |
	|        |           |   010 = GPIO Pin 19 takes alternate function 5               |      |       |
	| 26-24  | FSEL18    | FSEL18 - Function Select 18                                  | R/W  |   0   |
	| 23-21  | FSEL17    | FSEL17 - Function Select 17                                  | R/W  |   0   |
	| 20-18  | FSEL16    | FSEL16 - Function Select 16                                  | R/W  |   0   |
	| 17-15  | FSEL15    | FSEL15 - Function Select 15                                  | R/W  |   0   |
	| 14-12  | FSEL14    | FSEL14 - Function Select 14                                  | R/W  |   0   |
	| 11-9   | FSEL13    | FSEL13 - Function Select 13                                  | R/W  |   0   |
	| 8-6    | FSEL12    | FSEL12 - Function Select 12                                  | R/W  |   0   |
	| 5-3    | FSEL11    | FSEL11 - Function Select 11                                  | R/W  |   0   |
	| 2-0    | FSEL10    | FSEL10 - Function Select 10                                  | R/W  |   0   |
	+--------+-----------+--------------------------------------------------------------+------+-------+
	
	여기서 중요한 것이 있다.
	GPFSEL register에서 각각의 pin은 3bit로 표현된다.
	예를 들어, pin 14는 FSEL14 필드에 해당하며, bit 12, 13, 14로 구성된다.
	이 3bit는 pin 14의 기능을 설정하는데 사용된다.
	잘 보면 FSEL10 ~ FSEL19까지 각각 3bit씩 할당되어 있다. (하나의 GPFSL register는 32bit이므로 총 10개의 pin을 설정 가능
	나머지 3bit는 reserved)
	이 3bit를 활용해서 이 pin은 어떤 기능이냐를 숫자로 저장한다.

	GPFSEL1 레지스터에서 원하는 GPIO pin의 alternative function을 설정하는 방법은 다음과 같다.
	1. 먼저 GPFSEL1 레지스터의 현재 값을 읽어온다.
	2. 그런 다음, 해당 pin에 해당하는 3bit를 클리어(0으로 만듦)한다.
	3. 그 다음, 원하는 alternative function에 해당하는 값을 3bit에 설정한다.
	4. 마지막으로, 수정된 값을 GPFSEL1 레지스터에 다시 쓴다.

	이 때 원하는 GPIO pin을 찾는 방법은 다음 공식을 따르면 된다.
	+------------------------------------------------+
	| 핀이 p일 때:                                    |
	|											     |
	| 어느 GPFSEL에 있나: reg = p / 10                 |
	|												 |
	|그 레지스터 안에서 시작 비트: shift = (p % 10) * 3	 |
	+------------------------------------------------+
	예를 들어, pin 14의 alternative function을 설정하려면:
	1. reg = 14 / 10 = 1 (즉, GPFSEL1)
	2. shift = (14 % 10) * 3 = 4 * 3 = 12 (즉, bit 12부터 시작)
	3. alternative function 5는 010이므로, bit 12-14에 010을 설정해야 한다

	** 3bit 값 의미:
	- 000: 입력
	- 001: 출력
	- 100: ALT0
	- 101: ALT1		
	- 110: ALT2
	- 111: ALT3
	- 011: ALT4
	- 010: ALT5

	따라서 이 정보를 기반으로 아래 코드의 해석이 가능하다.

	selector = get32(GPFSEL1);
	selector &= ~(7<<12);                   // gpio14 부분: bit 12~14 클리어
	selector |= 2<<12;                      // gpio14 값을 alt5로 설정 -> 010
	selector &= ~(7<<15);                   // gpio15 부분: bit 15~17 클리어
	selector |= 2<<15;                      // gpio15 값을 alt5로 설정 -> 010
	put32(GPFSEL1,selector);				// 수정된 값을 GPFSEL1 레지스터에 다시 씀

	------------------------------------------------------------------------------------
	GPIO pull-up/down

	만약 어떤 특정 pin을 input으로 사용하고 아무것도 연결하지 않는다면, 해당 pin의 값이 0인지 1인지
	알 수 없다. 왜냐하면 특정 GPIO를 입력(input) 으로 두고 아무 것도 연결하지 않으면, 그 핀은 
	전기적으로 “공중에 떠 있는” 상태가 되기 때문이다. 전압이 0V로 확실히 내려가 있는 것도 아니고
	3.3V로 확실히 올라가 있는 것도 아니다. 따라서 해당 pin이 읽힐 때마다 주변의 전자기 간섭에
	의해서 0 또는 1로 무작위로 읽힐 수 있다. 즉, 해당 장치는 무작위 값을 report 할 것이다. 
	이를 방지하기 위해서, GPIO pins는 pull-up 또는 pull-down mechanism을 지원한다.

	pull-up/down mechanism이란 무엇인가?
	GPIO pin을 특정한 전압 상태로 강제하는 방법이다. 즉, 핀에 약한 저항(내부 pull resistor)을 
	통해 기본 전압을 걸어주는 기능!



	1. 만약 특정 pin을 pull-up state로 설정하고 아무것도 연결하지 않는다면
		-> 해당 pin은 기본적으로 높은 전압 상태(논리 1)를 유지 (3.3V 쪽으로 살짝 끌어올림)
	2. 만약 특정 pin을 pull-down state로 설정하고 아무것도 연결하지 않는다면
		-> 해당 pin은 기본적으로 낮은 전압 상태(논리 0)를 유지 (GND 쪽으로 살짝 끌어내림)

	+------------------------------------+
	|3개의 available states:			  |
	|1. No pull-up/pull-down (floating)  |
	|2. Pull-up  					     |            
	|3. Pull-down						 |
	+------------------------------------+

	pin 14와 15를 사용하는 경우에는 이런 설정이 필요 없다. 즉, UART pin(GPIO14[TX]/15[RX])는 
	1. No pull-up/pull-down (floating)를 사용한다. 왜냐하면 이 핀들은 UART TXD와 RXD로
	사용되기 때문이다. TXD는 항상 출력으로 동작해 UART가 계속 구동하고, RXD는 항상 입력으로 동작하기에 
	상대 장치가 신호를 넣어주기 때문이다. 이 상황에서 pull-up/down을 켜두면 오히려 신호 품질에 방해가 
	될 수 있다.

	pin의 state는 reboot 이후에도 유지된다. 따라서 어떠한 pin을 사용하기 전에 항상 그 pin의 설정을 
	확인하고 필요한 경우 적절히 설정하는 것이 중요하다. ALWAYS INITIALIZE GPIO PINS' STATE BEFORE USE.

	pin의 state를 변경하는 방법은 생각보다 어렵다. 왜냐하면 물리적으로 electric circuit의 switch를 
	조작해야 하기 때문이다. 이 과정은 GPPUD와 GPPUDCLK registers를 통해서 제어된다.

	아래는 GPIO pull-up/down 설정을 위한 절차이다 (page 101 of the BCM2837 ARM Peripherals manual)
	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	The GPIO Pull-up/down Clock Registers control the actuation of internal pull-downs on
	the respective GPIO pins. These registers must be used in conjunction with the GPPUD
	register to effect GPIO Pull-up/down changes. The following sequence of events is
	required:
	1. Write to GPPUD to set the required control signal (i.e. Pull-up or Pull-Down or neither
	to remove the current Pull-up/down)
	2. Wait 150 cycles – this provides the required set-up time for the control signal
	3. Write to GPPUDCLK0/1 to clock the control signal into the GPIO pads you wish to
	modify – NOTE only the pads which receive a clock will be modified, all others will
	retain their previous state.
	4. Wait 150 cycles – this provides the required hold time for the control signal
	5. Write to GPPUD to remove the control signal
	6. Write to GPPUDCLK0/1 to remove the clock
	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	위 절차를 설명하면
	GPIO 패드(실제 핀 회로) 안의 pull-up/down 저항 스위치를 물리적으로 바꾸는 과정은 2단계의 protocol을
	요구한다.

	- GPPUD: 		“어떤 상태로 바꿀 건지(UP / DOWN / OFF)”를 선택해두는 레지스터
		pull 설정의 의도(UP/DOWN/OFF) 를 내부로 내보내는 “컨트롤 신호”를 세팅한다.
		값 의미:
			00 = Off (disable pull-up/down)
			01 = Enable Pull-down control
			10 = Enable Pull-up control
			11 = Reserved (do not use)
	- GPPUDCLK0/1: 	“어느 GPIO 핀에 그 선택을 적용할지”를 클럭으로 확정시키는 레지스터 
					(여기서 1을 준 핀만 바뀜)
		GPIO 패드에 클럭 펄스를 줘서, 방금 GPPUD에 올려둔 컨트롤 신호를 특정 핀에 ‘적용’한다.
		CLK0는 GPIO 0~31, CLK1은 GPIO 32~53 담당.
		여기서 비트 = 1인 핀만 변경, 나머지는 그대로 유지.

	사용 방법은 아래와 같다.
	1. GPPUD 레지스터에 원하는 pull-up/down 설정을 쓴다.
	2. 약간의 딜레이를 준다 (150 cycles), set-up time 확보
	3. GPPUDCLK0/1 레지스터에 변경할 pin의 비트를 1로 세팅해 클럭을 준다.
	4. 약간의 딜레이를 준다 (150 cycles) hold time 확보
	5. GPPUD 레지스터에 0을 써서 컨트롤 신호를 제거한다.
	6. GPPUDCLK0/1 레지스터에 0을 써서 클럭 신호를 제거한다.

	이 과정의 예시가 바로 위 코드의 두 번째 뭉치다.

	put32(GPPUD,0);						// 1. GPPUD에 0을 써서 pull-up/down 비활성화
	delay(150);							// 2. 150 cycles 딜레이 (set-up time)	
	put32(GPPUDCLK0,(1<<14)|(1<<15));	// 3. GPPUDCLK0에 pin 14,15 비트를 1로 세팅해 클럭 적용
	delay(150);							// 4. 150 cycles 딜레이 (hold time)
	put32(GPPUDCLK0,0);					// 5. GPPUDCLK0에 0을 써서 클럭 제거

	-------------------------------------------------------------------------------------
	Mini UART initialization

	자, 이제 mini UART는 GPIO pins에 연결되었고, pull-up/down 설정도 끝났다.
	즉, UART 통신이 되려면 최소한 
	(1) 핀을 UART에 연결하고, 
	(2) 핀 전기적 상태를 안정화하고, 
	(3) UART 내부 동작 규칙(속도/데이터폭/TX·RX on/off)을 맞춰야 하기에 
	마지막 (3)단계로 Mini UART initialization을 해야 한다.

	이 단계에서 해야하는 일은 다음과 같다.
	- TX/RX를 켜야 함
		enable 안 하면 아무 데이터도 안 나가고 안 들어옴.
	- 데이터 프레임 규격(여기선 8-bit)
		상대는 8비트로 보내는데 나는 7비트로 받으면 데이터가 깨짐.
	- 속도(baudrate)
		이게 가장 중요함.
		115200으로 보내는데 나는 9600으로 받으면 “타이밍이 완전히 달라서” 해석 불가.
		그래서 AUX_MU_BAUD_REG 같은 레지스터를 반드시 맞춰야 함.
	- (선택) 인터럽트/흐름제어
		지금은 polling 방식으로 갈 거라서 interrupts off, flow-control off로 단순화.

	실제 코드를 살펴보자
	코드는 아래 흐름(순서)로 진행된다.
	1. Mini UART 블록을 켜서 레지스터 접근 가능하게 만든다
	2. TX/RX를 꺼둔 상태로 설정(인터럽트/데이터폭/RTS/baudrate) 끝낸다
	3. TX/RX를 켠다

	put32(AUX_ENABLES,1);                   
	-> 	“전원 스위치 + 레지스터 접근 게이트”를 켠다.
		이 레지스터는 Mini UART 블록 전체를 켜고 끄는 역할을 한다.
		1을 쓰면 Mini UART가 켜지고, 레지스터(AUX_MU 레지스터들)에 접근할 수 있게 된다.
		0을 쓰면 Mini UART가 꺼지고, 레지스터 접근도 차단된다.
    put32(AUX_MU_CNTL_REG,0);               
	->	잠시 auto flow control과 receiver and transmitter를 disable한다. 왜냐하면 
		설정을 바꾸는 동안에는 TX/RX가 켜져 있다면 baudrate 변경 중 라인이 깨지거나 FIFO/상태가
		엉킬 수 있기 때문이다. 그래서 설정이 모두 끝날 때까지 TX/RX를 끈다.

		AUX_MU_CNTL_REG는 “TX/RX enable + 자동 흐름제어”가 들어있는 레지스터이다.
		- bit 0: Receiver Enable (1 = enable, 0 = disable)
		- bit 1: Transmitter Enable (1 = enable, 0 = disable)
		- bit 2: Auto Flow Control Enable (1 = enable, 0 = disable)
    put32(AUX_MU_IER_REG,0);                
	->	Interrupt 말고 polling 방식으로 할 것이라서, 인터럽트를 꺼둔다.
		
		AUX_MU_IER_REG는 “Interrupt Enable” 레지스터이다.
		- bit 0: Enable Receive Interrupt (1 = enable, 0 = disable)
		- bit 1: Enable Transmit Interrupt (1 = enable, 0 = disable)
		0을 쓰면 둘 다 꺼진다.
    put32(AUX_MU_LCR_REG,3);                
	->	데이터 프레임을 8-bit 모드로 설정한다. (8-bit가 기본임)

		AUX_MU_LCR_REG는 “Line Control Register이다. 데이터 폭(7/8bit) 등을 정함
		- bit 0: Data Size (0 = 7 bits, 1 = 8 bits)
		따라서 3(0b11)을 쓰면 bit 0,1이 1이 되어 8-bit 모드가 된다.
    put32(AUX_MU_MCR_REG,0);               
	->	자동 흐름제어를 사용하지 않을 것이기에 RTS line을 항상 high로 설정한다. 여기서 RTS는 
		컴네게 시간 때 배웠던 request to send 신호이다.

		AUX_MU_MCR_REG는 “Modem Control Register”이다.
		- bit 0: RTS Control (0 = RTS line is always high)
		- bit 1: RTS Control (1 = RTS line is controlled by auto flow control
		따라서 0을 쓰면 RTS line이 항상 high가 된다.
    put32(AUX_MU_BAUD_REG,270);             
	->	UART의 baudrate를 설정한다. 여기서는 115200으로 설정한다.

		AUX_MU_BAUD_REG는 “Baudrate Register”이다.
		이 레지스터에 쓰는 값은 다음 공식으로 계산된다.
			Baudrate Register 값 = (system_clock_freq / (8 * desired_baudrate)) - 1
			
			예를 들어, system_clock_freq가 250MHz이고 desired_baudrate가 115200이라면,
			Baudrate Register 값 = (250,000,000 / (8 * 115,200)) - 1 ≈ 270
			따라서 270을 쓰면 baudrate가 약 115200으로 설정된다.
    put32(AUX_MU_CNTL_REG,3);              
	->	마지막으로 transmitter(TX)와 receiver(RX)를 켠다.

		AUX_MU_CNTL_REG은 “TX/RX enable + 자동 흐름제어”가 들어있는 레지스터이다.
		- bit 0: Receiver Enable (1 = enable, 0 = disable)
		- bit 1: Transmitter Enable (1 = enable, 0 = disable)
		따라서 3(0b11)을 쓰면 TX와 RX가 모두 켜진다.
	
	---------------------------------------------------------------------------
	Sending data using the Mini UART

	이제 Mini UART가 준비됨. 따라서 데이터를 송수신할 수 있다. 그리고 이를 위한 함수들이 위에 정의되어 있다.
	- uart_send(char c): 	문자 c를 Mini UART를 통해 전송
	- uart_recv(void): 		Mini UART를 통해 들어온 문자 하나를 수신
	- uart_send_string(char* str): 문자열 str을 Mini UART를 통해 전송

	자세히 살펴보자. 

	void uart_send ( char c )
	{
		while(1) {
			if(get32(AUX_MU_LSR_REG)&0x20) 
				break;
		}
		put32(AUX_MU_IO_REG,c);
	}
	char uart_recv ( void )
	{
		while(1) {
			if(get32(AUX_MU_LSR_REG)&0x01) 
				break;
		}
		return(get32(AUX_MU_IO_REG)&0xFF);
	}
	이 두 함수는 infinite loop으로 시작한다. 왜냐하면 Mini UART가 준비될 때까지 기다려야 하기 때문이다.
	즉, device가 송수신 준비가 될 때까지 기다리는 것이다.
	이 때 사용하는 register가 AUX_MU_LSR_REG (Line Status Register)이다.
	- uart_recv 함수에서는 bit 0(0x01)가 1이 될 때 -> 수신 데이터가 준비된 상태. DATA IS READY!
	- uart_send 함수에서는 bit 5(0x20)가 1이 될 때 -> 송신 버퍼가 비어있는 상태. READY TO SEND!
	따라서 각각의 함수는 해당 bit가 1이 될 때까지 기다린다.
	그 후에, AUX_MU_IO_REG에 value를 쓰거나 읽는다.
	- uart_send 함수는 AUX_MU_IO_REG 레지스터에 문자 c를 쓴다. 그러면 Mini UART가 그 문자를 전송한다.
	- uart_recv 함수는 AUX_MU_IO_REG 레지스터에서 수신된 문자를 읽어와서 반환한다.

	void uart_send_string(char* str)
	{
		for (int i = 0; str[i] != '\0'; i ++) {
			uart_send((char)str[i]);
		}
	}
	이 함수는 문자열 str을 하나씩 문자 단위로 uart_send 함수를 호출해서 전송한다.
	겉으로 보면 문자열을 보내는 것 처럼 보이는 high level function.
*/