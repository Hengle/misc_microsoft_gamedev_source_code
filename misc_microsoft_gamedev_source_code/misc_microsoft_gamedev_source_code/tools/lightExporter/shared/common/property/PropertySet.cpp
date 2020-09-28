// PropertyValue File Support -- Aaron Hill
// Copyright 2003 Blank Cartridge, Inc.

#include "common/core/core.h"
#include "PropertyFile.h"

PropertySet::PropertySet()
{
}

PropertySet::PropertySet(const PropertySet &propertySet)
	: propertyValueMap(propertySet.propertyValueMap)
{
}

PropertySet & PropertySet::operator =(const PropertySet &propertySet)
{
	propertyValueMap = propertySet.propertyValueMap;
	return *this;
}

PropertySet::~PropertySet()
{
}

const PropertyValue & PropertySet::GetPropertyValue(const std::string &name) const
{
	_ASSERTE(HasPropertyValue(name));
	
	return propertyValueMap.find(name)->second;
}

bool PropertySet::HasPropertyValue(const std::string &name) const
{
	return propertyValueMap.find(name) != propertyValueMap.end();
}

void PropertySet::SetPropertyValue(const PropertyValue &propertyValue, const std::string &name)
{
	typedef std::pair<std::string, PropertyValue> KeyPair;
	typedef std::pair<PropertyValueMap::iterator, bool> InsertResult;

	InsertResult insertResult = propertyValueMap.insert(KeyPair(name, propertyValue));

	if (!insertResult.second)
		insertResult.first->second = propertyValue;
}

std::vector<std::string> PropertySet::GetPropertyValueNames() const
{
	std::vector<std::string> names;
	
	PropertyValueMap::const_iterator iter = propertyValueMap.begin();
	for (; iter != propertyValueMap.end(); ++iter)
		names.push_back(iter->first);
	
	return names;
}