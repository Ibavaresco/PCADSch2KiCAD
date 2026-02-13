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
#include <string.h>
#include <ctype.h>
#include "Parser.h"
#include "PCADStructs.h"
#include "PCADProcessSchematic.h"
/*===========================================================================*/
static inline pcad_dimmension_t __attribute__((always_inline)) min( pcad_dimmension_t a, pcad_dimmension_t b )
	{
	return a < b ? a : b;
	}
/*===========================================================================*/
static inline pcad_dimmension_t __attribute__((always_inline)) max( pcad_dimmension_t a, pcad_dimmension_t b )
	{
	return a > b ? a : b;
	}
/*===========================================================================*/
typedef struct
	{
	pcad_dimmension_t	Bottom;
	pcad_dimmension_t	Left;
	pcad_dimmension_t	Top;
	pcad_dimmension_t	Right;
	} boundingrect_t;
/*===========================================================================*/
static boundingrect_t *CalculateBoundingRectangle( boundingrect_t *Rect, const pcad_point_t *p1, const pcad_point_t *p2 )
	{
	Rect->Bottom	= min( p1->y, p2->y );
	Rect->Left		= min( p1->x, p2->x );
	Rect->Top		= max( p1->y, p2->y );
	Rect->Right		= max( p1->x, p2->x );

	return Rect;
	}
/*===========================================================================*/
static boundingrect_t *BoundingRectangleAdd( boundingrect_t *Rect, const pcad_point_t *pt )
	{
	if( pt->y < Rect->Bottom )
		Rect->Bottom	= pt->y;
	if( pt->y > Rect->Top )
		Rect->Top		= pt->y;
	if( pt->x < Rect->Left )
		Rect->Left		= pt->x;
	if( pt->x > Rect->Right )
		Rect->Right		= pt->x;

	return Rect;
	}
/*===========================================================================*/
static int CompareBoundingRectangleTLtoBR( const boundingrect_t *RectA, const boundingrect_t *RectB )
	{
	if( RectA->Top != RectB->Top )
		return RectB->Top - RectA->Top;
	if( RectA->Left != RectB->Left )
		return RectA->Left - RectB->Left;
	if( RectA->Bottom != RectB->Bottom )
		return RectB->Bottom - RectA->Bottom;
	//if( RectA->Right != RectB->Right )
		return RectA->Right - RectB->Right;
	}
/*===========================================================================*/
static int CompareNames( const char *a, const char *b )
	{
	char	Num1[256], Num2[256];
	int		Len1, Len2, Zeros1, Zeros2, i;

	if( a == NULL || b == NULL )
		return a == b ? 0 : a == NULL ? -1 : 1;

	while( 1 )
		{
		for( ; *a != '\0' && *b != '\0' && ( !isdigit( *a ) || !isdigit( *b )); a++, b++ )
			if( *a != *b )
				return *a - *b;

		/* This is the ending condition, '\0' is not a digit, so unless we have numbers
			in both strings in this position, the loop will end here. */
		if( !isdigit( *a ) || !isdigit( *b ))
			return *a - *b;

		/* Copy all the numeric sequence to Num1. */
		for( Len1 = 0; isdigit( *a ) && Len1 < sizeof Num1 - 2; Len1++, a++ )
			Num1[Len1]	= *a;
		Num1[Len1]	= '\0';
		/* Count the leading zeros in Num1. */
		for( Zeros1 = 0; Num1[Zeros1] == '0' ; Zeros1++ )
			{}

		/* Copy all the numeric sequence to Num2. */
		for( Len2 = 0; isdigit( *b ) && Len2 < sizeof Num2 - 2; Len2++, b++ )
			Num2[Len2]	= *b;
		Num2[Len2]	= '\0';
		/* Count the leading zeros in Num2. */
		for( Zeros2 = 0; Num2[Zeros2] == '0' ; Zeros2++ )
			{}

		/* After removing the leading zeros, the longer number is greater. */
		if( Len1 - Zeros1 != Len2 - Zeros2 )
			return ( Len1 - Zeros1 ) - ( Len2 - Zeros2 );

		for( i = 0; i < Len1 - Zeros1; i++ )
			if( Num1[i+Zeros1] != Num2[i+Zeros2] )
				return Num1[i+Zeros1] - Num2[i+Zeros2];

		/* If we reached here, both numbers are identical, except for the leading zeros.
			The number with less leading zeros comes first. */
		if( Zeros1 != Zeros2 )
			return Zeros1 - Zeros2;
		}
	}
/*===========================================================================*/
static int CompareWires( const void *a, const void *b )
	{
	const pcad_wire_t	*pa	= *(const pcad_wire_t * const *)a;
	const pcad_wire_t	*pb	= *(const pcad_wire_t * const *)b;
	boundingrect_t		RectA, RectB;
	int					Result;

	Result	= CompareNames( pa->netnameref, pb->netnameref );
	if( Result != 0 )
		return Result;

	CalculateBoundingRectangle( &RectA, &pa->pt1, &pa->pt2 );
	CalculateBoundingRectangle( &RectB, &pb->pt1, &pb->pt2 );

	return CompareBoundingRectangleTLtoBR( &RectA, &RectB );
	}
/*===========================================================================*/
static int CompareBuses( const void *a, const void *b )
	{
	const pcad_bus_t	*pa	= *(const pcad_bus_t * const *)a;
	const pcad_bus_t	*pb	= *(const pcad_bus_t * const *)b;
	boundingrect_t		RectA, RectB;
	int					Result;

	Result	= CompareNames( pa->name, pb->name );
	if( Result != 0 )
		return Result;

	CalculateBoundingRectangle( &RectA, &pa->pt1, &pa->pt2 );
	CalculateBoundingRectangle( &RectB, &pb->pt1, &pb->pt2 );

	return CompareBoundingRectangleTLtoBR( &RectA, &RectB );
	}
/*===========================================================================*/
static int CompareSymbols( const void *a, const void *b )
	{
	const pcad_symbol_t *pa = *(const pcad_symbol_t * const *)a;
	const pcad_symbol_t *pb = *(const pcad_symbol_t * const *)b;
	int					Result;

	Result	= CompareNames( pa->refdesref, pb->refdesref );
	if( Result != 0 )
		return Result;
	if( pa->partnum != pb->partnum )
		return pa->partnum - pb->partnum;
	return  CompareNames( pa->symbolref, pb->symbolref );
	}
/*===========================================================================*/
static int CompareJunctions( const void *a, const void *b )
	{
	const pcad_junction_t	*pa = *(const pcad_junction_t * const *)a;
	const pcad_junction_t	*pb = *(const pcad_junction_t * const *)b;
	int						Result;

	Result	= CompareNames( pa->netnameref, pb->netnameref );
	if( Result != 0 )
		return Result;

	return pa->point.y != pb->point.y ? pb->point.y - pa->point.y : pa->point.x - pb->point.x;
	}
/*===========================================================================*/
static int CompareBusEntries( const void *a, const void *b )
	{
	const pcad_busentry_t	*pa = *(const pcad_busentry_t * const *)a;
	const pcad_busentry_t	*pb = *(const pcad_busentry_t * const *)b;
	int						Result;

	Result	= CompareNames( pa->busnameref, pb->busnameref );
	if( Result != 0 )
		return Result;

	Result	= pa->point.y != pb->point.y ? pb->point.y - pa->point.y : pa->point.x - pb->point.x;
	if( Result != 0 )
		return Result;

	return pa->orient - pb->orient;
	}
/*===========================================================================*/
static int ComparePorts( const void *a, const void *b )
	{
	const pcad_port_t	*pa = *(const pcad_port_t * const *)a;
	const pcad_port_t	*pb = *(const pcad_port_t * const *)b;
	int					Result;

	Result	= CompareNames( pa->netnameref, pb->netnameref );
	if( Result != 0 )
		return Result;

	Result	= pa->point.y != pb->point.y ? pb->point.y - pa->point.y : pa->point.x - pb->point.x;
	if( Result != 0 )
		return Result;

	return pa->rotation - pb->rotation;
	}
/*===========================================================================*/
static int CompareTexts( const void *a, const void *b )
	{
	const pcad_text_t	*pa = *(const pcad_text_t * const *)a;
	const pcad_text_t	*pb = *(const pcad_text_t * const *)b;
	int					Result;

	Result	= CompareNames( pa->value, pb->value );
	if( Result != 0 )
		return Result;

	Result	= pa->point.y != pb->point.y ? pb->point.y - pa->point.y : pa->point.x - pb->point.x;
	if( Result != 0 )
		return Result;

	Result	= pa->justify - pb->justify;
	if( Result != 0 )
		return Result;

	return pa->rotation - pb->rotation;
	}
/*===========================================================================*/
static int CompareArcs( const void *a, const void *b )
	{
	const pcad_triplepointarc_t *pa = *(const pcad_triplepointarc_t * const *)a;
	const pcad_triplepointarc_t *pb = *(const pcad_triplepointarc_t * const *)b;
	int							Result;

	Result	= pa->point1.y != pb->point1.y ? pb->point1.y - pa->point1.y : pa->point1.x - pb->point1.x;
	if( Result != 0 )
		return Result;

	Result	= pa->point2.y != pb->point2.y ? pb->point2.y - pa->point2.y : pa->point2.x - pb->point2.x;
	if( Result != 0 )
		return Result;

	return pa->point3.y != pb->point3.y ? pb->point3.y - pa->point3.y : pa->point3.x - pb->point3.x;
	}
