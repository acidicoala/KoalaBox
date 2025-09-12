.text
.global push_all
.type push_all, @function
push_all:
    push %ecx
    push %edx
    ret

.global pop_all
.type pop_all, @function
pop_all:
    pop %edx
    pop %ecx
    ret