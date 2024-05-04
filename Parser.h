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
#include <stdlib.h>
#include "PCADParser.h"
/*============================================================================*/
#if			!defined __PARSER2_H__
#define __PARSER2_H__
/*============================================================================*/
#define FLAG_NAKED			0
#define FLAG_WRAPPED		1
#define FLAG_CASESENSITIVE	2
#define	FLAG_LIST			4
#define	FLAG_OPTIONAL		8
/*============================================================================*/
typedef struct parsefield_tag		parsefield_t;
typedef struct parsestruct_tag		parsestruct_t;
/*===========================================================================*/
typedef int (*parsefunc_t)( cookie_t *Cookie, const parsefield_t *ParseField, const parsestruct_t *ParseStruct, void *Argument );
/*===========================================================================*/
typedef struct listhead_tag
	{
	size_t							OffsetHead;
	size_t							OffsetLink;
	} listhead_t;
/*===========================================================================*/
typedef struct parsefield_tag
	{
	const int						Flags;
	const char						*TagString;
	const parsefunc_t				ParseFunc;
	const size_t					Length;
	const ssize_t					Offset;
	const struct parsestruct_tag	*ParseStruct;
	} parsefield_t;
/*===========================================================================*/
typedef struct parsestruct_tag
	{
	const int						Flags;
	const size_t					NumFixedFields;
	const parsefield_t				*FixedFields;
	const size_t					NumFields;
	const parsefield_t				*Fields;
	const size_t					NumLists;
	const listhead_t				*Lists;
	const ssize_t					OffsetNext;
	} parsestruct_t;
/*============================================================================*/
void				*Allocate			( cookie_t *Cookie, size_t Size );

pcad_unsigned_t		GetUnsigned			( cookie_t *Cookie );
int					GetName				( cookie_t *Cookie, char *Buffer, size_t BufferLength );
int					GetString			( cookie_t *Cookie, char *Buffer, size_t BufferSize );
char				*GetAndStoreString	( cookie_t *Cookie );

int					ExpectName			( cookie_t *Cookie, const char *Name );
int					ExpectString		( cookie_t *Cookie, const char *String );
token_t				ExpectToken			( cookie_t *Cookie, token_t Token );

int					ParseDimmension		( cookie_t *Cookie, const parsefield_t *ParseField, const parsestruct_t *ParseStruct, void *Argument );
int					ParseReal			( cookie_t *Cookie, const parsefield_t *ParseField, const parsestruct_t *ParseStruct, void *Argument );
int					ParseBoolean		( cookie_t *Cookie, const parsefield_t *ParseField, const parsestruct_t *ParseStruct, void *Argument );
int					ParseUnsigned		( cookie_t *Cookie, const parsefield_t *ParseField, const parsestruct_t *ParseStruct, void *Argument );
int					ParseEnum			( cookie_t *Cookie, const parsefield_t *ParseField, const parsestruct_t *ParseStruct, void *Argument );
int					ParseName			( cookie_t *Cookie, const parsefield_t *ParseField, const parsestruct_t *ParseStruct, void *Argument );
int					ParseString			( cookie_t *Cookie, const parsefield_t *ParseField, const parsestruct_t *ParseStruct, void *Argument );
int					ParseGeneric		( cookie_t *Cookie, const parsefield_t *ParseField, const parsestruct_t *ParseStruct, void *Argument );

pcad_units_t		TranslateUnits		( cookie_t *Cookie, const char *Buffer );
pcad_dimmension_t	ProcessDimmension	( cookie_t *Cookie, const char *Buffer, pcad_units_t Unit );
/*============================================================================*/
int __attribute__((format(printf, 3, 4),noreturn))	Error		( cookie_t *Cookie, int ErrorCode, const char *Message, ... );
int __attribute__((format(printf, 2, 3)))			Warning		( cookie_t *Cookie, const char *Message, ... );
/*============================================================================*/
#endif	/*	!defined __PARSER2_H__ */
/*============================================================================*/