/*===========================================================================*/
static int CompareLines( const void *a, const void *b )
	{
	const pcad_line_t	*pa = *(const pcad_line_t * const *)a;
	const pcad_line_t	*pb = *(const pcad_line_t * const *)b;
	boundingrect_t		RectA, RectB;

	CalculateBoundingRectangle( &RectA, &pa->pt1, &pa->pt2 );
	CalculateBoundingRectangle( &RectB, &pb->pt1, &pb->pt2 );

	return CompareBoundingRectangleTLtoBR( &RectA, &RectB );
	}
/*===========================================================================*/
static int CompareAttrs( const void *a, const void *b )
	{
	const pcad_attr_t	*pa = *(const pcad_attr_t * const *)a;
	const pcad_attr_t	*pb = *(const pcad_attr_t * const *)b;

	return CompareNames( pa->name, pb->name );
	}
/*===========================================================================*/
static int ComparePolys( const void *a, const void *b )
	{
	const pcad_poly_t	*pa = *(const pcad_poly_t * const *)a;
	const pcad_poly_t	*pb = *(const pcad_poly_t * const *)b;
	boundingrect_t		RectA, RectB;
	int					i;

	CalculateBoundingRectangle( &RectA, pa->viopoints[0], pa->viopoints[0] );
	for( i = 1; i < pa->numpoints; i++ )
		BoundingRectangleAdd( &RectA, pa->viopoints[i] );

	CalculateBoundingRectangle( &RectB, pb->viopoints[0], pb->viopoints[0] );
	for( i = 1; i < pb->numpoints; i++ )
		BoundingRectangleAdd( &RectB, pb->viopoints[i] );

	i	= CompareBoundingRectangleTLtoBR( &RectA, &RectB );
	if( i != 0 )
		return i;

	return pa->numpoints - pb->numpoints;
	}
/*===========================================================================*/
static int ComparePins( const void *a, const void *b )
	{
	const pcad_pin_t	*pa = *(const pcad_pin_t * const *)a;
	const pcad_pin_t	*pb = *(const pcad_pin_t * const *)b;

	if( pa->pinnum != pb->pinnum)
		return pa->pinnum - pb->pinnum;

	if( pa->defaultpindes != NULL && pb->defaultpindes != NULL )
		{
		int	Result;
		Result	= CompareNames( pa->defaultpindes, pb->defaultpindes );
		if( Result != 0 )
			return Result;
		}
	else if( pa->defaultpindes != NULL )
		return 1;
	else if( pb->defaultpindes != NULL )
		return -1;

	return pa->point.y != pb->point.y ? pb->point.y - pa->point.y : pa->point.x - pb->point.x;
	}
/*===========================================================================*/
static int CompareIEEESymbols( const void *a, const void *b )
	{
	const pcad_ieeesymbol_t *pa = *(const pcad_ieeesymbol_t * const *)a;
	const pcad_ieeesymbol_t *pb = *(const pcad_ieeesymbol_t * const *)b;

	return pa->point.y != pb->point.y ? pb->point.y - pa->point.y : pa->point.x - pb->point.x;
	}
/*===========================================================================*/
static int CompareFields( const void *a, const void *b )
	{
	const pcad_field_t	*pa = *(const pcad_field_t * const *)a;
	const pcad_field_t	*pb = *(const pcad_field_t * const *)b;

	return strcmp( pa->name, pb->name );
	}
/*===========================================================================*/
static int CompareRefPoints( const void *a, const void *b )
	{
	const pcad_refpoint_t	*pa = *(const pcad_refpoint_t * const *)a;
	const pcad_refpoint_t	*pb = *(const pcad_refpoint_t * const *)b;

	return pa->point.y != pb->point.y ? pb->point.y - pa->point.y : pa->point.x - pb->point.x;
	}
/*===========================================================================*/
static int ProcessPoly( cookie_t *Cookie, pcad_poly_t *Poly )
	{
	int				i;

	/*------------------------------------------------------------------------*/

	pcad_point_t *pPoint;

	for( i = 0, pPoint = Poly->firstpoint; pPoint != NULL; i++, pPoint = pPoint->next )
		{}
	Poly->numpoints	= i;
	if( i > 0 )
		{
		Poly->viopoints = Allocate( Cookie, i * sizeof( pcad_point_t* ));
		for( i = 0, pPoint = Poly->firstpoint; i < Poly->numpoints && pPoint != NULL; i++, pPoint = pPoint->next )
			Poly->viopoints[i]	= pPoint;
		}

	return 0;
	}
/*===========================================================================*/
static int ProcessTitleSheet( cookie_t *Cookie, pcad_titlesheet_t *TitleSheet )
	{
	int				i;

	pcad_line_t	*pLine;

	for( i = 0, pLine = TitleSheet->firstline; pLine != NULL; i++, pLine = pLine->next )
		{}
	TitleSheet->numlines	= i;
	if( i > 0 )
		{
		TitleSheet->violines	= Allocate( Cookie, i * sizeof( pcad_line_t* ));
		for( i = 0, pLine = TitleSheet->firstline; i < TitleSheet->numlines && pLine != NULL; i++, pLine = pLine->next )
			TitleSheet->violines[i]	= pLine;
		if( Cookie->Sort )
			qsort( TitleSheet->violines, TitleSheet->numlines, sizeof( pcad_line_t* ), CompareLines );
		}

//	for( i = 0; i < TitleSheet->numlines; i++ )
//		ProcessLine( Cookie, TitleSheet->violines[i] );

	/*------------------------------------------------------------------------*/

	pcad_text_t *pText;

	for( i = 0, pText = TitleSheet->firsttext; pText != NULL; i++, pText = pText->next )
		{}
	TitleSheet->numtexts	= i;
	if( i > 0 )
		{
		TitleSheet->viotexts	= Allocate( Cookie, i * sizeof( pcad_text_t* ));
		for( i = 0, pText = TitleSheet->firsttext; i < TitleSheet->numtexts && pText != NULL; i++, pText = pText->next )
			TitleSheet->viotexts[i]	= pText;
		if( Cookie->Sort )
			qsort( TitleSheet->viotexts, TitleSheet->numtexts, sizeof( pcad_text_t* ), CompareTexts );
		}

//	for( i = 0; i < TitleSheet->numtexts; i++ )
//		ProcessText( Cookie, TitleSheet->viotexts[i] );

	/*------------------------------------------------------------------------*/
	/*------------------------------------------------------------------------*/

	return 0;
	}
/*===========================================================================*/
static int ProcessSymbol( cookie_t *Cookie, pcad_symbol_t *Symbol )
	{
	int				i;

	pcad_attr_t *pAttr;

	for( i = 0, pAttr = Symbol->firstattr; pAttr != NULL; i++, pAttr = pAttr->next )
		{}
	Symbol->numattrs	= i;
	if( i > 0 )
		{
		Symbol->vioattrs	= Allocate( Cookie, i * sizeof( pcad_attr_t* ));
		for( i = 0, pAttr = Symbol->firstattr; i < Symbol->numattrs && pAttr != NULL; i++, pAttr = pAttr->next )
			Symbol->vioattrs[i]	= pAttr;
		if( Cookie->Sort )
			qsort( Symbol->vioattrs, Symbol->numattrs, sizeof( pcad_attr_t* ), CompareAttrs );
		}

//	for( i = 0; i < Symbol->numattrs; i++ )
//		ProcessAttr( Cookie, Symbol->vioattrs[i] );

	return 0;
	}
