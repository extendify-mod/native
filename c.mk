CSRC=$(shell find src -name '*.c')

COUT=$(CSRC:.c=.o)

%.o: %.c
	@mkdir -p $(shell dirname $(patsubst src/%, dist/%, $@))
ifeq ($(OS),Windows_NT)
	$(CC) -MJ $(patsubst src/%, dist/%, $(<:.c=.json)) -c $(FLAGS) $(CFLAGS) $< -o $(patsubst src/%, dist/%, $@)
else
	$(CC) -c $(FLAGS) $(CFLAGS) $< -o $(patsubst src/%, dist/%, $@)
endif
all: $(COUT)
