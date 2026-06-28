APP := progvoraz
SRC := main.c app_console.c server_http.c vector_dinamico.c moneda_gestion.c bigint.c algoritmo_cambio.c csv_io.c logger.c json_io.c batch_cli.c exchange_api.c gui_launcher.c progvoraz_locale.c
GUI_APP := progvoraz_gui
GUI_SRC_WIN := gui_window.c vector_dinamico.c moneda_gestion.c bigint.c algoritmo_cambio.c exchange_api.c json_io.c gui_launcher.c progvoraz_locale.c
GUI_SRC_MAC := gui_macos.swift
GUI_SRC_PORTABLE := gui_portable.c vector_dinamico.c moneda_gestion.c bigint.c algoritmo_cambio.c exchange_api.c json_io.c gui_launcher.c progvoraz_locale.c
BUILD_DIR := .dist
TEST_APP := test_progvoraz
TEST_SRC := tests/test_bigint_algoritmo.c vector_dinamico.c bigint.c algoritmo_cambio.c csv_io.c

CC ?= gcc
CFLAGS ?= -std=c2x -Wall -Wextra -Wpedantic
LDFLAGS ?=

ifeq ($(USE_LIBCURL),1)
CFLAGS += -DUSE_LIBCURL
LDFLAGS += -lcurl
endif

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

.PHONY: all debug release debug-sanitize run gui run-gui test test-sanitize valgrind fd-check stress-test memory-safety docker-debug docker-sanitize docker-valgrind docker-stress docker-fd-check clean help

all: debug

debug: CFLAGS += -O0 -g
debug: $(TARGET)

release: CFLAGS += -O3 -flto
release: $(TARGET)

debug-sanitize: CFLAGS += -g -O1 -fsanitize=address,leak,undefined -fno-omit-frame-pointer
debug-sanitize: LDFLAGS += -fsanitize=address,leak,undefined
debug-sanitize: $(TARGET)

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

test-sanitize: CFLAGS += -g -O1 -fsanitize=address,leak,undefined -fno-omit-frame-pointer
test-sanitize: LDFLAGS += -fsanitize=address,leak,undefined
test-sanitize: $(TEST_TARGET) debug-sanitize
	ASAN_OPTIONS=detect_leaks=1:halt_on_error=1:abort_on_error=1 $(RUN_TEST_TARGET)
	ASAN_OPTIONS=detect_leaks=1:halt_on_error=1:abort_on_error=1 ./$(TARGET) --help >/dev/null
	ASAN_OPTIONS=detect_leaks=1:halt_on_error=1:abort_on_error=1 ./$(TARGET) --input tests/sample_batch.csv --output /tmp/progvoraz_asan.csv --log /tmp/progvoraz_asan.log
	printf 'mode,currency,amount,range\na,EUR,127,\nb,EUR,127,\na,NOPE,10,\na,EUR,,\n' | ASAN_OPTIONS=detect_leaks=1:halt_on_error=1:abort_on_error=1 ./$(TARGET) --stream >/tmp/progvoraz_asan_stream.csv

valgrind:
	scripts/valgrind_all_modes.sh

fd-check:
	scripts/fd_check.sh

stress-test:
	scripts/stress_test.sh

memory-safety:
	scripts/memory_safety_suite.sh

docker-debug:
	docker build -f docker/Dockerfile.debug -t progvoraz-debug .

docker-sanitize: docker-debug
	docker run --rm --memory=256m progvoraz-debug make test-sanitize

docker-valgrind: docker-debug
	docker run --rm --memory=256m progvoraz-debug make valgrind

docker-stress: docker-debug
	docker run --rm --memory=256m progvoraz-debug make stress-test

docker-fd-check: docker-debug
	docker run --rm --memory=256m progvoraz-debug make fd-check

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
	@echo "  make debug-sanitize -> compila con ASan/LSan/UBSan"
	@echo "  make test-sanitize  -> ejecuta pruebas y modos CLI con sanitizers"
	@echo "  make valgrind       -> ejecuta Valgrind en modos principales"
	@echo "  make fd-check       -> comprueba crecimiento de file descriptors"
	@echo "  make stress-test    -> ejecuta estrés repetido"
	@echo "  make memory-safety  -> batería completa reproducible"
	@echo "  make clean    -> elimina artefactos"
