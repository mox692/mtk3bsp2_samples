################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../mtk3_bsp2/sysdepend/nxp_mcux/lib/libtk/cpu/core/armv8m/int_armv8m.c \
../mtk3_bsp2/sysdepend/nxp_mcux/lib/libtk/cpu/core/armv8m/wusec_armv8m.c 

OBJS += \
./mtk3_bsp2/sysdepend/nxp_mcux/lib/libtk/cpu/core/armv8m/int_armv8m.o \
./mtk3_bsp2/sysdepend/nxp_mcux/lib/libtk/cpu/core/armv8m/wusec_armv8m.o 

C_DEPS += \
./mtk3_bsp2/sysdepend/nxp_mcux/lib/libtk/cpu/core/armv8m/int_armv8m.d \
./mtk3_bsp2/sysdepend/nxp_mcux/lib/libtk/cpu/core/armv8m/wusec_armv8m.d 


# Each subdirectory must supply rules for building sources it contributes
mtk3_bsp2/sysdepend/nxp_mcux/lib/libtk/cpu/core/armv8m/%.o mtk3_bsp2/sysdepend/nxp_mcux/lib/libtk/cpu/core/armv8m/%.su mtk3_bsp2/sysdepend/nxp_mcux/lib/libtk/cpu/core/armv8m/%.cyclo: ../mtk3_bsp2/sysdepend/nxp_mcux/lib/libtk/cpu/core/armv8m/%.c mtk3_bsp2/sysdepend/nxp_mcux/lib/libtk/cpu/core/armv8m/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32H533xx -D_STM32CUBE_NUCLEO_H533_ -c -I../Core/Inc -I../Drivers/STM32H5xx_HAL_Driver/Inc -I../Drivers/STM32H5xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H5xx/Include -I../Drivers/CMSIS/Include -I"/Users/motoyuki.kimura/work/tron_play/mtk3bsp2_samples/IDE_Projects/mtk3bsp2_stm32h533/mtk3_bsp2" -I"/Users/motoyuki.kimura/work/tron_play/mtk3bsp2_samples/IDE_Projects/mtk3bsp2_stm32h533/mtk3_bsp2/config" -I"/Users/motoyuki.kimura/work/tron_play/mtk3bsp2_samples/IDE_Projects/mtk3bsp2_stm32h533/mtk3_bsp2/include" -I"/Users/motoyuki.kimura/work/tron_play/mtk3bsp2_samples/IDE_Projects/mtk3bsp2_stm32h533/mtk3_bsp2/mtkernel/kernel/knlinc" -I"/Users/motoyuki.kimura/work/tron_play/mtk3bsp2_samples/Components/can/include" -I"/Users/motoyuki.kimura/work/tron_play/mtk3bsp2_samples/Components/can/src" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-mtk3_bsp2-2f-sysdepend-2f-nxp_mcux-2f-lib-2f-libtk-2f-cpu-2f-core-2f-armv8m

clean-mtk3_bsp2-2f-sysdepend-2f-nxp_mcux-2f-lib-2f-libtk-2f-cpu-2f-core-2f-armv8m:
	-$(RM) ./mtk3_bsp2/sysdepend/nxp_mcux/lib/libtk/cpu/core/armv8m/int_armv8m.cyclo ./mtk3_bsp2/sysdepend/nxp_mcux/lib/libtk/cpu/core/armv8m/int_armv8m.d ./mtk3_bsp2/sysdepend/nxp_mcux/lib/libtk/cpu/core/armv8m/int_armv8m.o ./mtk3_bsp2/sysdepend/nxp_mcux/lib/libtk/cpu/core/armv8m/int_armv8m.su ./mtk3_bsp2/sysdepend/nxp_mcux/lib/libtk/cpu/core/armv8m/wusec_armv8m.cyclo ./mtk3_bsp2/sysdepend/nxp_mcux/lib/libtk/cpu/core/armv8m/wusec_armv8m.d ./mtk3_bsp2/sysdepend/nxp_mcux/lib/libtk/cpu/core/armv8m/wusec_armv8m.o ./mtk3_bsp2/sysdepend/nxp_mcux/lib/libtk/cpu/core/armv8m/wusec_armv8m.su

.PHONY: clean-mtk3_bsp2-2f-sysdepend-2f-nxp_mcux-2f-lib-2f-libtk-2f-cpu-2f-core-2f-armv8m

