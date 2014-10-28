SRC_DIR := ./src
INCLUDE_DIR := ./include
BUILD_DIR := ./build

JSONRPC_SRC_DIR := $(SRC_DIR)/json-rpc
STUBGEN_SRC_DIR := $(SRC_DIR)/stubgen

LIB_DIR := ./lib 
LIB := -ljconer -lpthread

THIRD_INC_DIR :=
THIRD_LIB_DIR := 

CPP := g++
CC := gcc
AR := ar

CFLAG := -O2 -Wall -std=c++11 -DJSONRPC_DEBUG
LFLAG := -O2 -flto -lpthread -L$(LIB_DIR) $(LIB)
ARFLAG := -rcs

SRC := $(wildcard $(JSONRPC_SRC_DIR)/*.cpp)
OBJ := $(patsubst %.cpp,%.o, $(subst $(SRC_DIR),$(BUILD_DIR), $(SRC)))

ARCHIVE := libjson-rpc.a
BIN := bin/stubgen

.PHONY:all target  $(BUILD_DIR)
all: target $(BIN)
target: $(BUILD_DIR) $(OBJ) $(ARCHIVE)

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/%.o:$(SRC_DIR)/%.cpp
	mkdir -p $(shell dirname $@)
	$(CPP) -c $< $(CFLAG) -o $@ -I$(INCLUDE_DIR) $(THIRD_INC_DIR)

$(ARCHIVE):$(OBJ)
	$(AR) $(ARFLAG) $@ $(OBJ)

$(BIN):$(STUBGEN_SRC_DIR)/*.cpp
	mkdir -p bin
	$(CPP) -o $@ $< $(CFLAG) -I$(INCLUDE_DIR) $(THIRD_INC_DIR) $(LFLAG)

clean:
	rm -rf $(BUILD_DIR) $(TESTBIN_DIR) $(ARCHIVE) $(BIN)
