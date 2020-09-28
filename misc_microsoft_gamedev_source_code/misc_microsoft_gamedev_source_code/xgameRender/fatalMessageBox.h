// File: fatalMessageBox.h
#pragma once

#ifndef BUILD_FINAL
// Intended to be called BEFORE the D3D device is initialized!
void showFatalMessageBox(const char* pTitle, const char* pMsg);
#endif