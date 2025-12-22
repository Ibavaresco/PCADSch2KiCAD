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
#include "PCADEnums.h"
#include "Parser.h"
/*============================================================================*/
const parseenum_t	Tokens =
	{
	.numitems	= 8,
	.items		=
		{
		[RESULT_OK]								= "",
		[TOKEN_CLOSE_PAR]						= "\")\"",
		[TOKEN_OPEN_PAR]						= "\"(\"",
		[TOKEN_NAME]							= "name (unquoted string)",
		[TOKEN_STRING]							= "quoted string",
		[TOKEN_UNSIGNED]						= "unsigned integer number",
		[TOKEN_INTEGER]							= "integer number",
		[TOKEN_FLOAT]							= "number"
		}
	};
/*============================================================================*/
const parseenum_t	Units =
	{
	.numitems	= 4,
	.items		=
		{
		[PCAD_UNITS_NONE]						= "",
		[PCAD_UNITS_MM]							= "mm",
		[PCAD_UNITS_MIL]						= "Mil",
		[PCAD_UNITS_IN]							= "in"
		}
	};
/*============================================================================*/
const parseenum_t	NoteAnnotations =
	{
	.numitems	= 4,
	.items		=
		{
		[PCAD_NOTEANNOTATION_NONE]				= "",
		[PCAD_NOTEANNOTATION_BOX]				= "box",
		[PCAD_NOTEANNOTATION_CIRCLE]			= "circle",
		[PCAD_NOTEANNOTATION_TRIANGLE]			= "triangle"
		}
	};
/*============================================================================*/
const parseenum_t	ConstraintUnits =
	{
	.numitems	= 10,
	.items		=
		{
		[PCAD_CONSTRAINTUNITS_NONE]				= "",
		[PCAD_CONSTRAINTUNITS_MM]				= "mm",
		[PCAD_CONSTRAINTUNITS_MIL]				= "mil",
		[PCAD_CONSTRAINTUNITS_IN]				= "in",
		[PCAD_CONSTRAINTUNITS_BOOL]				= "bool",
		[PCAD_CONSTRAINTUNITS_DEGREE]			= "degree",
		[PCAD_CONSTRAINTUNITS_LAYERNAME]		= "layername",
		[PCAD_CONSTRAINTUNITS_QUANTITY]			= "quantity",
		[PCAD_CONSTRAINTUNITS_STRING]			= "string",
		[PCAD_CONSTRAINTUNITS_VIASTYLE]			= "viastyle"
		}
	};
/*============================================================================*/
const parseenum_t	Orients	=
	{
	.numitems	= 4,
	.items		=
		{
		[PCAD_ORIENT_UP]						= "Up",
		[PCAD_ORIENT_DOWN]						= "Down",
		[PCAD_ORIENT_LEFT]						= "Left",
		[PCAD_ORIENT_RIGHT]						= "Right"
		}
	};
/*============================================================================*/
const parseenum_t	IEEESymbols	=
	{
	.numitems	= 7,
	.items		=
		{
		[PCAD_IEEESYMBOL_GENERATOR]				= "Generator",
		[PCAD_IEEESYMBOL_AMPLIFIER]				= "Amplifier",
		[PCAD_IEEESYMBOL_ADDER]					= "Adder",
		[PCAD_IEEESYMBOL_COMPLEX]				= "Complex",
		[PCAD_IEEESYMBOL_HYSTERESIS]			= "Hysteresis",
		[PCAD_IEEESYMBOL_MULTIPLIER]			= "Multiplier",
		[PCAD_IEEESYMBOL_ASTABLE]				= "Astable"
		}
	};
