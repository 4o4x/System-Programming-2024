
ifeq (run, $(firstword $(MAKECMDGOALS)))
  runargs := $(wordlist 2, $(words $(MAKECMDGOALS)), $(MAKECMDGOALS))
  $(eval $(runargs):;@true)
endif


ifeq (run2, $(firstword $(MAKECMDGOALS)))
  runargs := $(wordlist 2, $(words $(MAKECMDGOALS)), $(MAKECMDGOALS))
  $(eval $(runargs):;@true)
endif

target: compile

compile: main.c 
	gcc main.c -o test


run:
	./test $(runargs)

run2:
	valgrind --leak-check=full --show-leak-kinds=all ./test $(runargs)

clean:
	rm test


	
	