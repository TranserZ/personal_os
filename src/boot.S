#include "include/mm.h" 	// check mm.h file in <include> for more information. 
							// memzero나 LOW_MEMORY같은 매크로/상수 or 선언을 가져온다.
.section ".text.boot"
/*
	'지금투너 나오는 코드를 'ELF 섹션 .text.boot에 넣어라' 라는 뜻
	linker.ld에서 설명했지만, .text.boot는 처음 실행될 boot/start-up code이다.
	Raspberry Pi의 kernel loader는 kernel8.img의 파일 맨 앞부터 실행한다. 
	따라서 아래 start function부터 execution이 시작된다.
*/

.globl _start
_start:
	mrs	x0, mpidr_el1		// mpidr_el1(register)에 있는 값을 x0로 옮김.
	and	x0, x0,#0xFF		// x0의 오른쪽 8bit를 x0에 다시 저장. processor ID를 확인하기 위해서.
	cbz	x0, master			// 만약 찾은 processor ID가 0일 경우 master로 jump.
	b	proc_hang
/*
	Raspberry Pi 3는 4개의 core processors를 가지고 있다. 
	Raspberry Pi의 전원이 들어오면, 각각의 core는 모두 이 코드를 실행한다.
	하지만, 우리는 이 코드가 4번 실행되기를 바라지 않음. Race condition이
	발생하기 때문이다. 따라서 오직 하나의 core에서 이 코드를 실행하도록 설정하는
	것이 목표다. 그리고 그 core를 processor ID가 0인 core로 지정하고자 한다. 
	나머지 core는 endless loop에 들어가도록 하여 마치 spin-lock에 걸린 것 처럼
	설정되도록 한다. 
*/

proc_hang: 					// endless loop for other processors
	b 	proc_hang

master:
	adr	x0, bss_begin
	adr	x1, bss_end
	sub	x1, x1, x0
	bl 	memzero
/*
	master의 시작부터 여기까지: memzero를 통해 .bss secion을 clean해주기.
	linker.ld에서 설명했지만, ELF 파일은 .bss section을 초기화하지 않는다.
	단순히 ELF 파일의 header 부분에 .bss section의 시작 부분과 끝 부분을 
	기록해 둔다. 

	시작과 끝 부분의 차를 구해 초기화해야 하는 부분의 영역을 구하고 
	x0 = bss_begin, x1 = 초기화해야 하는 부분의 영역(bss_end - bss_begin)
	를 저장한다. memzero는 x0-x6 registers를 인자로 받음. 따라서 정상적으로
	원하는 값을 memzero로 보낸다.
*/

	mov	sp, #LOW_MEMORY 
	bl	kernel_main
	b 	proc_hang			// should never come here
/*
	.bss section을 clean했다면, stack pointer를 initialize하고 kernel_main
	함수에 실행이 옮겨간다. 

	Raspberry Pi는 Kernel를 address 0부터 load한다(항상은 아님). 따라서 
	initial stack pointer는 stack이 어느정도의 크기를 갖추었을 때 kernel 
	image를 override 하지 않을 정도로 높게 설정하면 된다. 여기에 적합한 값이 
	LOW_MEMORY(=4MB). 왜냐하면 Raspberry Pi에서 kernel의 stack은 그렇게 
	많이 자라지 않기 때문이다.
	더 풀어서 설명하면 kernel img(code+data+bss) 끝 주소가 4MB아래여야 함.
	
	kernel_main()는 보통:
	1. infinite loop
	2. wfe로 wait
	3. panic/reboot
	같은 형태로 절대 return하지 않게 짜는 게 정상 설계. 
	따라서 정상이라면 kernel_main()함수에서 돌아오지 않는 것이 정상이다.
	비정상적인 경우(함수에서 돌아온다면) proc_hang으로 보내 infinite loop에
	가둔다. 

	!!!주의!!! 
	>> 'kernel_main이 안 돌아온다' != 'CPU가 그 함수 한 줄만 계속 실행한다'
	'kernel_main이 안 돌아온다'의 의미는 '부트스트랩으로 돌아오지 않고, OS의
	main control loop(scheduler/event loop)로 들어간다는 뜻이다. 
	그럼 proc_hang에 같혀있는 다른 코어들은?
	booting 초반엔 proc_hang에서 대기하지만, kernel이 준비되면 깨워서 secondary
	entry로 진입시킨다. 
*/

/*
	--ARM Assembly Instructions--
	1. mrs		: Load value from a system register to one of the
					general purpose registers (x0 - x30)
	2. and		: AND 연산. 
	3. cbz		: 가장 마지막에 실행된 연산의 결과를 0과 비교하고 비교결과가 True라면
					주어진 label로 jump한다.
	4. b		: 주어진 label로 unconditional branch
	5. adr		: 주어진 label의 relative address를 target register에 저장.
	6. sub		: 빼기. sub x1, x1, x0 -> x1 = x1 - x0
	7. bl		: 'Branch with a link'. Unconditional branch를 진행한 후 
					return address를 x30에 저장한다. Subroutine이 끝났다면 
					ret 명령어를 통해 return address로 jump back한다.
	8. move		: register로 register의 값 또는 constant의 값을 저장한다.
*/
