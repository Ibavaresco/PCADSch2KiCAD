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
#include <fcntl.h>
#include <sys/stat.h>
#include <stddef.h>
#include "Parser.h"
#include "PCADParser.h"
#include "Lexic.h"
#include "PCADProcessSchematic.h"
/*============================================================================*/
/*============================================================================*/
/*============================================================================*/
/*============================================================================*/
/*
The three functions below deal with special cases where the automated table-driven
parsing is not enough.
*/
/*============================================================================*/
/*
When we find the standard units used for the entire file, we must save it where
all functions can access it.
*/
/*----------------------------------------------------------------------------*/
static int Parse_FileUnits( cookie_t *Cookie, const parsefield_t *ParseField, const parsestruct_t *ParseStruct, void *Argument )
	{
	ParseEnum( Cookie, ParseField, ParseStruct, Argument );

	Cookie->FileUnits	= *(pcad_units_t*)Argument;

	return 0;
	}
/*============================================================================*/
static int Parse_LineInWire( cookie_t *Cookie, const parsefield_t *ParseField, const parsestruct_t *ParseStruct, void *Argument )
	{
	char		Buffer[BUFFER_SIZE];
	pcad_wire_t	*Wire	= Argument;

	ExpectToken( Cookie, TOKEN_OPEN_PAR );
	ExpectName( Cookie, "pt" );
	ParseGeneric( Cookie, &ParseStruct->FixedFields[0], ParseStruct->FixedFields[0].ParseStruct, &Wire->pt1 );
	ExpectToken( Cookie, TOKEN_CLOSE_PAR );

	ExpectToken( Cookie, TOKEN_OPEN_PAR );
	GetName( Cookie, Buffer, sizeof Buffer );

	if( stricmp( Buffer, "endStyle" ) == 0 )
		{
		ParseEnum( Cookie, &ParseStruct->FixedFields[1], ParseStruct->FixedFields[1].ParseStruct, &Wire->endstyle1 );
		ExpectToken( Cookie, TOKEN_CLOSE_PAR );

		ExpectToken( Cookie, TOKEN_OPEN_PAR );
		GetName( Cookie, Buffer, sizeof Buffer );
		}

	if( strcmp( Buffer, "pt" ) != 0 )
		Error( Cookie, -1, "Expecting \"pt\"" );

	ParseGeneric( Cookie, &ParseStruct->FixedFields[2], ParseStruct->FixedFields[2].ParseStruct, &Wire->pt2 );
	ExpectToken( Cookie, TOKEN_CLOSE_PAR );

	ExpectToken( Cookie, TOKEN_OPEN_PAR );
	GetName( Cookie, Buffer, sizeof Buffer );

	if( stricmp( Buffer, "endStyle" ) == 0 )
		{
		ParseEnum( Cookie, &ParseStruct->FixedFields[3], ParseStruct->FixedFields[3].ParseStruct, &Wire->endstyle2 );
		ExpectToken( Cookie, TOKEN_CLOSE_PAR );

		ExpectToken( Cookie, TOKEN_OPEN_PAR );
		GetName( Cookie, Buffer, sizeof Buffer );
		}

	if( strcmp( Buffer, "width" ) != 0 )
		Error( Cookie, -1, "Expecting \"width\"" );

	ParseDimmension( Cookie, &ParseStruct->FixedFields[4], ParseStruct->FixedFields[4].ParseStruct, &Wire->width );
	ExpectToken( Cookie, TOKEN_CLOSE_PAR );

	ExpectToken( Cookie, TOKEN_OPEN_PAR );
	ExpectName( Cookie, "netNameRef" );
	ParseString( Cookie, &ParseStruct->FixedFields[5], ParseStruct->FixedFields[5].ParseStruct, &Wire->netnameref );
	ExpectToken( Cookie, TOKEN_CLOSE_PAR );

	return 0;
	}
/*============================================================================*/
static int Parse_PadPinMap( cookie_t *Cookie, const parsefield_t *ParseField, const parsestruct_t *ParseStruct, void *Argument )
	{
	char				Buffer[BUFFER_SIZE];
	pcad_padpinmap_t	*Object;
	token_t				Token;
	void				***Parent	= Argument;

	ExpectToken( Cookie, TOKEN_OPEN_PAR );

	do
		{
		Object		= Allocate( Cookie, sizeof( pcad_padpinmap_t ));
		**Parent	= Object;
		if( ParseStruct != NULL && ParseStruct->OffsetNext >= 0 )
			*Parent = (void*)( (char*)Object + ParseStruct->OffsetNext );

		ExpectName( Cookie, "padNum" );
		Object->padnum		= GetUnsigned( Cookie );
		ExpectToken( Cookie, TOKEN_CLOSE_PAR );
		ExpectToken( Cookie, TOKEN_OPEN_PAR );
		ExpectName( Cookie, "compPinRef" );
		Object->comppinref	= GetAndStoreString( Cookie );
		ExpectToken( Cookie, TOKEN_CLOSE_PAR );
		}
	while(( Token = GetToken( Cookie, Buffer, sizeof Buffer )) == TOKEN_OPEN_PAR );

	UngetToken( Cookie, Token, Buffer );

	return 0;
	}
/*============================================================================*/
/*
Grids are given as strings inside quotes and can have an optional unit, cannot
be parsed by the standard routines.
*/
/*----------------------------------------------------------------------------*/
static int Parse_Grid( cookie_t *Cookie, const parsefield_t *ParseField, const parsestruct_t *ParseStruct, void *Argument )
	{
	char			Buffer[BUFFER_SIZE], Number[32], Unit[32];
	pcad_grid_t		*Grid	= Argument;
	int				i, j;

	GetString( Cookie, Buffer, sizeof Buffer );

	for( i = 0; isdigit( Buffer[i] ) && i < sizeof Number - 1; i++ )
		Number[i]	= Buffer[i];
	if( i >= sizeof Number - 1 )
		Error( Cookie, -1, "Number too large" );
	if( Buffer[i] == '.' || Buffer[i] == ',' )
		{
		if( i >= sizeof Number - 1 )
			Error( Cookie, -1, "Number too large" );
		Number[i]	= Buffer[i];
		i++;
		for( ; isdigit( Buffer[i] ) && i < sizeof Number - 1; i++ )
			Number[i]	= Buffer[i];
		if( i >= sizeof Number - 1 )
			Error( Cookie, -1, "Number too large" );
		}
	Number[i]	= '\0';

	for( ; isspace( Buffer[i] ); i++ )
		{}

	for( j = 0; isalpha( Buffer[i] ) && j < sizeof Unit - 1; i++, j++ )
		Unit[j] = Buffer[i];

	if( j >= sizeof Unit - 1 )
		Error( Cookie, -1, "Invalid unit" );

	Unit[j] = '\0';

	Grid->grid	= ProcessDimmension( Cookie, Number, TranslateUnits( Cookie, Unit ));

	return 0;
	}
/*===========================================================================*/
/*============================================================================*/
/*============================================================================*/
/*============================================================================*/
static const parsefield_t	Point_FixedFields[]	=
	{
	/*	Flags			TagString	ParseFunction		Size	Offset							ParseStruct */
		{ FLAG_NAKED,	NULL,		ParseDimmension,	0,		offsetof( pcad_point_t, x ),	NULL },
		{ FLAG_NAKED,	NULL,		ParseDimmension,	0,		offsetof( pcad_point_t, y ),	NULL }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	Point_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields = LENGTH( Point_FixedFields ),
	.FixedFields	= Point_FixedFields,
	.NumFields		=  0,
	.Fields			= NULL,
	.NumLists		=  0,
	.Lists			= NULL,
	.OffsetNext		= offsetof( pcad_point_t, next )
	};
/*============================================================================*/
static const parsefield_t	Attr_FixedFields[]	=
	{
	/*	Flags			TagString			ParseFunction		Size	Offset										ParseStruct */
		{ FLAG_NAKED,	NULL,				ParseString,		0,		offsetof( pcad_attr_t, name ),				NULL },
		{ FLAG_NAKED,	NULL,				ParseString,		0,		offsetof( pcad_attr_t, value ),				NULL }
	};
/*----------------------------------------------------------------------------*/
static const parsefield_t	Attr_Fields[]	=
	{
	/*	Flags			TagString			ParseFunction		Size	Offset										ParseStruct */
		{ FLAG_WRAPPED, "pt",				ParseGeneric,		0,		offsetof( pcad_attr_t, point ),				&Point_ParseStruct },
		{ FLAG_WRAPPED, "rotation",			ParseReal,			0,		offsetof( pcad_attr_t, rotation ),			NULL },
		{ FLAG_WRAPPED, "isVisible",		ParseBoolean,		0,		offsetof( pcad_attr_t, isvisible ),			NULL },
		{ FLAG_WRAPPED, "isFlipped",		ParseBoolean,		0,		offsetof( pcad_attr_t, isflipped ),			NULL },
		{ FLAG_WRAPPED, "justify",			ParseEnum,			0,		offsetof( pcad_attr_t, justify ),			(const parsestruct_t*)&Justify },
		{ FLAG_WRAPPED, "textStyleRef",		ParseString,		0,		offsetof( pcad_attr_t, textstyleref ),		NULL },
		{ FLAG_WRAPPED, "constraintUnits",	ParseEnum,			0,		offsetof( pcad_attr_t, constraintunits ),	(const parsestruct_t*)&ConstraintUnits }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	Attr_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields = LENGTH( Attr_FixedFields ),
	.FixedFields	= Attr_FixedFields,
	.NumFields		= LENGTH( Attr_Fields ),
	.Fields			= Attr_Fields,
	.NumLists		=  0,
	.Lists			= NULL,
	.OffsetNext		= offsetof( pcad_attr_t, next )
	};
/*============================================================================*/
static const parsefield_t	TriplePointArc_FixedFields[]	=
	{
	/*	Flags			TagString	ParseFunction		Size	Offset										ParseStruct */
		{ FLAG_WRAPPED, "pt",		ParseGeneric,		0,		offsetof( pcad_triplepointarc_t, point1 ),	&Point_ParseStruct },
		{ FLAG_WRAPPED, "pt",		ParseGeneric,		0,		offsetof( pcad_triplepointarc_t, point2 ),	&Point_ParseStruct },
		{ FLAG_WRAPPED, "pt",		ParseGeneric,		0,		offsetof( pcad_triplepointarc_t, point3 ),	&Point_ParseStruct },
		{ FLAG_WRAPPED, "width",	ParseDimmension,	0,		offsetof( pcad_triplepointarc_t, width ),	NULL }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	TriplePointArc_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields = LENGTH( TriplePointArc_FixedFields ),
	.FixedFields	= TriplePointArc_FixedFields,
	.NumFields		=  0,
	.Fields			= NULL,
	.NumLists		=  0,
	.Lists			= NULL,
	.OffsetNext		= offsetof( pcad_triplepointarc_t, next )
	};
/*============================================================================*/
static const parsefield_t	IEEESymbol_FixedFields[]	=
	{
	/*	Flags			TagString		ParseFunction		Size	Offset										ParseStruct */
		{ FLAG_NAKED,	NULL,			ParseEnum,			0,		offsetof( pcad_ieeesymbol_t, type ),		(const parsestruct_t*)&IEEESymbols },
		{ FLAG_WRAPPED, "pt",			ParseGeneric,		0,		offsetof( pcad_ieeesymbol_t, point ),		&Point_ParseStruct },
		{ FLAG_WRAPPED, "height",		ParseDimmension,	0,		offsetof( pcad_ieeesymbol_t, height ),		NULL }
	};
/*----------------------------------------------------------------------------*/
static const parsefield_t	IEEESymbol_Fields[]	=
	{
	/*	Flags			TagString		ParseFunction		Size	Offset										ParseStruct */
		{ FLAG_WRAPPED, "isflipped",	ParseBoolean,		0,		offsetof( pcad_ieeesymbol_t, isflipped ),	NULL }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	IEEESymbol_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields = LENGTH( IEEESymbol_FixedFields ),
	.FixedFields	= IEEESymbol_FixedFields,
	.NumFields		= LENGTH( IEEESymbol_Fields ),
	.Fields			= IEEESymbol_Fields,
	.NumLists		=  0,
	.Lists			= NULL,
	.OffsetNext		= offsetof( pcad_ieeesymbol_t, next )
	};
/*============================================================================*/
static const parsefield_t	Line_FixedFields[]	=
	{
	/*	Flags			TagString			ParseFunction		Size	Offset									ParseStruct */
		{ FLAG_WRAPPED, "pt",				ParseGeneric,		0,		offsetof( pcad_line_t, pt1 ),			&Point_ParseStruct },
		{ FLAG_WRAPPED, "pt",				ParseGeneric,		0,		offsetof( pcad_line_t, pt2 ),			&Point_ParseStruct }
	};
/*----------------------------------------------------------------------------*/
static const parsefield_t	Line_Fields[]	=
	{
	/*	Flags			TagString			ParseFunction		Size	Offset									ParseStruct */
		{ FLAG_WRAPPED, "style",			ParseEnum,			0,		offsetof( pcad_line_t, style ),			(const parsestruct_t*)&LineStyles },
		{ FLAG_WRAPPED, "width",			ParseDimmension,	0,		offsetof( pcad_line_t, width ),			NULL }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	Line_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields = LENGTH( Line_FixedFields ),
	.FixedFields	= Line_FixedFields,
	.NumFields		= LENGTH( Line_Fields ),
	.Fields			= Line_Fields,
	.NumLists		=  0,
	.Lists			= NULL,
	.OffsetNext		= offsetof( pcad_line_t, next )
	};
/*============================================================================*/
static const parsefield_t	Extent_Fields[]	=
	{
	/*	Flags			TagString	ParseFunction		Size	Offset									ParseStruct */
		{ FLAG_NAKED,	NULL,		ParseDimmension,	0,		offsetof( pcad_extent_t, extentx ),		NULL },
		{ FLAG_NAKED,	NULL,		ParseDimmension,	0,		offsetof( pcad_extent_t, extenty ),		NULL }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	Extent_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields = LENGTH( Extent_Fields ),
	.FixedFields	= Extent_Fields,
	.NumFields		=  0,
	.Fields			= NULL,
	.NumLists		=  0,
	.Lists			= NULL,
	.OffsetNext		= -1
	};
