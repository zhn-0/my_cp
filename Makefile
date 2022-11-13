CC = g++
SRC = Huffman.cpp crypto.cpp mytar.cpp
OBJ = $(patsubst %.cpp, %.o, $(SRC))

test: $(OBJ) test.o
	$(CC) $^ -lcrypto

cp: $(OBJ) main.o
	$(CC) $^ -lcrypto -o $@

%.o: %.cpp
	@$(CC) -c $< -std=c++17 -o $@

.PHONY:clean
clean:
	rm -f *.o *.out