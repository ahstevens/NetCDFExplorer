OBJS = main.o
CC = g++
DEBUG = -g
CFLAGS = -Wall -c $(DEBUG)
LFLAGS = -Wall $(DEBUG) -lnetcdf

netCDFExplorer : $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) -o netCDFExplorer

main.o : src/main.cpp
	$(CC) $(CFLAGS) src/main.cpp

clean:
	\rm *.o netCDFExplorer
