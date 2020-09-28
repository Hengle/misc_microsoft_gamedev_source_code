//=============================================================================
// maximumsupportedplayers.h
//
// Copyright (c) 2006 Ensemble Studios
//=============================================================================
#pragma once

// Things that gate this constant: BVisibleMap implementation -- gates to 16.
__declspec(selectany) extern const uint cMaximumSupportedPlayers = 9;
__declspec(selectany) extern const uint cMaximumSupportedHumanPlayers = 6;
__declspec(selectany) extern const uint cMaximumSupportedMultiplayers = 6;
__declspec(selectany) extern const uint cMaximumSupportedTeams = 5;
__declspec(selectany) extern const uint cMaximumSupportedCivs = 5;
__declspec(selectany) extern const long cGaiaPlayer = 0;