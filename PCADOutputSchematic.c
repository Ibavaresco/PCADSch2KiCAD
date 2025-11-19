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
#include <math.h>
#include <stdarg.h>
#include "PCADOutputSchematic.h"
#include "PCADStructs.h"
#include "Parser.h"
/*=============================================================================*/
typedef struct
	{
	cookie_t			*Cookie;
	FILE				*File;
	} parameters_t;
/*=============================================================================*/
static int FormatReal( const parameters_t *Params, unsigned FracDigs, pcad_dimmension_t Origin, pcad_real_t Scale, pcad_real_t v, char *Buffer, size_t BufferSize )
	{
	int			Sign;
	uint32_t	Int;
	uint32_t	Frac;
	int			Res, Dig;

	if( Scale < 0 )
		v	= Origin - v;
	else
		v	= v - Origin;

	Sign	= v < 0;

	Int		= abs( v ) / 1000000ul;
	Frac	= abs( v ) % 1000000ul;

	for( Dig = 6; Dig > 1 && Dig > FracDigs && Frac % 10 == 0 /*&& Frac >= 100*/; Dig-- )
		Frac /= 10;

	Res		= snprintf( Buffer, BufferSize, "%s%u.%0*u", Sign ? "-" : "", Int, Dig, Frac );
	if( Res <= 0 || Res >= BufferSize )
		Error( Params->Cookie, -1, "Invalid number" );

	return Res;
	}
/*=============================================================================*/
static int __attribute__((format(printf, 3, 4))) OutputToFile( const parameters_t *Params, unsigned Level, const char *s, ... )
	{
	va_list	ap;
	int		CharsWritten;

	va_start( ap, s );

	fprintf( Params->File, "%.*s", Level, "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t" );
	CharsWritten	= vfprintf( Params->File, s, ap );

	va_end( ap );

	return CharsWritten + Params->Cookie->TabSize * Level;
	}
/*=============================================================================*/
#if 0
static void Rotate( pcad_dimmension_t *xRes, pcad_dimmension_t *yRes, const pcad_dimmension_t *x, const pcad_dimmension_t *y, pcad_dimmension_t xc, pcad_dimmension_t yc, float_t angle )
	{
	double	sinA, cosA;
	double	dX, dY;

	sincos( angle / 1.0e6, &sinA, &cosA );
	dX		= ( *( x != NULL ? x : xRes ) - xc ) / 1.0e6;
	dY		= ( *( y != NULL ? y : yRes ) - yc ) / 1.0e6;

	*xRes	= (int)(( dX * cosA - dY * sinA ) * 1.0e6 ) + xc;	//10000 ) * 100 + xc;
	*yRes	= (int)(( dY * cosA + dX * sinA ) * 1.0e6 ) + yc;	//10000 ) * 100 + yc;
	}
#endif
/*=============================================================================*/
/*=============================================================================*/
/*=============================================================================*/
/*=============================================================================*/
static int OutputFont( const parameters_t *Params, int Level, const pcad_font_t *Font )
	{
	char	Buffer[32];

	OutputToFile( Params, Level,	"(font\r\n" );

	OutputToFile( Params, Level + 1, "(fontType %s)\r\n", FontType.items[Font->fonttype%FontType.numitems] );
	OutputToFile( Params, Level + 1, "(fontFamily %s)\r\n", Font->fontfamily );
	OutputToFile( Params, Level + 1, "(fontFace \"%s\")\r\n", Font->fontface );

	FormatReal( Params, 0, 0, 1, Font->fontheight, Buffer, sizeof Buffer );
	OutputToFile( Params, Level + 1, "(fontHeight %s)\r\n", Buffer );

	FormatReal( Params, 0, 0, 1, Font->strokewidth, Buffer, sizeof Buffer );
	OutputToFile( Params, Level + 1, "(strokeWidth %s)\r\n", Buffer );

	if( Font->fonttype == PCAD_FONTTYPE_TRUETYPE )
		{
		OutputToFile( Params, Level + 1, "(fontWeight %u)\r\n", Font->fontweight );
		OutputToFile( Params, Level + 1, "(fontCharSet %u)\r\n", Font->fontcharset );
		OutputToFile( Params, Level + 1, "(fontOutPrecision %u)\r\n", Font->fontoutprecision );
		OutputToFile( Params, Level + 1, "(fontClipPrecision %u)\r\n", Font->fontclipprecision );
		OutputToFile( Params, Level + 1, "(fontQuality %u)\r\n", Font->fontquality );
		OutputToFile( Params, Level + 1, "(fontPitchAndFamily %u)\r\n", Font->fontpitchandfamily );
		}

	OutputToFile( Params, Level,	")\r\n" );

	return 0;
	}
/*=============================================================================*/
static int OutputTextStyleDef( const parameters_t *Params, int Level, const pcad_textstyledef_t *TextStyleDef )
	{
	int	i;

	OutputToFile( Params, Level,	"(textStyleDef \"%s\"\r\n", TextStyleDef->name );

	for( i = 0; i < TextStyleDef->numfonts; i++ )
		OutputFont( Params, Level + 1, TextStyleDef->viofonts[i] );

	OutputToFile( Params, Level + 1, "(textStyleAllowTType %s)\r\n", TextStyleDef->allowttype == PCAD_BOOLEAN_TRUE ? "True" : "False" );

	OutputToFile( Params, Level + 1, "(textStyleDisplayTType %s)\r\n", TextStyleDef->displayttype == PCAD_BOOLEAN_TRUE ? "True" : "False" );

	OutputToFile( Params, Level,	")\r\n" );

	return 0;
	}
/*=============================================================================*/
static int OutputText( const parameters_t *Params, int Level, const pcad_text_t *Text )
	{
	char	x[32], y[32];

	FormatReal( Params, 0, 0, 1, Text->point.x, x, sizeof x );
	FormatReal( Params, 0, 0, 1, Text->point.y, y, sizeof y );

	OutputToFile( Params, Level, "(text (pt %s %s) \"%s\" (textStyleRef \"%s\")", x, y, Text->value, Text->textstyleref );

	if( Text->rotation != 0 )
		{
		FormatReal( Params, 0, 0, 1, Text->rotation, x, sizeof x );
		OutputToFile( Params, 0, " (rotation %s)", x );
		}

	if( Text->isflipped == PCAD_BOOLEAN_TRUE )
		OutputToFile( Params, 0, " (isFlipped True)" );

	if( Text->justify != PCAD_JUSTIFY_LOWERLEFT )
		OutputToFile( Params, 0, " (justify %s)", Justify.items[Text->justify] );

	FormatReal( Params, 0, 0, 1, Text->extent.extentx, x, sizeof x );
	FormatReal( Params, 0, 0, 1, Text->extent.extenty, y, sizeof y );
	OutputToFile( Params, 0, " (extent %s %s))", x, y );

	return 0;
	}
/*=============================================================================*/
static int OutputAttr( int OutputPoint, const parameters_t *Params, int Level, const pcad_attr_t *Attr )
	{
	char	x[32];

	OutputToFile( Params, Level, "(attr \"%s\" \"%s\"", Attr->name, Attr->value );

	if( OutputPoint )
		{
		char	y[32];
		FormatReal( Params, 0, 0, 1, Attr->point.x, x, sizeof x );
		FormatReal( Params, 0, 0, 1, Attr->point.y, y, sizeof y );
		OutputToFile( Params, 0, " (pt %s %s)", x, y );
		}
	if( Attr->rotation != 0 )
		{
		FormatReal( Params, 0, 0, 1, Attr->rotation, x, sizeof x );
		OutputToFile( Params, 0, " (rotation %s)", x );
		}

	if( Attr->isflipped == PCAD_BOOLEAN_TRUE )
		OutputToFile( Params, 0, " (isFlipped True)" );

	if( Attr->isvisible == PCAD_BOOLEAN_TRUE )
		OutputToFile( Params, 0, " (isVisible True)" );

	if( Attr->justify != PCAD_JUSTIFY_LOWERLEFT )
		OutputToFile( Params, 0, " (justify %s)", Justify.items[Attr->justify] );

	OutputToFile( Params, 0, " (textStyleRef \"%s\")", Attr->textstyleref );

	if( Attr->constraintunits != PCAD_UNITS_NONE )
		OutputToFile( Params, 0, " (constraintUnits %s)", ConstraintUnits.items[Attr->constraintunits] );

	OutputToFile( Params, 0, ")\r\n" );

	return 0;
	}
