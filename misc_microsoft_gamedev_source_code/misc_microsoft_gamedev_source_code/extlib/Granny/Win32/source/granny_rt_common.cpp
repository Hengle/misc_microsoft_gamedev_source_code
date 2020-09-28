/* ========================================================================
   $RCSfile: granny_rt_common.cpp,v $
   $Date: 2002/06/20 22:30:21 $
   $Revision: 1.9 $
   $Creator: Casey Muratori $
   (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
   ======================================================================== */

/* @cdep pre

   $GrannyRTPaths
   $StandardH
   $StandardCPP
   $StandardLIB

   $default(TakeCRPs,*.h,pre
            $set(CRPFile,$BuildTo/$clipdir($extension($file,crp)))
            $set(CRPs,$CRPs $CRPFile)
            $set(RefreshCRPs,$RefreshCRPs $if($newer($file,$CRPFile),$file, ))
            )

   $RequireAll($GrannyRTDir/granny_transform.h$S
               $GrannyRTDir/granny_model.h$S
               $GrannyRTDir/granny_model_control_binding.h$S
               $GrannyRTDir/granny_model_instance.h$S
               $GrannyRTDir/granny_data_type_definition.h$S
               $GrannyRTDir/granny_curve.h$S
               $GrannyRTDir/granny_material.h$S
               $GrannyRTDir/granny_pixel_layout.h$S
               $wildfiles($GrannyRTDir/granny_*.h,$S))
*/

/* @cdep post

   $if($RefreshCRPs,
       $run($MakeEXEName(crapi),extract $RefreshCRPs,$BuildTo)
       $DONTecho($MakeEXEName(crapi) build $GrannyRTDir/dll_header.h $GrannyRTDir/dll_header.cpp $CRPs $GrannyRTDir/dll_footer.h)
       $run($MakeEXEName(crapi),build $GrannyRTDir/dll_header.h $GrannyRTDir/dll_header.cpp $CRPs $GrannyRTDir/dll_footer.h,$BuildTo)
       $movefile($BuildTo/build_out.h,$BuildTo/granny.h)
       $movefile($BuildTo/build_out.cpp,$BuildTo/granny.cpp), )

   $ProcessH($BuildTo/granny.h)
   $ProcessCPP($BuildTo/granny.cpp)
   $set(CPPs,$CPPs $BuildTo/granny.cpp)
*/
