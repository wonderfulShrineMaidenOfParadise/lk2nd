/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <debug.h>
#include <bits.h>
#include <arch/arm.h>
#include <kernel/thread.h>

void dump_fault_frame(struct arm_fault_frame *frame)
{
	dprintf(CRITICAL, "r0  0x%08x r1  0x%08x r2  0x%08x r3  0x%08x\n", frame->r[0], frame->r[1], frame->r[2], frame->r[3]);
	dprintf(CRITICAL, "r4  0x%08x r5  0x%08x r6  0x%08x r7  0x%08x\n", frame->r[4], frame->r[5], frame->r[6], frame->r[7]);
	dprintf(CRITICAL, "r8  0x%08x r9  0x%08x r10 0x%08x r11 0x%08x\n", frame->r[8], frame->r[9], frame->r[10], frame->r[11]);
	dprintf(CRITICAL, "r12 0x%08x usp 0x%08x ulr 0x%08x pc  0x%08x\n", frame->r[12], frame->usp, frame->ulr, frame->pc);
	dprintf(CRITICAL, "spsr 0x%08x\n", frame->spsr);

	struct arm_mode_regs regs;
	arm_save_mode_regs(&regs);

	dprintf(CRITICAL, "%c%s r13 0x%08x r14 0x%08x\n", ((frame->spsr & MODE_MASK) == MODE_FIQ) ? '*' : ' ', "fiq", regs.fiq_r13, regs.fiq_r14);
	dprintf(CRITICAL, "%c%s r13 0x%08x r14 0x%08x\n", ((frame->spsr & MODE_MASK) == MODE_IRQ) ? '*' : ' ', "irq", regs.irq_r13, regs.irq_r14);
	dprintf(CRITICAL, "%c%s r13 0x%08x r14 0x%08x\n", ((frame->spsr & MODE_MASK) == MODE_SVC) ? '*' : ' ', "svc", regs.svc_r13, regs.svc_r14);
	dprintf(CRITICAL, "%c%s r13 0x%08x r14 0x%08x\n", ((frame->spsr & MODE_MASK) == MODE_UND) ? '*' : ' ', "und", regs.und_r13, regs.und_r14);
	dprintf(CRITICAL, "%c%s r13 0x%08x r14 0x%08x\n", ((frame->spsr & MODE_MASK) == MODE_SYS) ? '*' : ' ', "sys", regs.sys_r13, regs.sys_r14);

	// dump the bottom of the current stack
	addr_t stack;
	switch (frame->spsr & MODE_MASK) {
		case MODE_FIQ: stack = regs.fiq_r13; break;
		case MODE_IRQ: stack = regs.irq_r13; break;
		case MODE_SVC: stack = regs.svc_r13; break;
		case MODE_UND: stack = regs.und_r13; break;
		case MODE_SYS: stack = regs.sys_r13; break;
		default:
			stack = 0;
	}

	if (stack != 0) {
		dprintf(CRITICAL, "bottom of stack at 0x%08x:\n", (unsigned int)stack);
		hexdump((void *)stack, 128);
	}
}

static void exception_die(struct arm_fault_frame *frame, int pc_off, const char *msg)
{
	inc_critical_section();
	frame->pc += pc_off;
	dprintf(CRITICAL, msg);
	dump_fault_frame(frame);
	
	halt();
	for(;;);
}

void arm_syscall_handler(struct arm_fault_frame *frame)
{
	exception_die(frame, -4, "unhandled syscall, halting\n");
}

void arm_undefined_handler(struct arm_fault_frame *frame)
{
	exception_die(frame, -4, "undefined abort, halting\n");
}

