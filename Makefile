all:
	cd libalphatree; make all
	cd pyalphatree; python setup.py install

clean:
	cd libalphatree; make clean