/*=============================================================================*/
static int OutputPin( const parameters_t *Params, int Level, const pcad_pin_t *Pin )
	{
	char	x[32], y[32];

	FormatReal( Params, 0, 0, 1, Pin->point.x, x, sizeof x );
	FormatReal( Params, 0, 0, 1, Pin->point.y, y, sizeof y );
	OutputToFile( Params, Level,	"(pin (pinNum %u) (pt %s %s)", Pin->pinnum, x, y );

	if( Pin->rotation > 0 )
		{
		FormatReal( Params, 0, 0, 1, Pin->rotation, x, sizeof x );
		OutputToFile( Params, 0,	" (rotation %s)", x );
		}

	if( Pin->isflipped == PCAD_BOOLEAN_TRUE )
		OutputToFile( Params, 0,	" (isFlipped True)" );

	if( Pin->pinlength != 7620000000 && Pin->pinlength != 0 )
		{
		FormatReal( Params, 0, 0, 1, Pin->pinlength, x, sizeof x );
		OutputToFile( Params, 0,	" (pinLength %s)", x );
		}

	OutputToFile( Params, 0, "\r\n" );

	if( Pin->outsidestyle != PCAD_OUTSIDESTYLE_NONE )
		OutputToFile( Params, Level + 1, "(outsideStyle %s)\r\n", OutsideStyle.items[Pin->outsidestyle] );

	if( Pin->outsideedgestyle != PCAD_OUTSIDEEDGESTYLE_NONE )
		OutputToFile( Params, Level + 1, "(outsideEdgeStyle %s)\r\n", OutsideEdgeStyle.items[Pin->outsideedgestyle] );

	if( Pin->insideedgestyle != PCAD_INSIDEEDGESTYLE_NONE )
		OutputToFile( Params, Level + 1, "(insideEdgeStyle %s)\r\n", InsideEdgeStyle.items[Pin->insideedgestyle] );

	if( Pin->insidestyle != PCAD_INSIDESTYLE_NONE )
		OutputToFile( Params, Level + 1, "(insideStyle %s)\r\n", InsideStyle.items[Pin->insidestyle] );

	if( Pin->displaypinname == PCAD_BOOLEAN_TRUE || Pin->displaypindes == PCAD_BOOLEAN_FALSE )
		OutputToFile( Params, Level + 1, "(pinDisplay%s%s)\r\n", Pin->displaypinname == PCAD_BOOLEAN_TRUE ? " (dispPinName True)" : "", Pin->displaypindes == PCAD_BOOLEAN_FALSE ? " (dispPinDes False)" : "" );

	OutputToFile( Params, Level + 1, "(pinDes " );
	OutputText( Params, 0, &Pin->pindes );
	OutputToFile( Params, 0, ")\r\n" );

	OutputToFile( Params, Level + 1, "(pinName " );
	OutputText( Params, 0, &Pin->pinname );
	OutputToFile( Params, 0, ")\r\n" );

	if( Pin->defaultpindes != NULL )
		OutputToFile( Params, Level + 1, "(defaultPinDes \"%s\")\r\n", Pin->defaultpindes );

	OutputToFile( Params, Level,	")\r\n" );

	return 0;
	}
/*=============================================================================*/
static int OutputLine( const parameters_t *Params, unsigned Level, pcad_line_t *Line )
	{
	char	x1[32], y1[32], x2[32], y2[32];

	FormatReal( Params, 0, 0, 1, Line->pt1.x, x1, sizeof x1 );
	FormatReal( Params, 0, 0, 1, Line->pt1.y, y1, sizeof y1 );
	FormatReal( Params, 0, 0, 1, Line->pt2.x, x2, sizeof x2 );
	FormatReal( Params, 0, 0, 1, Line->pt2.y, y2, sizeof y2 );

	OutputToFile( Params, Level,	"(line (pt %s %s) (pt %s %s)", x1, y1, x2, y2 );

	if( Line->width != 0 )
		{
		FormatReal( Params, 0, 0, 1, Line->width, x1, sizeof x1 );
		OutputToFile( Params, 0, " (width %s)", x1 );
		}

	if( Line->style != PCAD_LINESTYLE_SOLIDLINE )
		OutputToFile( Params, 0, " (style %s)", LineStyles.items[Line->style] );

	if( Line->netnameref != NULL )
		OutputToFile( Params, 0, " (netNameRef \"%s\")", Line->netnameref );

	OutputToFile( Params, 0, ")\r\n" );

	return 0;
	}
/*=============================================================================*/
static int OutputArc( const parameters_t *Params, unsigned Level, const pcad_triplepointarc_t *Arc )
	{
	char	x[32], y[32];

	FormatReal( Params, 0, 0, 1, Arc->point1.x, x, sizeof x );
	FormatReal( Params, 0, 0, 1, Arc->point1.y, y, sizeof y );
	OutputToFile( Params, Level, "(triplePointArc (pt %s %s)", x, y );

	FormatReal( Params, 0, 0, 1, Arc->point2.x, x, sizeof x );
	FormatReal( Params, 0, 0, 1, Arc->point2.y, y, sizeof y );
	OutputToFile( Params, 0,	" (pt %s %s)", x, y );

	FormatReal( Params, 0, 0, 1, Arc->point3.x, x, sizeof x );
	FormatReal( Params, 0, 0, 1, Arc->point3.y, y, sizeof y );
	OutputToFile( Params, 0,	" (pt %s %s)", x, y );

	if( Arc->width != 0 )
		{
		FormatReal( Params, 0, 0, 1, Arc->width, x, sizeof x );
		OutputToFile( Params, 0, " (width %s)", x );
		}

	OutputToFile( Params, 0, ")\r\n" );

	return 0;
	}
/*=============================================================================*/
static int OutputIEEESymbol( const parameters_t *Params, unsigned Level, pcad_ieeesymbol_t *IEEESymbol )
	{
	char	x[32], y[32], Height[32];

	FormatReal( Params, 0, 0, 1, IEEESymbol->point.x, x, sizeof x );
	FormatReal( Params, 0, 0, 1, IEEESymbol->point.y, y, sizeof y );
	FormatReal( Params, 0, 0, 1, IEEESymbol->height, Height, sizeof Height );
	OutputToFile( Params, Level, "(ieeeSymbol %s (pt %s %s) (height %s)%s)\r\n", IEEESymbols.items[IEEESymbol->type], x, y, Height, IEEESymbol->isflipped == PCAD_BOOLEAN_TRUE ? "(isFlipped True)" : "" );

	return 0;
	}
/*=============================================================================*/
static int OutputPoly( const parameters_t *Params, unsigned Level, const pcad_poly_t *Poly )
	{
	char	x[32], y[32];
	int		i;

	OutputToFile( Params, Level, "(poly\r\n" );
	OutputToFile( Params, Level, "\t" );

	for( i = 0; i < Poly->numpoints; i++ )
		{
		FormatReal( Params, 0, 0, 1, Poly->viopoints[i]->x, x, sizeof x );
		FormatReal( Params, 0, 0, 1, Poly->viopoints[i]->y, y, sizeof y );
		OutputToFile( Params, 0, " (pt %s %s)", x, y );
		}

	OutputToFile( Params, Level + 1, "\r\n" );
	OutputToFile( Params, Level, ")\r\n" );

	return 0;
	}
