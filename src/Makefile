IDIR = ../include
CC = gcc
CFLAGS = -I$(IDIR) -pthread -std=gnu11

ODIR = obj
LDIR = ../lib

LIBS = -lm -lpigpio -lrt

_DEPS =  one-wire.h # Set of .h files on which the .c files depend
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = temp-control.o one-wire.o # All of the object files should be listed
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

temp-control: $(OBJ) # Name of the executable file
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~
