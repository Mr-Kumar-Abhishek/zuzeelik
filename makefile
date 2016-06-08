all:
	cc -std=c99 -Wall zuzeelik.c mpc/mpc.c -ledit -lm -o zuzeelik

clean:
	rm zuzeelik