/*=============================================================================*/
static int OutputSymbolDef( const parameters_t *Params, int Level, const pcad_symboldef_t *SymbolDef )
	{
	int		i;

	OutputToFile( Params, Level,	"(symbolDef \"%s\"\r\n", SymbolDef->name );

	OutputToFile( Params, Level + 1, "(originalName \"%s\")\r\n", SymbolDef->originalname );

	for( i = 0; i < SymbolDef->numpins; i++ )
		OutputPin( Params, Level + 1, SymbolDef->viopins[i] );

	for( i = 0; i < SymbolDef->numattrs; i++ )
		OutputAttr( 1, Params, Level + 1, SymbolDef->vioattrs[i] );

	for( i = 0; i < SymbolDef->numtexts; i++ )
		{
		OutputText( Params, Level + 1, SymbolDef->viotexts[i] );
		OutputToFile( Params, 0, "\r\n" );
		}

	for( i = 0; i < SymbolDef->numieeesymbols; i++ )
		OutputIEEESymbol( Params, Level + 1, SymbolDef->vioieeesymbols[i] );

	for( i = 0; i < SymbolDef->numlines; i++ )
		OutputLine( Params, Level + 1, SymbolDef->violines[i] );

	for( i = 0; i < SymbolDef->numtriplepointarcs; i++ )
		OutputArc( Params, Level + 1, SymbolDef->viotriplepointarcs[i] );

	for( i = 0; i < SymbolDef->numpolys; i++ )
		OutputPoly( Params, Level + 1, SymbolDef->viopolys[i] );

	OutputToFile( Params, Level,	")\r\n" );

	return 0;
	}
/*=============================================================================*/
static int OutputCompPin( const parameters_t *Params, int Level, const pcad_comppin_t *CompPin )
	{
	OutputToFile( Params, Level, "(compPin \"%s\"", CompPin->pinnumber );

	if( CompPin->pinname != NULL )
		OutputToFile( Params, 0, " (pinName \"%s\")", CompPin->pinname );

	OutputToFile( Params, 0, " (partNum %d) (symPinNum %u) (gateEq %d) (pinEq %d)",
		CompPin->partnum, CompPin->sympinnum, CompPin->gateeq, CompPin->pineq );

	if( CompPin->pintype != PCAD_PINTYPE_NONE )
		OutputToFile( Params, 0, " (pinType %s)", PinType.items[CompPin->pintype] );

	OutputToFile( Params, 0, ")\r\n" );

	return 0;
	}
/*=============================================================================*/
static int OutputAttachedSymbol( const parameters_t *Params, int Level, const pcad_attachedsymbol_t *AttachedSymbol )
	{
	OutputToFile( Params, Level, "(attachedSymbol (partNum %u) (altType %s) (symbolName \"%s\"))\r\n",
		AttachedSymbol->partnum, AltTypes.items[AttachedSymbol->alttype], AttachedSymbol->symbolname );

	return 0;
	}
/*=============================================================================*/
static int OutputPadPinMap( const parameters_t *Params, int Level, const pcad_padpinmap_t *PadPinMap )
	{
	OutputToFile( Params, Level, "(padNum %u) (compPinRef \"%s\")\r\n", PadPinMap->padnum, PadPinMap->comppinref );

	return 0;
	}
/*=============================================================================*/
static int OutputAttachedPattern( const parameters_t *Params, int Level, const pcad_attachedpattern_t *AttachedPattern )
	{
	int i;

	OutputToFile( Params, Level,	"(attachedPattern (patternNum %u) (patternName \"%s\")\r\n", AttachedPattern->patternnum, AttachedPattern->patternname );
	OutputToFile( Params, Level + 1,	"(numPads %u)\r\n", AttachedPattern->numpads );

	OutputToFile( Params, Level + 1,	"(padPinMap\r\n" );
	for( i = 0; i < AttachedPattern->numpadpinmaps; i++ )
		OutputPadPinMap( Params, Level + 2, AttachedPattern->viopadpinmaps[i] );
	OutputToFile( Params, Level + 1,	")\r\n" );

	OutputToFile( Params, Level,	")\r\n" );

	return 0;
	}
/*=============================================================================*/
static int OutputCompDef( parameters_t *Params, int Level, const pcad_compdef_t *CompDef )
	{
	int		i;

	OutputToFile( Params, Level,	"(compDef \"%s\"\r\n", CompDef->name );
	OutputToFile( Params, Level + 1,	"(originalName \"%s\")\r\n", CompDef->originalname );

	OutputToFile( Params, Level + 1,	"(compHeader\r\n" );
	OutputToFile( Params, Level + 2,		"(sourceLibrary \"%s\")\r\n", CompDef->compheader.sourcelibrary );

	if( CompDef->compheader.comptype != PCAD_COMPTYPE_NORMAL )
		OutputToFile( Params, Level + 2,		"(compType %s)\r\n", CompType.items[CompDef->compheader.comptype] );

	OutputToFile( Params, Level + 2,		"(numPins %u)\r\n", CompDef->compheader.numpins );
	OutputToFile( Params, Level + 2,		"(numParts %u)\r\n", CompDef->compheader.numparts );

	if( CompDef->compheader.composition == PCAD_COMPOSITION_HETEROGENEOUS )
		OutputToFile( Params, Level + 2,	"(composition Heterogeneous)\r\n" );

	OutputToFile( Params, Level + 2,		"(alts (ieeeAlt %s) (deMorganAlt %s))\r\n", CompDef->compheader.alts.ieeealt == PCAD_BOOLEAN_TRUE ? "True" : "False", CompDef->compheader.alts.demorganalt == PCAD_BOOLEAN_TRUE ? "True" : "False" );
	OutputToFile( Params, Level + 2,		"(refDesPrefix \"%s\")\r\n", CompDef->compheader.refdesprefix );
	OutputToFile( Params, Level + 1,	")\r\n" );

	for( i = 0; i < CompDef->numcomppins; i++ )
		OutputCompPin( Params, Level + 1, CompDef->viocomppins[i] );

	for( i = 0; i < CompDef->numattachedsymbols; i++ )
		OutputAttachedSymbol( Params, Level + 1, CompDef->vioattachedsymbols[i] );

	if( CompDef->attachedpattern.patternname != NULL )
		OutputAttachedPattern( Params, Level + 1, &CompDef->attachedpattern );

	OutputToFile( Params, Level,	")\r\n" );

	return 0;
	}
/*=============================================================================*/
static int OutputLibrary( parameters_t *Params, int Level, const pcad_library_t *Library )
	{
	int	i;

	OutputToFile( Params, 0,		"\r\n" );
	OutputToFile( Params, Level,	"(library \"%s\"\r\n", Library->name );

	for( i = 0; i < Library->numtextstyledefs; i++ )
		OutputTextStyleDef( Params, Level + 1, Library->viotextstyledefs[i] );

	for( i = 0; i < Library->numsymboldefs; i++ )
		OutputSymbolDef( Params, Level + 1, Library->viosymboldefs[i] );

	for( i = 0; i < Library->numcompdefs; i++ )
		OutputCompDef( Params, Level + 1, Library->viocompdefs[i] );

	OutputToFile( Params, Level,	")\r\n" );

	return 0;
	}
