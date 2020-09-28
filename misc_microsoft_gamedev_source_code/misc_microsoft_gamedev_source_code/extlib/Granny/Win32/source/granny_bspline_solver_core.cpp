// ========================================================================
// $File: //jeffr/granny/rt/granny_bspline_solver_core.cpp $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #6 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_BSPLINE_SOLVER_CORE_H)
#include "granny_bspline_solver_core.h"
#endif

#if !defined(GRANNY_STATISTICS_H)
#include "granny_statistics.h"
#endif

#if !defined(GRANNY_BSPLINE_H)
#include "granny_bspline.h"
#endif

#if !defined(GRANNY_BSPLINE_SOLVER_DEBUGGING_H)
#include "granny_bspline_solver_debugging.h"
#endif

#if !defined(GRANNY_MATH_H)
#include "granny_math.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif


/* BSpline solver core routines

   The BSpline solver harness is in granny_bspline_solver.cpp, but the
   actual routines that do the heavy lifting are isolated here, because
   they're pretty dense and need to be excessively commented in order
   for people other than me to maintain them.

   The way the solver works is that it solves Ax = b, where A is a
   rectangular matrix that contains b-spline coefficients, b is the
   set of samples you're trying to hit with the b-spline, and x is
   the vector of control points for the b-spline that you want to
   generate.  Since A is not square, the solver uses least squares
   to compute A^T Ax = A^T b.

   The solver core has two components: a routine to generate ATA and
   ATb, and a pair of routines to factor ATA and substitute the ATb
   through it to get the x we're looking for.

   There are some fundamental observations that you need to be aware
   of when reading through these routines.  I have tried to collect
   some of them here, to provide as much context as possible.

   1) The A matrix is SampleCount x KnotCount in dimension.  Hey, it
      maps knots to samples and vice versa, so it has to be those
      dimensions.

   2) The A matrix is sparse.  This should be obvious from the fact
      that it maps sample points to control points.  Since each
      sample of a b-spline can only have (Degree + 1) control points
      involved, you can only have (Degree + 1) entries in any row
      of A.  Since there are KnotCount columns of A, and the number
      of knots is typically much larger than the degree of the spline,
      you will have mostly zeroes in each row.

   3) The non-zero entries of A occur in adjacent columns.  Once again,
      this should be obvious, because sampling a spline only involves
      adjacent knots, and knots correspond to columns.

   4) No row ever has entries in a column _prior_ to the leftmost
      column in the previous row.  This is because the rows correspond
      to the samples, and each sample occurs at a later t.  Thus,
      the knots involved in computing any sample must increase, and
      thus the columns filled in for that sample's row must move
      right (or stay the same) compared to the previous sample's row.

   Those 4 relatively obvious observations about A give a reasonable
   picture of what A looks like: it's got (Degree + 1) entries per
   row, and each row either fills in the same entries as the previous
   row, or fills them in shifted one to the right.  We start at the
   leftmost column, and as we get to the bottom of the matrix, we get
   to the rightmost column.

   5) The ATA matrix is KnotCount x KnotCount, because it is a squared
      version of A.

   6) The ATA matrix is symmetric.  All squared matrices are symmetric,
      so this is a given.

   7) The ATA matrix is band-diagonal.  This is because A is made up
      of bands which only move from left to right, continuously, so
      there is no way to ever get a 0 in the ATA matrix that does not
      have all zeroes moving away from the diagonal (once there stops
      being an overlap between columns of A, then there will never
      again be one, thus ATA can not have a nonzero inner product
      there).

   8) The BandWidth of the ATA matrix is exactly (Degree + 1)
      everywhere.  This is because the bands in A are exactly (Degree
      + 1) on every row, so you can never have more than (Degree + 1)
      columns of overlap when computing the ATA element inner products.

   There 4 observations about ATA give an easy description of what it
   looks like: it's a square matrix with a thick swath of entries down
   the diagonal, with zeroes everywhere else.

   Now, throughout this comment (and the subsequent ones) I am ignoring
   the fact that you may actually have _additional_ zeroes, brought on
   by the fact that this solver actually handles arbitrarily doubled
   (or trippled, or quadrupled, or whatever you want) knots, which
   create thinner bands in ATA, or even pinch points where there is
   a zero on the diagonal.  So long as all the code is written to
   properly pass values through, this doesn't actually affect any
   of the routines, so no real special care is given to this problem
   except in the back and forward substitutors, where they have to
   do an epsilon check.
 */

