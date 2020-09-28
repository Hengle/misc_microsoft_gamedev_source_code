// Property File Support -- Aaron Hill
// Copyright 2003 Blank Cartridge, Inc.

#include "common/core/core.h"

#include "PropertyFile.h"

// Property file loader base class
PropertyLoader::PropertyLoader(const std::string &filename)
	: filename(filename)
{
	errorCount = 0;
	warningCount = 0;
	lineNumber = 0;
}

PropertyLoader::~PropertyLoader()
{
}

std::ostream & PropertyLoader::Error()
{
	++errorCount;
	return errorsAndWarnings << "Error: " << filename << ", line " << lineNumber << ": ";
}

std::ostream & PropertyLoader::Warning()
{
	++warningCount;
	return errorsAndWarnings << "Warning: " << filename << ", line " << lineNumber << ": ";
}

float PropertyLoader::StringToNumeric(const std::string &string)
{
	if (string.length() == 0)
		return 0.0f;
	
	size_t invalidChar = string.find_first_not_of("0123456789+-./%");
	if (invalidChar != std::string::npos)
	{
		char c = string[invalidChar];

		if (c > 32 && c < 127)
			Error() << "Invalid character '" << c
				<< "' in numeric or vector element." << std::endl;
		else
			Error() << "Invalid character 0x" << std::hex
				<< (int)c << " in numeric or vector element." << std::endl;
				
		return 0.0f;
	}
	
	size_t divide = string.find("/");
	if (divide != std::string::npos)
	{
		float numerator = StringToNumeric(string.substr(0, divide));
		float divisor = StringToNumeric(string.substr(divide + 1));
		
		if (divisor != 0.0f)
			return numerator / divisor;
		else
		{
			Error() << "Division by zero." << std::endl;
			return 0.0f;
		}
	}
	else
	{
		size_t percent = string.find("%");
		if (percent != std::string::npos)
		{
			if (percent != string.length() - 1)
				Error() << "Invalid specification of percentage." << std::endl;
			
			return (float)atof(string.substr(0, percent).data()) / 100.0f;
		}
		else
			return (float)atof(string.data());
	}
}

std::string PropertyLoader::GetErrorsAndWarnings() const
{
	return errorsAndWarnings.str();
}

