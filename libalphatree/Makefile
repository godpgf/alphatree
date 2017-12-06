LIB_DIR := ../lib/
STOCK_DIR := ./alpha/db

OBJECTS := ${STOCK_DIR}/stock.pb.o libalphatree_api.o

all: libAlphatree

libAlphatree: $(OBJECTS)
	mkdir -p $(LIB_DIR)
	ar crv $(LIB_DIR)libalphatree_api.a $(OBJECTS)
	g++ -std=c++11 -shared -o $(LIB_DIR)libalphatree_api.so $(OBJECTS) -lprotobuf

%.o: %.cpp
    #g++ -std=c++11 -O3 -Wall -c $< -o $@
	g++ -std=c++11 -O3 -Wall -c -g -rdynamic $< -o $@

${STOCK_DIR}/%.o: ${STOCK_DIR}/%.cc
    g++ -std=c++11 -O3 -Wall -c $< -o $@

clean:	clean_lib
	mkdir -p $(LIB_DIR)
	rm -rf $(LIB_DIR)

clean_lib:
	rm -f $(OBJECTS)