/* ATA addressing (this applies to ALL routines)

   The squared matrix A^T A is symmetric and band-diagonal.  Therefore,
   there is no need to store most of the matrix.  The optimal storage
   for a symmetric band-diagonal matrix is to simply store the diagonal
   element for each row, and then the elements to its right.  There is
   never any reason to store elements to its left, because they are
   just the same elements from previous rows (because of symmetry).
   Thus, the matrix is stored like this:

   Full Matrix      Ignore Redundant Elements      Align Left
   x x x . . .             x x x . . .               x x x
   x x x x . .             - x x x . .               x x x
   x x x x x .             - - x x x .               x x x
   . x x x x x      ->     . - - x x x        ->     x x x
   . . x x x x             . . - - x x               x x .
   . . . x x x             . . . - - x               x . .

   This makes it a little bit trick to figure out where the element
   ATA(i, j) is going to be in the compressed form ATA[?].  Here are
   some rules:

       ATA(i, j) = ATA(j, i)

       ATA(i, i)        ->  ATA[i * BandWidth]
       ATA(i, i + j)    ->  ATA[i * BandWidth + j]
       ATA(i, i - j)    ->  ATA[(i-j) * BandWidth + j]
*/

/* ATA / ATb Generator

   BuildATAAndATb() does just what it says - it takes the knot values
   and the sample values, and produces an ATA from the knots and an
   ATb from the samples, so that the Cholesky solver can do its job.

   This routine builds the A matrix in condensed form.  Since we know
   there are only going to be BandWidth entries in each row of A,
   we simply treat A as a block that is BandWidth x SampleCount.
   In reality, the A matrix is actually KnotCount x SampleCount,
   which is much larger, but it's mostly zeroes.

   The condensed A matrix is a little non-intuitive, because the
   _last_ entry in a row of the condensed block is actually the
   diagonal entry of A (this is why there are "+ (Bandwidth - 1)"
   terms in the A addressing all over the place).  So, it looks like:

       Original A       Condensed A block
   p p x . . . . .            p p x
     p x x . . . .            p x x
     p x x . . . .            p x x
       x x x . . .            x x x
       . x x x . .            x x x
       . x x x . .     ->     x x x
       . x x x . .            x x x
       . x x x . .            x x x
       . . x x x .            x x x
       . . . x x x            x x x

   where x are filled entries, and p are phantom entries that exist
   and are used, but in reality lie outside the real A matrix.  These
   values are folded into their closest neighboring column at the
   beginning and never looked at subsequently.

   There are only KnotCount "characteristic rows" of A, in the sense
   that the indices of the columns that are filled do not vary in
   rows that correspond to different _samples_ but the same _knot_.
   In the above diagram, you can see that there are multiple rows that
   all have the exact same columns filled.  These are all the rows
   that correspond to a single anchor knot during b-spline evaluation.

   This is an important observation, and this routine uses an array
   called KnotStart that actually records the sample index at which
   the characteristic row changes (ie., when the A matrix goes from
   having columns 3->6 filled to having 4->7 filled, etc.)  All
   fundamental calculations are done in loops that run over groups
   that have the same characteristic.  So in that sense, you can think
   of the A matrix as being in continguous "blocks" that have the same
   columns filled, and the KnotStart array is what says where these
   blocks begin and end.

   The core of this routine, where the actual ATA and ATb terms are
   computed, works on the fundamental observation that any entry of
   ATA only involves the product of values in a small neighborhood of
   A.  This neighborhood is the knot range that corresponds to the
   current row of ATA that is being computed (i), and the knot ranges
   of the BandWidth knots after it.  Every entry in ATA can be
   computed by taking products of these ranges, and the i/j/k nested
   loops in this routine do exactly that, collecting the minimum number
   of terms necessary.

   To understand the terms, being by thinking about the diagonal
   elements of ATA.  These are just the inner products of some column
   in A with itself.  To compute these, all we have to do is squares
   of the entries in the (Degree + 1) knot ranges for that column.
   To compute the off-diagonal entries (of which there are Degree) of
   ATA, we just have to do that inner product with some _other_ column.
   This means we actually have less terms, because we know that at least
   one of the knot ranges of the central column will be occupied by
   zeroes in the other column (the diagonal entries have the maximum
   overlap, because they're the same column times itself - all other
   entries have one of more _less_ overlapping ranges).

   And there you have it.
*/

