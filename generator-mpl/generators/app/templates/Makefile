# <%= projectName %> — MPL project
#
# MPC is the MPL compiler. Override on the command line if it is not on PATH:
#   make MPC=/path/to/comp
MPC ?= <%= compiler %>

SRC = src/main.mpl
EXE = src/main.exe
GEN = src/main.asm src/main.o src/main.exe

.PHONY: all build run clean

all: build

build: $(EXE)

# `-e` makes comp assemble + link, emitting <name>.asm, <name>.o, <name>.exe
# next to the source file.
$(EXE): $(SRC)
	$(MPC) $(SRC) -e

run: build
	./$(EXE)

clean:
	@rm -f $(GEN)
	@echo "clean"