/*===========================================================================*/
static int ProcessSheet( cookie_t *Cookie, pcad_sheet_t *Sheet )
	{
	int				i;

	/*------------------------------------------------------------------------*/
	ProcessTitleSheet( Cookie, &Sheet->titlesheet );
	/*------------------------------------------------------------------------*/

	pcad_wire_t *pWire;

	for( i = 0, pWire = Sheet->firstwire; pWire != NULL; i++, pWire = pWire->next )
		{}
	Sheet->numwires	= i;
	if( i > 0 )
		{
		Sheet->viowires	= Allocate( Cookie, i * sizeof( pcad_wire_t* ));
		for( i = 0, pWire = Sheet->firstwire; i < Sheet->numwires && pWire != NULL; i++, pWire = pWire->next )
			Sheet->viowires[i]	= pWire;
		if( Cookie->Sort )
			qsort( Sheet->viowires, Sheet->numwires, sizeof( pcad_wire_t* ), CompareWires );
		}

//	for( i = 0; i < Sheet->numwires; i++ )
//		ProcessWire( Cookie, Sheet->viowires[i] );

	/*------------------------------------------------------------------------*/

	pcad_bus_t	*pBus;

	for( i = 0, pBus = Sheet->firstbus; pBus != NULL; i++, pBus = pBus->next )
		{}
	Sheet->numbuses = i;
	if( i > 0 )
		{
		Sheet->viobuses	= Allocate( Cookie, i * sizeof( pcad_bus_t* ));
		for( i = 0, pBus = Sheet->firstbus; i < Sheet->numbuses && pBus != NULL; i++, pBus = pBus->next )
			Sheet->viobuses[i]	= pBus;
		if( Cookie->Sort )
			qsort( Sheet->viobuses, Sheet->numbuses, sizeof( pcad_bus_t* ), CompareBuses );
		}

//	for( i = 0; i < Sheet->numbuses; i++ )
//		ProcessBus( Cookie, Sheet->viobuses[i] );

	/*------------------------------------------------------------------------*/

	pcad_busentry_t	*pBusEntry;

	for( i = 0, pBusEntry = Sheet->firstbusentry; pBusEntry != NULL; i++, pBusEntry = pBusEntry->next )
		{}
	Sheet->numbusentries = i;
	if( i > 0 )
		{
		Sheet->viobusentries	= Allocate( Cookie, i * sizeof( pcad_busentry_t* ));
		for( i = 0, pBusEntry = Sheet->firstbusentry; i < Sheet->numbusentries && pBusEntry != NULL; i++, pBusEntry = pBusEntry->next )
			Sheet->viobusentries[i]	= pBusEntry;
		if( Cookie->Sort )
			qsort( Sheet->viobusentries, Sheet->numbusentries, sizeof( pcad_busentry_t* ), CompareBusEntries );
		}

//	for( i = 0; i < Sheet->numbusentries; i++ )
//		ProcessBusEntry( Cookie, Sheet->viobusentries[i] );

	/*------------------------------------------------------------------------*/

	pcad_symbol_t *pSymbol;

	for( i = 0, pSymbol = Sheet->firstsymbol; pSymbol != NULL; i++, pSymbol = pSymbol->next )
		{}
	Sheet->numsymbols	= i;
	if( i > 0 )
		{
		Sheet->viosymbols	= Allocate( Cookie, i * sizeof( pcad_symbol_t* ));
		for( i = 0, pSymbol = Sheet->firstsymbol; i < Sheet->numsymbols && pSymbol != NULL; i++, pSymbol = pSymbol->next )
			Sheet->viosymbols[i]	= pSymbol;
		if( Cookie->Sort )
			qsort( Sheet->viosymbols, Sheet->numsymbols, sizeof( pcad_symbol_t* ), CompareSymbols );
		}

	for( i = 0; i < Sheet->numsymbols; i++ )
		{
//		fprintf( stderr, "\tProcessing symbol %u: ", i );
//		fprintf( stderr, "%s\n", Sheet->viosymbols[i]->refdesref );
		ProcessSymbol( Cookie, Sheet->viosymbols[i] );
		}

	/*------------------------------------------------------------------------*/

	pcad_junction_t *pJunction;

	for( i = 0, pJunction = Sheet->firstjunction; pJunction != NULL; i++, pJunction = pJunction->next )
		{}
	Sheet->numjunctions	= i;
	if( i > 0 )
		{
		Sheet->viojunctions	= Allocate( Cookie, i * sizeof( pcad_junction_t* ));
		for( i = 0, pJunction = Sheet->firstjunction; i < Sheet->numjunctions && pJunction != NULL; i++, pJunction = pJunction->next )
			Sheet->viojunctions[i]	= pJunction;
		if( Cookie->Sort )
			qsort( Sheet->viojunctions, Sheet->numjunctions, sizeof( pcad_junction_t* ), CompareJunctions );
		}

//	for( i = 0; i < Sheet->numjunctions; i++ )
//		ProcessJunctions( Cookie, Sheet->viojunctions[i] );

	/*------------------------------------------------------------------------*/

	pcad_port_t *pPort;

	for( i = 0, pPort = Sheet->firstport; pPort != NULL; i++, pPort = pPort->next )
		{}
	Sheet->numports	= i;
	if( i > 0 )
		{
		Sheet->vioports	= Allocate( Cookie, i * sizeof( pcad_port_t* ));
		for( i = 0, pPort = Sheet->firstport; i < Sheet->numports && pPort != NULL; i++, pPort = pPort->next )
			Sheet->vioports[i]	= pPort;
		if( Cookie->Sort )
			qsort( Sheet->vioports, Sheet->numports, sizeof( pcad_port_t* ), ComparePorts );
		}

//	for( i = 0; i < Sheet->numports; i++ )
//		ProcessPort( Cookie, Sheet->vioports[i] );

	/*------------------------------------------------------------------------*/

	pcad_text_t *pText;

	for( i = 0, pText = Sheet->firsttext; pText != NULL; i++, pText = pText->next )
		{}
	Sheet->numtexts	= i;
	if( i > 0 )
		{
		Sheet->viotexts	= Allocate( Cookie, i * sizeof( pcad_text_t* ));
		for( i = 0, pText = Sheet->firsttext; i < Sheet->numtexts && pText != NULL; i++, pText = pText->next )
			Sheet->viotexts[i]	= pText;
		if( Cookie->Sort )
			qsort( Sheet->viotexts, Sheet->numtexts, sizeof( pcad_text_t* ), CompareTexts );
		}

//	for( i = 0; i < Sheet->numtexts; i++ )
//		ProcessText( Cookie, Sheet->viotexts[i] );

	/*------------------------------------------------------------------------*/

	pcad_triplepointarc_t *pArc;

	for( i = 0, pArc = Sheet->firsttriplepointarc; pArc != NULL; i++, pArc = pArc->next )
		{}
	Sheet->numtriplepointarcs	= i;
	if( i > 0 )
		{
		Sheet->viotriplepointarcs	= Allocate( Cookie, i * sizeof( pcad_triplepointarc_t* ));
		for( i = 0, pArc = Sheet->firsttriplepointarc; i < Sheet->numtriplepointarcs && pArc != NULL; i++, pArc = pArc->next )
			Sheet->viotriplepointarcs[i]	= pArc;
		if( Cookie->Sort )
			qsort( Sheet->viotriplepointarcs, Sheet->numtriplepointarcs, sizeof( pcad_triplepointarc_t* ), CompareArcs );
		}

//	for( i = 0; i < Sheet->numarcs; i++ )
//		ProcessArc( Cookie, Sheet->viotriplepointarcs[i] );

	/*------------------------------------------------------------------------*/

	pcad_attr_t *pAttr;

	for( i = 0, pAttr = Sheet->firstattr; pAttr != NULL; i++, pAttr = pAttr->next )
		{}
	Sheet->numattrs	= i;
	if( i > 0 )
		{
		Sheet->vioattrs	= Allocate( Cookie, i * sizeof( pcad_attr_t* ));
		for( i = 0, pAttr = Sheet->firstattr; i < Sheet->numattrs && pAttr != NULL; i++, pAttr = pAttr->next )
			Sheet->vioattrs[i]	= pAttr;
		if( Cookie->Sort )
			qsort( Sheet->vioattrs, Sheet->numattrs, sizeof( pcad_attr_t* ), CompareAttrs );
		}

//	for( i = 0; i < Sheet->numattrs; i++ )
//		ProcessAttr( Cookie, Sheet->vioattrs[i] );

	/*------------------------------------------------------------------------*/

	pcad_poly_t *pPoly;

	for( i = 0, pPoly = Sheet->firstpoly; pPoly != NULL; i++, pPoly = pPoly->next )
		{}
	Sheet->numpolys	= i;
	if( i > 0 )
		{
		Sheet->viopolys	= Allocate( Cookie, i * sizeof( pcad_poly_t* ));
		for( i = 0, pPoly = Sheet->firstpoly; i < Sheet->numpolys && pPoly != NULL; i++, pPoly = pPoly->next )
			{
//				fprintf( stderr, "\tProcessing polygon %u\n", i );
			Sheet->viopolys[i]	= pPoly;
			ProcessPoly( Cookie, Sheet->viopolys[i] );
			}
		if( Cookie->Sort )
			qsort( Sheet->viopolys, Sheet->numpolys, sizeof( pcad_poly_t* ), ComparePolys );
		}

	/*------------------------------------------------------------------------*/

	pcad_line_t	*pLine;

	for( i = 0, pLine = Sheet->firstline; pLine != NULL; i++, pLine = pLine->next )
		{}
	Sheet->numlines	= i;
	if( i > 0 )
		{
		Sheet->violines	= Allocate( Cookie, i * sizeof( pcad_line_t* ));
		for( i = 0, pLine = Sheet->firstline; i < Sheet->numlines && pLine != NULL; i++, pLine = pLine->next )
			Sheet->violines[i]	= pLine;
		if( Cookie->Sort )
			qsort( Sheet->violines, Sheet->numlines, sizeof( pcad_line_t* ), CompareLines );
		}

//	for( i = 0; i < Sheet->numlines; i++ )
//		ProcessLine( Cookie, Sheet->violines[i] );

	/*------------------------------------------------------------------------*/

	pcad_pin_t *pPin;

	for( i = 0, pPin = Sheet->firstpin; pPin != NULL; i++, pPin = pPin->next )
		{}
	Sheet->numpins	= i;
	if( i > 0 )
		{
		Sheet->viopins	= Allocate( Cookie, i * sizeof( pcad_pin_t* ));
		for( i = 0, pPin = Sheet->firstpin; i < Sheet->numpins && pPin != NULL; i++, pPin = pPin->next )
			Sheet->viopins[i]	= pPin;
		if( Cookie->Sort )
			qsort( Sheet->viopins, Sheet->numpins, sizeof( pcad_pin_t* ), ComparePins );
		}

//	for( i = 0; i < Sheet->numpins; i++ )
//		ProcessPin( Cookie, Sheet->viopins[i] );

	/*------------------------------------------------------------------------*/

	pcad_ieeesymbol_t *pIEEESymbol;

	for( i = 0, pIEEESymbol = Sheet->firstieeesymbol; pIEEESymbol != NULL; i++, pIEEESymbol = pIEEESymbol->next )
		{}
	Sheet->numieeesymbols	= i;
	if( i > 0 )
		{
		Sheet->vioieeesymbols	= Allocate( Cookie, i * sizeof( pcad_ieeesymbol_t* ));
		for( i = 0, pIEEESymbol = Sheet->firstieeesymbol; i < Sheet->numieeesymbols && pIEEESymbol != NULL; i++, pIEEESymbol = pIEEESymbol->next )
			Sheet->vioieeesymbols[i]	= pIEEESymbol;
		if( Cookie->Sort )
			qsort( Sheet->vioieeesymbols, Sheet->numieeesymbols, sizeof( pcad_ieeesymbol_t* ), CompareIEEESymbols );
		}

//	for( i = 0; i < Sheet->numieeesymbols; i++ )
//		ProcessIEEESymbol( Cookie, Sheet->vioieeesymbols[i] );

	/*------------------------------------------------------------------------*/

	pcad_field_t *pField;

	for( i = 0, pField = Sheet->firstfield; pField != NULL; i++, pField = pField->next )
		{}
	Sheet->numfields	= i;
	if( i > 0 )
		{
		Sheet->viofields	= Allocate( Cookie, i * sizeof( pcad_field_t* ));
		for( i = 0, pField = Sheet->firstfield; i < Sheet->numfields && pField != NULL; i++, pField = pField->next )
			Sheet->viofields[i]	= pField;
		if( Cookie->Sort )
			qsort( Sheet->viofields, Sheet->numfields, sizeof( pcad_field_t* ), CompareFields );
		}

//	for( i = 0; i < Sheet->numfields; i++ )
//		ProcessField( Cookie, Sheet->viofields[i] );

	/*------------------------------------------------------------------------*/

	pcad_refpoint_t *pRefPoint;

	for( i = 0, pRefPoint = Sheet->firstrefpoint; pRefPoint != NULL; i++, pRefPoint = pRefPoint->next )
		{}
	Sheet->numrefpoints	= i;
	if( i > 0 )
		{
		Sheet->viorefpoints	= Allocate( Cookie, i * sizeof( pcad_refpoint_t* ));
		for( i = 0, pRefPoint = Sheet->firstrefpoint; i < Sheet->numrefpoints && pRefPoint != NULL; i++, pRefPoint = pRefPoint->next )
			Sheet->viorefpoints[i]	= pRefPoint;
		if( Cookie->Sort )
			qsort( Sheet->viorefpoints, Sheet->numrefpoints, sizeof( pcad_refpoint_t* ), CompareRefPoints );
		}

//	for( i = 0; i < Sheet->numrefpoints; i++ )
//		ProcessRefPoint( Cookie, Sheet->viorefpoints[i] );

	/*------------------------------------------------------------------------*/

	return 0;
	}