void GRANNY
BuildATAAndATb(int32x bWidth,
               int32x Degree,
               int32x KnotCount, real32 *Knots,
               int32x *KnotStart,
               int32x bCount,
               real32 const *b,
               real32 *A,
               real32 *ATA,
               real32 *ATb)
{
    COUNT_BLOCK("BuildATAAndATb");

    int32x BandWidth = Degree + 1;
    int32x ATn = KnotCount;

    // The KnotStart array says, for each knot, which sample first
    // causes this knot to become the "anchor" knot.  ie., when you
    // sample a b-spline, you are sampling with some knot as the k_i
    // knot, and all other computations are done relative to that
    // knot.  The KnotStart array is saying, for each knot, which
    // sample index first causes the knot to be the k_i knot.
    //
    // The A matrix is the array of b-spline coefficients
    // for each sample index.  So, if you sampled the spline at that
    // sample's t value, you would use those coefficients.
    //
    // We build both the KnotStart array and the A matrix here in a
    // single loop, since they are intimately related.
    int32x SampleIndex = 0;
    {for(int32x KnotIndex = 0;
         KnotIndex < KnotCount;
         ++KnotIndex)
    {
        KnotStart[KnotIndex] = SampleIndex;

        // Advance as many samples as necessary to get to the
        // next knot, whilst filling in the A matrix with coefficients
        // for each sample we pass.
        while((Knots[KnotIndex] >= (real32)SampleIndex) &&
              (SampleIndex < bCount))
        {
            Coefficients(Degree, &Knots[KnotIndex], (real32)SampleIndex,
                         &A[SampleIndex * BandWidth + (BandWidth - 1)]);
            ++SampleIndex;
        }
    }}
    KnotStart[KnotCount] = SampleIndex;

    // The A array is really actually skewed somewhat, in the sense
    // that the first entry in the A array is actually a non-existant
    // entry in the matrix (it is in a phantom column to the left of
    // the actual first column of A).  This is because the b-splines
    // in Granny were written with a leading knot notation, such that
    // the coefficients extend _behind_ the leading knot.  Since the
    // leading knot corresponds to the first column of the matrix,
    // the coefficients for sampling the first few samples necessarily
    // involve phantom knots that don't exist.  To solve this problem,
    // we wrap or clamp the knots at run-time.  Here when we solve,
    // we choose clamping, since it is the easiest to solve, and it
    // makes no noticable difference which we choose.
    //
    // To handle clamping in the matrix, we simply add the columns that
    // aren't there into the columns that are, "folding" the data to the
    // right.   Since there will be no more phantom columns after BandWidth
    // knots have passed (because the coefficients only extend BandWidth
    // across, by definition), we only have to process the first BandWidth
    // knots for folding.
    {for(int32x KnotIndex = 0;
         KnotIndex < BandWidth;
         ++KnotIndex)
    {
        {for(int32x s = KnotStart[KnotIndex];
             s < KnotStart[KnotIndex + 1];
             ++s)
        {
            // For each knot we do, we have one less column to fold,
            // because there will be no coefficients in that column.
            // So this inner folding loop's length is inversely
            // proportional to the knot index we're on.
            {for(int32x k = 0;
                 k < (BandWidth - KnotIndex - 1);
                 ++k)
            {
                // Fold in a column from the left
                A[s*BandWidth + k + 1] += A[s*BandWidth + k];
            }}
        }}
    }}

    // OK, now we come to by far the most complicated loop in the
    // whole solver.  If you can understand this loop, then you
    // can understand anything else in this file because everything
    // else is pretty wussie compared to this ridiculousness.  Ideally,
    // my comments will help make it clearer what is going on, but in
    // reality, I think it's one of those things you just have to figure
    // out yourself why it works.  Anyhow...
    //
    // The main iteration is over the diagonal of ATn.  This is the
    // natural choice, because hey, we are computing a band-diagonal
    // symmetric matrix, so we really just want to go along the diagonal,
    // compute that entry, and then compute some entries to the right.
    // Then move on.
    //
    // In the body of the loop, we build both ATA and ATb together,
    // since they generally access the same data in the same order.
    // But they do not _need_ to be done together - you could very
    // easily pull out the ATb part and the ATA part and do them
    // separately, because they do not rely on eachother's computations.
    //
    // When reading this loop, it's also worth noting that all
    // computations are wrapped in for(s) loops that run them for a
    // range of sample values.  This is because hey, you're
    // multiplying a large rectangular matrix into a small square one,
    // so you have to do a lot of identical operations on different
    // adjacent rows of A, and the for(s) loops do this.  You can
    // ignore these s loops when looking at the logic initially,
    // because all values in a knot range are treated identically, so
    // you can look at the overall structure of the computation
    // without worrying about the KnotStart particulars.
    //
    // Similarly, all operations on the ATb vector will involve a
    // for(e) loop.  This is because the ATb vector has extra
    // dimensions that are solved as if they were a single row (ie.,
    // position has a bWidth of 3, because there are 3 values per
    // actual row of the vector).  So you can ignore those too.
    // It cuts down the logic to just a for(i)/for(j)/for(k) loop,
    // and it helps to read it that way first.
    {for(int32x i = 0;
         i < ATn;
         ++i)
    {
        // Clear the ATb row.
        real32 *ATbEntry = &ATb[i*bWidth];
        {for(int32x e = 0;
             e < bWidth;
             ++e)
        {
            ATbEntry[e] = 0.0f;
        }}

        // Loop over the BandWidth elements that make up this row
        // of the ATA matrix (the diagonal and some elements to its right).
        {for(int32x j = 0;
             j < BandWidth;
             ++j)
        {
            real32 *ATAEntry = &ATA[i*BandWidth + j];

            // Clear the ATA entry we're working on
            *ATAEntry = 0.0f;

            // Each entry has up to BandWidth terms in it.  Diagonal
            // elements of the matrix have the most terms, and the
            // furthest off-diagonal entries have the least.
            {for(int32x k = 0;
                 k < (BandWidth - j);
                 ++k)
            {
                // Add in the kth term of this ATA entry
                {for(int32x s = KnotStart[i + j + k];
                     s < KnotStart[i + j + k + 1];
                     ++s)
                {
                    real32 A0 = A[BandWidth*s + (BandWidth - 1) - k];
                    real32 A1 = A[BandWidth*s + (BandWidth - 1) - (j + k)];

                    // ATA(i, i + j) += A(i + j + k, R - k) *
                    //                  A(i + j + k, R - (j + k))
                    *ATAEntry += A0*A1;
                }}
            }}

            // Add in the jth term of this ATb entry
            {for(int32x s = KnotStart[i + j];
                 s < KnotStart[i + j + 1];
                 ++s)
            {
                real32 A0 = A[BandWidth*s + (BandWidth - 1) - j];
                {for(int32x e = 0;
                     e < bWidth;
                     ++e)
                {
                    real32 b0 = b[bWidth*s + e];

                    // ATb(i) += A(i + j, R - j) * b(i + j)
                    ATbEntry[e] += A0*b0;
                }}
            }}
        }}
    }}

#if 0
    // This code is for debugging purposes.  It will force the ATA
    // and ATb results to contain that start and end knots exactly,
    // and 1's and 0's in the ATA matrix for the first and final rows /
    // columns so that you should get an exact solve for the endpoints.
    // If you don't, well, something's messed up with the rest of the
    // solver.
    {for(int32x j = 1;
         j < BandWidth;
         ++j)
    {
        ATA[0*BandWidth + j] = 0.0f;
        ATA[(KnotCount - 1)*BandWidth + j] = 0.0f;
        ATA[(KnotCount - 1 - j)*BandWidth + j] = 0.0f;
    }}
    ATA[0*BandWidth] = 1.0f;
    ATA[(KnotCount - 1)*BandWidth] = 1.0f;

    {for(int32x e = 0;
         e < bWidth;
         ++e)
    {
        ATb[0*bWidth + e] = b[0*bWidth + e];
        ATb[(KnotCount - 1)*bWidth + e] =
            b[(KnotStart[KnotCount]-1)*bWidth + e];
    }}
#endif

    DebugDumpA(KnotCount, KnotStart, BandWidth, A);
    DebugDumpb(bCount, bWidth, b);
    DebugDumpATb(KnotCount, bWidth, ATb);
    DebugDumpATA(ATn, BandWidth, KnotStart, Knots, ATA);
}

