LDFLAGS = -lpthread 

BOOST = -I~/Downloads/boost_1_72_0
BOOSTLD = -L~/Downloads/boost_1_72_0/stage/lib

CEP_OBJ = query/PatternMatcher.o query/Query.o  query/Cache_ordered.o query/FetchWorker.o _shared/PredicateMiner.o _shared/MurmurHash3.o _shared/MonitorThread.o utils/freegetopt/getopt.o query/lex/lex.yy.o
# CEP_MATCH_OBJ = $(CEP_OBJ) cep_match/MonitorThread.o cep_match/cep_match.o
CEP_MATCH_OBJ = $(CEP_OBJ) cep/cep_match.o
CPARSE = utils/cparse/builtin-features.o utils/cparse/core-shunting-yard.o

all: bin/cep_match 
default: all

clean:
	rm query/*.o
	rm query/lex/*.o
	rm _shared/*.o
	rm utils/freegetopt/*.o
	rm cep/*.o

bin/cep_match: $(CEP_MATCH_OBJ)
	g++ -std=c++14  -o bin/cep_match $(CEP_MATCH_OBJ) $(LDFLAGS) $(CPARSE) $(BOOSTLD)




%.o: %.c
	gcc -c $< -o $@

%.o: %.cpp
#	g++ -std=c++14 -O3 -mavx2 -c $< -o $@ $(BOOST) $(BOOSTLD) 
	g++ -std=c++14 -O3 -c $< -o $@ $(BOOST) $(BOOSTLD) 
