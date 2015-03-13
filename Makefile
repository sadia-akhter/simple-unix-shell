# macros
OBJS = main.o shell.o builtins.o extern.o redirect.o parse.o
CC = gcc
DEBUG = -g
LFLAGS = -Wall $(DEBUG)
CFLAGS = -c -Wall -Werror $(DEBUG) 
#LIBS = -lbsd
DIR = sakhter

# executables
all: sish

sish: $(OBJS) 
	$(CC) $(LFLAGS) $(OBJS) -o sish 
#$(LIBS)

# object files
main.o: main.c shell.h extern.h
	$(CC) $(CFLAGS) main.c

shell.o: shell.c shell.h extern.h redirect.h parse.h
	$(CC) $(CFLAGS) shell.c

builtins.o: builtins.c builtins.h shell.h extern.h
	$(CC) $(CFLAGS) builtins.c

extern.o: extern.c extern.h
	$(CC) $(CFLAGS) extern.c

redirect.o: redirect.h redirect.h extern.h
	$(CC) $(CFLAGS) redirect.c

parse.o: parse.h parse.c extern.h
	$(CC) $(CFLAGS) parse.c

# remove files
clean:
	rm *.o sish

# submit package
tar:
	mkdir $(DIR)
	cp main.c $(DIR)
	cp builtins.c $(DIR)
	cp builtins.h $(DIR)
	cp shell.c $(DIR)
	cp shell.h $(DIR)
	cp extern.c $(DIR)
	cp extern.h $(DIR)
	cp parse.c $(DIR)
	cp parse.h $(DIR)
	cp redirect.c $(DIR)
	cp redirect.h $(DIR)
	cp Makefile $(DIR)
	cp README $(DIR)
	git log > $(DIR)/GitLog
	tar cvf $(DIR)-sish.tar $(DIR)/
	rm -r $(DIR)