/*===========================================================================*/
/*===========================================================================*/
/*===========================================================================*/
static int CompareFieldDefs( const void *a, const void *b )
	{
	const pcad_fielddef_t	*pa = *(const pcad_fielddef_t * const *)a;
	const pcad_fielddef_t	*pb = *(const pcad_fielddef_t * const *)b;

	return strcmp( pa->name, pb->name );
	}
/*===========================================================================*/
static int CompareNotes( const void *a, const void *b )
	{
	const pcad_note_t	*pa = *(const pcad_note_t * const *)a;
	const pcad_note_t	*pb = *(const pcad_note_t * const *)b;

	return pa->number - pb->number;
	}
/*===========================================================================*/
static int CompareRevisionNotes( const void *a, const void *b )
	{
	const pcad_revisionnote_t	*pa = *(const pcad_revisionnote_t * const *)a;
	const pcad_revisionnote_t	*pb = *(const pcad_revisionnote_t * const *)b;

	return pa->number - pb->number;
	}
/*===========================================================================*/
static int ProcessFieldSet( cookie_t *Cookie, pcad_fieldset_t *FieldSet )
	{
	int				i;

	/*------------------------------------------------------------------------*/

	pcad_fielddef_t *pFieldDef;

	for( i = 0, pFieldDef = FieldSet->firstfielddef; pFieldDef != NULL; i++, pFieldDef = pFieldDef->next )
		{}
	FieldSet->numfielddefs	= i;
	if( i > 0 )
		{
		FieldSet->viofielddefs	= Allocate( Cookie, i * sizeof( pcad_fielddef_t* ));
		for( i = 0, pFieldDef = FieldSet->firstfielddef; i < FieldSet->numfielddefs && pFieldDef != NULL; i++, pFieldDef = pFieldDef->next )
			FieldSet->viofielddefs[i]	= pFieldDef;
		if( Cookie->Sort )
			qsort( FieldSet->viofielddefs, FieldSet->numfielddefs, sizeof( pcad_fielddef_t* ), CompareFieldDefs );
		}

//	for( i = 0; i < FieldSet->numfielddefs; i++ )
//		ProcessFieldDef( Cookie, FieldSet->viofielddefs[i] );

	/*------------------------------------------------------------------------*/

	pcad_note_t *pNote;

	for( i = 0, pNote = FieldSet->firstnote; pNote != NULL; i++, pNote = pNote->next )
		{}
	FieldSet->numnotes	= i;
	if( i > 0 )
		{
		FieldSet->vionotes	= Allocate( Cookie, i * sizeof( pcad_note_t* ));
		for( i = 0, pNote = FieldSet->firstnote; i < FieldSet->numnotes && pNote != NULL; i++, pNote = pNote->next )
			FieldSet->vionotes[i]	= pNote;
		if( Cookie->Sort )
			qsort( FieldSet->vionotes, FieldSet->numnotes, sizeof( pcad_note_t* ), CompareNotes );
		}

//	for( i = 0; i < FieldSet->numnotes; i++ )
//		ProcessNote( Cookie, FieldSet->vionotes[i] );

	/*------------------------------------------------------------------------*/

	pcad_revisionnote_t *pRevisionNote;

	for( i = 0, pRevisionNote = FieldSet->firstrevisionnote; pRevisionNote != NULL; i++, pRevisionNote = pRevisionNote->next )
		{}
	FieldSet->numrevisionnotes	= i;
	if( i > 0 )
		{
		FieldSet->viorevisionnotes	= Allocate( Cookie, i * sizeof( pcad_revisionnote_t* ));
		for( i = 0, pRevisionNote = FieldSet->firstrevisionnote; i < FieldSet->numrevisionnotes && pRevisionNote != NULL; i++, pRevisionNote = pRevisionNote->next )
			FieldSet->viorevisionnotes[i]	= pRevisionNote;
		if( Cookie->Sort )
			qsort( FieldSet->viorevisionnotes, FieldSet->numrevisionnotes, sizeof( pcad_revisionnote_t* ), CompareRevisionNotes );
		}

//	for( i = 0; i < FieldSet->numrevisionnotes; i++ )
//		ProcessRevisionNote( Cookie, FieldSet->viorevisionnotes[i] );

	/*------------------------------------------------------------------------*/

	return 0;
	}
/*===========================================================================*/
/*===========================================================================*/
/*===========================================================================*/
/*===========================================================================*/
#if 0
static int CompareReportFields( const void *a, const void *b )
	{
	const pcad_reportfield_t	*pa = *(const pcad_reportfield_t * const *)a;
	const pcad_reportfield_t	*pb = *(const pcad_reportfield_t * const *)b;

	return strcmp( pa->reportfieldname, pb->reportfieldname );
	}
/*===========================================================================*/
static int CompareReportFieldss( const void *a, const void *b )
	{
	const pcad_reportfields_t	*pa = *(const pcad_reportfields_t * const *)a;
	const pcad_reportfields_t	*pb = *(const pcad_reportfields_t * const *)b;

	if( pa->numreportfields != pb->numreportfields )
		return pa->numreportfields - pb->numreportfields;

	return strcmp( pa->vioreportfields[0]->reportfieldname, pb->vioreportfields[0]->reportfieldname );
	}
#endif
/*===========================================================================*/
static int ProcessReportField( cookie_t *Cookie, pcad_reportfield_t *ReportField )
	{
	pcad_reportfieldcondition_t *pReportFieldCondition;
	int						i;

	for( i = 0, pReportFieldCondition = ReportField->reportfieldconditions.firstreportfieldcondition; pReportFieldCondition != NULL; i++, pReportFieldCondition = pReportFieldCondition->next )
		{}
	ReportField->reportfieldconditions.numreportfieldconditions	= i;
	if( i > 0 )
		{
		ReportField->reportfieldconditions.vioreportfieldconditions	= Allocate( Cookie, i * sizeof( pcad_reportfieldcondition_t* ));
		for( i = 0, pReportFieldCondition = ReportField->reportfieldconditions.firstreportfieldcondition; i < ReportField->reportfieldconditions.numreportfieldconditions && pReportFieldCondition != NULL; i++, pReportFieldCondition = pReportFieldCondition->next )
			ReportField->reportfieldconditions.vioreportfieldconditions[i]	= pReportFieldCondition;
//		if( Cookie->Sort )
//			qsort( ReportField->reportfieldconditions.vioreportfieldconditions, ReportField->reportfieldconditions.numreportfieldconditions, sizeof( pcad_reportfieldcondition_t* ), CompareReportFieldConditions );
		}

//	for( i = 0; i < ReportField->reportfieldconditions.numreportfieldconditions; i++ )
//		ProcessGrid( Cookie, ReportField->reportfieldconditions.vioreportfieldconditions[i] );

	/*------------------------------------------------------------------------*/

	return 0;
	}
