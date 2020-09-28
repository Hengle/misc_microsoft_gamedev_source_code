//============================================================================
//
//  tokenize.h
//
//  Copyright (c) 2007, Ensemble Studios
//
//============================================================================
#include "xcore.h"
#include "containers\dynamicArray.h"

// Straightforward string tokenization function. Supports quoting, space and tabs are delimiters.   
bool tokenizeString(const char* pStr, BDynamicArray<BString>& tokens);
