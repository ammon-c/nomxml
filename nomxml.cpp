//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// nomxml.cpp -- Implementation module for the NomXML library.
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
// See additional comments in nomxml.h for information about how to use the
// XmlParser class.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#include "nomxml.h"
#include <algorithm>
#include <ctype.h>

// Define _DUMP to enable debug output to stdio.  Useful for debugging.
//#define _DUMP

namespace nomxml {

//--------------------------------------------------------------------
// Interface object for reading characters from an XML file on disk.
//--------------------------------------------------------------------
class XmlFileInputInterface : public XmlStreamInputInterface
{
public:
   explicit XmlFileInputInterface(FILE *fp) : m_File(fp) { }
   ~XmlFileInputInterface() { }

   // Retrieve the total number of characters in the file.
   size_t GetFileLength(void)
   {
      if (!m_File)
         return 0;
      fseek(m_File, 0, SEEK_END);
      size_t length = ftell(m_File);
      fseek(m_File, 0, SEEK_SET);
      return length;
   }

   // Seek to a particular character in the input file.
   // Returns true if successful.
   bool Seek(size_t offset)
   {
#ifdef _DUMP
      wprintf(L"Seek to %Iu\n", offset);
#endif
      if (!m_File)
         return false;
      return (fseek(m_File, static_cast<long>(offset), SEEK_SET) == 0);   // Zero indicates success.
   }

   // Read the next character from the file.
   // Returns true if successful.
   // Returns false if error or past end of file.
   bool ReadChar(wchar_t &c)
   {
      if (!m_File)
      {
         c = 0;
         return false;
      }
      unsigned char cc;
      if (fread(&cc, 1, 1, m_File) != 1)
      {
         c = 0;
         return false;
      }
      c = static_cast<wchar_t>(cc);
      return true;
   }

   // Returns true if end of file has been reached.
   bool EndOfFile(void)
   {
      if (!m_File)
         return true;
      return (feof(m_File) != 0);
   }

   // Close the file immediately.
   void ForceClose(void)
   {
      if (m_File)
         fclose(m_File);
      m_File = nullptr;
   }

   // Return a copy of this interface, allocated on the heap via new.
   XmlStreamInputInterface *Clone()
   {
      return new XmlFileInputInterface(*this);
   }

private:
   FILE *m_File;
   XmlFileInputInterface() : m_File(nullptr) { }
};

//--------------------------------------------------------------------
// Interface object for reading characters from an XML file in memory.
//--------------------------------------------------------------------
class XmlMemoryInputInterface : public XmlStreamInputInterface
{
private:
   const char *m_Data;
   size_t      m_DataSize;
   size_t      m_DataPos;

public:
   XmlMemoryInputInterface(const void *data, size_t datasize) : m_Data(static_cast<const char *>(data)), m_DataSize(datasize), m_DataPos(0) { }

   ~XmlMemoryInputInterface() { }

   XmlStreamInputInterface *Clone()
   {
      return new XmlMemoryInputInterface(*this);
   }

   // Retrieve the total number of characters in the file.
   size_t GetFileLength(void)
   {
      return m_DataSize;
   }

   // Seek to a particular character in the input file.
   // Returns true if successful.
   bool Seek(size_t offset)
   {
      m_DataPos = offset;
      if (m_DataPos > m_DataSize)
      {
         m_DataPos = m_DataSize;
         return false;
      }
      return true;
   }

   // Read the next character from the file.
   // Returns true if successful.
   // Returns false if error or past end of file.
   bool ReadChar(wchar_t &c)
   {
      if (m_Data && m_DataPos < m_DataSize)
      {
         char cc = m_Data[m_DataPos++];
         c = static_cast<wchar_t>(cc);
         return true;
      }
      return false;
   }

   // Returns true if end of file has been reached.
   bool EndOfFile(void)
   {
      return (!m_Data || m_DataPos > m_DataSize);
   }
};

// Delimiter character sets used with NextToken() calls.
//
const wchar_t xmlDelimsWithEquals[] = L"=><? \r\n\t";
const wchar_t xmlDelimsWithSlash [] = L"/><? \r\n\t";

//--------------------------------------------------------------------
// Returns true if all characters in {text} are whitespace.
//--------------------------------------------------------------------
static bool IsWhiteSpace(const std::wstring &text)
{
   for (auto iter = text.begin(); iter != text.end(); ++iter)
      if (!iswspace(*iter))
         return false;
   return true;
}

//--------------------------------------------------------------------
// Append {indent} number of spaces onto {text}.
//--------------------------------------------------------------------
static void AddSpaces(std::wstring &text, size_t indent)
{
   for (size_t index = 0; index < indent; ++index)
      text += L" ";
}

//--------------------------------------------------------------------
// Construct.
//--------------------------------------------------------------------
XmlParser::XmlParser() :
   m_PriorTagType(XmlNodeBase::XmlNodeType_Invalid),
   m_PriorWasEmptyTag(false),
   m_DataSize(0), m_DataPos(0), m_CurChar('\0')
{
#ifdef _DUMP
   wprintf(L"XmlParser constructing.\n");
   fflush(stdout);
#endif
}

//--------------------------------------------------------------------
// Destruct.
//--------------------------------------------------------------------
XmlParser::~XmlParser()
{
#ifdef _DUMP
   wprintf(L"XmlParser destructing.\n");
   fflush(stdout);
#endif
   Reset();
}

//--------------------------------------------------------------------
// Begin parsing the XML document that was just opened.
// Returns false if error.
//--------------------------------------------------------------------
bool XmlParser::BeginParsingImpl(void)
{
   if (!m_Reader)
      return false;

   // Initialize file position tracking.
   //
   m_DataSize = m_Reader->GetFileLength();

#ifdef _DUMP
   wprintf(L"File length:  %Iu characters\n", m_DataSize);
   fflush(stdout);
#endif

   // Prime the current character.
   //
   if (!NextChar())
   {
      m_ErrorInfo = L"Empty document.  No XML tags found.";
      return false;
   }

   return true;
}

//--------------------------------------------------------------------
// Begin parsing XML document contained in file {filename}.
// Returns false if error.
//--------------------------------------------------------------------
bool XmlParser::BeginParsingFromFile(const wchar_t *filename)
{
#ifdef _DUMP
   wprintf(L"XmlParser::BeginParsingFromFile(\"%s\")\n", filename);
   fflush(stdout);
#endif

   Reset();
   FILE *fp = nullptr;
   if (_wfopen_s(&fp, filename, L"rb") || fp == nullptr)
   {
      m_ErrorInfo = L"Failed opening input file:  ";
      m_ErrorInfo += filename;
      return false;
   }
   XmlFileInputInterface reader(fp);
   m_Reader.reset(reader.Clone());

   return BeginParsingImpl();
}

//--------------------------------------------------------------------
// Begin parsing XML document contained in file {filename}.
// Returns false if error.
// Overload of above for callers that want to use a narrow
// filename string.
//--------------------------------------------------------------------
bool XmlParser::BeginParsingFromFile(const char *filename)
{
#ifdef _DUMP
   printf("XmlParser::BeginParsingFromFile(\"%s\")\n", filename);
   fflush(stdout);
#endif

   // Note the simple narrow-to-wide conversion doesn't take foreign
   // character code pages etc. into account.
   std::wstring filenameW;
   while (*filename)
      filenameW += static_cast<wchar_t>(*filename++);

   return BeginParsingFromFile(filenameW.c_str());
}

//--------------------------------------------------------------------
// Begin parsing XML document contained in the memory block pointed
// to by {data}.
// The memory block is {numbytes} in size.
// The memory block must remain accessible until parsing is completed.
// Returns false if error.
//--------------------------------------------------------------------
bool XmlParser::BeginParsingFromMemory(void *data, size_t numbytes)
{
#ifdef _DUMP
   printf("XmlParser::BeginParsingFromMemory(data:%p, numbytes:%Iu)\n", data, numbytes);
   fflush(stdout);
#endif

   Reset();
   XmlMemoryInputInterface reader(data, numbytes);
   m_Reader.reset(reader.Clone());

   return BeginParsingImpl();
}

//--------------------------------------------------------------------
// Begin parsing XML document using the specified interface to read
// the file.  Returns false if error.
//--------------------------------------------------------------------
bool XmlParser::BeginParsingFromInterface(XmlStreamInputInterface *iface)
{
#ifdef _DUMP
   printf("XmlParser::BeginParsingFromInterface(iface:%p)\n", iface);
   fflush(stdout);
#endif

   Reset();
   m_Reader.reset(iface->Clone());

   return BeginParsingImpl();
}

//--------------------------------------------------------------------
// Parses the value portion of an XML tag, meaning the text that
// appears between the begin tag and the end tag.  Caller is
// assumed to have already determined that it is time to parse
// the tag value.  If the caller already read some of the
// characters of the value text, they should be passed in {prefix}.
//
// The value node will be added to the current tag on the stack
// if successful, with a copy of the node being returned in {ptrref}.
//
// Returns false if error or no more data.
//--------------------------------------------------------------------
bool XmlParser::ParseTagValueNode(const std::wstring &prefix, std::unique_ptr<XmlNodeBase> &ptrref)
{
   std::wstring token = prefix;

   // Any characters between here and the next tag become part of the current
   // tag's value.
   //
   while (CurChar() != 0 && CurChar() != '<')
   {
      token += CurChar();
      NextChar();
   }

   // If the stack of tags is empty, then the XML document apparently has
   // garbage text outside any tags.  This is okay if it's just whitespace,
   // but not okay if it's anything else.
   //
   if (m_Stack.empty())
   {
      if (!IsWhiteSpace(token))
      {
         m_ErrorInfo = L"Unexpected data outside of all tags:  '";
         m_ErrorInfo += token;
         m_ErrorInfo += L"'";
      }
      // else we probably just read some whitespace at the end of
      // the XML document.
      return false;
   }
   else
   {
      XmlElementBase & celm = m_Stack[m_Stack.size() - 1];
      celm.m_Value.m_Name = celm.m_Begin.m_Name;
      celm.m_Value.m_Value = token;
      ptrref.reset(celm.m_Value.Clone());
      return true;
   }
}

//--------------------------------------------------------------------
// Parses an end tag from the XML document.  Caller is assumed
// to have already read the "</" at the beginning of the tag,
// leaving the "/" in CurChar().
//
// If successful, the current tag will be removed from the stack,
// and a pointer to the end node will be returned in {ptrref}.
//
// Returns false if error.
//--------------------------------------------------------------------
bool XmlParser::ParseEndTagNode(std::unique_ptr<XmlNodeBase> &ptrref)
{
   NextChar();    // Eat the '/'

   if (!NextToken(xmlDelimsWithEquals))
   {
      m_ErrorInfo = L"Unexpected end of input.";
      return false;
   }
   std::wstring token = CurToken();

   if (CurChar() != '>')
   {
      m_ErrorInfo = L"Expected '>' at end of tag:  ";
      m_ErrorInfo += CurToken();
      return false;
   }
   NextChar();    // Eat the '>'

   if (m_Stack.empty())
   {
      m_ErrorInfo = L"Unexpected end tag outside of all tags:  ";
      m_ErrorInfo += token;
      return false;
   }

   XmlElementBase & celm = m_Stack[m_Stack.size() - 1];
   if (wcscmp(token.c_str(), celm.m_Begin.m_Name.c_str()) != 0)
   {
      m_ErrorInfo = L"Mismatched end tag, found '";
      m_ErrorInfo += token;
      m_ErrorInfo += L"', expected '";
      m_ErrorInfo += celm.m_Begin.m_Name;
      m_ErrorInfo += L"'";
      return false;
   }
   celm.m_End.m_Name = celm.m_Begin.m_Name;
   ptrref.reset(celm.m_End.Clone());
   m_Stack.erase(m_Stack.begin() + m_Stack.size() - 1);
   return true;
}

//--------------------------------------------------------------------
// Skips the remainder of a comment, up to and including the "-->" at
// the end of the comment.  Assumes the "<!--" at the beginning of the
// comment has already been processed.
//--------------------------------------------------------------------
bool XmlParser::EatComment(void)
{
   bool done = false;

   while (!done)
   {
      if (CurChar() == '-')
      {
         NextChar();
         if (CurChar() == '-')
         {
            NextChar();
            if (CurChar() == '>')
            {
               // Found end of comment.
               done = true;
               if (!NextChar()) // Eat the '>'
               {
                  m_ErrorInfo = L"Unexpected end of input.";
                  return false;
               }
            }
         }
      }
      else
      {
         // Still inside the comment.
         if (!NextChar())
         {
            m_ErrorInfo = L"Unexpected end of input.";
            return false;
         }
      }
   }

   return true;
}

//--------------------------------------------------------------------
// Skips the remainder of a marked section, up to and including the
// "]>" at the end of the section.  Assumes the "<![" at the beginning
// of the section has already been processed.
//--------------------------------------------------------------------
bool XmlParser::EatMarkedSection(void)
{
   bool done = false;

   while (!done)
   {
      if (CurChar() == ']')
      {
         NextChar();
         if (CurChar() == ']')
         {
            NextChar();
            if (CurChar() == '>')
            {
               // Found end of section.
               done = true;
               if (!NextChar()) // Eat the '>'
               {
                  m_ErrorInfo = L"Unexpected end of input.";
                  return false;
               }
            }
         }
      }
      else
      {
         // Still inside the marked section.
         if (!NextChar())
         {
            m_ErrorInfo = L"Unexpected end of input.";
            return false;
         }
      }
   }

   return true;
}

//--------------------------------------------------------------------
// Parses a tag that starts with "<!" from the XML document.
// Caller is assumed to have already read the "<!" at the
// beginning of the tag, leaving the "!" in CurChar().
//
// If the tag is a comment, such as "<!--comment-->" the,
// comment is skipped and the next node after the comment is
// parsed and returned in {ptrref}.
//
// If the tag is a marked section, such as "<![CDATA[...]]>",
// it is skipped and the next node after the marked section
// is parsed and returned in {ptrref}.
//
// Returns false if error.
//--------------------------------------------------------------------
bool XmlParser::ParseBangTag(std::unique_ptr<XmlNodeBase> &ptrref)
{
   NextChar();       // Eat the '!'
   wchar_t c1 = CurChar();
   NextChar();       // Eat the '-' or '['

   if (c1 == '[')
   {
      EatMarkedSection();

      // Recursively parse the next node.
      return NextNode(ptrref);
   }
   else if (c1 == '-' && CurChar() == '-')
   {
      NextChar();    // Eat the '-'
      EatComment();

      // Recursively parse the next node.
      return NextNode(ptrref);
   }
   else
   {
      m_ErrorInfo = L"Malformed tag beginning with '!'";
      return false;
   }
}

//--------------------------------------------------------------------
// Parses a begin tag from the XML document.  Caller is assumed
// to have already read the "<" at the beginning of the tag,
// leaving CurChar() at the next character after the "<".
// Caller is also assumed to have already handled any possible
// end tag, comment tag, etc., that could have been initiated
// by the "<".
//
// This handles begin tags of the form:
//    <name [attrib1[=value1] [attrib2[=value2] ...]] [/]>
//    <?name [attrib1[=value1] [attrib2[=value2] ...]] ?>
//
// Note that due to some malformed XML files, the following form
// may also be encountered in the real world.  Note the slash
// at the end that should not be there.
//    <?name [attrib1[=value1] [attrib2[=value2] ...]] ?/>
//
// If successful, the a new tag will be added to the stack,
// and a pointer to the begin node will be returned in {ptrref}.
//
// Returns false if error.
//--------------------------------------------------------------------
bool XmlParser::ParseBeginTagNode(std::unique_ptr<XmlNodeBase> &ptrref)
{
#ifdef _DUMP
   wprintf(L"ParseBeginTagNode  m_DataPos=%Iu\n", m_DataPos);
#endif

   size_t tagoffset = m_DataPos - 2;

   // First check for '?' immediately after the '<'.
   bool bQuestionMarked = false;
   if (CurChar() == '?')
   {
      bQuestionMarked = true;
      NextChar();
   }

   // Get the tag's name.
   if (!NextToken(xmlDelimsWithSlash))
   {
      m_ErrorInfo = L"Unexpected end of input.";
      return false;
   }
   std::wstring token = CurToken();

   XmlElementBase elm;
   elm.m_Begin.m_Name = token;
   elm.m_Begin.m_Offset = tagoffset;

   // Now parse the tag's attributes, if any.
   while (CurChar() != '/' && CurChar() != '>' && CurChar() != '?')
   {
      NextToken(xmlDelimsWithEquals);
      std::wstring attribname = CurToken();

      XmlAttribute att;
      att.m_Name = attribname;

      if (CurChar() == '=')
      {
         NextChar();
         NextToken(xmlDelimsWithEquals);
         att.m_Value = CurToken();
      }

      elm.m_Begin.m_Attribs.push_back(att);
   }

   // If there was a starting question mark, check for the ending
   // question mark.
   if (bQuestionMarked)
   {
      if (CurChar() != '?')
      {
         // Expected trailing question mark.
         m_ErrorInfo = L"Expected '?' at end of tag.";
         return false;
      }

      // Question marked tag is always empty (has no value portion and no separate end tag).
      m_PriorWasEmptyTag = true;
      NextChar();
      elm.m_End.m_Name = elm.m_Begin.m_Name;
   }

   // Check if the tag ends with a slash, which indicates that there
   // will not be a separate ending tag for this tag.
   if (CurChar() == '/')
   {
      // Tag is empty (has no value portion and no separate end tag).
      m_PriorWasEmptyTag = true;
      NextChar();
      elm.m_End.m_Name = elm.m_Begin.m_Name;
   }

   // Lastly, check for the trailing marker.
   if (CurChar() != '>')
   {
      m_ErrorInfo = L"Expected '>' at end of tag.";
      return false;
   }
   NextChar(); // Eat the trailing '>'

   ptrref.reset(elm.m_Begin.Clone());
   m_Stack.push_back(elm);
   return true;
}

//--------------------------------------------------------------------
// Parse the next XmlNode from the XML document.
// Returns false if parsing error or no more nodes in document.
//--------------------------------------------------------------------
bool XmlParser::NextNode(std::unique_ptr<XmlNodeBase> &ptrref)
{
   ptrref.reset(nullptr);

   // If prior begin tag was actually an empty tag (begin and end in one tag),
   // we now have to generate the end tag node for it.
   if (m_PriorWasEmptyTag && !m_Stack.empty())
   {
      m_PriorWasEmptyTag = false;
      ptrref.reset(m_Stack[m_Stack.size() - 1].m_End.Clone());
      m_Stack.erase(m_Stack.begin() + m_Stack.size() - 1);
      return true;
   }

   // Accumulate any leading whitespace.
   //
   std::wstring token;
   while (iswspace(CurChar()))
   {
      token += CurChar();
      NextChar();
   }

   // Determine if we're parsing a tag or value.
   //
   if (CurChar() == '<')
   {
      // We're parsing a tag.
      // It should be in one of the following formats:
      //
      //    <name [attrib1[=value1] [attrib2[=value2] ...]] [/]>
      //    <?name [attrib1[=value1] [attrib2[=value2] ...]] ?>
      //    </name>
      //    <!-- comment -->
      //
      // Anything else is assumed to be the value text that comes between
      // a begin tag and an end tag.

      NextChar();
      if (CurChar() == '/')
      {
         // Parse end tag.
         return ParseEndTagNode(ptrref);
      }

      // Check for possible comment tag.
      if (CurChar() == '!')
      {
         // Note that this actually parses the comment and the
         // node that comes after the comment.
         //
         return ParseBangTag(ptrref);
      }

      // Must be a begin tag.
      //
      return ParseBeginTagNode(ptrref);
   }
   else
   {
      // This must be the value portion a.k.a. the content between
      // a start tag and end tag.
      //
      return ParseTagValueNode(token, ptrref);
   }
}

//--------------------------------------------------------------------
// Retrieve a description of the most recent error into {errorinfo}.
// If no error, {errorinfo} will be empty string.
//--------------------------------------------------------------------
void XmlParser::ErrorInfo(std::wstring &errorinfo)
{
   errorinfo = m_ErrorInfo;
}

// Discard all allocated memory and close all files.
//
void XmlParser::Reset(void)
{
   m_Stack.clear();
   m_PriorWasEmptyTag = false;
   m_PriorName = L"";

   XmlFileInputInterface *p = dynamic_cast<XmlFileInputInterface *>(m_Reader.get());
   if (p != nullptr)
      p->ForceClose();
   m_Reader.reset();

   m_DataSize = m_DataPos = 0;
}

//--------------------------------------------------------------------
// Returns true if the end of the XML document has been reached.
//--------------------------------------------------------------------
bool XmlParser::EndOfDocument(void)
{
   if (!m_Reader)
      return true;
   return m_Reader->EndOfFile();
}

//--------------------------------------------------------------------
// Retrieves the current character position in the XML document.
//--------------------------------------------------------------------
size_t XmlParser::CurPosition(void)
{
   return m_DataPos;
}

//--------------------------------------------------------------------
// Retrieve the current character from the XML document.
// Returns zero if no character available.
//--------------------------------------------------------------------
wchar_t XmlParser::CurChar(void)
{
   return m_CurChar;
}

//--------------------------------------------------------------------
// Advance to next character in XML document.
// Returns false if no more characters available.
//--------------------------------------------------------------------
bool XmlParser::NextChar(void)
{
   m_CurChar = 0;
   if (!m_Reader)
      return false;

   wchar_t c;
   if (!m_Reader->ReadChar(c))
   {
      return false;
   }
   m_CurChar = c;
   m_DataPos++;
   return true;
}

//--------------------------------------------------------------------
// Retrieve a reference to the current token from the XML document.
//--------------------------------------------------------------------
const std::wstring & XmlParser::CurToken(void)
{
   return m_CurToken;
}

//--------------------------------------------------------------------
// Extract next token from XML document, where token is delimited
// by double quotes or by any of the delimiters given in {delims}.
// Returns false if no more tokens.
//--------------------------------------------------------------------
bool XmlParser::NextToken(const wchar_t *delims)
{
   m_CurToken = L"";

   // Eat any leading whitespace.
   while (iswspace(CurChar()))
      NextChar();

   // How we scan the token depends on if it's quoted or not.
   if (CurChar() == '"')
   {
      // Accumulate the quoted token.
      NextChar();
      while (CurChar() != 0 && CurChar() != '"')
      {
         m_CurToken += CurChar();
         NextChar();
      }
      if (CurChar() == '"')
         NextChar();
   }
   else if (CurChar() == '\'')
   {
      // Accumulate the single-quoted token.
      NextChar();
      while (CurChar() != 0 && CurChar() != '\'')
      {
         m_CurToken += CurChar();
         NextChar();
      }
      if (CurChar() == '\'')
         NextChar();
   }
   else
   {
      // Accumulate the non-quoted token until a delimiter is hit.
      while (CurChar() != '\0' &&
             !wcschr((delims == nullptr ? L"" : delims), CurChar()))
      {
         m_CurToken += CurChar();
         NextChar();
      }
   }

   // Eat any trailing whitespace.
   while (iswspace(CurChar()))
      NextChar();

#ifdef _DUMP
   wprintf(L"token \"%s\"\n", m_CurToken.c_str());
   fflush(stdout);
#endif

   if (m_CurToken.empty() && EndOfDocument())
      return false;

   return true;
}

//--------------------------------------------------------------------
// Dump contents of this tree to caller-provided string {text},
// for human viewing.
//--------------------------------------------------------------------
void XmlElementTree::dump(std::wstring &text, size_t indent)
{
   if (m_Begin.m_Type == XmlNodeBase::XmlNodeType_Begin)
   {
      AddSpaces(text, indent);
      text += L"Begin:  ";
      text += m_Begin.m_Name;
      text += L"\n";
      for (auto attribp = m_Begin.m_Attribs.begin(); attribp != m_Begin.m_Attribs.end(); ++attribp)
      {
         text += L"  Attrib:  ";
         text += attribp->m_Name;
         text += L"=";
         text += attribp->m_Value;
         text += L"\n";
      }
   }
   if (m_Value.m_Type == XmlNodeBase::XmlNodeType_Value)
   {
      AddSpaces(text, indent + 4);
      text += L"Value:  ";
      text += m_Value.m_Value;
      text += L"\n";
   }
   if (m_End.m_Type == XmlNodeBase::XmlNodeType_End)
   {
      AddSpaces(text, indent);
      text += L"End:  ";
      text += m_End.m_Name;
      text += L"\n";
   }
   if (!m_Children.empty())
   {
      for (auto childp = m_Children.begin(); childp != m_Children.end(); ++childp)
         childp->dump(text, indent + 4);
   }
}

}  // namespace nomxml

