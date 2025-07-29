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
#include <limits.h>
#include "KiCADOutputSchematic.h"
#include "PCADStructs.h"
#include "Parser.h"
/*=============================================================================*/
typedef struct
	{
	char				*SheetName;
	cookie_t			*Cookie;
	FILE				*File;
	pcad_dimmension_t	DefaultLineWidth;
	pcad_dimmension_t	PolygonBorderWidth;
	int					PolygonExtraVertex;
	int					StraightBusEntries;
	pcad_dimmension_t	OriginX;
	pcad_dimmension_t	OriginY;
	pcad_real_t			ScaleX;
	pcad_real_t			ScaleY;
	} parameters_t;
/*=============================================================================*/
static const char *FormatLabel( const char *Label, char *Buffer, size_t BufferSize )
	{
	const char	*p	= Label;
	char		*q	= Buffer;
	int			Negated;

	if( Label == NULL )
		return "";

	for( Negated = 0; *p != 0 && BufferSize > 3; p++, BufferSize-- )
		{
		if( *p != '~' )
			*q++	= *p;
		else if( Negated )
			{
			*q++	= '}';
			Negated = 0;
			}
		else
			{
			*q++	= '~';
			*q++	= '{';
			Negated = 1;
			}
		}
	if( Negated )
		*q++	= '}';
	*q	= '\0';

	return Buffer;
	}
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

	Res	= snprintf( Buffer, BufferSize, "%s%u.%0*u", Sign ? "-" : "", Int, Dig, Frac );
	if( Res <= 0 || Res >= BufferSize )
		Error( Params->Cookie, -1, "Invalid number" );

	return Res;
	}
/*=============================================================================*/
static int OutputToFile( const parameters_t *Params, unsigned Level, const char *s, ... )
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
static const char *JustifyKiCAD[]	=
	{
	" (justify left bottom)",	" (justify bottom)",	" (justify right bottom)",
	" (justify left)",			"",						" (justify right)",
	" (justify left top)",		" (justify top)",		" (justify right top)"
	};
/*=============================================================================*/
pcad_busentry_t *FindBusEntry( pcad_sheet_t *Sheet, const pcad_point_t *p, const pcad_point_t *p2 )
	{
	int	i;

	for( i = 0; i < Sheet->numbusentries; i++ )
		{
		const pcad_busentry_t	*be	= Sheet->viobusentries[i];

		if( p->x == be->point.x && p->y == be->point.y && (
				( be->orient == PCAD_ORIENT_RIGHT && p2->x > p->x ) ||
				( be->orient == PCAD_ORIENT_LEFT && p2->x < p->x ) ||
				( be->orient == PCAD_ORIENT_UP && p2->y > p->y ) ||
				( be->orient == PCAD_ORIENT_DOWN && p2->y < p->y )))
			return Sheet->viobusentries[i];
		}

	return NULL;
	}
/*=============================================================================*/
static int OutputWire( const parameters_t *Params, unsigned Level, pcad_sheet_t *Sheet, pcad_wire_t *Wire )
	{
	char			x1[32], y1[32], x2[32], y2[32];
	pcad_busentry_t	*be1, *be2;
	pcad_point_t	pt1, pt2;

	pt1	= Wire->pt1;
	pt2	= Wire->pt2;

	if( Wire->endstyle1 != PCAD_ENDSTYLE_NONE && ( be1 = FindBusEntry( Sheet, &pt1, &pt2 )) && be1 != NULL )
		{
		if( pt2.x == pt1.x )
			{
			if( pt2.y > pt1.y + 2540000 )
				pt1.y += 2540000;
			else if( pt2.y < pt1.y - 2540000 )
				pt1.y -= 2540000;
			}
		else if( pt2.y == pt1.y )
			{
			if( pt2.x > pt1.x + 2540000 )
				pt1.x += 2540000;
			else if( pt2.x < pt1.x - 2540000 )
				pt1.x -= 2540000;
			else
				Warning( Params->Cookie, "Wire too short, won't adjust endpoint at (%.3f,%.3f) ", pt1.x / 1.0e6, pt1.y / 1.0e6 );
			}
		else
			Warning( Params->Cookie, "Wire meets bus non-perpendicularly, won't adjust endpoint at (%.3f,%.3f) ", pt1.x / 1.0e6, pt1.y / 1.0e6 );
		be1->point	= pt1;
		/* We must copy the style from the wire endpoint to the busentry, because it will be output later and we will need that information. */
		be1->style	= Wire->endstyle1;
		}

	if( Wire->endstyle2 != PCAD_ENDSTYLE_NONE && ( be2 = FindBusEntry( Sheet, &pt2, &pt1 )) && be2 != NULL )
		{
		if( pt1.x == pt2.x )
			{
			if( pt1.y > pt2.y + 2540000 )
				pt2.y += 2540000;
			else if( pt1.y < pt2.y - 2540000 )
				pt2.y -= 2540000;
			}
		else if( pt1.y == pt2.y )
			{
			if( pt1.x > pt2.x + 2540000 )
				pt2.x += 2540000;
			else if( pt1.x < pt2.x - 2540000 )
				pt2.x -= 2540000;
			else
				Warning( Params->Cookie, "Wire too short, won't adjust endpoint at (%.3f,%.3f) ", pt2.x / 1.0e6, pt2.y / 1.0e6 );
			}
		else
			Warning( Params->Cookie, "Wire meets bus non-perpendicularly, won't adjust endpoint at (%.3f,%.3f) ", pt2.x / 1.0e6, pt2.y / 1.0e6 );
		be2->point	= pt2;
		/* We must copy the style from the wire endpoint to the busentry, because it will be output later and we will need that information. */
		be2->style	= Wire->endstyle2;
		}

	FormatReal( Params, 0, Params->OriginX, Params->ScaleX, pt1.x, x1, sizeof x1 );
	FormatReal( Params, 0, Params->OriginY, Params->ScaleY, pt1.y, y1, sizeof y1 );
	FormatReal( Params, 0, Params->OriginX, Params->ScaleX, pt2.x, x2, sizeof x2 );
	FormatReal( Params, 0, Params->OriginY, Params->ScaleY, pt2.y, y2, sizeof y2 );

	OutputToFile( Params, Level, "(wire (pts (xy %s %s) (xy %s %s)) (stroke (width 0) (type default)))\n", x1, y1, x2, y2 );

	return 0;
	}
/*=============================================================================*/
static int OutputJunction( const parameters_t *Params, unsigned Level, pcad_junction_t *Junction )
	{
	char	x[32], y[32];

	FormatReal( Params, 0, Params->OriginX, Params->ScaleX, Junction->point.x, x, sizeof x );
	FormatReal( Params, 0, Params->OriginY, Params->ScaleY, Junction->point.y, y, sizeof y );

	OutputToFile( Params, Level, "(junction (at %s %s) (diameter 0) (color 0 0 0 0))\n", x, y );

	return 0;
	}
