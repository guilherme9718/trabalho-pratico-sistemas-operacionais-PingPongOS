all:
	gcc -o escalonador ppos-core-aux.c  pingpong-preempcao.c libppos_static.a
	gcc -o preempcao ppos-core-aux.c  pingpong-scheduler.c libppos_static.a
clean:
	rm ppos-test-1 ppos-test-2