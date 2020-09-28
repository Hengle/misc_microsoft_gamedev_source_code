#include <xtl.h>
#include <xgraphics.h>
#include <xaudio.h>

#include "bink.h"
#include "binktextures.h"
#include "xenonfont.h"

//############################################################################
//##                                                                        ##
//##  Example of playing a Bink movie with alpha plane support.             ##
//##                                                                        ##
//##    Left joystick -  Move the video around on screen.                   ##
//##    Right joystick - Scale the movie up or down.                        ##
//##    A button -       Skip to the next Bink movie.                       ##
//##    B button -       Play the movie as fast as possible (up to 60 Hz).  ##
//##    X button -       Pause the current Bink movie.                      ##
//##    Y button -       Loop the current movie (or skip to the next).      ##
//##    Right trigger -  Control the top alpha level.                       ##
//##    Start button -   Show help.                                         ##
//##    Back button -    Return to the Xbox menu.                           ##
//##                                                                        ##
//############################################################################

#define PLAYBACK_PATH "d:\\" // path on the Xbox to play from.

#define ALPHA_VIDEO "alphatst.bik" 

//
// If your Bink video has premultiplied alpha (which is recommended),
//   then set this define to 1, otherwise set it to zero.
//

#define MY_BINK_ALPHA_IS_PREMULTIPLIED 1


//
// Example globals
//

static HBINK Back_Bink = 0;
static HBINK Alpha_Bink = 0;

static BINKTEXTURESET Back_texture_set = { 0 };
static BINKTEXTURESET Alpha_texture_set = { 0 };

static U32 Play_fast = 0;
static S32 Loop_current = 0;
static S32 Paused = 0;
static S32 Display_help = 0;
static F32 Width_scale = 1.0f;
static F32 Height_scale = 1.0f;
static F32 X_adjust = 0.0f;
static F32 Y_adjust = 0.0f;
static F32 Alpha_level = 1.0f;

static IDirect3DDevice9 * d3d_device = 0;
static D3DPRESENT_PARAMETERS Presentation = { 0 };
static int hdtv = 0;

//
// Start up the Xbox video mode
//

static IDirect3DDevice9 * Init_Xbox_video( void )
{
  IDirect3DDevice9 * d3d;

  // Set the screen mode.

  Presentation.BackBufferFormat = D3DFMT_X8R8G8B8;  // for 16-bit use: D3DFMT_R5G6B5
  Presentation.BackBufferCount = 1;
  Presentation.MultiSampleType = D3DMULTISAMPLE_NONE;
  Presentation.SwapEffect = D3DSWAPEFFECT_DISCARD;
  Presentation.AutoDepthStencilFormat = D3DFMT_D24S8;
  Presentation.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
  Presentation.FrontBufferFormat = D3DFMT_X8R8G8B8;  // for 16-bit use: D3DFMT_R5G6B5

  XVIDEO_MODE mode; 
  XMemSet( &mode, 0, sizeof( XVIDEO_MODE ) ); 
  XGetVideoMode( &mode );
  
  Presentation.BackBufferWidth = mode.dwDisplayWidth;
  Presentation.BackBufferHeight = mode.dwDisplayHeight;
  hdtv = mode.fIsHiDef;

  Direct3D_CreateDevice( 0, D3DDEVTYPE_HAL, NULL, 
                            D3DCREATE_HARDWARE_VERTEXPROCESSING, 
                            &Presentation, &d3d );

  return( d3d );
}


static void show_help( HBINK bink )
{
  BINKREALTIME rt;
  U32 fps;
  char buf[ 256 ];

  BinkGetRealtime( bink, &rt, 0 );
  fps = (U32) ( (U64) rt.Frames * 10000 / rt.FramesTime );

  wsprintf( buf, "A: Skip  B: Playing %s  X: %s  Y: %s\n"
                 "Joysticks: Scale and move  FPS:%i.%0i\n",
                 Play_fast ? "fast" : "normal",
                 Paused ? "Paused" : "Resumed",
                 Loop_current ? "Loop" : "Don't loop",
                 fps / 10, fps % 10);

  DrawText( d3d_device, buf, 40, 40, 0xff9f9f9f, 0 );
}


//############################################################################
//##                                                                        ##
//## Show_frame - shows the next Bink frame.                                ##
//##                                                                        ##
//############################################################################

