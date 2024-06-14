target: compile

compile: main.c 
	gcc main.c -o test -pthread


run:
	./test

run2:
	valgrind --leak-check=full --show-leak-kinds=all -s --track-origins=yes ./test

clean:
	rm test


	
	