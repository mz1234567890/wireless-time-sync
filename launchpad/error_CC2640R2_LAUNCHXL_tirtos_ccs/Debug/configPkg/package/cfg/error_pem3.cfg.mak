# invoke SourceDir generated makefile for error.pem3
error.pem3: .libraries,error.pem3
.libraries,error.pem3: package/cfg/error_pem3.xdl
	$(MAKE) -f /home/jackie/ti/workspaces/workspace_1/error_CC2640R2_LAUNCHXL_tirtos_ccs/src/makefile.libs

clean::
	$(MAKE) -f /home/jackie/ti/workspaces/workspace_1/error_CC2640R2_LAUNCHXL_tirtos_ccs/src/makefile.libs clean

