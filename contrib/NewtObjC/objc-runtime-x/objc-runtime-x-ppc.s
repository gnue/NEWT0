/*
 * objc-runtime extension based on Apple s objc4-237.
 *
 * Copyright (c) 1999-2003 Apple Computer, Inc.  All Rights Reserved.
 * Copyright (c) 2005 Paul Guyot.  All Rights Reserved.
 * 
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 */
/*
 * 2005-03-19	Creation of the file from code of objc-msg-ppc.s
 */

/********************************************************************
 * id		objc_methodCallv(id	self,
 *			SEL			op,
 *			IMP			method,
 *			unsigned	arg_size,
 *			marg_list	arg_frame);
 *
 * But the marg_list is prepended with the 13 double precision
 * floating point registers that could be used as parameters into
 * the method (fortunately, the same registers are used for either
 * single or double precision floats).  So the "marg_list" is actually:
 *
 * typedef struct objc_sendv_margs {
 *	double		floatingPointArgs[13];
 *	int		linkageArea[6];
 *	int		registerArgs[8];
 *	int		stackArgs[variable];
 * };
 *
 * arg_size is the number of bytes of parameters in registerArgs and
 * stackArgs combined (i.e. it is method_getSizeOfArguments(method)).
 * Specifically, it is NOT the overall arg_frame size, because that
 * would include the floatingPointArgs and linkageArea, which are
 * PowerPC-specific.  This is consistent with the other architectures.
 ********************************************************************/
	.text
	.align    5
	.globl    _objc_methodCallv
_objc_methodCallv:

	mflr    r0
	stw     r0,8(r1)		; save lr

	cmplwi  r6,32			; check parameter size against minimum
	ble+    MinFrame		; is less than minimum, go use minimum
	mr      r12,r1			; remember current stack pointer
	sub     r11,r1,r6		; push parameter area
	rlwinm  r1,r11,0,0,27	; align stack pointer to 16 byte boundary
	stwu    r12,-32(r1)		; push aligned linkage area, set stack link 
	b       HaveFrame

MinFrame:
	stwu    r1,-64(r1)		; push aligned linkage and parameter areas, set stack link

HaveFrame:
	; restore floating point register parameters from marg_list
	lfd     f1,  0(r7)		;
	lfd     f2,  8(r7)		;
	lfd     f3, 16(r7)		;
	lfd     f4, 24(r7)		;
	lfd     f5, 32(r7)		;
	lfd     f6, 40(r7)		;
	lfd     f7, 48(r7)		;
	lfd     f8, 56(r7)		;
	lfd     f9, 64(r7)		;
	lfd     f10,72(r7)		;
	lfd     f11,80(r7)		;
	lfd     f12,88(r7)		;
	lfd     f13,96(r7)		; 

; save the address to jump to to the link register
	mtlr	r5

; load the register based arguments from the marg_list
; the first two parameters are already in r3 and r4, respectively
	subi    r0,r6,(2*4)-3			; make word count from byte count rounded up to multiple of 4...
	srwi.   r0,r0,2					; ... and subtracting for params already in r3 and r4
	beq     CallIt					; branch if there are no parameters to load
	mtctr   r0						; counter = number of remaining words
	lwz     r5,32+(13*8)(r7)		; load 3rd parameter
	bdz     CallIt					; decrement counter, branch if result is zero
	lwz     r6,36+(13*8)(r7)		; load 4th parameter
	bdz     CallIt					; decrement counter, branch if result is zero
	addi    r11,r7,40+(13*8)		; switch to r11, because we are setting r7
	lwz     r7,0(r11)				; load 5th parameter
	bdz     CallIt					; decrement counter, branch if result is zero
	lwz     r8,4(r11)				; load 6th parameter
	bdz     CallIt					; decrement counter, branch if result is zero
	lwz     r9,8(r11)				; load 7th parameter
	bdz     CallIt					; decrement counter, branch if result is zero
	lwzu    r10,12(r11)				; load 8th parameter, and update r11
	bdz     CallIt					; decrement counter, branch if result is zero

; copy the stack based arguments from the marg_list
	addi    r12,r1,24+32-4			; target = address of stack based parameters
StackArgLoop:
	lwzu    r0,4(r11)				; loop to copy remaining marg_list words to stack
	stwu    r0,4(r12)				;
	bdnz    StackArgLoop			; decrement counter, branch if still non-zero

CallIt:
	mflr	r12						; r12 is target address.
	blrl							; (IMP)(self, selector, ...)

	lwz     r1,0(r1)				; restore stack pointer
	lwz     r0,8(r1)				; restore lr
	mtlr    r0						;
	blr     						;