/*===========================================================================*/
static int ProcessReportFields( cookie_t *Cookie, pcad_reportfields_t *ReportFields )
	{
	int					i;
	pcad_reportfield_t	*pReportField;

	for( i = 0, pReportField = ReportFields->firstreportfield; pReportField != NULL; i++, pReportField = pReportField->next )
		{}
	ReportFields->numreportfields	= i;
	if( i > 0 )
		{
		ReportFields->vioreportfields	= Allocate( Cookie, i * sizeof( pcad_reportfield_t* ));
		for( i = 0, pReportField = ReportFields->firstreportfield; i < ReportFields->numreportfields && pReportField != NULL; i++, pReportField = pReportField->next )
			ReportFields->vioreportfields[i]	= pReportField;
		//if( Cookie->Sort )
		//	qsort( ReportFields->vioreportfields, ReportFields->numreportfields, sizeof( pcad_reportfield_t* ), CompareReportFields );
		}

	for( i = 0; i < ReportFields->numreportfields; i++ )
		ProcessReportField( Cookie, ReportFields->vioreportfields[i] );

	/*------------------------------------------------------------------------*/

	return 0;
	}
/*===========================================================================*/
static int ProcessReportDefinition( cookie_t *Cookie, pcad_reportdefinition_t *ReportDefinition )
	{
	int					i;
	pcad_reportfields_t *pReportFields;

	for( i = 0, pReportFields = ReportDefinition->reportfieldssections.firstreportfields; pReportFields != NULL; i++, pReportFields = pReportFields->next )
		{}
	ReportDefinition->reportfieldssections.numreportfieldss	= i;
	if( i > 0 )
		{
		ReportDefinition->reportfieldssections.vioreportfieldss	= Allocate( Cookie, i * sizeof( pcad_reportfields_t* ));
		for( i = 0, pReportFields = ReportDefinition->reportfieldssections.firstreportfields; i < ReportDefinition->reportfieldssections.numreportfieldss && pReportFields != NULL; i++, pReportFields = pReportFields->next )
			{
			ReportDefinition->reportfieldssections.vioreportfieldss[i]	= pReportFields;
			ProcessReportFields( Cookie, ReportDefinition->reportfieldssections.vioreportfieldss[i] );
			}
		//if( Cookie->Sort )
		//	qsort( ReportDefinition->reportfieldssections.vioreportfieldss, ReportDefinition->reportfieldssections.numreportfieldss, sizeof( pcad_reportfields_t* ), CompareReportFieldss );
		}

//	for( i = 0; i < ReportDefinition->reportfieldssections.numreportfieldss; i++ )
//		ProcessReportFields( Cookie, ReportDefinition->reportfieldssections.vioreportfieldss[i] );

	/*------------------------------------------------------------------------*/

	return 0;
	}
/*===========================================================================*/
static int CompareGrids( const void *a, const void *b )
	{
	const pcad_grid_t	*pa = *(const pcad_grid_t * const *)a;
	const pcad_grid_t	*pb = *(const pcad_grid_t * const *)b;

	return pa->grid - pb->grid;
	}
/*===========================================================================*/
static int CompareSheets( const void *a, const void *b )
	{
	const pcad_sheet_t	*pa = *(const pcad_sheet_t * const *)a;
	const pcad_sheet_t	*pb = *(const pcad_sheet_t * const *)b;

	return pa->sheetordernum - pb->sheetordernum;
	}
/*===========================================================================*/
static int CompareFieldSets( const void *a, const void *b )
	{
	const pcad_fieldset_t	*pa = *(const pcad_fieldset_t * const *)a;
	const pcad_fieldset_t	*pb = *(const pcad_fieldset_t * const *)b;

	return strcmp( pa->name, pb->name );
	}
/*===========================================================================*/
static int CompareSheetRefs( const void *a, const void *b )
	{
	const pcad_sheetref_t	*pa = *(const pcad_sheetref_t * const *)a;
	const pcad_sheetref_t	*pb = *(const pcad_sheetref_t * const *)b;

	return pa->sheetref - pb->sheetref;
	}
/*===========================================================================*/
#if 0
static int CompareReportDefinitions( const void *a, const void *b )
	{
	const pcad_reportdefinition_t	*pa = *(const pcad_reportdefinition_t * const *)a;
	const pcad_reportdefinition_t	*pb = *(const pcad_reportdefinition_t * const *)b;

	return strcmp( pa->reportname, pb->reportname );
	}
#endif
/*===========================================================================*/
static int ProcessPCADSchematicDesign( cookie_t *Cookie, pcad_schematicdesign_t *SchematicDesign )
	{
	int				i;

	/*------------------------------------------------------------------------*/

	pcad_grid_t	*pGrids;

	for( i = 0, pGrids = SchematicDesign->schdesignheader.griddfns.firstgrid; pGrids != NULL; i++, pGrids = pGrids->next )
		{}
	SchematicDesign->schdesignheader.griddfns.numgrids	= i;
	if( i > 0 )
		{
		SchematicDesign->schdesignheader.griddfns.viogrids	= Allocate( Cookie, i * sizeof( pcad_grid_t* ));
		for( i = 0, pGrids = SchematicDesign->schdesignheader.griddfns.firstgrid; i < SchematicDesign->schdesignheader.griddfns.numgrids && pGrids != NULL; i++, pGrids = pGrids->next )
			SchematicDesign->schdesignheader.griddfns.viogrids[i]	= pGrids;
		if( Cookie->Sort )
			qsort( SchematicDesign->schdesignheader.griddfns.viogrids, SchematicDesign->schdesignheader.griddfns.numgrids, sizeof( pcad_grid_t* ), CompareGrids );
		}

//	for( i = 0; i < SchematicDesign->schdesignheader.griddfns.numgrids; i++ )
//		ProcessGrid( Cookie, SchematicDesign->schdesignheader.griddfns.viogrids[i] );

	/*------------------------------------------------------------------------*/

	pcad_fieldset_t *pFieldSet;

	for( i = 0, pFieldSet = SchematicDesign->schdesignheader.designinfo.firstfieldset; pFieldSet != NULL; i++, pFieldSet = pFieldSet->next )
		{}
	SchematicDesign->schdesignheader.designinfo.numfieldsets	= i;
	if( i > 0 )
		{
		SchematicDesign->schdesignheader.designinfo.viofieldsets	= Allocate( Cookie, i * sizeof( pcad_fieldset_t* ));
		for( i = 0, pFieldSet = SchematicDesign->schdesignheader.designinfo.firstfieldset; i < SchematicDesign->schdesignheader.designinfo.numfieldsets && pFieldSet != NULL; i++, pFieldSet = pFieldSet->next )
			SchematicDesign->schdesignheader.designinfo.viofieldsets[i]	= pFieldSet;
		if( Cookie->Sort )
			qsort( SchematicDesign->schdesignheader.designinfo.viofieldsets, SchematicDesign->schdesignheader.designinfo.numfieldsets, sizeof( pcad_fieldset_t* ), CompareFieldSets );
		}

	for( i = 0; i < SchematicDesign->schdesignheader.designinfo.numfieldsets; i++ )
		ProcessFieldSet( Cookie, SchematicDesign->schdesignheader.designinfo.viofieldsets[i] );

	/*------------------------------------------------------------------------*/

	ProcessTitleSheet( Cookie, &SchematicDesign->titlesheet );

#if 0
	pcad_line_t	*pLine;

	for( i = 0, pLine = SchematicDesign->titlesheet.firstline; pLine != NULL; i++, pLine = pLine->next )
		{}
	SchematicDesign->titlesheet.numlines	= i;
	if( i > 0 )
		{
		SchematicDesign->titlesheet.violines	= Allocate( i * sizeof( pcad_line_t* ));
		for( i = 0, pLine = SchematicDesign->titlesheet.firstline; i < SchematicDesign->titlesheet.numlines && pLine != NULL; i++, pLine = pLine->next )
			SchematicDesign->titlesheet.violines[i]	= pLine;
		if( Cookie->Sort )
			qsort( SchematicDesign->titlesheet.violines, SchematicDesign->titlesheet.numlines, sizeof( pcad_line_t* ), CompareLines );
		}

//	for( i = 0; i < SchematicDesign->titlesheet.numlines; i++ )
//		ProcessLine( Cookie, SchematicDesign->titlesheet.violines[i] );

	/*------------------------------------------------------------------------*/

	pcad_text_t *pText;

	for( i = 0, pText = SchematicDesign->titlesheet.firsttext; pText != NULL; i++, pText = pText->next )
		{}
	SchematicDesign->titlesheet.numtexts	= i;
	if( i > 0 )
		{
		SchematicDesign->titlesheet.viotexts	= Allocate( i * sizeof( pcad_text_t* ));
		for( i = 0, pText = SchematicDesign->titlesheet.firsttext; i < SchematicDesign->titlesheet.numtexts && pText != NULL; i++, pText = pText->next )
			SchematicDesign->titlesheet.viotexts[i]	= pText;
		if( Cookie->Sort )
			qsort( SchematicDesign->titlesheet.viotexts, SchematicDesign->titlesheet.numtexts, sizeof( pcad_text_t* ), CompareTexts );
		}

//	for( i = 0; i < SchematicDesign->titlesheet.numtexts; i++ )
//		ProcessText( Cookie, SchematicDesign->titlesheet.viotexts[i] );
#endif
	/*------------------------------------------------------------------------*/

	pcad_sheet_t *pSheet;

	for( i = 0, pSheet = SchematicDesign->firstsheet; pSheet != NULL; i++, pSheet = pSheet->next )
		{}
	SchematicDesign->numsheets	= i;
	if( i > 0 )
		{
		SchematicDesign->viosheets	= Allocate( Cookie, i * sizeof( pcad_sheet_t* ));
		for( i = 0, pSheet = SchematicDesign->firstsheet; i < SchematicDesign->numsheets && pSheet != NULL; i++, pSheet = pSheet->next )
			SchematicDesign->viosheets[i]	= pSheet;
		if( Cookie->Sort )
			qsort( SchematicDesign->viosheets, SchematicDesign->numsheets, sizeof( pcad_sheet_t* ), CompareSheets );
		}

	for( i = 0; i < SchematicDesign->numsheets; i++ )
		{
//		fprintf( stderr, "Processing sheet %u: ", i );
//		fprintf( stderr, "%s\n", SchematicDesign->viosheets[i]->name );
		ProcessSheet( Cookie, SchematicDesign->viosheets[i] );
		}

	/*------------------------------------------------------------------------*/

	pcad_sheetref_t *pSheetRef;

	for( i = 0, pSheetRef = SchematicDesign->schematicPrintSettings.sheetlist.firstsheetref; pSheetRef != NULL; i++, pSheetRef = pSheetRef->next )
		{}
	SchematicDesign->schematicPrintSettings.sheetlist.numsheetrefs	= i;
	if( i > 0 )
		{
		SchematicDesign->schematicPrintSettings.sheetlist.viosheetrefs	= Allocate( Cookie, i * sizeof( pcad_sheetref_t* ));
		for( i = 0, pSheetRef = SchematicDesign->schematicPrintSettings.sheetlist.firstsheetref; i < SchematicDesign->schematicPrintSettings.sheetlist.numsheetrefs && pSheetRef != NULL; i++, pSheetRef = pSheetRef->next )
			SchematicDesign->schematicPrintSettings.sheetlist.viosheetrefs[i]	= pSheetRef;
		if( Cookie->Sort )
			qsort( SchematicDesign->schematicPrintSettings.sheetlist.viosheetrefs, SchematicDesign->schematicPrintSettings.sheetlist.numsheetrefs, sizeof( pcad_sheetref_t* ), CompareSheetRefs );
		}

//	for( i = 0; i < SchematicDesign->schematicPrintSettings.sheetlist.numsheetrefs; i++ )
//		ProcessSheetRef( Cookie, SchematicDesign->schematicPrintSettings.sheetlist.viosheetrefs[i] );

	/*------------------------------------------------------------------------*/

	pcad_reportdefinition_t *pReportDefinition;

	for( i = 0, pReportDefinition = SchematicDesign->reportsettings.reportdefinitions.firstreportdefinition; pReportDefinition != NULL; i++, pReportDefinition = pReportDefinition->next )
		{}
	SchematicDesign->reportsettings.reportdefinitions.numreportdefinitions	= i;
	if( i > 0 )
		{
		SchematicDesign->reportsettings.reportdefinitions.vioreportdefinitions	= Allocate( Cookie, i * sizeof( pcad_reportdefinition_t* ));
		for( i = 0, pReportDefinition = SchematicDesign->reportsettings.reportdefinitions.firstreportdefinition;
				i < SchematicDesign->reportsettings.reportdefinitions.numreportdefinitions && pReportDefinition != NULL; i++, pReportDefinition = pReportDefinition->next )
			SchematicDesign->reportsettings.reportdefinitions.vioreportdefinitions[i]	= pReportDefinition;
		//if( Cookie->Sort )
		//	qsort( SchematicDesign->reportsettings.reportdefinitions.vioreportdefinitions, SchematicDesign->reportsettings.reportdefinitions.numreportdefinitions, sizeof( pcad_reportdefinition_t* ), CompareReportDefinitions );
		}

	for( i = 0; i < SchematicDesign->reportsettings.reportdefinitions.numreportdefinitions; i++ )
		{
//		fprintf( stderr, "Processing sheet %u: ", i );
//		fprintf( stderr, "%s\n", SchematicDesign->viosheets[i]->name );
		ProcessReportDefinition( Cookie, SchematicDesign->reportsettings.reportdefinitions.vioreportdefinitions[i] );
		}

	/*------------------------------------------------------------------------*/

	return 0;
	}
