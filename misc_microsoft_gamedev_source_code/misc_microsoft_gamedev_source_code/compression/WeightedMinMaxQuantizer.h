// File: WeightedMinMaxQuantizer.cpp
#pragma once

template <typename BCellType>
class Weighted_MinMax_Quantizer
{
public:
   struct Weighted_Vec
   {
      BCellType vec;
      float weight;
      int cell_index;

      Weighted_Vec() { }
      Weighted_Vec(const BCellType& v, float w, int c) : vec(v), weight(w), cell_index(c) { }
   };
   
protected:

   typedef BDynamicArray<Weighted_Vec> Weighted_Vec_Cont;
   typedef BDynamicArray<BCellType> Cell_Cont;

   Weighted_Vec_Cont m_vecs;

   BCellType m_centroid;
   double m_total_weight;

   Cell_Cont m_cells;

   // -1 if no split can be found
   int find_split(float& md2) const
   {
      float max_dist2 = 0.0f;
      int split_vec = -1;

      for (uint j = 0; j < m_vecs.size(); j++)
      {
         const Weighted_Vec& vec = m_vecs[j];
         const BCellType& cell = m_cells.at(vec.cell_index);

         const float dist2 = cell.dist2(vec.vec) * vec.weight;
         if (dist2 > max_dist2)
         {
            max_dist2 = dist2;
            split_vec = j;
         }
      }

      md2 = max_dist2;

      return split_vec;
   }

   void reassign_vecs(void)
   {
      const BCellType& new_cell = m_cells.back();
      const int new_cell_index = m_cells.size() - 1;

      for (uint i = 0; i < m_vecs.size(); i++)
      {
         Weighted_Vec& weighted_vec = m_vecs[i];				

         if (weighted_vec.cell_index != new_cell_index)
         {
            const BCellType& orig_cell = m_cells.at(weighted_vec.cell_index);

            if (weighted_vec.vec.dist2(orig_cell) > weighted_vec.vec.dist2(new_cell))
               weighted_vec.cell_index = new_cell_index;
         }
      }
   }

   void calc_centroids(void)
   {
      BDynamicArray<double> cell_weight(m_cells.size());

      uint i;

      for (i = 0; i < m_cells.size(); i++)
         m_cells[i].setZero();

      for (i = 0; i < m_vecs.size(); i++)
      {
         const Weighted_Vec& weighted_vec = m_vecs[i];				

         const int cell_index = weighted_vec.cell_index;

         m_cells.at(cell_index) += weighted_vec.vec * weighted_vec.weight;
         cell_weight[cell_index] += weighted_vec.weight;
      }

      for (i = 0; i < m_cells.size(); i++)
      {
         if (cell_weight[i] == 0.0f)
            continue;

         //assert(cell_weight[i] > 0.0f);
         m_cells[i] *= static_cast<float>(1.0f / cell_weight[i]);
      }
   }

public:
   Weighted_MinMax_Quantizer() : m_total_weight(0.0f)
   {
      m_centroid.setZero();
   }

   void insert(const BCellType& v, float w)
   {
      m_vecs.pushBack(Weighted_Vec(v, w, 0));

      m_centroid += v * w;
      m_total_weight += w;
   }

   int num_input_vecs(void) const
   {
      return m_vecs.size();
   }

   double total_input_weight(void) const
   {
      return m_total_weight;
   }

   const Weighted_Vec& input_vec(int i) const
   {
      return m_vecs.at(i);
   }

   int num_output_cells(void) const
   {
      return m_cells.size();
   }

   const BCellType& output_cell(int i) const
   {
      return m_cells.at(i);
   }
   
   int find_best_cell(const BCellType& cell) const
   {
      float bestDist2 = 1e+30f;
      int bestCell;
      for (uint i = 0; i < m_cells.size(); i++)
      {
         float dist2 = cell.dist2(m_cells[i]);
         if (dist2 < bestDist2)
         {
            bestDist2 = dist2;
            bestCell = i;
         }
      }
      return bestCell;
   }

   void quantize(float thresh_dist2, int max_output_cells)
   {
      if ((!num_input_vecs()) || (m_cells.size()))
         return;

      assert(m_total_weight > 0.0f);

      m_cells.pushBack(BCellType(m_centroid * static_cast<float>((1.0f / m_total_weight))));

      int k = 1;
      for ( ; ; )
      {
         float max_dist2;
         const int split_vec_index = find_split(max_dist2);

         if (split_vec_index == -1)
            break;

#if WEIGHTED_MINMAX_QUANTIZER_DEBUG
         debug_message("Weighted_MinMax_Quantizer: cells: %i max_dist2: %f", k, max_dist2);
#endif

         //if ((max_dist2 < thresh_dist2) && (k >= num_cells))
         if (max_dist2 < thresh_dist2) 
         {
#if WEIGHTED_MINMAX_QUANTIZER_DEBUG
            debug_message("Weighted_MinMax_Quantizer: done");
#endif
            break;
         }

         m_vecs.at(split_vec_index).cell_index = m_cells.size();

         m_cells.pushBack( BCellType(m_vecs[split_vec_index].vec) );

         assert(m_cells.back().dist2(m_vecs[split_vec_index].vec) == 0.0f);

         reassign_vecs();

         calc_centroids();

         k++;
         if (k == max_output_cells)         
            break;
         debugPrintf("Cells: %i\n", k);
      }
   }
   
private:   
   static void debugPrintf(const char* pMsg, ...)
   {
      va_list args;
      va_start(args, pMsg);
      char buf[512];
      vsprintf_s(buf, sizeof(buf), pMsg, args);
      va_end(args);
      OutputDebugString(buf);
   }
};
