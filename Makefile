SRCFILES := \
	AckNewLeader.pb.cc \
	BGCommunicator.cc \
	Commit.pb.cc \
	Entry.pb.cc \
	FastLeaderElection.cc \
	Follower.cc \
	FollowerInfo.pb.cc \
	Leader.cc \
	Log.cc \
	NewLeaderInfo.pb.cc \
	ProposalAck.pb.cc \
	Proposal.pb.cc \
	Vote.pb.cc \
	Zab.cc
HDRFILES := \
	AckNewLeader.pb.h \
	BGCommunicator.hpp \
	Commit.pb.h \
	Entry.pb.h \
	FastLeaderElection.hpp \
	Follower.hpp \
	FollowerInfo.pb.h \
	Leader.hpp \
	Log.hpp \
	NewLeaderInfo.pb.h \
	ProposalAck.pb.h \
	Proposal.pb.h \
	Vote.pb.h \
	Zab.hpp

OBJFILES :=  $(patsubst %.cc,%.o,$(SRCFILES))

DEPFILES := $(patsubst %.cc, %.d, $(SRCFILES)) bgtest.d test.d

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
	-$(RM) $(wildcard $(OBJFILES) $(DEPFILES) test test.o bgtest bgtest.o \
	$(filter %.pb.cc, $(SRCFILES)) $(filter %.pb.h, $(HDRFILES)))

-include $(DEPFILES)

%.o: %.cc Makefile
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

%.pb.h %.pb.cc: %.proto
	protoc --cpp_out=. $<
