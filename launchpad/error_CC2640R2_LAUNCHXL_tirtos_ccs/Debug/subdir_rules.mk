################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
%.obj: ../%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: ARM Compiler'
	"/home/jackie/ti/ccs920/ccs/tools/compiler/ti-cgt-arm_18.12.3.LTS/bin/armcl" -mv7M3 --code_state=16 --float_support=vfplib -me --include_path="/home/jackie/ti/workspaces/workspace_1/error_CC2640R2_LAUNCHXL_tirtos_ccs" --include_path="/opt/ti/simplelink_cc2640r2_sdk_3_30_00_20/source/ti/posix/ccs" --include_path="/home/jackie/ti/ccs920/ccs/tools/compiler/ti-cgt-arm_18.12.3.LTS/include" --define=DeviceFamily_CC26X0R2 -g --diag_warning=225 --diag_warning=255 --diag_wrap=off --display_error_number --gen_func_subsections=on --preproc_with_compile --preproc_dependency="$(basename $(<F)).d_raw" $(GEN_OPTS__FLAG) "$(shell echo $<)"
	@echo 'Finished building: "$<"'
	@echo ' '

build-1610498527:
	@$(MAKE) --no-print-directory -Onone -f subdir_rules.mk build-1610498527-inproc

build-1610498527-inproc: ../error.cfg
	@echo 'Building file: "$<"'
	@echo 'Invoking: XDCtools'
	"/home/jackie/ti/ccs920/xdctools_3_51_03_28_core/xs" --xdcpath="/opt/ti/simplelink_cc2640r2_sdk_3_30_00_20/source;/opt/ti/simplelink_cc2640r2_sdk_3_30_00_20/kernel/tirtos/packages;" xdc.tools.configuro -o configPkg -t ti.targets.arm.elf.M3 -p ti.platforms.simplelink:CC2640R2F -r release -c "/home/jackie/ti/ccs920/ccs/tools/compiler/ti-cgt-arm_18.12.3.LTS" --compileOptions "-mv7M3 --code_state=16 --float_support=vfplib -me --include_path=\"/home/jackie/ti/workspaces/workspace_1/error_CC2640R2_LAUNCHXL_tirtos_ccs\" --include_path=\"/opt/ti/simplelink_cc2640r2_sdk_3_30_00_20/source/ti/posix/ccs\" --include_path=\"/home/jackie/ti/ccs920/ccs/tools/compiler/ti-cgt-arm_18.12.3.LTS/include\" --define=DeviceFamily_CC26X0R2 -g --diag_warning=225 --diag_warning=255 --diag_wrap=off --display_error_number --gen_func_subsections=on  " "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

configPkg/linker.cmd: build-1610498527 ../error.cfg
configPkg/compiler.opt: build-1610498527
configPkg/: build-1610498527