/*============================================================================*/
static const parsefield_t	Text_FixedFields[]	=
	{
	/*	Flags			TagString		ParseFunction		Size	Offset									ParseStruct */
		{ FLAG_WRAPPED, "pt",			ParseGeneric,		0,		offsetof( pcad_text_t, point ),			&Point_ParseStruct },
		{ FLAG_NAKED,	NULL,			ParseString,		0,		offsetof( pcad_text_t, value ),			NULL },
	};
/*----------------------------------------------------------------------------*/
static const parsefield_t	Text_Fields[]	=
	{
	/*	Flags			TagString		ParseFunction		Size	Offset									ParseStruct */
		{ FLAG_WRAPPED, "textStyleRef",	ParseString,		0,		offsetof( pcad_text_t, textstyleref ),	NULL },
		{ FLAG_WRAPPED, "rotation",		ParseReal,			0,		offsetof( pcad_text_t, rotation ),		NULL },
		{ FLAG_WRAPPED, "isFlipped",	ParseBoolean,		0,		offsetof( pcad_text_t, isflipped ),		NULL },
		{ FLAG_WRAPPED, "justify",		ParseEnum,			0,		offsetof( pcad_text_t, justify ),		(const parsestruct_t*)&Justify },
		{ FLAG_WRAPPED, "extent",		ParseGeneric,		0,		offsetof( pcad_text_t, extent ),		&Extent_ParseStruct }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	Text_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields = LENGTH( Text_FixedFields ),
	.FixedFields	= Text_FixedFields,
	.NumFields		= LENGTH( Text_Fields ),
	.Fields			= Text_Fields,
	.NumLists		=  0,
	.Lists			= NULL,
	.OffsetNext		= offsetof( pcad_text_t, next )
	};
/*============================================================================*/
static const parsefield_t	ASCIIVersion_Fields[]	=
	{
	/*	Flags			TagString			ParseFunction		Size	Offset									ParseStruct */
		{ FLAG_NAKED,	NULL,				ParseUnsigned,		0,		offsetof( pcad_asciiversion_t, high ),	NULL },
		{ FLAG_NAKED,	NULL,				ParseUnsigned,		0,		offsetof( pcad_asciiversion_t, low ),	NULL },
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	ASCIIVersion_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields = LENGTH( ASCIIVersion_Fields ),
	.FixedFields	= ASCIIVersion_Fields,
	.NumFields		=  0,
	.Fields			= NULL,
	.NumLists		=  0,
	.Lists			= NULL,
	.OffsetNext		= -1
	};
/*============================================================================*/
static const parsefield_t	TimeStamp_Fields[]	=
	{
	/*	Flags			TagString			ParseFunction		Size	Offset									ParseStruct */
		{ FLAG_NAKED,	NULL,				ParseUnsigned,		0,		offsetof( pcad_timestamp_t, year ),		NULL },
		{ FLAG_NAKED,	NULL,				ParseUnsigned,		0,		offsetof( pcad_timestamp_t, month ),	NULL },
		{ FLAG_NAKED,	NULL,				ParseUnsigned,		0,		offsetof( pcad_timestamp_t, day ),		NULL },
		{ FLAG_NAKED,	NULL,				ParseUnsigned,		0,		offsetof( pcad_timestamp_t, hour ),		NULL },
		{ FLAG_NAKED,	NULL,				ParseUnsigned,		0,		offsetof( pcad_timestamp_t, minute ),	NULL },
		{ FLAG_NAKED,	NULL,				ParseUnsigned,		0,		offsetof( pcad_timestamp_t, second ),	NULL },
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	TimeStamp_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields = LENGTH( TimeStamp_Fields ),
	.FixedFields	= TimeStamp_Fields,
	.NumFields		=  0,
	.Fields			= NULL,
	.NumLists		=  0,
	.Lists			= NULL,
	.OffsetNext		= -1
	};
/*============================================================================*/
static const parsefield_t	Program_Fields[]	=
	{
	/*	Flags			TagString			ParseFunction		Size	Offset									ParseStruct */
		{ FLAG_NAKED,	NULL,				ParseString,		0,		offsetof( pcad_program_t, name ),		NULL },
		{ FLAG_NAKED,	NULL,				ParseString,		0,		offsetof( pcad_program_t, version ),	NULL }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	Program_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields = LENGTH( Program_Fields ),
	.FixedFields	= Program_Fields,
	.NumFields		=  0,
	.Fields			= NULL,
	.NumLists		=  0,
	.Lists			= NULL,
	.OffsetNext		= -1
	};
/*============================================================================*/
static const parsefield_t	ASCIIHeader_Fields[]	=
	{
	/*	Flags			TagString			ParseFunction		Size	Offset											ParseStruct */
		{ FLAG_WRAPPED, "asciiVersion",		ParseGeneric,		0,		offsetof( pcad_asciiheader_t, asciiversion ),	&ASCIIVersion_ParseStruct },
		{ FLAG_WRAPPED, "timeStamp",		ParseGeneric,		0,		offsetof( pcad_asciiheader_t, timestamp ),		&TimeStamp_ParseStruct },
		{ FLAG_WRAPPED, "program",			ParseGeneric,		0,		offsetof( pcad_asciiheader_t, program ),		&Program_ParseStruct },
		{ FLAG_WRAPPED, "copyright",		ParseString,		0,		offsetof( pcad_asciiheader_t, copyright ),		NULL },
		{ FLAG_WRAPPED, "fileAuthor",		ParseString,		0,		offsetof( pcad_asciiheader_t, fileauthor ),		NULL },
		{ FLAG_WRAPPED, "headerString",		ParseString,		0,		offsetof( pcad_asciiheader_t, headerstring ),	NULL },
		{ FLAG_WRAPPED, "fileUnits",		Parse_FileUnits,	0,		offsetof( pcad_asciiheader_t, fileunits ),		(const parsestruct_t*)&Units },
		{ FLAG_WRAPPED, "guidString",		ParseString,		0,		offsetof( pcad_asciiheader_t, guidstring ),		NULL }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	ASCIIHeader_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields = LENGTH( ASCIIHeader_Fields ),
	.FixedFields	= ASCIIHeader_Fields,
	.NumFields		=  0,
	.Fields			= NULL,
	.NumLists		=  0,
	.Lists			= NULL,
	.OffsetNext		= -1
	};
/*============================================================================*/
static const parsefield_t	Font_Fields[]	=
	{
	/*	Flags			TagString				ParseFunction		Size	Offset											ParseStruct */
		{ FLAG_WRAPPED, "fontType",				ParseEnum,			0,		offsetof( pcad_font_t, fonttype ),				(const parsestruct_t*)&FontType },
		{ FLAG_WRAPPED, "fontFamily",			ParseName,			0,		offsetof( pcad_font_t, fontfamily ),			NULL },
		{ FLAG_WRAPPED, "fontFace",				ParseString,		0,		offsetof( pcad_font_t, fontface ),				NULL },
		{ FLAG_WRAPPED, "fontHeight",			ParseDimmension,	0,		offsetof( pcad_font_t, fontheight ),			NULL },
		{ FLAG_WRAPPED, "strokeWidth",			ParseDimmension,	0,		offsetof( pcad_font_t, strokewidth ),			NULL },
		{ FLAG_WRAPPED, "fontWeight",			ParseUnsigned,		0,		offsetof( pcad_font_t, fontweight ),			NULL },
		{ FLAG_WRAPPED, "fontCharSet",			ParseUnsigned,		0,		offsetof( pcad_font_t, fontcharset ),			NULL },
		{ FLAG_WRAPPED, "fontOutPrecision",		ParseUnsigned,		0,		offsetof( pcad_font_t, fontoutprecision ),		NULL },
		{ FLAG_WRAPPED, "fontClipPrecision",	ParseUnsigned,		0,		offsetof( pcad_font_t, fontclipprecision ),		NULL },
		{ FLAG_WRAPPED, "fontQuality",			ParseUnsigned,		0,		offsetof( pcad_font_t, fontquality ),			NULL },
		{ FLAG_WRAPPED, "fontPitchAndFamily",	ParseUnsigned,		0,		offsetof( pcad_font_t, fontpitchandfamily ),	NULL }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	Font_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields =  0,
	.FixedFields	= NULL,
	.NumFields		= LENGTH( Font_Fields ),
	.Fields			= Font_Fields,
	.NumLists		=  0,
	.Lists			= NULL,
	.OffsetNext		= offsetof( pcad_font_t, next )
	};
/*============================================================================*/
static const listhead_t	Poly_Lists[]		=
	{
		{ .OffsetHead = offsetof( pcad_poly_t, firstpoint ),	.OffsetLink = offsetof( pcad_poly_t, viopoints ) },
	};
/*----------------------------------------------------------------------------*/
static const parsefield_t	Poly_Fields[]	=
	{
	/*	Flags						TagString	ParseFunction	Size					Offset								ParseStruct */
		{ FLAG_WRAPPED | FLAG_LIST,	"pt",		ParseGeneric,	sizeof( pcad_point_t ), offsetof( pcad_poly_t, viopoints ), &Point_ParseStruct }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	Poly_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields =  0,
	.FixedFields	= NULL,
	.NumFields		= LENGTH( Poly_Fields ),
	.Fields			= Poly_Fields,
	.NumLists		= LENGTH( Poly_Lists ),
	.Lists			= Poly_Lists,
	.OffsetNext		= offsetof( pcad_poly_t, next )
	};
/*============================================================================*/
static const parsefield_t	PinDisplay_Fields[]	=
	{
	/*	Flags			TagString		ParseFunction	Size	Offset										ParseStruct */
		{ FLAG_WRAPPED, "dispPinDes",	ParseBoolean,	0,		offsetof( pcad_pin_t, displaypindes ),		NULL },
		{ FLAG_WRAPPED, "dispPinName",	ParseBoolean,	0,		offsetof( pcad_pin_t, displaypinname ),		NULL }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	PinDisplay_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields =  0,
	.FixedFields	= NULL,
	.NumFields		= LENGTH( PinDisplay_Fields ),
	.Fields			= PinDisplay_Fields,
	.NumLists		=  0,
	.Lists			= NULL,
	.OffsetNext		= offsetof( pcad_pin_t, next )
	};
/*============================================================================*/
static const parsefield_t	PinNameDes_FixedFields[]	=
	{
	/*	Flags			TagString			ParseFunction		Size	Offset			ParseStruct */
		{ FLAG_WRAPPED, "text",				ParseGeneric,		0,		0,				&Text_ParseStruct }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	PinNameDes_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields = LENGTH( PinNameDes_FixedFields ),
	.FixedFields	= PinNameDes_FixedFields,
	.NumFields		=  0,
	.Fields			= NULL,
	.NumLists		=  0,
	.Lists			= NULL,
	.OffsetNext		= -1
	};
/*============================================================================*/
static const parsefield_t	Pin_FixedFields[]	=
	{
	/*	Flags			TagString			ParseFunction		Size	Offset										ParseStruct */
		{ FLAG_WRAPPED, "pinNum",			ParseUnsigned,		0,		offsetof( pcad_pin_t, pinnum ),				NULL },
		{ FLAG_WRAPPED, "pt",				ParseGeneric,		0,		offsetof( pcad_pin_t, point ),				&Point_ParseStruct }
	};
/*----------------------------------------------------------------------------*/
static const parsefield_t	Pin_Fields[]	=
	{
	/*	Flags			TagString			ParseFunction		Size	Offset										ParseStruct */
		{ FLAG_WRAPPED, "rotation",			ParseReal,			0,		offsetof( pcad_pin_t, rotation ),			NULL },
		{ FLAG_WRAPPED, "pinLength",		ParseDimmension,	0,		offsetof( pcad_pin_t, pinlength ),			NULL },
		{ FLAG_WRAPPED, "outsideEdgeStyle",	ParseEnum,			0,		offsetof( pcad_pin_t, outsideedgestyle ),	(const parsestruct_t*)&OutsideEdgeStyle },
		{ FLAG_WRAPPED, "insideEdgeStyle",	ParseEnum,			0,		offsetof( pcad_pin_t, insideedgestyle ),	(const parsestruct_t*)&InsideEdgeStyle },
		{ FLAG_WRAPPED, "outsideStyle",		ParseEnum,			0,		offsetof( pcad_pin_t, outsidestyle ),		(const parsestruct_t*)&OutsideStyle },
		{ FLAG_WRAPPED, "insideStyle",		ParseEnum,			0,		offsetof( pcad_pin_t, insidestyle ),		(const parsestruct_t*)&InsideStyle },
		{ FLAG_WRAPPED, "isFlipped",		ParseBoolean,		0,		offsetof( pcad_pin_t, isflipped ),			NULL },
		{ FLAG_WRAPPED, "pinDisplay",		ParseGeneric,		0,		0,											&PinDisplay_ParseStruct },
		{ FLAG_WRAPPED, "pinDes",			ParseGeneric,		0,		offsetof( pcad_pin_t, pindes ),				&PinNameDes_ParseStruct },
		{ FLAG_WRAPPED, "pinName",			ParseGeneric,		0,		offsetof( pcad_pin_t, pinname ),			&PinNameDes_ParseStruct },
		{ FLAG_WRAPPED, "defaultPinDes",	ParseString,		0,		offsetof( pcad_pin_t, defaultpindes ),		NULL }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	Pin_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields = LENGTH( Pin_FixedFields ),
	.FixedFields	= Pin_FixedFields,
	.NumFields		= LENGTH( Pin_Fields ),
	.Fields			= Pin_Fields,
	.NumLists		=  0,
	.Lists			= NULL,
	.OffsetNext		= offsetof( pcad_pin_t, next )
	};
/*============================================================================*/
static const listhead_t	SymbolDef_Lists[]		=
	{
		{ .OffsetHead = offsetof( pcad_symboldef_t, firstpin ),				.OffsetLink = offsetof( pcad_symboldef_t, viopins ) },
		{ .OffsetHead = offsetof( pcad_symboldef_t, firsttext ),			.OffsetLink = offsetof( pcad_symboldef_t, viotexts ) },
		{ .OffsetHead = offsetof( pcad_symboldef_t, firstpoly ),			.OffsetLink = offsetof( pcad_symboldef_t, viopolys ) },
		{ .OffsetHead = offsetof( pcad_symboldef_t, firstline ),			.OffsetLink = offsetof( pcad_symboldef_t, violines ) },
		{ .OffsetHead = offsetof( pcad_symboldef_t, firstieeesymbol ),		.OffsetLink = offsetof( pcad_symboldef_t, vioieeesymbols ) },
		{ .OffsetHead = offsetof( pcad_symboldef_t, firsttriplepointarc ),	.OffsetLink = offsetof( pcad_symboldef_t, viotriplepointarcs ) },
		{ .OffsetHead = offsetof( pcad_symboldef_t, firstattr ),			.OffsetLink = offsetof( pcad_symboldef_t, vioattrs ) }
	};
