###################################################
# Makefile
#	- Build libraries for e7 utilities
###################################################

# Libraries
LIBATTENDIR = libatten
LIBCOMMONDIR = libcommon
LIBCONVERTDIR = libconvert
LIBECAT7DIR = libecat7
LIBEXCEPTIONDIR = libexception
LIBFFTDIR = libfft
LIBFILTERDIR = libfilter
LIBIMAGEDIR = libimage
LIBINTERFILEDIR = libinterfile
LIBIPCDIR = libipc
LIBPARSERDIR = libparser
LIBSINOGRAMDIR = libsinogram
E7_SUBDIRS = $(LIBATTENDIR) $(LIBCOMMONDIR) $(LIBCONVERTDIR) $(LIBECAT7DIR) $(LIBEXCEPTIONDIR) $(LIBFFTDIR) $(LIBFILTERDIR) $(LIBIMAGEDIR) $(LIBINTERFILEDIR) $(LIBIPCDIR) $(LIBPARSERDIR) $(LIBSINOGRAMDIR)

# E7 Tools
E7_FWD_PRJ_DIR = e7_fwd_prj
E7_SINO_PRJ_DIR = e7_sino_prj
E7_ATTEN_PRJ_DIR = e7_atten_prj
GM_CHECK_PRJ_DIR = gm_check_prj
E7_EXEDIRS = $(E7_FWD_PRJ_DIR) $(E7_SINO_PRJ_DIR) $(E7_ATTEN_PRJ_DIR) $(GM_CHECK_PRJ_DIR)

E7_BUILD_SUBDIRS = $(E7_SUBDIRS) $(E7_EXEDIRS)

# Help/Tips
help:
	@echo "The following are the valid targets for this Makefile:"
	@echo "	libs - build libraries"
	@echo "	install_libs - install in ./LinkLibs"
	@echo "	e7 - build e7 tools"
	@echo "	install_e7 - install in ./E7_Bin"
	@echo "	clean - delete object files etc."
	@echo "	clean_all - delete object files, object libraries (not those in LinkLibs), executables (not those in ./E7_Bin)"
	@echo "	help - this listing"
	@echo ""
	@echo "To build 'e7_tools' from scratch  - scatter libraries in ./LinkLibsScatter"
	@echo "	1. make libs"
	@echo "	2. make install_libs"
	@echo "	3. make e7"
	@echo "	4. make install_e7"
	@echo "	5. make clean_all"

# All
all install:
	$(MAKE) 'TARGET=$@' 'E7_SUBDIRS=$(E7_BUILD_SUBDIRS)' subdirs

subdirs:
	@for dir in $(E7_SUBDIRS); \
	  do if [ -d $$dir ]; \
	    then \
	      cd $$dir; \
	      echo Making $(TARGET) in $$dir; \
	      $(MAKE) $(TARGET); \
	      cd ..; \
	    fi; \
	 done

# Libraries only
libs:
	for i in $(E7_SUBDIRS) ; do \
		( cd $$i ; make ) ; \
	done

# Install libraries
install_libs:
	for i in $(E7_SUBDIRS) ; do \
		( cd $$i ; make install ) ; \
	done

# E7_tools only
e7:
	for i in $(E7_EXEDIRS) ; do \
		( cd $$i ; make ) ; \
	done

# Install e7_tools
install_e7:
	for i in $(E7_EXEDIRS) ; do \
		( cd $$i ; make install ) ; \
	done

# Rules to delete *.o & *~
clean:
	rm -f *.o *~
	for i in $(E7_SUBDIRS) $(E7_EXEDIRS) ; do \
		( cd $$i ; make clean) ; \
	done

# Rules to delete *.a, *.so, *.o & *~
clean_all:
	rm -f *.o *~
	for i in $(E7_SUBDIRS) $(E7_EXEDIRS) ; do \
		( cd $$i ; make clean_all) ; \
	done
