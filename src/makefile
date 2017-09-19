USE_SDL := 1

# This will likely need to be changed to suit your installation.
ALE := ../../Arcade-Learning-Environment

FLAGS := -std=c++11 -I$(ALE)/src -I$(ALE)/src/controllers -I$(ALE)/src/os_dependent -I$(ALE)/src/environment -I$(ALE)/src/external -L$(ALE)
CXX := clang++
FILE := rom_planner
LDFLAGS := -lale -lz

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    FLAGS += -Wl,-rpath=$(ALE)
endif
ifeq ($(UNAME_S),Darwin)
    FLAGS += -framework Cocoa
endif

ifeq ($(strip $(USE_SDL)), 1)
  DEFINES += -D__USE_SDL -DSOUND_SUPPORT
  FLAGS += $(shell sdl-config --cflags)
  LDFLAGS += $(shell sdl-config --libs)
endif

LDFLAGS += -lboost_filesystem -lboost_system -lboost_program_options

all: $(FILE)

$(FILE):    main.cc node.h planner.h sim_planner.h bfsIW.h rolloutIW.h screen.h utils.h
	$(CXX) $(DEFINES) $(FLAGS) main.cc $(LDFLAGS) -o $(FILE) -Wall -O3

clean:
	rm -rf *.o *~ $(FILE)

