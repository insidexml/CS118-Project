NAME = webserver
SRC = $(wildcard *.c)
DEPS = $(wildcard *.h)
OBJ = ${SRC:.c=.o}
CFLAGS = -Wall
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

%.o: %.cpp $(DEPS)
	$(CC) $(INCFLAGS) $(CFLAGS) -c -o $@ $<

$(NAME): $(OBJ)
	$(CC) $(OBJ) -o $(NAME) $(LNKFLAGS) $(LDFLAGS)

clean:
	@ rm $(OBJ) 2>/dev/null || true
	@ rm $(NAME) 2>/dev/null || true
