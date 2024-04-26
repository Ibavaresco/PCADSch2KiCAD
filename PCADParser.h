/*============================================================================*/
/*
 Copyright (c) 2024, Isaac Marino Bavaresco
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
	 * Redistributions of source code must retain the above copyright
	   notice, this list of conditions and the following disclaimer.
	 * Neither the name of the author nor the
	   names of its contributors may be used to endorse or promote products
	   derived from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE AUTHOR ''AS IS'' AND ANY
 EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/*============================================================================*/
#if			!defined __PCADPARSER_H__
#define __PCADPARSER_H__
/*===========================================================================*/
#include <stdio.h>
#include <setjmp.h>
#include "PCADStructs.h"
/*===========================================================================*/
#define BUFFER_SIZE					128
/*============================================================================*/
#define PARSE_FLAGS_OMMIT_CLOSE_PAR	  1
#define	PARSE_FLAGS_REQUIRE_EOF		  2
/*===========================================================================*/
typedef enum
	{
	TOKEN_NONE		= EOF - 2,
	TOKEN_INVALID	= EOF - 1,
	TOKEN_EOF		= EOF,
	RESULT_OK,
	TOKEN_CLOSE_PAR,
	TOKEN_OPEN_PAR,
	TOKEN_NAME,
	TOKEN_STRING,
	TOKEN_UNSIGNED,
	TOKEN_INTEGER,
	TOKEN_FLOAT
	} token_t;
/*============================================================================*/
typedef struct
	{
	jmp_buf			JumpBuffer;
	FILE			*File;
	unsigned		LineNumber;
	unsigned		Column;
	unsigned		TabSize;
	size_t			HeapSize;
	size_t			HeapTop;
	uint8_t			*Heap;
	pcad_units_t	FileUnits;
	token_t			UngettedToken;
	char			UngetBuffer[BUFFER_SIZE];
	int				Sort;
	} cookie_t;
/*===========================================================================*/
pcad_schematicfile_t	*ParsePCAD	( cookie_t *Cookie, const char *pNameIn, const char *pNameOut );
/*===========================================================================*/
#endif	/*	!defined __PCADPARSER_H__ */
/*===========================================================================*/

