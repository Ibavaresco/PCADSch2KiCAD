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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "Parser.h"
#include "PCADOutputSchematic.h"
#include "KiCADOutputSchematic.h"
/*============================================================================*/
void SplitPath( const char *pFullPath, char *pPath, char *pName, char *pExt )
	{
	char	FullPath[256], *p, *q;

	strncpy( FullPath, pFullPath, sizeof FullPath );
	FullPath[sizeof FullPath - 1]	= '\0';

	if(( p = strrchr( FullPath, '\\' )) != NULL )
		{
		*p++	= '\0';
		if( pPath != NULL )
			{
			strcpy( pPath, FullPath );
			strcat( pPath, "\\" );
			}
		}
	else if(( p = strrchr( FullPath, ':' )) != NULL )
		{
		*p++	= '\0';
		if( pPath != NULL )
			{
			strcpy( pPath, FullPath );
			strcat( pPath, ":" );
			}
		}
	else
		{
		p		= FullPath;
		if( pPath != NULL )
			strcpy( pPath, "" );
		}

	if(( q = strrchr( p, '.' )) != NULL )
		{
		if( pExt != NULL )
			strcpy( pExt, q );
		*q		= '\0';
		}
	else
		strcpy( pExt, "" );

	if( pName != NULL )
		strcpy( pName, p );
	}
/*============================================================================*/
#define	OUTPUTFORMAT_INVALID	0
#define	OUTPUTFORMAT_KICAD		1
#define	OUTPUTFORMAT_PCAD		2
/*============================================================================*/
static const char KiCADExtension[]	= ".kicad_sch";
static const char PCADExtension[]	= ".sch";
//static const char BackUpExtension[]	= ".cvt_bak";
/*============================================================================*/
static int Process( FILE *f, const char *pNameIn, const char *pNameOut, int OutputFormat )
	{
	pcad_schematicfile_t	*s;
	char					NameIn[256], ExtIn[256], PathOut[256], NameOut[256], ExtOut[256] /*, PathBkp[256]*/;
	cookie_t				Cookie;
	size_t					size;
	uint8_t					*heap;

	fseek( f, 0, SEEK_END );
	size					= sizeof( void* ) * ftell( f );
	fseek( f, 0, SEEK_SET );

	heap					= malloc( size );
	if( heap == NULL )
		{
		printf( "\nError: Not enough memory.\n\n" );
		return -1;
		}

	Cookie.File				= f;
	Cookie.LineNumber		= 1;
	Cookie.Column			= 1;
	Cookie.TabSize			= 4;
	Cookie.HeapSize			= size;
	Cookie.HeapTop			= 0;
	Cookie.Heap				= heap;
	Cookie.FileUnits		= PCAD_UNITS_MIL;
	Cookie.UngettedToken	= TOKEN_NONE;
	Cookie.UngetBuffer[0]	= '\0';
	Cookie.Sort				= 1;

	if( setjmp( Cookie.JumpBuffer ) != 0 )
		return -1;

	memset( heap, 0x00, size );

	s	= ParsePCAD( &Cookie, pNameIn, pNameOut );

	SplitPath( pNameIn, NULL, NameIn, ExtIn );

	if( pNameOut == NULL )
		{
		strcpy( PathOut, "" );
		strcpy( NameOut, NameIn );
		if( OutputFormat == OUTPUTFORMAT_PCAD )
			{
//			if( strcmp( ExtIn, "" ) == 0 )
//				strcpy( ExtOut, PCADExtension );
//			else
				strcpy( ExtOut, ExtIn );
			}
		else
			strcpy( ExtOut, KiCADExtension );
		}
	else
		{
		char	Temp[256];

		if( pNameOut[0] != '\'' )
			strcpy( Temp, pNameOut );
		else
			{
			int len = strlen( pNameOut );
			strcpy( Temp, &pNameOut[1] );
			if( len >= 3 && Temp[len-2] != '\'' )
				Error( &Cookie, -1, "Invalid File Name %s", pNameOut );
			Temp[len-2]	= '\0';
			}

		SplitPath( Temp, PathOut, NameOut, ExtOut );
		if( strcmp( NameOut, "" ) == 0 && strcmp( ExtOut, "" ) == 0 )
			{
			strcpy( NameOut, NameIn );
			if( OutputFormat == OUTPUTFORMAT_PCAD )
				strcpy( ExtOut, ExtIn );
			else
				strcpy( ExtOut, KiCADExtension );
			}
		else
			{
			if( strcmp( NameOut, "*" ) == 0 )
				strcpy( NameOut, NameIn );
			if( strcmp( ExtOut, ".*" ) == 0 )
				strcpy( ExtOut, ExtIn );
			else if( strcmp( ExtOut, "" ) == 0 )
				{
				if( OutputFormat == OUTPUTFORMAT_PCAD )
					strcpy( ExtOut, PCADExtension );
				else
					strcpy( ExtOut, KiCADExtension );
				}
			}
		}

	strcat( PathOut, NameOut );
	strcat( PathOut, ExtOut );

/*
	strcpy( PathBkp, PathOut );
	strcat( PathBkp, BackUpExtension );

	remove( PathBkp );
	rename( PathOut, PathBkp );
*/

	if( OutputFormat == OUTPUTFORMAT_PCAD )
		OutputPCAD( &Cookie, s, PathOut );
	else
		OutputKiCAD( &Cookie, s, PathOut );

	free( heap );

	return 0;
	}