/*=============================================================================*/
static int OutputPort( const parameters_t *Params, unsigned Level, pcad_port_t *Port )
	{
	static const char *PortShapeKiCAD[] = { "passive", "input", "output", "bidirectional" };

	char		x[32], y[32], Angle[32];
	char		Buffer[3*strlen( Port->netnameref )+1];
	char		*Justify	= "left";
	int			i;
	pcad_real_t	PortRotation	= Port->rotation;
	int			PortIsFlipped	= Port->isflipped;

	i	= Port->porttype;
	if( i >= PCAD_PORTTYPE_NOANGLE_DBL_HORZ )
		fprintf( stderr, "Warning: KiCAD does not support 2-pins port type \"%s\", please review the resulting circuit.\n", PortTypes.items[Port->porttype] );
	else if( i >= PCAD_PORTTYPE_NOANGLE_DBL_HORZ )
		fprintf( stderr, "Warning: KiCAD does not have vertical port \"%s\", horizontal model used.\n", PortTypes.items[Port->porttype] );
	else if( i >= PCAD_PORTTYPE_VERTLINE_SGL_HORZ )
		fprintf( stderr, "Warning: KiCAD does not have port \"%s\", passive model used.\n", PortTypes.items[Port->porttype] );

	if(( i %= 6 ) >= LENGTH( PortShapeKiCAD ))
		i	= 0;

	FormatReal( Params, 0, Params->OriginX, Params->ScaleX, Port->point.x, x, sizeof x );
	FormatReal( Params, 0, Params->OriginY, Params->ScaleY, Port->point.y, y, sizeof y );
	FormatReal( Params, 0, 0,				1,				PortRotation, Angle, sizeof Angle );

	if( PortIsFlipped )
		{
		switch( PortRotation )
			{
			case 0:
				PortRotation	= 180000000;
				break;
			case 180000000:
				PortRotation	= 0;
				break;
			}
		PortIsFlipped = 0;
		}

	if( PortRotation == 180000000 || PortRotation == 270000000 )
		Justify = "right";

	OutputToFile( Params, Level, "(global_label \"%s\" (shape %s) (at %s %s %s) (effects (font (size 1.0 1.0)) (justify %s)))\n", FormatLabel( Port->netnameref, Buffer, sizeof Buffer ), PortShapeKiCAD[i], x, y, Angle, Justify );

	return 0;
	}
/*=============================================================================*/
static int OutputText( const parameters_t *Params, unsigned Level, pcad_text_t *Text )
	{
	char	x[32], y[32], Angle[32], Buffer[3*strlen( Text->value )+1];

	FormatReal( Params, 0, Params->OriginX, Params->ScaleX, Text->point.x, x, sizeof x );
	FormatReal( Params, 0, Params->OriginY, Params->ScaleY, Text->point.y, y, sizeof y );
	FormatReal( Params, 0, 0,				1,				Text->rotation, Angle, sizeof Angle );

	OutputToFile( Params, Level, "(text \"%s\" (exclude_from_sim no) (at %s %s %s) (effects (font (size 1.27 1.27))%s))\n", FormatLabel( Text->value, Buffer, sizeof Buffer ), x, y, Angle, JustifyKiCAD[Text->justify % LENGTH( JustifyKiCAD )] );
	return 0;
	}
/*=============================================================================*/
/*
static int OutputAttribute( const parameters_t *Params, unsigned Level, const char *NamePCAD, const char *NameKiCAD, const char *Value, pcad_attr_t **Attributes, size_t NumAttributes )
	{
	int i;
	char	x[32], y[32], Angle[32];

	for( i = 0; i < NumAttributes; i++ )
		if( stricmp( NamePCAD, Attributes[i]->name ) == 0 )
			break;
	if( i >= NumAttributes )
		return 0;

	pcad_attr_t *Attr	= Attributes[i];
	char		Buffer[3*strlen( Value != NULL ? Value : Attr->value )+1];

	FormatReal( Params, 0, Params->OriginX, Params->ScaleX, Attr->point.x, x, sizeof x );
	FormatReal( Params, 0, Params->OriginY, Params->ScaleY, Attr->point.y, y, sizeof y );
	FormatReal( Params, 0, 0,				1,				Attr->rotation, Angle, sizeof Angle );

	OutputToFile( Params, Level, "(property \"%s\" \"%s\" (at %s %s %s) (effects (font (size 1.27 1.27))))\n", NameKiCAD, FormatLabel( Value != NULL ? Value : Attr->value, Buffer, sizeof Buffer ), x, y, Angle );
	return 1;
	}
*/
/*=============================================================================*/
static int OutputLine( const parameters_t *Params, unsigned Level, pcad_line_t	*Line )
	{
	char	x1[32], y1[32], x2[32], y2[32], Width[32];

	FormatReal( Params, 0, Params->OriginX, Params->ScaleX, Line->pt1.x, x1, sizeof x1 );
	FormatReal( Params, 0, Params->OriginY, Params->ScaleY, Line->pt1.y, y1, sizeof y1 );
	FormatReal( Params, 0, Params->OriginX, Params->ScaleX, Line->pt2.x, x2, sizeof x2 );
	FormatReal( Params, 0, Params->OriginY, Params->ScaleY, Line->pt2.y, y2, sizeof y2 );
	if( Line->width == 0 )
		FormatReal( Params, 0, 0, 1, Params->DefaultLineWidth, Width, sizeof Width );
	else
		FormatReal( Params, 0, 0, 1, Line->width, Width, sizeof Width );

	OutputToFile( Params, Level, "(polyline (pts (xy %s %s) (xy %s %s)) (stroke (width %s) (type default)) (fill (type none)))\n", x1, y1, x2, y2, Width );

	return 0;
	}
/*=============================================================================*/
#if 0
static void Rotate( pcad_dimmension_t *x, pcad_dimmension_t *y, pcad_dimmension_t xc, pcad_dimmension_t yc, float_t angle )
	{
	double	sinA, cosA;
	double	dX, dY;

	sincos( angle / 1.0e6, &sinA, &cosA );
	dX	= ( *x - xc ) / 1.0e6;
	dY	= ( *y - yc ) / 1.0e6;

	*x	= (int)(( dX * cosA - dY * sinA ) * 1.0e6 ) + xc;	//10000 ) * 100 + xc;
	*y	= (int)(( dY * cosA + dX * sinA ) * 1.0e6 ) + yc;	//10000 ) * 100 + yc;
	}
