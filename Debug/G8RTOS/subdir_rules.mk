################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
G8RTOS/%.obj: ../G8RTOS/%.s $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"C:/ti/ccs1200/ccs/tools/compiler/ti-cgt-arm_20.2.6.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 -me -Ooff --opt_for_speed=0 --include_path="C:/Users/evana/OneDrive/UF/2022Fall/EEL4745C/Pool" --include_path="C:/ti/TivaWare_C_Series-2.2.0.295" --include_path="C:/Users/evana/OneDrive/UF/2022Fall/EEL4745C/Pool/BoardSupport" --include_path="C:/Users/evana/OneDrive/UF/2022Fall/EEL4745C/Pool/BoardSupport/inc" --include_path="C:/Users/evana/OneDrive/UF/2022Fall/EEL4745C/Pool/G8RTOS" --include_path="C:/ti/ccs1200/ccs/tools/compiler/ti-cgt-arm_20.2.6.LTS/include" --define=ccs="ccs" --define=PART_TM4C123GH6PM -g --c99 --gcc --diag_warning=225 --diag_wrap=off --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="G8RTOS/$(basename $(<F)).d_raw" --obj_directory="G8RTOS" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

G8RTOS/%.obj: ../G8RTOS/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"C:/ti/ccs1200/ccs/tools/compiler/ti-cgt-arm_20.2.6.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 -me -Ooff --opt_for_speed=0 --include_path="C:/Users/evana/OneDrive/UF/2022Fall/EEL4745C/Pool" --include_path="C:/ti/TivaWare_C_Series-2.2.0.295" --include_path="C:/Users/evana/OneDrive/UF/2022Fall/EEL4745C/Pool/BoardSupport" --include_path="C:/Users/evana/OneDrive/UF/2022Fall/EEL4745C/Pool/BoardSupport/inc" --include_path="C:/Users/evana/OneDrive/UF/2022Fall/EEL4745C/Pool/G8RTOS" --include_path="C:/ti/ccs1200/ccs/tools/compiler/ti-cgt-arm_20.2.6.LTS/include" --define=ccs="ccs" --define=PART_TM4C123GH6PM -g --c99 --gcc --diag_warning=225 --diag_wrap=off --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="G8RTOS/$(basename $(<F)).d_raw" --obj_directory="G8RTOS" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