static void Show_frame( void )
{
  //
  // Start the scene
  //

  d3d_device->BeginScene();

  //
  // Clear the screen.
  //

  d3d_device->Clear(0, 0, D3DCLEAR_TARGET, 0, 1.0f, 0);

  //
  // Draw the image on the screen (centered)...
  //

  F32 x, y;

  x = ( X_adjust * Width_scale * ( F32 ) Back_Bink->Width ) + ( Presentation.BackBufferWidth - ( Width_scale * Back_Bink->Width ) ) / 2.0f;
  y = ( Y_adjust * Height_scale * ( F32 ) Back_Bink->Height ) + ( Presentation.BackBufferHeight - ( Height_scale * Back_Bink->Height ) ) / 2.0f;

  Draw_Bink_textures( d3d_device,
                      &Back_texture_set,
                      Back_Bink->Width, Back_Bink->Height,
                      x, y,
                      Width_scale, Height_scale,
                      1.0f, 0 );

  x = ( (F32) Presentation.BackBufferWidth - Alpha_Bink->Width ) / 2.0f;
  y = ( (F32) Presentation.BackBufferHeight - Alpha_Bink->Height ) / 2.0f;

  Draw_Bink_textures( d3d_device,
                      &Alpha_texture_set,
                      Alpha_Bink->Width, Alpha_Bink->Height,
                      x, y,
                      1.0f, 1.0f,
                      Alpha_level,
                      MY_BINK_ALPHA_IS_PREMULTIPLIED );

  if ( Display_help )
  {
    show_help( Back_Bink );
  }

  //
  // End the rendering.
  //

  d3d_device->EndScene();
}


//
// helper function to give the joystick values a deadzone in the middle
//

static F32 deadzone_scaler( S32 val )
{
  F32 fval;

  fval = ( ( F32 ) val ) / 32768.0f;
  
  if ( fval < 0.0f )
  {
    if ( fval > -0.5f )
      fval = 0;
    else
    {
      fval += 0.5f;
      if ( fval < -1.0f )
        fval = -1.0f;
    }
  }
  else
  {
    if ( fval < 0.5f )
      fval = 0;
    else
    {
      fval -= 0.5f;
      if ( fval > 1.0f )
        fval = 1.0f;
    }
  }
  return( fval );
}


//############################################################################
//##                                                                        ##
//## Update_input - read the gamepad and act on it:                         ##
//##                                                                        ##
//##    Left joystick -  Move the video around on screen.                   ##
//##    Right joystick - Scale the movie up or down.                        ##
//##    A button -       Skip to the next Bink movie.                       ##
//##    B button -       Play the movie as fast as possible (up to 60 Hz).  ##
//##    X button -       Pause the current Bink movie.                      ##
//##    Y button -       Loop the current movie (or skip to the next).      ##
//##    Right trigger -  Control the top alpha level.                       ##
//##    Start button -   Show help.                                         ##
//##    Back button -    Return to the Xbox menu.                           ##
//##                                                                        ##
//## This function returns 1 if the current movie should be skipped.        ##
//##                                                                        ##
//############################################################################

