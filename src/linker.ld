SECTIONS
{
	.text.boot : { *(.text.boot) }
	.text : { *(.text) }
	.rodata : { *(.rodata) }
	.data : { *(.data) }
	. = ALIGN(0x8);
	bss_begin = .;
	.bss : { *(.bss*) } 
	bss_end = .;
}

/*
    linker script의 primary purpose는 input object files(_c.o and _s.o)가
    어떻게 output file(.elf)에 mapping되어야 하는지 describe한다. 

    linker는 final binary file를 만들 때,
    1. .text = Code 영역, 명령어.
    2. .rodata = Constant data. const int, const char 등이 여기에 저장됨
    3. .data = Regular data. data 영역. initialized global & static 변수들이 저장됨.
    4. .bss = bss 영역. Un-initialized global & static 변수들이 저장됨.
                이 영역의 값은 모두 0으로 초기화된다.
    이걸 memory에 어떤 순서로, 어디에 놓을지 정해준다. 이게 바로 SECTIONS { ... }

    .text.boot       :   처음 실행될 boot/start-up code
    Raspbery Pi는 boot할 때 kernel8.img를 memory(RAM)에 load하고 file의 시작점
    부터 execution을 진행한다. 그렇기 때문에 .text.boot가 가장 먼저 실행되어야 하고
    이를 담는 .text.boot section이 맨 앞에 위치해야하는 것이다. 

    나머지 sections   :   특별한 처리 거의 없음.

    .bss             :   0으로 initialize해야 하는 variables 영역
    이 값들을 실제로 파일 안에 0으로 다 저장하면 kernel image가 너무 커진다.
    따라서 ELF는 .bss를 이렇게 처리한다.
    1. '이 구간 크기가 얼마다'만 ELF의 header에 기록
    2. 실제 파일 데이터에는 .bss 내용을 넣지 않음. 즉, 0 덩어리를 저장 X.
    이처럼 공간을 절약하기 위해 section을 나누는 것.
    
    추후에 final binary file이 RAM이 올라갈 때는 kernel이 직접 .bss를 0으로 채워주어야 한다.
    파일에는 .bss가 없기에 boot code에서 
    . = ALIGN(0x8);         // 아래 배치의 시작 주소를 8의 배수로 맞춘다. 
                            //  (str instruction = 64bit = 8B) 
    bss_begin = .;          // .bss 시작 주소부터
	.bss : { *(.bss*) } 
	bss_end = .;            // .bss 끝 주소까지 0으로 채운다.
*/