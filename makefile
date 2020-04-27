all: compile
	./gest_mem

compile:
	gcc -Wall gestion_mem.c -o gest_mem -pthread 

valgrind:
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./gest_mem


clean:
	rm -f gest_mem