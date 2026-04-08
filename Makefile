APP := progvoraz
SRC := main.c vectordinamico.c monedagestion.c bigint.c

CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic
LDFLAGS ?=

BIN_EXT :=
ifeq ($(OS),Windows_NT)
BIN_EXT := .exe
endif

TARGET := $(APP)$(BIN_EXT)

.PHONY: all debug release run clean help

all: debug

debug: CFLAGS += -O0 -g
debug: $(TARGET)

release: CFLAGS += -O2
release: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f progvoraz progvoraz.exe temporal.exe stock.tmp *.o

help:
	@echo "Targets disponibles:"
	@echo "  make debug    -> compila con simbolos de depuracion"
	@echo "  make release  -> compila optimizado"
	@echo "  make run      -> ejecuta el programa"
	@echo "  make clean    -> elimina artefactos"