/*----------------------------------------------------------------------------*/
static const parsefield_t	SymbolDef_FixedFields[]	=
	{
	/*	Flags			TagString			ParseFunction	Size								Offset												ParseStruct */
		{ FLAG_NAKED,	NULL,				ParseString,	0,									offsetof( pcad_symboldef_t, name ),				NULL },
		{ FLAG_WRAPPED, "originalName",		ParseString,	0,									offsetof( pcad_symboldef_t, originalname ),		NULL },
	};
/*----------------------------------------------------------------------------*/
static const parsefield_t	SymbolDef_Fields[]	=
	{
	/*	Flags						TagString			ParseFunction	Size								Offset												ParseStruct */
		{ FLAG_WRAPPED | FLAG_LIST, "pin",				ParseGeneric,	sizeof( pcad_pin_t ),				offsetof( pcad_symboldef_t, viopins ),				&Pin_ParseStruct },
		{ FLAG_WRAPPED | FLAG_LIST, "text",				ParseGeneric,	sizeof( pcad_text_t ),				offsetof( pcad_symboldef_t, viotexts ),			&Text_ParseStruct },
		{ FLAG_WRAPPED | FLAG_LIST, "poly",				ParseGeneric,	sizeof( pcad_poly_t ),				offsetof( pcad_symboldef_t, viopolys ),			&Poly_ParseStruct },
		{ FLAG_WRAPPED | FLAG_LIST, "line",				ParseGeneric,	sizeof( pcad_line_t ),				offsetof( pcad_symboldef_t, violines ),			&Line_ParseStruct },
		{ FLAG_WRAPPED | FLAG_LIST, "ieeeSymbol",		ParseGeneric,	sizeof( pcad_ieeesymbol_t ),		offsetof( pcad_symboldef_t, vioieeesymbols ),		&IEEESymbol_ParseStruct },
		{ FLAG_WRAPPED | FLAG_LIST, "triplePointArc",	ParseGeneric,	sizeof( pcad_triplepointarc_t ),	offsetof( pcad_symboldef_t, viotriplepointarcs ),	&TriplePointArc_ParseStruct },
		{ FLAG_WRAPPED | FLAG_LIST, "attr",				ParseGeneric,	sizeof( pcad_attr_t ),				offsetof( pcad_symboldef_t, vioattrs ),			&Attr_ParseStruct }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	SymbolDef_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields = LENGTH( SymbolDef_FixedFields ),
	.FixedFields	= SymbolDef_FixedFields,
	.NumFields		= LENGTH( SymbolDef_Fields ),
	.Fields			= SymbolDef_Fields,
	.NumLists		= LENGTH( SymbolDef_Lists ),
	.Lists			= SymbolDef_Lists,
	.OffsetNext		= offsetof( pcad_symboldef_t, next )
	};
/*============================================================================*/
static const listhead_t	TextStyleDef_Lists[]		=
	{
		{ .OffsetHead = offsetof( pcad_textstyledef_t, firstfont ),	.OffsetLink = offsetof( pcad_textstyledef_t, viofonts ) },
	};
/*----------------------------------------------------------------------------*/
static const parsefield_t	TextStyleDef_FixedFields[]	=
	{
	/*	Flags			TagString					ParseFunction		Size					Offset											ParseStruct */
		{ FLAG_NAKED,	NULL,						ParseString,		0,						offsetof( pcad_textstyledef_t, name ),			NULL }
	};
/*----------------------------------------------------------------------------*/
static const parsefield_t	TextStyleDef_Fields[]	=
	{
	/*	Flags						TagString					ParseFunction		Size					Offset											ParseStruct */
		{ FLAG_WRAPPED | FLAG_LIST, "font",						ParseGeneric,		sizeof( pcad_font_t ),	offsetof( pcad_textstyledef_t, viofonts ),		&Font_ParseStruct },
		{ FLAG_WRAPPED | FLAG_LIST, "textStyleAllowTType",		ParseBoolean,		0,						offsetof( pcad_textstyledef_t, allowttype ),	NULL },
		{ FLAG_WRAPPED | FLAG_LIST, "textStyleDisplayTType",	ParseBoolean,		0,						offsetof( pcad_textstyledef_t, displayttype ),	NULL }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	TextStyleDef_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields = LENGTH( TextStyleDef_FixedFields ),
	.FixedFields	= TextStyleDef_FixedFields,
	.NumFields		= LENGTH( TextStyleDef_Fields ),
	.Fields			= TextStyleDef_Fields,
	.NumLists		= LENGTH( TextStyleDef_Lists ),
	.Lists			= TextStyleDef_Lists,
	.OffsetNext		= offsetof( pcad_textstyledef_t, next )
	};
/*============================================================================*/
static const parsefield_t	Alts_Fields[]	=
	{
	/*	Flags			TagString		ParseFunction	Size	Offset									ParseStruct */
		{ FLAG_WRAPPED, "ieeeAlt",		ParseBoolean,	0,		offsetof( pcad_alts_t, ieeealt ),		NULL },
		{ FLAG_WRAPPED, "deMorganAlt",	ParseBoolean,	0,		offsetof( pcad_alts_t, demorganalt ),	NULL }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	Alts_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields = LENGTH( Alts_Fields ),
	.FixedFields	= Alts_Fields,
	.NumFields		=  0,
	.Fields			= NULL,
	.NumLists		=  0,
	.Lists			= NULL,
	.OffsetNext		= -1
	};
/*============================================================================*/
static const parsefield_t	CompHeader_Fields[]	=
	{
	/*	Flags			TagString			ParseFunction	Size	Offset											ParseStruct */
		{ FLAG_WRAPPED, "sourceLibrary",	ParseString,	0,		offsetof( pcad_compheader_t, sourcelibrary ),	NULL },
		{ FLAG_WRAPPED, "compType",			ParseEnum,		0,		offsetof( pcad_compheader_t, comptype ),		(const parsestruct_t*)&CompType },
		{ FLAG_WRAPPED, "numPins",			ParseUnsigned,	0,		offsetof( pcad_compheader_t, numpins ),			NULL },
		{ FLAG_WRAPPED, "numParts",			ParseUnsigned,	0,		offsetof( pcad_compheader_t, numparts ),		NULL },
		{ FLAG_WRAPPED, "composition",		ParseEnum,		0,		offsetof( pcad_compheader_t, composition ),		(const parsestruct_t*)&Composition },
		{ FLAG_WRAPPED, "alts",				ParseGeneric,	0,		offsetof( pcad_compheader_t, alts ),			&Alts_ParseStruct },
		{ FLAG_WRAPPED, "refDesPrefix",		ParseString,	0,		offsetof( pcad_compheader_t, refdesprefix ),	NULL },
		{ FLAG_WRAPPED, "numType",			ParseEnum,		0,		offsetof( pcad_compheader_t, numtype ),			(const parsestruct_t*)&NumType }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	CompHeader_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields =  0,
	.FixedFields	= NULL,
	.NumFields		= LENGTH( CompHeader_Fields ),
	.Fields			= CompHeader_Fields,
	.NumLists		=  0,
	.Lists			= NULL,
	.OffsetNext		= -1
	};
/*============================================================================*/
static const parsefield_t	CompPin_FixedFields[]	=
	{
	/*	Flags			TagString		ParseFunction	Size	Offset									ParseStruct */
		{ FLAG_NAKED,	NULL,			ParseString,	0,		offsetof( pcad_comppin_t, pinnumber ),	NULL }
	};
/*----------------------------------------------------------------------------*/
static const parsefield_t	CompPin_Fields[]	=
	{
	/*	Flags			TagString		ParseFunction	Size	Offset									ParseStruct */
		{ FLAG_WRAPPED, "pinName",		ParseString,	0,		offsetof( pcad_comppin_t, pinname ),	NULL },
		{ FLAG_WRAPPED, "partNum",		ParseSigned,	0,		offsetof( pcad_comppin_t, partnum ),	NULL },
		{ FLAG_WRAPPED, "symPinNum",	ParseUnsigned,	0,		offsetof( pcad_comppin_t, sympinnum ),	NULL },
		{ FLAG_WRAPPED, "gateEq",		ParseSigned,	0,		offsetof( pcad_comppin_t, gateeq ),		NULL },
		{ FLAG_WRAPPED, "pinEq",		ParseSigned,	0,		offsetof( pcad_comppin_t, pineq ),		NULL },
		{ FLAG_WRAPPED, "pinType",		ParseEnum,		0,		offsetof( pcad_comppin_t, pintype ),	(const parsestruct_t*)&PinType }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	CompPin_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields = LENGTH( CompPin_FixedFields ),
	.FixedFields	= CompPin_FixedFields,
	.NumFields		= LENGTH( CompPin_Fields ),
	.Fields			= CompPin_Fields,
	.NumLists		=  0,
	.Lists			= NULL,
	.OffsetNext		= offsetof( pcad_comppin_t, next )
	};
/*============================================================================*/
static const parsefield_t	AttachedSymbol_Fields[]	=
	{
	/*	Flags			TagString		ParseFunction	Size	Offset											ParseStruct */
		{ FLAG_WRAPPED, "partNum",		ParseUnsigned,	0,		offsetof( pcad_attachedsymbol_t, partnum ),		NULL },
		{ FLAG_WRAPPED, "altType",		ParseName,		0,		offsetof( pcad_attachedsymbol_t, alttype ),		NULL },
		{ FLAG_WRAPPED, "symbolName",	ParseString,	0,		offsetof( pcad_attachedsymbol_t, symbolname ),	NULL }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	AttachedSymbol_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields = LENGTH( AttachedSymbol_Fields ),
	.FixedFields	= AttachedSymbol_Fields,
	.NumFields		=  0,
	.Fields			= NULL,
	.NumLists		=  0,
	.Lists			= NULL,
	.OffsetNext		= offsetof( pcad_attachedsymbol_t, next )
	};
/*============================================================================*/
static const parsefield_t	PadPinMap_Fields[]	=
	{
	/*	Flags			TagString		ParseFunction	Size	Offset										ParseStruct */
		{ FLAG_WRAPPED, "padNum",		ParseUnsigned,	0,		offsetof( pcad_padpinmap_t, padnum ),		NULL },
		{ FLAG_WRAPPED, "compPinRef",	ParseString,	0,		offsetof( pcad_padpinmap_t, comppinref ),	NULL }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	PadPinMap_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields =  0,
	.FixedFields	= NULL,
	.NumFields		= LENGTH( PadPinMap_Fields ),
	.Fields			= PadPinMap_Fields,
	.NumLists		=  0,
	.Lists			= NULL,
	.OffsetNext		= offsetof( pcad_padpinmap_t, next )
	};
/*============================================================================*/
static const listhead_t	AttachedPattern_Lists[]		=
	{
		{ .OffsetHead = offsetof( pcad_attachedpattern_t, firstpadpinmap ),	.OffsetLink = offsetof( pcad_attachedpattern_t, viopadpinmaps ) }
	};
/*----------------------------------------------------------------------------*/
static const parsefield_t	AttachedPattern_Fields[]	=
	{
	/*	Flags						TagString		ParseFunction		Size						Offset												ParseStruct */
		{ FLAG_WRAPPED, 			"patternNum",	ParseUnsigned,		0,							offsetof( pcad_attachedpattern_t, patternnum ),		NULL },
		{ FLAG_WRAPPED, 			"patternName",	ParseString,		0,							offsetof( pcad_attachedpattern_t, patternname ),	NULL },
		{ FLAG_WRAPPED, 			"numPads",		ParseUnsigned,		0,							offsetof( pcad_attachedpattern_t, numpads ),		NULL },
		{ FLAG_WRAPPED | FLAG_LIST, "padPinMap",	Parse_PadPinMap,	sizeof( pcad_padpinmap_t ),	offsetof( pcad_attachedpattern_t, viopadpinmaps ),	&PadPinMap_ParseStruct }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	AttachedPattern_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields = LENGTH( AttachedPattern_Fields ),
	.FixedFields	= AttachedPattern_Fields,
	.NumFields		=  0,
	.Fields			= NULL,
	.NumLists		= LENGTH( AttachedPattern_Lists ),
	.Lists			= AttachedPattern_Lists,
	.OffsetNext		= -1
	};
/*============================================================================*/
static const listhead_t	CompDef_Lists[]		=
	{
		{ .OffsetHead = offsetof( pcad_compdef_t, firstcomppin ),			.OffsetLink = offsetof( pcad_compdef_t, viocomppins ) },
		{ .OffsetHead = offsetof( pcad_compdef_t, firstattachedsymbol ),	.OffsetLink = offsetof( pcad_compdef_t, vioattachedsymbols ) }
	};
/*----------------------------------------------------------------------------*/
static const parsefield_t	CompDef_FixedFields[]	=
	{
	/*	Flags			TagString			ParseFunction	Size								Offset											ParseStruct */
		{ FLAG_NAKED,	NULL,				ParseString,	0,									offsetof( pcad_compdef_t, name ),				NULL },
	};
/*----------------------------------------------------------------------------*/
static const parsefield_t	CompDef_Fields[]	=
	{
	/*	Flags						TagString			ParseFunction	Size								Offset											ParseStruct */
		{ FLAG_WRAPPED,				"originalName",		ParseString,	0,									offsetof( pcad_compdef_t, originalname ),		NULL },
		{ FLAG_WRAPPED,				"compHeader",		ParseGeneric,	0,									offsetof( pcad_compdef_t, compheader ),			&CompHeader_ParseStruct },
		{ FLAG_WRAPPED | FLAG_LIST,	"compPin",			ParseGeneric,	sizeof( pcad_comppin_t ),			offsetof( pcad_compdef_t, viocomppins ),		&CompPin_ParseStruct },
		{ FLAG_WRAPPED | FLAG_LIST, "attachedSymbol",	ParseGeneric,	sizeof( pcad_attachedsymbol_t ),	offsetof( pcad_compdef_t, vioattachedsymbols ), &AttachedSymbol_ParseStruct },
		{ FLAG_WRAPPED,				"attachedPattern",	ParseGeneric,	0,									offsetof( pcad_compdef_t, attachedpattern ),	&AttachedPattern_ParseStruct }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	CompDef_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields = LENGTH( CompDef_FixedFields ),
	.FixedFields	= CompDef_FixedFields,
	.NumFields		= LENGTH( CompDef_Fields ),
	.Fields			= CompDef_Fields,
	.NumLists		= LENGTH( CompDef_Lists ),
	.Lists			= CompDef_Lists,
	.OffsetNext		= offsetof( pcad_compdef_t, next )
	};
