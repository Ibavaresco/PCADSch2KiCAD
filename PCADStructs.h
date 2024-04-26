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
#if			!defined __PCADSTRUCTS_H__
#define __PCADSTRUCTS_H__
 /*===========================================================================*/
#include <stdint.h>
#include "PCADEnums.h"
/*===========================================================================*/
#define	LENGTH(a)	(sizeof(a)/sizeof(a[0]))
/*===========================================================================*/
typedef int32_t								pcad_dimmension_t;
typedef int32_t								pcad_real_t;
typedef int32_t								pcad_integer_t;
typedef uint32_t							pcad_unsigned_t;
/*===========================================================================*/
typedef struct pcad_point_tag
	{
	pcad_dimmension_t						x;
	pcad_dimmension_t						y;
	struct pcad_point_tag					*next;
	} pcad_point_t;
/*=============================================================================*/
typedef struct
	{
	pcad_point_t							topleft;
	pcad_point_t							bottomright;
	} pcad_printregion_t;
/*=============================================================================*/
/*=============================================================================*/
/*=============================================================================*/
/*=============================================================================*/
typedef struct
	{
	pcad_unsigned_t							high;
	pcad_unsigned_t							low;
	} pcad_asciiversion_t;
/*=============================================================================*/
typedef struct
	{
	pcad_unsigned_t							year;
	pcad_unsigned_t							month;
	pcad_unsigned_t							day;
	pcad_unsigned_t							hour;
	pcad_unsigned_t							minute;
	pcad_unsigned_t							second;
	} pcad_timestamp_t;
/*=============================================================================*/
typedef struct
	{
	char									*name;
	char									*version;
	} pcad_program_t;
/*============================================================================*/
typedef struct pcad_asciiheader_tag
	{
	pcad_asciiversion_t						asciiversion;
	pcad_timestamp_t						timestamp;
	pcad_program_t							program;
	char									*copyright;
	char									*fileauthor;
	char									*headerstring;
	pcad_units_t							fileunits;
	char									*guidstring;
	} pcad_asciiheader_t;
/*===========================================================================*/
/*===========================================================================*/
/*===========================================================================*/
/*===========================================================================*/
typedef struct pcad_font_tag
	{
/*
	pcad_fontfamily_t						fontfamily;
*/
	pcad_enum_fonttype_t					fonttype;
	char									*fontfamily;
	char									*fontface;
	pcad_dimmension_t						fontheight;
	pcad_dimmension_t						strokewidth;
	pcad_unsigned_t							fontweight;
	pcad_unsigned_t							fontcharset;
	pcad_unsigned_t							fontoutprecision;
	pcad_unsigned_t							fontclipprecision;
	pcad_unsigned_t							fontquality;
	pcad_unsigned_t							fontpitchandfamily;
	struct pcad_font_tag					*next;
	} pcad_font_t;
/*=============================================================================*/
typedef struct pcad_textstyledef_tag
	{
	char									*name;
	pcad_font_t								*firstfont;
	pcad_boolean_t							allowttype;
	pcad_boolean_t							displayttype;
	struct pcad_textstyledef_tag			*next;
	size_t									numfonts;
	pcad_font_t								**viofonts;
	} pcad_textstyledef_t;
/*=============================================================================*/
typedef struct
	{
	pcad_dimmension_t						extentx;
	pcad_dimmension_t						extenty;
	} pcad_extent_t;
/*=============================================================================*/
typedef struct pcad_text_tag
	{
	pcad_point_t							point;
	char									*value;
	char									*textstyleref;
	pcad_real_t								rotation;
	pcad_boolean_t							isflipped;
	pcad_enum_justify_t						justify;
	pcad_extent_t							extent;
	struct pcad_text_tag					*next;
	} pcad_text_t;