void arm_data_abort_handler(struct arm_fault_frame *frame)
{
	uint32_t fsr = arm_read_dfsr();
	uint32_t far = arm_read_dfar();

	uint32_t fault_status = (BIT_GET(fsr, 10) ? (1<<4) : 0) |  BITS(fsr, 3, 0);

	dprintf(CRITICAL, "data abort\n");
	bool write = !!BIT_GET(fsr, 11);

	/* decode the fault status (from table B3-23) */
	switch (fault_status) {
		case 0b00001: // alignment fault
			dprintf(CRITICAL, "alignment fault on %s\n", write ? "write" : "read");
			break;
		case 0b00101:
		case 0b00111: // translation fault
			dprintf(CRITICAL, "translation fault on %s\n", write ? "write" : "read");
			break;
		case 0b00011:
		case 0b00110: // access flag fault
			dprintf(CRITICAL, "access flag fault on %s\n", write ? "write" : "read");
			break;
		case 0b01001:
		case 0b01011: // domain fault
			dprintf(CRITICAL, "domain fault, domain %d\n", BITS_SHIFT(fsr, 7, 4));
			break;
		case 0b01101:
		case 0b01111: // permission fault
			dprintf(CRITICAL, "permission fault on %s\n", write ? "write" : "read");
			break;
		case 0b00010: // debug event
			dprintf(CRITICAL, "debug event\n");
			break;
		case 0b01000: // synchronous external abort
			dprintf(CRITICAL, "synchronous external abort on %s\n", write ? "write" : "read");
			break;
			break;
		case 0b10110: // asynchronous external abort
			dprintf(CRITICAL, "asynchronous external abort on %s\n", write ? "write" : "read");
			break;
		case 0b10000: // TLB conflict event
		case 0b11001: // synchronous parity error on memory access
		case 0b00100: // fault on instruction cache maintenance
		case 0b01100: // synchronous external abort on translation table walk
		case 0b01110: //    "
		case 0b11100: // synchronous parity error on translation table walk
		case 0b11110: //    "
		case 0b11000: // asynchronous parity error on memory access
		default:
			dprintf(CRITICAL, "unhandled fault\n");
			;
	}

	dprintf(CRITICAL, "DFAR 0x%x (fault address)\n", far);
	dprintf(CRITICAL, "DFSR 0x%x (fault status register)\n", fsr);

	exception_die(frame, -8, "data abort, halting\n");
}

void arm_prefetch_abort_handler(struct arm_fault_frame *frame)
{
	uint32_t fsr = arm_read_ifsr();
	uint32_t far = arm_read_ifar();

	uint32_t fault_status = (BIT_GET(fsr, 10) ? (1<<4) : 0) |  BITS(fsr, 3, 0);

	dprintf(CRITICAL, "prefetch abort\n");

	/* decode the fault status (from table B3-23) */
	switch (fault_status) {
		case 0b00001: // alignment fault
			dprintf(CRITICAL, "alignment fault\n");
			break;
		case 0b00101:
		case 0b00111: // translation fault
			dprintf(CRITICAL, "translation fault\n");
			break;
		case 0b00011:
		case 0b00110: // access flag fault
			dprintf(CRITICAL, "access flag fault\n");
			break;
		case 0b01001:
		case 0b01011: // domain fault
			dprintf(CRITICAL, "domain fault, domain %d\n", BITS_SHIFT(fsr, 7, 4));
			break;
		case 0b01101:
		case 0b01111: // permission fault
			dprintf(CRITICAL, "permission fault\n");
			break;
		case 0b00010: // debug event
			dprintf(CRITICAL, "debug event\n");
			break;
		case 0b01000: // synchronous external abort
			dprintf(CRITICAL, "synchronous external abort\n");
			break;
			break;
		case 0b10110: // asynchronous external abort
			dprintf(CRITICAL, "asynchronous external abort\n");
			break;
		case 0b10000: // TLB conflict event
		case 0b11001: // synchronous parity error on memory access
		case 0b00100: // fault on instruction cache maintenance
		case 0b01100: // synchronous external abort on translation table walk
		case 0b01110: //    "
		case 0b11100: // synchronous parity error on translation table walk
		case 0b11110: //    "
		case 0b11000: // asynchronous parity error on memory access
		default:
			dprintf(CRITICAL, "unhandled fault\n");
			;
	}

	dprintf(CRITICAL, "IFAR 0x%x (fault address)\n", far);
	dprintf(CRITICAL, "IFSR 0x%x (fault status register)\n", fsr);

	exception_die(frame, -4, "prefetch abort, halting\n");
}
