################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
/Users/motoyuki.kimura/work/tron_play/mtk3bsp2_samples/Components/can/src/can.c 

OBJS += \
./can_driver/src/can.o 

C_DEPS += \
./can_driver/src/can.d 


# Each subdirectory must supply rules for building sources it contributes
can_driver/src/can.o: /Users/motoyuki.kimura/work/tron_play/mtk3bsp2_samples/Components/can/src/can.c can_driver/src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32H723xx -D_STM32CUBE_NUCLEO_H723_ -c -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I"/Users/motoyuki.kimura/work/tron_play/mtk3bsp2_samples/IDE_Projects/mtk3bsp2_stm32h723/mtk3_bsp2" -I"/Users/motoyuki.kimura/work/tron_play/mtk3bsp2_samples/IDE_Projects/mtk3bsp2_stm32h723/mtk3_bsp2/config" -I"/Users/motoyuki.kimura/work/tron_play/mtk3bsp2_samples/IDE_Projects/mtk3bsp2_stm32h723/mtk3_bsp2/include" -I"/Users/motoyuki.kimura/work/tron_play/mtk3bsp2_samples/IDE_Projects/mtk3bsp2_stm32h723/mtk3_bsp2/mtkernel/kernel/knlinc" -I"/Users/motoyuki.kimura/work/tron_play/mtk3bsp2_samples/Components/can/include" -I"/Users/motoyuki.kimura/work/tron_play/mtk3bsp2_samples/Components/can/src" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-can_driver-2f-src

clean-can_driver-2f-src:
	-$(RM) ./can_driver/src/can.cyclo ./can_driver/src/can.d ./can_driver/src/can.o ./can_driver/src/can.su

.PHONY: clean-can_driver-2f-src

