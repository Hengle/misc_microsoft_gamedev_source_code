// Property File Support -- Aaron Hill
// Copyright 2003 Blank Cartridge, Inc.

#ifndef _PROPERTYFILE_H_
#define _PROPERTYFILE_H_

//FIXME: Remove dependency on d3dx!
#include <d3d9.h>
#include <d3dx9.h>

#include <crtdbg.h>

#include "common/utils/string.h"
#include "common/utils/stream.h"
#include "common/filesys/file_system.h"

// Forward declarations.

class PropertyFile;
class PropertySet;
class PropertyValue;

// Enumerations.

enum ValueType
{
	UnknownType,
	NumericType,
	StringType,
	VectorType,
	BooleanType,
};

// Classes.

class PropertyLoader
{
	private:
		std::string filename;
		unsigned int lineNumber;
		unsigned int errorCount;
		unsigned int warningCount;
		std::stringstream errorsAndWarnings;
					
		// Do not copy construct or assign.
		PropertyLoader(const PropertyLoader &loader) { };
		PropertyLoader & operator =(const PropertyLoader &loader) { return *this; };

		virtual std::string GetNextLine();
		virtual void ReadLine(gr::BigString& dst) = 0;
		
		std::ostream & Error();
		std::ostream & Warning();
		
		float StringToNumeric(const std::string &string);

	public:
		PropertyLoader(const std::string &filename);
		virtual ~PropertyLoader();
		
		unsigned int GetErrorCount() const { return errorCount; };
		unsigned int GetWarningCount() const { return warningCount; };
		std::string GetErrorsAndWarnings() const;

		virtual bool CanRead() const = 0;

		enum Element
		{
			UnknownElement,
			PropertySetElement,
			PropertyElement,
		};

		struct ElementData
		{
			Element element;
			
			std::string name;
			bool hasIndex;
			unsigned int index;

			ValueType type;
			float numeric;
			std::string string;
			std::vector<float> vector;
			bool boolean;
		};
		
		bool GetNextElement(ElementData &data);
};

class PropertyFileLoader : public PropertyLoader
{
	gr::Stream* pStream;
	virtual void ReadLine(gr::BigString& dst);
	
public:
	PropertyFileLoader(const std::string &filename);
	virtual ~PropertyFileLoader();
		
	virtual bool CanRead() const;
};

class PropertyStreamLoader : public PropertyLoader
{
	gr::Stream& stream;
	virtual void ReadLine(gr::BigString& dst);
	
public:
	PropertyStreamLoader(const std::string &filename, gr::Stream& s);
	virtual ~PropertyStreamLoader();
		
	virtual bool CanRead() const;
};

class PropertyFile
{
	private:
						
		unsigned int errorCount;
		unsigned int warningCount;
		std::string errorsAndWarnings;
		
		typedef std::map<std::string, PropertySet> PropertySetMap;
		PropertySetMap propertySetMap;
				
	public:
		PropertyFile();
		PropertyFile(PropertyLoader& loader);
		
		PropertyFile(const PropertyFile &propertyFile);
		PropertyFile & operator =(const PropertyFile &propertyFile);
		~PropertyFile();

		unsigned int GetErrorCount() const { return errorCount; };
		unsigned int GetWarningCount() const { return warningCount; };
		const std::string & GetErrorsAndWarnings() const { return errorsAndWarnings; };
		
		const PropertySet & GetPropertySet(const std::string &name) const;
		bool HasPropertySet(const std::string &name) const;
		void SetPropertySet(const PropertySet &propertySet, const std::string &name);

		std::vector<std::string> GetPropertySetNames() const;
};

class PropertySet
{
	private:
		typedef std::map<std::string, PropertyValue> PropertyValueMap;
		PropertyValueMap propertyValueMap;
	
	public:
		PropertySet();
		
		PropertySet(const PropertySet &propertySet);
		PropertySet & operator =(const PropertySet &propertySet);
		~PropertySet();
		
		const PropertyValue & GetPropertyValue(const std::string &name) const;
		bool HasPropertyValue(const std::string &name) const;
		void SetPropertyValue(const PropertyValue &propertyValue, const std::string &name);
		
		std::vector<std::string> GetPropertyValueNames() const;
};

class PropertyValue
{
	private:
		struct Value
		{
			float numeric;
			std::string string;
			std::vector<float> vector;
			bool boolean;
		};
		
		typedef std::vector<Value> Values;
		Values values;

		ValueType type;
		unsigned int nextElement;
	
	public:
		PropertyValue();
		
		PropertyValue(const PropertyValue &property);
		PropertyValue & operator =(const PropertyValue & property);
		~PropertyValue();
		
		ValueType GetType() const;
		unsigned int GetElementCount() const;
		
		float GetNumeric(unsigned int element = 0) const;
		const std::string & GetString(unsigned int element = 0) const;
		const std::vector<float> & GetVector(unsigned int element = 0) const;
		bool GetBoolean(unsigned int element = 0) const;
		
		void SetNumeric(float numeric);
		void SetString(const std::string &string);
		void SetVector(const std::vector<float> &vector);
		void SetBoolean(bool boolean);