/*=============================================================================*/
typedef struct pcad_pin_tag
	{
	pcad_unsigned_t							pinnum;
	pcad_point_t							point;
	pcad_real_t								rotation;
	pcad_dimmension_t						pinlength;
	pcad_enum_outsideedgestyle_t			outsideedgestyle;
	pcad_enum_insideedgestyle_t				insideedgestyle;
	pcad_enum_outsidestyle_t				outsidestyle;
	pcad_enum_insidestyle_t					insidestyle;
	pcad_boolean_t							displaypindes;
	pcad_boolean_t							displaypinname;
	pcad_boolean_t							isflipped;
	pcad_text_t								pindes;
	pcad_text_t								pinname;
	char									*defaultpindes;
	struct pcad_pin_tag						*next;
	} pcad_pin_t;
/*=============================================================================*/
typedef struct pcad_line_tag
	{
	pcad_point_t							pt1;
	pcad_point_t							pt2;
	pcad_dimmension_t						width;
	pcad_enum_linestyle_t					style;
	char									*netnameref;
	struct pcad_line_tag					*next;
	} pcad_line_t;
/*=============================================================================*/
typedef struct pcad_ieeesymbol_tag
	{
	pcad_enum_ieeesymbol_t					type;
	pcad_point_t							point;
	pcad_dimmension_t						height;
	pcad_boolean_t							isflipped;
	struct pcad_ieeesymbol_tag				*next;
	} pcad_ieeesymbol_t;
/*=============================================================================*/
typedef struct pcad_attr_tag
	{
	char									*name;
	char									*value;
	pcad_point_t							point;
	pcad_real_t								rotation;
	pcad_boolean_t							isvisible;
	pcad_enum_justify_t						justify;
	pcad_boolean_t							isflipped;
	char									*textstyleref;
	pcad_units_t							constraintunits;
	struct pcad_attr_tag					*next;
	} pcad_attr_t;
/*=============================================================================*/
typedef struct pcad_triplepointarc_tag
	{
	pcad_point_t							point1;
	pcad_point_t							point2;
	pcad_point_t							point3;
	pcad_dimmension_t						width;
	struct pcad_triplepointarc_tag			*next;
	} pcad_triplepointarc_t;
/*=============================================================================*/
typedef struct pcad_poly_tag
	{
	pcad_point_t							*firstpoint;
	struct pcad_poly_tag					*next;

	size_t									numpoints;
	pcad_point_t							**viopoints;
	} pcad_poly_t;
/*=============================================================================*/
typedef struct pcad_symboldef_tag
	{
	char									*name;
	char									*originalname;
	pcad_pin_t								*firstpin;
	pcad_line_t								*firstline;
	pcad_attr_t								*firstattr;
	pcad_triplepointarc_t					*firsttriplepointarc;
	pcad_text_t								*firsttext;
	pcad_poly_t								*firstpoly;
	pcad_ieeesymbol_t						*firstieeesymbol;
	struct pcad_symboldef_tag				*next;

	size_t									numpins;
	pcad_pin_t								**viopins;
	size_t									numlines;
	pcad_line_t								**violines;
	size_t									numattrs;
	pcad_attr_t								**vioattrs;
	size_t									numtriplepointarcs;
	pcad_triplepointarc_t					**viotriplepointarcs;
	size_t									numtexts;
	pcad_text_t								**viotexts;
	size_t									numpolys;
	pcad_poly_t								**viopolys;
	size_t									numieeesymbols;
	pcad_ieeesymbol_t						**vioieeesymbols;
	} pcad_symboldef_t;
/*=============================================================================*/
typedef struct pcad_comppin_tag
	{
	char									*name;
	char									*pinnumber;
	char									*pinname;
	pcad_unsigned_t							partnum;
	pcad_unsigned_t							sympinnum;
	pcad_unsigned_t							gateeq;
	pcad_unsigned_t							pineq;
	pcad_enum_pintype_t						pintype;
	struct pcad_comppin_tag					*next;
	} pcad_comppin_t;
