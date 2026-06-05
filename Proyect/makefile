include sources.mk

EXEC        = app.elf
OBJ_DIR     = Objects
LINKER_FILE = Linker/stm32f4.ld

CPU   = cortex-m4
ARCH  = armv7e-m
SPECS = nosys.specs
FPU   = fpv4-sp-d16

ARCHFLAGS = -mcpu=$(CPU) -mthumb -march=$(ARCH) \
            -mfloat-abi=hard -mfpu=$(FPU) --specs=$(SPECS)

OBJS := $(patsubst %.c, $(OBJ_DIR)/%.o, $(notdir $(SRCS)))

CC      = arm-none-eabi-gcc
CFLAGS  = -g -O0 -std=c99 -Wall -DSTM32F411xE $(ARCHFLAGS)
LDFLAGS = -nostdlib -T $(LINKER_FILE)

vpath %.c $(sort $(dir $(SRCS)))

# Detectar sistema operativo
ifeq ($(OS),Windows_NT)
    SHELL   := C:/Program Files/Git/usr/bin/sh.exe
    MKDIR   = mkdir -p $(OBJ_DIR)
    RM_DIR  = rm -rf $(OBJ_DIR)
    RM_FILE = rm -f $(EXEC)
    RM_NUL  = rm -f nul
else
    MKDIR   = mkdir -p $(OBJ_DIR)
    RM_DIR  = rm -rf $(OBJ_DIR)
    RM_FILE = rm -f $(EXEC)
    RM_NUL  = true
endif

$(OBJ_DIR)/%.o : %.c | $(OBJ_DIR)
	$(CC) -c $< $(CFLAGS) $(INCLUDES) -o $@

$(OBJ_DIR) :
	$(MKDIR)

.PHONY: build
build : $(OBJ_DIR) $(EXEC)

$(EXEC) : $(OBJS)
	$(CC) $(OBJS) $(CFLAGS) $(INCLUDES) $(LDFLAGS) -o $(EXEC)
	@echo "==> Compilacion exitosa: $(EXEC)"

.PHONY: flash
flash :
	openocd -f board/st_nucleo_f4.cfg \
	  -c "init"               \
	  -c "reset halt"         \
	  -c "program $(EXEC) verify" \
	  -c "reset run"          \
	  -c "shutdown"

.PHONY: size
size : $(EXEC)
	arm-none-eabi-size $(EXEC)

.PHONY: clean
clean :
	$(RM_DIR)
	$(RM_FILE)
	$(RM_NUL)