CC := cc
CFLAGS := -Wall -g -Os

SHDIR := ../common

OBJS = $(SHDIR)/common.o

all: ftserve

ftserve: $(OBJS)
	@$(CC) -D_REENTRANT ftserve.c -o ftserve -lpthread $(CFLAGS) $(OBJS)

$(OBJS) : %.o: %.c 
	@$(CC) -c $(CFLAGS) $< -o $@

.PHONY:
clean:
	@rm -f *.o ftserve
	@rm -f ../common/*.o
	@echo Done cleaning

