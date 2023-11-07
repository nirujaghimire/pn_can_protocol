################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../utils/crc.c 

OBJS += \
./utils/crc.o 

C_DEPS += \
./utils/crc.d 


# Each subdirectory must supply rules for building sources it contributes
utils/%.o utils/%.su utils/%.cyclo: ../utils/%.c utils/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I"C:/Users/NIRUJA/Desktop/Github/pn_can_protocol/pn_can_protocol_2_0_0/pn_protocol" -I"C:/Users/NIRUJA/Desktop/Github/pn_can_protocol/pn_can_protocol_2_0_0/test" -I"C:/Users/NIRUJA/Desktop/Github/pn_can_protocol/pn_can_protocol_2_0_0/utils" -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-utils

clean-utils:
	-$(RM) ./utils/crc.cyclo ./utils/crc.d ./utils/crc.o ./utils/crc.su

.PHONY: clean-utils

