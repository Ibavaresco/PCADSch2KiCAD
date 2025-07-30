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
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include "Parser.h"
#include "Lexic.h"
#include "PCADEnums.h"
/*============================================================================*/
pcad_dimmension_t ProcessDimmension( cookie_t *Cookie, const char *Buffer, pcad_enum_units_t Unit )
	{
	pcad_dimmension_t	Value	= 0;
	int					Sign	= 0;
	const char			*p		= Buffer;
	int					Exp, Scale;

	switch( Unit )
		{
		case PCAD_UNITS_NONE:
			Error( Cookie, -1, "Invalid unit \"%s\"", Units.items[Unit] );
		case PCAD_UNITS_MM:
			Exp	=	6;
			Scale	=	1;
			break;
		case PCAD_UNITS_MIL:
			Exp	=	2;
			Scale	= 254;
			break;
		case PCAD_UNITS_IN:
			Exp	=	5;
			Scale	= 254;
			break;
		}

	if( *p == '+' || *p == '-' )
		{
		Sign	= *p == '-';
		p++;
		}

	// Convert the integer part.
	while( isdigit( *p ))
		Value	= Value * 10 + *p++ - '0';

	// Convert the decimal part (if any).
	if( *p == '.' || *p == ',' )
		for( p++; isdigit( *p ) && Exp > 0; Exp-- )
			Value	= Value * 10 + *p++ - '0';

	// Finishing rising to the correct exponent.
	for( ; Exp > 0; Exp-- )
		Value	= Value * 10;

	// Scale the result to mm.
	Value	*= Scale;

	if( Sign != 0 )
		Value	= -Value;

	return Value;
	}
/*============================================================================*/
pcad_enum_units_t TranslateUnits( cookie_t *Cookie, const char *Buffer )
	{
	static const char	*Units[]	= { [PCAD_UNITS_MM]="mm", [PCAD_UNITS_MIL]="Mil", [PCAD_UNITS_IN]="in" };
	int					i;

	if( Buffer == NULL || Buffer[0] == '\0' )
		return Cookie->FileUnits;

	for( i = 0; i < LENGTH( Units ) && stricmp( Buffer, Units[i] ) != 0; i++ )
		{}

	if( i >= LENGTH( Units ))
		Error( Cookie, -1, "Unknown unit" );

	return i;
	}
/*============================================================================*/
int ExpectName( cookie_t *Cookie, const char *Name )
	{
	char	Buffer[BUFFER_SIZE];

	if( GetToken( Cookie, Buffer, sizeof Buffer ) != TOKEN_NAME )
		{
		if( Name != NULL )
			Error( Cookie, -1, "Expecting name %s", Name );
		else
			Error( Cookie, -1, "Expecting name" );
		}

	if( Name != NULL && stricmp( Buffer, Name ) != 0 )
		Error( Cookie, -1, "Expecting name \"%s\"", Name );

	return TOKEN_NAME;
	}
/*============================================================================*/
pcad_real_t GetReal( cookie_t *Cookie )
	{
	char	Buffer[BUFFER_SIZE], *p = Buffer;
	int32_t	Value	= 0;
	int		Sign	= 0;
	int		Exp;
	token_t	Token	= GetToken( Cookie, Buffer, sizeof Buffer );

	if( Token != TOKEN_INTEGER && Token != TOKEN_UNSIGNED && Token != TOKEN_FLOAT )
		Error( Cookie, -1, "Expecting number" );

	if( *p == '+' || *p == '-' )
		Sign	= *p++ == '-';

	// Convert the integer part.
	while( isdigit( *p ))
		Value	= Value * 10 + *p++ - '0';

	// Convert the decimal part (if any).
	if( *p == '.' || *p == ',' )
		for( p++, Exp = 6; isdigit( *p ) && Exp > 0; Exp-- )
			Value	= Value * 10 + *p++ - '0';

	// Finishing rising to the correct exponent.
	for( ; Exp > 0; Exp-- )
		Value	= Value * 10;

	if( Sign != 0 )
		Value	= -Value;

	return Value;
	}
/*============================================================================*/
int ParseReal( cookie_t *Cookie, const parsefield_t *ParseField, const parsestruct_t *ParseStruct, void *Argument )
	{
	pcad_real_t	*Real	= (pcad_real_t*)Argument;

	*Real	= GetReal( Cookie );

	return 0;
	}