/*===========================================================================*/
/*===========================================================================*/
/*===========================================================================*/
/*===========================================================================*/
static int CompareCompInsts( const void *a, const void *b )
	{
	const pcad_compinst_t	*pa = *(const pcad_compinst_t * const *)a;
	const pcad_compinst_t	*pb = *(const pcad_compinst_t * const *)b;

	return CompareNames( pa->name, pb->name );
	}
/*===========================================================================*/
static int CompareNets( const void *a, const void *b )
	{
	const pcad_net_t	*pa = *(const pcad_net_t * const *)a;
	const pcad_net_t	*pb = *(const pcad_net_t * const *)b;

	return CompareNames( pa->name, pb->name );
	}
/*===========================================================================*/
static int CompareNodes( const void *a, const void *b )
	{
	const pcad_node_t	*pa = *(const pcad_node_t * const *)a;
	const pcad_node_t	*pb = *(const pcad_node_t * const *)b;
	int					Result;

	Result	= CompareNames( pa->component, pb->component );
	if( Result != 0 )
		return Result;
	return CompareNames( pa->pin, pb->pin );
	}
/*===========================================================================*/
static int ProcessCompInst( cookie_t *Cookie, pcad_compinst_t *CompInst )
	{
	int				i;

	/*------------------------------------------------------------------------*/

	pcad_attr_t *pAttr;

	for( i = 0, pAttr = CompInst->firstattr; pAttr != NULL; i++, pAttr = pAttr->next )
		{}
	CompInst->numattrs	= i;
	if( i > 0 )
		{
		CompInst->vioattrs	= Allocate( Cookie, i * sizeof( pcad_attr_t* ));
		for( i = 0, pAttr = CompInst->firstattr; i < CompInst->numattrs && pAttr != NULL; i++, pAttr = pAttr->next )
			CompInst->vioattrs[i]	= pAttr;
		if( Cookie->Sort )
			qsort( CompInst->vioattrs, CompInst->numattrs, sizeof( pcad_attr_t* ), CompareAttrs );
		}

//	for( i = 0; i < CompInst->numattrs; i++ )
//		ProcessAttr( Cookie, CompInst->vioattrs[i] );

	/*------------------------------------------------------------------------*/

	return 0;
	}
/*===========================================================================*/
static int ProcessNet( cookie_t *Cookie, pcad_net_t *Net )
	{
	int				i;

	/*------------------------------------------------------------------------*/

	pcad_node_t *pNode;

	for( i = 0, pNode = Net->firstnode; pNode != NULL; i++, pNode = pNode->next )
		{}
	Net->numnodes	= i;
	if( i > 0 )
		{
		Net->vionodes	= Allocate( Cookie, i * sizeof( pcad_node_t* ));
		for( i = 0, pNode = Net->firstnode; i < Net->numnodes && pNode != NULL; i++, pNode = pNode->next )
			Net->vionodes[i]	= pNode;
		if( Cookie->Sort )
			qsort( Net->vionodes, Net->numnodes, sizeof( pcad_node_t* ), CompareNodes );
		}

//	for( i = 0; i < Net->numnodes; i++ )
//		ProcessNode( Cookie, Net->vionodes[i] );

	/*------------------------------------------------------------------------*/

	return 0;
	}
/*===========================================================================*/
static int ProcessPCADNetList( cookie_t *Cookie, pcad_netlist_t *NetList )
	{
	int				i;

	/*------------------------------------------------------------------------*/

	pcad_attr_t	*pAttr;

	for( i = 0, pAttr = NetList->globalattrs.firstattr; pAttr != NULL; i++, pAttr = pAttr->next )
		{}
	NetList->globalattrs.numattr	= i;
	if( i > 0 )
		{
		NetList->globalattrs.vioattrs	= Allocate( Cookie, i * sizeof( pcad_attr_t* ));
		for( i = 0, pAttr = NetList->globalattrs.firstattr; i < NetList->globalattrs.numattr && pAttr != NULL; i++, pAttr = pAttr->next )
			NetList->globalattrs.vioattrs[i]	= pAttr;
		if( Cookie->Sort )
			qsort( NetList->globalattrs.vioattrs, NetList->globalattrs.numattr, sizeof( pcad_attr_t* ), CompareAttrs );
		}

//	for( i = 0; i < NetList->globalattrs.numattr; i++ )
//		ProcessAttr( Cookie, NetList->globalattrs.vioattrs[i] );

	/*------------------------------------------------------------------------*/

	pcad_compinst_t *pCompInst;

	for( i = 0, pCompInst = NetList->firstcompinst; pCompInst != NULL; i++, pCompInst = pCompInst->next )
		{}
	NetList->numcompinsts	= i;
	if( i > 0 )
		{
		NetList->viocompinsts	= Allocate( Cookie, i * sizeof( pcad_compinst_t* ));
		for( i = 0, pCompInst = NetList->firstcompinst; i < NetList->numcompinsts && pCompInst != NULL; i++, pCompInst = pCompInst->next )
			NetList->viocompinsts[i]	= pCompInst;
		if( Cookie->Sort )
			qsort( NetList->viocompinsts, NetList->numcompinsts, sizeof( pcad_compinst_t* ), CompareCompInsts );
		}

	for( i = 0; i < NetList->numcompinsts; i++ )
		ProcessCompInst( Cookie, NetList->viocompinsts[i] );

	/*------------------------------------------------------------------------*/

	pcad_net_t	*pNet;

	for( i = 0, pNet = NetList->firstnet; pNet != NULL; i++, pNet = pNet->next )
		{}
	NetList->numnets	= i;
	if( i > 0 )
		{
		NetList->vionets	= Allocate( Cookie, i * sizeof( pcad_net_t* ));
		for( i = 0, pNet = NetList->firstnet; i < NetList->numnets && pNet != NULL; i++, pNet = pNet->next )
			NetList->vionets[i]	= pNet;
		if( Cookie->Sort )
			qsort( NetList->vionets, NetList->numnets, sizeof( pcad_net_t* ), CompareNets );
		}

	for( i = 0; i < NetList->numnets; i++ )
		ProcessNet( Cookie, NetList->vionets[i] );

	/*------------------------------------------------------------------------*/

	return 0;
	}