/*=============================================================================*/
typedef struct pcad_alts_tag
	{
	pcad_boolean_t							ieeealt;
	pcad_boolean_t							demorganalt;
	} pcad_alts_t;
/*=============================================================================*/
typedef struct pcad_compheader_tag
	{
	char									*sourcelibrary;
	pcad_enum_comptype_t					comptype;
	pcad_unsigned_t							numpins;
	pcad_unsigned_t							numparts;
	pcad_enum_composition_t					composition;
	pcad_alts_t								alts;
	char									*refdesprefix;
	pcad_enum_numtype_t						numtype;
	} pcad_compheader_t;
/*=============================================================================*/
typedef struct pcad_attachedsymbol_tag
	{
	pcad_unsigned_t							partnum;
	char									*alttype;
	char									*symbolname;
	struct pcad_attachedsymbol_tag			*next;
	} pcad_attachedsymbol_t;
/*=============================================================================*/
typedef struct pcad_padpinmap_tag
	{
	pcad_unsigned_t							padnum;
	char									*comppinref;
	struct pcad_padpinmap_tag				*next;
	} pcad_padpinmap_t;
/*=============================================================================*/
typedef struct pcad_attachedpattern_tag
	{
	pcad_unsigned_t							patternnum;
	char									*patternname;
	pcad_unsigned_t							numpads;
	pcad_padpinmap_t						*firstpadpinmap;

	size_t									numpadpinmaps;
	pcad_padpinmap_t						**viopadpinmaps;
	} pcad_attachedpattern_t;
/*=============================================================================*/
typedef struct pcad_compdef_tag
	{
	char									*name;
	char									*originalname;
	pcad_compheader_t						compheader;
	pcad_comppin_t							*firstcomppin;
	pcad_attachedsymbol_t					*firstattachedsymbol;
	pcad_attachedpattern_t					attachedpattern;
	struct pcad_compdef_tag					*next;

	size_t									numcomppins;
	pcad_comppin_t							**viocomppins;
	size_t									numattachedsymbols;
	pcad_attachedsymbol_t					**vioattachedsymbols;
	} pcad_compdef_t;
/*=============================================================================*/
typedef struct pcad_library_tag
	{
	char									*name;
	pcad_textstyledef_t						*firsttextstyledef;
	pcad_symboldef_t						*firstsymboldef;
	pcad_compdef_t							*firstcompdef;

	size_t									numtextstyledefs;
	pcad_textstyledef_t						**viotextstyledefs;
	size_t									numsymboldefs;
	pcad_symboldef_t						**viosymboldefs;
	size_t									numcompdefs;
	pcad_compdef_t							**viocompdefs;
	} pcad_library_t;
/*===========================================================================*/
/*===========================================================================*/
/*===========================================================================*/
/*===========================================================================*/
typedef struct pcad_compinst_tag
	{
	char									*name;
	char									*compref;
	char									*originalname;
	char									*compvalue;
	char									*patternname;
	pcad_attr_t								*firstattr;
	struct pcad_compinst_tag				*next;

	size_t									numattrs;
	pcad_attr_t								**vioattrs;
	} pcad_compinst_t;
/*=============================================================================*/
typedef struct pcad_node_tag
	{
	char									*component;
	char									*pin;
	struct pcad_node_tag					*next;
	} pcad_node_t;
/*=============================================================================*/
typedef struct pcad_net_tag
	{
	char									*name;
	pcad_node_t								*firstnode;
	struct pcad_net_tag						*next;

	size_t									numnodes;
	pcad_node_t								**vionodes;
	} pcad_net_t;
/*=============================================================================*/
typedef struct pcad_globalattrs_tag
	{
	pcad_attr_t								*firstattr;

	size_t									numattr;
	pcad_attr_t								**vioattrs;
	} pcad_globalattrs_t;
