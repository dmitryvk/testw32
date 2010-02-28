.text
.globl _planted_trampoline
_planted_trampoline:
	pushfl
	pushal
	#the planted function argument is at %esp + 32 + 4 + 4
	pushl 40(%esp)
	#the planted function pointer is now offset at 32 + 4 + 4 (32 bytes from pusha, 4 bytes from pushf, 4 bytes for arg)
	call *40(%esp)
	leal 4(%esp), %esp
	popal
	popfl
	#pop the planted function pointer and argument
	leal 8(%esp), %esp
	#return to original place
	ret
	