/*============================================================================*/
pcad_dimmension_t GetDimmension( cookie_t *Cookie )
	{
	char				Buffer1[BUFFER_SIZE];
	char				Buffer2[BUFFER_SIZE];
	token_t				Token;
	pcad_dimmension_t	Dimmension;

	Token	= GetToken( Cookie, Buffer1, sizeof Buffer1 );
	if( Token != TOKEN_FLOAT && Token != TOKEN_INTEGER && Token != TOKEN_UNSIGNED )
		Error( Cookie, -1, "Expecting a number" );

	Token	= GetToken( Cookie, Buffer2, sizeof Buffer2 );
	if( Token == TOKEN_NAME )
		Dimmension	= ProcessDimmension( Cookie, Buffer1, TranslateUnits( Cookie, Buffer2 ));
	else
		{
		UngetToken( Cookie, Token, Buffer2 );
		Dimmension	= ProcessDimmension( Cookie, Buffer1, Cookie->FileUnits );
		}

	return Dimmension;
	}
/*============================================================================*/
int ParseDimmension( cookie_t *Cookie, const parsefield_t *ParseField, const parsestruct_t *ParseStruct, void *Argument )
	{
	pcad_dimmension_t	*Dimmension = (pcad_dimmension_t*)Argument;

	*Dimmension = GetDimmension( Cookie );

	return 0;
	}
/*============================================================================*/
pcad_enum_boolean_t GetBoolean( cookie_t *Cookie )
	{
	char	Buffer[BUFFER_SIZE];

	if( GetToken( Cookie, Buffer, sizeof Buffer ) != TOKEN_NAME )
		Error( Cookie, -1, "Expecting boolean value" );
	if( stricmp( Buffer, "False" ) == 0 )
		return PCAD_BOOLEAN_FALSE;
	if( stricmp( Buffer, "True" ) == 0 )
		return PCAD_BOOLEAN_TRUE;

	Error( Cookie, -1, "Expecting boolean value" );
	}
/*============================================================================*/
int ParseBoolean( cookie_t *Cookie, const parsefield_t *ParseField, const parsestruct_t *ParseStruct, void *Argument )
	{
	pcad_enum_boolean_t	*Boolean	= (pcad_unsigned_t*)Argument;

	*Boolean	= GetBoolean( Cookie );

	return 0;
	}
/*============================================================================*/
uint32_t GetUnsigned( cookie_t *Cookie )
	{
	char		Buffer[BUFFER_SIZE], *p = Buffer;
	uint32_t	Value	= 0;
	token_t		Token	= GetToken( Cookie, Buffer, sizeof Buffer );

	if( Token != TOKEN_UNSIGNED )
		Error( Cookie, -1, "Expecting unsigned integer number" );

	while( isdigit( *p ))
		Value	= Value * 10 + *p++ - '0';

	return Value;
	}
/*============================================================================*/
int ParseUnsigned( cookie_t *Cookie, const parsefield_t *ParseField, const parsestruct_t *ParseStruct, void *Argument )
	{
	pcad_unsigned_t	*Unsigned	= (pcad_unsigned_t*)Argument;

	*Unsigned	= GetUnsigned( Cookie );

	return 0;
	}
/*============================================================================*/
int32_t GetSigned( cookie_t *Cookie )
	{
	char		Buffer[BUFFER_SIZE], *p = Buffer;
	int32_t	    Value	= 0;
    int32_t     Sign    = 1;
	token_t		Token	= GetToken( Cookie, Buffer, sizeof Buffer );

	if( Token != TOKEN_UNSIGNED && Token != TOKEN_INTEGER )
		Error( Cookie, -1, "Expecting signed integer number" );

    if (*p == '-') {
        Sign = -1;
        p++;
    }

	while( isdigit( *p ))
		Value	= Value * 10 + *p++ - '0';

	return Value * Sign;
	}
/*============================================================================*/
int ParseSigned( cookie_t *Cookie, const parsefield_t *ParseField, const parsestruct_t *ParseStruct, void *Argument )
	{
	pcad_unsigned_t	*Unsigned	= (pcad_unsigned_t*)Argument;

	*Unsigned	= GetSigned( Cookie );

	return 0;
	}
/*============================================================================*/
int GetName( cookie_t *Cookie, char *Buffer, size_t BufferLength )
	{
	if( GetToken( Cookie, Buffer, BufferLength ) != TOKEN_NAME )
		Error( Cookie, -1, "Expecting name" );

	return TOKEN_NAME;
	}
/*============================================================================*/
int ParseEnum( cookie_t *Cookie, const parsefield_t *ParseField, const parsestruct_t *ParseStruct, void *Argument )
	{
	char			Buffer[BUFFER_SIZE];
	pcad_unsigned_t	*Enum		= (pcad_unsigned_t*)Argument;
	parseenum_t		*EnumStruct = (parseenum_t*)ParseStruct;
	int				i;

	*Enum	= 0;
	GetName( Cookie, Buffer, sizeof Buffer );
	for( i = 0; i < EnumStruct->numitems; i++ )
		if( stricmp( Buffer, EnumStruct->items[i] ) == 0 )
			{
			*Enum	= i;
			return 0;
			}

	Warning( Cookie, "Unrecognized enumeration value \"%s\"", Buffer );

	return 0;
	}
