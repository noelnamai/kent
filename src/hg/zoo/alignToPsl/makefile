include ../../../inc/common.mk

L += -lm $(MYSQLLIBS)
MYLIBDIR = ../zooLib/$(MACHTYPE)
MYLIBS =  $(MYLIBDIR)/zoo.a

A = alignToPsl

O = $A.o

compile: $O 
	${CC} ${COPT} ${CFLAGS} $O $(MYLIBS) $(L) 
	mv ${AOUT} $A${EXE}
	chmod a+rx $A
	mv $A ${BINDIR}/alignToPsl

clean::
	rm -f ${O}