/*===========================================================================*/
/*===========================================================================*/
/*===========================================================================*/
/*===========================================================================*/
static int CompareCompPadPinMaps( const void *a, const void *b )
	{
	const pcad_padpinmap_t	*pa = *(const pcad_padpinmap_t * const *)a;
	const pcad_padpinmap_t	*pb = *(const pcad_padpinmap_t * const *)b;

	return pa->padnum - pb->padnum;
	}
/*===========================================================================*/
static int ProcessSymbolDef( cookie_t *Cookie, pcad_symboldef_t *SymbolDef )
	{
	int				i;

	/*------------------------------------------------------------------------*/

	pcad_pin_t *pPin;

	for( i = 0, pPin = SymbolDef->firstpin; pPin != NULL; i++, pPin = pPin->next )
		{}
	SymbolDef->numpins	= i;
	if( i > 0 )
		{
		SymbolDef->viopins	= Allocate( Cookie, i * sizeof( pcad_pin_t* ));
		for( i = 0, pPin = SymbolDef->firstpin; i < SymbolDef->numpins && pPin != NULL; i++, pPin = pPin->next )
			SymbolDef->viopins[i]	= pPin;
		if( Cookie->Sort )
			qsort( SymbolDef->viopins, SymbolDef->numpins, sizeof( pcad_pin_t* ), ComparePins );
		}

//	for( i = 0; i < SymbolDef->numpins; i++ )
//		ProcessPin( Cookie, SymbolDef->viopins[i] );

	/*------------------------------------------------------------------------*/

	pcad_line_t	*pLine;

	for( i = 0, pLine = SymbolDef->firstline; pLine != NULL; i++, pLine = pLine->next )
		{}
	SymbolDef->numlines	= i;
	if( i > 0 )
		{
		SymbolDef->violines	= Allocate( Cookie, i * sizeof( pcad_line_t* ));
		for( i = 0, pLine = SymbolDef->firstline; i < SymbolDef->numlines && pLine != NULL; i++, pLine = pLine->next )
			SymbolDef->violines[i]	= pLine;
		if( Cookie->Sort )
			qsort( SymbolDef->violines, SymbolDef->numlines, sizeof( pcad_line_t* ), CompareLines );
		}

//	for( i = 0; i < SymbolDef->numlines; i++ )
//		ProcessPin( Cookie, SymbolDef->violines[i] );

	/*------------------------------------------------------------------------*/

	pcad_poly_t *pPoly;

	for( i = 0, pPoly = SymbolDef->firstpoly; pPoly != NULL; i++, pPoly = pPoly->next )
		{}
	SymbolDef->numpolys	= i;
	if( i > 0 )
		{
		SymbolDef->viopolys	= Allocate( Cookie, i * sizeof( pcad_poly_t* ));
		for( i = 0, pPoly = SymbolDef->firstpoly; i < SymbolDef->numpolys && pPoly != NULL; i++, pPoly = pPoly->next )
			{
//			fprintf( stderr, "\tProcessing polygon %u\n", i );
			SymbolDef->viopolys[i]	= pPoly;
			ProcessPoly( Cookie, SymbolDef->viopolys[i] );
			}
		if( Cookie->Sort )
			qsort( SymbolDef->viopolys, SymbolDef->numpolys, sizeof( pcad_poly_t* ), ComparePolys );
		}

	/*------------------------------------------------------------------------*/

	pcad_attr_t *pAttr;

	for( i = 0, pAttr = SymbolDef->firstattr; pAttr != NULL; i++, pAttr = pAttr->next )
		{}
	SymbolDef->numattrs	= i;
	if( i > 0 )
		{
		SymbolDef->vioattrs	= Allocate( Cookie, i * sizeof( pcad_attr_t* ));
		for( i = 0, pAttr = SymbolDef->firstattr; i < SymbolDef->numattrs && pAttr != NULL; i++, pAttr = pAttr->next )
			SymbolDef->vioattrs[i]	= pAttr;
		if( Cookie->Sort )
			qsort( SymbolDef->vioattrs, SymbolDef->numattrs, sizeof( pcad_attr_t* ), CompareAttrs );
		}

//	for( i = 0; i < SymbolDef->numattrs; i++ )
//		ProcessAttr( Cookie, SymbolDef->vioattrs[i] );

	/*------------------------------------------------------------------------*/

	pcad_triplepointarc_t *pArc;

	for( i = 0, pArc = SymbolDef->firsttriplepointarc; pArc != NULL; i++, pArc = pArc->next )
		{}
	SymbolDef->numtriplepointarcs	= i;
	if( i > 0 )
		{
		SymbolDef->viotriplepointarcs	= Allocate( Cookie, i * sizeof( pcad_triplepointarc_t* ));
		for( i = 0, pArc = SymbolDef->firsttriplepointarc; i < SymbolDef->numtriplepointarcs && pArc != NULL; i++, pArc = pArc->next )
			SymbolDef->viotriplepointarcs[i]	= pArc;
		if( Cookie->Sort )
			qsort( SymbolDef->viotriplepointarcs, SymbolDef->numtriplepointarcs, sizeof( pcad_triplepointarc_t* ), CompareArcs );
		}

//	for( i = 0; i < SymbolDef->numarcs; i++ )
//		ProcessPin( Cookie, SymbolDef->vioarcs[i] );

	/*------------------------------------------------------------------------*/

	pcad_text_t *pText;

	for( i = 0, pText = SymbolDef->firsttext; pText != NULL; i++, pText = pText->next )
		{}
	SymbolDef->numtexts	= i;
	if( i > 0 )
		{
		SymbolDef->viotexts	= Allocate( Cookie, i * sizeof( pcad_text_t* ));
		for( i = 0, pText = SymbolDef->firsttext; i < SymbolDef->numtexts && pText != NULL; i++, pText = pText->next )
			SymbolDef->viotexts[i]	= pText;
		if( Cookie->Sort )
			qsort( SymbolDef->viotexts, SymbolDef->numtexts, sizeof( pcad_text_t* ), CompareTexts );
		}

//	for( i = 0; i < SymbolDef->numtexts; i++ )
//		ProcessText( Cookie, SymbolDef->viotexts[i] );

	/*------------------------------------------------------------------------*/

	pcad_ieeesymbol_t	*pIEEESymbol;

	for( i = 0, pIEEESymbol = SymbolDef->firstieeesymbol; pIEEESymbol != NULL; i++, pIEEESymbol = pIEEESymbol->next )
		{}
	SymbolDef->numieeesymbols	= i;
	if( i > 0 )
		{
		SymbolDef->vioieeesymbols	= Allocate( Cookie, i * sizeof( pcad_ieeesymbol_t* ));
		for( i = 0, pIEEESymbol = SymbolDef->firstieeesymbol; i < SymbolDef->numieeesymbols && pIEEESymbol != NULL; i++, pIEEESymbol = pIEEESymbol->next )
			SymbolDef->vioieeesymbols[i]	= pIEEESymbol;
		if( Cookie->Sort )
			qsort( SymbolDef->vioieeesymbols, SymbolDef->numieeesymbols, sizeof( pcad_ieeesymbol_t* ), CompareIEEESymbols );
		}

//	for( i = 0; i < SymbolDef->numieeesymbols; i++ )
//		ProcessIEEESymbol( Cookie, SymbolDef->vioieeesymbols[i] );

	/*------------------------------------------------------------------------*/

	return 0;
	}
/*===========================================================================*/
/*===========================================================================*/
/*===========================================================================*/
/*===========================================================================*/
static int CompareCompPins( const void *a, const void *b )
	{
	const pcad_comppin_t	*pa = *(const pcad_comppin_t * const *)a;
	const pcad_comppin_t	*pb = *(const pcad_comppin_t * const *)b;

	return CompareNames( pa->pinnumber, pb->pinnumber );
	}
/*===========================================================================*/
static int CompareAttachedSymbols( const void *a, const void *b )
	{
	const pcad_attachedsymbol_t *pa = *(const pcad_attachedsymbol_t * const *)a;
	const pcad_attachedsymbol_t *pb = *(const pcad_attachedsymbol_t * const *)b;

	return pa->partnum != pb->partnum ? pa->partnum - pb->partnum : pa->alttype - pb->alttype;
	}
