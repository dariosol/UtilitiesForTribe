
#files
FILE = analysis_farm.cpp
OFILE = analysis_farm

#compiler
CXX= /usr/local/Cellar/gcc/11.2.0/bin/g++-11
CC= $(CXX)

ROOT_INC= /Applications/root_v6.14.06/include
ROOT_LIB= /Applications/root_v6.14.06/lib

LDFLAG_RPATH = -Wl,-rpath,$(ROOT_LIB)

SYSTEM_FLAGS= -lm -ldl -lMinuit -lpthread

FLAGS = -std=c++17 -g -pthread -m64 
ROOT_FLAGS = -lCore -lRIO -lNet -lHist -lGraf -lGraf3d -lGpad -lTree -lRint -lPostscript -lMatrix -lPhysics -lMathCore -lThread
ROOT_LDFLAGS = -L$(ROOT_LIB)

INCDIR += -I$(ROOT_INC)
CXXFLAGS += $(CFLAGS) $(ROOT_FLAGS)
LDFLAGS  += $(ROOT_LDFLAGS) $(SYSTEM_FLAGS)


analysisfarm: $(FILE)
	$(CXX) $(FLAGS) $(INCDIR) $(FILE) $(LDFLAGS) $(CXXFLAGS) $(LDFLAG_RPATH) -o $(OFILE)
	rm -rf analysis_farm.dSYM/
all: analysis_farm


.PHONY: clean

clean:
	rm -f $(OFILE) *~ core $(INCDIR)/*~ 
