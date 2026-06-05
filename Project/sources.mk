#******************************************************************************
# Copyright (C) 2026 by Carlos Villarreal - CETYS Universidad
#
# Redistribution, modification or use of this software in source or binary
# forms is permitted as long as the files maintain this copyright. Users are
# permitted to modify this and use it to learn about the field of embedded
# software. Carlos Villarreal and CETYS Universidad are not liable for any
# misuse of this material.
#
#*****************************************************************************

#------------------------------------------------------------------------------
# sources.mk — Lista de archivos fuente del sistema de riego automatico
#
# Estructura de carpetas:
#   Sources/   — archivos .c de todos los modulos
#   Includes/  — archivos .h de todos los modulos
#   CMSIS/     — cabeceras del hardware STM32F4xx
#   Linker/    — script del linker stm32f4.ld
#
#------------------------------------------------------------------------------

SRC_DIR     = Sources
INCLUDE_DIR = Includes
CMSIS_DIR   = ./CMSIS

# Lista de todos los archivos fuente del proyecto
SRCS = \
$(SRC_DIR)/main.c              \
$(SRC_DIR)/stm32_startup.c     \
$(SRC_DIR)/system_stm32f4xx.c  \
$(SRC_DIR)/gpio_driver.c       \
$(SRC_DIR)/tim_driver.c        \
$(SRC_DIR)/timer.c             \
$(SRC_DIR)/systick.c           \
$(SRC_DIR)/adc_driver.c        \
$(SRC_DIR)/pwm.c               \
$(SRC_DIR)/uart.c              \
$(SRC_DIR)/utils.c             \
$(SRC_DIR)/i2c_driver.c        \
$(SRC_DIR)/lcd.c   		       \
$(SRC_DIR)/soil_moisture.c     \
$(SRC_DIR)/servo.c             \
$(SRC_DIR)/irrigation.c        \
$(SRC_DIR)/ultrasonic.c        \
$(SRC_DIR)/pump.c              \
$(SRC_DIR)/button.c            \
$(SRC_DIR)/flowsensor.c

# Directorios de cabeceras
INCLUDES = \
-I$(INCLUDE_DIR)              \
-I$(CMSIS_DIR)                \
-I$(CMSIS_DIR)/STM32F4xx