/*============================================================================*/
static const listhead_t	Library_Lists[]		=
	{
		{ .OffsetHead = offsetof( pcad_library_t, firsttextstyledef ),	.OffsetLink = offsetof( pcad_library_t, viotextstyledefs ) },
		{ .OffsetHead = offsetof( pcad_library_t, firstsymboldef ),		.OffsetLink = offsetof( pcad_library_t, viosymboldefs ) },
		{ .OffsetHead = offsetof( pcad_library_t, firstcompdef ),		.OffsetLink = offsetof( pcad_library_t, viocompdefs ) }
	};
/*----------------------------------------------------------------------------*/
static const parsefield_t	Library_FixedFields[]	=
	{
	/*	Flags			TagString	ParseFunction		Size	Offset								ParseStruct */
		{ FLAG_NAKED,	NULL,		ParseString,		0,		offsetof( pcad_library_t, name ),	NULL }
	};
/*----------------------------------------------------------------------------*/
static const parsefield_t	Library_Fields[]	=
	{
	/*	Flags						TagString			ParseFunction		Size							Offset											ParseStruct */
		{ FLAG_WRAPPED | FLAG_LIST, "textStyleDef",		ParseGeneric,		sizeof( pcad_textstyledef_t ),	offsetof( pcad_library_t, viotextstyledefs ),	&TextStyleDef_ParseStruct },
		{ FLAG_WRAPPED | FLAG_LIST, "symbolDef",		ParseGeneric,		sizeof( pcad_symboldef_t ),		offsetof( pcad_library_t, viosymboldefs ),		&SymbolDef_ParseStruct },
		{ FLAG_WRAPPED | FLAG_LIST, "compDef",			ParseGeneric,		sizeof( pcad_compdef_t ),		offsetof( pcad_library_t, viocompdefs ),		&CompDef_ParseStruct }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	Library_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields = LENGTH( Library_FixedFields ),
	.FixedFields	= Library_FixedFields,
	.NumFields		= LENGTH( Library_Fields ),
	.Fields			= Library_Fields,
	.NumLists		= LENGTH( Library_Lists ),
	.Lists			= Library_Lists,
	.OffsetNext		= -1
	};
/*============================================================================*/
static const listhead_t	CompInst_Lists[]		=
	{
		{ .OffsetHead = offsetof( pcad_compinst_t, firstattr ),	.OffsetLink = offsetof( pcad_compinst_t, vioattrs ) }
	};
/*----------------------------------------------------------------------------*/
static const parsefield_t	CompInst_FixedFields[]	=
	{
	/*	Flags			TagString			ParseFunction	Size					Offset									ParseStruct */
		{ FLAG_NAKED,	NULL,				ParseString,	0,						offsetof( pcad_compinst_t, name ),		NULL }
	};
/*----------------------------------------------------------------------------*/
static const parsefield_t	CompInst_Fields[]	=
	{
	/*	Flags						TagString		ParseFunction	Size					Offset										ParseStruct */
		{ FLAG_WRAPPED,				"compRef",		ParseString,	0,						offsetof( pcad_compinst_t, compref ),		NULL },
		{ FLAG_WRAPPED,				"originalName", ParseString,	0,						offsetof( pcad_compinst_t, originalname ),	NULL },
		{ FLAG_WRAPPED,				"compValue",	ParseString,	0,						offsetof( pcad_compinst_t, compvalue ),		NULL },
		{ FLAG_WRAPPED,				"patternName",	ParseString,	0,						offsetof( pcad_compinst_t, patternname ),	NULL },
		{ FLAG_WRAPPED | FLAG_LIST, "attr",			ParseGeneric,	sizeof( pcad_attr_t ),	offsetof( pcad_compinst_t, vioattrs ),		&Attr_ParseStruct }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	CompInst_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields = LENGTH( CompInst_FixedFields ),
	.FixedFields	= CompInst_FixedFields,
	.NumFields		= LENGTH( CompInst_Fields ),
	.Fields			= CompInst_Fields,
	.NumLists		= LENGTH( CompInst_Lists ),
	.Lists			= CompInst_Lists,
	.OffsetNext		= offsetof( pcad_compinst_t, next )
	};
/*============================================================================*/
static const parsefield_t	Node_FixedFields[]	=
	{
	/*	Flags			TagString	ParseFunction	Size	Offset								ParseStruct */
		{ FLAG_NAKED,	NULL,		ParseString,	0,	offsetof( pcad_node_t, component ),	NULL },
		{ FLAG_NAKED,	NULL,		ParseString,	0,	offsetof( pcad_node_t, pin ),		NULL }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	Node_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields = LENGTH( Node_FixedFields ),
	.FixedFields	= Node_FixedFields,
	.NumFields		=  0,
	.Fields			= NULL,
	.NumLists		=  0,
	.Lists			= NULL,
	.OffsetNext		= offsetof( pcad_node_t, next )
	};
/*============================================================================*/
static const listhead_t	Net_Lists[]		=
	{
		{ .OffsetHead = offsetof( pcad_net_t, firstnode ),	.OffsetLink = offsetof( pcad_net_t, vionodes ) }
	};
/*----------------------------------------------------------------------------*/
static const parsefield_t	Net_FixedFields[]	=
	{
	/*	Flags			TagString	ParseFunction	Size					Offset								ParseStruct */
		{ FLAG_NAKED,	NULL,		ParseString,	0,						offsetof( pcad_net_t, name ),		NULL }
	};
/*----------------------------------------------------------------------------*/
static const parsefield_t	Net_Fields[]	=
	{
	/*	Flags						TagString	ParseFunction	Size					Offset								ParseStruct */
		{ FLAG_WRAPPED | FLAG_LIST, "node",		ParseGeneric,	sizeof( pcad_node_t ),	offsetof( pcad_net_t, vionodes ),	&Node_ParseStruct }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	Net_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields = LENGTH( Net_FixedFields ),
	.FixedFields	= Net_FixedFields,
	.NumFields		= LENGTH( Net_Fields ),
	.Fields			= Net_Fields,
	.NumLists		= LENGTH( Net_Lists ),
	.Lists			= Net_Lists,
	.OffsetNext		= offsetof( pcad_net_t, next )
	};
/*============================================================================*/
static const listhead_t	GlobalAttrs_Lists[]		=
	{
		{ .OffsetHead = offsetof( pcad_globalattrs_t, firstattr ),	.OffsetLink = offsetof( pcad_globalattrs_t, vioattrs ) }
	};
/*----------------------------------------------------------------------------*/
static const parsefield_t	GlobalAttrs_Fields[]	=
	{
	/*	Flags						TagString	ParseFunction	Size					Offset										ParseStruct */
		{ FLAG_WRAPPED | FLAG_LIST, "attr",		ParseGeneric,	sizeof( pcad_attr_t ),	offsetof( pcad_globalattrs_t, vioattrs ),	&Attr_ParseStruct }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	GlobalAttrs_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields =  0,
	.FixedFields	= NULL,
	.NumFields		= LENGTH( GlobalAttrs_Fields ),
	.Fields			= GlobalAttrs_Fields,
	.NumLists		= LENGTH( GlobalAttrs_Lists ),
	.Lists			= GlobalAttrs_Lists,
	.OffsetNext		= -1
	};
/*============================================================================*/
static const listhead_t	NetList_Lists[]		=
	{
		{ .OffsetHead = offsetof( pcad_netlist_t, firstcompinst ),	.OffsetLink = offsetof( pcad_netlist_t, viocompinsts ) },
		{ .OffsetHead = offsetof( pcad_netlist_t, firstnet ),		.OffsetLink = offsetof( pcad_netlist_t, vionets ) }
	};
/*----------------------------------------------------------------------------*/
static const parsefield_t	NetList_FixedFields[]	=
	{
	/*	Flags			TagString	ParseFunction	Size						Offset										ParseStruct */
		{ FLAG_NAKED,	NULL,		ParseString,	0,							offsetof( pcad_netlist_t, name ),			NULL },
	};
/*----------------------------------------------------------------------------*/
static const parsefield_t	NetList_Fields[]	=
	{
	/*	Flags						TagString		ParseFunction	Size						Offset										ParseStruct */
		{ FLAG_WRAPPED,				"globalAttrs",	ParseGeneric,	0,							offsetof( pcad_netlist_t, globalattrs ),	&GlobalAttrs_ParseStruct },
		{ FLAG_WRAPPED | FLAG_LIST,	"compInst",		ParseGeneric,	sizeof( pcad_compinst_t ),	offsetof( pcad_netlist_t, viocompinsts ),	&CompInst_ParseStruct },
		{ FLAG_WRAPPED | FLAG_LIST,	"net",			ParseGeneric,	sizeof( pcad_net_t ),		offsetof( pcad_netlist_t, vionets ),		&Net_ParseStruct }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	NetList_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields = LENGTH( NetList_FixedFields ),
	.FixedFields	= NetList_FixedFields,
	.NumFields		= LENGTH( NetList_Fields ),
	.Fields			= NetList_Fields,
	.NumLists		= LENGTH( NetList_Lists ),
	.Lists			= NetList_Lists,
	.OffsetNext		= -1
	};
/*============================================================================*/
/*============================================================================*/
/*============================================================================*/
/*============================================================================*/
static const parsefield_t	Port_FixedFields[]	=
	{
	/*	Flags			TagString			ParseFunction		Size	Offset									ParseStruct */
		{ FLAG_WRAPPED,	"pt",				ParseGeneric,		0,		offsetof( pcad_port_t, point ),			&Point_ParseStruct },
		{ FLAG_WRAPPED,	"portType",			ParseEnum,			0,		offsetof( pcad_port_t, porttype ),		(const parsestruct_t*)&PortTypes }
	};
/*----------------------------------------------------------------------------*/
static const parsefield_t	Port_Fields[]	=
	{
	/*	Flags			TagString			ParseFunction		Size	Offset									ParseStruct */
		{ FLAG_WRAPPED,	"portPinLength",	ParseEnum,			0,		offsetof( pcad_port_t, portpinlength ), (const parsestruct_t*)&PortPinLengths },
		{ FLAG_WRAPPED,	"netNameRef",		ParseString,		0,		offsetof( pcad_port_t, netnameref ),	NULL },
		{ FLAG_WRAPPED,	"rotation",			ParseReal,			0,		offsetof( pcad_port_t, rotation ),		NULL },
		{ FLAG_WRAPPED,	"isFlipped",		ParseBoolean,		0,		offsetof( pcad_port_t, isflipped ),		NULL }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	Port_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields = LENGTH( Port_FixedFields ),
	.FixedFields	= Port_FixedFields,
	.NumFields		= LENGTH( Port_Fields ),
	.Fields			= Port_Fields,
	.NumLists		=  0,
	.Lists			= NULL,
	.OffsetNext		= offsetof( pcad_port_t, next )
	};
/*===========================================================================*/
static const parsefield_t	LineInWire_FixedFields[]	=
	{
	/*	Flags			TagString		ParseFunction		Size	Offset									ParseStruct */
		{ FLAG_WRAPPED,	"pt",			ParseGeneric,		0,		offsetof( pcad_wire_t, pt1 ),			&Point_ParseStruct },
		{ FLAG_WRAPPED,	"endStyle",		ParseEnum,			0,		offsetof( pcad_wire_t, endstyle1 ),		(const parsestruct_t*)&EndStyles },
		{ FLAG_WRAPPED,	"pt",			ParseGeneric,		0,		offsetof( pcad_wire_t, pt2 ),			&Point_ParseStruct },
		{ FLAG_WRAPPED,	"endStyle",		ParseEnum,			0,		offsetof( pcad_wire_t, endstyle2 ),		(const parsestruct_t*)&EndStyles },
		{ FLAG_WRAPPED,	"width",		ParseDimmension,	0,		offsetof( pcad_wire_t, width ),			NULL },
		{ FLAG_WRAPPED,	"netNameRef",	ParseString,		0,		offsetof( pcad_wire_t, netnameref ),	NULL }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	LineInWire_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields = LENGTH( LineInWire_FixedFields ),
	.FixedFields	= LineInWire_FixedFields,
	.NumFields		=  0,
	.Fields			= NULL,
	.NumLists		=  0,
	.Lists			= NULL,
	.OffsetNext		= offsetof( pcad_wire_t, next )
	};
/*===========================================================================*/
static const parsefield_t	Wire_FixedFields[]	=
	{
	/*	Flags			TagString	ParseFunction		Size	Offset	ParseStruct */
		{ FLAG_WRAPPED,	"line",	Parse_LineInWire,		0,		0,		&LineInWire_ParseStruct }
	};
/*----------------------------------------------------------------------------*/
static const parsefield_t	Wire_Fields[]	=
	{
	/*	Flags			TagString	ParseFunction		Size	Offset								ParseStruct */
		{ FLAG_WRAPPED,	"dispName",	ParseBoolean,		0,		offsetof( pcad_wire_t, dispname ),	NULL },
		{ FLAG_WRAPPED,	"text",		ParseGeneric,		0,		offsetof( pcad_wire_t, text ),		&Text_ParseStruct }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	Wire_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields = LENGTH( Wire_FixedFields ),
	.FixedFields	= Wire_FixedFields,
	.NumFields		= LENGTH( Wire_Fields ),
	.Fields			= Wire_Fields,
	.NumLists		=  0,
	.Lists			= NULL,
	.OffsetNext		= offsetof( pcad_wire_t, next )
	};
/*===========================================================================*/
static const parsefield_t	Bus_FixedFields[]	=
	{
	/*	Flags			TagString	ParseFunction	Size					Offset								ParseStruct */
		{ FLAG_NAKED,	NULL,		ParseString,	0,						offsetof( pcad_bus_t, name ),		NULL },
		{ FLAG_WRAPPED,	"pt",		ParseGeneric,	0,						offsetof( pcad_bus_t, pt1 ),		&Point_ParseStruct },
		{ FLAG_WRAPPED,	"pt",		ParseGeneric,	0,						offsetof( pcad_bus_t, pt2 ),		&Point_ParseStruct }
	};