static S32 Update_input( void )
{
  static U32 last_check = 0;
  int Game_pad;
  
  //
  // Only check every 100 ms
  //

  U32 time = GetTickCount();

  if ( ( time - last_check ) > 100 )
  {
    XINPUT_STATE state;
    static XINPUT_STATE last_state = { 0 };

    last_check = time;

    //
    // Read the current state
    //

    for ( Game_pad = 0 ; Game_pad < 4 ; Game_pad++ )
    {
      if ( XInputGetState( Game_pad, &state ) == ERROR_SUCCESS )
      {
        goto connected;
      }
    }
    return( 0 );
    
   connected: 

    //
    // Figure out which buttons have pushed since last time
    //

    WORD buttons_down;

    buttons_down = state.Gamepad.wButtons & ( state.Gamepad.wButtons ^ last_state.Gamepad.wButtons );

    //
    // Save our current state for next time.
    //

    last_state = state;

    //
    // Handle the pause control.
    //

    if ( buttons_down & XINPUT_GAMEPAD_X )
    {
      Paused = ! Paused;
      BinkPause( Back_Bink, Paused );
      BinkPause( Alpha_Bink, Paused );
      Play_fast = 0;
    }

    //
    // Handle the skip to the next files
    //

    if ( buttons_down & XINPUT_GAMEPAD_A )
    {
      return( -1 );
    }

    //
    // Handle the speed control
    //

    if ( buttons_down & XINPUT_GAMEPAD_B )
    {
      Play_fast = ! Play_fast;
      if ( Play_fast )
      {
        BinkPause( Back_Bink, 1 );
        BinkPause( Alpha_Bink, 1 );
        Presentation.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
      }
      else
      {
        // reset playback
        BinkPause( Back_Bink, 0 );
        BinkPause( Alpha_Bink, 0 );
        Presentation.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
      }
      Paused = 0;
      d3d_device->Reset( &Presentation );
    }

    //
    // Handle the loop control
    //

    if ( buttons_down & XINPUT_GAMEPAD_Y )
    {
      Loop_current = ! Loop_current;
    }

    //
    // Handle the position joystick
    //

    X_adjust = 1.5f * deadzone_scaler( state.Gamepad.sThumbLX );
    Y_adjust = 1.5f * deadzone_scaler( -(S32)state.Gamepad.sThumbLY );

    //
    // Handle the scale joystick
    //

    Width_scale = pow( 2.0f, 3.0f * deadzone_scaler( state.Gamepad.sThumbRX ) );
    Height_scale = pow( 2.0f, 3.0f * deadzone_scaler( -(S32)state.Gamepad.sThumbRY ) );
    
    //
    // adjust for screen resolution
    //

    F32 video_width_adjust = (F32) Presentation.BackBufferWidth / (F32) Back_Bink->Width;
    F32 video_height_adjust = (F32) Presentation.BackBufferHeight / (F32) Back_Bink->Height;
    F32 video_adjust = ( video_width_adjust < video_height_adjust ) ? video_width_adjust : video_height_adjust;

    Width_scale *= video_adjust;
    Height_scale *= video_adjust;

    //
    // Read the alpha level
    //
    
    Alpha_level = deadzone_scaler( 256 * ( 256 - (S32) state.Gamepad.bRightTrigger ) );
    
    //
    // Handle the display help button
    //

    if ( buttons_down & XINPUT_GAMEPAD_START )
    {
      Display_help = ! Display_help;
    }

    if ( buttons_down & XINPUT_GAMEPAD_BACK )
    {
      XLaunchNewImage( 0, 0 );
    }
  }

  return( 0 ); 
}


//############################################################################
//##                                                                        ##
//## Main entry point to the this example application.                      ##
//##                                                                        ##
//############################################################################

