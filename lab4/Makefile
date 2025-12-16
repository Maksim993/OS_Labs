CC = gcc
CFLAGS = -fPIC -Wall -I./include
LDFLAGS = -shared
PROG1_LDFLAGS = -L./bin -ltranslation_bin -lsort_bubble -Wl,-rpath,./bin
PROG2_LDFLAGS = -ldl

# Директории
SRC_DIR = lib
PROG_DIR = prog
BIN_DIR = bin
INCLUDE_DIR = include

# Все цели
all: directories libraries programs

# Создание директорий
directories:
	mkdir -p $(BIN_DIR)

# Компиляция библиотек
$(BIN_DIR)/libtranslation_bin.so: $(SRC_DIR)/lib1_v1.c $(INCLUDE_DIR)/contract.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

$(BIN_DIR)/libtranslation_ter.so: $(SRC_DIR)/lib1_v2.c $(INCLUDE_DIR)/contract.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

$(BIN_DIR)/libsort_bubble.so: $(SRC_DIR)/lib2_v1.c $(INCLUDE_DIR)/contract.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

$(BIN_DIR)/libsort_quick.so: $(SRC_DIR)/lib2_v2.c $(INCLUDE_DIR)/contract.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

libraries: $(BIN_DIR)/libtranslation_bin.so $(BIN_DIR)/libtranslation_ter.so \
           $(BIN_DIR)/libsort_bubble.so $(BIN_DIR)/libsort_quick.so

# Компиляция программ
programs: $(BIN_DIR)/prog1_static $(BIN_DIR)/prog2_dynamic

$(BIN_DIR)/prog1_static: $(PROG_DIR)/prog1_static.c libraries
	$(CC) $(CFLAGS) -o $@ $(PROG_DIR)/prog1_static.c $(PROG1_LDFLAGS)

$(BIN_DIR)/prog2_dynamic: $(PROG_DIR)/prog2_dynamic.c
	$(CC) $(CFLAGS) -o $@ $(PROG_DIR)/prog2_dynamic.c $(PROG2_LDFLAGS)

# Очистка
clean:
	rm -rf $(BIN_DIR)/*

.PHONY: all clean directories libraries programs