NAME = server
SRC := $(wildcard *.c) $(wildcard ../*.c)
DEPS := $(wildcard *.h) $(wildcard ../*.h)
OBJ := ${SRC:.c=.o}
CFLAGS = -Wall -D PC=$(PC) -D PI=$(PI)
LDFLAGS =
INCFLAGS =
LNKFLAGS =

.PHONY: all clean debug release verbose

all: debug

verbose: CFLAGS += -D VERBOSE
verbose: debug

debug: CFLAGS += -g
debug: $(NAME)

release: CFLAGS += -O2
release: $(NAME)

%.o: %.c $(DEPS)
	$(CC) $(INCFLAGS) $(CFLAGS) -c -o $@ $<

$(NAME): $(OBJ)
	$(CC) $(OBJ) -o $(NAME) $(LNKFLAGS) $(LDFLAGS)
	@ cp $(NAME) $(ROOTDIR)

clean:
	@ rm $(OBJ) $(NAME) $(ROOTDIR)/$(NAME) 2>/dev/null || true