void __cdecl main( void )
{
  WIN32_FIND_DATA find;
  HANDLE find_handle;

  //
  // Init the video mode
  //
 
  d3d_device = Init_Xbox_video( );

  //
  // Init the viewport
  //

  D3DVIEWPORT9 vp = { 0, 0, Presentation.BackBufferWidth, Presentation.BackBufferHeight, 0.0f, 1.0f };
  d3d_device->SetViewport( &vp );

  //
  // Create the Bink shaders to use
  //
  
  if ( Create_Bink_shaders( d3d_device ) )
  {

    //
    // Setup the alpha video and 3D image handles
    //

    Alpha_Bink = BinkOpen( PLAYBACK_PATH ALPHA_VIDEO, BINKALPHA | BINKPRELOADALL );

    if ( Alpha_Bink == 0 )
    {
      //
      // Copy the the alpha test file over (get at ftp.radgametools.com/bink)
      //   or copy your own alpha file and change the ALPHA_VIDEO define.
      //

      return;
    }

    //
    // Ask Bink for the buffer details in this new file
    //

    BinkGetFrameBuffersInfo( Alpha_Bink, &Alpha_texture_set.bink_buffers );

    //
    // Try to create textures for Bink to use.
    //

    if ( !Create_Bink_textures( d3d_device,
                               &Alpha_texture_set ) )
    {
      //
      // Error opening the alpha textures.
      //

      return;
    }
      
    //
    // Register our locked texture pointers with Bink
    //

    BinkRegisterFrameBuffers( Alpha_Bink, &Alpha_texture_set.bink_buffers );


    //
    // find a file
    //

    find_handle = FindFirstFile( PLAYBACK_PATH "*.bik", &find );

    if ( find_handle == INVALID_HANDLE_VALUE )
    {
      return;  // Yo, copy a file to the Xbox to be played.
    }

    //
    // Turn on XAudio and tell Bink to use it
    //

    XAUDIOENGINEINIT EngineInit = { 0 };
    EngineInit.pEffectTable = &XAudioDefaultEffectTable;
    XAudioInitialize( &EngineInit );
    BinkSoundUseXAudio();

    while (1)
    {
      char filename[ MAX_PATH ];

      wsprintf( filename, PLAYBACK_PATH "%s", find.cFileName );

      Paused = 0;

      //
      // Try to open the Bink file.
      //

      Back_Bink = BinkOpen( filename, BINKNOFRAMEBUFFERS );

      if ( Back_Bink )
      {

        //
        // Ask Bink for the buffer details in this new file
        //

        BinkGetFrameBuffersInfo( Back_Bink, &Back_texture_set.bink_buffers );

        //
        // Try to create textures for Bink to use.
        //

        if (  Create_Bink_textures( d3d_device,
                                    &Back_texture_set ) )
        {
          //
          // Register our locked texture pointers with Bink
          //

          BinkRegisterFrameBuffers( Back_Bink, &Back_texture_set.bink_buffers );

          //
          // Start the playback loop.
          //

          while ( 1 )
          {
            //
            // wait for the GPU to finish the previous frame
            //   (I've never seen this wait - the xenon gpu is really fast)
            //

            Wait_for_Bink_textures( &Back_texture_set );


            //
            // decompress the next frame (but we don't show it yet)
            //

            BinkDoFrame( Back_Bink );

            
            //
            // do we need to skip a frame?
            //

            while ( BinkShouldSkip( Back_Bink ) )
            {
              BinkNextFrame( Back_Bink );
              BinkDoFrame( Back_Bink );
            }


            //
            // Now check the foreground alpha movie
            //

            if ( !BinkWait( Alpha_Bink ) )
            {
              //
              // wait for the GPU to finish the previous frame
              //   (I've never seen this wait - the xenon gpu is really fast)
              //

              Wait_for_Bink_textures( &Alpha_texture_set );


              //
              // decompress the next frame (but we don't show it yet)
              //

              BinkDoFrame( Alpha_Bink );

              
              //
              // Keep playing the movie.
              //

              BinkNextFrame( Alpha_Bink );
            }


            //
            // Draw the next frame.
            //
  
            Show_frame( );


            //
            // wait/input loop: wait until it's time to show the frame
            //

            while ( 1 )
            {
              //
              // Check the input status 
              //

              if ( Update_input( ) == -1 )
              {
                //
                // User pressed the skip button.
                //
                
                goto next_movie;
              }

              //
              // is it time for the next frame? if so, break out of this loop
              //

              if ( ( ! BinkWait( Back_Bink ) ) || ( Play_fast ) )
              {
                break;
              }

              //
              // Show the frame if we're paused (so we can use zoom and pan).
              //
  
              if ( Paused )
              {
                Show_frame( );
                d3d_device->Present(0, 0, 0, 0);
              }
            }


            //
            // flip buffers and show the frame. 
            //

            d3d_device->Present(0, 0, 0, 0);


            //
            // check to see if we've hit the end of the movie
            //

            if ( ! Loop_current )
            {
              //
              // Did we hit the end of the movie?
              //

              if ( Back_Bink->FrameNum == Back_Bink->Frames )
              {
                break;
              }
            }

            //
            // Keep playing the movie.
            //

            BinkNextFrame( Back_Bink );
          }
  
         next_movie:

          //
          // Free textures.
          //

          Free_Bink_textures( d3d_device, &Back_texture_set );
        }

        //
        // Close the Bink file.
        //

        BinkClose( Back_Bink );
        Back_Bink = 0;
      }

      //
      // Find the next file to be played.
      //

      if ( FindNextFile( find_handle, &find ) == 0 )
      {
        FindClose( find_handle );
        find_handle = FindFirstFile("d:\\*.bik", &find );
      }

    }
    
    Free_Bink_shaders( );  
  }

  //
  // free any resources used by the text system
  //
  
  FreeDrawTextResources();
  
  //
  // shut down audio
  //
  
  XAudioShutDown();
}


// some stuff for the RAD build utility
// @cdep pre $DefaultsEXEXe
// @cdep pre $requiresbinary($BuildDir/binkxenon.lib)
// @cdep post $BuildEXEXe( ,"Bink Alpha Example")
// some stuff for the RAD build utility
