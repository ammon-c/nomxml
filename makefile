# Build script for NomXML code

.SUFFIXES: .cpp

.cpp.obj:
    cl -c -W4 -EHsc -Zi $<

all: xmldump.exe

xmldump.exe: nomxml.obj xmldump.obj
    link /NOLOGO /DEBUG /OUT:xmldump.exe xmldump.obj nomxml.obj

nomxml.obj:   nomxml.cpp   nomxml.h
xmldump.obj:  xmldump.cpp  nomxml.h

clean:
    if exist *.ilk del *.ilk
    if exist *.obj del *.obj
    if exist *.exe del *.exe
    if exist *.pdb del *.pdb
    if exist *.bak del *.bak
    if exist *.out del *.out
