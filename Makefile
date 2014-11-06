CC = gcc
CFLAGS = -g -c -O -Wall
INCLUDES = -I/usr/X11R6/include -I/usr/X11R6/include/X11
LIBINC = -L/usr/X11R6/lib -L/usr/X11R6/lib/X11
LIBS = -lm -lX11 -lXpm -lXext


TARGET  = wmwl
OBJECTS = wmdl.o


# ------------------- END OF CONFIGURATION SECTION -----------------------


.c.o:
	${CC} ${CFLAGS} ${INCLUDES} $< -o $*.o

${TARGET}: ${OBJECTS}
	${CC} -o ${TARGET} $^ ${LIBINC} ${LIBS} ${OBJECTS}

clean:
	rm -f ${OBJECTS} ${TARGET}
