#include "include/mini_uart.h"

void kernel_main(void)
{
	uart_init();
	uart_send_string("Hello, world!\r\n");

	while (1) {
		uart_send(uart_recv());
	}
}

/*
	Mini UART 장치를 이용하여 screen에 "Hello, world!" 문자열을 출력하고,
	이후에는 infinite loop에 진입하여 Mini UART로부터 수신된 문자를 다시 송신하는 프로그램.
*/