/*----------------------------------------------------------------------------*/
static const parsefield_t	Bus_Fields[]	=
	{
	/*	Flags			TagString	ParseFunction	Size					Offset								ParseStruct */
		{ FLAG_WRAPPED,	"dispName",	ParseBoolean,	0,						offsetof( pcad_bus_t, dispname ),	NULL },
		{ FLAG_WRAPPED,	"text",		ParseGeneric,	sizeof( pcad_text_t ),	offsetof( pcad_bus_t, text ),		&Text_ParseStruct }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	Bus_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields = LENGTH( Bus_FixedFields ),
	.FixedFields	= Bus_FixedFields,
	.NumFields		= LENGTH( Bus_Fields ),
	.Fields			= Bus_Fields,
	.NumLists		=  0,
	.Lists			= NULL,
	.OffsetNext		= offsetof( pcad_bus_t, next )
	};
/*===========================================================================*/
static const parsefield_t	BusEntry_FixedFields[]	=
	{
	/*	Flags			TagString		ParseFunction	Size	Offset										ParseStruct */
		{ FLAG_WRAPPED,	"busNameRef",	ParseString,	0,		offsetof( pcad_busentry_t, busnameref ),	NULL },
		{ FLAG_WRAPPED,	"pt",			ParseGeneric,	0,		offsetof( pcad_busentry_t, point ),			&Point_ParseStruct },
		{ FLAG_WRAPPED,	"orient",		ParseEnum,		0,		offsetof( pcad_busentry_t, orient ),		(const parsestruct_t*)&Orients }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	BusEntry_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields = LENGTH( BusEntry_FixedFields ),
	.FixedFields	= BusEntry_FixedFields,
	.NumFields		=  0,
	.Fields			= NULL,
	.NumLists		=  0,
	.Lists			= NULL,
	.OffsetNext		= offsetof( pcad_busentry_t, next )
	};
/*===========================================================================*/
static const parsefield_t	Junction_FixedFields[]	=
	{
	/*	Flags		TagString			ParseFunction	Size	Offset										ParseStruct */
		{ FLAG_WRAPPED,	"pt",			ParseGeneric,	0,		offsetof( pcad_junction_t, point ),			&Point_ParseStruct },
		{ FLAG_WRAPPED,	"netNameRef",	ParseString,	0,		offsetof( pcad_junction_t, netnameref ),	NULL }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	Junction_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields = LENGTH( Junction_FixedFields ),
	.FixedFields	= Junction_FixedFields,
	.NumFields		=  0,
	.Fields			= NULL,
	.NumLists		=  0,
	.Lists			= NULL,
	.OffsetNext		= offsetof( pcad_junction_t, next )
	};
/*===========================================================================*/
static const listhead_t	Symbol_Lists[]		=
	{
		{ .OffsetHead = offsetof( pcad_symbol_t, firstattr ),	.OffsetLink = offsetof( pcad_symbol_t, vioattrs ) }
	};
/*----------------------------------------------------------------------------*/
static const parsefield_t	Symbol_FixedFields[]	=
	{
	/*	Flags			TagString		ParseFunction		Size	Offset									ParseStruct */
		{ FLAG_WRAPPED,	"symbolRef",	ParseString,		0,		offsetof( pcad_symbol_t, symbolref ),	NULL },
		{ FLAG_WRAPPED,	"refDesRef",	ParseString,		0,		offsetof( pcad_symbol_t, refdesref ),	NULL },
		{ FLAG_WRAPPED,	"partNum",		ParseUnsigned,		0,		offsetof( pcad_symbol_t, partnum ),		NULL },
		{ FLAG_WRAPPED,	"pt",			ParseGeneric,		0,		offsetof( pcad_symbol_t, pt ),			&Point_ParseStruct }
	};
/*----------------------------------------------------------------------------*/
static const parsefield_t	Symbol_Fields[]	=
	{
	/*	Flags						TagString		ParseFunction		Size					Offset									ParseStruct */
		{ FLAG_WRAPPED,				"rotation",		ParseReal,			0,						offsetof( pcad_symbol_t, rotation ),	NULL },
		{ FLAG_WRAPPED,				"isFlipped",	ParseBoolean,		0,						offsetof( pcad_symbol_t, isflipped ),	NULL },
		{ FLAG_WRAPPED | FLAG_LIST,	"attr",			ParseGeneric,		sizeof( pcad_attr_t ),	offsetof( pcad_symbol_t, vioattrs ),	&Attr_ParseStruct }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	Symbol_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields = LENGTH( Symbol_FixedFields ),
	.FixedFields	= Symbol_FixedFields,
	.NumFields		= LENGTH( Symbol_Fields ),
	.Fields			= Symbol_Fields,
	.NumLists		= LENGTH( Symbol_Lists ),
	.Lists			= Symbol_Lists,
	.OffsetNext		= offsetof( pcad_symbol_t, next )
	};
/*===========================================================================*/
static const parsefield_t	RefPoint_FixedFields[]	=
	{
	/*	Flags			TagString		ParseFunction	Size	Offset								ParseStruct */
		{ FLAG_WRAPPED,	"pt",			ParseGeneric,	0,		offsetof( pcad_refpoint_t, point ),	&Point_ParseStruct }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	RefPoint_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields = LENGTH( RefPoint_FixedFields ),
	.FixedFields	= RefPoint_FixedFields,
	.NumFields		=  0,
	.Fields			= NULL,
	.NumLists		=  0,
	.Lists			= NULL,
	.OffsetNext		= offsetof( pcad_refpoint_t,	next )
	};
/*===========================================================================*/
/*===========================================================================*/
/*============================================================================*/
/*============================================================================*/
static const parsefield_t	Field_FixedFields[]	=
	{
	/*	Flags			TagString		ParseFunction	Size	Offset									ParseStruct */
		{ FLAG_NAKED,	NULL,			ParseString,	0,		offsetof( pcad_field_t, name ),			NULL },
		{ FLAG_WRAPPED,	"pt",			ParseGeneric,	0,		offsetof( pcad_field_t, point ),		&Point_ParseStruct }
	};
/*----------------------------------------------------------------------------*/
static const parsefield_t	Field_Fields[]	=
	{
	/*	Flags			TagString		ParseFunction	Size	Offset									ParseStruct */
		{ FLAG_WRAPPED, "isFlipped",	ParseBoolean,	0,		offsetof( pcad_field_t, isflipped ),	NULL },
		{ FLAG_WRAPPED,	"justify",		ParseEnum,		0,		offsetof( pcad_field_t, justify ),		(const parsestruct_t*)&Justify },
		{ FLAG_WRAPPED,	"textStyleRef", ParseString,	0,		offsetof( pcad_field_t, textstyleref ), NULL }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	Field_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields = LENGTH( Field_FixedFields ),
	.FixedFields	= Field_FixedFields,
	.NumFields		= LENGTH( Field_Fields ),
	.Fields			= Field_Fields,
	.NumLists		=  0,
	.Lists			= NULL,
	.OffsetNext		= offsetof( pcad_field_t, next )
	};
/*===========================================================================*/
static const parsefield_t	Offset_FixedFields[]	=
	{
	/*	Flags			TagString		ParseFunction		Size	Offset									ParseStruct */
		{ FLAG_NAKED,	NULL,			ParseDimmension,	0,		offsetof( pcad_offset_t, offsetx ),		NULL },
		{ FLAG_NAKED,	NULL,			ParseDimmension,	0,		offsetof( pcad_offset_t, offsety ),		NULL }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	Offset_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields = LENGTH( Offset_FixedFields ),
	.FixedFields	= Offset_FixedFields,
	.NumFields		=  0,
	.Fields			= NULL,
	.NumLists		=  0,
	.Lists			= NULL,
	.OffsetNext		= -1
	};
/*===========================================================================*/
static const parsefield_t	Border_FixedFields[]	=
	{
	/*	Flags			TagString		ParseFunction		Size	Offset									ParseStruct */
		{ FLAG_WRAPPED,	"isVisible",	ParseBoolean,		0,		offsetof( pcad_border_t, isvisible ),	NULL },
		{ FLAG_WRAPPED,	"height",		ParseDimmension,	0,		offsetof( pcad_border_t, height ),		NULL },
		{ FLAG_WRAPPED,	"width",		ParseDimmension,	0,		offsetof( pcad_border_t, width ),		NULL },
		{ FLAG_WRAPPED,	"offset",		ParseGeneric,		0,		offsetof( pcad_border_t, offset ),		&Offset_ParseStruct }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	Border_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields	= LENGTH( Border_FixedFields ),
	.FixedFields	= Border_FixedFields,
	.NumFields		=  0,
	.Fields			= NULL,
	.NumLists		=  0,
	.Lists			= NULL,
	.OffsetNext		= -1
	};
/*===========================================================================*/
static const parsefield_t	HVZones_Fields[]	=
	{
	/*	Flags			TagString		ParseFunction	Size	Offset										ParseStruct */
		{ FLAG_NAKED,	"count",		ParseUnsigned,	0,		offsetof( pcad_hvzones_t, count ),			NULL },
		{ FLAG_WRAPPED,	"numDirection", ParseEnum,		0,		offsetof( pcad_hvzones_t, numdirection ),	(const parsestruct_t*)&NumDirection },
		{ FLAG_WRAPPED,	"numType",		ParseEnum,		0,		offsetof( pcad_hvzones_t, numtype ),		(const parsestruct_t*)&NumType },
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	HVZones_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields = LENGTH( HVZones_Fields ),
	.FixedFields	= HVZones_Fields,
	.NumFields		=  0,
	.Fields			= NULL,
	.NumLists		=  0,
	.Lists			= NULL,
	.OffsetNext		= -1
	};
/*===========================================================================*/
static const parsefield_t	Zones_Fields[]	=
	{
	/*	Flags			TagString			ParseFunction	Size	Offset										ParseStruct */
		{ FLAG_WRAPPED,	"isVisible",		ParseBoolean,	0,		offsetof( pcad_zones_t, isvisible ),		NULL },
		{ FLAG_WRAPPED,	"textStyleRef",		ParseString,	0,		offsetof( pcad_zones_t, textstyleref ),		NULL },
		{ FLAG_WRAPPED,	"horizontalZones",	ParseGeneric,	0,		offsetof( pcad_zones_t, horizontalzones ),	&HVZones_ParseStruct },
		{ FLAG_WRAPPED,	"verticalZones",	ParseGeneric,	0,		offsetof( pcad_zones_t, verticalzones ),	&HVZones_ParseStruct }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	Zones_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields = LENGTH( Zones_Fields ),
	.FixedFields	= Zones_Fields,
	.NumFields		=  0,
	.Fields			= NULL,
	.NumLists		=  0,
	.Lists			= NULL,
	.OffsetNext		= -1
	};
/*===========================================================================*/
static const listhead_t	TitleSheet_Lists[]		=
	{
		{ .OffsetHead = offsetof( pcad_titlesheet_t, firstline ),	.OffsetLink = offsetof( pcad_titlesheet_t, violines ) },
		{ .OffsetHead = offsetof( pcad_titlesheet_t, firsttext ),	.OffsetLink = offsetof( pcad_titlesheet_t, viotexts ) }
	};
/*----------------------------------------------------------------------------*/
static const parsefield_t	TitleSheet_FixedFields[]	=
	{
	/*	Flags			TagString		ParseFunction		Size					Offset										ParseStruct */
		{ FLAG_NAKED,	NULL,			ParseString,		0,						offsetof( pcad_titlesheet_t, name ),		NULL },
		{ FLAG_NAKED,	NULL,			ParseReal,			0,						offsetof( pcad_titlesheet_t, scale ),		NULL },
		{ FLAG_WRAPPED,	"isVisible",	ParseBoolean,		0,						offsetof( pcad_titlesheet_t, isvisible ),	NULL },
		{ FLAG_WRAPPED,	"offset",		ParseGeneric,		0,						offsetof( pcad_titlesheet_t, offset ),		&Offset_ParseStruct },
		{ FLAG_WRAPPED,	"border",		ParseGeneric,		0,						offsetof( pcad_titlesheet_t, border ),		&Border_ParseStruct },
		{ FLAG_WRAPPED,	"zones",		ParseGeneric,		0,						offsetof( pcad_titlesheet_t, zones ),		&Zones_ParseStruct }
	};
/*----------------------------------------------------------------------------*/
static const parsefield_t	TitleSheet_Fields[]	=
	{
	/*	Flags						TagString		ParseFunction		Size					Offset										ParseStruct */
		{ FLAG_WRAPPED | FLAG_LIST,	"line",			ParseGeneric,		sizeof( pcad_line_t ),	offsetof( pcad_titlesheet_t, violines ),	&Line_ParseStruct },
		{ FLAG_WRAPPED | FLAG_LIST,	"text",			ParseGeneric,		sizeof( pcad_text_t ),	offsetof( pcad_titlesheet_t, viotexts ),	&Text_ParseStruct }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	TitleSheet_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields = LENGTH( TitleSheet_FixedFields ),
	.FixedFields	= TitleSheet_FixedFields,
	.NumFields		= LENGTH( TitleSheet_Fields ),
	.Fields			= TitleSheet_Fields,
	.NumLists		= LENGTH( TitleSheet_Lists ),
	.Lists			= TitleSheet_Lists,
	.OffsetNext		= -1
	};
/*===========================================================================*/
static const parsefield_t	PrintRegion_Fields[]	=
	{
	/*	Flags			TagString	ParseFunction	Size	Offset											ParseStruct */
		{ FLAG_WRAPPED,	"pt",		ParseGeneric,	0,		offsetof( pcad_printregion_t, topleft ),		&Point_ParseStruct },
		{ FLAG_WRAPPED,	"pt",		ParseGeneric,	0,		offsetof( pcad_printregion_t, bottomright ),	&Point_ParseStruct }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	PrintRegion_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields = LENGTH( PrintRegion_Fields ),
	.FixedFields	= PrintRegion_Fields,
	.NumFields		=  0,
	.Fields			= NULL,
	.NumLists		=  0,
	.Lists			= NULL,
	.OffsetNext		= -1
	};
