// File: memoryStats.h
#pragma once

void printKernelMemoryStats(BConsoleOutput& consoleOutput);
void printHeapStats(BConsoleOutput& consoleOutput, bool progressiveUpdate);
void printDetailedHeapStats(BConsoleOutput& consoleOutput, bool totals = true, bool deltas = true);
void printXboxTextureHeapStats(BConsoleOutput& consoleOutput);
void dumpRockallStats();
void createAssetStatsCSV(const char* pFilename);
void dumpQuickCompareStats();
void dumpQuickCompareStats(const char* file, long line);

#ifndef BUILD_FINAL
float getHeapBarChartValue();
#endif

#define QUICKMEM  dumpQuickCompareStats(__FILE__, __LINE__);
void displayFontMemoryStats(BDebugTextDisplay& textDisplay);

