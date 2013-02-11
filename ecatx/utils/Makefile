CC = gcc

.SUFFIXES: .o .c .h

.c.o:
		$(CC) -c $(CFLAGS) ${MYCFLAGS}  $<
LDFLAGS = 
RM = rm -f
RANLIB  =
DEBUG = -g
BIN_DIR = ${HOME}/bin
SRCDIR = .
INCLUDE_DIR = .. 
CFLAGS = $(DEBUG) -c -I$(INCLUDE_DIR)
PROGRAMS = cti2analyze matcopy fix_mh matlist show_header sum_frames
LIBS = ../libecatx.a -lm
INSTALLDIR      = ../../bin

all : $(PROGRAMS)
cti2analyze  : cti2analyze.o 
	$(CC) $(LDFLAGS) -o $@ cti2analyze.o $(LIBS)
fix_mh  : fix_mh.o 
	$(CC) $(LDFLAGS) -o $@ fix_mh.o $(LIBS)
matcopy  : matcopy.o 
	$(CC) $(LDFLAGS) -o $@ matcopy.o $(LIBS)
show_header  : show_header.o 
	$(CC) $(LDFLAGS) -o $@ show_header.o $(LIBS)
sum_frames  : sum_frames.o 
	$(CC) $(LDFLAGS) -o $@ sum_frames.o $(LIBS)

clean:
	$(RM) *.o $(PROGRAMS)
install:	$(PROGRAMS)
	mv $(PROGRAMS) $(INSTALLDIR)
depend:
	makedepend $(DEP_FLAGS) $(SRCDIR)/*.c 
# DO NOT DELETE