/*===========================================================================*/
/*===========================================================================*/
/*============================================================================*/
/*============================================================================*/
static const listhead_t	Sheet_Lists[]		=
	{
		{ .OffsetHead = offsetof( pcad_sheet_t, firstjunction ),		.OffsetLink = offsetof( pcad_sheet_t, viojunctions ) },
		{ .OffsetHead = offsetof( pcad_sheet_t, firstjunction ),		.OffsetLink = offsetof( pcad_sheet_t, viojunctions ) },
		{ .OffsetHead = offsetof( pcad_sheet_t, firstrefpoint ),		.OffsetLink = offsetof( pcad_sheet_t, viorefpoints ) },
		{ .OffsetHead = offsetof( pcad_sheet_t, firstwire ),			.OffsetLink = offsetof( pcad_sheet_t, viowires ) },
		{ .OffsetHead = offsetof( pcad_sheet_t, firstbusentry ),		.OffsetLink = offsetof( pcad_sheet_t, viobusentries ) },
		{ .OffsetHead = offsetof( pcad_sheet_t, firstbus ),				.OffsetLink = offsetof( pcad_sheet_t, viobuses ) },
		{ .OffsetHead = offsetof( pcad_sheet_t, firstport ),			.OffsetLink = offsetof( pcad_sheet_t, vioports ) },
		{ .OffsetHead = offsetof( pcad_sheet_t, firstpin ),				.OffsetLink = offsetof( pcad_sheet_t, viopins ) },
		{ .OffsetHead = offsetof( pcad_sheet_t, firsttext ),			.OffsetLink = offsetof( pcad_sheet_t, viotexts ) },
		{ .OffsetHead = offsetof( pcad_sheet_t, firsttriplepointarc ),	.OffsetLink = offsetof( pcad_sheet_t, viotriplepointarcs ) },
		{ .OffsetHead = offsetof( pcad_sheet_t, firstattr ),			.OffsetLink = offsetof( pcad_sheet_t, vioattrs ) },
		{ .OffsetHead = offsetof( pcad_sheet_t, firstsymbol ),			.OffsetLink = offsetof( pcad_sheet_t, viosymbols ) },
		{ .OffsetHead = offsetof( pcad_sheet_t, firstpoly ),			.OffsetLink = offsetof( pcad_sheet_t, viopolys ) },
		{ .OffsetHead = offsetof( pcad_sheet_t, firstline ),			.OffsetLink = offsetof( pcad_sheet_t, violines ) },
		{ .OffsetHead = offsetof( pcad_sheet_t, firstieeesymbol ),		.OffsetLink = offsetof( pcad_sheet_t, vioieeesymbols ) },
		{ .OffsetHead = offsetof( pcad_sheet_t, firstfield ),			.OffsetLink = offsetof( pcad_sheet_t, viofields ) }
	};
/*----------------------------------------------------------------------------*/
static const parsefield_t	Sheet_FixedFields[]	=
	{
	/*	Flags			TagString			ParseFunction		Size								Offset											ParseStruct */
		{ FLAG_NAKED,	NULL,				ParseString,		0,									offsetof( pcad_sheet_t, name ),				NULL },
		{ FLAG_WRAPPED,	"sheetNum",			ParseUnsigned,		0,									offsetof( pcad_sheet_t, sheetnum ),			NULL },
	};
/*----------------------------------------------------------------------------*/
static const parsefield_t	Sheet_Fields[]	=
	{
	/*	Flags						TagString			ParseFunction		Size								Offset											ParseStruct */
		{ FLAG_WRAPPED,				"titleSheet",		ParseGeneric,		0,									offsetof( pcad_sheet_t, titlesheet ),			&TitleSheet_ParseStruct },
		{ FLAG_WRAPPED,				"fieldSetRef",		ParseString,		0,									offsetof( pcad_sheet_t, fieldsetref ),			NULL },
		{ FLAG_WRAPPED | FLAG_LIST,	"junction",			ParseGeneric,		sizeof( pcad_junction_t ),			offsetof( pcad_sheet_t, viojunctions ),			&Junction_ParseStruct },
		{ FLAG_WRAPPED | FLAG_LIST,	"refPoint",			ParseGeneric,		sizeof( pcad_refpoint_t ),			offsetof( pcad_sheet_t, viorefpoints ),			&RefPoint_ParseStruct },
		{ FLAG_WRAPPED | FLAG_LIST,	"wire",				ParseGeneric,		sizeof( pcad_wire_t ),				offsetof( pcad_sheet_t, viowires ),				&Wire_ParseStruct },
		{ FLAG_WRAPPED | FLAG_LIST,	"busEntry",			ParseGeneric,		sizeof( pcad_busentry_t ),			offsetof( pcad_sheet_t, viobusentries ),		&BusEntry_ParseStruct },
		{ FLAG_WRAPPED | FLAG_LIST,	"bus",				ParseGeneric,		sizeof( pcad_bus_t ),				offsetof( pcad_sheet_t, viobuses ),				&Bus_ParseStruct },
		{ FLAG_WRAPPED | FLAG_LIST,	"port",				ParseGeneric,		sizeof( pcad_port_t ),				offsetof( pcad_sheet_t, vioports ),				&Port_ParseStruct },
		{ FLAG_WRAPPED | FLAG_LIST,	"pin",				ParseGeneric,		sizeof( pcad_pin_t ),				offsetof( pcad_sheet_t, viopins ),				&Pin_ParseStruct },
		{ FLAG_WRAPPED | FLAG_LIST,	"text",				ParseGeneric,		sizeof( pcad_text_t ),				offsetof( pcad_sheet_t, viotexts ),				&Text_ParseStruct },
		{ FLAG_WRAPPED | FLAG_LIST,	"triplePointArc",	ParseGeneric,		sizeof( pcad_triplepointarc_t ),	offsetof( pcad_sheet_t, viotriplepointarcs ),		&TriplePointArc_ParseStruct },
		{ FLAG_WRAPPED | FLAG_LIST,	"attr",				ParseGeneric,		sizeof( pcad_attr_t ),				offsetof( pcad_sheet_t, vioattrs ),				&Attr_ParseStruct },
		{ FLAG_WRAPPED | FLAG_LIST,	"symbol",			ParseGeneric,		sizeof( pcad_symbol_t ),			offsetof( pcad_sheet_t, viosymbols ),			&Symbol_ParseStruct },
		{ FLAG_WRAPPED | FLAG_LIST,	"poly",				ParseGeneric,		sizeof( pcad_poly_t ),				offsetof( pcad_sheet_t, viopolys ),				&Poly_ParseStruct },
		{ FLAG_WRAPPED | FLAG_LIST,	"line",				ParseGeneric,		sizeof( pcad_line_t ),				offsetof( pcad_sheet_t, violines ),				&Line_ParseStruct },
		{ FLAG_WRAPPED | FLAG_LIST,	"ieeeSymbol",		ParseGeneric,		sizeof( pcad_ieeesymbol_t ),		offsetof( pcad_sheet_t, vioieeesymbols ),			&IEEESymbol_ParseStruct },
		{ FLAG_WRAPPED | FLAG_LIST,	"field",			ParseGeneric,		sizeof( pcad_field_t ),				offsetof( pcad_sheet_t, viofields ),				&Field_ParseStruct },
		{ FLAG_WRAPPED,				"drawBorder",		ParseBoolean,		0,									offsetof( pcad_sheet_t, drawborder ),			NULL },
		{ FLAG_WRAPPED,				"EntireDesign",		ParseBoolean,		0,									offsetof( pcad_sheet_t, entiredesign ),			NULL },
		{ FLAG_WRAPPED,				"isRotated",		ParseBoolean,		0,									offsetof( pcad_sheet_t, isrotated ),			NULL },
		{ FLAG_WRAPPED,				"pageSize",			ParseEnum,			0,									offsetof( pcad_sheet_t, pagesize ),				(const parsestruct_t*)&PageSize },
		{ FLAG_WRAPPED,				"scaleFactor",		ParseReal,			0,									offsetof( pcad_sheet_t, scalefactor ),			NULL },
		{ FLAG_WRAPPED,				"offset",			ParseGeneric,		0,									offsetof( pcad_sheet_t, offset),				&Offset_ParseStruct },
		{ FLAG_WRAPPED,				"PrintRegion",		ParseGeneric,		0,									offsetof( pcad_sheet_t, printregion),			&PrintRegion_ParseStruct },
		{ FLAG_WRAPPED,				"sheetOrderNum",	ParseUnsigned,		0,									offsetof( pcad_sheet_t, sheetordernum ),		NULL }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	Sheet_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields = LENGTH( Sheet_FixedFields ),
	.FixedFields	= Sheet_FixedFields,
	.NumFields		= LENGTH( Sheet_Fields ),
	.Fields			= Sheet_Fields,
	.NumLists		= LENGTH( Sheet_Lists ),
	.Lists			= Sheet_Lists,
	.OffsetNext		= offsetof( pcad_sheet_t, next )
	};
/*===========================================================================*/
static const parsefield_t	SheetRef_FixedFields[]	=
	{
	/*	Flags			TagString	ParseFunction		Size	Offset									ParseStruct */
		{ FLAG_NAKED,	NULL,		ParseUnsigned,		0,		offsetof( pcad_sheetref_t, sheetref ),	NULL },
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	SheetRef_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields = LENGTH( SheetRef_FixedFields ),
	.FixedFields	= SheetRef_FixedFields,
	.NumFields		=  0,
	.Fields			= NULL,
	.NumLists		=  0,
	.Lists			= NULL,
	.OffsetNext		= offsetof( pcad_sheetref_t, next )
	};
/*===========================================================================*/
static const listhead_t	SheetList_Lists[]		=
	{
		{ .OffsetHead = offsetof( pcad_sheetlist_t, firstsheetref ),		.OffsetLink = offsetof( pcad_sheetlist_t, viosheetrefs ) },
	};
/*----------------------------------------------------------------------------*/
static const parsefield_t	SheetList_Fields[]	=
	{
	/*	Flags						TagString		ParseFunction		Size						Offset										ParseStruct */
		{ FLAG_WRAPPED | FLAG_LIST,	"sheetRef",		ParseGeneric,		sizeof( pcad_sheetref_t ),	offsetof( pcad_sheetlist_t, viosheetrefs ),	&SheetRef_ParseStruct },
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	SheetList_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields =  0,
	.FixedFields	= NULL,
	.NumFields		= LENGTH( SheetList_Fields ),
	.Fields			= SheetList_Fields,
	.NumLists		= LENGTH( SheetList_Lists ),
	.Lists			= SheetList_Lists,
	.OffsetNext		= -1
	};
/*===========================================================================*/
static const parsefield_t	PrintSettings_FixedFields[]	=
	{
	/*	Flags			TagString		ParseFunction	Size	Offset											ParseStruct */
		{ FLAG_WRAPPED,	"sheetList",	ParseGeneric,	0,		offsetof( pcad_schematicprintst_t, sheetlist ),	&SheetList_ParseStruct },
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	PrintSettings_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields = LENGTH( PrintSettings_FixedFields ),
	.FixedFields	= PrintSettings_FixedFields,
	.NumFields		=  0,
	.Fields			= NULL,
	.NumLists		=  0,
	.Lists			= NULL,
	.OffsetNext		= -1
	};
/*===========================================================================*/
/*===========================================================================*/
/*===========================================================================*/
/*===========================================================================*/
static const parsefield_t	CurrentLayer_Fields[]	=
	{
	/*	Flags			TagString		ParseFunction	Size	Offset												ParseStruct */
		{ FLAG_WRAPPED,	"layerNumRef",	ParseUnsigned,	0,		offsetof( pcad_currentlayer_t, layernumref ),		NULL }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	CurrentLayer_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields = LENGTH( CurrentLayer_Fields ),
	.FixedFields	= CurrentLayer_Fields,
	.NumFields		=  0,
	.Fields			= NULL,
	.NumLists		=  0,
	.Lists			= NULL,
	.OffsetNext		= -1
	};
/*===========================================================================*/
static const parsefield_t	LayerState_Fields[]	=
	{
	/*	Flags			TagString		ParseFunction	Size	Offset												ParseStruct */
		{ FLAG_WRAPPED,	"currentLayer", ParseGeneric,	0,		offsetof( pcad_layerstate_t, currentlayer ),		&CurrentLayer_ParseStruct },
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	LayerState_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields = LENGTH( LayerState_Fields ),
	.FixedFields	= LayerState_Fields,
	.NumFields		=  0,
	.Fields			= NULL,
	.NumLists		=  0,
	.Lists			= NULL,
	.OffsetNext		= -1
	};
/*===========================================================================*/
static const parsefield_t	GridState_Fields[]	=
	{
	/*	Flags			TagString			ParseFunction	Size	Offset											ParseStruct */
		{ FLAG_WRAPPED,	"currentAbsGrid",	Parse_Grid,		0,		offsetof( pcad_gridstate_t, currentabsgrid ),	NULL },
		{ FLAG_WRAPPED,	"currentRelGrid",	Parse_Grid,		0,		offsetof( pcad_gridstate_t, currentrelgrid ),	NULL },
		{ FLAG_WRAPPED,	"isAbsoluteGrid",	ParseBoolean,	0,		offsetof( pcad_gridstate_t, isabsolutegrid ),	NULL },
		{ FLAG_WRAPPED,	"isDottedGrid",		ParseBoolean,	0,		offsetof( pcad_gridstate_t, isdottedgrid ),		NULL },
		{ FLAG_WRAPPED,	"isVisibleGrid",	ParseBoolean,	0,		offsetof( pcad_gridstate_t, isvisiblegrid ),	NULL },
		{ FLAG_WRAPPED,	"isPromptForRel",	ParseBoolean,	0,		offsetof( pcad_gridstate_t, ispromptforrel ),	NULL }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	GridState_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields = LENGTH( GridState_Fields ),
	.FixedFields	= GridState_Fields,
	.NumFields		=  0,
	.Fields			= NULL,
	.NumLists		=  0,
	.Lists			= NULL,
	.OffsetNext		= -1
	};
/*===========================================================================*/
static const parsefield_t	EcoState_Fields[]	=
	{
	/*	Flags			TagString			ParseFunction		Size	Offset											ParseStruct */
		{ FLAG_WRAPPED,	"ecoRecording",		ParseBoolean,		0,		offsetof( pcad_ecostate_t, ecorecording ),		NULL }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	EcoState_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields = LENGTH( EcoState_Fields ),
	.FixedFields	= EcoState_Fields,
	.NumFields		=  0,
	.Fields			= NULL,
	.NumLists		=  0,
	.Lists			= NULL,
	.OffsetNext		= -1
	};
