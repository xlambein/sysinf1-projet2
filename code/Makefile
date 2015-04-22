CC=gcc
CFLAGS=-W -Wall
LDFLAGS=-lpthread
EXEC=lifeafterdeath
SRC= $(wildcard *.c)
OBJ= $(SRC:.c=.o)

all: $(EXEC)

# Print a warning if there are any to-dos left in the directory
ifneq ($(shell "to""do" -l | wc -l), 0)
	@echo "\033[1;4;7;34mWarning:\033[0;34m some tasks are still left to do\033[0m"
endif

project2: $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS) 

-include $(OBJ:.o=.d)

# Static rule for .o files
%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)
	$(CC) -MM $(CFLAGS) $*.cpp > $*.d
	@mv -f $*.d $*.d.tmp
	@sed -e 's|.*:|$*.o:|' < $*.d.tmp > $*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | \
		sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp

clean:
	rm -f *.o *.d

mrproper: clean
	rm -f $(EXEC)