SRCFILES := \
	FastLeaderElection.cc \
	Log.cc \
	Message.pb.cc \
	test.cc \
	Zab.cc \
	Zxid.cc
HDRFILES := \
	FastLeaderElection.hpp \
	Log.hpp \
	Message.pb.h \
	Zab.hpp \
	Zxid.hpp

OBJFILES :=  $(patsubst %.cc,%.o,$(SRCFILES))

DEPFILES := $(patsubst %.cc, %.d, $(SRCFILES))

.PHONY: all clean

CXXFLAGS := -g -std=gnu++0x -Wall -Werror
LIBS := -lboost_system -lboost_thread -lpthread -lprotobuf

all: $(OBJECTS)

all: test

test: $(OBJFILES)
	g++ $^ $(LIBS) -o $@

clean:
	-$(RM) $(wildcard $(OBJFILES) $(DEPFILES) test)

-include $(DEPFILES)

%.o: %.cc Makefile
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@