#endif
/*=============================================================================*/
static int OutputArc( const parameters_t *Params, unsigned Level, pcad_triplepointarc_t *Arc )
	{
	char	x1[32], y1[32], x2[32], y2[32], x3[32], y3[32], Width[32], Radius[32];

	if( Arc->width == 0 )
		FormatReal( Params, 0, 0, 1, Params->DefaultLineWidth, Width, sizeof Width );
	else
		FormatReal( Params, 0, 0, 1, Arc->width, Width, sizeof Width );

	if( Arc->point2.x == Arc->point3.x && Arc->point2.y == Arc->point3.y )
		{
		pcad_dimmension_t	r;
		double				dx, dy;

		dx	= ( Arc->point1.x - Arc->point2.x ) / 1.0e6;
		dy	= ( Arc->point1.y - Arc->point2.y ) / 1.0e6;
		r	= (pcad_dimmension_t)( sqrtl( dx * dx + dy * dy ) * 1.0e6 ); //* 100 + 0.5 ) * 10000;

		FormatReal( Params, 0, Params->OriginX, Params->ScaleX, Arc->point1.x, x1, sizeof x1 );
		FormatReal( Params, 0, Params->OriginY, Params->ScaleY, Arc->point1.y, y1, sizeof y1 );
		FormatReal( Params, 0, 0,				1,				r, Radius, sizeof Radius );

		OutputToFile( Params, Level, "(circle (center %s %s) (radius %s) (stroke (width %s) (type default)) (fill (type none)))\n", x1, y1, Radius, Width );
		}
	else
		{
		double	a2, a3, a;
		double	dx, dy, mx, my;
		pcad_dimmension_t	ArcPoint1X	= Arc->point1.x;
		pcad_dimmension_t	ArcPoint1Y	= Arc->point1.y;
		pcad_dimmension_t	ArcPoint2X	= Arc->point2.x;
		pcad_dimmension_t	ArcPoint2Y	= Arc->point2.y;
		pcad_dimmension_t	ArcPoint3X	= Arc->point3.x;
		pcad_dimmension_t	ArcPoint3Y	= Arc->point3.y;

		a2	= atan2( ( ArcPoint1Y - ArcPoint2Y ) / 1.0e6, ( ArcPoint1X - ArcPoint2X ) / 1.0e6 );
//		if( a2 < 0 )
//			a2 += 2 * M_PI;
		a3	= atan2( ( ArcPoint1Y - ArcPoint3Y ) / 1.0e6, ( ArcPoint1X - ArcPoint3X ) / 1.0e6 );
//		if( a3 <= 0 )
//			a3 += 2 * M_PI;

		if( a3 <= a2 )
			a3	+= 2 * M_PI;

		a	= fabs( a3 - a2 ) / 2.0;

		dx	= ( ArcPoint2X - ArcPoint1X ) / 1.0e6;
		dy	= ( ArcPoint2Y - ArcPoint1Y ) / 1.0e6;

		mx	= dx * cos( a ) - dy * sin( a ) + ArcPoint1X / 1.0e6;
		my	= dy * cos( a ) + dx * sin( a ) + ArcPoint1Y / 1.0e6;

		ArcPoint1X	= ( (int)( mx * 1.0e6 ));	//10000 )) * 100;
		ArcPoint1Y	= ( (int)( my * 1.0e6 ));	//10000 )) * 100;
		FormatReal( Params, 0, Params->OriginX, Params->ScaleX, ArcPoint1X, x1, sizeof x1 );
		FormatReal( Params, 0, Params->OriginY, Params->ScaleY, ArcPoint1Y, y1, sizeof y1 );

		FormatReal( Params, 0, Params->OriginX, Params->ScaleX, ArcPoint2X, x2, sizeof x2 );
		FormatReal( Params, 0, Params->OriginY, Params->ScaleY, ArcPoint2Y, y2, sizeof y2 );
		FormatReal( Params, 0, Params->OriginX, Params->ScaleX, ArcPoint3X, x3, sizeof x3 );
		FormatReal( Params, 0, Params->OriginY, Params->ScaleY, ArcPoint3Y, y3, sizeof y3 );

		OutputToFile( Params, Level, "(arc (start %s %s) (mid %s %s) (end %s %s) (stroke (width %s) (type default)) (fill (type none)))\n", x2, y2, x1, y1, x3, y3, Width );
		}

	return 0;
	}
/*=============================================================================*/
static pcad_comppin_t *FindPin( const parameters_t *Params, unsigned PartNumber, unsigned PinNumber, const pcad_compdef_t *CompDef )
	{
	int	i;

	for( i = 0; i < CompDef->numcomppins; i++ )
		if( CompDef->viocomppins[i]->partnum == PartNumber && CompDef->viocomppins[i]->sympinnum == PinNumber )
			return CompDef->viocomppins[i];

	return NULL;
	}
/*=============================================================================*/
static int OutputPin( const parameters_t *Params, unsigned Level, pcad_pin_t *Pin, int PinType, unsigned PartNumber, unsigned PinNumber, const pcad_compdef_t *CompDef )
	{
	static const char	*GraphStyles[2][4]	=
		{
			{ "line",	"inverted",			"input_low",	"output_low" },
			{ "clock",	"inverted_clock",	"clock_low",	"output_low" }
		};

	static const char	*PinTypes[16]		=
		{
		"unspecified",
		"unspecified",
		"passive",
		"input",
		"output",
		"bidirectional",
		"open_collector",
		"open_emitter",
		"passive",
		"passive",
		"tri_state",
		"power_in",
		"unspecified",
		"unspecified",
		"unspecified",
		"unspecified"
		};

	char					x[32], y[32], Angle[32], Length[32], Buffer[3*strlen( Pin->pinname.value )+1];
	const char				*GraphStyle;
	pcad_real_t				PinRotation = ( Pin->rotation + 180000000 ) % 360000000;
	pcad_real_t				PinX		= Pin->point.x;
	pcad_real_t				PinY		= Pin->point.y;
	const pcad_comppin_t	*CompPin;

	switch( PinRotation )
		{
		case 0:
			PinX -= Pin->pinlength;
			break;
		case 90000000:
			PinY -= Pin->pinlength;
			break;
		case 180000000:
			PinX += Pin->pinlength;
			break;
		case 270000000:
			PinY += Pin->pinlength;
			break;
		}

	FormatReal( Params, 0, Params->OriginX, Params->ScaleX, PinX, x, sizeof x );
	FormatReal( Params, 0, Params->OriginY, Params->ScaleY, PinY, y, sizeof y );
	FormatReal( Params, 0, 0,				1,				PinRotation, Angle, sizeof Angle );
	FormatReal( Params, 0, 0,				1,				Pin->pinlength, Length, sizeof Length );

	GraphStyle	= GraphStyles[Pin->insideedgestyle&1][Pin->outsideedgestyle&3];

	if(( CompPin = FindPin( Params, PartNumber, PinNumber, CompDef )) == NULL )
		Error( Params->Cookie, -1, "CompPin not found" );

	if( CompPin->pinname == NULL )
		OutputToFile( Params, Level, "(pin %s %s (at %s %s %s) (length %s) (number \"%s\" (effects (font (size 1.27 1.27)))))\n", PinTypes[PinType&15] /*IsPower ? "power_out" : "passive"*/, GraphStyle, x, y, Angle, Length, CompPin->pinnumber );
	else
		OutputToFile( Params, Level, "(pin %s %s (at %s %s %s) (length %s) (name \"%s\" (effects (font (size 1.27 1.27)))) (number \"%s\" (effects (font (size 1.27 1.27)))))\n", PinTypes[PinType&15] /*IsPower ? "power_out" : "passive"*/, GraphStyle, x, y, Angle, Length, FormatLabel( CompPin->pinname, Buffer, sizeof Buffer ), CompPin->pinnumber );

	return 0;
	}
/*=============================================================================*/
static const pcad_compdef_t *FindCompDef( const pcad_schematicfile_t *Schematic, const char *Name )
	{
	int i;

	for( i = 0; i < Schematic->library.numcompdefs; i++ )
		if( stricmp( Name, Schematic->library.viocompdefs[i]->name ) == 0 )
			return Schematic->library.viocompdefs[i];

	return NULL;
	}
