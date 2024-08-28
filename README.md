# NomXML - Yet another XML parser for C++

---

                           PLEASE NOTE!

             THIS IS AN ARCHIVE OF AN OLDER PROJECT
           THAT HAS NOT BEEN UPDATED FOR SEVERAL YEARS

---

**Description:**

NomXML is a module for converting raw XML into a format that's
useful to a C++ program.  I wrote this as a self-learning
exercise, not because the world needs yet another XML library
(it doesn't).  

**Language:** C++

**Platform:** Windows

**Source Files:**

* nomxml.h: C++ header for the NomXML module. This would be included by any program that uses NomXML.

* nomxml.cpp: C++ implementation for the NomXML module.

* xmldump.cpp: Minimal test program. It reads an XML file and outputs a detailed dump of the XML tags to the console. 

**To Do List (Unfinished Stuff):**

* Add the ability to extract data from CDATA chunks.
* Add support for reading XML files with UTF-8 or UTF-16 character encodings.
* Add support for prefixes on tag names, or a quick way to examine just the prefix or just the suffix of a node.

