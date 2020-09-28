//==============================================================================
// converttoken.h
//
// Copyright (c) 1998-2003 Ensemble Studios
//==============================================================================
#pragma once

bool     convertTokenToFloat(const char *token, float &num);
bool     convertTokenToInt(const char *token, long &num);
bool     convertTokenToInt(const char *token, int &num);
bool     convertTokenToBOOL(const char *token, BOOL &b);
bool     convertTokenToDWORD(const char *token, DWORD &num);
bool     convertTokenToBYTE(const char *token, BYTE &num);

// Unicode functions.
bool     convertTokenToDWORDColor(const BString &token, DWORD &color, BYTE defaultAlpha=255);
bool     convertTokenToLongXY(const BString &token, long &x, long &y);
bool     convertTokenToVector(const BString &token, BVector& v);
bool     convertTokenToVector4(const BString &token, BVector4& v);
bool     convertTokenToBool(const BCHAR_T *token, bool &b);
#ifdef XBOX
bool     convertTokenToXMHALF4(const BString &token, XMHALF4& v);
#endif

bool     convertSimTokenToFloat(const BSimString &token, float& v);
bool     convertSimTokenToVector(const BSimString &token, BVector& v);
bool     convertSimTokenToVector2(const BSimString &token, BVector2& v);
bool     convertSimTokenToVector4(const BSimString &token, BVector4& v);