/*============================================================================*/
int GetString( cookie_t *Cookie, char *Buffer, size_t BufferLength )
	{
	if( GetToken( Cookie, Buffer, BufferLength ) != TOKEN_STRING )
		Error( Cookie, -1, "Expecting quoted string" );

	return TOKEN_STRING;
	}
/*============================================================================*/
char *StoreString( cookie_t *Cookie, const char *Buffer )
	{
	void	*Address;
	size_t	size	= strlen( Buffer ) + 1;

	if( Cookie->HeapTop >= Cookie->HeapSize || Cookie->HeapTop + size >= Cookie->HeapSize )
		Error( Cookie, -1, "Not enough memory" );

	Address = &Cookie->Heap[Cookie->HeapTop];

	memcpy( Address, Buffer, size );

	Cookie->HeapTop += size;

	return Address;
	}
/*============================================================================*/
char *GetAndStoreString( cookie_t *Cookie )
	{
	char	Buffer[BUFFER_SIZE];

	if( GetToken( Cookie, Buffer, sizeof Buffer ) != TOKEN_STRING )
		Error( Cookie, -1, "Expecting quoted string" );

	return StoreString( Cookie, Buffer );
	}
/*============================================================================*/
int ParseName( cookie_t *Cookie, const parsefield_t *ParseField, const parsestruct_t *ParseStruct, void *Argument )
	{
	char	Buffer[256];
	char	**p	= (char**)Argument;

	GetName( Cookie, Buffer, sizeof Buffer );

	if( p != NULL )
		*p	= StoreString( Cookie, Buffer );

	return 0;
	}
/*============================================================================*/
int ParseString( cookie_t *Cookie, const parsefield_t *ParseField, const parsestruct_t *ParseStruct, void *Argument )
	{
	char	Buffer[256];
	char	**p	= (char**)Argument;

	GetString( Cookie, Buffer, sizeof Buffer );

	if( p != NULL )
		*p	= StoreString( Cookie, Buffer );

	return 0;
	}
/*============================================================================*/
void *Allocate( cookie_t *Cookie, size_t size )
	{
	void	*Address;

	if( Cookie->HeapTop >= Cookie->HeapSize || Cookie->HeapTop + size >= Cookie->HeapSize )
		Error( Cookie, -1, "Not enough memory" );

	Cookie->HeapTop	= ( Cookie->HeapTop + sizeof( void* ) - 1 ) & -sizeof( void* );
	Address			= &Cookie->Heap[ Cookie->HeapTop ];

	memset( Address, 0x00, size );

	Cookie->HeapTop += size;

	return Address;
	}
/*============================================================================*/
token_t ExpectToken( cookie_t *Cookie, token_t tk )
	{
	if( GetToken( Cookie, NULL, 0 ) != tk )
		Error( Cookie, -1, "Expecting \"%s\"", Tokens.items[tk%Tokens.numitems] );

	return tk;
	}
/*============================================================================*/
int SkipAll( cookie_t *Cookie )
	{
	char	Buffer[BUFFER_SIZE];
	token_t Token;

	Token	= GetToken( Cookie, Buffer, sizeof Buffer );
	do
		{
		if( Token == TOKEN_OPEN_PAR && SkipAll( Cookie ) < RESULT_OK )
			Error( Cookie, -1, "Unknown error" );
		Token	= GetToken( Cookie, Buffer, sizeof Buffer );
		}
	while( Token != TOKEN_CLOSE_PAR && Token != TOKEN_INVALID && Token != TOKEN_EOF );

	if( Token != TOKEN_CLOSE_PAR )
		Error( Cookie, -1, "Expecting \")\"" );

	return Token;
	}
/*============================================================================*/
//#define BREAKPOINT

