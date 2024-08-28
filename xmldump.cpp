//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// xmldump.cpp -- Source code example for using the NomXML library.
//                This program scans the contents of an XML file and dumps
//                the contents to the console.
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
// If we parse an XML file that looks like this:
//
//   <?xml version="1.0" encoding="ISO-8859-1"?>
//   <!-- Edited by XMLSpy® -->
//   <note>
//      <to>Mary</to>
//      <from>Jane</from>
//      <heading>Reminder</heading>
//      <body>Don't forget the meetup this weekend!</body>
//   </note>
//
// This program will output something like the following to the console:
//
//    BEGIN DUMP OF FILE 'note.xml'
//        BEGIN 'xml', offset=0
//            ATTRIBUTE 0:  'version'='1.0'
//            ATTRIBUTE 1:  'encoding'='ISO-8859-1'
//        END 'xml'
//        BEGIN 'note', offset=73
//            BEGIN 'to', offset=82
//                NAME 'to', VALUE 'Mary'
//            END 'to'
//            BEGIN 'from', offset=98
//                NAME 'from', VALUE 'Jane'
//            END 'from'
//            BEGIN 'heading', offset=118
//                NAME 'heading', VALUE 'Reminder'
//            END 'heading'
//            BEGIN 'body', offset=148
//                NAME 'body', VALUE 'Don't forget the meetup this weekend!'
//            END 'body'
//        END 'note'
//    END DUMP OF FILE 'note.xml'
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#include "nomxml.h"
#include <stdlib.h>
#include <stdio.h>

//--------------------------------------------------------------------
// Indent the given number of tabular levels to the console.
// Each tab consists of four spaces.
//--------------------------------------------------------------------
static void indent(size_t level)
{
   for (size_t iter = 0; iter < level; ++iter)
      printf("    ");
}

//--------------------------------------------------------------------
// Parse XML using the given parser, sending the parsed information
// to stdout.  Returns true if successful.
//--------------------------------------------------------------------
bool parse(nomxml::XmlParser &xml)
{
   size_t nestlevel = 1;
   std::unique_ptr<nomxml::XmlNodeBase> node(nullptr);

   while (xml.NextNode(node))
   {
      if (!node)
      {
         printf("Empty node retrieved but NextNode didn't return false!\n");
         return false;
      }

      switch(node->m_Type)
      {
         case nomxml::XmlNodeBase::XmlNodeType_Begin:
         {
            const nomxml::XmlBeginNode *p = dynamic_cast<const nomxml::XmlBeginNode *>(node.get());
            if (!p)
            {
               printf("Node type said XmlNodeType_Begin but dynamic cast failed!\n");
               return false;
            }
            indent(nestlevel);
            wprintf(L"BEGIN '%s', offset=%Iu\n", p->m_Name.c_str(), p->m_Offset);
            for (size_t iter = 0; iter < p->m_Attribs.size(); ++iter)
            {
               indent(nestlevel + 1);
               wprintf(L"ATTRIBUTE %Iu:  '%s'='%s'\n", iter, p->m_Attribs[iter].m_Name.c_str(), p->m_Attribs[iter].m_Value.c_str());
            }
            ++nestlevel;
            break;
         }

         case nomxml::XmlNodeBase::XmlNodeType_Value:
         {
            const nomxml::XmlValueNode *p = dynamic_cast<const nomxml::XmlValueNode *>(node.get());
            if (!p)
            {
               printf("Node type said XmlNodeType_Value but dynamic cast failed!\n");
               return false;
            }
            indent(nestlevel);
            wprintf(L"NAME '%s', VALUE '%s'\n", p->m_Name.c_str(), p->m_Value.c_str());
            break;
         }

         case nomxml::XmlNodeBase::XmlNodeType_End:
         {
            const nomxml::XmlEndNode *p = dynamic_cast<const nomxml::XmlEndNode *>(node.get());
            if (!p)
            {
               printf("Node type said XmlNodeType_End but dynamic cast failed!\n");
               return false;
            }
            --nestlevel;
            indent(nestlevel);
            wprintf(L"END '%s'\n", p->m_Name.c_str());
            break;
         }

         default:
            printf("Invalid node type!\n");
            return false;
      }
   }

   std::wstring errtext;
   xml.ErrorInfo(errtext);
   if (!errtext.empty())
   {
      wprintf(L"Error:  %s\n", errtext.c_str());
      wprintf(L"Near offset:  %Iu\n", xml.CurPosition());
      return false;
   }

   return true;
}