/*=============================================================================*/
static int OutputPolygon( const parameters_t *Params, unsigned Level, pcad_poly_t *Polygon )
	{
	char	x[32], y[32], Width[32];
	int		i;

	OutputToFile( Params, Level, "(polyline (pts" );

	for( i = 0; i < Polygon->numpoints; i++ )
		{
		FormatReal( Params, 0, Params->OriginX, Params->ScaleX, Polygon->viopoints[i]->x, x, sizeof x );
		FormatReal( Params, 0, Params->OriginY, Params->ScaleY, Polygon->viopoints[i]->y, y, sizeof y );
		OutputToFile( Params, 0, " (xy %s %s)", x, y );
		}

	FormatReal( Params, 0, 0, 1, Params->PolygonBorderWidth, Width, sizeof Width );

	if( Params->PolygonExtraVertex )
		{
		FormatReal( Params, 0, Params->OriginX, Params->ScaleX, Polygon->viopoints[0]->x, x, sizeof x );
		FormatReal( Params, 0, Params->OriginY, Params->ScaleY, Polygon->viopoints[0]->y, y, sizeof y );
		OutputToFile( Params, 0, " (xy %s %s)) (stroke (width %s) (type default)) (fill (type outline)))\n", x, y, Width );
		}
	else
		OutputToFile( Params, 0, ") (stroke (width %s) (type default)) (fill (type outline)))\n", Width );

	return 0;
	}
/*=============================================================================*/
static int OutputSymbolDef( const parameters_t *Params, unsigned Level, unsigned Index, const pcad_schematicfile_t *Schematic, pcad_symboldef_t *SymbolDef, const pcad_compdef_t *CompDef )
	{
//	char					*RefDesPrefix	= NULL;
	int						IsPower			= 0;
	int						PinType			= 0;
	int						i;
	parameters_t			LocalParams		= *Params;

	LocalParams.ScaleX	= 1;
	LocalParams.OriginX = 0;
	LocalParams.ScaleY	= 1;
	LocalParams.OriginY = 0;

	IsPower			= CompDef->compheader.comptype == PCAD_COMPTYPE_POWER;
//	RefDesPrefix	= CompDef->compheader.refdesprefix;

/*
	OutputToFile( &LocalParams, Level, "(symbol \"%s\"\n", SymbolDef->name );

	if(( CompDef = FindCompDef( Schematic, SymbolDef->originalname )) != NULL )
		{
		IsPower			= CompDef->compheader.comptype == PCAD_COMPTYPE_POWER;
		RefDesPrefix	= CompDef->compheader.refdesprefix;
		}

	if( IsPower )
		OutputToFile( &LocalParams, Level + 1, "(power) (pin_numbers hide) (pin_names (offset 0.5) hide) (exclude_from_sim no) (in_bom yes) (on_board yes)\n" );
	else
		{
		int HidePinNumbers	= 1;
		int HidePinNames	= 1;
		for( i = 0; i < SymbolDef->numpins; i++ )
			{
			if( SymbolDef->viopins[i]->displaypindes != PCAD_BOOLEAN_FALSE )
				HidePinNumbers	= 0;
			if( SymbolDef->viopins[i]->displaypinname == PCAD_BOOLEAN_TRUE )
				HidePinNames	= 0;
			}
		OutputToFile( &LocalParams, Level + 1, "%s(pin_names (offset 0.5)%s) (exclude_from_sim no) (in_bom yes) (on_board yes)\n", HidePinNumbers ? "(pin_numbers hide) " : "", HidePinNames ? " hide" : "" );
		}

	OutputAttribute( &LocalParams, Level + 1, "RefDes", "Reference", RefDesPrefix, SymbolDef->vioattrs, SymbolDef->numattrs );
	OutputAttribute( &LocalParams, Level + 1, "Value", "Value", NULL, SymbolDef->vioattrs, SymbolDef->numattrs );
*/

	OutputToFile( &LocalParams, Level, "(symbol \"%s_%u_1\"\n", CompDef->name, Index + 1 );


	for( i = 0; i < SymbolDef->numlines; i++ )
		OutputLine( &LocalParams, Level + 1, SymbolDef->violines[i] );

	for( i = 0; i < SymbolDef->numtriplepointarcs; i++ )
		OutputArc( &LocalParams, Level + 1, SymbolDef->viotriplepointarcs[i] );

	for( i = 0; i < SymbolDef->numpolys; i++ )
		OutputPolygon( &LocalParams, Level + 1, SymbolDef->viopolys[i] );

	for( i = 0; i < SymbolDef->numpins; i++ )
		{
		if( IsPower )
			PinType	= PCAD_PINTYPE_POWER;
		else
			PinType	= 0;	//@@@@

		OutputPin( &LocalParams, Level + 1, SymbolDef->viopins[i], PinType, Index + 1, i + 1, CompDef );
		}
	OutputToFile( &LocalParams, Level, ")\n" );
	return 0;
	}
/*=============================================================================*/
static pcad_symboldef_t *FindSymbolDef( const pcad_schematicfile_t *Schematic, const char *Name )
	{
	int i;

	for( i = 0; i < Schematic->library.numsymboldefs; i++ )
		if( stricmp( Name, Schematic->library.viosymboldefs[i]->name ) == 0 )
			break;
	if( i < Schematic->library.numsymboldefs )
		return Schematic->library.viosymboldefs[i];

	return NULL;
	}
/*=============================================================================*/
static pcad_symboldef_t *FindSymbolDefByOriginalName( const pcad_schematicfile_t *Schematic, const char *Name )
	{
	int i;

	for( i = 0; i < Schematic->library.numsymboldefs; i++ )
		if( stricmp( Name, Schematic->library.viosymboldefs[i]->originalname ) == 0 )
			break;
	if( i < Schematic->library.numsymboldefs )
		return Schematic->library.viosymboldefs[i];

	return NULL;
	}
