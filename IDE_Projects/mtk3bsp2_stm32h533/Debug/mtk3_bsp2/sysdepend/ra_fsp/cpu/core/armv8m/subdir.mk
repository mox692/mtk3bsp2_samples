################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../mtk3_bsp2/sysdepend/ra_fsp/cpu/core/armv8m/cpu_cntl.c \
../mtk3_bsp2/sysdepend/ra_fsp/cpu/core/armv8m/exc_hdr.c \
../mtk3_bsp2/sysdepend/ra_fsp/cpu/core/armv8m/interrupt.c \
../mtk3_bsp2/sysdepend/ra_fsp/cpu/core/armv8m/sys_start.c 

S_UPPER_SRCS += \
../mtk3_bsp2/sysdepend/ra_fsp/cpu/core/armv8m/dispatch.S 

OBJS += \
./mtk3_bsp2/sysdepend/ra_fsp/cpu/core/armv8m/cpu_cntl.o \
./mtk3_bsp2/sysdepend/ra_fsp/cpu/core/armv8m/dispatch.o \
./mtk3_bsp2/sysdepend/ra_fsp/cpu/core/armv8m/exc_hdr.o \
./mtk3_bsp2/sysdepend/ra_fsp/cpu/core/armv8m/interrupt.o \
./mtk3_bsp2/sysdepend/ra_fsp/cpu/core/armv8m/sys_start.o 

S_UPPER_DEPS += \
./mtk3_bsp2/sysdepend/ra_fsp/cpu/core/armv8m/dispatch.d 

C_DEPS += \
./mtk3_bsp2/sysdepend/ra_fsp/cpu/core/armv8m/cpu_cntl.d \
./mtk3_bsp2/sysdepend/ra_fsp/cpu/core/armv8m/exc_hdr.d \
./mtk3_bsp2/sysdepend/ra_fsp/cpu/core/armv8m/interrupt.d \
./mtk3_bsp2/sysdepend/ra_fsp/cpu/core/armv8m/sys_start.d 


# Each subdirectory must supply rules for building sources it contributes
mtk3_bsp2/sysdepend/ra_fsp/cpu/core/armv8m/%.o mtk3_bsp2/sysdepend/ra_fsp/cpu/core/armv8m/%.su mtk3_bsp2/sysdepend/ra_fsp/cpu/core/armv8m/%.cyclo: ../mtk3_bsp2/sysdepend/ra_fsp/cpu/core/armv8m/%.c mtk3_bsp2/sysdepend/ra_fsp/cpu/core/armv8m/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32H533xx -D_STM32CUBE_NUCLEO_H533_ -c -I../Core/Inc -I../Drivers/STM32H5xx_HAL_Driver/Inc -I../Drivers/STM32H5xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H5xx/Include -I../Drivers/CMSIS/Include -I"/Users/motoyuki.kimura/work/tron_play/mtk3bsp2_samples/IDE_Projects/mtk3bsp2_stm32h533/mtk3_bsp2" -I"/Users/motoyuki.kimura/work/tron_play/mtk3bsp2_samples/IDE_Projects/mtk3bsp2_stm32h533/mtk3_bsp2/config" -I"/Users/motoyuki.kimura/work/tron_play/mtk3bsp2_samples/IDE_Projects/mtk3bsp2_stm32h533/mtk3_bsp2/include" -I"/Users/motoyuki.kimura/work/tron_play/mtk3bsp2_samples/IDE_Projects/mtk3bsp2_stm32h533/mtk3_bsp2/mtkernel/kernel/knlinc" -I"/Users/motoyuki.kimura/work/tron_play/mtk3bsp2_samples/Components/can/include" -I"/Users/motoyuki.kimura/work/tron_play/mtk3bsp2_samples/Components/can/src" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
mtk3_bsp2/sysdepend/ra_fsp/cpu/core/armv8m/%.o: ../mtk3_bsp2/sysdepend/ra_fsp/cpu/core/armv8m/%.S mtk3_bsp2/sysdepend/ra_fsp/cpu/core/armv8m/subdir.mk
	arm-none-eabi-gcc -mcpu=cortex-m33 -g3 -DDEBUG -D_STM32CUBE_NUCLEO_H533_ -c -I"/Users/motoyuki.kimura/work/tron_play/mtk3bsp2_samples/IDE_Projects/mtk3bsp2_stm32h533/mtk3_bsp2" -I"/Users/motoyuki.kimura/work/tron_play/mtk3bsp2_samples/IDE_Projects/mtk3bsp2_stm32h533/mtk3_bsp2/config" -I"/Users/motoyuki.kimura/work/tron_play/mtk3bsp2_samples/IDE_Projects/mtk3bsp2_stm32h533/mtk3_bsp2/include" -I"/Users/motoyuki.kimura/work/tron_play/mtk3bsp2_samples/IDE_Projects/mtk3bsp2_stm32h533/mtk3_bsp2/mtkernel/kernel/knlinc" -x assembler-with-cpp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@" "$<"

clean: clean-mtk3_bsp2-2f-sysdepend-2f-ra_fsp-2f-cpu-2f-core-2f-armv8m

clean-mtk3_bsp2-2f-sysdepend-2f-ra_fsp-2f-cpu-2f-core-2f-armv8m:
	-$(RM) ./mtk3_bsp2/sysdepend/ra_fsp/cpu/core/armv8m/cpu_cntl.cyclo ./mtk3_bsp2/sysdepend/ra_fsp/cpu/core/armv8m/cpu_cntl.d ./mtk3_bsp2/sysdepend/ra_fsp/cpu/core/armv8m/cpu_cntl.o ./mtk3_bsp2/sysdepend/ra_fsp/cpu/core/armv8m/cpu_cntl.su ./mtk3_bsp2/sysdepend/ra_fsp/cpu/core/armv8m/dispatch.d ./mtk3_bsp2/sysdepend/ra_fsp/cpu/core/armv8m/dispatch.o ./mtk3_bsp2/sysdepend/ra_fsp/cpu/core/armv8m/exc_hdr.cyclo ./mtk3_bsp2/sysdepend/ra_fsp/cpu/core/armv8m/exc_hdr.d ./mtk3_bsp2/sysdepend/ra_fsp/cpu/core/armv8m/exc_hdr.o ./mtk3_bsp2/sysdepend/ra_fsp/cpu/core/armv8m/exc_hdr.su ./mtk3_bsp2/sysdepend/ra_fsp/cpu/core/armv8m/interrupt.cyclo ./mtk3_bsp2/sysdepend/ra_fsp/cpu/core/armv8m/interrupt.d ./mtk3_bsp2/sysdepend/ra_fsp/cpu/core/armv8m/interrupt.o ./mtk3_bsp2/sysdepend/ra_fsp/cpu/core/armv8m/interrupt.su ./mtk3_bsp2/sysdepend/ra_fsp/cpu/core/armv8m/sys_start.cyclo ./mtk3_bsp2/sysdepend/ra_fsp/cpu/core/armv8m/sys_start.d ./mtk3_bsp2/sysdepend/ra_fsp/cpu/core/armv8m/sys_start.o ./mtk3_bsp2/sysdepend/ra_fsp/cpu/core/armv8m/sys_start.su

.PHONY: clean-mtk3_bsp2-2f-sysdepend-2f-ra_fsp-2f-cpu-2f-core-2f-armv8m

