
../build-parity/auto-verify0/netbench:     file format elf64-x86-64


Disassembly of section .init:

Disassembly of section .plt:

Disassembly of section .plt.got:

Disassembly of section .text:

0000000000026020 <netbench_memory_work.dc_guard>:
   26020:	41 57                	push   %r15
   26022:	41 56                	push   %r14
   26024:	53                   	push   %rbx
   26025:	4c 8d 3d 84 ea 1b 00 	lea    0x1bea84(%rip),%r15        # 1e4ab0 <__dc_functions>
   2602c:	49 89 fe             	mov    %rdi,%r14
   2602f:	48 89 f3             	mov    %rsi,%rbx
   26032:	4c 89 ff             	mov    %r15,%rdi
   26035:	e8 f6 fe ff ff       	call   25f30 <dc_acquire>
   2603a:	4c 89 f7             	mov    %r14,%rdi
   2603d:	48 89 de             	mov    %rbx,%rsi
   26040:	e8 ab ff ff ff       	call   25ff0 <netbench_memory_work.dc_body>
   26045:	4c 89 ff             	mov    %r15,%rdi
   26048:	e8 03 ff ff ff       	call   25f50 <dc_release>
   2604d:	5b                   	pop    %rbx
   2604e:	41 5e                	pop    %r14
   26050:	41 5f                	pop    %r15
   26052:	c3                   	ret

Disassembly of section .fini:
