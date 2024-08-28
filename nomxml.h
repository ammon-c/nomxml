//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// nomxml.h -- Primary header file for the NomXML library.
//
// NomXML is a small, minimalist C++ library for extracting tags and data
// from XML documents.  I wrote this for use in my own educational and
// experimental programs, but you may also freely use it in yours as long
// as you abide by the following terms and conditions.
//
// (C) Copyright 2008,2015 by Ammon R. Campbell.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * The names of the authors and contributors may not be used to endorse
//       or promote products derived from this software without specific
//       prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
// SUCH DAMAGE.  IN OTHER WORDS, USE AT YOUR OWN RISK, NOT OURS.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Summary of how to use NonXML from a C++ program:
//
// * #include the NomXL.h header file in your C++ module.
//
// * Construct an XmlParser object.
//
// * Call the parser's BeginParsingFromFile, BeginParsingFromMemory, or
//   BeginParsingFromInterface member function to initiate a parsing
//   operation on your XML data.
//
// * Call the parser's NextNode member function as many times as desired
//   to parse XML nodes one-by-one from the input data.  NextNode will
//   return false when there are no more nodes left to parse.
//
// * When finished, destruct the XmlParser object or call the parser's
//   Reset member to close any open files and discard any allocated
//   memory.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Limitations / Bugs:
//
// * NomXML doesn't work with XML files larger than 2GB in size.
//
// * NomXML's built-in file reader and memory reader only work with 8-bit
//   input files.  They don't support UTF-16 text and don't fully support
//   UTF-8 text.  However, you can still use NomXML on Unicode input
//   files if you supply your own reader (via the supplied interface) to
//   do the UTF-to-ANSI conversion.  Improved support for Unicode may be
//   added in a future version.
//
// * NomXML skips any CDATA chunks in the XML file.  Extracting the
//   CDATA contents may be added in a future version.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#ifdef _MSC_VER
# pragma once
#endif
#ifndef __NOMXML_INCLUDED
#define __NOMXML_INCLUDED

#include <vector>
#include <memory>
#include <string>
#include <stdio.h>

namespace nomxml {

//----------------------------------------------------------
// Describes one attribute from an XML tag.
// Typically appears as "attribname=value" in an XML tag.
// Could have used an std::pair for this, but then these
// objects become a pain to work with in the VS debugger
// with the template indirection.
//----------------------------------------------------------
struct XmlAttribute
{
   std::wstring m_Name;
   std::wstring m_Value;
};

//----------------------------------------------------------
// Base class of all XML node classes.
//----------------------------------------------------------
class XmlNodeBase
{
public:
   typedef enum { XmlNodeType_Invalid, XmlNodeType_Begin,
                  XmlNodeType_Value, XmlNodeType_End } XmlNodeType;

   XmlNodeType m_Type;     // What kind of node is this.
   std::wstring m_Name;    // The node's tag name/title text, if applicable.

   // Construction and destruction.
   explicit XmlNodeBase(XmlNodeType t) : m_Type(t) { }
   XmlNodeBase(const wchar_t *name, XmlNodeType t) : m_Name(name), m_Type(t) { }
   XmlNodeBase(const std::wstring &name, XmlNodeType t) : m_Name(name), m_Type(t) { }
   XmlNodeBase(const XmlNodeBase &copy) = default;
   virtual ~XmlNodeBase() { }

   // Return a copy of this node (allocated on the heap via new).
   virtual XmlNodeBase *Clone()=0;

   // Reset this node's contents to the empty/default state.
   virtual void clear()=0;

private:
   XmlNodeBase() { }
};

//----------------------------------------------------------
// Node for an opening XML tag, including any optional
// attributes.
//
// A typical opening tag looks like this in an XML file:
//    <tagname attrib1=value1 attrib2=value2>
//----------------------------------------------------------
class XmlBeginNode : public XmlNodeBase
{
public:
   std::vector<XmlAttribute> m_Attribs;
   size_t m_Offset;  // Byte offset of the start of this tag in the XML document, or zero if not specified.

   XmlBeginNode() : XmlNodeBase(XmlNodeType_Begin), m_Offset(0) { }

   // Return a copy of this node.
   XmlNodeBase *Clone(void)
   {
      return new XmlBeginNode(*this);
   }

   // Reset this node's contents to the empty/default state.
   void clear()
   {
      m_Name.clear();
      m_Offset = 0;
      m_Attribs.clear();
   }
};

//----------------------------------------------------------
// Node for the value content text that may appear between
// an opening and closing tag.
//----------------------------------------------------------
class XmlValueNode : public XmlNodeBase
{
public:
   std::wstring m_Value;

   XmlValueNode() : XmlNodeBase(XmlNodeType_Value) { }

   // Return a copy of this node.
   XmlNodeBase *Clone(void)
   {
      return new XmlValueNode(*this);
   }

   // Reset the node's contents to the empty/default state.
   void clear()
   {
      m_Name.clear();
      m_Value.clear();
   }
};

//----------------------------------------------------------
// Node for a closing XML tag.
//
// A closing tag looks like this in an XML file:  </tagname>
//----------------------------------------------------------
class XmlEndNode : public XmlNodeBase
{
public:
   XmlEndNode() : XmlNodeBase(XmlNodeType_End) { }

   // Return a copy of this node.
   XmlNodeBase *Clone(void)
   {
      return new XmlEndNode(*this);
   }

   // Reset the node's contents to the empty/default state.
   void clear()
   {
      m_Name.clear();
   }
};

//----------------------------------------------------------
// Contains an XML element without its children.
// An fully qualified element consists of a beginning tag,
// an optional value, and an ending tag.
//----------------------------------------------------------
class XmlElementBase
{
public:
   XmlBeginNode   m_Begin;
   XmlValueNode   m_Value;
   XmlEndNode     m_End;

   // Reset the element's contents to the empty/default state.
   void clear() { *this = XmlElementBase(); }
};

//----------------------------------------------------------
// Contains an XML element and its children.
//----------------------------------------------------------
class XmlElementTree : public XmlElementBase
{
public:
   std::vector<XmlElementTree> m_Children;

   // Dump this node's contents to the console.
   void dump(std::wstring &text, size_t indent = 0);
};

//---------------------------------------------------------------
// Base class for file reader interface.  You may provide your
// own derivative of this if you wish to use your own functions
// to pass characters from the XML file to the parser instead
// of using one of the XmlParser's built-in file readers.
//---------------------------------------------------------------
class XmlStreamInputInterface
{
public:
   XmlStreamInputInterface() { }
   virtual ~XmlStreamInputInterface() { }

   // Return a new copy of this object.
   virtual XmlStreamInputInterface *Clone(void) = 0;

   // Retrieve the total number of characters in the file.
   virtual size_t GetFileLength(void) = 0;

   // Seek to a particular character in the input file.
   // Returns true if successful.
   virtual bool Seek(size_t offset) = 0;

   // Read the next character from the file.
   // Returns true if successful.
   // Returns false if error or past end of file.
   virtual bool ReadChar(wchar_t &c) = 0;

   // Returns true if end of file has been reached.
   virtual bool EndOfFile(void) = 0;
};

//---------------------------------------------------------------
// Parses an XML document into XmlNodes.
// The input data may be from a file or a block of memory.
//---------------------------------------------------------------
class XmlParser
{
public:

   XmlParser();
   ~XmlParser();

   // Begin parsing XML document contained in file {filename}.
   // Returns false if error.
   //
   bool BeginParsingFromFile(const wchar_t *filename);
   bool BeginParsingFromFile(const char *filename);

   // Begin parsing XML document contained in the memory block pointed to by {data}.
   // The memory block is {numbytes} in size.
   // The memory block must remain accessible until parsing is completed.
   // Returns false if error.
   //
   bool BeginParsingFromMemory(void *data, size_t numbytes);

   // Begin parsing XML document using the specified interface to read the file.
   // Returns false if error.
   //
   bool BeginParsingFromInterface(XmlStreamInputInterface *iface);

   // Parse the next XmlNode from the XML document.
   // Returns false if parsing error or no more nodes in document.
   //
   bool NextNode(std::unique_ptr<XmlNodeBase> &ptrref);

   // Retrieve a description of the most recent error into {errorinfo}.
   // If no error, {errorinfo} will be empty string.
   //
   void ErrorInfo(std::wstring &errorinfo);

   // Discard all allocated memory and close all files.
   void Reset(void);

   // Returns true if the end of the XML document has been reached.
   bool EndOfDocument(void);

   // Retrieves the current character position in the XML document.
   size_t CurPosition(void);

private:

   // Stack of nested elements for the current parse operation.
   std::vector<XmlElementBase> m_Stack;

   // Last type of tag parsed.
   XmlNodeBase::XmlNodeType m_PriorTagType;

   // True if last start tag parsed was actually an empty tag.
   bool m_PriorWasEmptyTag;

   // Name of last tag parsed.
   // Only valid if m_PriorWasEmptyTag == true
   //
   std::wstring m_PriorName;

   // Interface to input file, if m_Reader != nullptr
   std::unique_ptr<XmlStreamInputInterface> m_Reader;

   // Used for tracking position in memory block or file
   size_t m_DataSize;
   size_t m_DataPos;

   // Description of most recent error.
   // Example:  "Premature end of document"
   //
   std::wstring m_ErrorInfo;

   // Current character from XML document.
   // Populated each time NextChar() is called.
   // Zero if no character available.
   //
   wchar_t m_CurChar;

   // Current token from XML document.
   // Populated each time NextToken() is called.
   //
   std::wstring m_CurToken;

   // Non-copyable.
   XmlParser(const XmlParser &copy);

   // Internal helper functions.  See nomxml.cpp for details.
   const std::wstring & CurToken(void);
   wchar_t  CurChar(void);
   bool     NextChar(void);
   bool     NextToken(const wchar_t *delims=nullptr);
   bool     ParseTagValueNode(const std::wstring &prefix, std::unique_ptr<XmlNodeBase> &ptrref);
   bool     ParseEndTagNode(std::unique_ptr<XmlNodeBase> &ptrref);
   bool     EatComment(void);
   bool     EatMarkedSection(void);
   bool     ParseBangTag(std::unique_ptr<XmlNodeBase> &ptrref);
   bool     ParseBeginTagNode(std::unique_ptr<XmlNodeBase> &ptrref);
   bool     BeginParsingImpl(void);
};

}  // End namespace nomxml

#endif  //__NOMXML_INCLUDED

