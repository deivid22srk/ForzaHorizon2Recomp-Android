# FH2 host boot harness (x86_64 Linux, LOCAL ONLY).
# Links the XenonRecomp-generated TUs against a logging kernel-shim to prove
# the recompiled guest actually executes. Generated TUs are game-derived and
# are never committed; point PPC_DIR at your own codegen output.
CXX      ?= clang++
STD      ?= -std=c++17
OPT      ?= -O0 -g0
PPC_DIR  ?= ../recomp/generated
SIMDE    ?= ../thirdparty/XenonRecomp/thirdparty/simde
INC      := -I$(PPC_DIR) -I$(SIMDE)
OBJ      := obj
TUS      := $(shell ls $(PPC_DIR)/ppc_recomp.*.cpp 2>/dev/null | sed 's#.*/##; s#\.cpp##')
OBJS     := $(addprefix $(OBJ)/,$(addsuffix .o,$(TUS)))

all: fh2boot

$(OBJ)/%.o: $(PPC_DIR)/%.cpp
	@mkdir -p $(OBJ)
	$(CXX) $(STD) $(OPT) $(INC) -c $< -o $@

$(OBJ)/mapping.o: $(PPC_DIR)/ppc_func_mapping.cpp
	@mkdir -p $(OBJ)
	$(CXX) $(STD) $(OPT) $(INC) -c $< -o $@

$(OBJ)/kernel_stubs.o: kernel_stubs.cpp
	$(CXX) $(STD) $(OPT) $(INC) -c $< -o $@

$(OBJ)/stubs.o: stubs.cpp
	$(CXX) $(STD) $(OPT) $(INC) -c $< -o $@

$(OBJ)/host_main.o: host_main.cpp
	$(CXX) $(STD) $(OPT) $(INC) -c $< -o $@

fh2boot: $(OBJS) $(OBJ)/mapping.o $(OBJ)/kernel_stubs.o $(OBJ)/stubs.o $(OBJ)/host_main.o
	$(CXX) $(OPT) $^ -o $@

clean:
	rm -rf $(OBJ) fh2boot
