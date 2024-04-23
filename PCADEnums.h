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
#if			!defined __PCADENUMS_H__
#define __PCADENUMS_H__
/*============================================================================*/
#include <stdlib.h>
/*============================================================================*/
/*============================================================================*/
typedef struct
    {
    size_t  numitems;
    char    *items[];
    } parseenum_t;
/*============================================================================*/
/*============================================================================*/
typedef enum
    {
    PCAD_BOOLEAN_ABSENT,
    PCAD_BOOLEAN_FALSE,
    PCAD_BOOLEAN_TRUE
    } pcad_boolean_t;
/*============================================================================*/
typedef enum
    {
    PCAD_UNITS_NONE,
    PCAD_UNITS_MM,
    PCAD_UNITS_MIL,
    PCAD_UNITS_IN
    } pcad_units_t;
/*============================================================================*/
typedef enum
    {
	PCAD_NOTEANNOTATION_NONE,
	PCAD_NOTEANNOTATION_BOX,
	PCAD_NOTEANNOTATION_CIRCLE,
	PCAD_NOTEANNOTATION_TRIANGLE
    } pcad_noteannotation_t;
/*============================================================================*/
typedef enum
    {
    PCAD_CONSTRAINTUNITS_NONE,
    PCAD_CONSTRAINTUNITS_MM,
    PCAD_CONSTRAINTUNITS_MIL,
    PCAD_CONSTRAINTUNITS_IN,
    PCAD_CONSTRAINTUNITS_BOOL,
    PCAD_CONSTRAINTUNITS_DEGREE,
    PCAD_CONSTRAINTUNITS_LAYERNAME,
    PCAD_CONSTRAINTUNITS_QUANTITY,
    PCAD_CONSTRAINTUNITS_STRING,
    PCAD_CONSTRAINTUNITS_VIASTYLE
    } pcad_constraintunits_t;
/*============================================================================*/
typedef enum
	{
	PCAD_ORIENT_UP,
	PCAD_ORIENT_DOWN,
	PCAD_ORIENT_LEFT,
	PCAD_ORIENT_RIGHT
	} pcad_orient_t;
/*============================================================================*/
typedef enum
	{
	PCAD_IEEESYMBOL_GENERATOR,
	PCAD_IEEESYMBOL_AMPLIFIER,
	PCAD_IEEESYMBOL_ADDER,
	PCAD_IEEESYMBOL_COMPLEX,
	PCAD_IEEESYMBOL_HYSTERESIS,
	PCAD_IEEESYMBOL_MULTIPLIER,
	PCAD_IEEESYMBOL_ASTABLE
    } pcad_enum_ieeesymbol_t;
/*============================================================================*/
typedef enum
	{
	PCAD_JUSTIFY_LOWERLEFT,   	/* Note: "LowerLeft" doesn't exist, it is the default by omission. */
	PCAD_JUSTIFY_LOWERCENTER,
	PCAD_JUSTIFY_LOWERRIGHT,
	PCAD_JUSTIFY_LEFT,
	PCAD_JUSTIFY_CENTER,
	PCAD_JUSTIFY_RIGHT,
	PCAD_JUSTIFY_UPPERLEFT,
	PCAD_JUSTIFY_UPPERCENTER,
	PCAD_JUSTIFY_UPPERRIGHT
	} pcad_enum_justify_t;
/*============================================================================*/
typedef enum
	{
	PCAD_LINESTYLE_SOLIDLINE,
	PCAD_LINESTYLE_DASHEDLINE,
	PCAD_LINESTYLE_DOTTEDLINE
	} pcad_enum_linestyle_t;
/*============================================================================*/
typedef enum
	{
	PCAD_ENDSTYLE_NONE,
	PCAD_ENDSTYLE_LEFTLEAD,
	PCAD_ENDSTYLE_RIGHTLEAD,
	PCAD_ENDSTYLE_TWOLEADS
	} pcad_endstyle_t;
/*============================================================================*/
typedef enum
	{
	PCAD_PORTTYPE_NOANGLE_SGL_HORZ,
	PCAD_PORTTYPE_LEFTANGLE_SGL_HORZ,
	PCAD_PORTTYPE_RIGHTANGLE_SGL_HORZ,
	PCAD_PORTTYPE_BOTHANGLE_SGL_HORZ,	/* Up to here, KiCAD has equivalent port types. */
	PCAD_PORTTYPE_VERTLINE_SGL_HORZ,
	PCAD_PORTTYPE_NOOUTLINE_SGL_HORZ,

	PCAD_PORTTYPE_NOANGLE_SGL_VERT,
	PCAD_PORTTYPE_LEFTANGLE_SGL_VERT,
	PCAD_PORTTYPE_RIGHTANGLE_SGL_VERT,
	PCAD_PORTTYPE_BOTHANGLE_SGL_VERT,
	PCAD_PORTTYPE_VERTLINE_SGL_VERT,
	PCAD_PORTTYPE_NOOUTLINE_SGL_VERT,   /* Up to here, it is possible to replace a P-CAD port with a KiCAD one without losing conectivity. */

	PCAD_PORTTYPE_NOANGLE_DBL_HORZ,
	PCAD_PORTTYPE_LEFTANGLE_DBL_HORZ,
	PCAD_PORTTYPE_RIGHTANGLE_DBL_HORZ,
	PCAD_PORTTYPE_BOTHANGLE_DBL_HORZ,
	PCAD_PORTTYPE_VERTLINE_DBL_HORZ,
	PCAD_PORTTYPE_NOOUTLINE_DBL_HORZ,

	PCAD_PORTTYPE_NOANGLE_DBL_VERT,
	PCAD_PORTTYPE_LEFTANGLE_DBL_VERT,
	PCAD_PORTTYPE_RIGHTANGLE_DBL_VERT,
	PCAD_PORTTYPE_BOTHANGLE_DBL_VERT,
	PCAD_PORTTYPE_VERTLINE_DBL_VERT,
	PCAD_PORTTYPE_NOOUTLINE_DBL_VERT
	} pcad_enum_porttype_t;
