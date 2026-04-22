all: bloom_model

bloom_model: bloom_model.cpp
	g++ bloom_model.cpp -o bloom_model -std=c++17 \
	-I/usr/local/systemc-2.3.3/include \
	-L/usr/local/systemc-2.3.3/lib-linux64 \
	-lsystemc -lm \
	-Wl,-rpath,/usr/local/systemc-2.3.3/lib-linux64

clean:
	rm -f bloom_model