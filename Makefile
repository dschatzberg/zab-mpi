SRCFILES := \
	FastLeaderElection.cc \
	Follower.cc \
	Leader.cc \
	Log.cc \
	Message.pb.cc \
	test.cc \
	ZabImpl.cc
HDRFILES := \
	FastLeaderElection.hpp \
	Follower.hpp \
	Leader.hpp \
	Log.hpp \
	Message.pb.h \
	Zab.hpp \
	ZabImpl.hpp

OBJFILES :=  $(patsubst %.cc,%.o,$(SRCFILES))

DEPFILES := $(patsubst %.cc, %.d, $(SRCFILES))

.PHONY: all clean

CXXFLAGS := -g -std=gnu++0x -Wall -Werror -I/usr/include
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

%.pb.h %.pb.cc: %.proto
	protoc -cpp_out=. $<
