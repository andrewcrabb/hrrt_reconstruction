# CC = gcc -arch x86_64
CC = gcc

.SUFFIXES: .o .c .h

LDFLAGS = -I/usr/local/air/include -L/usr/local/air/lib
RM = rm -f
RANLIB  =
# DEBUG = -g
BIN_DIR = ${HOME}/bin
SRCDIR = .
INCLUDE_DIR = ../include
LIB_DIR = ../lib
CFLAGS = $(DEBUG) -c -I$(INCLUDE_DIR) -I/usr/include
PROGRAMS = ecat_alignlinear ecat_make_air ecat_reslice
LIB_OBJS = ecat2air.o air2ecat.o ecat_map_value.o ecat_do_alignlinear.o \
	ecat_do_reslice.o

.c.o:
		$(CC) $(CFLAGS)  $<

LIBS = ../ecatx/libecatx.a -lRPW -lm

all : $(PROGRAMS)
ecat_alignlinear : ecat_alignlinear.o $(LIB_OBJS)
	$(CC) $(LDFLAGS) -o $@ ecat_alignlinear.o $(LIB_OBJS) $(LIBS)

ecat_make_air : ecat_make_air.o $(LIB_OBJS) 
	$(CC) $(LDFLAGS) -o $@ ecat_make_air.o $(LIB_OBJS) $(LIBS)

ecat_reslice : ecat_reslice.o $(LIB_OBJS) 
	$(CC) $(LDFLAGS) -o $@ $@.o $(LIB_OBJS) $(LIBS)

$(LIB) : $(LIB_OBJS)
	ar r $(LIB) $(LIB_OBJS)
	ranlib $(LIB)
clean:
	$(RM) *.o $(PROGRAMS)
install:	$(PROGRAMS)
	mv $(PROGRAMS) $(BIN_DIR)
depend:
	makedepend $(DEP_FLAGS) $(SRCDIR)/*.c 
# DO NOT DELETE