/*===========================================================================*/
static const parsefield_t	ProgramState_Fields[]	=
	{
	/*	Flags			TagString			ParseFunction	Size	Offset												ParseStruct */
		{ FLAG_WRAPPED,	"layerState",		ParseGeneric,	0,		offsetof( pcad_programstate_t, layerstate ),		&LayerState_ParseStruct },
		{ FLAG_WRAPPED,	"gridState",		ParseGeneric,	0,		offsetof( pcad_programstate_t, gridstate ),			&GridState_ParseStruct },
		{ FLAG_WRAPPED,	"ecoState",			ParseGeneric,	0,		offsetof( pcad_programstate_t, ecostate ),			&EcoState_ParseStruct },
		{ FLAG_WRAPPED,	"currentTextStyle",	ParseString,	0,		offsetof( pcad_programstate_t, currenttextstyle ),	NULL },
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	ProgramState_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields =  0,
	.FixedFields	= NULL,
	.NumFields		= LENGTH( ProgramState_Fields ),
	.Fields			= ProgramState_Fields,
	.NumLists		=  0,
	.Lists			= NULL,
	.OffsetNext		= -1
	};
/*===========================================================================*/
static const parsefield_t	ReportFieldCondition_FixedFields[]	=
	{
	/*	Flags			TagString	ParseFunction	Size	Offset												ParseStruct */
		{ FLAG_NAKED,	NULL,		ParseString,	0,		offsetof( pcad_reportfieldcondition_t, condition ),	NULL },
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	ReportFieldCondition_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields = LENGTH( ReportFieldCondition_FixedFields ),
	.FixedFields	= ReportFieldCondition_FixedFields,
	.NumFields		=  0,
	.Fields			= NULL,
	.NumLists		=  0,
	.Lists			= NULL,
	.OffsetNext		= offsetof( pcad_reportfieldcondition_t, next )
	};
/*===========================================================================*/
static const listhead_t	ReportFieldConditions_Lists[]	=
	{
		{ .OffsetHead = offsetof( pcad_reportfieldconditions_t, firstreportfieldcondition ),	.OffsetLink = offsetof( pcad_reportfieldconditions_t, vioreportfieldconditions ) },
	};
/*----------------------------------------------------------------------------*/
static const parsefield_t	ReportFieldConditions_Fields[]	=
	{
	/*	Flags						TagString					ParseFunction	Size									Offset																ParseStruct */
		{ FLAG_WRAPPED | FLAG_LIST,	"reportFieldCondition",		ParseGeneric,	sizeof( pcad_reportfieldcondition_t ),	offsetof( pcad_reportfieldconditions_t, vioreportfieldconditions ),	&ReportFieldCondition_ParseStruct },
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	ReportFieldConditions_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields =  0,
	.FixedFields	= NULL,
	.NumFields		= LENGTH( ReportFieldConditions_Fields ),
	.Fields			= ReportFieldConditions_Fields,
	.NumLists		= LENGTH( ReportFieldConditions_Lists ),
	.Lists			= ReportFieldConditions_Lists,
	.OffsetNext		= -1
	};
/*===========================================================================*/
static const parsefield_t	ReportField_FixedFields[]	=
	{
	/*	Flags			TagString					ParseFunction	Size	Offset														ParseStruct */
		{ FLAG_WRAPPED,	"reportFieldName",			ParseString,	0,		offsetof( pcad_reportfield_t,	reportfieldname ),			NULL },
		{ FLAG_WRAPPED,	"reportFieldType",			ParseEnum,		0,		offsetof( pcad_reportfield_t,	reportfieldtype ),			(const parsestruct_t*)&ReportPropertyTypes },
		{ FLAG_WRAPPED,	"reportFieldSortOrder",		ParseUnsigned,	0,		offsetof( pcad_reportfield_t,	reportfieldsortorder ),		NULL },
		{ FLAG_WRAPPED,	"reportFieldSortType",		ParseEnum,		0,		offsetof( pcad_reportfield_t,	reportfieldsorttype ),		(const parsestruct_t*)&SortTypes },
		{ FLAG_WRAPPED,	"reportFieldShowFlag",		ParseBoolean,	0,		offsetof( pcad_reportfield_t,	reportfieldshowflag ),		NULL },
		{ FLAG_WRAPPED,	"reportFieldColumnWidth",	ParseUnsigned,	0,		offsetof( pcad_reportfield_t,	reportfieldcolumnwidth ),	NULL },
	};
/*----------------------------------------------------------------------------*/
static const parsefield_t	ReportField_Fields[]	=
	{
	/*	Flags			TagString					ParseFunction	Size	Offset														ParseStruct */
		{ FLAG_WRAPPED,	"reportFieldConditions",	ParseGeneric,	0,		offsetof( pcad_reportfield_t,	reportfieldconditions ),	&ReportFieldConditions_ParseStruct },
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	ReportField_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields = LENGTH( ReportField_FixedFields ),
	.FixedFields	= ReportField_FixedFields,
	.NumFields		= LENGTH( ReportField_Fields ),
	.Fields			= ReportField_Fields,
	.NumLists		=  0,
	.Lists			= NULL,
	.OffsetNext		= offsetof( pcad_reportfield_t, next )
	};
/*===========================================================================*/
static const listhead_t	ReportFields_Lists[]	=
	{
		{ .OffsetHead = offsetof( pcad_reportfields_t, firstreportfield ),	.OffsetLink = offsetof( pcad_reportfields_t, vioreportfields ) },
	};
/*----------------------------------------------------------------------------*/
static const parsefield_t	ReportFields_Fields[]	=
	{
	/*	Flags						TagString					ParseFunction	Size							Offset												ParseStruct */
		{ FLAG_WRAPPED | FLAG_LIST,	"reportField",				ParseGeneric,	sizeof( pcad_reportfield_t ),	offsetof( pcad_reportfields_t, vioreportfields ),	&ReportField_ParseStruct },
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	ReportFields_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields =  0,
	.FixedFields	= NULL,
	.NumFields		= LENGTH( ReportFields_Fields ),
	.Fields			= ReportFields_Fields,
	.NumLists		= LENGTH( ReportFields_Lists ),
	.Lists			= ReportFields_Lists,
	.OffsetNext		= offsetof( pcad_reportfields_t, next )
	};
/*===========================================================================*/
/*===========================================================================*/
static const listhead_t	ReportFieldsSections_Lists[]	=
	{
		{ .OffsetHead = offsetof( pcad_reportfieldssection_t, firstreportfields ),	.OffsetLink = offsetof( pcad_reportfieldssection_t, vioreportfieldss ) },
	};
/*----------------------------------------------------------------------------*/
static const parsefield_t	ReportFieldsSections_Fields[]	=
	{
	/*	Flags						TagString					ParseFunction	Size							Offset													ParseStruct */
		{ FLAG_WRAPPED | FLAG_LIST,	"reportFields",				ParseGeneric,	sizeof( pcad_reportfields_t ),	offsetof( pcad_reportfieldssection_t, vioreportfieldss ),	&ReportFields_ParseStruct },
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	ReportFieldsSections_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields =  0,
	.FixedFields	= NULL,
	.NumFields		= LENGTH( ReportFieldsSections_Fields ),
	.Fields			= ReportFieldsSections_Fields,
	.NumLists		= LENGTH( ReportFieldsSections_Lists ),
	.Lists			= ReportFieldsSections_Lists,
	.OffsetNext		= -1
	};
/*===========================================================================*/
static const parsefield_t	ReportDefinition_Fields[]	=
	{
	/*	Flags			TagString					ParseFunction	Size			Offset														ParseStruct */
		{ FLAG_WRAPPED,	"reportName",				ParseString,	0,				offsetof( pcad_reportdefinition_t, reportname ),			NULL },
		{ FLAG_WRAPPED,	"reportExtension",			ParseString,	0,				offsetof( pcad_reportdefinition_t, reportextension ),		NULL },
		{ FLAG_WRAPPED,	"reportShowFlag",			ParseBoolean,	0,				offsetof( pcad_reportdefinition_t, reportshowflag ),		NULL },
		{ FLAG_WRAPPED,	"reportType",				ParseEnum,		0,				offsetof( pcad_reportdefinition_t, reporttype ),			(const parsestruct_t*)&ReportTypes },
		{ FLAG_WRAPPED,	"reportLinesPerPage",		ParseUnsigned,	0,				offsetof( pcad_reportdefinition_t, reportlinesperpage ),	NULL },
		{ FLAG_WRAPPED,	"reportUserDefined",		ParseBoolean,	0,				offsetof( pcad_reportdefinition_t, reportuserdefined ),		NULL },
		{ FLAG_WRAPPED,	"reportColumnWidth",		ParseUnsigned,	0,				offsetof( pcad_reportdefinition_t, reportcolumnwidth ),		NULL },
		{ FLAG_WRAPPED,	"reportUseHeader",			ParseBoolean,	0,				offsetof( pcad_reportdefinition_t, reportuseheader ),		NULL },
		{ FLAG_WRAPPED,	"reportHeader",				ParseString,	0,				offsetof( pcad_reportdefinition_t, reportheader ),			NULL },
		{ FLAG_WRAPPED,	"reportUseFooter",			ParseBoolean,	0,				offsetof( pcad_reportdefinition_t, reportusefooter ),		NULL },
		{ FLAG_WRAPPED,	"reportFooter",				ParseString,	0,				offsetof( pcad_reportdefinition_t, reportfooter ),			NULL },
		{ FLAG_WRAPPED,	"reportUseDesignInfo",		ParseBoolean,	0,				offsetof( pcad_reportdefinition_t, reportusedesigninfo ),	NULL },
		{ FLAG_WRAPPED,	"reportShowDate",			ParseBoolean,	0,				offsetof( pcad_reportdefinition_t, reportshowdate ),		NULL },
		{ FLAG_WRAPPED,	"reportPaginateFlag",		ParseBoolean,	0,				offsetof( pcad_reportdefinition_t, reportpaginateflag ),	NULL },
		{ FLAG_WRAPPED,	"reportShowCDFPreface",		ParseBoolean,	0,				offsetof( pcad_reportdefinition_t, reportshowcdfpreface ),	NULL },
		{ FLAG_WRAPPED,	"reportShowColumnNames",	ParseBoolean,	0,				offsetof( pcad_reportdefinition_t, reportshowcolumnnames ),	NULL },
		{ FLAG_WRAPPED,	"reportFieldsSections",		ParseGeneric,	0,				offsetof( pcad_reportdefinition_t, reportfieldssections ),	&ReportFieldsSections_ParseStruct },
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	ReportDefinition_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields =  0,
	.FixedFields	= NULL,
	.NumFields		= LENGTH( ReportDefinition_Fields ),
	.Fields			= ReportDefinition_Fields,
	.NumLists		=  0,
	.Lists			= NULL,
	.OffsetNext		= offsetof( pcad_reportdefinition_t, next )
	};
/*===========================================================================*/
static const listhead_t	ReportDefinitions_Lists[]		=
	{
		{ .OffsetHead = offsetof( pcad_reportdefinitions_t, firstreportdefinition ),		.OffsetLink = offsetof( pcad_reportdefinitions_t, vioreportdefinitions ) }
	};
/*----------------------------------------------------------------------------*/
static const parsefield_t	ReportDefinitions_Fields[]	=
	{
	/*	Flags						TagString				ParseFunction	Size								Offset														ParseStruct */
		{ FLAG_WRAPPED | FLAG_LIST,	"reportDefinition",		ParseGeneric,	sizeof( pcad_reportdefinition_t ),	offsetof( pcad_reportdefinitions_t, vioreportdefinitions ),	&ReportDefinition_ParseStruct }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	ReportDefinitions_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields =  0,
	.FixedFields	= NULL,
	.NumFields		= LENGTH( ReportDefinitions_Fields ),
	.Fields			= ReportDefinitions_Fields,
	.NumLists		= LENGTH( ReportDefinitions_Lists ),
	.Lists			= ReportDefinitions_Lists,
	.OffsetNext		= -1
	};
