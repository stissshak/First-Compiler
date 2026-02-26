CC=g++
CFLAGS= -Wall -Wextra -pedantic -std=c++23 -Iinc -Wno-unused-result 


LIBS=

SRC_DIR = src
OBJ_DIR = obj
DEP_DIR = tmp
INC_DIR = inc
BIN_DIR = bin


SRC=$(wildcard $(SRC_DIR)/*.cpp)
OBJ=$(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRC))
DEP=$(patsubst $(SRC_DIR)/%.cpp, $(DEP_DIR)/%.d, $(SRC))
EXE=$(BIN_DIR)/comp

all: debug #run

run: $(EXE)
	@./$(EXE)

debug: CFLAGS += -g
debug: $(EXE)

remake: clean debug

release: CFLAGS += -O3 -DNDEBUG
release: clean $(EXE)

$(EXE): $(OBJ)
	@mkdir -p $(BIN_DIR)
	$(CC) -o $@ $^
	@echo "Files sucsessfuly compiled"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR) $(DEP_DIR)
	$(CC) $(CFLAGS) -MMD -MP -MF $(DEP_DIR)/$*.d -c $< -o $@

clean:
	@rm -rf $(OBJ) $(DEP) $(EXE)
	@echo "Directory is clean"

-include $(DEP)