/*=============================================================================*/
static int OutputCompDef( const parameters_t *Params, unsigned Level, const pcad_schematicfile_t *Schematic, const pcad_library_t *Library, pcad_compdef_t *CompDef )
	{
	parameters_t	LocalParams		= *Params;
	int				i;

	LocalParams.ScaleX	= 1;
	LocalParams.OriginX = 0;
	LocalParams.ScaleY	= 1;
	LocalParams.OriginY = 0;

	if( CompDef->compheader.sourcelibrary != NULL && CompDef->compheader.sourcelibrary[0] != '\0' && CompDef->compheader.sourcelibrary[0] != '.' )
		{
		const char	*p;
		if(( p = strrchr( CompDef->compheader.sourcelibrary, '.' )) != NULL )
			OutputToFile( &LocalParams, Level, "(symbol \"%.*s:%s\"\n", p - CompDef->compheader.sourcelibrary, CompDef->compheader.sourcelibrary, CompDef->name );
		else
			OutputToFile( &LocalParams, Level, "(symbol \"%s:%s\"\n", CompDef->compheader.sourcelibrary, CompDef->name );
		}
	else
		OutputToFile( &LocalParams, Level, "(symbol \"%s\"\n", CompDef->name );

	if( CompDef->compheader.comptype == PCAD_COMPTYPE_POWER )
		OutputToFile( &LocalParams, Level + 1, "(power) (pin_numbers hide) (pin_names (offset 0.5) hide) (exclude_from_sim no) (in_bom yes) (on_board yes)\n" );
	else
		{
		int HidePinNumbers	= 0;
		int HidePinNames	= 0;
/*
		for( i = 0; i < SymbolDef->numpins; i++ )
			{
			if( SymbolDef->viopins[i]->displaypindes != PCAD_BOOLEAN_FALSE )
				HidePinNumbers	= 0;
			if( SymbolDef->viopins[i]->displaypinname == PCAD_BOOLEAN_TRUE )
				HidePinNames	= 0;
			}
*/
		OutputToFile( &LocalParams, Level + 1, "%s(pin_names (offset 0.5)%s) (exclude_from_sim no) (in_bom yes) (on_board yes)\n", HidePinNumbers ? "(pin_numbers hide) " : "", HidePinNames ? " hide" : "" );
		}

	OutputToFile( &LocalParams, Level + 1, "(property \"Reference\" \"%s\" (at 6.35 -1.27 0)(effects (font (size 1.27 1.27)) (justify left)))\n", CompDef->compheader.refdesprefix );
	OutputToFile( &LocalParams, Level + 1, "(property \"Value\" \"%s\" (at 6.35 1.27 0)(effects (font (size 1.27 1.27)) (justify left)))\n", CompDef->originalname );
	if( CompDef->attachedpattern.patternname != NULL )
		OutputToFile( &LocalParams, Level + 1, "(property \"Footprint\" \"%s\" (at 6.35 -3.81 0)(effects (font (size 1.27 1.27)) (justify left) (hide yes)))\n", CompDef->attachedpattern.patternname );

	for( i = 0; i < CompDef->numattachedsymbols; i++ )
		{
		pcad_symboldef_t	*SymbolDef;
//		fprintf( stderr, "\nSymbol %d %s", i, CompDef->vioattachedsymbols[i]->symbolname );

		if(( SymbolDef = FindSymbolDefByOriginalName( Schematic, CompDef->vioattachedsymbols[i]->symbolname )) != NULL )
			OutputSymbolDef( Params, Level + 1, i, Schematic, SymbolDef, CompDef );
		}
	OutputToFile( &LocalParams, Level, ")\n" );
	return 0;
	}
/*=============================================================================*/
static int OutputLibrary( const parameters_t *Params, unsigned Level, const pcad_schematicfile_t *Schematic, const pcad_library_t *Library )
	{
	int i;

	OutputToFile( Params, Level, "(lib_symbols\n" );

	for( i = 0; i < Library->numcompdefs; i++ )
		OutputCompDef( Params, Level + 1, Schematic, Library, Library->viocompdefs[i] );
/*
	for( i = 0; i < Library->numsymboldefs; i++ )
		OutputSymbolDef( Params, Level + 1, Schematic, Library->viosymboldefs[i] );
*/
	OutputToFile( Params, Level, ")\n" );
	return 0;
	}
/*=============================================================================*/
static pcad_attr_t *FindAttr( pcad_attr_t **Attributes, size_t NumAttributes, char *Name )
	{
	int i;

	for( i = 0; i < NumAttributes; i++ )
		if( stricmp( Name, Attributes[i]->name ) == 0 )
			break;
	if( i < NumAttributes )
		return Attributes[i];

	return NULL;
	}
/*=============================================================================*/
static pcad_compinst_t *FindCompInst( const pcad_netlist_t *NetList, const char *Name )
	{
	int i;

	for( i = 0; i < NetList->numcompinsts; i++ )
		if( stricmp( Name, NetList->viocompinsts[i]->name ) == 0 )
			break;
	if( i < NetList->numcompinsts )
		return NetList->viocompinsts[i];

	return NULL;
	}
