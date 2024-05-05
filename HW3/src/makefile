
ifeq (run_client, $(firstword $(MAKECMDGOALS)))
  runargs := $(wordlist 2, $(words $(MAKECMDGOALS)), $(MAKECMDGOALS))
  $(eval $(runargs):;@true)
endif



target: compile

compile:
	gcc server.c -o server -lrt -lpthread
	gcc client.c -o client -lrt -lpthread



run_client2:
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes  ./client Connect $(runargs)

clean:
	rm -f server
	rm -f client
	rm -f *.o
	rm -f *.out
	rm -f *.txt
	rm -f *.log
	rm -f *.bin
	rm -f *.dat
	rm -f *.pipe
	rm -f *.sock
	rm -f *.fifo
	rm -f *.shm
	

	


	
	


	
	