/*============================================================================*/
const parseenum_t	Justify	=
	{
	.numitems	= 9,
	.items		=
		{
		[PCAD_JUSTIFY_LOWERLEFT]				= "LowerLeft",	/* Note: "LowerLeft" doesn't exist, it is the default by omission. */
		[PCAD_JUSTIFY_LOWERCENTER]				= "LowerCenter",
		[PCAD_JUSTIFY_LOWERRIGHT]				= "LowerRight",
		[PCAD_JUSTIFY_LEFT]						= "Left",
		[PCAD_JUSTIFY_CENTER]					= "Center",
		[PCAD_JUSTIFY_RIGHT]					= "Right",
		[PCAD_JUSTIFY_UPPERLEFT]				= "UpperLeft",
		[PCAD_JUSTIFY_UPPERCENTER]				= "UpperCenter",
		[PCAD_JUSTIFY_UPPERRIGHT]				= "UpperRight"
		}
	};
/*============================================================================*/
const parseenum_t LineStyles	=
	{
	.numitems	= 3,
	.items		=
		{
		[PCAD_LINESTYLE_SOLIDLINE]				= "SolidLine",
		[PCAD_LINESTYLE_DASHEDLINE]				= "DashedLine",
		[PCAD_LINESTYLE_DOTTEDLINE]				= "DottedLine"
		}
	};
/*============================================================================*/
const parseenum_t	EndStyles	=
	{
	.numitems	= 4,
	.items		=
		{
		[PCAD_ENDSTYLE_NONE]					= "",
		[PCAD_ENDSTYLE_LEFTLEAD]				= "LeftLead",
		[PCAD_ENDSTYLE_RIGHTLEAD]				= "RightLead",
		[PCAD_ENDSTYLE_TWOLEADS]				= "TwoLeads"
		}
	};
/*============================================================================*/
const parseenum_t	AltTypes	=
	{
	.numitems	= 3,
	.items		=
		{
		[PCAD_ALTTYPE_NORMAL]					= "Normal",			/* Note: "Normal" doesn't exist, it is the default by omission. */
		[PCAD_ALTTYPE_DEMORGAN]					= "DeMorgan",
		[PCAD_ALTTYPE_IEEE]						= "IEEE"
		}
	};
/*============================================================================*/
const parseenum_t PortTypes	=
	{
	.numitems	= 24,
	.items		=
		{
		[PCAD_PORTTYPE_NOANGLE_SGL_HORZ]		= "NoAngle_Sgl_Horz",
		[PCAD_PORTTYPE_LEFTANGLE_SGL_HORZ]		= "LeftAngle_Sgl_Horz",
		[PCAD_PORTTYPE_RIGHTANGLE_SGL_HORZ]		= "RightAngle_Sgl_Horz",
		[PCAD_PORTTYPE_BOTHANGLE_SGL_HORZ]		= "BothAngle_Sgl_Horz",
		[PCAD_PORTTYPE_VERTLINE_SGL_HORZ]		= "VertLine_Sgl_Horz",
		[PCAD_PORTTYPE_NOOUTLINE_SGL_HORZ]		= "NoOutline_Sgl_Horz",

		[PCAD_PORTTYPE_NOANGLE_SGL_VERT]		= "NoAngle_Sgl_Vert",
		[PCAD_PORTTYPE_LEFTANGLE_SGL_VERT]		= "LeftAngle_Sgl_Vert",
		[PCAD_PORTTYPE_RIGHTANGLE_SGL_VERT]		= "RightAngle_Sgl_Vert",
		[PCAD_PORTTYPE_BOTHANGLE_SGL_VERT]		= "BothAngle_Sgl_Vert",
		[PCAD_PORTTYPE_VERTLINE_SGL_VERT]		= "VertLine_Sgl_Vert",
		[PCAD_PORTTYPE_NOOUTLINE_SGL_VERT]		= "NoOutline_Sgl_Vert",

		[PCAD_PORTTYPE_NOANGLE_DBL_HORZ]		= "NoAngle_Dbl_Horz",
		[PCAD_PORTTYPE_LEFTANGLE_DBL_HORZ]		= "LeftAngle_Dbl_Horz",
		[PCAD_PORTTYPE_RIGHTANGLE_DBL_HORZ]		= "RightAngle_Dbl_Horz",
		[PCAD_PORTTYPE_BOTHANGLE_DBL_HORZ]		= "BothAngle_Dbl_Horz",
		[PCAD_PORTTYPE_VERTLINE_DBL_HORZ]		= "VertLine_Dbl_Horz",
		[PCAD_PORTTYPE_NOOUTLINE_DBL_HORZ]		= "NoOutline_Dbl_Horz",

		[PCAD_PORTTYPE_NOANGLE_DBL_VERT]		= "NoAngle_Dbl_Vert",
		[PCAD_PORTTYPE_LEFTANGLE_DBL_VERT]		= "LeftAngle_Dbl_Vert",
		[PCAD_PORTTYPE_RIGHTANGLE_DBL_VERT]		= "RightAngle_Dbl_Vert",
		[PCAD_PORTTYPE_BOTHANGLE_DBL_VERT]		= "BothAngle_Dbl_Vert",
		[PCAD_PORTTYPE_VERTLINE_DBL_VERT]		= "VertLine_Dbl_Vert",
		[PCAD_PORTTYPE_NOOUTLINE_DBL_VERT]		= "NoOutline_Dbl_Vert"
		}
	};