#if			defined BREAKPOINT
static const char Bkpt[]	= "schematicPrintSettings";
#endif	/*	defined BREAKPOINT */
/*============================================================================*/
int ParseGeneric( cookie_t *Cookie, const parsefield_t *ParseField, const parsestruct_t *ParseStruct, void *Argument )
	{
	char	Buffer[256];
	void	*Object;
	int		i;

//	fprintf( stderr, "(%u,%u)", Cookie->LineNumber, Cookie->Column );
	if( ParseStruct == NULL )
		Error( Cookie, -1, "Internal error, \"ParseStruct\" is NULL" );

	/* This field is a pointer inside its data structure, it does not have an area reserved for it... */
	if( ParseField != NULL && ParseField->Length > 0 )
		{
		/* ...so let's allocate memory for it. */
		Object		= Allocate( Cookie, ParseField->Length );

		/* This field is a list, possibly with multiple instances... */
		if( ParseField->Flags & FLAG_LIST )
			{
			/* ...let's link it to its list. */
			void	***Parent;
			Parent		= Argument;
			**Parent	= Object;
			if( ParseStruct != NULL && ParseStruct->OffsetNext >= 0 )
				*Parent = (void*)( (char*)Object + ParseStruct->OffsetNext );
			}
		/* This field is a simple pointer to a single element... */
		else
			{
			/* ...let's make the pointer point to the memory area just allocated. */
			void	**Parent;
			Parent		= Argument;
			*Parent		= Object;
			}
		}
	else
		Object	= Argument;

	/* Initialize all the lists of this object. */
	if( ParseStruct->NumLists > 0 && ParseStruct->Lists != NULL )
		{
		for( i = 0; i < ParseStruct->NumLists; i++ )
			{
			const listhead_t *List	= &ParseStruct->Lists[i];
			*(void**)( (char*)Object + List->OffsetHead )	= NULL;
			*(void**)( (char*)Object + List->OffsetLink )	= (void*)( (char*)Object + List->OffsetHead );
			}
		}

	/* Process all the fixed fields of this object. */
	if( ParseStruct->NumFixedFields > 0 && ParseStruct->FixedFields != NULL )
		{
		for( i = 0; i < ParseStruct->NumFixedFields; i++ )
			{
			const parsefield_t	*Field	= &ParseStruct->FixedFields[i];

			if( Field->Flags & FLAG_WRAPPED )
				{
				ExpectToken( Cookie, TOKEN_OPEN_PAR );
				GetName( Cookie, Buffer, sizeof Buffer );

#if			defined BREAKPOINT
				if( stricmp( Bkpt, Buffer ) == 0 )
					asm volatile( "int3" );
#endif	/*	defined BREAKPOINT */

				if( Field->TagString != NULL && strcmp( Buffer, Field->TagString ) != 0 )
					Error( Cookie, -1, "Expecting name" );
				Field->ParseFunc( Cookie, Field, Field->ParseStruct, Field->Offset < 0 ? NULL : (char*)Object + Field->Offset );
				ExpectToken( Cookie, TOKEN_CLOSE_PAR );
				}
			else
				Field->ParseFunc( Cookie, Field, Field->ParseStruct, Field->Offset < 0 ? NULL : (char*)Object + Field->Offset );
			}
		}

	if( ParseStruct->NumFields > 0 && ParseStruct->Fields != NULL )
		{
		token_t Token;

		while(( Token = GetToken( Cookie, Buffer, sizeof Buffer )) == TOKEN_OPEN_PAR )
			{
			if(( Token = GetToken( Cookie, Buffer, sizeof Buffer )) != TOKEN_NAME )
				Error( Cookie, -1, "Expecting tag at" );

#if			defined BREAKPOINT
			if( stricmp( Bkpt, Buffer ) == 0 )
				asm volatile( "int3" );
#endif	/*	defined BREAKPOINT */

			for( i = 0; i < ParseStruct->NumFields && (( ParseStruct->Fields[i].Flags & FLAG_CASESENSITIVE ) ? ( strcmp( Buffer, ParseStruct->Fields[i].TagString ) != 0 ) : ( stricmp( Buffer, ParseStruct->Fields[i].TagString ) != 0 )); i++ )
				{}

			if( i < ParseStruct->NumFields )
				{
				const parsefield_t	*Field	= &ParseStruct->Fields[i];
				Field->ParseFunc( Cookie, Field, Field->ParseStruct, Field->Offset < 0 ? NULL : (char*)Object + Field->Offset );
				ExpectToken( Cookie, TOKEN_CLOSE_PAR );
				}
			else
				{
				Warning( Cookie, "Skipping \"%s\"", Buffer );
				SkipAll( Cookie );
				}
			}
		UngetToken( Cookie, Token, Buffer );
		}

	return 0;
	}
/*============================================================================*/
int Error( cookie_t *Cookie, int ErrorCode, const char *Message, ... )
	{
	va_list ap;

	va_start( ap, Message );

	fprintf( stderr, "Error in line %u column %u: ", Cookie->LineNumber, Cookie->Column );
	vfprintf( stderr, Message, ap );
	fprintf( stderr, "\n" );

	va_end( ap );

	longjmp( Cookie->JumpBuffer, ErrorCode );
	}
/*============================================================================*/
int Warning( cookie_t *Cookie, const char *Message, ... )
	{
	va_list ap;

	va_start( ap, Message );

	fprintf( stderr, "Warning in line %u column %u: ", Cookie->LineNumber, Cookie->Column );
	vfprintf( stderr, Message, ap );
	fprintf( stderr, "\n" );

	va_end( ap );

	return 0;
	}
/*============================================================================*/
