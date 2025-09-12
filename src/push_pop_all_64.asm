.text
.global push_all
.type push_all, @function
push_all:
    push %rcx
    push %rdx
    push %rsi
    push %rdi
    push %r8
    push %r9
    push %r10
    push %r11
    push %xmm0
    push %xmm1
    push %xmm2
    push %xmm3
    push %xmm4
    push %xmm5
    push %xmm6
    push %xmm7
    push %xmm8
    push %xmm9
    push %xmm10
    push %xmm11
    push %xmm12
    push %xmm13
    push %xmm14
    push %xmm15
    ret

.global pop_all
.type pop_all, @function
pop_all:
    pop %xmm15
    pop %xmm14
    pop %xmm13
    pop %xmm12
    pop %xmm11
    pop %xmm10
    pop %xmm9
    pop %xmm8
    pop %xmm7
    pop %xmm6
    pop %xmm5
    pop %xmm4
    pop %xmm3
    pop %xmm2
    pop %xmm1
    pop %xmm0
    pop %r11
    pop %r10
    pop %r9
    pop %r8
    pop %rdi
    pop %rsi
    pop %rdx
    pop %rcx
    ret