/*============================================================================*/
const parseenum_t PinType	=
	{
	.numitems	= 12,
	.items		=
		{
		[PCAD_PINTYPE_NONE]						= "",
		[PCAD_PINTYPE_UNKNOWN]					= "Unknown",
		[PCAD_PINTYPE_PASSIVE]					= "Passive",
		[PCAD_PINTYPE_INPUT]					= "Input",
		[PCAD_PINTYPE_OUTPUT]					= "Output",
		[PCAD_PINTYPE_BIDIRECTIONAL]			= "Bidirectional",
		[PCAD_PINTYPE_OPEN_H]					= "Open-H",
		[PCAD_PINTYPE_OPEN_L]					= "Open-L",
		[PCAD_PINTYPE_PASSIVE_H]				= "Passive-H",
		[PCAD_PINTYPE_PASSIVE_L]				= "Passive-L",
		[PCAD_PINTYPE_3_STATE]					= "3-State",
		[PCAD_PINTYPE_POWER]					= "Power"
		}
	};
/*============================================================================*/
const parseenum_t PortPinLengths	=
	{
	.numitems	= 2,
	.items		=
		{
		[PCAD_PORTPINLENGTH_PORTPINLONG]		= "PortPinLong",	/* Note: "PortPinLong" doesn't exist, it is the default by omission. */
		[PCAD_PORTPINLENGTH_PORTPINSHORT]		= "PortPinShort"
		}
	};
/*============================================================================*/
const parseenum_t InsideEdgeStyle	=
	{
	.numitems	= 2,
	.items		=
		{
		[PCAD_INSIDEEDGESTYLE_NONE]				= "None",			/* Note: "None" doesn't exist, it is the default by omission. */
		[PCAD_INSIDEEDGESTYLE_CLOCK]			= "Clock"
		}
	};
/*============================================================================*/
const parseenum_t InsideStyle	=
	{
	.numitems	= 12,
	.items		=
		{
		[PCAD_INSIDESTYLE_NONE]					= "None",			/* Note: "None" doesn't exist, it is the default by omission. */
		[PCAD_INSIDESTYLE_AMPLIFIER]			= "Amplifier",
		[PCAD_INSIDESTYLE_GENERATOR]			= "Generator",
		[PCAD_INSIDESTYLE_HYSTERESIS]			= "Hysteresis",
		[PCAD_INSIDESTYLE_OPEN]					= "Open",
		[PCAD_INSIDESTYLE_OPENHIGH]				= "OpenHigh",
		[PCAD_INSIDESTYLE_OPENLOW]				= "OpenLow",
		[PCAD_INSIDESTYLE_PASSIVEDOWN]			= "PassiveDown",
		[PCAD_INSIDESTYLE_PASSIVEUP]			= "PassiveUp",
		[PCAD_INSIDESTYLE_POSTPONED]			= "Postponed",
		[PCAD_INSIDESTYLE_SHIFT]				= "Shift",
		[PCAD_INSIDESTYLE_THREESTATE]			= "ThreeState"
		}
	};
