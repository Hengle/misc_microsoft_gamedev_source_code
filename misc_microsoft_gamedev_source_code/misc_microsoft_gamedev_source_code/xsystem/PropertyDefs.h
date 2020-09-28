// -------------------------------------------------------------------
//	PropertyDefs.h : A .h file defining PropertyDefs
//		in the ModelSystem project.
//
//	Created 2005/09/05 mjubran <mjubran@microsoft.com>
//	Modified 2005/09/10 mjubran
//
// -------------------------------------------------------------------
// | Copyright (c) Microsoft Corporation. All Rights Reserved.       |
// | This software contains proprietary and confidential             |
// | information of Microsoft and its suppliers.  Use, disclosure or |
// | reproduction is prohibited without the prior express written    |
// | consent of Microsoft.                                           |
// -------------------------------------------------------------------
// -------------------------------------------------------------------
#pragma once

//#define INSTRUMENT_CLASS
#define SPLIT_DATA

#ifdef SPLIT_DATA
// use to count the # of rw accesses per data member
#define INSTRUMENT_FIELD(type, name)																\
   __declspec(property(get=get_ ## name, put=set_ ## name)) type name;								\
   static unsigned rd_ ## name;																		\
   static unsigned wr_ ## name;																		\
   type & get_ ## name() const { rd_ ## name++; return m_pRarelyAccessedData->name; }				\
   void set_ ## name(type n) { wr_ ## name++; m_pRarelyAccessedData->name = n; }					\

#define INSTRUMENT_ARRAY(type, name)																\
   __declspec(property(get=get_ ## name, put=set_ ## name)) type name[];							\
   static unsigned rd_ ## name;																		\
   static unsigned wr_ ## name;																		\
   type &get_ ## name(int i) const { rd_ ## name++; return m_pRarelyAccessedData->name[i]; }		\
   void set_ ## name(int i, type n) { wr_ ## name++; m_pRarelyAccessedData->name[i] = n; }			\

#define INSTRUMENT_SARRAY(type, sub_type, name)														\
   __declspec(property(get=get_ ## name, put=set_ ## name)) type name;								\
   static unsigned rd_ ## name;																		\
   static unsigned wr_ ## name;																		\
   type &get_ ## name() const { rd_ ## name++; return m_pRarelyAccessedData->name; }				\
   sub_type &get_ ## name(int i) const { rd_ ## name++; return m_pRarelyAccessedData->name[i]; }	\
   void set_ ## name(type n) { wr_ ## name++; m_pRarelyAccessedData->name = n; }					\
   void set_ ## name(int i, sub_type n) { wr_ ## name++; m_pRarelyAccessedData->name[i] = n; }		\


#define INIT_FIELD(type, name)														\
	unsigned type::rd_ ## name = 0;													\
	unsigned type::wr_ ## name = 0;

#define PRINT_FIELD(hFile, type, name)												\
	fprintf(hFile, "%s\t%d\t", #type "::rd_" #name, type::rd_ ## name );			\
	fprintf(hFile, "%s\t%d\t", #type "::wr_" #name, type::wr_ ## name );			\
	fprintf(hFile, "%s\t%d\n", #type " total_" #name, (type::rd_ ## name + type::wr_ ## name ));

// no instrumentation macro
#define SPLIT_FIELD(type, name)														\
   __declspec(property(get=get_ ## name, put=set_ ## name)) type name;				\
   type & get_ ## name() const { return m_pRarelyAccessedData->name; }				\
   void set_ ## name(type n) { m_pRarelyAccessedData->name = n; }

#define SPLIT_ARRAY(type, name)														\
   __declspec(property(get=get_ ## name, put=set_ ## name)) type name[];			\
   type &get_ ## name(int i) const { return m_pRarelyAccessedData->name[i]; }		\
   void set_ ## name(int i, type n) { m_pRarelyAccessedData->name[i] = n; }			

#define SPLIT_SARRAY(type, sub_type, name)											\
   __declspec(property(get=get_ ## name, put=set_ ## name)) type name;				\
   type &get_ ## name() const { return m_pRarelyAccessedData->name; }				\
   sub_type &get_ ## name(int i) const {  return m_pRarelyAccessedData->name[i]; }	\
   void set_ ## name(type n) {  m_pRarelyAccessedData->name = n; }					\
   void set_ ## name(int i, sub_type n) { m_pRarelyAccessedData->name[i] = n; }		\



#else
#define SPLIT_FIELD
#define SPLIT_ARRAY
#define SPLIT_SARRAY
#define PRINT_FIELD
#define INIT_FIELD
	// general case
#endif