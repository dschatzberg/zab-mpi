SRCFILES := \
	AckNewLeader.cpp \
	Commit.cpp \
	CommitMsg.cpp \
	FastLeaderElection.cpp \
	Follower.cpp \
	Leader.cpp \
	NewLeaderInfo.cpp \
	PeerId.cpp \
	Proposal.cpp \
	ProposalAck.cpp \
	QuorumPeerPosix.cpp \
	StableStore.cpp \
	State.cpp \
	test.cpp \
	Trunc.cpp \
	Vote.cpp \
	Zxid.cpp
HDRFILES := \
	AckNewLeader.hpp \
	Commit.hpp \
	CommitMsg.hpp \
	FastLeaderElection.hpp \
	Follower.hpp \
	Leader.hpp \
	Message.hpp \
	NewLeaderInfo.hpp \
	PeerId.hpp \
	Proposal.hpp \
	ProposalAck.hpp \
	QuorumPeer.hpp \
	QuorumPeerPosix.hpp \
	StableStore.hpp \
	State.hpp \
	Trunc.hpp \
	Vote.hpp \
	Zxid.hpp

OBJFILES :=  $(patsubst %.cpp,%.o,$(SRCFILES))

DEPFILES := $(patsubst %.cpp, %.d, $(SRCFILES))

.PHONY: all clean

CXXFLAGS := -g -std=gnu++0x -Wall -Werror
LIBS := -lboost_system -lboost_thread -lpthread

all: $(OBJECTS)

all: test

test: $(OBJFILES)
	g++ $^ $(LIBS) -o $@

clean:
	-$(RM) $(wildcard $(OBJFILES) $(DEPFILES) test)

-include $(DEPFILES)

%.o: %.cpp Makefile
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@