/*============================================================================*/
typedef enum
	{
	PCAD_PINTYPE_NONE,
	PCAD_PINTYPE_UNKNOWN,
	PCAD_PINTYPE_PASSIVE,
	PCAD_PINTYPE_INPUT,
	PCAD_PINTYPE_OUTPUT,
	PCAD_PINTYPE_BIDIRECTIONAL,
	PCAD_PINTYPE_OPEN_H,
	PCAD_PINTYPE_OPEN_L,
	PCAD_PINTYPE_PASSIVE_H,
	PCAD_PINTYPE_PASSIVE_L,
	PCAD_PINTYPE_3_STATE,
	PCAD_PINTYPE_POWER
	} pcad_enum_pintype_t;
/*============================================================================*/
typedef enum
	{
	PCAD_PORTPINLENGTH_PORTPINLONG,		/* Note: "PortPinLong" doesn't exist, it is the default by omission. */
	PCAD_PORTPINLENGTH_PORTPINSHORT
	} pcad_enum_portpinlength_t;
/*============================================================================*/
typedef enum
	{
	PCAD_INSIDEEDGESTYLE_NONE,		/* Note: "None" doesn't exist, it is the default by omission. */
	PCAD_INSIDEEDGESTYLE_CLOCK
	} pcad_enum_insideedgestyle_t;
/*============================================================================*/
typedef enum
	{
	PCAD_INSIDESTYLE_NONE,			/* Note: "None" doesn't exist, it is the default by omission. */
	PCAD_INSIDESTYLE_AMPLIFIER,
	PCAD_INSIDESTYLE_GENERATOR,
	PCAD_INSIDESTYLE_HYSTERESIS,
	PCAD_INSIDESTYLE_OPEN,
	PCAD_INSIDESTYLE_OPENHIGH,
	PCAD_INSIDESTYLE_OPENLOW,
	PCAD_INSIDESTYLE_PASSIVEDOWN,
	PCAD_INSIDESTYLE_PASSIVEUP,
	PCAD_INSIDESTYLE_POSTPONED,
	PCAD_INSIDESTYLE_SHIFT,
	PCAD_INSIDESTYLE_THREESTATE
	} pcad_enum_insidestyle_t;
/*============================================================================*/
typedef enum
	{
	PCAD_OUTSIDEEDGESTYLE_NONE,
	PCAD_OUTSIDEEDGESTYLE_DOT,
	PCAD_OUTSIDEEDGESTYLE_POLARITYIN,
	PCAD_OUTSIDEEDGESTYLE_POLARITYOUT
	} pcad_enum_outsideedgestyle_t;
/*============================================================================*/
typedef enum
	{
	PCAD_OUTSIDESTYLE_NONE,
	PCAD_OUTSIDESTYLE_ANALOG,
	PCAD_OUTSIDESTYLE_DIGITAL,
	PCAD_OUTSIDESTYLE_FLOWBI,
	PCAD_OUTSIDESTYLE_FLOWIN,
	PCAD_OUTSIDESTYLE_FLOWOUT,
	PCAD_OUTSIDESTYLE_NONLOGIC
	} pcad_enum_outsidestyle_t;
/*============================================================================*/
typedef enum
	{
	PCAD_FONTTYPE_STROKE,
	PCAD_FONTTYPE_TRUETYPE
	} pcad_enum_fonttype_t;
/*============================================================================*/
typedef enum
	{
	PCAD_NUMTYPE_ALPHABETIC,
	PCAD_NUMTYPE_NUMERIC
	} pcad_enum_numtype_t;
/*============================================================================*/
typedef enum
	{
	PCAD_NUMTYPE_ASCENDING,
	PCAD_NUMTYPE_DESCENDING
	} pcad_enum_numdirection_t;
/*============================================================================*/
typedef enum
	{
	PCAD_COMPOSITION_HOMOGENEOUS,
	PCAD_COMPOSITION_HETEROGENEOUS
	} pcad_enum_composition_t;
