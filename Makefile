SRCFILES := \
	FastLeaderElection.cc \
	Follower.cc \
	Leader.cc \
	Log.cc \
	Message.pb.cc \
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

CXXFLAGS := $(CXXFLAGS) -g -Wall -Werror -I/usr/include
LIBS := -lboost_system -lboost_thread -lpthread -lprotobuf
BGLIBS := -lpthread -lprotobuf
BGLDFLAGS := -L ~/usr/lib

all: $(OBJECTS)

all: test

test: $(HDRFILES) $(OBJFILES) test.o
	$(CXX) $(CXXFLAGS) $(OBJFILES) test.o $(LIBS) -o $@

bgtest: $(HDRFILES) $(OBJFILES) bgtest.o
	$(CXX) $(CXXFLAGS) $(BGLDFLAGS) $(OBJFILES) bgtest.o $(BGLIBS) -o $@

clean:
	-$(RM) $(wildcard $(OBJFILES) $(DEPFILES) test \
	$(filter %.pb.cc, $(SRCFILES)) $(filter %.pb.h, $(HDRFILES)))

-include $(DEPFILES)

%.o: %.cc Makefile
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

%.pb.h %.pb.cc: %.proto
	protoc --cpp_out=. $<