		void SetNumeric(float numeric, unsigned int element);
		void SetString(const std::string &string, unsigned int element);
		void SetVector(const std::vector<float> &vector, unsigned int element);
		void SetBoolean(bool boolean, unsigned int element);
		
		void Unset();
};

// Inlines.  (Common.inl is included too early, so these are here.)

inline float ReadNumeric(
	const PropertySet &propertySet,
	const std::string &name,
	float defaultValue
)
{
	if (propertySet.HasPropertyValue(name))
	{
		PropertyValue propertyValue = propertySet.GetPropertyValue(name);
		
		switch (propertyValue.GetType())
		{
			case NumericType:
			return propertyValue.GetNumeric();
			
			case VectorType:
			return propertyValue.GetVector()[0];
		}
	}
	
	return defaultValue;
}

inline unsigned int ReadNumericInt(
	const PropertySet &propertySet,
	const std::string &name,
	unsigned int defaultValue
)
{
	float value = ReadNumeric(propertySet, name, static_cast<float>(defaultValue));
	return static_cast<unsigned int>(ceil(fabs(value)));
}

inline gr::BigString ReadBigString(
	const PropertySet &propertySet,
	const std::string &name,
	const gr::BigString &defaultValue)
{
	if (propertySet.HasPropertyValue(name))
	{
		PropertyValue propertyValue = propertySet.GetPropertyValue(name);
		
		switch (propertyValue.GetType())
		{
			case StringType:
			return gr::BigString(propertyValue.GetString().c_str());
		}
	}
	
	return defaultValue;
}

inline std::string ReadString(
	const PropertySet &propertySet,
	const std::string &name,
	const std::string &defaultValue
)
{
	if (propertySet.HasPropertyValue(name))
	{
		PropertyValue propertyValue = propertySet.GetPropertyValue(name);
		
		switch (propertyValue.GetType())
		{
			case StringType:
			return propertyValue.GetString();
		}
	}
	
	return defaultValue;
}

inline std::vector<float> ReadVector(
	const PropertySet &propertySet,
	const std::string &name,
	const std::vector<float> &defaultValue
)
{
	if (propertySet.HasPropertyValue(name))
	{
		PropertyValue propertyValue = propertySet.GetPropertyValue(name);
		
		switch (propertyValue.GetType())
		{
			case NumericType:
			return std::vector<float>(1, propertyValue.GetNumeric());
			
			case VectorType:
			return propertyValue.GetVector();
		}
	}
	
	return defaultValue;
}

inline std::vector<float> ReadVectorSized(
	const PropertySet &propertySet,
	const std::string &name,
	const std::vector<float> &defaultValue,
	unsigned int expectedSize
)
{
	std::vector<float> vector = ReadVector(propertySet, name, defaultValue);
	float replicate = vector.at(vector.size() - 1);
	while (vector.size() < expectedSize)
		vector.push_back(replicate);
	
	return vector;
}

inline D3DXVECTOR3 ReadVector3(
	const PropertySet &propertySet,
	const std::string &name,
	const D3DXVECTOR3 &defaultValue
)
{
	std::vector<float> defaultCopy(3);
	defaultCopy[0] = defaultValue.x;
	defaultCopy[1] = defaultValue.y;
	defaultCopy[2] = defaultValue.z;
	
	std::vector<float> vector = ReadVectorSized(propertySet, name, defaultCopy, 3);
	
	return D3DXVECTOR3(vector[0], vector[1], vector[2]);
}


inline std::vector<unsigned int> ReadVectorInt(
	const PropertySet &propertySet,
	const std::string &name,
	const std::vector<unsigned int> &defaultValue
)
{
	size_t index;

	std::vector<float> defaultCopy(defaultValue.size());
	for (index = 0; index < defaultValue.size(); ++index)
		defaultCopy[index] = static_cast<float>(defaultValue[index]);
	
	std::vector<float> vector = ReadVector(propertySet, name, defaultCopy);
	
	std::vector<unsigned int> vectorCopy(vector.size());
	for (index = 0; index < vector.size(); ++index)
		vectorCopy[index] = static_cast<unsigned int>(ceil(fabs(vector[index])));
	
	return vectorCopy;
}

inline std::vector<unsigned int> ReadVectorIntSized(
	const PropertySet &propertySet,
	const std::string &name,
	const std::vector<unsigned int> &defaultValue,
	unsigned int expectedSize
)
{
	std::vector<unsigned int> vector = ReadVectorInt(propertySet, name, defaultValue);
	unsigned int replicate = vector.at(vector.size() - 1);
	while (vector.size() < expectedSize)
		vector.push_back(replicate);
	
	return vector;
}

inline bool ReadBoolean(
	const PropertySet &propertySet,
	const std::string &name,
	bool defaultValue
)
{
	if (propertySet.HasPropertyValue(name))
	{
		PropertyValue propertyValue = propertySet.GetPropertyValue(name);
		
		switch (propertyValue.GetType())
		{
			case BooleanType:
			return propertyValue.GetBoolean();
		}
	}
	
	return defaultValue;
}

#endif // _PROPERTYFILE_H_