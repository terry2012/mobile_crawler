CXX 		= g++
CXXFLAGS 	= -g -Wall -std=c++14

INCLUDES 	= -I.
LIBS	 	= -lboost_graph -lboost_regex -lcurl -lgumbo

GOAL 		= crawler


SRCS=$(wildcard *.cpp)
OBJS=$(SRCS:.cpp=.o)


all: $(GOAL)

$(GOAL): $(OBJS)
	$(CXX) $(OBJS) -o $(GOAL) $(LIBS)
	@mv -f $(GOAL) ../bin/
	@make clean

-include $(OBJS:.o=.d)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

	@$(CXX) -MM $(CXXFLAGS) $*.cpp > $*.d
	@mv -f $*.d $*.d.tmp
	@sed -e 's|.*:|$*.o:|' < $*.d.tmp > $*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | \
	  sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp


clean:
	@rm -f *.o *.d *.tmp $(GOAL)
