IDIR=include
CC=g++
CFLAGS=-I$(IDIR) -std=c++11

ODIR=obj
LDIR=lib
SRCDIR=src

LIBS=-lm -lnetcdf

_DEPS = NetCDFHelper.h ArakawaCGrid.h 
DEPS = $(patsubst %,$(SRCDIR)/%,$(_DEPS))

_OBJ = main.o NetCDFHelper.o ArakawaCGrid.o 
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: $(SRCDIR)/%.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

netCDFExplorer: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ 
