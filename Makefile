CC = gcc -Wall -O3
CCC = gcc -c -Wall -O3 

#CC = gcc -Wall
#CCC = gcc -c -Wall

all:	laglag

laglag:laglag.h laglag.c 
	$(CC)  laglag.c -lm -o laglag 

debug:
	$(CC) -g laglag.c -lm -o laglag 

profil:
	$gcc -Wall -p laglag.c -lm -o laglag 

compet:
	$(CC) -D NDEBUG laglag.c -lm -o laglag  -static

clean:
	rm -f laglag
	rm -f *.o
	rm -f *~