/*=============================================================================*/
#if			!defined MAX
#define MAX(a,b)	((a)>(b)?(a):(b))
#endif	/*	!defined MAX */
/*=============================================================================*/
static int OutputSymbol( const parameters_t *Params, unsigned Level, const pcad_schematicfile_t *Schematic, pcad_symbol_t *Symbol )
	{
	const pcad_symboldef_t	*SymbolDef	= NULL;
	const pcad_compdef_t	*CompDef	= NULL;
	const pcad_compinst_t	*CompInst;
	char					x[32], y[32], Angle[32];
	int						IsPower = 0;

	FormatReal( Params, 0, Params->OriginX, Params->ScaleX, Symbol->pt.x, x, sizeof x );
	FormatReal( Params, 0, Params->OriginY, Params->ScaleY, Symbol->pt.y, y, sizeof y );
	FormatReal( Params, 0, 0,				1,				Symbol->rotation, Angle, sizeof Angle );

	if(( SymbolDef = FindSymbolDef( Schematic, Symbol->symbolref )) == NULL )
		Error( Params->Cookie, -1, "SymbolDef \"%s\" not found", Symbol->symbolref );

	if(( CompInst = FindCompInst( &Schematic->netlist, Symbol->refdesref )) == NULL )
		Error( Params->Cookie, -1, "CompInst \"%s\" not found", Symbol->refdesref );

	if(( CompDef = FindCompDef( Schematic, CompInst->compref )) == NULL )
		Error( Params->Cookie, -1, "CompDef \"%s\" not found", CompInst->compref );

	IsPower = CompDef->compheader.comptype == PCAD_COMPTYPE_POWER;

	OutputToFile( Params, Level, "(symbol\n" );

	if( CompDef->compheader.sourcelibrary != NULL && CompDef->compheader.sourcelibrary[0] != '\0' && CompDef->compheader.sourcelibrary[0] != '.' )
		{
		const char	*p;
		if(( p = strrchr( CompDef->compheader.sourcelibrary, '.' )) != NULL )
			OutputToFile( Params, Level + 1, "(lib_id \"%.*s:%s\")\n", p - CompDef->compheader.sourcelibrary, CompDef->compheader.sourcelibrary, CompDef->name );
		else
			OutputToFile( Params, Level + 1, "(lib_id \"%s:%s\")\n", CompDef->compheader.sourcelibrary, CompDef->name );
		}
	else
		OutputToFile( Params, Level + 1, "(lib_id \"%s\")\n", CompDef->name );

	OutputToFile( Params, Level + 1, "(at %s %s %s)\n", x, y, Angle );
	if( Symbol->isflipped )
		OutputToFile( Params, Level + 1, "(mirror y)\n" );
	OutputToFile( Params, Level + 1, "(unit %u)\n", Symbol->partnum );
	OutputToFile( Params, Level + 1, "(exclude_from_sim no)\n" );
	OutputToFile( Params, Level + 1, "(in_bom yes)\n" );
	OutputToFile( Params, Level + 1, "(on_board yes)\n" );
	OutputToFile( Params, Level + 1, "(dnp no)\n" );

	if( SymbolDef != NULL )
		{
		int					i, Column;
		pcad_dimmension_t	dx = 0, dy = 0, dAngle = 0;
		pcad_attr_t			*Attr;
		pcad_enum_justify_t	jf	= 0;
		int					RefDesVisible	= 0;

		if(( Attr = FindAttr( SymbolDef->vioattrs, SymbolDef->numattrs, "RefDes" )) != NULL )
			{
			RefDesVisible	= Attr->isvisible;
			dx				= ( Symbol->isflipped && ( Symbol->rotation == 0 || Symbol->rotation == 180000000 ) ? -1 : 1 ) * Attr->point.x;
			dy				= ( Symbol->isflipped && ( Symbol->rotation == 90000000 || Symbol->rotation == 270000000 ) ? -1 : 1 ) * Attr->point.y;
			dAngle			= Attr->rotation;
			jf				= Attr->justify;
			}

		if( Symbol->rotation != 0 )
			{
			double	X, Y, a;
			X	= dx / 1.0e6;
			Y	= dy / 1.0e6;
			a	= Symbol->rotation / 180.0e6 * M_PI;
			dx	= (int)(( X * cos( a ) - Y * sin( a )) * 1.0e6 );	//10000 ) * 100;
			dy	= (int)(( Y * cos( a ) + X * sin( a )) * 1.0e6 );	//10000 ) * 100;
			}

		FormatReal( Params, 0, Params->OriginX, Params->ScaleX, Symbol->pt.x + dx, x, sizeof x );
		FormatReal( Params, 0, Params->OriginY, Params->ScaleY, Symbol->pt.y + dy, y, sizeof y );
		FormatReal( Params, 0, 0,				1,				dAngle, Angle, sizeof Angle );

		OutputToFile( Params, Level + 1, "(property \"Reference\" \"%s\" (at %s %s %s) (effects (font (size 1.27 1.27))%s%s))\n", Symbol->refdesref, x, y, Angle, JustifyKiCAD[jf % LENGTH( JustifyKiCAD )], RefDesVisible ? "" : " (hide yes)" );

		if( CompInst != NULL )
			{
			char	*CompValue		= CompInst->compvalue != NULL ? CompInst->compvalue : "";
			char	*Footprint		= CompInst->patternname != NULL ? CompInst->patternname : "";
			int		ValueVisible	= 0;
			char	Buffer[3*MAX( strlen( IsPower ? CompInst->originalname : CompValue ), strlen( Footprint ))+1];

			dx = dy = dAngle			= 0;
			pcad_attr_t *Value			= FindAttr( SymbolDef->vioattrs, SymbolDef->numattrs, "Value" );
			jf	= 0;
			if( Value != NULL )
				{
				ValueVisible	= Value->isvisible;
				dx		= ( Symbol->isflipped && ( Symbol->rotation == 0 || Symbol->rotation == 180000000 ) ? -1 : 1 ) * Value->point.x;
				dy		= ( Symbol->isflipped && ( Symbol->rotation == 90000000 || Symbol->rotation == 270000000 ) ? -1 : 1 ) * Value->point.y;
				dAngle	= Value->rotation;
				jf		= Value->justify;
				}

			if( Symbol->rotation != 0 )
				{
				double	X, Y, a;
				X	= dx / 1.0e6;
				Y	= dy / 1.0e6;
				a	= Symbol->rotation / 180.0e6 * M_PI;
				dx	= (int)(( X * cos( a ) - Y * sin( a )) * 1.0e6 );	//10000 ) * 100;
				dy	= (int)(( Y * cos( a ) + X * sin( a )) * 1.0e6 );	//10000 ) * 100;
				}

			FormatReal( Params, 0, Params->OriginX, Params->ScaleX, Symbol->pt.x + dx, x, sizeof x );
			FormatReal( Params, 0, Params->OriginY, Params->ScaleY, Symbol->pt.y + dy, y, sizeof y );
			FormatReal( Params, 0, 0,				1,				dAngle, Angle, sizeof Angle );

			OutputToFile( Params, Level + 1, "(property \"Value\" \"%s\" (at %s %s %s) (effects (font (size 1.27 1.27))%s%s))\n", FormatLabel( IsPower ? CompInst->originalname : CompValue, Buffer, sizeof Buffer ), x, y, Angle, JustifyKiCAD[jf % LENGTH( JustifyKiCAD )], ValueVisible || IsPower ? "" : " (hide yes)"	);
			OutputToFile( Params, Level + 1, "(property \"Footprint\" \"%s\" (at %s %s %s) (effects (font (size 1.27 1.27))%s (hide yes)))\n", FormatLabel( Footprint, Buffer, sizeof Buffer ), x, y, Angle, JustifyKiCAD[jf % ( LENGTH( JustifyKiCAD ) - 1 )] );
			}

		OutputToFile( Params, Level + 1, "" );
		for( i = 0, Column = 0; i < SymbolDef->numpins; i++ )
			{
			if( Column > 80 )
				{
				OutputToFile( Params, 0, "\n" );
				Column	= OutputToFile( Params, Level + 1, "" );
				}
			Column += OutputToFile( Params, 0, "(pin \"%u\") ", SymbolDef->viopins[i]->pinnum );
			}
		if( Column != 0 )
			OutputToFile( Params, 0, "\n" );
		OutputToFile( Params, Level + 1, "(instances (project \"%s\" (path \"/5eab5f85-83b1-44f7-98c0-9af69d3534bc\" (reference \"%s\") (unit %u))))\n", Params->SheetName, Symbol->refdesref, Symbol->partnum );
		}

	OutputToFile( Params, Level, ")\n" );

	return 0;
	}
/*=============================================================================*/
static int OutputBus( const parameters_t *Params, unsigned Level, pcad_bus_t *Bus )
	{
	char	x1[32], y1[32], x2[32], y2[32] /*, Width[32]*/;

	FormatReal( Params, 0, Params->OriginX, Params->ScaleX, Bus->pt1.x, x1, sizeof x1 );
	FormatReal( Params, 0, Params->OriginY, Params->ScaleY, Bus->pt1.y, y1, sizeof y1 );
	FormatReal( Params, 0, Params->OriginX, Params->ScaleX, Bus->pt2.x, x2, sizeof x2 );
	FormatReal( Params, 0, Params->OriginY, Params->ScaleY, Bus->pt2.y, y2, sizeof y2 );

/*
	if( Bus->->width == 0 )
		FormatReal( Params, 0, 0, 1, Params->DefaultLineWidth, Width, sizeof Width );
	else
		FormatReal( Params, 0, 0, 1, Bus->width, Width, sizeof Width );
*/

	OutputToFile( Params, Level, "(bus (pts (xy %s %s) (xy %s %s)) (stroke (width 0) (type default)))\n", x1, y1, x2, y2 );
	return 0;
	}
