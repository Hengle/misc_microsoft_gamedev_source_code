//============================================================================
// flashinput.h
//============================================================================

#pragma once 



class BFlashInputMap
{
   public:
      enum BFlashInputEnum
      {
         cStickLeftUp = 0,
         cStickLeftDown,
         cStickLeftLeft,
         cStickLeftRight,
         cStickRightUp,
         cStickRightDown,
         cStickRightLeft,
         cStickRightRight,
         cDpadUp,
         cDpadDown,
         cDpadLeft,
         cDpadRight,
         cButtonA,
         cButtonB,
         cButtonX,
         cButtonY,
         cButtonStart,
         cButtonBack,
         cButtonShoulderRight,
         cButtonShoulderLeft,
         cButtonThumbLeft,
         cButtonThumbRight,
         cTriggerLeft,
         cTriggerRight,

         cFlashInputTotal
      };


   int            mGameInput;
   GFxKey::Code   mFlashInput;
};

BFlashInputMap gFlashInputMap[BFlashInputMap::cFlashInputTotal] = 
{
   {BFlashInputMap::cStickLeftUp,                GFxKey::Up   },
   {BFlashInputMap::cStickLeftDown,              GFxKey::Down },
   {BFlashInputMap::cStickLeftLeft,              GFxKey::Left },
   {BFlashInputMap::cStickLeftRight,             GFxKey::Right},
   {BFlashInputMap::cStickRightUp,               GFxKey::Up   },
   {BFlashInputMap::cStickRightDown,             GFxKey::Down },
   {BFlashInputMap::cStickRightLeft,             GFxKey::Left },
   {BFlashInputMap::cStickRightRight,            GFxKey::Right},
   {BFlashInputMap::cDpadUp,                     GFxKey::Up   },
   {BFlashInputMap::cDpadDown,                   GFxKey::Down },
   {BFlashInputMap::cDpadLeft,                   GFxKey::Left },
   {BFlashInputMap::cDpadRight,                  GFxKey::Right},
   {BFlashInputMap::cButtonA,                    GFxKey::A    },
   {BFlashInputMap::cButtonB,                    GFxKey::B    },
   {BFlashInputMap::cButtonX,                    GFxKey::X    },
   {BFlashInputMap::cButtonY,                    GFxKey::Y    },
   {BFlashInputMap::cButtonStart,                GFxKey::KP_Enter  },
   {BFlashInputMap::cButtonBack,                 GFxKey::Backspace },
   {BFlashInputMap::cButtonShoulderRight,        GFxKey::Insert    },
   {BFlashInputMap::cButtonShoulderLeft,         GFxKey::Delete    },
   {BFlashInputMap::cButtonThumbLeft,            GFxKey::Home      },
   {BFlashInputMap::cButtonThumbRight,           GFxKey::End       },
   {BFlashInputMap::cTriggerLeft,                GFxKey::PageUp    },
   {BFlashInputMap::cTriggerRight,               GFxKey::PageDown  },   
};