/*=============================================================================*/
static int OutputASCIIHeader( const parameters_t *Params, int Level, const pcad_asciiheader_t *ASCIIHeader )
	{
	OutputToFile( Params, 0,		"\r\n" );
	OutputToFile( Params, Level,	"(asciiHeader\r\n" );
	OutputToFile( Params, Level + 1, "(asciiVersion %u %u)\r\n", ASCIIHeader->asciiversion.high, ASCIIHeader->asciiversion.low );
	OutputToFile( Params, Level + 1, "(timeStamp %u %u %u %u %u %u)\r\n", ASCIIHeader->timestamp.year, ASCIIHeader->timestamp.month, ASCIIHeader->timestamp.day, ASCIIHeader->timestamp.hour, ASCIIHeader->timestamp.minute, ASCIIHeader->timestamp.second );
	OutputToFile( Params, Level + 1, "(program \"%s\" \"%s\")\r\n", ASCIIHeader->program.name, ASCIIHeader->program.version );
	OutputToFile( Params, Level + 1, "(copyright \"%s\")\r\n", ASCIIHeader->copyright );
	OutputToFile( Params, Level + 1, "(fileAuthor \"%s\")\r\n", ASCIIHeader->fileauthor );
	OutputToFile( Params, Level + 1, "(headerString \"%s\")\r\n", ASCIIHeader->headerstring );
	OutputToFile( Params, Level + 1, "(fileUnits mm)\r\n" );
	OutputToFile( Params, Level + 1, "(guidString \"%s\")\r\n", ASCIIHeader->guidstring );
	OutputToFile( Params, Level,	")\r\n" );

	return 0;
	}
/*=============================================================================*/
static int OutputCompInst( const parameters_t *Params, int Level, const pcad_compinst_t *CompInst )
	{
	int	i;

	OutputToFile( Params, Level,	"(compInst \"%s\"\r\n", CompInst->name );

	OutputToFile( Params, Level + 1, "(compRef \"%s\")\r\n", CompInst->compref );
	OutputToFile( Params, Level + 1, "(originalName \"%s\")\r\n", CompInst->originalname );

	if( CompInst->compvalue != NULL )
		OutputToFile( Params, Level + 1, "(compValue \"%s\")\r\n", CompInst->compvalue );

	if( CompInst->patternname != NULL )
		OutputToFile( Params, Level + 1, "(patternName \"%s\")\r\n", CompInst->patternname );

	for( i = 0; i < CompInst->numattrs; i++ )
		OutputAttr( 0, Params, Level + 1, CompInst->vioattrs[i] );

	OutputToFile( Params, Level,	")\r\n" );

	return 0;
	}
/*=============================================================================*/
static int OutputNode( const parameters_t *Params, int Level, const pcad_node_t *Node )
	{
	OutputToFile( Params, Level,	"(node \"%s\" \"%s\")\r\n", Node->component, Node->pin );

	return 0;
	}
/*=============================================================================*/
static int OutputNet( const parameters_t *Params, int Level, const pcad_net_t *Net )
	{
	int	i;

	OutputToFile( Params, Level,	"(net \"%s\"\r\n", Net->name );

	for( i = 0; i < Net->numnodes; i++ )
		OutputNode( Params, Level + 1, Net->vionodes[i] );

	OutputToFile( Params, Level,	")\r\n" );

	return 0;
	}
/*=============================================================================*/
static int OutputNetList( parameters_t *Params, int Level, const pcad_netlist_t *NetList )
	{
	int	i;

	OutputToFile( Params, 0,		"\r\n" );
	OutputToFile( Params, Level,	"(netlist \"%s\"\r\n", NetList->name );

	if( NetList->globalattrs.numattr > 0 )
		{
		OutputToFile( Params, Level + 1, "(globalAttrs\r\n" );
		for( i = 0; i < NetList->globalattrs.numattr; i++ )
			OutputAttr( 0, Params, Level + 2, NetList->globalattrs.vioattrs[i] );
		OutputToFile( Params, Level + 1, ")\r\n" );
		}

	for( i = 0; i < NetList->numcompinsts; i++ )
		OutputCompInst( Params, Level + 1, NetList->viocompinsts[i] );

	for( i = 0; i < NetList->numnets; i++ )
		OutputNet( Params, Level + 1, NetList->vionets[i] );

	OutputToFile( Params, Level,	")\r\n" );

	return 0;
	}
/*=============================================================================*/
static int OutputFieldSet( const parameters_t *Params, int Level, const pcad_fieldset_t *FieldSet )
	{
	int		i;

	OutputToFile( Params, Level, "(fieldSet \"%s\"\r\n", FieldSet->name );

	for( i = 0; i < FieldSet->numfielddefs; i++ )
		OutputToFile( Params, Level + 1, "(fieldDef \"%s\" \"%s\")\r\n", FieldSet->viofielddefs[i]->name, FieldSet->viofielddefs[i]->value );

	for( i = 0; i < FieldSet->numnotes; i++ )
		{
		OutputToFile( Params, Level + 1, "(note %u \"%s\"", FieldSet->vionotes[i]->number, FieldSet->vionotes[i]->value );
		if( FieldSet->vionotes[i]->noteannotation != PCAD_NOTEANNOTATION_NONE )
			OutputToFile( Params, 0, " (noteannotation %s)", NoteAnnotations.items[FieldSet->vionotes[i]->noteannotation] );
		OutputToFile( Params, 0, ")\r\n" );
		}

	for( i = 0; i < FieldSet->numrevisionnotes; i++ )
		OutputToFile( Params, Level + 1, "(revisionNote %u \"%s\")\r\n", FieldSet->viorevisionnotes[i]->number, FieldSet->viorevisionnotes[i]->value );

	OutputToFile( Params, Level, ")\r\n" );

	return 0;
	}
/*=============================================================================*/
static int OutputSchDesignHeader( const parameters_t *Params, int Level, const pcad_schdesignheader_t *SchDesignHeader )
	{
	char	x[32], y[32];
	int		i;

	OutputToFile( Params, Level,	"(schDesignHeader\r\n" );

	FormatReal( Params, 0, 0, 1, SchDesignHeader->workspacesize.extentx, x, sizeof x );
	FormatReal( Params, 0, 0, 1, SchDesignHeader->workspacesize.extenty, y, sizeof y );
	OutputToFile( Params, Level + 1, "(workspaceSize %s %s)\r\n", x, y );

	OutputToFile( Params, Level + 1, "(gridDfns\r\n" );
	for( i = 0; i < SchDesignHeader->griddfns.numgrids; i++ )
		{
		FormatReal( Params, 3, 0, 1, SchDesignHeader->griddfns.viogrids[i]->grid, x, sizeof x );
		OutputToFile( Params, Level + 2, "(grid \"%s\")\r\n", x );
		}
	OutputToFile( Params, Level + 1, ")\r\n" );

	OutputToFile( Params, Level + 1, "(designInfo\r\n" );
	for( i = 0; i < SchDesignHeader->designinfo.numfieldsets; i++ )
		OutputFieldSet( Params, Level + 2, SchDesignHeader->designinfo.viofieldsets[i] );
	OutputToFile( Params, Level + 1, ")\r\n" );


	FormatReal( Params, 0, 0, 1, SchDesignHeader->refpointsize, x, sizeof x );
	OutputToFile( Params, Level + 1, "(refPointSize %s)\r\n", x );
	FormatReal( Params, 0, 0, 1, SchDesignHeader->infopointsize, x, sizeof x );
	OutputToFile( Params, Level + 1, "(infoPointSize %s)\r\n", x );
	FormatReal( Params, 0, 0, 1, SchDesignHeader->junctionsize, x, sizeof x );
	OutputToFile( Params, Level + 1, "(junctionSize %s)\r\n", x );
	FormatReal( Params, 0, 0, 1, SchDesignHeader->refpointsizeprint, x, sizeof x );
	OutputToFile( Params, Level + 1, "(refPointSizePrint %s)\r\n", x );
	FormatReal( Params, 0, 0, 1, SchDesignHeader->infopointsizeprint, x, sizeof x );
	OutputToFile( Params, Level + 1, "(infoPointSizePrint %s)\r\n", x );
	FormatReal( Params, 0, 0, 1, SchDesignHeader->junctionsizeprint, x, sizeof x );
	OutputToFile( Params, Level + 1, "(junctionSizePrint %s)\r\n", x );

	OutputToFile( Params, Level,	")\r\n" );

	return 0;
	}