/********************************************************************
 * struct_type	objc_methodCallv_stret(id		self,
 *				SEL		op,
 *				IMP			method,
 *				unsigned	arg_size,
 *				marg_list	arg_frame); 
 *
 * objc_msgSendv_stret is the struct-return form of msgSendv.
 * The ABI calls for r3 to be used as the address of the structure
 * being returned, with the parameters in the succeeding registers.
 * 
 * An equally correct way to prototype this routine is:
 *
 * void objc_methodCallv_stret(void	*structStorage,
 *			id		self,
 *			SEL		op,
 *			IMP			method,
 *			unsigned	arg_size,
 *			marg_list	arg_frame);
 *
 * which is useful in, for example, message forwarding where the
 * structure-return address needs to be passed in.
 *
 * The ABI for the two cases are identical.
 *
 * On entry:	r3 is the address in which the returned struct is put,
 *		r4 is the message receiver,
 *		r5 is the selector,
 *		r6 is the method to call,
 *		r7 is the size of the marg_list, in bytes,
 *		r8 is the address of the marg_list
 ********************************************************************/

	.text
	.align    5
	.globl    _objc_methodCallv_stret
_objc_methodCallv_stret:

	mflr    r0
	stw     r0,8(r1)				; (save return pc)

	cmplwi  r7,32					; check parameter size against minimum
	ble+    StretMinFrame			; is less than minimum, go use minimum
	mr      r12,r1					; remember current stack pointer
	sub     r11,r1,r7				; push parameter area
	rlwinm  r1,r11,0,0,27			; align stack pointer to 16 byte boundary
	stwu    r12,-32(r1)				; push aligned linkage area, set stack link 
	b       StretHaveFrame

StretMinFrame:
	stwu    r1,-64(r1)				; push aligned linkage and parameter areas, set stack link

StretHaveFrame:
	; restore floating point register parameters from marg_list
	lfd     f1,0(r8)		;
	lfd     f2,8(r8)		;
	lfd     f3,16(r8)		;
	lfd     f4,24(r8)		;
	lfd     f5,32(r8)		;
	lfd     f6,40(r8)		;
	lfd     f7,48(r8)		;
	lfd     f8,56(r8)		;
	lfd     f9,64(r8)		;
	lfd     f10,72(r8)		;
	lfd     f11,80(r8)		;
	lfd     f12,88(r8)		;
	lfd     f13,96(r8)		; 

; save the address to jump to to the link register
	mtlr	r6

; load the register based arguments from the marg_list
; the structure return address and the first two parameters
; are already in r3, r4, and r5, respectively.
; NOTE: The callers r3 probably, but not necessarily, matches
; the r3 in the marg_list.  That is, the struct-return
; storage used by the caller could be an intermediate buffer
; that will end up being copied into the original
; struct-return buffer (pointed to by the marg_listed r3).
	subi    r0,r7,(3*4)-3		; make word count from byte count rounded up to multiple of 4...
	srwi.   r0,r0,2				; ... and subtracting for params already in r3 and r4 and r5
	beq     StretCallIt			; branch if there are no parameters to load
	mtctr   r0					; counter = number of remaining words
	lwz     r6,36+(13*8)(r8)	; load 4th parameter
	bdz     StretCallIt			; decrement counter, branch if result is zero
	lwz     r7,40+(13*8)(r8)	; load 5th parameter
	bdz     StretCallIt			; decrement counter, branch if result is zero
	addi    r11,r8,44+(13*8)	; switch to r11, because we are setting r8
	lwz     r8,0(r11)			; load 6th parameter
	bdz     StretCallIt			; decrement counter, branch if result is zero
	lwz     r9,4(r11)			; load 7th parameter
	bdz     StretCallIt			; decrement counter, branch if result is zero
	lwzu    r10,8(r11)			; load 8th parameter, and update r11
	bdz     StretCallIt			; decrement counter, branch if result is zero

; copy the stack based arguments from the marg_list
	addi    r12,r1,24+32-4		; target = address of stack based parameters
StretArgLoop:
	lwzu    r0,4(r11)			; loop to copy remaining marg_list words to stack
	stwu    r0,4(r12)			;
	bdnz    StretArgLoop		; decrement counter, branch if still non-zero

StretCallIt:
	mflr	r12					; r12 is target address.
	blrl						; (IMP)(self, selector, ...)

	lwz     r1,0(r1)			; restore stack pointer
	lwz     r0,8(r1)			; restore return pc
	mtlr    r0
	blr     					; return
