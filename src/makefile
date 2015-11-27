CXX 		= g++
CXXFLAGS 	= -g -Wall -std=c++14
# CXXFLAGS 	= -Wall -std=c++14 -O3

INCLUDES 	= -I.
LIBS	 	= -lboost_graph -lboost_regex -lcurl -lgumbo -lpthread 

GOAL 		= crawler


SRCS=$(wildcard *.cpp)
OBJS=$(SRCS:.cpp=.o)


all: $(GOAL)

$(GOAL): $(OBJS)
	$(CXX) $(OBJS) -o $(GOAL) $(LIBS)
	@cp -f $(GOAL) ../bin/

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