/*=============================================================================*/
static int OutputBusEntry( const parameters_t *Params, unsigned Level, pcad_busentry_t *BusEntry )
	{
	static const char	*Orientations[][4]	=
		{
			{ "0.0 2.54",	"0.0 -2.54",	"2.54 0.0",		"-2.54 0.0", },
			{ "1.27 2.54",	"-1.27 -2.54",	"2.54 -1.27",	"-2.54 1.27", },
			{ "-1.27 2.54",	"1.27 -2.54",	"2.54 1.27",	"-2.54 -1.27", }
		};
	char	x[32], y[32] /*, Width[32]*/;

	FormatReal( Params, 0, Params->OriginX, Params->ScaleX, BusEntry->point.x, x, sizeof x );
	FormatReal( Params, 0, Params->OriginY, Params->ScaleY, BusEntry->point.y, y, sizeof y );

	if( Params->StraightBusEntries )
		OutputToFile( Params, Level, "(bus_entry (at %s %s) (size %s) (stroke (width 0) (type default)))\n", x, y, Orientations[0][BusEntry->orient % LENGTH( Orientations[0] )]);
	else switch( BusEntry->style )
		{
		case PCAD_ENDSTYLE_TWOLEADS:
			OutputToFile( Params, Level, "(bus_entry (at %s %s) (size %s) (stroke (width 0) (type default)))\n", x, y, Orientations[1][BusEntry->orient % LENGTH( Orientations[0] )]);
		case PCAD_ENDSTYLE_RIGHTLEAD:
			OutputToFile( Params, Level, "(bus_entry (at %s %s) (size %s) (stroke (width 0) (type default)))\n", x, y, Orientations[2][BusEntry->orient % LENGTH( Orientations[0] )]);
			break;
		case PCAD_ENDSTYLE_NONE:
		case PCAD_ENDSTYLE_LEFTLEAD:
			OutputToFile( Params, Level, "(bus_entry (at %s %s) (size %s) (stroke (width 0) (type default)))\n", x, y, Orientations[1][BusEntry->orient % LENGTH( Orientations[0] )]);
			break;
		}

	return 0;
	}
/*=============================================================================*/
static int OutputSchematic( const parameters_t *Params, unsigned Level, const pcad_schematicfile_t *Schematic, pcad_sheet_t *Sheet )
	{
	int i;

	for( i = 0; i < Sheet->numsymbols; i++ )
		OutputSymbol( Params, Level, Schematic, Sheet->viosymbols[i] );

	for( i = 0; i < Sheet->numbuses; i++ )
		OutputBus( Params, Level, Sheet->viobuses[i] );

	for( i = 0; i < Sheet->numwires; i++ )
		OutputWire( Params, Level, Sheet, Sheet->viowires[i] );

	for( i = 0; i < Sheet->numbusentries; i++ )
		OutputBusEntry( Params, Level, Sheet->viobusentries[i] );

	for( i = 0; i < Sheet->numjunctions; i++ )
		OutputJunction( Params, Level, Sheet->viojunctions[i] );

	for( i = 0; i < Sheet->numports; i++ )
		OutputPort( Params, Level, Sheet->vioports[i] );

	for( i = 0; i < Sheet->numlines; i++ )
		OutputLine( Params, Level, Sheet->violines[i] );

	for( i = 0; i < Sheet->numtexts; i++ )
		OutputText( Params, Level, Sheet->viotexts[i] );

	for( i = 0; i < Sheet->numtriplepointarcs; i++ )
		OutputArc( Params, Level, Sheet->viotriplepointarcs[i] );

	for( i = 0; i < Sheet->numpolys; i++ )
		OutputPolygon( Params, Level, Sheet->viopolys[i] );

	return 0;
	}
/*=============================================================================*/
typedef struct
	{
	const char			*Name;
	pcad_dimmension_t	Width;
	pcad_dimmension_t	Height;
	} papersize_t;
/*=============================================================================*/
static papersize_t MetricPapers[]	=
	{
	[0] = { "A0",		1189000000,	841000000 },
	[1] = { "A1",		 841000000,	594000000 },
	[2] = { "A2",		 594000000,	420000000 },
	[3] = { "A3",		 420000000,	297000000 },
	[4] = { "A4",		 297000000,	210000000 },
	[5] = { "A5",		 210000000,	148000000 }
	};
/*=============================================================================*/
static papersize_t AmericanPapers[]	=
	{
	[0] = { "E",		1117600000,	863600000 },
	[1] = { "D",		 863600000,	558800000 },
	[2] = { "C",		 558800000,	431800000 },
	[3] = { "B",		 431800000,	279400000 },
	[4] = { "A",		 279400000,	215900000 }
	};
/*=============================================================================*/
static papersize_t OtherPapers[]	=
	{
	[0] = { "USLedger",	 432000000,	279000000 },
	[1] = { "USLegal",	 356000000,	216000000 },
	[2] = { "USLetter",	 279000000,	216000000 },
	};
/*=============================================================================*/
static int FindPaperSize( parameters_t *Params, pcad_dimmension_t Width, pcad_dimmension_t Height, char *Buffer )
	{
	char	BufferWidth[32], BufferHeight[32];
	int		i;

	for( i = 0; i < LENGTH( MetricPapers ); i++ )
		if( MetricPapers[i].Width == Width && MetricPapers[i].Height == Height )
			{
			sprintf( Buffer, "(Paper \"%s\")", MetricPapers[i].Name );
			return 1;
			}

	for( i = 0; i < LENGTH( AmericanPapers ); i++ )
		if( AmericanPapers[i].Width == Width && AmericanPapers[i].Height == Height )
			{
			sprintf( Buffer, "(Paper \"%s\")", AmericanPapers[i].Name );
			return 1;
			}

	for( i = 0; i < LENGTH( OtherPapers ); i++ )
		if( OtherPapers[i].Width == Width && OtherPapers[i].Height == Height )
			{
			sprintf( Buffer, "(Paper \"%s\")", OtherPapers[i].Name );
			return 1;
			}

	if(( Width > MetricPapers[0].Width || Height > MetricPapers[0].Height ) && ( Width > AmericanPapers[0].Width || Height > AmericanPapers[0].Height ))
		{
		FormatReal( Params, 0, 0, 1, Width, BufferWidth, sizeof BufferWidth );
		FormatReal( Params, 0, 0, 1, Height, BufferHeight, sizeof BufferHeight );
		sprintf( Buffer, "(Paper \"User\" %s %s)", BufferWidth, BufferHeight );
		return 1;
		}
/*
	if( Width < MetricPapers[5].Width || Height < MetricPapers[5].Height )
		{
		sprintf( Buffer, "(Paper \"User\" %s %s)", FormatReal( Params, 0, 0, 1, Width, BufferWidth, sizeof BufferWidth ), FormatReal( Params, 0, 0, 1, Height, BufferHeight, sizeof BufferHeight ));
		return 1;
		}
*/
	for( i = LENGTH( MetricPapers ) - 1; i >= 0; i-- )
		{
		if( i < LENGTH( MetricPapers ) && Width < MetricPapers[i].Width && Height < MetricPapers[i]. Height )
			{
			sprintf( Buffer, "(paper \"%s\")", MetricPapers[i].Name );
			return 1;
			}
		if( i < LENGTH( AmericanPapers ) && Width < AmericanPapers[i].Width && Height < AmericanPapers[i]. Height )
			{
			sprintf( Buffer, "(paper \"%s\")", AmericanPapers[i].Name );
			return 1;
			}
		if( i < LENGTH( OtherPapers ) && Width < OtherPapers[i].Width && Height < OtherPapers[i]. Height )
			{
			sprintf( Buffer, "(paper \"%s\")", OtherPapers[i].Name );
			return 1;
			}
		}

	return 0;
	}
