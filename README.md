Program to convert ASCII P-CAD 2006 Schematics to KiCAD v8.0 format. It can also output P-CAD format with all the fields sorted.

The schematic file is read, parsed and converted to an internal representation, then all the elements are sorted and output back in the desired format.

Usually P-CAD rearranges the order of elements in the file each time it is saved, so it is impossible to used diff to see what has really changed.
If the output format is P-CAD, the result is very useful to help with version control, because only real changes will reflect in the resulting file.

For now, KiCAD output is not competely correct for components with multiple parts/gates and components that have the numbering of the pins different in symbol and footprint.
