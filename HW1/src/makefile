target: compile

compile: main.c 
	gcc main.c -o test


run:
	./test

run2:
	valgrind --leak-check=full --show-leak-kinds=all ./test

clean:
	rm test

clean2:
	rm *.txt
	rm test
	rm *.log
	
	