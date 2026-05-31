section .text
global f
f:
	push rbp
	mov rbp, rsp
	sub rsp, 16
	mov [rbp-8], rdi
	mov [rbp-16], rsi
.Lreturn_f:
	leave
	ret

global main
main:
	push rbp
	mov rbp, rsp
	sub rsp, 32
	mov [rbp-8], rdi
.Lreturn_main:
	leave
	ret

