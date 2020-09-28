/**********************************************************************

Filename    :   FontConfigParser.cpp
Content     :   Parsing logic for GFxPlayer font configuration file
Created     :
Authors     :   Michael Antonov

Copyright   :   (c) 2005-2006 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "FontConfigParser.h"

#include "GFxFontLib.h"
#include "GFxPlayer.h"

#include <stdarg.h>


// ***** ConfigParser implementaion

ConfigParser::ConfigParser(const char* pfilename)
{
    Pos         = 0;
    Line        = 1;

    // if absolute path, then save the parent path
    ParentPath.AppendString(pfilename);
    GFxURLBuilder::ExtractFilePath(&ParentPath);
    if (ParentPath.CompareNoCase(pfilename) == 0)
    {
        ParentPath = "";
    } 

    GSysFile file(pfilename);
    if (file.IsValid())
    {
        SInt flen = 0;
        if ((flen = file.GetLength()) != 0)
        {
            UByte* td = (UByte*) GALLOC(flen);
            file.Read(td, flen);

            // PPS: the following converts byte stream to appropriate endianness
            // PPS: only for UTF16
            UInt16* prefix16 = (UInt16*)td;

            if (prefix16[0] == GByteUtil::BEToSystem((UInt16)0xFFFE)) // little endian
            {
		        prefix16++;
                SInt numChars = (flen / 2) - 1;
                if (sizeof(wchar_t) == 2)
                {
                    UInt16* chars = (UInt16*) GALLOC(numChars * 2);
                    Buffer.Resize(numChars);
                    for (SInt i=0; i < numChars; i++)
                        chars[i] = GByteUtil::LEToSystem(prefix16[i]);
                    Buffer.SetString( ((const wchar_t*)chars), numChars );
                    GFREE(chars);
                }
                else if (sizeof(wchar_t) == 4)
                {
                    UInt32* chars = (UInt32*) GALLOC(numChars * 4);
                    Buffer.Resize(numChars);
                    for (SInt i=0; i < numChars; i++)
                        chars[i] = GByteUtil::LEToSystem(prefix16[i]);
                    Buffer.SetString( ((const wchar_t*)chars), numChars );
                    GFREE(chars);
                }
                else
                {
                    GFC_DEBUG_ASSERT1(0, "Unsupported wchar_t size (%d)\n", sizeof(wchar_t));
                }
            }

            else if (prefix16[0] == GByteUtil::BEToSystem((UInt16)0xFEFF)) // big endian
            {
		        prefix16++;
                SInt numChars = (flen / 2) - 1;
                if (sizeof(wchar_t) == 2)
                {
                    UInt16* chars = (UInt16*) GALLOC(numChars * 2);
                    Buffer.Resize(numChars);
                    for (SInt i=0; i < numChars; i++)
                        chars[i] = GByteUtil::BEToSystem(prefix16[i]);
                    Buffer.SetString( ((const wchar_t*)chars), numChars );
                    GFREE(chars);
                }
                else if (sizeof(wchar_t) == 4)
                {
                    UInt32* chars = (UInt32*) GALLOC(numChars * 4);
                    Buffer.Resize(numChars);
                    for (SInt i=0; i < numChars; i++)
                        chars[i] = GByteUtil::BEToSystem(prefix16[i]);
                    Buffer.SetString( ((const wchar_t*)chars), numChars );
                    GFREE(chars);
                }
                else
                {
                    GFC_DEBUG_ASSERT1(0, "Unsupported wchar_t size (%d)\n", sizeof(wchar_t));
                }            
            }

            else
            {
                Buffer.Resize(flen);
                char* srcBuf = (char*)td;
                for (SInt i=0; i < flen; i++)
                {
                    Buffer[i] = (wchar_t)(srcBuf[i]);
                }
            }

            GFREE(td);
        }
    }
}

ConfigParser::~ConfigParser()
{
    
}


ConfigParser::TokenType ConfigParser::NextToken(Token* ptoken)
{
    if (!IsValid() || (Pos == (SInt)Buffer.GetLength()))
        return ptoken->Set(Tok_EOF);

    SkipWhitespace();

    // Skip EOF byte, if any.
    if (Buffer[Pos] == 0x1A)
    {
        Pos++;
        if (Pos == (SInt)Buffer.GetLength())
            return ptoken->Set(Tok_EOF);
    }

    // Handle '\r\n' and just '\n' combinations.
    if ((Buffer[Pos] == '\r') && (Buffer[Pos+1] == '\n'))
    {
        Line++;
        Pos+=2;
        return ptoken->Set(Tok_EOL, &Buffer[Pos-2], 2);
    }

    if (Buffer[Pos] == '\n')
    {
        Line++;
        Pos++;
        return ptoken->Set(Tok_EOL, &Buffer[Pos-1], 1);
    }

    // Process identifiers.
    if (IsFirstIdChar(Buffer[Pos]))
    {
        wchar_t * pstart = &Buffer[Pos];
        UInt   start = Pos;
        Pos++;

        while((Pos < (SInt)Buffer.GetLength()) && IsNextIdChar(Buffer[Pos]))
            Pos++;
        return ptoken->Set(Tok_Id, pstart, Pos-start);
    }

    // Process Quoted strings.
    if (Buffer[Pos] == '\"')
    {
        Pos++;
        wchar_t* pstart = &Buffer[Pos];
        UInt   start  = Pos;

        while((Pos < (SInt)Buffer.GetLength()) &&
            (Buffer[Pos] != '\"') )
            Pos++;

        if (Buffer[Pos] == '\"')
        {
            Pos++;
            return ptoken->Set(Tok_String, pstart, Pos-start-1);
        }
        else
        {
            // Incorrect.
            ptoken->Set(Tok_Error);
            ptoken->pError = "closing quote \" character expected";
            return ptoken->Type;
        }
    }

    // Report other characters as Tok_Char.
    Pos++;
    return ptoken->Set(Tok_Char, &Buffer[Pos-1], 1);
}

bool    ConfigParser::TokenizeLine(TokenLine* ptokens, UInt max)
{
    GUNUSED(max);
    Token tok;
    ptokens->ErrorIndex = -1;
    ptokens->LineIndex  = Line;
    ptokens->Tokens.clear();
    
    do {
        NextToken(&tok);
        if ((tok.Type == Tok_Error) && (ptokens->ErrorIndex == -1))
            ptokens->ErrorIndex = (SInt)ptokens->Tokens.size();
        ptokens->Tokens.push_back(tok);
    } while(tok.Type > Tok_EOF);

    return (ptokens->Tokens.size() > 1) || (tok.Type != Tok_EOF);
}

bool    ConfigParser::MatchLine(GTL::garray<Token> &tokens, const wchar_t* pformat, ...)
{
    va_list vl;
    va_start(vl, pformat);

    UInt tokenIndex = 0;

    while(*pformat != 0)
    {
        // Must have tokens for format string.
        if (tokenIndex >= tokens.size())
        {
            va_end(vl);
            return 0;
        }

        Token &tok = tokens[tokenIndex];

        switch (*pformat)
        {
        case '%':

            // %i = matching id
            if (*(pformat+1) == 'i')
            {
                if (!tok.IsId(va_arg(vl, const wchar_t*)))
                {
                    va_end(vl);
                    return 0;
                }
            }
            // %s feed back string
            else if (*(pformat+1) == 's')
            {
                if (!tok.IsString())
                {
                    va_end(vl);
                    return 0;
                }
                GFxString* pstr = va_arg(vl, GFxString*);
                *pstr = tok.GetString();
            }

            // %I feed back identifier
            else if (*(pformat+1) == 'I')
            {
                if (!tok.IsId())
                {
                    va_end(vl);
                    return 0;
                }
                GFxString* pstr = va_arg(vl, GFxString*);
                *pstr = tok.GetString();
            }

            pformat++;
            break;

            // By default, token must match a character.
        default:
            if (!tok.IsChar(*pformat))
            {
                va_end(vl);
                return 0;
            }
        }

        // Go to next character and 
        pformat++;
        tokenIndex++;
    }

    va_end(vl);

    // Last token must be new line or error.
    if (tokenIndex >= tokens.size())    
        return 0;
    Token &tok = tokens[tokenIndex];
    if ((tok.Type == Tok_EOL) || (tok.Type == Tok_EOF))
        return 1;

    // Errors at the end of line would yield parse error.
    return 0;
}


// ***** Font translation
//
void FontTranslator::AddMapping(const GFxString &key, const GFxString &value)
{
    Mappings.add(key, value);
}

bool FontTranslator::Translate(GFxWStringBuffer* pbuffer, const wchar_t *pkey)
{
    GFxString key;
    key = pkey;
    GFxString* pvalue = Mappings.get(key);
    if (pvalue)
    {
        *pbuffer = *pvalue;
        return true;
    }
    return false;
}


// ***** FontFonfig parsing

// Apply config settings to loader.
void FontConfig::Apply(GFxLoader* ploader)
{
    ploader->SetFontLib(0);
    ploader->SetFontMap(pFontMap.GetPtr());

    ploader->SetTranslator(pTranslation);

    // Load font SWF/GFX files
    if (FontLibFiles.size())
    {
        GPtr<GFxFontLib> plib = *new GFxFontLib;
        ploader->SetFontLib(plib);
        for(UInt i=0; i<FontLibFiles.size(); i++)
        {
            GPtr<GFxMovieDef> pdef = *ploader->CreateMovie(FontLibFiles[i]);
            if (pdef)
                plib->AddFontsFrom(pdef, 0);
        }
    }
   
}

FontConfigSet::FontConfigSet()
{
}

// Helper function used to decode font style
static GFxFontMap::MapFontFlags UpdateFontFlags(GFxFontMap::MapFontFlags flags, const GFxString& symbol, UInt lineIndex)
{    
    GFxFontMap::MapFontFlags newFlags = GFxFontMap::MFF_Original;

    if (!symbol.CompareNoCase("bold"))
        newFlags = GFxFontMap::MFF_Bold;
    else if (!symbol.CompareNoCase("normal"))
        newFlags = GFxFontMap::MFF_Normal;
    else if (!symbol.CompareNoCase("italic"))
        newFlags = GFxFontMap::MFF_Italic;
    else
    {
        fprintf(stderr, "Warning: FontConfig(%d) - unknown map font style '%s'\n",
                        lineIndex, symbol.ToCStr());
        return flags;
    }

    if (flags == GFxFontMap::MFF_Original)
    {
        flags = newFlags;
    }
    else if ( ((flags == GFxFontMap::MFF_Normal) && ((newFlags & GFxFontMap::MFF_BoldItalic) != 0)) ||
              ((newFlags == GFxFontMap::MFF_Normal) && ((flags & GFxFontMap::MFF_BoldItalic) != 0)) )
    {
        // Normal combined with incorrect flag.
        fprintf(stderr, "Warning: FontConfig(%d) - unexpected map font style '%s'\n",
                lineIndex, symbol.ToCStr());
    }
    else
    {
          flags = (GFxFontMap::MapFontFlags) (newFlags | flags);
    }

    return flags;
}


void FontConfigSet::Parse(ConfigParser* pparser)
{
    // Process file with tokens    
    ConfigParser::TokenLine     line;
    GTL::garray<Token>  &       tokens = line.Tokens;

    GFxString s1, s2, s3, s4;

    Configs.clear();
    if (!pparser->IsValid())
        return;

    while(pparser->TokenizeLine(&line, 6))
    {
        if (line.HasErrors())
        {
            fprintf(stderr, "Error: FontConfig(%d) - %s\n",
                             line.LineIndex, tokens[line.ErrorIndex].pError);
            break;            
        }

        // [FontConfig] - unnamed
        if (ConfigParser::MatchLine(tokens, L"[%i]", L"FontConfig"))
        {
            GPtr<FontConfig> pfontConfig = *new FontConfig;
            pfontConfig->ConfigName = "Unnamed";
            Configs.push_back(pfontConfig);
        }
        // [FontConfig "English"]
        else if (ConfigParser::MatchLine(tokens, L"[%i%s]", L"FontConfig", &s1))
        {
            GPtr<FontConfig> pfontConfig = *new FontConfig;
            pfontConfig->ConfigName = s1;
            Configs.push_back(pfontConfig);
        }

        else
        {
            // Empty lines ok.
            if (ConfigParser::MatchLine(tokens, L""))
                continue;

            if (Configs.size() == 0)
            {
                fprintf(stderr, "Error: FontConfig(%d) - file mappings outside of [FontConfig] section\n",
                                line.LineIndex);
                break;
            }

            FontConfig* pconfig = Configs[Configs.size()-1];
            GASSERT(pconfig);

            // map "Arial" = "takoma"
            if (ConfigParser::MatchLine(tokens, L"%i%s=%s", L"map", &s1, &s2))
            {
                if (!pconfig->pFontMap)                
                    pconfig->pFontMap = *new GFxFontMap;
                pconfig->pFontMap->MapFont(s1.ToCStr(), s2.ToCStr());
            }

            // map "Arial" = "takoma" Bold
            else if (ConfigParser::MatchLine(tokens, L"%i%s=%s%I", L"map", &s1, &s2, &s3))
            {
                if (!pconfig->pFontMap)                
                    pconfig->pFontMap = *new GFxFontMap;

                GFxFontMap::MapFontFlags mapFlags =
                        UpdateFontFlags(GFxFontMap::MFF_Original, s3, line.LineIndex);
                pconfig->pFontMap->MapFont(s1.ToCStr(), s2.ToCStr(), mapFlags);
            }

            // map "Arial" = "takoma" Bold Italic
            else if (ConfigParser::MatchLine(tokens, L"%i%s=%s%I%I", L"map", &s1, &s2, &s3, &s4))
            {
                if (!pconfig->pFontMap)                
                    pconfig->pFontMap = *new GFxFontMap;

                GFxFontMap::MapFontFlags mapFlags =
                           UpdateFontFlags(GFxFontMap::MFF_Original, s3, line.LineIndex);
                mapFlags = UpdateFontFlags(mapFlags, s4, line.LineIndex);
                pconfig->pFontMap->MapFont(s1.ToCStr(), s2.ToCStr(), mapFlags);
            }

            // fontlib "myfile.swf"
            else if (ConfigParser::MatchLine(tokens, L"%i%s", L"fontlib", &s1))
            {
                // prepend parent path to the filename.
                GFxString path( pparser->ParentPath );
                path += s1;
                pconfig->FontLibFiles.push_back(path);
            }

            // tr "Good morning" = "Guten Morgen"
            else if (ConfigParser::MatchLine(tokens, L"%i%s=%s", L"tr", &s1, &s2))
            {

                pconfig->pTranslation->AddMapping(s1, s2);
            }            

            else
            {
                // Unsupported line
                fprintf(stderr, "Error: FontConfig(%d) - unexpected statement\n",
                                line.LineIndex);
                break;
            }
        }       
    }
}