/*=============================================================================*/
typedef struct pcad_netlist_tag
	{
	char									*name;
	pcad_globalattrs_t						globalattrs;
	pcad_compinst_t							*firstcompinst;
	pcad_net_t								*firstnet;

	size_t									numcompinsts;
	pcad_compinst_t							**viocompinsts;
	size_t									numnets;
	pcad_net_t								**vionets;
	} pcad_netlist_t;
/*===========================================================================*/
/*===========================================================================*/
/*===========================================================================*/
/*===========================================================================*/
typedef struct pcad_grid_tag
	{
	pcad_dimmension_t						grid;
	struct pcad_grid_tag					*next;
	} pcad_grid_t;
/*===========================================================================*/
typedef struct pcad_gridfns_tag
	{
	pcad_grid_t								*firstgrid;

	size_t									numgrids;
	pcad_grid_t								**viogrids;
	} pcad_griddfns_t;
/*===========================================================================*/
typedef struct pcad_fielddef_tag
	{
	char									*name;
	char									*value;
	struct pcad_fielddef_tag				*next;
	} pcad_fielddef_t;
/*===========================================================================*/
typedef struct pcad_note_tag
	{
	pcad_unsigned_t							number;
	char									*value;
	pcad_noteannotation_t					noteannotation;
	struct pcad_note_tag					*next;
	} pcad_note_t;
/*===========================================================================*/
typedef struct pcad_revisionnote_tag
	{
	pcad_unsigned_t							number;
	char									*value;
	struct pcad_revisionnote_tag			*next;
	} pcad_revisionnote_t;
/*===========================================================================*/
typedef struct pcad_fieldset_tag
	{
	char									*name;
	pcad_fielddef_t							*firstfielddef;
	pcad_note_t								*firstnote;
	pcad_revisionnote_t						*firstrevisionnote;
	struct pcad_fieldset_tag				*next;

	size_t									numfielddefs;
	pcad_fielddef_t							**viofielddefs;
	size_t									numnotes;
	pcad_note_t								**vionotes;
	size_t									numrevisionnotes;
	pcad_revisionnote_t						**viorevisionnotes;

	} pcad_fieldset_t;
/*===========================================================================*/
typedef struct pcad_designinfo_tag
	{
	pcad_fieldset_t							*firstfieldset;

	size_t									numfieldsets;
	pcad_fieldset_t							**viofieldsets;

	} pcad_designinfo_t;
/*===========================================================================*/
typedef struct pcad_schdesignheader_tag
	{
	pcad_extent_t							workspacesize;
	pcad_griddfns_t							griddfns;
	pcad_designinfo_t						designinfo;
	pcad_dimmension_t						refpointsize;
	pcad_dimmension_t						infopointsize;
	pcad_dimmension_t						junctionsize;
	pcad_dimmension_t						refpointsizeprint;
	pcad_dimmension_t						infopointsizeprint;
	pcad_dimmension_t						junctionsizeprint;
	} pcad_schdesignheader_t;
/*===========================================================================*/
typedef struct
	{
	pcad_dimmension_t						offsetx;
	pcad_dimmension_t						offsety;
	} pcad_offset_t;
/*===========================================================================*/
typedef struct pcad_border_tag
	{
	pcad_boolean_t							isvisible;
	pcad_dimmension_t						height;
	pcad_dimmension_t						width;
	pcad_offset_t							offset;
	} pcad_border_t;
/*===========================================================================*/
typedef struct pcad_hvzones_tag
	{
	pcad_unsigned_t							count;
	pcad_enum_numdirection_t				numdirection;
	pcad_enum_numtype_t						numtype;
	} pcad_hvzones_t;
/*===========================================================================*/
typedef struct pcad_zones_tag
	{
	pcad_boolean_t							isvisible;

	pcad_hvzones_t							horizontalzones;
	pcad_hvzones_t							verticalzones;
	char									*textstyleref;
	} pcad_zones_t;
