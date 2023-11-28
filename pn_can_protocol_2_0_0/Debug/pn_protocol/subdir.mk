################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../pn_protocol/CAN_queue.c \
../pn_protocol/pn_can_protocol.c \
../pn_protocol/pn_can_protocol_test.c \
../pn_protocol/pn_can_sync_layer.c \
../pn_protocol/pn_can_sync_layer_test.c 

OBJS += \
./pn_protocol/CAN_queue.o \
./pn_protocol/pn_can_protocol.o \
./pn_protocol/pn_can_protocol_test.o \
./pn_protocol/pn_can_sync_layer.o \
./pn_protocol/pn_can_sync_layer_test.o 

C_DEPS += \
./pn_protocol/CAN_queue.d \
./pn_protocol/pn_can_protocol.d \
./pn_protocol/pn_can_protocol_test.d \
./pn_protocol/pn_can_sync_layer.d \
./pn_protocol/pn_can_sync_layer_test.d 


# Each subdirectory must supply rules for building sources it contributes
pn_protocol/%.o pn_protocol/%.su pn_protocol/%.cyclo: ../pn_protocol/%.c pn_protocol/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I"C:/Users/peter/OneDrive/Desktop/Github/pn_can_protocol/pn_can_protocol_2_0_0/pn_protocol" -I"C:/Users/peter/OneDrive/Desktop/Github/pn_can_protocol/pn_can_protocol_2_0_0/utils" -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-pn_protocol

clean-pn_protocol:
	-$(RM) ./pn_protocol/CAN_queue.cyclo ./pn_protocol/CAN_queue.d ./pn_protocol/CAN_queue.o ./pn_protocol/CAN_queue.su ./pn_protocol/pn_can_protocol.cyclo ./pn_protocol/pn_can_protocol.d ./pn_protocol/pn_can_protocol.o ./pn_protocol/pn_can_protocol.su ./pn_protocol/pn_can_protocol_test.cyclo ./pn_protocol/pn_can_protocol_test.d ./pn_protocol/pn_can_protocol_test.o ./pn_protocol/pn_can_protocol_test.su ./pn_protocol/pn_can_sync_layer.cyclo ./pn_protocol/pn_can_sync_layer.d ./pn_protocol/pn_can_sync_layer.o ./pn_protocol/pn_can_sync_layer.su ./pn_protocol/pn_can_sync_layer_test.cyclo ./pn_protocol/pn_can_sync_layer_test.d ./pn_protocol/pn_can_sync_layer_test.o ./pn_protocol/pn_can_sync_layer_test.su

.PHONY: clean-pn_protocol