/* Cholesky Solver

   The idea here is that once the ATA/ATb builder has completed its
   task, we have a symmetric band-diagonal matrix, where the bandwidth
   is very small compared to the total size of the matrix.  For
   example, a quadratic b-spline curve with 30 knots would be a 30x30
   matrix with a bandwidth of 3.

   Cholesky factorization is convenient and compact in this situation,
   because we can quickly run down the diagonal, touch 10 or less (10
   for cubic, 6 for quadratic, 2 for linear, 1 for point) entries per
   diagonal entry, and have a nicely factored result.  We can then
   solve the entire b vector back through it quickly, which is
   important because there are actually many different b vectors for
   the spline (3 for position, 4 for orientation, 9 for scale/shear).
   Since the Cholesky factorization leaves us with most of the work
   already done, it is relatively cheap to push between 3 and 9 solves
   back through it at once.

   Because we never actually need to use the matrix ATA for anything,
   all the factorizing and solving is done in place.  ATA comes into
   SymmetricBandDiagonalCholeskyFactor(), and it leaves as the
   factored matrix C (again, stored just as bands - it's symmetric, so
   only the diagonal and the elements to its right are stored for any
   row).  Then, that factored C matrix is used in
   SymmetricBandDiagonalCholeskySolve() to do a forward-substitution
   (interpretting the bands as lower-triangular), and then a
   backward-substitution (interpretting the bands as
   upper-triangular).

   Mostly everything about this Cholesky factoring and solving code is
   by-the-book.  There are only two exceptions.

   The first exception is that the optimized storage of the ATA matrix
   makes the addressing a bit tricky.  I have annotated every read and
   write into the ATA matrix with what it's proper ATA(i, j) name
   would be to make things clearer.  See the comment about ATA
   addressing for more information on this.

   The second exception is that traditionally, I believe the Cholesky
   factorization is stored with the diagonal elements _uninverted_...
   meaning that the factoring writes back

       ATA(i, i) = SquareRoot(ATA(i, i))

   This is of course because you want the factor C to square back to
   the original matrix.  But since I never actually need to use C for
   anything other than the solve, I save doing inversions later in the
   backwards/forwards substitutions by storing back

       ATA(i, i) = 1.0f / SquareRoot(ATA(i, i))

   which is all the substitutors want.

   It is also worth noting that these routines do not check to see if
   they are at the beginning or end of the matrix, so the code is
   cleaner and there are no branches.  The calling code prepares the
   ATA buffer to be suitable for this behavior (ie., there is an extra
   BandWidth x BandWidth square at the top and bottom of the ATA
   buffer, so it is never possible for it to perform an illegal
   access), by filling the overrun areas with 0's.
*/