/*===========================================================================*/
typedef struct pcad_titlesheet_tag
	{
	char									*name;
	pcad_real_t								scale;
	pcad_boolean_t							isvisible;
	pcad_offset_t							offset;
	pcad_border_t							border;
	pcad_zones_t							zones;
	pcad_line_t								*firstline;
	pcad_text_t								*firsttext;

	size_t									numlines;
	pcad_line_t								**violines;
	size_t									numtexts;
	pcad_text_t								**viotexts;
	} pcad_titlesheet_t;
/*===========================================================================*/
typedef struct pcad_region_tag
	{
	pcad_point_t							p1;
	pcad_point_t							p2;
	} pcad_region_t;
/*=============================================================================*/
typedef struct pcad_wire_tag
	{
	pcad_point_t							pt1;
	pcad_endstyle_t							endstyle1;
	pcad_point_t							pt2;
	pcad_endstyle_t							endstyle2;
	pcad_dimmension_t						width;
	char									*netnameref;
	pcad_boolean_t							dispname;
	pcad_text_t								text;
	struct pcad_wire_tag					*next;
	} pcad_wire_t;
/*=============================================================================*/
typedef struct pcad_bus_tag
	{
	char									*name;
	pcad_point_t							pt1;
	pcad_point_t							pt2;
	pcad_boolean_t							dispname;
	pcad_text_t								*text;
	pcad_text_t								**viotexts;
	struct pcad_bus_tag						*next;
	} pcad_bus_t;
/*=============================================================================*/
typedef struct pcad_busentry_tag
	{
	char									*busnameref;
	pcad_point_t							point;
	pcad_orient_t							orient;
	struct pcad_busentry_tag				*next;
	} pcad_busentry_t;
/*=============================================================================*/
typedef struct	pcad_symbol_tag
	{
	char									*symbolref;
	char									*refdesref;
	pcad_unsigned_t							partnum;
	pcad_point_t							pt;
	pcad_real_t								rotation;
	pcad_boolean_t							isflipped;
	pcad_attr_t								*firstattr;

	size_t									numattrs;
	pcad_attr_t								**vioattrs;

	struct pcad_symbol_tag					*next;
	} pcad_symbol_t;
/*=============================================================================*/
typedef struct pcad_junction_tag
	{
	pcad_point_t							point;
	char									*netnameref;
	struct pcad_junction_tag				*next;
	} pcad_junction_t;
/*=============================================================================*/
typedef struct pcad_port_tag
	{
	pcad_point_t							point;
	pcad_enum_porttype_t					porttype;
	pcad_enum_portpinlength_t				portpinlength;
	char									*netnameref;
	pcad_real_t								rotation;
	pcad_boolean_t							isflipped;
	struct pcad_port_tag					*next;
	} pcad_port_t;
/*=============================================================================*/
typedef struct pcad_field_tag
	{
	char									*name;
	pcad_point_t							point;
	pcad_boolean_t							isflipped;
	pcad_enum_justify_t						justify;
	char									*textstyleref;
	struct pcad_field_tag					*next;
	} pcad_field_t;
/*=============================================================================*/
typedef struct pcad_refpoint_tag
	{
	pcad_point_t							point;
	struct pcad_refpoint_tag				*next;
	} pcad_refpoint_t;
