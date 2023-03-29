README.TXT

Fairchild Semiconductor FUSB305 Linux Platform Driver Integration Notes
_______________________________________________________________________________
Device Tree
Currently, the driver requires the use of Device Tree in order to 
function with the I2C bus and the required GPIO pins. Modify the 
following Device Tree snippet to specify resources specific to your 
system and include them in your kernel's Device Tree. The FUSB305 
requires a minimum of 2 GPIO pins, a valid I2C bus, and I2C slave 
address 0x22.

> kernel/arch/arm/boot/dts/qcom/msm8994.dtsi

In "i2c_6: i2c@f9928000" group, change "disabled" to "okay".

At end of file, add:

&soc {
	i2c@f9928000 {
		fusb3601@25 {
			compatible = "fairchild,fusb3601";
			reg = <0x25>;
			status = "okay";
			interrupt-parent = <&msm_gpio>;          //IRQ parent should be same node as INT_N gpio_target
			interrupts = <39 8>;                     //<gpio# IRQ_TYPE_LEVEL_LOW>
			fairchild,int_n = <&msm_gpio 39 8>;      //<&gpio_target gpio# ActiveLow>
			fairchild,vbus5v = <&pm8994_gpios 4 0>;  //<&gpio_target gpio# ActiveHigh>
			fairchild,dbg_sm = <&pmi8994_gpios 7 0>; //This is for debug out
		}; 
	};        
};
> kernel/arch/arm/boot/dts/qcom/msm8994-pinctrl.dtsi

In "pmx_i2c_6" group, set "bias-disable" to "bias-pull-up".  (Optional w/ onboard pullups?)

> kernel/arch/arm/boot/dts/qcom/apq8094-dragonboard.dtsi

In "gpio@c600" group with comment "GPIO 7 - EDU_GPIO3", add line: "qcom,master-en = <1>;".

> kernel/arch/arm/boot/dts/qcom/msm-pmi8994.dtsi

In "gpio@c600" group, change "disabled" to "okay".

_______________________________________________________________________________

Compilation/Makefile
	You must define the preprocessor macro "FSC_PLATFORM_LINUX" in order to 
	pull in the correct typedefs.
	
	The following example snippet is from a Makefile expecting the 
	following directory structure:
	
	path/to/MakefileDir/
		|---- Makefile
		|---- Platform_Linux/
		|---- core/
			|---- vdm/
				|---- DisplayPort/
	
	Makefile
	/*********************************************************************/
	# Required flag to configure the core to operate with the Linux kernel
	ccflags-$(CONFIG_FUSB_30X) += -DFSC_PLATFORM_LINUX
	# Optional flag to enable debug/hostcomm interface and functionality
	ccflags-$(CONFIG_FUSB_30X) += -DFSC_DEBUG
	
	# The following flags are used to configure which features are compiled in,
	# and the allowed combinations are:
	#	FSC_HAVE_SRC - Source only
	#	FSC_HAVE_SNK - Sink only
	#	FSC_HAVE_SRC, FSC_HAVE_SNK - Source or sink configurable
	#	FSC_HAVE_SRC, FSC_HAVE_SNK, FSC_HAVE_DRP - DRP capable source or sink
	#	FSC_HAVE_ACCMODE - Accessory mode. Requires FSC_HAVE_SRC.
	#	FSC_HAVE_VDM - Vendor Defined Message (VDM) support.
	#	FSC_HAVE_DP - Display Port (DP) support. Requires FSC_HAVE_VDM.
	ccflags-$(CONFIG_FUSB_30X) += -DFSC_HAVE_SRC
	ccflags-$(CONFIG_FUSB_30X) += -DFSC_HAVE_SNK
	ccflags-$(CONFIG_FUSB_30X) += -DFSC_HAVE_DRP
	ccflags-$(CONFIG_FUSB_30X) += -DFSC_HAVE_ACCMODE
	ccflags-$(CONFIG_FUSB_30X) += -DFSC_HAVE_VDM
	ccflags-$(CONFIG_FUSB_30X) += -DFSC_HAVE_DP

  The remainder of the makefile can be seen in Platform_Linux/Makefile.

  To include it in the kernel build, append the following line to
  kernel/drivers/power/Makefile

  obj-y += fusb3601_firmware/Platform_Linux/
	/*********************************************************************/
	
_______________________________________________________________________________
SysFs HostComm/Debug interface
	When FSC_HAVE_DEBUG is defined, this driver will attempt to create sysfs files to provide
	user-space access to driver functionality. If it is not defined, then these files will not exist.
	You can find these files in:
		/sys/class/i2c-dev/i2c-<i2c bus number>/device/<i2c bus number>-0022/
		|---- fairchild,dbg_sm (access to debug GPIO)
		|---- fairchild,int_n (access to INT_N GPIO)
		|---- fairchild,vbus5v (access to VBus5V GPIO)
		|---- control/
			|---- fusb30x_hostcomm (Provides access to HostComm interface. Examine Platform_Linux/hostcomm.(h|c) for details.)
			|---- pd_state_log (read this file to fetch and display the driver's PD State Log)
			|---- typec_state_log (read this file to fetch and display the driver's Type-C State Log)
			|---- reinitialize (read this file to reinitialize the FUSB305)
			
	Usage examples:
		PD State Log:
		$ cat pd_state_log
			PD State Log has <0-12> entries:
			[sec.ms]	State		Detail
			[sec.ms]	State		Detail
			...
		
		Type-C State Log:
		$ cat typec_state_log
			Type-C State Log has 6 entries:
			[sec.ms]        State
			[sec.ms]        State
			...
		
		Reinitialize the Device:
		$ cat reinitialize
			FUSB305 Reinitialized!
		
		Interact with HostComm interface:
		To use fusb30x_hostcomm, first make sure you have write permission:
			$ sudo chmod 666 fusb30x_hostcomm
		Then write a command to it to perform an action, and read it to get the result.
		The expected response for most commands is the command issued as the first byte, and zeros thereafter (unless the command is supposed to return data). If the 2nd byte is 0x01, then the specified command is not supported.
		
		Send a hard reset:
			$ echo -n "0xAE" > fusb30x_hostcomm
			$ cat fusb30x_hostcomm
				0xae 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
		
		I2C read on register 0x01:
			$ echo -n "0xC0 0x01" > fusb30x_hostcomm
			$ cat fusb30x_hostcomm
				0x80 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
				
			This I2C read returned a value of 0x80 from register 0x01 on the FUSB305.
		
		I2C write to a register (Note - example only, it's typically a bad idea to write to registers by hand):
			Write syntax is: "Write-CMD Register NumBytes Data0 Data1 Data2..."
			$ echo -n "0xC1 0x01 0x03 0x01 0x02 0x03" > fusb30x_hostcomm
			$ $ cat fusb30x_hostcomm
				First byte will be 0x00 on failure, 0x01 on success - all other bytes are 0x00.
		
		Please see Platform_Linux/hostcomm.h for the command mapping.
