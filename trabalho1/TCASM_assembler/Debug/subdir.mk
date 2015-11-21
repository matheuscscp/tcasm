################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../TCASM_assembler.c \
../TCASM_hashtable.c \
../TCASM_list.c \
../TCASM_main.c \
../TCASM_symbol.c 

OBJS += \
./TCASM_assembler.o \
./TCASM_hashtable.o \
./TCASM_list.o \
./TCASM_main.o \
./TCASM_symbol.o 

C_DEPS += \
./TCASM_assembler.d \
./TCASM_hashtable.d \
./TCASM_list.d \
./TCASM_main.d \
./TCASM_symbol.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -std=c99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