/*=============================================================================*/
typedef struct pcad_sheet_tag
	{
	char									*name;
	pcad_titlesheet_t						titlesheet;
	pcad_unsigned_t							sheetnum;
	char									*fieldsetref;
	pcad_boolean_t							drawborder;
	pcad_boolean_t							entiredesign;
	pcad_boolean_t							isrotated;
	pcad_enum_pagesize_t					pagesize;
	pcad_real_t								scalefactor;
	pcad_point_t							offset;
	pcad_region_t							printregion;
	pcad_unsigned_t							sheetordernum;
	pcad_bus_t								*firstbus;
	pcad_wire_t								*firstwire;
	pcad_busentry_t							*firstbusentry;
	pcad_symbol_t							*firstsymbol;
	pcad_junction_t							*firstjunction;
	pcad_port_t								*firstport;
	pcad_text_t								*firsttext;
	pcad_triplepointarc_t					*firsttriplepointarc;
	pcad_attr_t								*firstattr;
	pcad_poly_t								*firstpoly;
	pcad_line_t								*firstline;
	pcad_pin_t								*firstpin;
	pcad_ieeesymbol_t						*firstieeesymbol;
	pcad_field_t							*firstfield;
	pcad_refpoint_t							*firstrefpoint;
	struct pcad_sheet_tag					*next;

	size_t									numwires;
	pcad_wire_t								**viowires;
	size_t									numbuses;
	pcad_bus_t								**viobuses;
	size_t									numbusentries;
	pcad_busentry_t							**viobusentries;
	size_t									numsymbols;
	pcad_symbol_t							**viosymbols;
	size_t									numjunctions;
	pcad_junction_t							**viojunctions;
	size_t									numports;
	pcad_port_t								**vioports;
	size_t									numtexts;
	pcad_text_t								**viotexts;
	size_t									numtriplepointarcs;
	pcad_triplepointarc_t					**viotriplepointarcs;
	size_t									numattrs;
	pcad_attr_t								**vioattrs;
	size_t									numpolys;
	pcad_poly_t								**viopolys;
	size_t									numlines;
	pcad_line_t								**violines;
	size_t									numpins;
	pcad_pin_t								**viopins;
	size_t									numieeesymbols;
	pcad_ieeesymbol_t						**vioieeesymbols;
	size_t									numfields;
	pcad_field_t							**viofields;
	size_t									numrefpoints;
	pcad_refpoint_t							**viorefpoints;
	} pcad_sheet_t;
/*=============================================================================*/
typedef struct pcad_sheetref_tag
	{
	unsigned								sheetref;
	struct pcad_sheetref_tag				*next;
	} pcad_sheetref_t;
/*=============================================================================*/
typedef struct pcad_sheetlist_tag
		{
		pcad_sheetref_t						*firstsheetref;

		size_t								numsheetrefs;
		pcad_sheetref_t						**viosheetrefs;
		} pcad_sheetlist_t;
/*=============================================================================*/
typedef struct pcad_schematicprintst_tag
	{
	pcad_sheetlist_t						sheetlist;
	} pcad_schematicprintst_t;
/*=============================================================================*/
typedef struct pcad_currentlayer_tag
	{
	pcad_unsigned_t							layernumref;
	} pcad_currentlayer_t;
/*===========================================================================*/
typedef struct pcad_layerstate_tag
	{
	pcad_currentlayer_t						currentlayer;
	} pcad_layerstate_t;
/*===========================================================================*/
typedef struct pcad_gridstate_tag
	{
	pcad_dimmension_t						currentabsgrid;
	pcad_dimmension_t						currentrelgrid;
	pcad_boolean_t							isabsolutegrid;
	pcad_boolean_t							isdottedgrid;
	pcad_boolean_t							isvisiblegrid;
	pcad_boolean_t							ispromptforrel;
	} pcad_gridstate_t;
/*===========================================================================*/
typedef struct pcad_ecostate_tag
	{
	pcad_boolean_t							ecorecording;
	} pcad_ecostate_t;
/*===========================================================================*/
typedef struct pcad_programstate_tag
	{
	pcad_layerstate_t						layerstate;
	pcad_gridstate_t						gridstate;
	pcad_ecostate_t							ecostate;
	char									*currenttextstyle;
	} pcad_programstate_t;
/*===========================================================================*/
typedef struct pcad_reportfieldcondition_tag
	{
	char									*condition;
	struct pcad_reportfieldcondition_tag	*next;
	} pcad_reportfieldcondition_t;
/*===========================================================================*/
typedef struct pcad_reportfieldconditions_tag
	{
	pcad_reportfieldcondition_t				*firstreportfieldcondition;

	size_t									numreportfieldconditions;
	pcad_reportfieldcondition_t				**vioreportfieldconditions;
	} pcad_reportfieldconditions_t;
