template <long Size> bool UTBitVector<Size>::save(BChunkWriter *chunkWriter) const
{
   if(!chunkWriter)
   {
      BASSERT(0);
      return(false);
   }


   long result=chunkWriter->writeBYTEArray((Size/8), (const BYTE*) mValue);
   if (!result)
      return(false);

   return(true);
}

template <long Size> bool UTBitVector<Size>::load(BChunkReader *chunkReader)
{
   if(!chunkReader)
   {
      BASSERT(0);
      return(false);
   }

   //Get the number of BYTEs we want to read.
   long countExpected=0;
   long result=chunkReader->peekArrayLength(&countExpected);
   if (!result)
      return(false);
   //Set the number of bytes by taking the count (times 8 for BYTE size) and
   //calling the setNumber method that takes the number of bits.
   // ajl 6/8/01 - made it not resize down because it makes loading old scenarios not
   // work after you increase the number of flags in BUnit.
   DWORD newNumber=countExpected*8;
   if (newNumber > Size)
   {
      return (false);
   }
   // ajl 6/8/01 - make sure the bits are all reset in case the number of flags
   // being loaded in is different.
   zero();

   //Actually read the BYTEs.
   long countRead=0;
   result=chunkReader->readBYTEArray(&countRead, (BYTE*) mValue, countExpected);
   if (!result)
      return(false);
   BASSERT(countRead == countExpected);

   return(true);
}
