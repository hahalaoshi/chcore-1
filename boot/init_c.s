	.file	"init_c.c"
	.text
	.globl	boot_cpu_stack
	.bss
	.align 16
	.type	boot_cpu_stack, @object
	.size	boot_cpu_stack, 262144
boot_cpu_stack:
	.zero	262144
	.globl	secondary_boot_flag
	.align 32
	.type	secondary_boot_flag, @object
	.size	secondary_boot_flag, 520
secondary_boot_flag:
	.zero	520
	.section	.rodata
	.align 8
.LC0:
	.string	"[BOOT] Install boot page table\r\n"
.LC1:
	.string	"[BOOT] Enable el1 MMU\r\n"
	.align 8
.LC2:
	.string	"[BOOT] Jump to kernel main at 0x%lx\r\n\n\n\n\n"
.LC3:
	.string	"[BOOT] Should not be here!\r\n"
	.text
	.globl	init_c
	.type	init_c, @function
init_c:
.LFB0:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	call	early_uart_init@PLT
	leaq	.LC0(%rip), %rdi
	movl	$0, %eax
	call	tfp_printf@PLT
	call	init_boot_pt@PLT
	leaq	.LC1(%rip), %rdi
	movl	$0, %eax
	call	tfp_printf@PLT
	call	el1_mmu_activate@PLT
	movq	start_kernel@GOTPCREL(%rip), %rax
	movq	%rax, %rsi
	leaq	.LC2(%rip), %rdi
	movl	$0, %eax
	call	tfp_printf@PLT
	leaq	secondary_boot_flag(%rip), %rdi
	call	start_kernel@PLT
	leaq	.LC3(%rip), %rdi
	movl	$0, %eax
	call	tfp_printf@PLT
	nop
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE0:
	.size	init_c, .-init_c
	.ident	"GCC: (Ubuntu 7.4.0-1ubuntu1~18.04.1) 7.4.0"
	.section	.note.GNU-stack,"",@progbits
