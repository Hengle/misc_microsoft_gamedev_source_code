// jpghuff.cpp                                                               

/*----------------------------------------------------------------------------*/
#include "jpge.h"
/*----------------------------------------------------------------------------*/
/*   This  function implements an implicit binary tree based priority list.   */
/* This  is  used  to  speed  up  the  Huffman  tree  construction process.   */
/* Specifically, this function takes a node and moves it down the  tree (if   */
/* possible) to keep the tree properly organized.                             */
/*                                                                            */
/*   All children in this binary  tree  must  be  greater than or  equal to   */
/* their parents.  There is not distinction between left and right children   */
/* in this implementation.                                                    */
/*                                                                            */
/*   The binary  tree  is  stored  implicitly  in  a  table.   There are no   */
/* children/parent pointers needed  to  navigate  the  tree,  because  each   */
/* node's location in the table is significant.                               */
/*----------------------------------------------------------------------------*/
void JPEG_Encoder::down_heap(int *heap, uint *sym_freq, int heap_len, int i)
{
  int j, k;

  k = heap[i];

  while ((j = 2 * i) <= heap_len)  /* Find parent's two children */
  {
    /* Since left and right children aren't in any
     * particular order, find the lowest of the two.
     */

    if (j < heap_len)
    {
      if (sym_freq[heap[j]] > sym_freq[heap[j + 1]])
        j++;
      else if (sym_freq[heap[j]] == sym_freq[heap[j + 1]])
      {
        if (heap[j] < heap[j + 1])
          j++;
      }
    }

    /* Stop if the node doesn't need to move down the tree. */

    if (sym_freq[k] < sym_freq[heap[j]])
      break;
    else if (sym_freq[k] == sym_freq[heap[j]])
    {
      if (k > heap[j])
        break;
    }

    heap[i] = heap[j]; i = j;
  }

  heap[i] = k;
}
/*----------------------------------------------------------------------------*/
void JPEG_Encoder::huffman_code_sizes(
  int num_symbols, uint *sym_freq,
  int * *pcode_sizes)
{
  int i, j, k;
  int *code_sizes = NULL;
  int *others = NULL;       /* linked lists */
  int *heap = NULL;         /* implicit binary tree based priority list */
  int heap_len;             /* number of entries in priority list */
  
   /* Allocate implicit binary tree, plus
   * one extra entry for a dummy node.
   */

   heap = reinterpret_cast<int*>(malloc_msg((num_symbols + 1) * sizeof(int)));

   /* Allocate linked lists table */

   others = reinterpret_cast<int*>(malloc_msg(num_symbols * sizeof(int)));

   /* Allocate codesize table */

   code_sizes = reinterpret_cast<int*>(malloc_msg(num_symbols * sizeof(int)));

   /* Initialize linked lists */

   for (i = 0; i < num_symbols; i++)
      others[i] = -1;

   /* All symbols start at codesize 0 */

   for (i = 0; i < num_symbols; i++)
      code_sizes[i] = 0;

   /* Enter all non-zero symbols into
   * the implicit binary  tree based
   * priority list.
   */

   heap_len = 1;

   for (i = 0; i < num_symbols; i++)
      if (sym_freq[i] != 0)
         heap[heap_len++] = i;

   heap_len--;

   /* Are their enough symbols for
   * at least one internal  node?
   */

   if (heap_len <= 1)
   {
      if (heap_len == 0) /* No symbols? */
         goto hcs_exit;

   /* Only one non-zero symbol: assign it
      * the obvious codesize of 1 and exit.
      */

      code_sizes[heap[1]] = 1;

      goto hcs_exit;
   }

   /* At  this point the priority list is totally disorganized.
   * In order to make the tree into a priority list, start at
   * the last leaf's parent and order subtrees until the root
   * is reached.
   */

   j = (heap_len >> 1);

   while (j != 0)      /* node 0 is a dummy node */
      down_heap(heap, sym_freq, heap_len, j--); /* make subtree */

   /* Build Huffman tree until root is created */

   do
   {
      i = heap[1];                             /* i = lowest node */

      heap[1] = heap[heap_len--];              /* delete lowest node */

      down_heap(heap, sym_freq, heap_len, 1);  /* fix the priority list */

      j = heap[1];                             /* j = next lowest node */

      sym_freq[j] += sym_freq[i];              /* make a new internal node */

      down_heap(heap, sym_freq, heap_len, 1);  /* fix the priority list */

      /* Increment the code sizes of the new internal node's children,
         * and all other nodes which share the same subtree. This 
         * algorithm comes from the JPEG specification.
         */

      do { code_sizes[k = j]++; } while ((j = others[j]) != -1);

      others[k] = i;        /* chain the two node lists together */

      do { code_sizes[i]++; } while ((i = others[i]) != -1);

   } while (heap_len != 1); /* until there's only one internal node left */

   /* Note: At this point, sym_freq[heap[1]] contains
   * the total frequency of all symbols.
   */

hcs_exit:
   ;
  
  free(heap);
  free(others);

  *pcode_sizes = code_sizes;
}
/*----------------------------------------------------------------------------*/
/*    This function sorts the  symbols  by  their  Huffman  code  sizes  in   */
/* preparation for the code assignment process.                               */
/*----------------------------------------------------------------------------*/
void JPEG_Encoder::huffman_sort_code_sizes(
  int num_symbols, int *code_sizes,
  int * *pnum_codes,
  int * *pcode_list, int *pcode_list_len)
{
   int i, j;
   int *num_codes = NULL;
   int *next_code = NULL;
   int *code_list = NULL;
   int code_list_len = 0;
  
   num_codes = reinterpret_cast<int*>(malloc_msg((1 + 32) * sizeof(int)));
   next_code = reinterpret_cast<int*>(malloc_msg((1 + 32) * sizeof(int)));
   code_list = reinterpret_cast<int*>(malloc_msg(num_symbols * sizeof(int)));

   /* Find number of symbols of each codesize */

   /* Note that the maximum  code  size  better  be  32,  otherwise  this
   * function is  going  to trash memory.  There should be no chance of
   * this ever  happening  here:  The  maximum  total  frequency  of all
   * symbols in this implementation  is  well below the threshold needed
   * for a symbol's Huffman code to exceed 32 bits.  (See the "Fibonacci"
   * sequence.)
   */

   for (i = 0; i <= 32; i++)
   num_codes[i] = 0;

   for (i = 0; i < num_symbols; i++)
   num_codes[code_sizes[i]]++;

   /* Make a table of all symbols sorted by codesize */

   for (i = 1, j = 0; i <= 32; i++)
   {
      next_code[i] = j;
      j += num_codes[i];
   }

   for (i = 0; i < num_symbols; i++)
   {
      if ((j = code_sizes[i]) != 0)
      {
         code_list[next_code[j]++] = i;
         code_list_len++;
      }
   }

   free(next_code);

   *pnum_codes    = num_codes;
   *pcode_list    = code_list;
   *pcode_list_len = code_list_len;
}
/*----------------------------------------------------------------------------*/
/*   This  function  limits  the  maximum  code size  of  each sumbol to      */
/* max_code_size.   In  order to decrease the code size of some               */
/* symbols, other symbols must be increased in size.                          */
/*                                                                            */
/* Does a Huffman tree modified so that each symbol's code size is below a    */
/* threshold still obey the sibling property?                                 */
/*----------------------------------------------------------------------------*/
void JPEG_Encoder::huffman_fix_code_sizes(
  int *num_codes,
  int code_list_len,
  int max_code_size)
{
  int i;
  uint total;

  if (code_list_len > 1)  /* 2 or more codes? */
  {
    /* clamp & count all codes which exceed the maximum code size */

    for (i = max_code_size + 1; i <= 32; i++)
      num_codes[max_code_size] += num_codes[i];

    /* find the total of all the *backwards* Huffman codes */

    total = 0;
    for (i = max_code_size; i > 0; i--)
      total += ( ((uint)num_codes[i]) << (max_code_size - i) );

    /* At this point, some codes may have decreased in size. Fix the tree. */

    while (total != (1UL << max_code_size))  /* Until the code set is valid */
    {
      /* Take the bad symbol out of the tree */

      num_codes[max_code_size]--;

      /* Find the symbol a partner */

      for (i = max_code_size - 1; i > 0; i--)
      {
        if (num_codes[i])
        {
          /* This code, in effect, creates an internal
           * node for the bad symbol and its partner.
           */

          num_codes[i]--;          /* remove leaf from level i */
          num_codes[i + 1] += 2;   /* move the two leafs to level i + 1 */

          break;
        }
      }

      total--;   /* total sum = one less */
    }
  }
}
/*----------------------------------------------------------------------------*/

