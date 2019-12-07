#
_XDCBUILDCOUNT = 
ifneq (,$(findstring path,$(_USEXDCENV_)))
override XDCPATH = /opt/ti/simplelink_cc2640r2_sdk_3_30_00_20/source;/opt/ti/simplelink_cc2640r2_sdk_3_30_00_20/kernel/tirtos/packages
override XDCROOT = /home/jackie/ti/ccs920/xdctools_3_51_03_28_core
override XDCBUILDCFG = ./config.bld
endif
ifneq (,$(findstring args,$(_USEXDCENV_)))
override XDCARGS = 
override XDCTARGETS = 
endif
#
ifeq (0,1)
PKGPATH = /opt/ti/simplelink_cc2640r2_sdk_3_30_00_20/source;/opt/ti/simplelink_cc2640r2_sdk_3_30_00_20/kernel/tirtos/packages;/home/jackie/ti/ccs920/xdctools_3_51_03_28_core/packages;..
HOSTOS = Linux
endif
