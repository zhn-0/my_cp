CC = g++
SRC = Huffman.cpp crypto.cpp mytar.cpp
OBJ = $(patsubst %.cpp, %.o, $(SRC))

cp: $(OBJ) main.o
	$(CC) $^ -lcrypto -o $@

test: $(OBJ) test.o
	$(CC) $^ -lcrypto

%.o: %.cpp
	@$(CC) -c $< -std=c++17 -o $@

.PHONY:clean
clean:
	rm -f *.o *.out tmp* cp