/*===========================================================================*/
static const parsefield_t	ReportSettings_Fields[]	=
	{
	/*	Flags			TagString				ParseFunction	Size	Offset													ParseStruct */
		{ FLAG_WRAPPED,	"reportStyle",			ParseEnum,		0,		offsetof( pcad_reportsettings_t, reportstyle ),			(const parsestruct_t*)&ReportStyles },
		{ FLAG_WRAPPED,	"reportDestination",	ParseEnum,		0,		offsetof( pcad_reportsettings_t, reportdestination ),	(const parsestruct_t*)&ReportDestinations },
		{ FLAG_WRAPPED,	"pt",					ParseGeneric,	0,		offsetof( pcad_reportsettings_t, reportorigin ),		&Point_ParseStruct },
		{ FLAG_WRAPPED,	"reportDefinitions",	ParseGeneric,	0,		offsetof( pcad_reportsettings_t, reportdefinitions ),	&ReportDefinitions_ParseStruct }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	ReportSettings_ParseStruct =
	{
	.Flags			=  0,
	.NumFixedFields =  0,
	.FixedFields	= NULL,
	.NumFields		= LENGTH( ReportSettings_Fields ),
	.Fields			= ReportSettings_Fields,
	.NumLists		=  0,
	.Lists			= NULL,
	.OffsetNext		= -1
	};
/*===========================================================================*/
/*===========================================================================*/
/*===========================================================================*/
/*===========================================================================*/
static const parsefield_t	Grid_Fields[]	=
	{
	/*	Flags			TagString	ParseFunction	Size	Offset							ParseStruct */
		{ FLAG_NAKED,	NULL,		Parse_Grid,		0,		offsetof( pcad_grid_t, grid ),	NULL }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	Grid_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields = LENGTH( Grid_Fields ),
	.FixedFields	= Grid_Fields,
	.NumFields		=  0,
	.Fields			= NULL,
	.NumLists		=  0,
	.Lists			= NULL,
	.OffsetNext		= offsetof( pcad_grid_t, next )
	};
/*===========================================================================*/
static const listhead_t	GridDfns_Lists[]		=
	{
		{ .OffsetHead = offsetof( pcad_griddfns_t, firstgrid ),	.OffsetLink = offsetof( pcad_griddfns_t, viogrids ) }
	};
/*----------------------------------------------------------------------------*/
static const parsefield_t	GridDfns_Fields[]	=
	{
	/*	Flags						TagString	ParseFunction	Size					Offset									ParseStruct */
		{ FLAG_WRAPPED | FLAG_LIST, "grid",		ParseGeneric,	sizeof( pcad_grid_t ),	offsetof( pcad_griddfns_t, viogrids ),	&Grid_ParseStruct }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	GridDfns_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields =  0,
	.FixedFields	= NULL,
	.NumFields		= LENGTH( GridDfns_Fields ),
	.Fields			= GridDfns_Fields,
	.NumLists		= LENGTH( GridDfns_Lists ),
	.Lists			= GridDfns_Lists,
	.OffsetNext		= -1
	};
/*===========================================================================*/
static const parsefield_t	FieldDef_FixedFields[]	=
	{
	/*	Flags			TagString	ParseFunction	Size	Offset								ParseStruct
		-------------	---------	-------------	----	-----------------------------------	-----------*/
		{ FLAG_NAKED,	NULL,		ParseString,	0,		offsetof( pcad_fielddef_t, name ),	NULL },
		{ FLAG_NAKED,	NULL,		ParseString,	0,		offsetof( pcad_fielddef_t, value ),	NULL }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	FieldDef_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields = LENGTH( FieldDef_FixedFields ),
	.FixedFields	= FieldDef_FixedFields,
	.NumFields		=  0,
	.Fields			= NULL,
	.NumLists		=  0,
	.Lists			= NULL,
	.OffsetNext		= offsetof( pcad_fielddef_t, next )
	};
/*===========================================================================*/
static const parsefield_t	Note_FixedFields[]	=
	{
	/*	Flags			TagString	ParseFunction	Size	Offset								ParseStruct
		-------------	---------	-------------	----	-----------------------------------	-----------*/
		{ FLAG_NAKED,	NULL,		ParseUnsigned,	0,		offsetof( pcad_note_t, number ),	NULL },
		{ FLAG_NAKED,	NULL,		ParseString,	0,		offsetof( pcad_note_t, value ),		NULL }
	};
/*----------------------------------------------------------------------------*/
static const parsefield_t	Note_Fields[]	=
	{
	/*	Flags			TagString			ParseFunction	Size	Offset										ParseStruct
		------------	----------------	-------------	----	---------------------------------------		-----------*/
		{ FLAG_WRAPPED,	"noteannotation",	ParseEnum,		0,		offsetof( pcad_note_t, noteannotation ),	(const parsestruct_t*)&NoteAnnotations }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	Note_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields = LENGTH( Note_FixedFields ),
	.FixedFields	= Note_FixedFields,
	.NumFields		= LENGTH( Note_Fields ),
	.Fields			= Note_Fields,
	.NumLists		=  0,
	.Lists			= NULL,
	.OffsetNext		= offsetof( pcad_note_t, next )
	};
/*===========================================================================*/
static const parsefield_t	RevisionNote_FixedFields[]	=
	{
	/*	Flags			TagString	ParseFunction	Size	Offset										ParseStruct
		------------	---------	-------------	----	---------------------------------------		-----------*/
		{ FLAG_NAKED,	NULL,		ParseUnsigned,	0,		offsetof( pcad_revisionnote_t, number ),	NULL },
		{ FLAG_NAKED,	NULL,		ParseString,	0,		offsetof( pcad_revisionnote_t, value ),		NULL }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	RevisionNote_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields = LENGTH( RevisionNote_FixedFields ),
	.FixedFields	= RevisionNote_FixedFields,
	.NumFields		=  0,
	.Fields			= NULL,
	.NumLists		=  0,
	.Lists			= NULL,
	.OffsetNext		= offsetof( pcad_revisionnote_t, next )
	};
/*===========================================================================*/
static const listhead_t	FieldSet_Lists[]		=
	{
		{ .OffsetHead = offsetof( pcad_fieldset_t, firstfielddef ),		.OffsetLink = offsetof( pcad_fieldset_t, viofielddefs ) },
		{ .OffsetHead = offsetof( pcad_fieldset_t, firstnote ),			.OffsetLink = offsetof( pcad_fieldset_t, vionotes ) },
		{ .OffsetHead = offsetof( pcad_fieldset_t, firstrevisionnote ),	.OffsetLink = offsetof( pcad_fieldset_t, viorevisionnotes ) }
	};
/*----------------------------------------------------------------------------*/
static const parsefield_t	FieldSet_FixedFields[]	=
	{
	/*	Flags			TagString	ParseFunction	Size	Offset								ParseStruct */
		{ FLAG_NAKED,	NULL,		ParseString,	0,		offsetof( pcad_fieldset_t, name ),	NULL }
	};
/*----------------------------------------------------------------------------*/
static const parsefield_t	FieldSet_Fields[]	=
	{
	/*	Flags						TagString		ParseFunction	Size							Offset											ParseStruct */
		{ FLAG_WRAPPED | FLAG_LIST, "fieldDef",		ParseGeneric,	sizeof( pcad_fielddef_t ),		offsetof( pcad_fieldset_t, viofielddefs ),		&FieldDef_ParseStruct },
		{ FLAG_WRAPPED | FLAG_LIST, "note",			ParseGeneric,	sizeof( pcad_note_t ),			offsetof( pcad_fieldset_t, vionotes ),			&Note_ParseStruct },
		{ FLAG_WRAPPED | FLAG_LIST, "revisionNote",	ParseGeneric,	sizeof( pcad_revisionnote_t ),	offsetof( pcad_fieldset_t, viorevisionnotes ),	&RevisionNote_ParseStruct }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	FieldSet_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields = LENGTH( FieldSet_FixedFields ),
	.FixedFields	= FieldSet_FixedFields,
	.NumFields		= LENGTH( FieldSet_Fields ),
	.Fields			= FieldSet_Fields,
	.NumLists		= LENGTH( FieldSet_Lists ),
	.Lists			= FieldSet_Lists,
	.OffsetNext		= offsetof( pcad_fieldset_t, next )
	};
/*===========================================================================*/
static const listhead_t	DesignInfo_Lists[]		=
	{
		{ .OffsetHead = offsetof( pcad_designinfo_t, firstfieldset ),	.OffsetLink = offsetof( pcad_designinfo_t, viofieldsets ) }
	};
/*----------------------------------------------------------------------------*/
static const parsefield_t	DesignInfo_Fields[]	=
	{
	/*	Flags						TagString		ParseFunction	Size						Offset											ParseStruct */
		{ FLAG_WRAPPED | FLAG_LIST, "fieldSet",		ParseGeneric,	sizeof( pcad_fieldset_t ),	offsetof( pcad_designinfo_t, viofieldsets ),	&FieldSet_ParseStruct }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	DesignInfo_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields =  0,
	.FixedFields	= NULL,
	.NumFields		= LENGTH( DesignInfo_Fields ),
	.Fields			= DesignInfo_Fields,
	.NumLists		= LENGTH( DesignInfo_Lists ),
	.Lists			= DesignInfo_Lists,
	.OffsetNext		= -1
	};
/*===========================================================================*/
static const parsefield_t	SchDesignHeader_FixedFields[]	=
	{
	/*	Flags			TagString				ParseFunction		Size	Offset													ParseStruct */
		{ FLAG_WRAPPED, "workspaceSize",		ParseGeneric,		0,		offsetof( pcad_schdesignheader_t, workspacesize ),		&Extent_ParseStruct },
		{ FLAG_WRAPPED, "gridDfns",				ParseGeneric,		0,		offsetof( pcad_schdesignheader_t, griddfns ),			&GridDfns_ParseStruct },
		{ FLAG_WRAPPED, "designInfo",			ParseGeneric,		0,		offsetof( pcad_schdesignheader_t, designinfo ),			&DesignInfo_ParseStruct },
		{ FLAG_WRAPPED, "refPointSize",			ParseDimmension,	0,		offsetof( pcad_schdesignheader_t, refpointsize ),		NULL },
		{ FLAG_WRAPPED, "infoPointSize",		ParseDimmension,	0,		offsetof( pcad_schdesignheader_t, infopointsize ),		NULL },
		{ FLAG_WRAPPED, "junctionSize",			ParseDimmension,	0,		offsetof( pcad_schdesignheader_t, junctionsize ),		NULL },
		{ FLAG_WRAPPED, "refPointSizePrint",	ParseDimmension,	0,		offsetof( pcad_schdesignheader_t, refpointsizeprint ),	NULL },
		{ FLAG_WRAPPED, "infoPointSizePrint",	ParseDimmension,	0,		offsetof( pcad_schdesignheader_t, infopointsizeprint ),	NULL },
		{ FLAG_WRAPPED, "junctionSizePrint",	ParseDimmension,	0,		offsetof( pcad_schdesignheader_t, junctionsizeprint ),	NULL }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	SchDesignHeader_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields = LENGTH( SchDesignHeader_FixedFields ),
	.FixedFields	= SchDesignHeader_FixedFields,
	.NumFields		=  0,
	.Fields			= NULL,
	.NumLists		=  0,
	.Lists			= NULL,
	.OffsetNext		= -1
	};
/*===========================================================================*/
static const listhead_t	SchematicDesign_Lists[]		=
	{
		{ .OffsetHead = offsetof( pcad_schematicdesign_t, firstsheet ),		.OffsetLink = offsetof( pcad_schematicdesign_t, viosheets ) }
	};
/*----------------------------------------------------------------------------*/
static const parsefield_t	SchematicDesign_FixedFields[]	=
	{
	/*	Flags			TagString					ParseFunction			Size					Offset														ParseStruct */
		{ FLAG_NAKED,	NULL,						ParseString,			0,						offsetof( pcad_schematicdesign_t, name ),					NULL },
		{ FLAG_WRAPPED,	"schDesignHeader",			ParseGeneric,			0,						offsetof( pcad_schematicdesign_t, schdesignheader ),		&SchDesignHeader_ParseStruct }
	};
/*----------------------------------------------------------------------------*/
static const parsefield_t	SchematicDesign_Fields[]	=
	{
	/*	Flags						TagString					ParseFunction			Size					Offset														ParseStruct */
		{ FLAG_WRAPPED,				"titleSheet",				ParseGeneric,			0,						offsetof( pcad_schematicdesign_t, titlesheet ),				&TitleSheet_ParseStruct },
		{ FLAG_WRAPPED | FLAG_LIST,	"sheet",					ParseGeneric,			sizeof( pcad_sheet_t ), offsetof( pcad_schematicdesign_t, viosheets ),				&Sheet_ParseStruct },
		{ FLAG_WRAPPED,				"schematicPrintSettings",	ParseGeneric,			0,						offsetof( pcad_schematicdesign_t, schematicPrintSettings ),	&PrintSettings_ParseStruct },
		{ FLAG_WRAPPED,				"programState",				ParseGeneric,			0,						offsetof( pcad_schematicdesign_t, programstate ),			&ProgramState_ParseStruct },
		{ FLAG_WRAPPED,				"reportSettings",			ParseGeneric,			0,						offsetof( pcad_schematicdesign_t, reportsettings ),			&ReportSettings_ParseStruct }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	SchematicDesign_ParseStruct	=
	{
	.Flags			=  0,
	.NumFixedFields = LENGTH( SchematicDesign_FixedFields ),
	.FixedFields	= SchematicDesign_FixedFields,
	.NumFields		= LENGTH( SchematicDesign_Fields ),
	.Fields			= SchematicDesign_Fields,
	.NumLists		= LENGTH( SchematicDesign_Lists ),
	.Lists			= SchematicDesign_Lists,
	.OffsetNext		= -1
	};
/*===========================================================================*/
static const parsefield_t	SchematicFile_FixedFields[]	=
	{
	/*	Flags			TagString			ParseFunction	Size	Offset												ParseStruct */
		{ FLAG_NAKED,	"ACCEL_ASCII",		ParseName,		0,		-1,													NULL },
		{ FLAG_NAKED,	NULL,				ParseString,	0,		offsetof( pcad_schematicfile_t, name ),				NULL },
		{ FLAG_WRAPPED, "asciiHeader",		ParseGeneric,	0,		offsetof( pcad_schematicfile_t, asciiheader ),		&ASCIIHeader_ParseStruct },
		{ FLAG_WRAPPED, "library",			ParseGeneric,	0,		offsetof( pcad_schematicfile_t, library ),			&Library_ParseStruct },
		{ FLAG_WRAPPED, "netlist",			ParseGeneric,	0,		offsetof( pcad_schematicfile_t, netlist ),			&NetList_ParseStruct },
		{ FLAG_WRAPPED, "schematicDesign",	ParseGeneric,	0,		offsetof( pcad_schematicfile_t, schematicdesign ),	&SchematicDesign_ParseStruct }
	};
/*----------------------------------------------------------------------------*/
static const parsestruct_t	SchematicFile_ParseStruct	=
	{
	.Flags			= PARSE_FLAGS_OMMIT_CLOSE_PAR | PARSE_FLAGS_REQUIRE_EOF,
	.NumFixedFields = LENGTH( SchematicFile_FixedFields ),
	.FixedFields	= SchematicFile_FixedFields,
	.NumFields		=  0,
	.Fields			= NULL,
	.NumLists		=  0,
	.Lists			= NULL,
	.OffsetNext		= -1
	};
/*============================================================================*/
/*============================================================================*/
/*============================================================================*/
/*============================================================================*/
static pcad_schematicfile_t *PCADParseSchematicFile( cookie_t *Cookie )
	{
	pcad_schematicfile_t	*s;

	s	= Allocate( Cookie, sizeof( pcad_schematicfile_t ));

	ParseGeneric( Cookie, NULL, &SchematicFile_ParseStruct, s );

	return s;
	}
/*============================================================================*/
pcad_schematicfile_t *ParsePCAD( cookie_t *Cookie, const char *pNameIn, const char *pNameOut )
	{
	pcad_schematicfile_t	*s;

	s	= PCADParseSchematicFile( Cookie );

#if 0
	char	fn[256];
	int		g;

	strcpy( fn, pNameIn );
	strcat( fn, ".schbin" );
	if(( g = open( fn, O_CREAT | O_BINARY | O_WRONLY | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO )) == -1 )
		return -1;
	write( g, s, Cookie.HeapTop );
	close( g );
#endif

	PCADProcesSchematic( Cookie, s );

	return s;
	}
/*============================================================================*/