void GRANNY
SymmetricBandDiagonalCholeskyFactor(
    int32x ATn,          // Size of ATA - it is ATn x ATn
    int32x BandWidth,    // The bandwidth of ATA
    real32 *ATA)         // ATA itself, stored optimized (just the bands)
{
    COUNT_BLOCK("SymmetricBandDiagonalCholeskyFactor");

    // Step along the diagonal of ATA
    {for(int32x i = 0;
         i < ATn;
         ++i)
    {
        // Calculate the effect of all entries to the left and below
        // the diagonal on all the entries _directly_ below the
        // diagonal.  This looks at an upper triangular set of values
        // whose upper right corner is the diagonal element.  It
        // does computations on the columns of this upper triangular
        // block and then uses the results to modify the column of
        // the diagonal itself (but only below it, never above)
        {for(int32x j = 1;
             j < BandWidth;
             ++j)
        {
            // ATA(i, i - j)
            real32 Multiplier = ATA[BandWidth*(i - j) + j];

            {for(int32x k = 0;
                 k < (BandWidth - j);
                 ++k)
            {
                // ATA(i + k, i - j)
                real32 Entry = ATA[BandWidth*(i - j) + j + k];

                // ATA(i + k, i)
                ATA[BandWidth*i + k] -= Entry*Multiplier;
            }}
        }}

        // Read the diagonal entry ATA(i, i)
        real32 *DiagonalEntry = &ATA[i*BandWidth];

        if(ATA[BandWidth*i] > BSplineSolverDiagonalEpsilon)
        {
            // Turn the diagonal into its own inverse square root.  Note
            // that it would be swell to be able to launch this ahead of
            // time, to gain overlap with the previous loops.  But sadly,
            // we cannot, because we actually update the diagonal many
            // times during that loop, so we can't take the inverse square
            // root until now, when we're done.
            (*DiagonalEntry) = 1.0f / SquareRoot(*DiagonalEntry);

            // Now we finish with this diagonal element by multiplying
            // through all the values _directly_ below the diagonal
            // by the inverse square root we found.
            {for(int32x j = 1;
                 j < BandWidth;
                 ++j)
            {
                // ATA(i, i + j)
                ATA[BandWidth*i + j] *= (*DiagonalEntry);
            }}
        }
    }}

    DebugDumpC(ATn, BandWidth, ATA);
}

