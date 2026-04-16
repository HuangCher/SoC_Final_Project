cat > Makefile << 'EOF'
CXX = g++
SC_FLAGS = -std=c++17 -I/usr/local/systemc-2.3.3/include
LDFLAGS = -L/usr/local/systemc-2.3.3/lib-linux64 -lsystemc -Wl,-rpath,/usr/local/systemc-2.3.3/lib-linux64

all: bloom_model

bloom_model: bloom_model.cpp
	$(CXX) $(SC_FLAGS) bloom_model.cpp -o bloom_model $(LDFLAGS)

clean:
	rm -f bloom_model
EOF