/*============================================================================*/
const parseenum_t OutsideEdgeStyle	=
	{
	.numitems	= 4,
	.items		=
		{
		[PCAD_OUTSIDEEDGESTYLE_NONE]			= "None",			/* Note: "None" doesn't exist, it is the default by omission. */
		[PCAD_OUTSIDEEDGESTYLE_DOT]				= "Dot",
		[PCAD_OUTSIDEEDGESTYLE_POLARITYIN]		= "PolarityIn",
		[PCAD_OUTSIDEEDGESTYLE_POLARITYOUT]		= "PolarityOut"
		}
	};
/*============================================================================*/
const parseenum_t OutsideStyle	=
	{
	.numitems	= 7,
	.items		=
		{
		[PCAD_OUTSIDESTYLE_NONE]				= "None",			/* Note: "None" doesn't exist, it is the default by omission. */
		[PCAD_OUTSIDESTYLE_ANALOG]				= "Analog",
		[PCAD_OUTSIDESTYLE_DIGITAL]				= "Digital",
		[PCAD_OUTSIDESTYLE_FLOWBI]				= "FlowBi",
		[PCAD_OUTSIDESTYLE_FLOWIN]				= "FlowIn",
		[PCAD_OUTSIDESTYLE_FLOWOUT]				= "FlowOut",
		[PCAD_OUTSIDESTYLE_NONLOGIC]			= "NonLogic"
		}
	};
/*============================================================================*/
const parseenum_t FontType	=
	{
	.numitems	= 2,
	.items		=
		{
		[PCAD_FONTTYPE_STROKE]					= "Stroke",
		[PCAD_FONTTYPE_TRUETYPE]				= "TrueType"
		}
	};
/*============================================================================*/
const parseenum_t NumType	=
	{
	.numitems	= 2,
	.items		=
		{
		[PCAD_NUMTYPE_ALPHABETIC]				= "Alphabetic",
		[PCAD_NUMTYPE_NUMERIC]					= "Numeric"
		}
	};
/*============================================================================*/
const parseenum_t NumDirection	=
	{
	.numitems	= 2,
	.items		=
		{
		[PCAD_NUMTYPE_ASCENDING]				= "ascending",
		[PCAD_NUMTYPE_DESCENDING]				= "descending"
		}
	};
/*============================================================================*/
const parseenum_t Composition	=
	{
	.numitems	= 2,
	.items		=
		{
		[PCAD_COMPOSITION_HOMOGENEOUS]			= "Homogeneous",		/* Note: "Homogeneous" does not exist, it is the default by omission. */
		[PCAD_COMPOSITION_HETEROGENEOUS]		= "Heterogeneous"
		}
	};
/*============================================================================*/
const parseenum_t CompType	=
	{
	.numitems	= 5,
	.items		=
		{
		[PCAD_COMPTYPE_NORMAL]					= "Normal",			/* Note: "Normal" does not exist, it is the default by omission. */
		[PCAD_COMPTYPE_POWER]					= "Power",
		[PCAD_COMPTYPE_SHEETCONNECTOR]			= "SheetConnector",
		[PCAD_COMPTYPE_MODULE]					= "Module",
		[PCAD_COMPTYPE_LINK]					= "Link"
		}
	};
/*============================================================================*/
const parseenum_t PageSize	=
	{
	.numitems	= 12,
	.items		=
		{
		[PCAD_PAGESIZE_SIZE_A]					= "Size_A",
		[PCAD_PAGESIZE_SIZE_B]					= "Size_B",
		[PCAD_PAGESIZE_SIZE_C]					= "Size_C",
		[PCAD_PAGESIZE_SIZE_D]					= "Size_D",
		[PCAD_PAGESIZE_SIZE_E]					= "Size_E",
		[PCAD_PAGESIZE_SIZE_A4]					= "Size_A4",
		[PCAD_PAGESIZE_SIZE_A3]					= "Size_A3",
		[PCAD_PAGESIZE_SIZE_A2]					= "Size_A2",
		[PCAD_PAGESIZE_SIZE_A1]					= "Size_A1",
		[PCAD_PAGESIZE_SIZE_A0]					= "Size_A0",
		[PCAD_PAGESIZE_SCALETOFITPAGE]			= "ScaleToFitPage",
		[PCAD_PAGESIZE_USER]					= "User"
		}
	};