bool PropertyLoader::GetNextElement(ElementData &data)
{
	// Default all values.
	
	data.element = UnknownElement;
	data.name = std::string();
	data.hasIndex = false;
	data.index = 0;
	data.type = UnknownType;
	data.numeric = 0;
	data.string = std::string();
	data.vector = std::vector<float>();
	data.boolean = false;

	// Get the next line of text.

	std::string line = GetNextLine();
	size_t length = line.length();
	if (length == 0)
		return false;

	if (line[0] == '[')
	{
		// Line is a property set header.
		
		data.element = PropertySetElement;

		if (line[length - 1] != ']')
		{
			Error() << "Missing ']' in property set header." << std::endl;
			return false;
		}
	
		data.name = line.substr(1, line.length() - 2);
		
		if (data.name.length() == 0)
		{
			Error() << "Missing property set name." << std::endl;
			return false;
		}
		
		size_t invalidChar = data.name.find_first_not_of(
			"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
		);
		if (invalidChar != std::string::npos)
		{
			char c = data.name[invalidChar];

			if (c > 32 && c < 127)
				Error() << "Invalid character '" << c
					<< "' in property set name." << std::endl;
			else
				Error() << "Invalid character 0x" << std::hex
					<< (int)c << " in property set name." << std::endl;
					
			return false;
		}
		
		return true;
	}
	else
	{
		// Line is a property definition.
	
		data.element = PropertyElement;

		size_t equals = line.find_first_of("=");
		if (equals == std::string::npos)
		{
			Error() << "Missing '=' in property definition." << std::endl;
			return false;
		}
		
		std::string name = line.substr(0, equals);
		
		size_t openBracket = name.find_first_of("[");
		if (openBracket != std::string::npos)
		{
			data.name = name.substr(0, openBracket);
			std::string index = name.substr(openBracket + 1);

			size_t closeBracket = index.find_last_of("]");
			if (closeBracket == std::string::npos)
			{
				Error() << "Missing ']' in property element indexer." << std::endl;
				return false;
			}
			
			if (closeBracket != index.length() - 1)
				Warning() << "Extra characters after property element indexer." << std::endl;
			
			index = index.substr(0, closeBracket);
			
			if (index.length() != 0)
			{
				data.hasIndex = true;
				
				size_t invalidChar = index.find_first_not_of("0123456789");
				if (invalidChar != std::string::npos)
				{
					char c = index[invalidChar];

					if (c > 32 && c < 127)
						Error() << "Invalid character '" << c
							<< "' in property element indexer." << std::endl;
					else
						Error() << "Invalid character 0x" << std::hex
							<< (int)c << " in property element indexer." << std::endl;
							
					return false;
				}
				
				data.index = atoi(index.data());
			}
		}
		else
			data.name = name;

		if (data.name.length() == 0)
		{
			Error() << "Missing property name." << std::endl;
			return false;
		}
		
		size_t invalidChar = data.name.find_first_not_of(
			"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
		);
		if (invalidChar != std::string::npos)
		{
			char c = data.name[invalidChar];

			if (c > 32 && c < 127)
				Error() << "Invalid character '" << c
					<< "' in property name." << std::endl;
			else
				Error() << "Invalid character 0x" << std::hex
					<< (int)c << " in property name." << std::endl;
					
			return false;
		}

		std::string value = line.substr(equals + 1);
		
		if (value.length() == 0)
		{
			data.type = BooleanType;
			data.boolean = true;
			return true;
		}
		
		if (value[0] == '"')
		{
			data.type = StringType;
			
			size_t endQuote = value.find_first_of("\"", 1);
			if (endQuote == std::string::npos)
			{
				Warning() << "Missing '\"' at end of string." << std::endl;
				data.string = value.substr(1);
			}
			else
			{
				if (endQuote < value.length() - 1)
					Warning() << "Extra characters after string." << std::endl;
			
				data.string = value.substr(1, endQuote - 1);
			}
			
			return true;
		}
		
		size_t comma = value.find(",");
		if (comma != std::string::npos)
		{
			data.type = VectorType;
			
			size_t start = 0;
			for (;;)
			{
				comma = value.find_first_of(",", start);
				if (comma != std::string::npos)
				{
					data.vector.push_back(StringToNumeric(value.substr(start, comma - start)));
					start = comma + 1;
				}
				else
				{
					data.vector.push_back(StringToNumeric(value.substr(start)));
					break;
				}	
			}
			
			return true;
		}
		
		data.type = NumericType;
		data.numeric = StringToNumeric(value);
		
		return true;
	}
}

std::string PropertyLoader::GetNextLine()
{
	++lineNumber;
			
	gr::BigString lineBuffer;
	
	ReadLine(lineBuffer);
				
	std::stringstream stripped;
	
	bool inQuote = false;
	for (int index = 0; index < lineBuffer.len(); ++index)
	{
		char c = lineBuffer[index];
		
		if (c == '"')
			inQuote = !inQuote;
		
		if (c == '\0' || (c == ';' && !inQuote))
			break;
		
		if (inQuote || (c != ' ' && c != '\t'))
			stripped << c;
	}
	
	return stripped.str();	
}

// Property file loader

PropertyFileLoader::PropertyFileLoader(const std::string& filename)	:
	PropertyLoader(filename),
	pStream(gr::gFileSystem.createStream(filename.c_str()))
{
}

PropertyFileLoader::~PropertyFileLoader()
{
	delete pStream;
}

void PropertyFileLoader::ReadLine(gr::BigString& dst)
{
	if (pStream)
		dst.readLine(*pStream);
}

bool PropertyFileLoader::CanRead() const
{
	return pStream && !(*pStream);
}

// Property file stream loader

PropertyStreamLoader::PropertyStreamLoader(const std::string& filename, gr::Stream& s) :
	PropertyLoader(filename),
	stream(s)
{	
}

PropertyStreamLoader::~PropertyStreamLoader()
{
}

bool PropertyStreamLoader::CanRead() const
{
	return !stream;
}

void PropertyStreamLoader::ReadLine(gr::BigString& dst)
{
	dst.readLine(stream);
}

