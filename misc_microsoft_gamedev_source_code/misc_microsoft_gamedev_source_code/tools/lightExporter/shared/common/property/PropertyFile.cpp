// Property File Support -- Aaron Hill
// Copyright 2003 Blank Cartridge, Inc.

#include "common/core/core.h"

#include "PropertyFile.h"

PropertyFile::PropertyFile()
{
}

PropertyFile::PropertyFile(PropertyLoader& loader)
{
	std::string currentSetName = "";
	SetPropertySet(PropertySet(), currentSetName);
	
	while (loader.CanRead())
	{
		PropertyLoader::ElementData data;
		if (loader.GetNextElement(data))
		{
			switch (data.element)
			{
				case PropertyLoader::PropertySetElement:
				{
					SetPropertySet(PropertySet(), data.name);
					currentSetName = data.name;
				}
				break;
				
				case PropertyLoader::PropertyElement:
				{
					if (HasPropertySet(currentSetName))
					{
						PropertySet set = GetPropertySet(currentSetName);
						
						PropertyValue value;
						
						if (set.HasPropertyValue(data.name))
							value = set.GetPropertyValue(data.name);
						
						if (data.hasIndex)
						{
							switch (data.type)
							{
								case NumericType:
									value.SetNumeric(data.numeric, data.index);
								break;

								case StringType:
									value.SetString(data.string, data.index);
								break;

								case VectorType:
									value.SetVector(data.vector, data.index);
								break;

								case BooleanType:
									value.SetBoolean(data.boolean, data.index);
								break;

								default:
									_RPT0(_CRT_ERROR, "Invalid value type.");
								break;
							}
						}
						else /* data.hasIndex */
						{
							switch (data.type)
							{
								case NumericType:
									value.SetNumeric(data.numeric);
								break;
			
								case StringType:
									value.SetString(data.string);
								break;
		
								case VectorType:
									value.SetVector(data.vector);
								break;
	
								case BooleanType:
									value.SetBoolean(data.boolean);
								break;

								default:
									_RPT0(_CRT_ERROR, "Invalid value type.");
								break;
							}
						} /* data.hasIndex */
						
						set.SetPropertyValue(value, data.name);
						
						SetPropertySet(set, currentSetName);
					}
				}
				break;
				
				default:
				break;
			}
		}
	}

	errorCount = loader.GetErrorCount();
	warningCount = loader.GetWarningCount();
	errorsAndWarnings = loader.GetErrorsAndWarnings();
/*	
#if _DEBUG
	unsigned int errorCount = loader.GetErrorCount();
	unsigned int warningCount = loader.GetWarningCount();

	if (errorCount > 0 || warningCount > 0)
	{
		std::stringstream stream;
		stream << loader.GetErrorsAndWarnings() << std::endl;
		stream << "Errors: " << errorCount << "  Warnings: " << warningCount << std::endl;
	
		_RPT0(_CRT_ERROR, stream.str().data());
	}
#endif
*/
}

PropertyFile::PropertyFile(const PropertyFile &propertyFile)
	: propertySetMap(propertyFile.propertySetMap)
{
}

PropertyFile & PropertyFile::operator =(const PropertyFile &propertyFile)
{
	propertySetMap = propertyFile.propertySetMap;
	return *this;
}

PropertyFile::~PropertyFile()
{
}

const PropertySet & PropertyFile::GetPropertySet(const std::string &name) const
{
	_ASSERTE(HasPropertySet(name));
	
	return propertySetMap.find(name)->second;
}

bool PropertyFile::HasPropertySet(const std::string &name) const
{
	return propertySetMap.find(name) != propertySetMap.end();
}

void PropertyFile::SetPropertySet(const PropertySet &propertySet, const std::string &name)
{
	typedef std::pair<std::string, PropertySet> KeyPair;
	typedef std::pair<PropertySetMap::iterator, bool> InsertResult;

	InsertResult insertResult = propertySetMap.insert(KeyPair(name, propertySet));

	if (!insertResult.second)
		insertResult.first->second = propertySet;
}

std::vector<std::string> PropertyFile::GetPropertySetNames() const
{
	std::vector<std::string> names;
	
	PropertySetMap::const_iterator iter = propertySetMap.begin();
	for (; iter != propertySetMap.end(); ++iter)
		names.push_back(iter->first);
	
	return names;
}