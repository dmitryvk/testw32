.text
.globl _planted_trampoline
_planted_trampoline:
	call *%eax
	movl (%esp), %eax
	addl $4, %esp
	ret
	