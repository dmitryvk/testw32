all: ldt.exe plant_stack.exe cv_w32.exe

run: ldt.exe
	./ldt.exe

ldt.exe: ldt.c nt.c seg_gs.c seg_gs.h nt.h
	gcc -Wall -g -O2 -o $@ -mno-cygwin $+
	
plant_stack.exe: plant_stack.c nt.c seg_gs.c seg_gs.h nt.h plant_stack.s
	gcc -Wall -g -O2 -o $@ -mno-cygwin $+
	
cv_w32.exe: cv_w32.c
	gcc -Wall -g -O2 -o $@ -mno-cygwin $+