/*===========================================================================*/
typedef struct pcad_reportfield_tag
	{
	char									*reportfieldname;
	pcad_fieldtypes_t						reportfieldtype;
	pcad_unsigned_t							reportfieldsortorder;
	pcad_enum_numdirection_t				reportfieldsorttype;
	pcad_boolean_t							reportfieldshowflag;
	pcad_unsigned_t							reportfieldcolumnwidth;
	pcad_reportfieldconditions_t			reportfieldconditions;
	struct pcad_reportfield_tag				*next;
	} pcad_reportfield_t;
/*===========================================================================*/
typedef struct pcad_reportfields_tag
	{
	pcad_reportfield_t						*firstreportfield;

	size_t									numreportfields;
	pcad_reportfield_t						**vioreportfields;

	struct pcad_reportfields_tag			*next;
	} pcad_reportfields_t;
/*===========================================================================*/
typedef struct pcad_reportfieldssection_tag
	{
	pcad_reportfields_t						*firstreportfields;

	size_t									numreportfieldss;
	pcad_reportfields_t						**vioreportfieldss;
	} pcad_reportfieldssection_t;
/*===========================================================================*/
typedef struct pcad_reportdefinition_tag
	{
	char									*reportname;
	char									*reportextension;
	pcad_boolean_t							reportshowflag;
	pcad_reporttypes_t						reporttype;
	pcad_boolean_t							reportuserdefined;
	pcad_unsigned_t							reportlinesperpage;
	pcad_unsigned_t							reportcolumnwidth;
	pcad_boolean_t							reportuseheader;
	char									*reportheader;
	pcad_boolean_t							reportusefooter;
	char									*reportfooter;
	pcad_boolean_t							reportusedesigninfo;
	pcad_boolean_t							reportshowdate;
	pcad_boolean_t							reportpaginateflag;
	pcad_boolean_t							reportshowcdfpreface;
	pcad_boolean_t							reportshowcolumnnames;
	pcad_reportfieldssection_t				reportfieldssections;
	struct pcad_reportdefinition_tag		*next;
	} pcad_reportdefinition_t;
/*===========================================================================*/
typedef struct pcad_reportdefinitions_tag
	{
	pcad_reportdefinition_t					*firstreportdefinition;

	size_t									numreportdefinitions;
	pcad_reportdefinition_t					**vioreportdefinitions;
	} pcad_reportdefinitions_t;
/*===========================================================================*/
typedef struct pcad_reportsettings_tag
	{
	pcad_reportstyle_t						reportstyle;
	pcad_report_destination_t				reportdestination;
	pcad_point_t							reportorigin;
	pcad_reportdefinitions_t				reportdefinitions;
	} pcad_reportsettings_t;
/*===========================================================================*/
typedef struct pcad_schematicdesign_tag
	{
	char									*name;
	pcad_schdesignheader_t					schdesignheader;
	pcad_titlesheet_t						titlesheet;
	pcad_sheet_t							*firstsheet;
	pcad_schematicprintst_t					schematicPrintSettings;
	pcad_programstate_t						programstate;
	pcad_reportsettings_t					reportsettings;

	size_t									numsheets;
	pcad_sheet_t							**viosheets;
	} pcad_schematicdesign_t;
/*===========================================================================*/
/*===========================================================================*/
/*===========================================================================*/
/*===========================================================================*/
typedef struct pcad_schematicfile_tag
	{
	char									*name;
	pcad_asciiheader_t						asciiheader;
	pcad_library_t							library;
	pcad_netlist_t							netlist;
	pcad_schematicdesign_t					schematicdesign;
	} pcad_schematicfile_t;
/*============================================================================*/
#endif	/*	!defined __PCADSTRUCTS_H__ */
 /*===========================================================================*/
