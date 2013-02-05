# Makefile
###################################################
#	- Build HRRT_U
###################################################

BUILD_SUBDIRS = ecatx gen_delays_lib hrrt_osem3d norm_process air_ecat motion_distance gsmooth \
	e7_tools_HRRT compute_norm interfile_lib calcRingRoiRatio if2e7 gen_delays_main motion_correction gsmooth lmhistogram_mp

default: all

all build clean install:
	$(MAKE) 'TARGET=$@' 'SUBDIRS=$(BUILD_SUBDIRS)' subdirs

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

