CC = gcc

.SUFFIXES: .o .c .h

LDFLAGS = 
RM = rm -f
RANLIB  =
# set -g in MYCFLAGS
DEBUG =
BIN_DIR = ${HOME}/bin
SRCDIR = .
INCLUDE_DIR = .. 
CFLAGS = $(DEBUG) -c -I$(INCLUDE_DIR)
PROGRAMS = cti2analyze matcopy fix_mh matlist show_header sum_frames
LIBS = ../libecatx.a -lm
INSTALLDIR      = ../../bin

.c.o:
		$(CC) -c $(CFLAGS) ${MYCFLAGS}  $<

all : $(PROGRAMS)

cti2analyze  : cti2analyze.o 
	$(CC) $(LDFLAGS) ${MYCPPFLAGS} -o $@ cti2analyze.o $(LIBS)

fix_mh  : fix_mh.o 
	$(CC) $(LDFLAGS) ${MYCPPFLAGS} -o $@ fix_mh.o $(LIBS)

matcopy  : matcopy.o 
	$(CC) $(LDFLAGS) ${MYCPPFLAGS} -o $@ matcopy.o $(LIBS)

matlist  : matlist.o 
	$(CC) $(LDFLAGS) ${MYCPPFLAGS} -o $@ matlist.o $(LIBS)

show_header  : show_header.o 
	$(CC) $(LDFLAGS) ${MYCPPFLAGS} -o $@ show_header.o $(LIBS)

sum_frames  : sum_frames.o 
	$(CC) $(LDFLAGS) ${MYCPPFLAGS} -o $@ sum_frames.o $(LIBS)

clean:
	$(RM) *.o $(PROGRAMS)
install:	$(PROGRAMS)
	cp $(PROGRAMS) $(INSTALLDIR); chmod +x ${INSTALLDIR}/*
depend:
	makedepend $(DEP_FLAGS) $(SRCDIR)/*.c 
# DO NOT DELETE