/*=============================================================================*/
#if 0
static void EscapeString( char *Result, int ResultLength, const char *Original )
	{
	int		i, j;

	for( i = 0, j = 0; i < ResultLength - 1 && Original[i] != '\0'; i++ )
		{
		Result[j++]	= Original[i];
		if( Original[i] == '\\' )
			Result[j++]	= '\\';
		}
	Result[j]	= '\0';
	}
#endif
/*=============================================================================*/
static int OutputTitleSheet( const parameters_t *Params, int Level, const pcad_titlesheet_t *TitleSheet )
	{
//	char	Buffer[512];
	char	x[32], y[32];
	int		i;

//	EscapeString( Buffer, sizeof Buffer, TitleSheet->name );
	OutputToFile( Params, Level,	"(titleSheet\r\n" );
	OutputToFile( Params, Level + 1,"\"%s\"\r\n", TitleSheet->name );

	FormatReal( Params, 4, 0, 1, TitleSheet->scale, x, sizeof x );
	OutputToFile( Params, Level + 1,	"%s\r\n", x );
	OutputToFile( Params, Level + 1,	"(isVisible %s)\r\n", TitleSheet->isvisible == PCAD_BOOLEAN_TRUE ? "True" : "False" );
	FormatReal( Params, 0, 0, 1, TitleSheet->offset.offsetx, x, sizeof x );
	FormatReal( Params, 0, 0, 1, TitleSheet->offset.offsety, y, sizeof y );
	OutputToFile( Params, Level + 1,	"(offset %s %s)\r\n", x, y );

	OutputToFile( Params, Level + 1,	"(border\r\n" );
	OutputToFile( Params, Level + 2,		"(isVisible %s)\r\n", TitleSheet->border.isvisible == PCAD_BOOLEAN_TRUE ? "True" : "False" );
	FormatReal( Params, 0, 0, 1, TitleSheet->border.height, x, sizeof x );
	OutputToFile( Params, Level + 2,		"(height %s)\r\n", x );
	FormatReal( Params, 0, 0, 1, TitleSheet->border.width, x, sizeof x );
	OutputToFile( Params, Level + 2,		"(width %s)\r\n", x );
	FormatReal( Params, 0, 0, 1, TitleSheet->border.offset.offsetx, x, sizeof x );
	FormatReal( Params, 0, 0, 1, TitleSheet->border.offset.offsety, y, sizeof y );
	OutputToFile( Params, Level + 2,		"(offset %s %s)\r\n", x, y );
	OutputToFile( Params, Level + 1,	")\r\n" );

	OutputToFile( Params, Level + 1,	"(zones\r\n" );
	OutputToFile( Params, Level + 2,		"(isVisible %s)\r\n", TitleSheet->zones.isvisible == PCAD_BOOLEAN_TRUE ? "True" : "False" );
	OutputToFile( Params, Level + 2,		"(textStyleRef \"%s\")\r\n", TitleSheet->zones.textstyleref );
	OutputToFile( Params, Level + 2,		"(horizontalZones %u\r\n", TitleSheet->zones.horizontalzones.count );
	OutputToFile( Params, Level + 3,			"(numDirection %s)\r\n", NumDirection.items[TitleSheet->zones.horizontalzones.numdirection] );
	OutputToFile( Params, Level + 3,			"(numType %s)\r\n", NumType.items[TitleSheet->zones.horizontalzones.numtype] );
	OutputToFile( Params, Level + 2,		")\r\n" );
	OutputToFile( Params, Level + 2,		"(verticalZones %u\r\n", TitleSheet->zones.verticalzones.count );
	OutputToFile( Params, Level + 3,			"(numDirection %s)\r\n", NumDirection.items[TitleSheet->zones.verticalzones.numdirection] );
	OutputToFile( Params, Level + 3,			"(numType %s)\r\n", NumType.items[TitleSheet->zones.verticalzones.numtype] );
	OutputToFile( Params, Level + 2,		")\r\n" );
	OutputToFile( Params, Level + 1,	")\r\n" );

	for( i = 0; i < TitleSheet->numtexts; i++ )
		{
		OutputText( Params, Level + 1, TitleSheet->viotexts[i] );
		OutputToFile( Params, 0, "\r\n" );
		}

	for( i = 0; i < TitleSheet->numlines; i++ )
		OutputLine( Params, Level + 1, TitleSheet->violines[i] );

	OutputToFile( Params, Level,	")\r\n" );

	return 0;
	}
/*=============================================================================*/
static int OutputSymbol( const parameters_t *Params, int Level, const pcad_symbol_t *Symbol )
	{
	char	x[32], y[32];

	FormatReal( Params, 0, 0, 1, Symbol->pt.x, x, sizeof x );
	FormatReal( Params, 0, 0, 1, Symbol->pt.y, y, sizeof y );
	OutputToFile( Params, Level,	"(symbol (symbolRef \"%s\") (refDesRef \"%s\") (partNum %u) (pt %s %s)",
		Symbol->symbolref, Symbol->refdesref, Symbol->partnum, x, y );

	if( Symbol->rotation != 0 )
		{
		FormatReal( Params, 0, 0, 1, Symbol->rotation, x, sizeof x );
		OutputToFile( Params, 0, "(rotation %s)", x );
		}

	if( Symbol->isflipped == PCAD_BOOLEAN_TRUE )
		OutputToFile( Params, 0, " (isFlipped True)" );

	if( Symbol->numattrs > 0 )
		{
		int		i;

		OutputToFile( Params, 0, "\r\n" );
		for( i = 0; i < Symbol->numattrs; i++ )
			OutputAttr( 1, Params, Level + 1, Symbol->vioattrs[i] );
		if( Level > 0 )
			OutputToFile( Params, Level - 1, "\t" );
		}

	OutputToFile( Params, 0,	")\r\n" );

	return 0;
	}
/*=============================================================================*/
static int OutputWire( const parameters_t *Params, int Level, const pcad_wire_t *Wire )
	{
	char	x1[32], y1[32], x2[32], y2[32], Width[32];

	FormatReal( Params, 0, 0, 1, Wire->pt1.x, x1, sizeof x1 );
	FormatReal( Params, 0, 0, 1, Wire->pt1.y, y1, sizeof y1 );
	FormatReal( Params, 0, 0, 1, Wire->pt2.x, x2, sizeof x2 );
	FormatReal( Params, 0, 0, 1, Wire->pt2.y, y2, sizeof y2 );
	FormatReal( Params, 0, 0, 1, Wire->width, Width, sizeof Width );

	OutputToFile( Params, Level, "(wire (line (pt %s %s)", x1, y1 );
	if( Wire->endstyle1 != PCAD_ENDSTYLE_NONE )
		OutputToFile( Params, 0, " (endStyle %s)", EndStyles.items[Wire->endstyle1] );
	OutputToFile( Params, 0, " (pt %s %s)", x2, y2 );
	if( Wire->endstyle2 != PCAD_ENDSTYLE_NONE )
		OutputToFile( Params, 0, " (endStyle %s)", EndStyles.items[Wire->endstyle2] );

	OutputToFile( Params, 0, " (width %s) (netNameRef \"%s\"))", Width, Wire->netnameref );

	if( Wire->dispname )
		{
		OutputToFile( Params, 0, " (dispName True) " );
		OutputText( Params, 0, &Wire->text );
		}

	OutputToFile( Params, 0, ")\r\n" );

	return 0;
	}
