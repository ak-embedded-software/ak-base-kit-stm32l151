.text
.syntax unified
.thumb
.type asm_test_add, %function
.global asm_test_add

asm_test_add:
	adds r0, r0, r1
	bx lr
	.end
