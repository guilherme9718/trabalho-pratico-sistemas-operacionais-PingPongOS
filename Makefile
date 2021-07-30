preempcao:
	gcc -o preempcao ppos-core-aux.c  pingpong-preempcao.c libppos_static.a
escalonador:
	gcc -o escalonador ppos-core-aux.c  pingpong-scheduler.c libppos_static.a
stress:
	gcc -o escalonador-stress ppos-core-aux.c  pingpong-preempcao-stress.c libppos_static.a
contab:
	gcc -o contab-prio ppos-core-aux.c  pingpong-contab-prio.c libppos_static.a
clean:
	rm preempcao escalonador escalonador-stress contab-prio

.PHONY: all preempcao escalonador stress contab