PROG:=alert
PLATFORM:=$(shell uname -s)
CPP_FILES:=$(wildcard *.cpp) $(wildcard audio/$(PLATFORM)/*.cpp)
OBJ_FILES:=$(patsubst %.cpp,%.o,$(CPP_FILES))
CPPFLAGS:= -Wno-write-strings -I audio/$(PLATFORM)
LIBS:=-lXm -lpng -lXt -lXpm -lX11 -lXext
FRAMEWORKS:=
ifeq ($(PLATFORM),Darwin)
	FRAMEWORKS+=-framework CoreAudio -framework AudioUnit
endif

$(PROG): $(OBJ_FILES)
	$(CXX) -o $(PROG) $(notdir $(OBJ_FILES)) $(FRAMEWORKS) $(LIBS)

%.o: %.cpp
	$(CXX) -c $< $(CPPFLAGS)

.PHONY: daemon
daemon:
	$(MAKE) -C daemon

clean:
	$(RM) *.o $(PROG)
	$(MAKE) -C daemon clean