/*=============================================================================*/
static void FindTitleExtents( const pcad_titlesheet_t *TitleSheet, pcad_extent_t *Extent )
	{
	pcad_dimmension_t	Left	= LONG_MAX, Right = LONG_MIN, Top	= LONG_MIN, Bottom = LONG_MAX;
	int					i;

	for( i = 0; i < TitleSheet->numlines; i++ )
		{
		pcad_line_t	*pLine = TitleSheet->violines[i];
		if( pLine->pt1.x < Left )
			Left	= pLine->pt1.x;
		if( pLine->pt2.x < Left )
			Left	= pLine->pt2.x;
		if( pLine->pt1.x > Right )
			Right	= pLine->pt1.x;
		if( pLine->pt2.x > Right )
			Right	= pLine->pt2.x;
		if( pLine->pt1.y < Bottom )
			Bottom	= pLine->pt1.y;
		if( pLine->pt2.y < Bottom )
			Bottom	= pLine->pt2.y;
		if( pLine->pt1.y > Top )
			Top	= pLine->pt1.y;
		if( pLine->pt2.y > Top )
			Top	= pLine->pt2.y;
		}
	Extent->extentx = Right - Left;
	Extent->extenty = Top - Bottom;
	}
/*=============================================================================*/
#define ALIGNMENT_ROUNDING	2540000
/*=============================================================================*/
static int OutputSheet( parameters_t *Params, unsigned Level, const pcad_schematicfile_t *Schematic, pcad_sheet_t *Sheet )
	{
	char					Buffer[32];
	pcad_dimmension_t		OriginX, OriginY;
	pcad_extent_t			TitleExtent = { .extentx = 0, .extenty = 0 };
	const pcad_titlesheet_t	*TitleSheet;

	/* The sheet has its own border settings... */
	if( Sheet->titlesheet.border.width != 0 && Sheet->titlesheet.border.height != 0 )
		/* ...let's use it. */
		TitleSheet	= &Sheet->titlesheet;
	/* The sheet doesn't have private border settings.. */
	else
		/* ...let's use the global settings. */
		TitleSheet	= &Schematic->schematicdesign.titlesheet;

	/* Let's see whether we have a title block. */
	FindTitleExtents( TitleSheet, &TitleExtent );
	/* There is no title block definition... */
	if( TitleExtent.extentx == 0 || TitleExtent.extenty == 0 )
		{
		OriginX = TitleSheet->border.offset.offsetx;
		OriginY = TitleSheet->border.height;
		}
	else
		{
		OriginX = TitleSheet->border.width - TitleExtent.extentx;
		OriginY = TitleExtent.extenty + 2 * TitleSheet->border.offset.offsety;
		}

	FindPaperSize( Params, TitleExtent.extentx, TitleExtent.extenty, Buffer );

	OriginX = ( OriginX / ALIGNMENT_ROUNDING ) * ALIGNMENT_ROUNDING;
	OriginY = ( OriginY / ALIGNMENT_ROUNDING ) * ALIGNMENT_ROUNDING;

	OutputToFile( Params, Level, "(kicad_sch\n" );
	OutputToFile( Params, Level + 1, "(version 20231120)\n" );
	OutputToFile( Params, Level + 1, "(generator \"eeschema\")\n" );
	OutputToFile( Params, Level + 1, "(generator_version \"8.0\")\n" );
	OutputToFile( Params, Level + 1, "(uuid \"5eab5f85-83b1-44f7-98c0-9af69d3534bc\")\n" );

	OutputToFile( Params, Level + 1, "%s\n", Buffer );

	Params->ScaleY	= -1;
	Params->OriginY = OriginY;
//	if( Sheet->titlesheet.border.height > 0 )
//		Params->Height = (( Sheet->titlesheet.border.height + Sheet->titlesheet.border.offset.offsety + 2540000 - 1 ) / 2540000 ) * 2540000;
//	else
//		Params->Height = (( Schematic->schematicdesign.schdesignheader.workspacesize.extenty + 2540000 - 1 ) / 2540000 ) * 2540000;

	Params->ScaleX	=  1;
	Params->OriginX = OriginX;
//	if( Sheet->titlesheet.border.width > 0 )
//		Params->OriginX = Sheet->titlesheet.border.width;
//	else
//		Params->OriginX = 0;

	OutputLibrary( Params, Level + 1, Schematic, &Schematic->library );
	OutputSchematic( Params, Level + 1, Schematic, Sheet );

	OutputToFile( Params, Level, ")\n" );

	return 0;
	}
/*=============================================================================*/
void SplitPath( const char *pFullPath, char *pPath, char *pName, char *pExt );
/*=============================================================================*/
int OutputKiCAD( cookie_t *Cookie, pcad_schematicfile_t *PCADSchematic, const char *pName )
	{
	parameters_t	Params;
	char			Path[256], Name[256], Ext[256], OutPath[256], BkpPath[256], TmpPath[256];
	char			SheetName[256], *p;
	int				i;

	Params.Cookie				= Cookie;
	Params.DefaultLineWidth		= 254000;
	Params.PolygonBorderWidth	=    100;
	Params.PolygonExtraVertex	=      1;
	Params.StraightBusEntries	=	   0;

	SplitPath( pName, Path, Name, Ext );
	if( stricmp( Ext, "" ) == 0 )
		strcpy( Ext, ".kicad_sch" );

	strcat( Path, Name );

	for( i = 0; i < PCADSchematic->schematicdesign.numsheets; i++ )
		{
		strcpy( OutPath, Path );
		strcat( OutPath, "-" );
		strcpy( SheetName, PCADSchematic->schematicdesign.viosheets[i]->name );
		for( p = SheetName; ( p = strchr( p, '/' )) != NULL; p = SheetName )
			*p	= '_';
		for( p = SheetName; ( p = strchr( p, '\\' )) != NULL; p = SheetName )
			*p	= '_';
		for( p = SheetName; ( p = strchr( p, ':' )) != NULL; p = SheetName )
			*p	= '_';
		for( p = SheetName; ( p = strchr( p, '<' )) != NULL; p = SheetName )
			*p	= '_';
		for( p = SheetName; ( p = strchr( p, '>' )) != NULL; p = SheetName )
			*p	= '_';
		for( p = SheetName; ( p = strchr( p, '?' )) != NULL; p = SheetName )
			*p	= '_';
		for( p = SheetName; ( p = strchr( p, '*' )) != NULL; p = SheetName )
			*p	= '_';
		for( p = SheetName; ( p = strchr( p, '|' )) != NULL; p = SheetName )
			*p	= '_';
		for( p = SheetName; ( p = strchr( p, '"' )) != NULL; p = SheetName )
			*p	= '_';

		Params.SheetName	= SheetName;

		strcat( OutPath, SheetName );

		strcpy( TmpPath, OutPath );
		strcat( TmpPath, ".cvt_tmp" );

		remove( TmpPath );

		if(( Params.File = fopen( TmpPath, "wb" )) == NULL )
			Error( Cookie, -1, "Error creating file" );

		OutputSheet( &Params, 0, PCADSchematic, PCADSchematic->schematicdesign.viosheets[i] );

		fclose( Params.File );

		strcat( OutPath, Ext );

		strcpy( BkpPath, OutPath );
		strcat( BkpPath, ".cvt_bak" );

		remove( BkpPath );
		rename( OutPath, BkpPath );
		rename( TmpPath, OutPath );
		}

	return 0;
	}
/*=============================================================================*/
