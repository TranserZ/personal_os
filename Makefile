ARMGNU ?= aarch64-linux-gnu
# Makefile은 variable definition으로 시작한다.
# ARMGNU 변수는 cross-compiler prefix이며
# aarch64-linux-gnu로 설정되어 있다.
#
# cross-compiler prefix는 '내 PC에서 다른 기기(다른 프로세서를 가진)용
# 프로그램을 만들 때 쓰는 도구 이름 앞의 공통 이름표'이다. 예를 들어 보자.
# 컴파일러는 보통 gcc를 사용한다. 근데 gcc는 내 PC(x86)용 프로그램을 만든다.
# 근데 Raspberry Pi는 Broadcom의 64-bit quad-core 프로세스이다. (arm64를
# 위한 source code를 x86 PC에서 컴파일한다) 즉, 다른 프로세서를 탑재했다.
# 만약 이 cross-compiler prefix를 설정하지 않고 프로그램을 만들었을 경우,
# 이 컴퓨터에서는 작동하는데, 정작 프로그램을 실제로 탑재할 Raspberry Pi에서는
# 작동하지 않는 문제가 발생한다. 그래서 cross-compiler prefix를 설정해
# 주어야 한다.

COPS = -Wall -nostdlib -nostartfiles -ffreestanding -Iinclude -mgeneral-regs-only
ASMOPS = -Iinclude
# COPS와 ASMOPS 변수는 각각 C 컴파일러와 어셈블러에 전달할 옵션들을 정의한다.
# - Wall : 모든 경고 메시지를 출력
# - nostdlib : 표준 라이브러리를 사용하지 않음
# - nostartfiles : 표준 시작 파일을 사용하지 않음
# - ffreestanding : 프리스탠딩 환경에서 컴파일 (운영체제 없이 실행되는 코드)
# - Iinclude : include 디렉토리를 포함 경로에 추가
# - mgeneral-regs-only : 일반 레지스터만 사용하도록 지정

BUILD_DIR = build
SRC_DIR = src
# BUILD_DIR 변수는 빌드 출력 파일들(Compiled object files)이 저장될 디렉토리를 지정하고,
# SRC_DIR 변수는 소스 파일들이 위치한 디렉토리를 지정한다.

all : kernel8.img

clean :
	rm -rf $(BUILD_DIR) *.img
# 다음으로 make targets를 정의한다.
#
# 처음 2개의 targets는 비교적 간단하다.
# all    : default target(make은 언제나 첫 target을 default로 설정한다)으로,
#          kernel8.img 파일을 생성하도록 설정됨. 아무 argument 없이 make
#          명령어를 실행하면 이 target이 실행됨.
#
#          make 또는 make all 명령어를 실행 -> kernel8.img 파일 생성
#          kernel8.img 파일이 이미 존재하면 아무 작업도 수행하지 않음.
#          오래됐거나 없으면 kernel8.img 파일을 다시 생성함.
#
# clean  : 모든 compilation artifacts와 compiled kernel image를 삭제하는
#          역할. 빌드 아티팩트(빌드 결과물)를 삭제하는 명령어를 포함한다.
#
#          rm: remove(삭제) 명령어
#          -rf: recursive(재귀적으로 폴더 안까지), force(강제) 옵션
#          $(BUILD_DIR): build 디렉토리와 그 안의 모든 파일들
#          *.img: 현재 디렉토리의 모든 .img 파일들

$(BUILD_DIR)/%_c.o: $(SRC_DIR)/%.c
	mkdir -p $(@D)
	$(ARMGNU)-gcc $(COPS) -MMD -c $< -o $@

$(BUILD_DIR)/%_s.o: $(SRC_DIR)/%.S
	$(ARMGNU)-gcc $(ASMOPS) -MMD -c $< -o $@