/*=============================================================================*/
static int OutputBus( const parameters_t *Params, int Level, const pcad_bus_t *Bus )
	{
	char	x1[32], y1[32], x2[32], y2[32];

	FormatReal( Params, 0, 0, 1, Bus->pt1.x, x1, sizeof x1 );
	FormatReal( Params, 0, 0, 1, Bus->pt1.y, y1, sizeof y1 );
	FormatReal( Params, 0, 0, 1, Bus->pt2.x, x2, sizeof x2 );
	FormatReal( Params, 0, 0, 1, Bus->pt2.y, y2, sizeof y2 );

	OutputToFile( Params, Level, "(bus \"%s\" (pt %s %s) (pt %s %s)", Bus->name, x1, y1, x2, y2 );
	if( Bus->dispname == PCAD_BOOLEAN_TRUE )
		OutputToFile( Params, 0, " (dispName True)" );
	if( Bus->text != NULL )
		OutputText( Params, 0, Bus->text );
	OutputToFile( Params, 0, ")\r\n" );

	return 0;
	}
/*=============================================================================*/
static int OutputBusEntry( const parameters_t *Params, int Level, const pcad_busentry_t *BusEntry )
	{
	char	x[32], y[32];

	FormatReal( Params, 0, 0, 1, BusEntry->point.x, x, sizeof x );
	FormatReal( Params, 0, 0, 1, BusEntry->point.y, y, sizeof y );

	OutputToFile( Params, Level, "(busEntry (busNameRef \"%s\") (pt %s %s) (orient %s))\r\n", BusEntry->busnameref, x, y, Orients.items[BusEntry->orient] );

	return 0;
	}
/*=============================================================================*/
static int OutputJunction( const parameters_t *Params, int Level, const pcad_junction_t *Junction )
	{
	char	x[32], y[32];

	FormatReal( Params, 0, 0, 1, Junction->point.x, x, sizeof x );
	FormatReal( Params, 0, 0, 1, Junction->point.y, y, sizeof y );

	OutputToFile( Params, Level, "(junction (pt %s %s) (netNameRef \"%s\"))\r\n", x, y, Junction->netnameref );

	return 0;
	}
/*=============================================================================*/
static int OutputPort( const parameters_t *Params, int Level, const pcad_port_t *Port )
	{
	char	x[32], y[32];

	FormatReal( Params, 0, 0, 1, Port->point.x, x, sizeof x );
	FormatReal( Params, 0, 0, 1, Port->point.y, y, sizeof y );

	OutputToFile( Params, Level, "(port (pt %s %s) (portType %s) (portPinLength %s) (netNameRef \"%s\")",
		x, y, PortTypes.items[Port->porttype], PortPinLengths.items[Port->portpinlength], Port->netnameref );

	if( Port->rotation != 0 )
		{
		FormatReal( Params, 0, 0, 1, Port->rotation, x, sizeof x );
		OutputToFile( Params, 0, " (rotation %s)", x );
		}

	if( Port->isflipped == PCAD_BOOLEAN_TRUE )
		OutputToFile( Params, 0, " (isFlipped True)" );

	OutputToFile( Params, 0, ")\r\n" );

	return 0;
	}
/*=============================================================================*/
static int OutputField( const parameters_t *Params, int Level, const pcad_field_t *Field )
	{
	char	x[32], y[32];

	FormatReal( Params, 0, 0, 1, Field->point.x, x, sizeof x );
	FormatReal( Params, 0, 0, 1, Field->point.y, y, sizeof y );

	OutputToFile( Params, Level, "(field \"%s\" (pt %s %s)", Field->name, x, y );
	if( Field->isflipped == PCAD_BOOLEAN_TRUE )
		OutputToFile( Params, 0, " (isFlipped True)" );
	OutputToFile( Params, 0, " (justify %s)", Justify.items[Field->justify] );
	if( Field->textstyleref != NULL )
		OutputToFile( Params, 0, " (textStyleRef \"%s\")", Field->textstyleref );

	OutputToFile( Params, 0, ")\r\n" );

	return 0;
	}
/*=============================================================================*/
static int OutputRefPoint( const parameters_t *Params, int Level, const pcad_refpoint_t *RefPoint )
	{
	char	x[32], y[32];

	FormatReal( Params, 0, 0, 1, RefPoint->point.x, x, sizeof x );
	FormatReal( Params, 0, 0, 1, RefPoint->point.y, y, sizeof y );

	OutputToFile( Params, Level, "(refPoint (pt %s %s))\r\n", x, y );

	return 0;
	}
/*=============================================================================*/
static int OutputSheet( const parameters_t *Params, int Level, const pcad_sheet_t *Sheet )
	{
	char	x[32], y[32];
	int		i;

	OutputToFile( Params, Level,	"(sheet \"%s\" (sheetNum %u)\r\n", Sheet->name, Sheet->sheetnum );

	if( Sheet->titlesheet.zones.textstyleref != NULL )
		OutputTitleSheet( Params, Level + 1, &Sheet->titlesheet );

	OutputToFile( Params, Level + 1, "(fieldSetRef \"%s\")\r\n", Sheet->fieldsetref );

	for( i = 0; i < Sheet->numsymbols; i++ )
		OutputSymbol( Params, Level + 1, Sheet->viosymbols[i] );

	for( i = 0; i < Sheet->numbuses; i++ )
		OutputBus( Params, Level + 1, Sheet->viobuses[i] );

	for( i = 0; i < Sheet->numbusentries; i++ )
		OutputBusEntry( Params, Level + 1, Sheet->viobusentries[i] );

	for( i = 0; i < Sheet->numwires; i++ )
		OutputWire( Params, Level + 1, Sheet->viowires[i] );

	for( i = 0; i < Sheet->numjunctions; i++ )
		OutputJunction( Params, Level + 1, Sheet->viojunctions[i] );

	for( i = 0; i < Sheet->numports; i++ )
		OutputPort( Params, Level + 1, Sheet->vioports[i] );

	for( i = 0; i < Sheet->numtexts; i++ )
		{
		OutputText( Params, Level + 1, Sheet->viotexts[i] );
		OutputToFile( Params, 0, "\r\n" );
		}

	for( i = 0; i < Sheet->numieeesymbols; i++ )
		OutputIEEESymbol( Params, Level + 1, Sheet->vioieeesymbols[i] );

	for( i = 0; i < Sheet->numlines; i++ )
		OutputLine( Params, Level + 1, Sheet->violines[i] );

	for( i = 0; i < Sheet->numtriplepointarcs; i++ )
		OutputArc( Params, Level + 1, Sheet->viotriplepointarcs[i] );

	for( i = 0; i < Sheet->numpolys; i++ )
		OutputPoly( Params, Level + 1, Sheet->viopolys[i] );

	for( i = 0; i < Sheet->numpins; i++ )
		OutputPin( Params, Level + 1, Sheet->viopins[i] );

	for( i = 0; i < Sheet->numattrs; i++ )
		OutputAttr( 1, Params, Level + 1, Sheet->vioattrs[i] );

	for( i = 0; i < Sheet->numfields; i++ )
		OutputField( Params, Level + 1, Sheet->viofields[i] );

	for( i = 0; i < Sheet->numrefpoints; i++ )
		OutputRefPoint( Params, Level + 1, Sheet->viorefpoints[i] );

	OutputToFile( Params, Level + 1, "(drawBorder %s)\r\n", Sheet->drawborder == PCAD_BOOLEAN_TRUE ? "True" : "False" );
	OutputToFile( Params, Level + 1, "(EntireDesign %s)\r\n", Sheet->entiredesign == PCAD_BOOLEAN_TRUE ? "True" : "False" );
	OutputToFile( Params, Level + 1, "(isRotated %s)\r\n", Sheet->isrotated == PCAD_BOOLEAN_TRUE ? "True" : "False" );
	OutputToFile( Params, Level + 1, "(pageSize %s)\r\n", PageSize.items[Sheet->pagesize] );

	FormatReal( Params, 2, 0, 1, Sheet->scalefactor, x, sizeof x );
	OutputToFile( Params, Level + 1, "(scaleFactor %s)\r\n", x );
	FormatReal( Params, 0, 0, 1, Sheet->offset.x, x, sizeof x );
	FormatReal( Params, 0, 0, 1, Sheet->offset.y, y, sizeof y );
	OutputToFile( Params, Level + 1, "(offset %s %s)\r\n", x, y );
	FormatReal( Params, 0, 0, 1, Sheet->printregion.p1.x, x, sizeof x );
	FormatReal( Params, 0, 0, 1, Sheet->printregion.p1.y, y, sizeof y );
	OutputToFile( Params, Level + 1, "(PrintRegion\r\n" );
	OutputToFile( Params, Level + 2, "(pt %s %s)", x, y );
	FormatReal( Params, 0, 0, 1, Sheet->printregion.p2.x, x, sizeof x );
	FormatReal( Params, 0, 0, 1, Sheet->printregion.p2.y, y, sizeof y );
	OutputToFile( Params, 0, " (pt %s %s)\r\n", x, y );
	OutputToFile( Params, Level + 1, ")\r\n" );
	OutputToFile( Params, Level + 1, "(sheetOrderNum %u)\r\n", Sheet->sheetordernum );

	OutputToFile( Params, Level,	")\r\n" );

	return 0;
	}