/*============================================================================*/
void PrintUsage( int OutputFormat )
	{
#if			defined __linux__
	fprintf( stderr,
#else	/*	defined __linux__ */
	_fprintf_p( stderr,
#endif	/*	defined __linux__ */
		"\n"
		"%1$s v0.9\n"
		"Copyright(c) 2024, Isaac Marino Bavaresco\n"
		__DATE__ " " __TIME__ "\n\n"
		"%2$s.\n\n"
		"Usage: %1$s [--kicadout|--pcadout] [<pathin>]<filenamein>[.<extin>] [<pathout>][<filenameout|*>[.<extout|*>]]\n\n"
		"\"--pcadout\"	forces the output file to be in P-CAD format.\n"
		"\"--kicadout\" forces the output file to be in KiCAD format.\n"
#if			!defined __linux__
		"If the executable file name is \"PCADSch2KiCAD.exe\", the default output format is KiCAD.\n"
		"If it is \"PCADSchSort.exe\", the default output format is P-CAD.\n\n"
#endif	/*	!defined __linux__ */
		"If <extin> is omitted, the program will first try to open the file with the name as is, and if\n"
		"it can't, it will append the extension \".sch\" and try again.\n\n"
		"If <filenameout> is omitted or it is \'*\', the program will use <filenamein>. If <extout> is\n"
		"%3$s.\n"
		"If any \'*\' is used, the entire output name and extension must be enclosed in \' (single quotes)\n"
		"to prevent the C startup code to expand the \'*\' into a file name list.\n\n",

		OutputFormat == OUTPUTFORMAT_KICAD ? "PCADSch2KiCAD" : "PCADSchSort",
		OutputFormat == OUTPUTFORMAT_KICAD ? "Converts a P-CAD 2006 ASCII schematic file to KiCAD v8.0 format" : "Reads a P-CAD 2006 ASCII schematic file and outputs it with all the fields sorted\nin order to help with version control",
		OutputFormat == OUTPUTFORMAT_KICAD ? "\'*\', the program will use <extin>, if it is omitted, \".kicad_sch\" will be used" : "omitted or it is \'*\', the program will use <extin>"
		);

	}
/*============================================================================*/
/* Black magic, necessary so C runtime won't expand the '*' in the command-line arguments.	*/
int	_CRT_glob	= 0;
/*============================================================================*/
int main( int ArgC, char *ArgV[] )
	{
	char	PathIn[256];
	FILE	*f;
	int		Result, OutputFormat	= OUTPUTFORMAT_INVALID, FirstArg	= 1;

	if( ArgC > 1 )
		{
		FirstArg	= 2;
		if( stricmp( ArgV[1], "--kicadout" ) == 0 )
			OutputFormat	= OUTPUTFORMAT_KICAD;
		else if( stricmp( ArgV[1], "--pcadout" ) == 0 )
			OutputFormat	= OUTPUTFORMAT_PCAD;
		else
			FirstArg	= 1;
		}

	if( OutputFormat == OUTPUTFORMAT_INVALID )
#if			defined __linux__
		OutputFormat = OUTPUTFORMAT_KICAD;
#else	/*	defined __linux__ */
		{
		SplitPath( ArgV[0], NULL, PathIn, NULL );
		if( stricmp( PathIn, "PCADSch2KiCAD" ) == 0 )
			OutputFormat = OUTPUTFORMAT_KICAD;
		else if( stricmp( PathIn, "PCADSchSort" ) == 0 )
			OutputFormat = OUTPUTFORMAT_PCAD;
		else
			{
			printf( "\nError: The name of this file should be either \"PCADSch2KiCAD.exe\" or \"PCADSchSort.exe\".\n\n" );
			return -1;
			}
		}
#endif	/*	defined __linux__ */

	if( ArgC - FirstArg < 1 || ArgC - FirstArg > 2 )
		{
		PrintUsage( OutputFormat );
		if( ArgC != 1 )
			fprintf( stderr, "\nError in the arguments.\n\n" );
		return -1;
		}

	strcpy( PathIn, ArgV[FirstArg] );

	if(( f = fopen( PathIn, "rb" )) == NULL )
		{
		char	ExtIn[256];

		SplitPath( ArgV[FirstArg], NULL, NULL, ExtIn );
		strcat( PathIn, PCADExtension );

		if( strcmp( ExtIn, PCADExtension ) == 0 || ( f = fopen( PathIn, "rb" )) == NULL )
			{
			PrintUsage( OutputFormat );
			fprintf( stderr, "\nError opening file \"%s\".\n\n", ArgV[FirstArg] );
			return -1;
			}
		}

	Result	= Process( f, PathIn, ArgC - FirstArg == 2 ? ArgV[FirstArg+1] : NULL, OutputFormat );

	fclose( f );

	return Result;
	}
/*============================================================================*/
