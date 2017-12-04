# Makefile
###################################################
#	- Build HRRT_U
###################################################

SUBDIRS = gen_delays_lib interfile_lib ecatx ecatx/utils \
	hrrt_osem3d je_hrrt_osem3d \
	norm_process air_ecat motion_distance gsmooth e7_tools_HRRT compute_norm \
	calcRingRoiRatio if2e7 gen_delays_main motion_correction gsmooth lmhistogram_mp \
	TX_TV3DReg \
	common
# ahc 3/7/31 volume_reslice has unresolved call to matrix_flip()
# which is declared in ecatx/matrix_utils.h but not defined,
# and defined in air_ecat/ecat2air.c but not included in a library.
# volume_reslice
# AIR5.3.0 

default: all

all build clean clean_all install:
	$(MAKE) 'TARGET=$@' 'SUBDIRS=$(SUBDIRS)' subdirs

subdirs:
	@echo "ruunning subdirs"
	@for dir in $(SUBDIRS); \
	  do if [ -d $$dir ]; \
	  then \
	     cd $$dir; \
	     echo Making $(TARGET) in $$dir; \
	     $(MAKE) $(TARGET); \
	     cd -; \
	  fi; \
	done

