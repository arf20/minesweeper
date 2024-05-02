#ifndef GNU_EFI_ARM_SETJMP_H
#define GNU_EFI_ARM_SETJMP_H

typedef struct {
	UINT32 R3; // A copy of R13
	UINT32 R4;
	UINT32 R5;
	UINT32 R6;
	UINT32 R7;
	UINT32 R8;
	UINT32 R9;
	UINT32 R10;
	UINT32 R11;
	UINT32 R12;
	UINT32 R13;
	UINT32 R14;
} __attribute__((__aligned__(4))) jmp_buf;

#endif /* GNU_EFI_ARM_SETJMP_H */
