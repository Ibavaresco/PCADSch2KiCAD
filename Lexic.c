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
#include "Lexic.h"
/*============================================================================*/
static int	GetChar( cookie_t *Cookie )
	{
	return fgetc( Cookie->File );
	}
/*============================================================================*/
static int	UngetChar( cookie_t *Cookie, int c )
	{
	return ungetc( c, Cookie->File );
	}
/*============================================================================*/
token_t GetToken( cookie_t *Cookie, char *Buffer, size_t BufferSize )
	{
	int	c;

	if( Cookie->UngettedToken != TOKEN_NONE )
        {
        token_t Temp    = Cookie->UngettedToken;
        if( Buffer != NULL )
            strncpy( Buffer, Cookie->UngetBuffer, BufferSize );
        Cookie->UngettedToken   = TOKEN_NONE;
        return Temp;
        }

	do
		{
		c	= GetChar( Cookie );
		switch( c )
			{
			case ' ':
				Cookie->Column++;
				break;
			case '\r':
				c	= GetChar( Cookie );
				if( c != '\n' )
					{
					UngetChar(  Cookie, c );
					c	= '\r';
					}
				Cookie->LineNumber++;
				Cookie->Column	= 1;
				break;
			case '\n':
				c	= GetChar( Cookie );
				if( c != '\r' )
					{
					UngetChar( Cookie, c );
					c	= '\n';
					}
				Cookie->LineNumber++;
				Cookie->Column	= 1;
				break;
			case '\t':
				Cookie->Column	= ((( Cookie->Column - 1 ) / Cookie->TabSize ) + 1 ) * Cookie->TabSize + 1;
				break;
			case '\b':
				//if( Cookie->Column > 1 )
				//	Cookie->Column--;
				break;
			default:
				//Cookie->Column++;
				break;
			}
		}
	while( isspace( c ));

	switch( c )
		{
		case '(':
			Cookie->Column++;
			return TOKEN_OPEN_PAR;
		case ')':
			Cookie->Column++;
			return TOKEN_CLOSE_PAR;
        case EOF:
            return TOKEN_EOF;
		}

	if( c == '"' )
		{
		int Length;
        Cookie->Column++;
		for( Length = 0, c = GetChar( Cookie ); c != '\r' && c != '\n' && c != EOF && c != '"'; c = GetChar( Cookie ) )
			{
			Cookie->Column++;
#if 0
			if( c == '\\' )
                {
                c = GetChar( Cookie );
                if( Buffer != NULL && c != '\"' && c != '\\' && Length < BufferSize - 1 )
                    {
                    *Buffer++	= '\\';
                    Length++;
                    }
                Cookie->Column++;
                }
#endif
			if( Buffer != NULL && Length < BufferSize - 1 )
				{
				*Buffer++	= c;
				Length++;
				}
			}
        Cookie->Column++;
		Buffer != NULL && ( *Buffer	= '\0' );
		if( c != '"' )
			return TOKEN_INVALID;
		return TOKEN_STRING;
		}

	if( isalpha( c ))
		{
		int	Length = 0;

		do
			{
			if( Buffer != NULL && Length < BufferSize - 1 )
				{
				*Buffer++	= c;
				Length++;
				}
			Cookie->Column++;
			c	= GetChar( Cookie );
			}
		while( isalnum( c ) || c == '_' );
		Buffer != NULL && ( *Buffer = '\0' );
		UngetChar( Cookie, c );

		return TOKEN_NAME;
		}

	if( isdigit( c ) || c == '+' || c == '-' )
		{
		int	Length  = 0;
        int Signed  = c == '+' || c == '-';
        int Float   = 0;

		do
			{
            if( c == '.' || c == ',' )
                {
                if( Float != 0 )
                    longjmp( Cookie->JumpBuffer, -1 );
                Float = 1;
                }

			if( Buffer != NULL && Length < BufferSize - 1 )
				{
				*Buffer++	= c;
				Length++;
				}
			Cookie->Column++;
			c	= GetChar( Cookie );
			}
		while( isdigit( c ) || c == '.' );
		Buffer != NULL && ( *Buffer = '\0' );
		UngetChar( Cookie, c );

		return Float != 0 ? TOKEN_FLOAT : Signed != 0 ? TOKEN_INTEGER : TOKEN_UNSIGNED;
		}

	return TOKEN_INVALID;
	}
/*============================================================================*/
void UngetToken( cookie_t *Cookie, token_t Token, const char *TokenString )
    {
    if( TokenString != NULL )
        strncpy( Cookie->UngetBuffer, TokenString, sizeof Cookie->UngetBuffer );
    else
        Cookie->UngetBuffer[0]  = '\0';

    Cookie->UngettedToken   = Token;
    }
/*============================================================================*/