//--------------------------------------------------------------------
// Interface object for reading characters from an XML file on disk.
// This demonstrates how to hook up the XmlParser to caller-supplied
// file read routines (instead of using XmlParser's built-in file
// reading routines).
//--------------------------------------------------------------------
class MyXmlFileInputInterface : public nomxml::XmlStreamInputInterface
{
public:
   explicit MyXmlFileInputInterface(FILE *fp) : m_File(fp) { }

   ~MyXmlFileInputInterface() { }

   XmlStreamInputInterface *Clone()
   {
      return new MyXmlFileInputInterface(*this);
   }

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

private:
   FILE *m_File;
};

//--------------------------------------------------------------------
// Loads a file into memory, returning the data.
// Returns an empty buffer if the file was empty or couldn't be read.
// Any error messages is sent to the console.
//--------------------------------------------------------------------
std::vector<char> LoadFileToMemory(const wchar_t *filename)
{
   std::vector<char> buffer;

   FILE *fp = nullptr;
   if (_wfopen_s(&fp, filename, L"rb") || fp == nullptr)
   {
      wprintf(L"Failed opening file:  %s\n", filename);
      return buffer;
   }

   fseek(fp, 0, SEEK_END);
   size_t length = ftell(fp);
   if (length < 1)
   {
      wprintf(L"File is empty:  %s\n", filename);
      fclose(fp);
      return buffer;
   }
   try
   {
      buffer.resize(length);
   }
   catch(...)
   {
      wprintf(L"Out of memory loading file:  %s\n", filename);
      fclose(fp);
      buffer.clear();
      return buffer;
   }
   fseek(fp, 0, SEEK_SET);

   if (fread(buffer.data(), length, 1, fp) != 1)
   {
      wprintf(L"Failed reading data from file:  %s\n", filename);
      fclose(fp);
      return buffer;
   }
   fclose(fp);

   return buffer;
}

//--------------------------------------------------------------------
// Program entry point.  Takes standard args from the command line and
// returns EXIT_SUCCESS if no errors.
//--------------------------------------------------------------------
int wmain(int argc, wchar_t **argv)
{
   if (argc < 2 || argc > 3)
   {
      // The user needs command line help.
      wprintf(L"Usage:  xmldump filename.xml [file|memory|interface]\n");
      return EXIT_FAILURE;
   }

   const wchar_t *filename = argv[1];
   const wchar_t *readmode = (argc > 2) ? argv[2] : L"file";

   try
   {
      nomxml::XmlParser xml;
      std::unique_ptr<MyXmlFileInputInterface> myInterface;
      std::vector<char> myData;
      FILE *fp = nullptr;
   
      // Depending on which I/O interface the user asked for...
      if (_wcsicmp(readmode, L"memory") == 0)
      {
         // Load the XML file into memory before processing it.
         myData = LoadFileToMemory(filename);
         if (myData.empty())
         {
            // Couldn't read the file.
            // LoadFileToMemory has already output an error message.
            return EXIT_FAILURE;
         }
   
         if (!xml.BeginParsingFromMemory(myData.data(), myData.size()))
         {
            wprintf(L"Failed to begin parsing file:  %s\n", filename);
            return EXIT_FAILURE;
         }
      }
      else if (_wcsicmp(readmode, L"file") == 0)
      {
         // Parse the XML file directly from the file instead of from memory.
         if (!xml.BeginParsingFromFile(filename))
         {
            wprintf(L"Failed to begin parsing file:  %s\n", filename);
            return EXIT_FAILURE;
         }
      }
      else if (_wcsicmp(readmode, L"interface") == 0)
      {
         // Process the XML file using the callback interface.
         if (_wfopen_s(&fp, filename, L"rb") || fp == nullptr)
         {
            wprintf(L"Failed opening file:  %s\n", filename);
            return EXIT_FAILURE;
         }
         myInterface.reset(new MyXmlFileInputInterface(fp));
   
         if (!xml.BeginParsingFromInterface(myInterface.get()))
         {
            wprintf(L"Failed to begin parsing file:  %s\n", filename);
            return EXIT_FAILURE;
         }
      }
      else
      {
         wprintf(L"Unrecognized read mode keyword:  %s\n", readmode);
         return EXIT_FAILURE;
      }
   
      wprintf(L"BEGIN DUMP OF FILE '%s'\n", filename);
   
      if (!parse(xml))
      {
         wprintf(L"Terminating with error.\n");
         return EXIT_FAILURE;
      }
   
      xml.Reset();
      if (fp != nullptr)
         fclose(fp);

      wprintf(L"END DUMP OF FILE '%s'\n", filename);
   }
   catch(...)
   {
      wprintf(L"Exception!  Sorry, something bad happened and XmlDump has to shut down.\n");
      return EXIT_FAILURE;
   }

   return EXIT_SUCCESS;
}