/*============================================================================*/
const parseenum_t	ReportStyles	=
	{
	.numitems	= 2,
	.items		=
		{
		[PCAD_REPORTSTYLE_ACCEL]				= "reportStyleAccel",
		[PCAD_REPORTSTYLE_COMMA]				= "reportStyleComma"
		}
	};
/*============================================================================*/
const parseenum_t	ReportTypes	=
	{
	.numitems	= 8,
	.items		=
		{
		[PCAD_REPORTTYPE_ATTRIBUTES]			= "ReportTypeAttributes",
		[PCAD_REPORTTYPE_BILLOFMATERIALS]		= "ReportTypeBillOfMaterials",
		[PCAD_REPORTTYPE_GLOBALNETS]			= "ReportTypeGlobalNets",
		[PCAD_REPORTTYPE_LASTREFDES]			= "ReportTypeLastRefdes",
		[PCAD_REPORTTYPE_LIBRARY]				= "ReportTypeLibrary",
		[PCAD_REPORTTYPE_PARTSLOCATIONS]		= "ReportTypePartsLocations",
		[PCAD_REPORTTYPE_PARTSUSAGE]			= "ReportTypePartsUsage",
		[PCAD_REPORTTYPE_VARIANT]				= "ReportTypeVariant"
		}
	};
/*============================================================================*/
const parseenum_t	ReportDestinations	=
	{
	.numitems	= 3,
	.items		=
		{
		[PCAD_REPORTDESTINATIONS_SCREEN]		= "DestinationScreen",
		[PCAD_REPORTDESTINATIONS_FILE]			= "DestinationFile",
		[PCAD_REPORTDESTINATIONS_PRINTER]		= "DestinationPrinter"
		}
	};
/*============================================================================*/
const parseenum_t	ReportPropertyTypes	=
	{
	.numitems	= 15,
	.items		=
		{
		[PCAD_PROPERTYTYPE_COMPVALUE]			= "PropertyTypeCompValue",
		[PCAD_PROPERTYTYPE_COMPONENTLIBRARY]	= "PropertyTypeComponentLibrary",
		[PCAD_PROPERTYTYPE_COMPONENTNAME]		= "PropertyTypeComponentName",
		[PCAD_PROPERTYTYPE_COUNT]				= "PropertyTypeCount",
		[PCAD_PROPERTYTYPE_LOCATIONX]			= "PropertyTypeLocationX",
		[PCAD_PROPERTYTYPE_LOCATIONY]			= "PropertyTypeLocationY",
		[PCAD_PROPERTYTYPE_NETNAME]				= "PropertyTypeNetname",
		[PCAD_PROPERTYTYPE_PATTERNNAME]			= "PropertyTypePatternName",
		[PCAD_PROPERTYTYPE_REFDES]				= "PropertyTypeRefdes",
		[PCAD_PROPERTYTYPE_ROTATION]			= "PropertyTypeRotation",
		[PCAD_PROPERTYTYPE_SHEETNUMBER]			= "PropertyTypeSheetnumber",
		[PCAD_PROPERTYTYPE_TYPE]				= "PropertyTypeType",
		[PCAD_PROPERTYTYPE_UNUSEDPARTS]			= "PropertyTypeUnusedParts",
		[PCAD_PROPERTYTYPE_USER]				= "PropertyTypeUser",
		[PCAD_PROPERTYTYPE_VARIANT]				= "PropertyTypeVariant"
		}
	};
/*============================================================================*/
const parseenum_t SortTypes	=
	{
	.numitems	= 3,
	.items		=
		{
		[PCAD_SORTTYPES_NONE]					= "None",
		[PCAD_SORTTYPES_ASCENDING]				= "ascending",
		[PCAD_SORTTYPES_DESCENDING]				= "descending"
		}
	};
/*============================================================================*/