/*=============================================================================*/
static int OutputProgramState( parameters_t *Params, int Level, const pcad_programstate_t *ProgramState )
	{
	char	Buffer[32];

	OutputToFile( Params, 0,			"\r\n" );
	OutputToFile( Params, Level,		"(programState\r\n" );
	if( ProgramState->layerstate.currentlayer.layernumref != 0 )
		{
		OutputToFile( Params, Level + 1, "(layerState\r\n" );
		OutputToFile( Params, Level + 2, "(currentLayer (layerNumRef %u))\r\n", ProgramState->layerstate.currentlayer.layernumref );
		OutputToFile( Params, Level + 1, ")\r\n" );
		}

	OutputToFile( Params, Level + 1,	"(gridState\r\n" );
	FormatReal( Params, 3, 0, 1, ProgramState->gridstate.currentabsgrid, Buffer, sizeof Buffer );
	OutputToFile( Params, Level + 2,	"(currentAbsGrid \"%s%s\")\r\n", Buffer, Params->Cookie->FileUnits != PCAD_UNITS_MM ? "mm" : "" );
	FormatReal( Params, 3, 0, 1, ProgramState->gridstate.currentrelgrid, Buffer, sizeof Buffer );
	OutputToFile( Params, Level + 2,	"(currentRelGrid \"%s%s\")\r\n", Buffer, Params->Cookie->FileUnits != PCAD_UNITS_MM ? "mm" : "" );
	OutputToFile( Params, Level + 2,	"(isAbsoluteGrid %s)\r\n", ProgramState->gridstate.isabsolutegrid == PCAD_BOOLEAN_TRUE ? "True" : "False" );
	OutputToFile( Params, Level + 2,	"(isDottedGrid %s)\r\n", ProgramState->gridstate.isdottedgrid == PCAD_BOOLEAN_TRUE ? "True" : "False" );
	OutputToFile( Params, Level + 2,	"(isVisibleGrid %s)\r\n", ProgramState->gridstate.isvisiblegrid == PCAD_BOOLEAN_TRUE ? "True" : "False" );
	OutputToFile( Params, Level + 2,	"(isPromptForRel %s)\r\n", ProgramState->gridstate.ispromptforrel == PCAD_BOOLEAN_TRUE ? "True" : "False" );
	OutputToFile( Params, Level + 1,	")\r\n" );
	OutputToFile( Params, Level + 1,	"(ecoState (ecoRecording %s))\r\n", ProgramState->ecostate.ecorecording == PCAD_BOOLEAN_TRUE ? "True" : "False" );
	OutputToFile( Params, Level + 1,	"(currentTextStyle \"%s\")\r\n", ProgramState->currenttextstyle );
	OutputToFile( Params, Level,		")\r\n" );

	return 0;
	}
/*=============================================================================*/
static int OutputSchematicPrintSettings( const parameters_t *Params, int Level, const pcad_schematicprintst_t *SchematicPrintSettings )
	{
	if( SchematicPrintSettings->sheetlist.numsheetrefs > 0 )
		{
		int	i;

		OutputToFile( Params, Level,	"(schematicPrintSettings\r\n" );
		OutputToFile( Params, Level + 1,	"(sheetList\r\n" );
		for( i = 0; i < SchematicPrintSettings->sheetlist.numsheetrefs; i++ )
			OutputToFile( Params, Level + 2,	"(sheetRef %u)\r\n", SchematicPrintSettings->sheetlist.viosheetrefs[i]->sheetref );
		OutputToFile( Params, Level + 1,	")\r\n" );
		OutputToFile( Params, Level,	")\r\n" );

		}

	return 0;
	}
/*=============================================================================*/
static int OutputReportField( const parameters_t *Params, int Level, const pcad_reportfield_t *ReportField )
	{
	OutputToFile( Params, Level,	"(reportField\r\n" );
	OutputToFile( Params, Level + 1,	"(reportFieldName \"%s\")\r\n", ReportField->reportfieldname );
	OutputToFile( Params, Level + 1,	"(reportFieldType %s)\r\n", ReportPropertyTypes.items[ReportField->reportfieldtype] );
	OutputToFile( Params, Level + 1,	"(reportFieldSortOrder %u)\r\n", ReportField->reportfieldsortorder );
	OutputToFile( Params, Level + 1,	"(reportFieldSortType %s)\r\n", SortTypes.items[ReportField->reportfieldsorttype] );
	OutputToFile( Params, Level + 1,	"(reportFieldShowFlag %s)\r\n", ReportField->reportfieldshowflag == PCAD_BOOLEAN_TRUE ? "True" : "False" );
	OutputToFile( Params, Level + 1,	"(reportFieldColumnWidth %u)\r\n", ReportField->reportfieldcolumnwidth );

	if( ReportField->reportfieldconditions.numreportfieldconditions > 0 )
		{
		int i;

		OutputToFile( Params, Level + 1,"(reportFieldConditions\r\n" );
		for( i = 0; i < ReportField->reportfieldconditions.numreportfieldconditions; i++ )
			OutputToFile( Params, Level + 2,	"(reportFieldCondition \"%s\")\r\n", ReportField->reportfieldconditions.vioreportfieldconditions[i]->condition );
		OutputToFile( Params, Level + 1,")\r\n" );
		}

	OutputToFile( Params, Level,	")\r\n" );

	return 0;
	}
/*=============================================================================*/
static int OutputReportFields( const parameters_t *Params, int Level, const pcad_reportfields_t *ReportFields )
	{
	int	i;

	OutputToFile( Params, Level,	"(reportFields\r\n" );
	for( i = 0; i < ReportFields->numreportfields; i++ )
		OutputReportField( Params, Level + 1, ReportFields->vioreportfields[i] );
	OutputToFile( Params, Level,	")\r\n" );

	return 0;
	}
