all: ldt.exe

run: ldt.exe
	./ldt.exe

ldt.exe: ldt.c nt.c seg_gs.c seg_gs.h nt.h
	gcc -Wall -g -O2 -o $@ -mno-cygwin ldt.c nt.c seg_gs.c