/*===========================================================================*/
static int ProcessCompDef( cookie_t *Cookie, pcad_compdef_t *CompDef )
	{
	int				i;

	/*------------------------------------------------------------------------*/

	pcad_comppin_t *pCompPin;

	for( i = 0, pCompPin = CompDef->firstcomppin; pCompPin != NULL; i++, pCompPin = pCompPin->next )
		{}
	CompDef->numcomppins	= i;
	if( i > 0 )
		{
		CompDef->viocomppins	= Allocate( Cookie, i * sizeof( pcad_comppin_t* ));
		for( i = 0, pCompPin = CompDef->firstcomppin; i < CompDef->numcomppins && pCompPin != NULL; i++, pCompPin = pCompPin->next )
			CompDef->viocomppins[i]	= pCompPin;
		if( Cookie->Sort )
			qsort( CompDef->viocomppins, CompDef->numcomppins, sizeof( pcad_comppin_t* ), CompareCompPins );
		}

//	for( i = 0; i < CompDef->numcomppins; i++ )
//		ProcessCompPin( Cookie, CompDef->viocomppins[i] );

	/*------------------------------------------------------------------------*/

	pcad_attachedsymbol_t	*pAttachedSymbol;

	for( i = 0, pAttachedSymbol = CompDef->firstattachedsymbol; pAttachedSymbol != NULL; i++, pAttachedSymbol = pAttachedSymbol->next )
		{}
	CompDef->numattachedsymbols	= i;
	if( i > 0 )
		{
		CompDef->vioattachedsymbols	= Allocate( Cookie, i * sizeof( pcad_attachedsymbol_t* ));
		for( i = 0, pAttachedSymbol = CompDef->firstattachedsymbol; i < CompDef->numattachedsymbols && pAttachedSymbol != NULL; i++, pAttachedSymbol = pAttachedSymbol->next )
			CompDef->vioattachedsymbols[i]	= pAttachedSymbol;
		if( Cookie->Sort )
			qsort( CompDef->vioattachedsymbols, CompDef->numattachedsymbols, sizeof( pcad_attachedsymbol_t* ), CompareAttachedSymbols );
		}

//	for( i = 0; i < CompDef->numattachedsymbols; i++ )
//		ProcessAttachedSymbol( Cookie, CompDef->vioattachedsymbols[i] );

	/*------------------------------------------------------------------------*/

	pcad_padpinmap_t *pPadPinMap;

	for( i = 0, pPadPinMap = CompDef->attachedpattern.firstpadpinmap; pPadPinMap != NULL; i++, pPadPinMap = pPadPinMap->next )
		{}
	CompDef->attachedpattern.numpadpinmaps	= i;
	if( i > 0 )
		{
		CompDef->attachedpattern.viopadpinmaps	= Allocate( Cookie, i * sizeof( pcad_padpinmap_t* ));
		for( i = 0, pPadPinMap = CompDef->attachedpattern.firstpadpinmap; i < CompDef->attachedpattern.numpadpinmaps && pPadPinMap != NULL; i++, pPadPinMap = pPadPinMap->next )
			CompDef->attachedpattern.viopadpinmaps[i]	= pPadPinMap;
		if( Cookie->Sort )
			qsort( CompDef->attachedpattern.viopadpinmaps, CompDef->attachedpattern.numpadpinmaps, sizeof( pcad_padpinmap_t* ), CompareCompPadPinMaps );
		}

//	for( i = 0; i < CompDef->attachedpattern.numpadpinmaps; i++ )
//		ProcessCompPin( Cookie, CompDef->attachedpattern.viopadpinmaps[i] );

	/*------------------------------------------------------------------------*/

	return 0;
	}
/*===========================================================================*/
/*===========================================================================*/
/*===========================================================================*/
/*===========================================================================*/
static int CompareTextStyleDefs( const void *a, const void *b )
	{
	const pcad_textstyledef_t	*pa = *(const pcad_textstyledef_t * const *)a;
	const pcad_textstyledef_t	*pb = *(const pcad_textstyledef_t * const *)b;

	return CompareNames( pa->name, pb->name );
	}
/*===========================================================================*/
static int CompareSymbolDefs( const void *a, const void *b )
	{
	const pcad_symboldef_t	*pa = *(const pcad_symboldef_t * const *)a;
	const pcad_symboldef_t	*pb = *(const pcad_symboldef_t * const *)b;

	return CompareNames( pa->name, pb->name );
	}
/*===========================================================================*/
static int CompareCompDefs( const void *a, const void *b )
	{
	const pcad_compdef_t	*pa = *(const pcad_compdef_t * const *)a;
	const pcad_compdef_t	*pb = *(const pcad_compdef_t * const *)b;

	return CompareNames( pa->name, pb->name );
	}
/*===========================================================================*/
static int ProcessTextStyleDef( cookie_t *Cookie, pcad_textstyledef_t *TextStyleDef )
	{
	int				i;

	pcad_font_t *pFont;

	for( i = 0, pFont = TextStyleDef->firstfont; pFont != NULL; i++, pFont = pFont->next )
		{}
	TextStyleDef->numfonts	= i;
	if( i > 0 )
		{
		TextStyleDef->viofonts	= Allocate( Cookie, i * sizeof( pcad_textstyledef_t* ));
		for( i = 0, pFont = TextStyleDef->firstfont; i < TextStyleDef->numfonts && pFont != NULL; i++, pFont = pFont->next )
			TextStyleDef->viofonts[i]	= pFont;
//		if( Cookie->Sort )
//			qsort( TextStyleDef->viofonts, TextStyleDef->numfonts, sizeof( pcad_textstyledef_t* ), CompareFonts );
		}

//	for( i = 0; i < TextStyleDef->numfonts; i++ )
//		ProcessFont( Cookie, Cookie, TextStyleDef->viofonts[i] );
	/*------------------------------------------------------------------------*/

	return 0;
	}
/*===========================================================================*/
static int ProcessPCADLibrary( cookie_t *Cookie, pcad_library_t *PCADLibrary )
	{
	int				i;

	/*------------------------------------------------------------------------*/

	pcad_textstyledef_t *pTextStyleDef;

	for( i = 0, pTextStyleDef = PCADLibrary->firsttextstyledef; pTextStyleDef != NULL; i++, pTextStyleDef = pTextStyleDef->next )
		{}
	PCADLibrary->numtextstyledefs	= i;
	if( i > 0 )
		{
		PCADLibrary->viotextstyledefs	= Allocate( Cookie, i * sizeof( pcad_textstyledef_t* ));
		for( i = 0, pTextStyleDef = PCADLibrary->firsttextstyledef; i < PCADLibrary->numtextstyledefs && pTextStyleDef != NULL; i++, pTextStyleDef = pTextStyleDef->next )
			PCADLibrary->viotextstyledefs[i]	= pTextStyleDef;
		if( Cookie->Sort )
			qsort( PCADLibrary->viotextstyledefs, PCADLibrary->numtextstyledefs, sizeof( pcad_textstyledef_t* ), CompareTextStyleDefs );
		}

	for( i = 0; i < PCADLibrary->numtextstyledefs; i++ )
		ProcessTextStyleDef( Cookie, PCADLibrary->viotextstyledefs[i] );

	/*------------------------------------------------------------------------*/

	pcad_symboldef_t *pSymbolDef;

	for( i = 0, pSymbolDef = PCADLibrary->firstsymboldef; pSymbolDef != NULL; i++, pSymbolDef = pSymbolDef->next )
		{}
	PCADLibrary->numsymboldefs	= i;
	if( i > 0 )
		{
		PCADLibrary->viosymboldefs	= Allocate( Cookie, i * sizeof( pcad_textstyledef_t* ));
		for( i = 0, pSymbolDef = PCADLibrary->firstsymboldef; i < PCADLibrary->numsymboldefs && pSymbolDef != NULL; i++, pSymbolDef = pSymbolDef->next )
			PCADLibrary->viosymboldefs[i]	= pSymbolDef;
		if( Cookie->Sort )
			qsort( PCADLibrary->viosymboldefs, PCADLibrary->numsymboldefs, sizeof( pcad_textstyledef_t* ), CompareSymbolDefs );
		}

	for( i = 0; i < PCADLibrary->numsymboldefs; i++ )
		{
//		fprintf( stderr, "\tProcessing symboldef %u: ", i );
//		fprintf( stderr, "%s\n", PCADLibrary->viosymboldefs[i]->name );
		ProcessSymbolDef( Cookie, PCADLibrary->viosymboldefs[i] );
		}

	/*------------------------------------------------------------------------*/

	pcad_compdef_t *pCompDef;

	for( i = 0, pCompDef = PCADLibrary->firstcompdef; pCompDef != NULL; i++, pCompDef = pCompDef->next )
		{}
	PCADLibrary->numcompdefs	= i;
	if( i > 0 )
		{
		PCADLibrary->viocompdefs	= Allocate( Cookie, i * sizeof( pcad_compdef_t* ));
		for( i = 0, pCompDef = PCADLibrary->firstcompdef; i < PCADLibrary->numcompdefs && pCompDef != NULL; i++, pCompDef = pCompDef->next )
			PCADLibrary->viocompdefs[i]	= pCompDef;
		if( Cookie->Sort )
			qsort( PCADLibrary->viocompdefs, PCADLibrary->numcompdefs, sizeof( pcad_compdef_t* ), CompareCompDefs );
		}

	for( i = 0; i < PCADLibrary->numcompdefs; i++ )
		ProcessCompDef( Cookie, PCADLibrary->viocompdefs[i] );

	/*------------------------------------------------------------------------*/

	return 0;
	}
/*===========================================================================*/
int PCADProcesSchematic( cookie_t *Cookie, pcad_schematicfile_t *PCADSchematic )
	{
	ProcessPCADLibrary( Cookie, &PCADSchematic->library );
	ProcessPCADNetList( Cookie, &PCADSchematic->netlist );
	ProcessPCADSchematicDesign( Cookie, &PCADSchematic->schematicdesign );
	return 0;
	}
/*===========================================================================*/