# 다음 두 개의 패턴 규칙(pattern rules)은 C 소스 파일과 어셈블리 소스 파일을
# 컴파일하는 방법을 정의한다.
#
# 첫 번째 규칙은 C 소스 파일을 컴파일하는 방법을 정의한다.
# $(@D)           : 현재 타겟 파일의 디렉토리 부분을 나타냄.
#                  (예: build/main_c.o -> build)
# mkdir -p $(@D)  : 타겟 디렉토리가 존재하지 않으면 생성. 안전을 위해.
# $<              : 첫 번째 prerequisite(의존 파일)을 나타냄.
#                  (예: src/main.c)
#     -MMD        : dependency file(.d 파일)을 생성하도록 gcc에 지시
#     -c          : 컴파일만 수행하고 링크는 하지 않음
#     -o $@       : 출력 파일을 지정 ($@는 현재 타겟 파일을 나타냄)
#
# 두 번째 규칙은 어셈블리 소스 파일을 컴파일하는 방법을 정의한다.
# 나머지 부분은 첫 번째 규칙과 동일하다.
#
# 예시:
# src/에 foo.c와 foo.S 파일이 있다고 가정하자.
# 첫 번째 규칙에 의해 src/foo.c는 build/foo_c.o로 컴파일되고,
# 두 번째 규칙에 의해 src/foo.S는 build/foo_s.o로 컴파일된다.
# ($< 와 $@는 runtime 때 각각 input과 output 파일로 대체된다)

