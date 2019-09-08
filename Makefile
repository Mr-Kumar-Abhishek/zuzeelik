all:
	cc -std=c11 -Wall main.c mpc/mpc.c -ledit -lm -o zuzeelik

clean:
	rm zuzeelik