/*============================================================================*/
typedef enum
	{
	PCAD_COMPTYPE_NORMAL,
	PCAD_COMPTYPE_POWER,
	PCAD_COMPTYPE_SHEETCONNECTOR,
	PCAD_COMPTYPE_MODULE,
	PCAD_COMPTYPE_LINK
	} pcad_enum_comptype_t;
/*============================================================================*/
typedef enum
	{
	PCAD_PAGESIZE_SIZE_A,
	PCAD_PAGESIZE_SIZE_B,
	PCAD_PAGESIZE_SIZE_C,
	PCAD_PAGESIZE_SIZE_D,
	PCAD_PAGESIZE_SIZE_E,
	PCAD_PAGESIZE_SIZE_A4,
	PCAD_PAGESIZE_SIZE_A3,
	PCAD_PAGESIZE_SIZE_A2,
	PCAD_PAGESIZE_SIZE_A1,
	PCAD_PAGESIZE_SIZE_A0,
	PCAD_PAGESIZE_SCALETOFITPAGE,
	PCAD_PAGESIZE_USER
	} pcad_enum_pagesize_t;
/*============================================================================*/
typedef enum
	{
	PCAD_REPORTSTYLE_ACCEL,
	PCAD_REPORTSTYLE_COMMA
	} pcad_reportstyle_t;
/*============================================================================*/
typedef enum
	{
	PCAD_REPORTDESTINATIONS_SCREEN,
	PCAD_REPORTDESTINATIONS_FILE,
	PCAD_REPORTDESTINATIONS_PRINTER
	} pcad_report_destination_t;
/*============================================================================*/
typedef enum
	{
	PCAD_REPORTTYPE_ATTRIBUTES,
	PCAD_REPORTTYPE_BILLOFMATERIALS,
	PCAD_REPORTTYPE_GLOBALNETS,
	PCAD_REPORTTYPE_LASTREFDES,
	PCAD_REPORTTYPE_LIBRARY,
	PCAD_REPORTTYPE_PARTSLOCATIONS,
	PCAD_REPORTTYPE_PARTSUSAGE,
	PCAD_REPORTTYPE_VARIANT
	} pcad_reporttypes_t;
/*============================================================================*/
typedef enum
	{
	PCAD_PROPERTYTYPE_COMPVALUE,
	PCAD_PROPERTYTYPE_COMPONENTLIBRARY,
	PCAD_PROPERTYTYPE_COMPONENTNAME,
	PCAD_PROPERTYTYPE_COUNT,
	PCAD_PROPERTYTYPE_LOCATIONX,
	PCAD_PROPERTYTYPE_LOCATIONY,
	PCAD_PROPERTYTYPE_NETNAME,
	PCAD_PROPERTYTYPE_PATTERNNAME,
	PCAD_PROPERTYTYPE_REFDES,
	PCAD_PROPERTYTYPE_ROTATION,
	PCAD_PROPERTYTYPE_SHEETNUMBER,
	PCAD_PROPERTYTYPE_TYPE,
	PCAD_PROPERTYTYPE_UNUSEDPARTS,
	PCAD_PROPERTYTYPE_USER,
	PCAD_PROPERTYTYPE_VARIANT
	} pcad_fieldtypes_t;
/*============================================================================*/
typedef enum
	{
	PCAD_SORTTYPES_NONE,
	PCAD_SORTTYPES_ASCENDING,
	PCAD_SORTTYPES_DESCENDING
	} pcad_enum_sorttypes_t;
/*============================================================================*/
extern const parseenum_t	SortTypes;
extern const parseenum_t	ReportStyles;
extern const parseenum_t	ReportTypes;
extern const parseenum_t	ReportDestinations;
extern const parseenum_t	ReportPropertyTypes;
extern const parseenum_t	NoteAnnotations;
extern const parseenum_t	Tokens;
extern const parseenum_t    ConstraintUnits;
extern const parseenum_t	Orients;
extern const parseenum_t    Units;
extern const parseenum_t   	IEEESymbols;
extern const parseenum_t	Justify;
/*----------------------------------------------------------------------------*/
extern const parseenum_t	LineStyles;
extern const parseenum_t	EndStyles;
/*----------------------------------------------------------------------------*/
extern const parseenum_t	PortTypes;
extern const parseenum_t	PortPinLengths;
/*----------------------------------------------------------------------------*/
extern const parseenum_t    PinType;
extern const parseenum_t	InsideStyle;
extern const parseenum_t	InsideEdgeStyle;
extern const parseenum_t	OutsideEdgeStyle;
extern const parseenum_t    OutsideStyle;
/*----------------------------------------------------------------------------*/
extern const parseenum_t    FontType;
/*----------------------------------------------------------------------------*/
extern const parseenum_t    NumType;
extern const parseenum_t    NumDirection;
extern const parseenum_t    Composition;
extern const parseenum_t    CompType;
extern const parseenum_t    PageSize;
/*============================================================================*/
#endif	/*	!defined __PCADENUMS_H__ */
/*============================================================================*/

