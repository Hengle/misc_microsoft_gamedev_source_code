// PropertyValue File Support -- Aaron Hill
// Copyright 2003 Blank Cartridge, Inc.

#include "common/core/core.h"

#include "PropertyFile.h"

PropertyValue::PropertyValue()
{
	type = UnknownType;
	nextElement = 0;
}

PropertyValue::PropertyValue(const PropertyValue &propertyValue)
	: values(propertyValue.values), nextElement(propertyValue.nextElement)
{
	type = propertyValue.type;
	nextElement = propertyValue.nextElement;
}

PropertyValue & PropertyValue::operator =(const PropertyValue & propertyValue)
{
	values = propertyValue.values;
	type = propertyValue.type;
	nextElement = propertyValue.nextElement;
	return *this;
}

PropertyValue::~PropertyValue()
{
}

ValueType PropertyValue::GetType() const
{
	return type;
}

unsigned int PropertyValue::GetElementCount() const
{
	return static_cast<unsigned int>(values.size());
}

float PropertyValue::GetNumeric(unsigned int element) const
{
	_ASSERTE(type == NumericType);
	_ASSERTE(element < GetElementCount());
	
	return values[element].numeric;
}

const std::string & PropertyValue::GetString(unsigned int element) const
{
	_ASSERTE(type == StringType);
	_ASSERTE(element < GetElementCount());
	
	return values[element].string;
}

const std::vector<float> & PropertyValue::GetVector(unsigned int element) const
{
	_ASSERTE(type == VectorType);
	_ASSERTE(element < GetElementCount());
	
	return values[element].vector;
}

bool PropertyValue::GetBoolean(unsigned int element) const
{
	_ASSERTE(type == BooleanType);
	_ASSERTE(element < GetElementCount());
	
	return values[element].boolean;
}

void PropertyValue::SetNumeric(float numeric)
{
	SetNumeric(numeric, nextElement++);
}

void PropertyValue::SetString(const std::string &string)
{
	SetString(string, nextElement++);
}

void PropertyValue::SetVector(const std::vector<float> &vector)
{
	SetVector(vector, nextElement++);
}

void PropertyValue::SetBoolean(bool boolean)
{
	SetBoolean(boolean, nextElement++);
}

void PropertyValue::SetNumeric(float numeric, unsigned int element)
{
	if (type == UnknownType)
		type = NumericType;

	_ASSERTE(type == NumericType);
	
	if (element >= GetElementCount())
		values.resize(element + 1);
	
	values[element].numeric = numeric;
}

void PropertyValue::SetString(const std::string &string, unsigned int element)
{
	if (type == UnknownType)
		type = StringType;

	_ASSERTE(type == StringType);
	
	if (element >= GetElementCount())
		values.resize(element + 1);
	
	values[element].string = string;
}

void PropertyValue::SetVector(const std::vector<float> &vector, unsigned int element)
{
	if (type == UnknownType)
		type = VectorType;

	_ASSERTE(type == VectorType);
	
	if (element >= GetElementCount())
		values.resize(element + 1);
	
	values[element].vector = vector;
}

void PropertyValue::SetBoolean(bool boolean, unsigned int element)
{
	if (type == UnknownType)
		type = BooleanType;

	_ASSERTE(type == BooleanType);
	
	if (element >= GetElementCount())
		values.resize(element + 1);
	
	values[element].boolean = boolean;
}

void PropertyValue::Unset()
{
	values.clear();
	type = UnknownType;
	nextElement = 0;
}