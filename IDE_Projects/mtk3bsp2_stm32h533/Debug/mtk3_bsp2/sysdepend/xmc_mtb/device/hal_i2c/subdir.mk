################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../mtk3_bsp2/sysdepend/xmc_mtb/device/hal_i2c/hal_i2c.c 

OBJS += \
./mtk3_bsp2/sysdepend/xmc_mtb/device/hal_i2c/hal_i2c.o 

C_DEPS += \
./mtk3_bsp2/sysdepend/xmc_mtb/device/hal_i2c/hal_i2c.d 


# Each subdirectory must supply rules for building sources it contributes
mtk3_bsp2/sysdepend/xmc_mtb/device/hal_i2c/%.o mtk3_bsp2/sysdepend/xmc_mtb/device/hal_i2c/%.su mtk3_bsp2/sysdepend/xmc_mtb/device/hal_i2c/%.cyclo: ../mtk3_bsp2/sysdepend/xmc_mtb/device/hal_i2c/%.c mtk3_bsp2/sysdepend/xmc_mtb/device/hal_i2c/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32H533xx -D_STM32CUBE_NUCLEO_H533_ -c -I../Core/Inc -I../Drivers/STM32H5xx_HAL_Driver/Inc -I../Drivers/STM32H5xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H5xx/Include -I../Drivers/CMSIS/Include -I"/Users/motoyuki.kimura/work/tron_play/mtk3bsp2_samples/IDE_Projects/mtk3bsp2_stm32h533/mtk3_bsp2" -I"/Users/motoyuki.kimura/work/tron_play/mtk3bsp2_samples/IDE_Projects/mtk3bsp2_stm32h533/mtk3_bsp2/config" -I"/Users/motoyuki.kimura/work/tron_play/mtk3bsp2_samples/IDE_Projects/mtk3bsp2_stm32h533/mtk3_bsp2/include" -I"/Users/motoyuki.kimura/work/tron_play/mtk3bsp2_samples/IDE_Projects/mtk3bsp2_stm32h533/mtk3_bsp2/mtkernel/kernel/knlinc" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-mtk3_bsp2-2f-sysdepend-2f-xmc_mtb-2f-device-2f-hal_i2c

clean-mtk3_bsp2-2f-sysdepend-2f-xmc_mtb-2f-device-2f-hal_i2c:
	-$(RM) ./mtk3_bsp2/sysdepend/xmc_mtb/device/hal_i2c/hal_i2c.cyclo ./mtk3_bsp2/sysdepend/xmc_mtb/device/hal_i2c/hal_i2c.d ./mtk3_bsp2/sysdepend/xmc_mtb/device/hal_i2c/hal_i2c.o ./mtk3_bsp2/sysdepend/xmc_mtb/device/hal_i2c/hal_i2c.su

.PHONY: clean-mtk3_bsp2-2f-sysdepend-2f-xmc_mtb-2f-device-2f-hal_i2c