/*=============================================================================*/
static int OutputReportDefinition( parameters_t *Params, int Level, const pcad_reportdefinition_t *ReportDefinition )
	{

	OutputToFile( Params, Level,	"(reportDefinition\r\n" );
	OutputToFile( Params, Level + 1,	"(reportName \"%s\")\r\n", ReportDefinition->reportname );
	OutputToFile( Params, Level + 1,	"(reportExtension \"%s\")\r\n", ReportDefinition->reportextension );

	if( ReportDefinition->reportshowflag == PCAD_BOOLEAN_TRUE )
		OutputToFile( Params, Level + 1,"(reportShowFlag True)\r\n" );

	OutputToFile( Params, Level + 1,	"(reportType %s)\r\n", ReportTypes.items[ReportDefinition->reporttype] );

	if( ReportDefinition->reportuserdefined == PCAD_BOOLEAN_TRUE )
		OutputToFile( Params, Level + 1,"(reportUserDefined True)\r\n" );

	OutputToFile( Params, Level + 1,	"(reportLinesPerPage %u)\r\n", ReportDefinition->reportlinesperpage );
	OutputToFile( Params, Level + 1,	"(reportColumnWidth %u)\r\n", ReportDefinition->reportcolumnwidth );

	if( ReportDefinition->reportuseheader == PCAD_BOOLEAN_TRUE )
		OutputToFile( Params, Level + 1,"(reportUseHeader True)\r\n" );

	OutputToFile( Params, Level + 1,	"(reportHeader \"%s\")\r\n", ReportDefinition->reportheader );

	if( ReportDefinition->reportusefooter == PCAD_BOOLEAN_TRUE )
		OutputToFile( Params, Level + 1,"(reportUseFooter True)\r\n" );

	OutputToFile( Params, Level + 1,	"(reportFooter \"%s\")\r\n", ReportDefinition->reportfooter );

	if( ReportDefinition->reportusedesigninfo == PCAD_BOOLEAN_TRUE )
		OutputToFile( Params, Level + 1,"(reportUseDesignInfo True)\r\n" );
	if( ReportDefinition->reportshowdate == PCAD_BOOLEAN_TRUE )
		OutputToFile( Params, Level + 1,"(reportShowDate True)\r\n" );
	if( ReportDefinition->reportpaginateflag == PCAD_BOOLEAN_TRUE )
		OutputToFile( Params, Level + 1,"(reportPaginateFlag True)\r\n" );

	OutputToFile( Params, Level + 1,	"(reportShowCDFPreface %s)\r\n", ReportDefinition->reportshowcdfpreface == PCAD_BOOLEAN_TRUE ? "True" : "False" );
	OutputToFile( Params, Level + 1,	"(reportShowColumnNames %s)\r\n", ReportDefinition->reportshowcolumnnames == PCAD_BOOLEAN_TRUE ? "True" : "False" );

	if( ReportDefinition->reportfieldssections.numreportfieldss > 0 )
		{
		int	i;

		OutputToFile( Params, Level + 1,	"(reportFieldsSections\r\n" );
		for( i = 0; i < ReportDefinition->reportfieldssections.numreportfieldss; i++ )
			OutputReportFields( Params, Level + 2, ReportDefinition->reportfieldssections.vioreportfieldss[i] );
		OutputToFile( Params, Level + 1,	")\r\n" );
		}

	OutputToFile( Params, Level,	")\r\n" );

	return 0;
	}
/*=============================================================================*/
static int OutputReportSettings( parameters_t *Params, int Level, const pcad_reportsettings_t *ReportSettings )
	{
	char	x[32], y[32];
	int		i;

	OutputToFile( Params, 0,		"\r\n" );
	OutputToFile( Params, Level,	"(reportSettings\r\n" );
	OutputToFile( Params, Level + 1,	"(reportStyle %s)\r\n", ReportStyles.items[ReportSettings->reportstyle] );
	OutputToFile( Params, Level + 1,	"(reportDestination %s)\r\n", ReportDestinations.items[ReportSettings->reportdestination] );

	FormatReal( Params, 0, 0, 1, ReportSettings->reportorigin.x, x, sizeof x );
	FormatReal( Params, 0, 0, 1, ReportSettings->reportorigin.y, y, sizeof y );
	OutputToFile( Params, Level + 1,	"(pt %s %s)\r\n", x, y );

	OutputToFile( Params, Level + 1,	"(reportDefinitions\r\n" );
	for( i = 0; i < ReportSettings->reportdefinitions.numreportdefinitions; i++ )
		OutputReportDefinition( Params, Level + 2, ReportSettings->reportdefinitions.vioreportdefinitions[i] );
	OutputToFile( Params, Level + 1,	")\r\n" );

	OutputToFile( Params, Level,	")\r\n" );

	return 0;
	}
/*=============================================================================*/
static int OutputSchematicDesign( parameters_t *Params, int Level, const pcad_schematicdesign_t *SchematicDesign )
	{
	int	i;

	OutputToFile( Params, 0,		"\r\n" );
	OutputToFile( Params, Level,	"(schematicDesign \"%s\"\r\n", SchematicDesign->name );

	OutputSchDesignHeader( Params, Level + 1, &SchematicDesign->schdesignheader );
	if( SchematicDesign->titlesheet.zones.textstyleref != NULL )
		OutputTitleSheet( Params, Level + 1, &SchematicDesign->titlesheet );

	for( i = 0; i < SchematicDesign->numsheets; i++ )
		OutputSheet( Params, Level + 1, SchematicDesign->viosheets[i] );

	OutputSchematicPrintSettings( Params, Level + 1, &SchematicDesign->schematicPrintSettings );
	OutputProgramState( Params, Level + 1, &SchematicDesign->programstate );
	OutputReportSettings( Params, Level + 1, &SchematicDesign->reportsettings );

	OutputToFile( Params, Level,	")\r\n" );

	return 0;
	}
/*=============================================================================*/
void SplitPath( const char *pFullPath, char *pPath, char *pName, char *pExt );
/*=============================================================================*/
int OutputPCAD( cookie_t *Cookie, pcad_schematicfile_t *PCADSchematic, const char *pName )
	{
	parameters_t	Params;
	char			Path[256], Name[256], Ext[256], BkpPath[256], TmpPath[256];

	Params.Cookie	= Cookie;

	SplitPath( pName, Path, Name, Ext );
	if( stricmp( Ext, "" ) == 0 )
		strcpy( Ext, ".kicad_sch" );

	strcat( Path, Name );

	strcpy( TmpPath, Path );
	strcat( TmpPath, ".cvt_tmp" );

	if(( Params.File = fopen( TmpPath, "wb" )) == NULL )
		Error( Cookie, -1, "Error creating file %s", TmpPath );

	OutputToFile( &Params, 0, "ACCEL_ASCII \"%s\"\r\n", PCADSchematic->name );

	OutputASCIIHeader( &Params, 0, &PCADSchematic->asciiheader );
	OutputLibrary( &Params, 0, &PCADSchematic->library );
	if( PCADSchematic->netlist.vionets != NULL || PCADSchematic->netlist.viocompinsts != NULL )
		OutputNetList( &Params, 0, &PCADSchematic->netlist );
	OutputSchematicDesign( &Params, 0, &PCADSchematic->schematicdesign );

	fclose( Params.File );

	strcat( Path, Ext );

	strcpy( BkpPath, Path );
	strcat( BkpPath, ".cvt_bak" );

	remove( BkpPath );
	rename( Path, BkpPath );
	rename( TmpPath, Path );

	return 0;
	}
/*=============================================================================*/
