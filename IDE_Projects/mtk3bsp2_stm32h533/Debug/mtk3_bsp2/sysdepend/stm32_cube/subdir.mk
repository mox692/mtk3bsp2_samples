################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../mtk3_bsp2/sysdepend/stm32_cube/devinit.c \
../mtk3_bsp2/sysdepend/stm32_cube/hw_setting.c \
../mtk3_bsp2/sysdepend/stm32_cube/power_save.c 

OBJS += \
./mtk3_bsp2/sysdepend/stm32_cube/devinit.o \
./mtk3_bsp2/sysdepend/stm32_cube/hw_setting.o \
./mtk3_bsp2/sysdepend/stm32_cube/power_save.o 

C_DEPS += \
./mtk3_bsp2/sysdepend/stm32_cube/devinit.d \
./mtk3_bsp2/sysdepend/stm32_cube/hw_setting.d \
./mtk3_bsp2/sysdepend/stm32_cube/power_save.d 


# Each subdirectory must supply rules for building sources it contributes
mtk3_bsp2/sysdepend/stm32_cube/%.o mtk3_bsp2/sysdepend/stm32_cube/%.su mtk3_bsp2/sysdepend/stm32_cube/%.cyclo: ../mtk3_bsp2/sysdepend/stm32_cube/%.c mtk3_bsp2/sysdepend/stm32_cube/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32H533xx -D_STM32CUBE_NUCLEO_H533_ -c -I../Core/Inc -I../Drivers/STM32H5xx_HAL_Driver/Inc -I../Drivers/STM32H5xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H5xx/Include -I../Drivers/CMSIS/Include -I"/Users/motoyuki.kimura/work/tron_play/mtk3bsp2_samples/IDE_Projects/mtk3bsp2_stm32h533/mtk3_bsp2" -I"/Users/motoyuki.kimura/work/tron_play/mtk3bsp2_samples/IDE_Projects/mtk3bsp2_stm32h533/mtk3_bsp2/config" -I"/Users/motoyuki.kimura/work/tron_play/mtk3bsp2_samples/IDE_Projects/mtk3bsp2_stm32h533/mtk3_bsp2/include" -I"/Users/motoyuki.kimura/work/tron_play/mtk3bsp2_samples/IDE_Projects/mtk3bsp2_stm32h533/mtk3_bsp2/mtkernel/kernel/knlinc" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-mtk3_bsp2-2f-sysdepend-2f-stm32_cube

clean-mtk3_bsp2-2f-sysdepend-2f-stm32_cube:
	-$(RM) ./mtk3_bsp2/sysdepend/stm32_cube/devinit.cyclo ./mtk3_bsp2/sysdepend/stm32_cube/devinit.d ./mtk3_bsp2/sysdepend/stm32_cube/devinit.o ./mtk3_bsp2/sysdepend/stm32_cube/devinit.su ./mtk3_bsp2/sysdepend/stm32_cube/hw_setting.cyclo ./mtk3_bsp2/sysdepend/stm32_cube/hw_setting.d ./mtk3_bsp2/sysdepend/stm32_cube/hw_setting.o ./mtk3_bsp2/sysdepend/stm32_cube/hw_setting.su ./mtk3_bsp2/sysdepend/stm32_cube/power_save.cyclo ./mtk3_bsp2/sysdepend/stm32_cube/power_save.d ./mtk3_bsp2/sysdepend/stm32_cube/power_save.o ./mtk3_bsp2/sysdepend/stm32_cube/power_save.su

.PHONY: clean-mtk3_bsp2-2f-sysdepend-2f-stm32_cube

