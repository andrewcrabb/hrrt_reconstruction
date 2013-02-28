# Makefile
###################################################
#	- Build HRRT_U
###################################################

SUBDIRS = gen_delays_lib interfile_lib ecatx ecatx/utils hrrt_osem3d je_hrrt_osem3d norm_process air_ecat \
	motion_distance gsmooth e7_tools_HRRT compute_norm calcRingRoiRatio if2e7 \
	gen_delays_main motion_correction gsmooth lmhistogram_mp TX_TV3DReg

default: all

all build clean install:
	$(MAKE) 'TARGET=$@' 'SUBDIRS=$(SUBDIRS)' subdirs

subdirs:
	@echo "ruunning subdirs"
	@for dir in $(SUBDIRS); \
	  do if [ -d $$dir ]; \
	  then \
	     cd $$dir; \
	     echo Making $(TARGET) in $$dir; \
	     $(MAKE) $(TARGET); \
	     cd ..; \
	  fi; \
	done