C_FILES = $(wildcard $(SRC_DIR)/*.c)
ASM_FILES = $(wildcard $(SRC_DIR)/*.S)
OBJ_FILES = $(C_FILES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%_c.o)
OBJ_FILES += $(ASM_FILES:$(SRC_DIR)/%.S=$(BUILD_DIR)/%_s.o)
# 여기서 모든 object files의 array를 build한다. 이 때 object files는
# C 파일과 어셈블리 파일에서 각각 연쇄되어 생성된다.
# 즉, 소스 폴더에 있는 .c와 .S를 전부 찾고 → 그걸 빌드 폴더의 .o 파일 이름
# 목록으로 바꿔서 OBJ_FILES에 모아두는 작업.
# (실제 컴파일은 아직 안 함. 파일 ‘목록’만 만드는 단계)
#
# C_FILES = $(wildcard $(SRC_DIR)/*.c)
#     $(wildcard 경로패턴)은 해당 패턴에 매칭되는 파일들을 전부 나열.
#     SRC_DIR로 설정해 둔 디렉토리 안 .c로 끝나는 파일들을 전부 나열.
#     Ex ... SRC_DIR=src이고 src/에 a.c와 b.c가 있다면:
#     C_FILES = src/a.c src/b.c
#
# ASM_FILES = $(wildcard $(SRC_DIR)/*.S)
#     SRC_DIR로 설정해 둔 디렉토리 안 .S로 끝나는 파일들을 전부 나열.
#     Ex ... SRC_DIR=src이고 src/에 x.S와 y.S가 있다면:
#     ASM_FILES = src/x.S src/y.S
#
# OBJ_FILES = $(C_FILES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%_c.o)
#     이 문법은 '치환(substitution) 규칙'이다.
#     왼쪽 패턴: $(SRC_DIR)/%.c
#     오른쪽 패턴: $(BUILD_DIR)/%_c.o
#     즉, C_FILES에 있는 각 파일에 대해,
#     source 디렉토리 안 '파일이름.c'를 build 디렉토리 안 '파일이름_c.o'로 바꾼다.
#
# OBJ_FILES += $(ASM_FILES:$(SRC_DIR)/%.S=$(BUILD_DIR)/%_s.o)
#     마찬가지로, ASM_FILES에 있는 각 파일에 대해,
#     source 디렉토리 안 '파일이름.S'를 build 디렉토리 안 '파일이름_s.o'로 바꾼다.
#     += 연산자는 기존 OBJ_FILES에 이 결과를 추가/이어붙인다는 뜻이다.
#
# 결과적으로, OBJ_FILES에는 빌드 디렉토리 안에 생성될 모든 object 파일들의
# 목록이 담기게 된다.
# 예를 들어 src/에 a.c, main.c, start.S 파일이 있다면
# 결과는 다음과 같다:
# C_FILES = src/a.c src/main.c
# ASM_FILES = src/start.S
# OBJ_FILES = build/a_c.o build/main_c.o build/start_s.o
#
# 이 OBJ_FILES 목록은 이후에 컴파일과 링크 단계에서 사용된다.

DEP_FILES = $(OBJ_FILES:%.o=%.d)
-include $(DEP_FILES)
# 이 부분은 위에서 생성한 'object 파일 목록'을 그대로 'dependency
# 파일(.d) 목록'으로 바꾼다. 각 object file에 대해 대응되는 .d 파일
# (dependency 파일)을 생성한다. 예를 들어 build/main_c.o에 대해
# build/main_c.d 파일이 생성된다.
#
# dependency files란 무엇인가?
# 컴파일러가 소스 파일을 컴파일할 때, 해당 소스 파일이
# 어떤 헤더 파일들을 포함(include)하고 있는지 추적하는 파일이다.
# 이 정보는 Makefile이 어떤 소스 파일이 변경되었는지 파악하고,
# 그에 따라 어떤 object 파일들을 다시 컴파일해야 하는지 결정하는 데 사용된다.
#
# 어떤 파일이 변경되었을 때 -> 어느 파일들을 다시 컴파일해야 하는지 결정하는 데
#                              도움을 준다.
#
# -include $(DEP_FILES)
#     이 구문은 Makefile에 dependency 파일들을 포함시킨다.
#     -include는 해당 파일이 없어도 에러를 발생시키지 않는다.
#     dependency 파일들은 컴파일 시점에 자동으로 생성되므로,
#     처음 빌드할 때는 존재하지 않을 수 있다.

kernel8.img: $(SRC_DIR)/linker.ld $(OBJ_FILES)
	$(ARMGNU)-ld -T $(SRC_DIR)/linker.ld -o $(BUILD_DIR)/kernel8.elf  $(OBJ_FILES)
	$(ARMGNU)-objcopy $(BUILD_DIR)/kernel8.elf -O binary kernel8.img
# 최종 산출물 kernel8.img 파일을 만드는 'Link + Conversion'단계이다.
#
# 흐름은 딱 2단계:
#     1. .o 파일들을 link해서 kernel8.elf(elf: executable and linkable format) 만들고
#     2. 그 ELF를 'pure binary(.img) file'로 뽑아낸다.
#
# kernel8.img: $(SRC_DIR)/linker.ld $(OBJ_FILES)
#     linker.ld는 source 디렉토리에 있는 linker script 파일이다.
#     $(OBJ_FILES)는 위에서 만든 모든 object files의 목록이다.
#     kernel8.img를 만드려면 linker.ld(linker script)와 모든 object files($(OBJ_FILES))가 필요하다.
#     그래서 linker.ld나 어떤 object file이라도 변경되면 kernel8.img를 다시 만들도록 한다.
#
# $(ARMGNU)-ld -T $(SRC_DIR)/linker.ld -o $(BUILD_DIR)/kernel8.elf  $(OBJ_FILES)
#     첫 번째 명령어는 'Link해서 ELF 만들기'.
#     linker(ld)를 사용하여 모든 object files($(OBJ_FILES))를 하나의 ELF 형식의 kernel8.elf 파일로 링크한다.
#
#     -T 옵션은 linker script 파일을 지정하는 데 사용된다. ($(SRC_DIR)/linker.ld)
#     -o 옵션은 출력 파일 이름을 지정하는 데 사용된다. ($(BUILD_DIR)/kernel8.elf)
#     $(OBJ_FILES)는 링크할 object files의 목록이다.
#
# $(ARMGNU)-objcopy $(BUILD_DIR)/kernel8.elf -O binary kernel8.img
#     두 번째 명령어는 'ELF -> raw binary(.img)로 변환하기'.
#     objcopy로 kernel8.elf를 바이너리 형식의 kernel8.img 파일로 변환한다.
#
#     -O binary 옵션은 출력 형식을 raw binary로 지정한다.
#     ELF header/section table/symbol 같은 건 제거되고, '실제로 memory에 올라갈
#     순수한 바이너리 데이터'만 남게 된다.
#
# ELF 파일은 operating system이 존재할 때 실행 가능하다. 하지만 우리는
# 운영체제 없이 직접 하드웨어에서 실행할 수 있는 '순수한 바이너리 파일'이 필요하다.
# 왜냐하면 우리는 '운영체제를 만들고 있는 중'이기 때문이다. 그래서 ELF 파일을
# raw binary(.img) 파일로 변환하는 것이다. 따라서 ELF file의 executable and data
# sections만 남기고 나머지 메타데이터는 모두 제거하여 kernel8.img 파일을 생성한다.
#
# kernel8.img에서 '8'은 ARMv8, a 64-bit architecture를 의미한다. 이 파일 이름은
# firmware에게 '이 이미지는 ARMv8 아키텍처용이다'라고 알려주어 processor를 64-bit mode로
# 부팅하도록 지시한다.