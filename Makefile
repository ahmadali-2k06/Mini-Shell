CC       = gcc
CFLAGS   = -Wall -Wextra -std=c99 -g
LDFLAGS  =
TARGET   = minishell

SRCS     = main.c parser.c executor.c builtins.c jobs.c signals.c \
           scheduler.c fcfs.c sjf.c rr.c gantt.c sync_demo.c
OBJS     = $(SRCS:.c=.o)

# Default target
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Pattern rule for .c -> .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Header dependencies (explicit)
main.o:       main.c parser.h executor.h builtins.h jobs.h signals.h
parser.o:     parser.c parser.h
executor.o:   executor.c executor.h parser.h jobs.h
builtins.o:   builtins.c builtins.h parser.h jobs.h scheduler.h sync_demo.h
jobs.o:       jobs.c jobs.h
signals.o:    signals.c signals.h jobs.h
scheduler.o:  scheduler.c scheduler.h jobs.h gantt.h parser.h
fcfs.o:       fcfs.c fcfs.h scheduler.h jobs.h gantt.h
sjf.o:        sjf.c sjf.h scheduler.h jobs.h gantt.h
rr.o:         rr.c rr.h scheduler.h jobs.h gantt.h
gantt.o:      gantt.c gantt.h scheduler.h
sync_demo.o:  sync_demo.c sync_demo.h

clean:
	rm -f $(OBJS) $(TARGET)

# Rebuild everything
rebuild: clean all

.PHONY: all clean rebuild
