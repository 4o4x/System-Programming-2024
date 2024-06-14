target: compile

compile: main.c
	gcc main.c -o MWCp -pthread

run_test1:
	./MWCp 10 10 ../testdir/src/libvterm ../tocopy

run_test1_valgrind:
	valgrind ./MWCp 10 10 ../testdir/src/libvterm ../tocopy

run_test2:
	./MWCp 10 4 ../testdir/src/libvterm/src ../toCopy

run_test2_valgrind:
	valgrind ./MWCp 10 4 ../testdir/src/ libvterm/src ../toCopy

run_test3:
	./MWCp 10 10 ../testdir ../toCopy

run_test3_valgrind:
	valgrind ./MWCp 10 10 ../testdir ../toCopy

run_test4:
	./MWCp 1 1 ../testdir ../toCopy

run_test4_valgrind:
	valgrind ./MWCp 1 1 ../testdir ../toCopy


run_test5:
	./MWCp 1 10 ../testdir ../toCopy

run_test5_valgrind:
	valgrind ./MWCp 1 10 ../testdir ../toCopy


clean:
	rm -f MWCp

delete_dir:
	rm -rf ../toCopy
	mkdir ../toCopy


	
	