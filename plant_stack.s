.text
.globl _planted_trampoline
_planted_trampoline:
	pushfl
	pushal
	#the planted function pointer is now offset at 32 + 4 (32 bytes from pusha, 4 bytes from pushf)
	call *36(%esp)
	popal
	popfl
	#pop the planted function pointer
	leal 4(%esp), %esp
	#return to original place
	ret
	