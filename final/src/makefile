target: compile

compile: server.c client.c 
	gcc server.c -o PideShop -lm -lpthread
	gcc client.c -o HungryVeryMuch -lm -lpthread
	gcc main.c -o test

run:
	./test

run2:
	valgrind --leak-check=full --show-leak-kinds=all -s ./test

clean:
	rm test


	
	