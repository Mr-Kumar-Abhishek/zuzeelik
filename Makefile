all:
	cc -std=c99 -Wall main.c mpc/mpc.c -ledit -lm -o zuzeelik

clean:
	rm zuzeelik
