APP := progvoraz
SRC := main.c vector_dinamico.c moneda_gestion.c bigint.c algoritmo_cambio.c
GUI_APP := progvoraz_gui
GUI_SRC_WIN := gui_window.c moneda_gestion.c bigint.c algoritmo_cambio.c
GUI_SRC_MAC := gui_macos.swift
GUI_SRC_PORTABLE := gui_portable.c moneda_gestion.c bigint.c algoritmo_cambio.c
BUILD_DIR := .dist
TEST_APP := test_progvoraz
TEST_SRC := tests/test_bigint_algoritmo.c bigint.c algoritmo_cambio.c

CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic
LDFLAGS ?=

BIN_EXT :=
ifeq ($(OS),Windows_NT)
BIN_EXT := .exe
endif

UNAME_S :=
ifneq ($(OS),Windows_NT)
UNAME_S := $(shell uname -s 2>/dev/null)
endif

TARGET := $(APP)$(BIN_EXT)
TEST_TARGET := $(BUILD_DIR)/$(TEST_APP)$(BIN_EXT)

ifeq ($(OS),Windows_NT)
RUN_TARGET := $(TARGET)
RUN_TEST_TARGET := $(subst /,\,$(TEST_TARGET))
else
RUN_TARGET := ./$(TARGET)
RUN_TEST_TARGET := ./$(TEST_TARGET)
endif

.PHONY: all debug release run gui run-gui test clean help

all: debug

debug: CFLAGS += -O0 -g
debug: $(TARGET)

release: CFLAGS += -O3 -flto
release: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

run: $(TARGET)
	$(RUN_TARGET)

$(BUILD_DIR):
	mkdir $(BUILD_DIR)

$(TEST_TARGET): $(TEST_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I. $(TEST_SRC) -o $(TEST_TARGET) $(LDFLAGS)

test: CFLAGS += -O0 -g
test: $(TEST_TARGET)
	$(RUN_TEST_TARGET)

ifeq ($(OS),Windows_NT)
GUI_TARGET := $(GUI_APP).exe

gui: CFLAGS += -O3 -flto
gui: $(GUI_TARGET)

$(GUI_TARGET): $(GUI_SRC_WIN)
	$(CC) $(CFLAGS) $(GUI_SRC_WIN) -o $(GUI_TARGET) $(LDFLAGS) -mwindows

run-gui: $(GUI_TARGET)
	$(GUI_TARGET)
else ifeq ($(UNAME_S),Darwin)
GUI_TARGET := $(GUI_APP)

gui: $(GUI_TARGET)

$(GUI_TARGET): $(GUI_SRC_MAC)
	swiftc -parse-as-library $(GUI_SRC_MAC) -o $(GUI_TARGET)

run-gui: $(GUI_TARGET)
	./$(GUI_TARGET)
else
GUI_TARGET := $(GUI_APP)

gui: CFLAGS += -O3 -flto
gui: $(GUI_TARGET)

$(GUI_TARGET): $(GUI_SRC_PORTABLE)
	$(CC) $(CFLAGS) $(GUI_SRC_PORTABLE) -o $(GUI_TARGET) $(LDFLAGS)

run-gui: $(GUI_TARGET)
	./$(GUI_TARGET)
endif

clean:
	rm -f progvoraz progvoraz.exe progvoraz_gui progvoraz_gui.exe temporal.exe stock.tmp *.o $(TEST_TARGET)

help:
	@echo "Targets disponibles:"
	@echo "  make debug    -> compila con simbolos de depuracion"
	@echo "  make release  -> compila optimizado"
	@echo "  make run      -> ejecuta el programa"
	@echo "  make gui      -> compila interfaz GUI (Windows nativa / macOS/Linux portable)"
	@echo "  make run-gui  -> ejecuta interfaz GUI"
	@echo "  make test     -> compila y ejecuta pruebas unitarias"
	@echo "  make clean    -> elimina artefactos"
