.DEFAULT_GOAL:=all

CSRC=$(shell find src -name '*.c')
COUT=$(patsubst src/%, dist/%, $(CSRC:.c=.o))

CXXSRC=$(shell find src -name '*.cpp')
CXXOUT=$(patsubst src/%, dist/%, $(CXXSRC:.cpp=.o))

export CFLAGS=--std=c23
export CXXFLAGS=--std=c++2b
FLAGS=-ggdb3 -DBUILDING_CEF_SHARED -DNDEBUG -O0
OUTFILE=dist/libsadan.so
LIBS=
export WINDOWS_SHELL?=pwsh
ifeq ($(OS),Windows_NT)
	FLAGS+=-I windowsHeaders -D_WIN32 -D_WIN64
	FLAGS+= -D_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
	OUTFILE=dist/libsadan.dll
	CC=clang
	export CC
	CXX=clang++
	export CXX
	LIBS+= -L windowsLibs -llibcef_dll_wrapper -llibcef -lShell32 -ldetours -lUser32 -lOle32
	LIBS+= -Xlinker /export:DetourFinishHelperProcess,@1,NONAME
else
	LIBS+=-lcapstone -lcef_dll_wrapper
endif
export FLAGS
clean:
	rm -r dist || :


all: $(outfiles)
	$(MAKE) $(MAKEFLAGS) -f c.mk
	$(MAKE) $(MAKEFLAGS) -f cpp.mk
	$(CXX) $(FLAGS) $(COUT) $(CXXOUT) -shared -o $(OUTFILE) $(LIBS)
ifeq ($(OS),Windows_NT)
	$(WINDOWS_SHELL) ./scripts/generateCompileCommands.ps1
endif