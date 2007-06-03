/*
 * objc-runtime extension based on Apple s objc4-274.
 *
 * Copyright (c) 1999-2003 Apple Computer, Inc.  All Rights Reserved.
 * Copyright (c) 2006 Paul Guyot.  All Rights Reserved.
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
 * 2006-06-16	Creation of the file from code of objc-msg-i386.s
 * 2007-05-30	Replace _objc_methodCallv_stret by _objc_msgSendv_stret based
 */
//////////////////////////////////////////////////////////////////////
//
// ENTRY		functionName
//
// Assembly directives to begin an exported function.
//
// Takes: functionName - name of the exported function
//////////////////////////////////////////////////////////////////////

.macro ENTRY
	.text
	.globl	$0
	.align	4, 0x90
$0:
.endmacro

//////////////////////////////////////////////////////////////////////
//
// END_ENTRY	functionName
//
// Assembly directives to end an exported function.  Just a placeholder,
// a close-parenthesis for ENTRY, until it is needed for something.
//
// Takes: functionName - name of the exported function
//////////////////////////////////////////////////////////////////////

.macro END_ENTRY
.endmacro

/********************************************************************
 *
 * Offsets.
 *
 ********************************************************************/

	self            = 4
	selector        = 8
	method          = 12
	marg_size       = 16
	marg_list       = 20

	struct_addr     = 4

	self_stret      = 8
	selector_stret  = 12
	method_stret    = 16
	marg_size_stret = 20
	marg_list_stret = 24

/********************************************************************
 * id		objc_methodCallv(id	self,
 *			SEL			op,
 *			IMP			method,
 *			unsigned	arg_size,
 *			marg_list	arg_frame);
 *
 * On entry:
 *		(sp+4)  is the message receiver,
 *		(sp+8)	is the selector,
 *		(sp+12)	is the address of the method,
 *		(sp+16) is the size of the marg_list, in bytes,
 *		(sp+20) is the address of the marg_list
 *
 ********************************************************************/

	ENTRY	_objc_methodCallv

	pushl	%ebp
	movl	%esp, %ebp
	// stack is currently aligned assuming no extra arguments
	movl	(marg_list+4)(%ebp), %edx	// +4 because of saved %ebp.
	addl	$8, %edx					// skip self & selector
	movl	(marg_size+4)(%ebp), %ecx	// %ecx is the size.
	subl    $8, %ecx					// %ecx is the size - 8 (skip self & selector)
	shrl	$2, %ecx					// (%ecx is the size - 8) >> 2, so basically
										// numVariableArguments
	je      ArgsOK

	// %esp = %esp - (16 - ((numVariableArguments & 3) << 2))
	movl    %ecx, %eax			// 16-byte align stack
	andl    $3, %eax
	shll    $2, %eax
	subl    $16, %esp
	addl    %eax, %esp

ArgLoop:
	decl	%ecx
	movl	0(%edx, %ecx, 4), %eax
	pushl	%eax
	jg	ArgLoop

ArgsOK:
	movl	(selector+4)(%ebp), %ecx
	pushl	%ecx
	movl	(self+4)(%ebp),%ecx
	pushl	%ecx
	movl	(method+4)(%ebp),%ecx
	call	%ecx
	movl	%ebp,%esp
	popl	%ebp

	ret

	END_ENTRY	_objc_methodCallv

/********************************************************************
 * void objc_methodCallv_stret(
				void* stretAddr,
				id self,
				SEL _cmd,
				IMP method,
				unsigned arg_size,
				marg_list arg_frame);
 *
 * On entry:
 *		(sp+4)  is the address in which the returned struct is put,
 *		(sp+8)  is the message receiver,
 *		(sp+12) is the selector,
 *		(sp+26) is the method to call,
 *		(sp+20) is the size of the marg_list, in bytes,
 *		(sp+24) is the address of the marg_list
 *
 ********************************************************************/

	ENTRY	_objc_methodCallv_stret

	pushl	%ebp
	movl	%esp, %ebp
	subl    $12, %esp	// align stack assuming no extra arguments
	movl	(marg_list_stret+4)(%ebp), %edx
	addl	$8, %edx			// skip self & selector
	movl	(marg_size_stret+4)(%ebp), %ecx
	subl	$5, %ecx			// skip self & selector
	shrl	$2, %ecx
	je      StretArgsOK

	// %esp = %esp - (16 - ((numVariableArguments & 3) << 2))
	movl    %ecx, %eax			// 16-byte align stack
	andl    $3, %eax
	shll    $2, %eax
	subl    $16, %esp
	addl    %eax, %esp

StretArgLoop:
	decl	%ecx
	movl	0(%edx, %ecx, 4), %eax
	pushl	%eax
	jg	StretArgLoop

StretArgsOK:
	movl	(selector_stret+4)(%ebp), %ecx
	pushl	%ecx
	movl	(self_stret+4)(%ebp),%ecx
	pushl	%ecx
	movl	(struct_addr+4)(%ebp),%ecx
	pushl	%ecx
	movl	(method_stret+4)(%ebp),%ecx
	call	%ecx
	movl	%ebp,%esp
	popl	%ebp

	ret

	END_ENTRY	_objc_methodCallv_stret