void GRANNY
SymmetricBandDiagonalCholeskySolve(
    int32x ATn,        // Size of ATA - it is ATn x ATn
    int32x BandWidth,  // The bandwidth of ATA
    real32 *ATA,       // ATA itself, stored optimized (just the bands)
    int32x ATbStride,  // The number of elements in each vector in ATb
    real32 *ATb)       // ATb itself
{
    COUNT_BLOCK("SymmetricBandDiagonalCholeskySolve");

    // First, we solve Cy = b.
    // Since C is lower triangular, this is just simple forward-substitution.
    {for(int32x i = 0;
         i < ATn;
         ++i)
    {
        // We ignore degenerate rows, and we will fill them in on
        // the back-substitution pass.
        if(ATA[BandWidth*i] > BSplineSolverDiagonalEpsilon)
        {
            // We are working on ATb(i)
            real32 *Value = &ATb[ATbStride*i];

            // Move all the terms of this row over to the other side, so
            // the x value is left alone.  This amounts to subtracting
            // away the contribution of all other elements of ATb besides
            // the one we're solving, multiplied by their appropriate
            // coefficients in ATA.

            // (we guard the loop so we don't write outside ATb, which
            // is the actual result buffer, so it has not been allocated
            // with any padding)
            int32x LoopMax = i + 1;
            if(LoopMax > BandWidth)
            {
                LoopMax = BandWidth;
            }

            {for(int32x j = 1;
                 j < LoopMax;
                 ++j)
            {
                // ATA(i, i - j)
                real32 Coefficient = ATA[BandWidth*(i - j) + j];

                // We have to loop here, because each ATb location
                // actually has multiple elements (ie., 3 for position, etc.)
                {for(int32x c = 0;
                     c < ATbStride;
                     ++c)
                {
                    // ATb(i - j)
                    Value[c] -= ATb[ATbStride*(i - j) + c] * Coefficient;
                }}
            }}

            // Finish the solve of ATb(i) by dividing it by its own matrix
            // coefficient, the diagonal entry ATA(i, i).  Note that it is
            // actually a multiply here, because we stored it inverted in
            // the facoring routine.

            // ATA(i, i)
            real32 InverseCoefficient = ATA[BandWidth*i];

            // We have to loop here, because each ATb location
            // actually has multiple elements (ie., 3 for position, etc.)
            {for(int32x c = 0;
                 c < ATbStride;
                 ++c)
            {
                Value[c] *= InverseCoefficient;
            }}
        }
    }}

    DebugDumpFSATb(ATn, ATbStride, ATb);

    // Next, we solve C^Tx = y.
    // Since C^T is upper triangular, this amounts to back-substitution.
    // The comments for this section can be considered identical to the
    // previous, forward-substitution section.  Literally, the only
    // difference between the two is that this one goes from the bottom
    // of ATA/ATb up, whereas the previous one went from the top down,
    // because we're treating the matrix as upper triangular now.
    {for(int32x i = (ATn - 1);
         i >= 0;
         --i)
    {
        if(ATA[BandWidth*i] > BSplineSolverDiagonalEpsilon)
        {
            // ATb(i)
            real32 *Value = &ATb[ATbStride*i];

            int32x LoopMax = (ATn - i);
            if(LoopMax > BandWidth)
            {
                LoopMax = BandWidth;
            }

            {for(int32x j = 1;
                 j < LoopMax;
                 ++j)
            {
                // ATA(i, i + j)
                real32 Coefficient = ATA[BandWidth*i + j];

                {for(int32x c = 0;
                     c < ATbStride;
                     ++c)
                {
                    Value[c] -= ATb[ATbStride*(i + j) + c] * Coefficient;
                }}
            }}

            // ATA(i, i)
            real32 InverseCoefficient = ATA[BandWidth*i];

            {for(int32x c = 0;
                 c < ATbStride;
                 ++c)
            {
                Value[c] *= InverseCoefficient;
            }}
        }
        else
        {
            // As long as we're not on the final entry, when we
            // encounter a degenerate row, we just copy whatever the
            // next value was.  This is a perfectly valid thing to do
            // for b-spline solves, because duplicating values is
            // always going to be well-behaved.  Note that this only
            // occurs when multiple knots are present (ie., more than
            // one knot with the same t value).
            if(i < (ATn - 1))
            {
                real32 *To = &ATb[ATbStride*i];
                real32 *From = &ATb[ATbStride*(i + 1)];
                {for(int32x c = 0;
                     c < ATbStride;
                     ++c)
                {
                    To[c] = From[c];
                }}
            }
        }
    }}

    DebugDumpBSATb(ATn, ATbStride, ATb);
}
