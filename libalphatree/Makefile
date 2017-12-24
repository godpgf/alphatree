LIB_DIR := ../lib/

OBJECTS := libalphatree_api.o

all: libAlphatree

libAlphatree: $(OBJECTS)
	mkdir -p $(LIB_DIR)
	ar crv $(LIB_DIR)libalphatree_api.a $(OBJECTS)
	g++ -fPIC -std=c++11 -shared -o $(LIB_DIR)libalphatree_api.so $(OBJECTS)

%.o: %.cpp
    #g++ -std=c++11 -O3 -Wall -c $< -o $@
	g++ -fPIC -std=c++11 -O3 -Wall -c -g -rdynamic $< -o $@

clean:	clean_lib
	mkdir -p $(LIB_DIR)
	rm -rf $(LIB_DIR)

clean_lib:
	rm -f $(OBJECTS)



