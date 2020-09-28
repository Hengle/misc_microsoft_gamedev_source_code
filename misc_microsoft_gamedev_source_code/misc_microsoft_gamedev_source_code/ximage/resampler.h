// RESAMPLER.C, Filtering image rescaler v2.1 
// Rich Geldreich
// richgel@voicenet.com
// Feb. 1996: Creation, losely based on Schumacher's resampler in Graphics Gems 3.
// Oct. 2000: Ported to C++, tweaks.
// May 2001: Continous to discrete mapping fixed, box filter tweaked.
// March 9, 2002: Kaiser filter grabbed from Jonathan Blow's GD magazine mipmap sample code.
// Sept. 8, 2002: Comments cleaned up a bit.

#ifndef RESAMPLER_H
#define RESAMPLER_H

//#define RESAMPLE_DEBUG
//#define RESAMPLE_DEBUG_OPS

// float or double
typedef float Resample_Real;

class Resampler
{
public:
  typedef Resample_Real Sample;
  
  struct Contrib
  {
    Resample_Real weight;
    unsigned short pixel;
  };

  struct Contrib_List
  {
    unsigned short n;
    Contrib* p;
  };

  enum Boundary_Op
  {
    BOUNDARY_WRAP = 0,
    BOUNDARY_REFLECT = 1,
    BOUNDARY_CLAMP = 2
  };

  enum Status
  {
    STATUS_OKAY = 0,
    STATUS_OUT_OF_MEMORY = 1,
    STATUS_BAD_FILTER_NAME = 2
  };

  // The maximum number of scanlines that can be buffered at one time.
  enum { MAX_SCAN_BUF_SIZE = 4096 };

private:
  Resampler();
  Resampler(const Resampler& o);
  Resampler& operator= (const Resampler& o);
  
#ifdef RESAMPLE_DEBUG_OPS
  int total_ops;
#endif

  int m_intermediate_x;

  int m_resample_src_x;
  int m_resample_src_y;
  int m_resample_dst_x;
  int m_resample_dst_y;

  Boundary_Op m_boundary_op;

  Sample* m_Pdst_buf;
  Sample* m_Ptmp_buf;

  Contrib_List* m_Pclist_x;
  Contrib_List* m_Pclist_y;

  bool m_clist_x_forced;
  bool m_clist_y_forced;

  bool m_delay_x_resample;

  int* m_Psrc_y_count;
  unsigned char* m_Psrc_y_flag;

  struct Scan_Buf
  {
    int scan_buf_y[MAX_SCAN_BUF_SIZE];
    Sample* scan_buf_l[MAX_SCAN_BUF_SIZE];
  };

  Scan_Buf* m_Pscan_buf;

  int m_cur_src_y;
  int m_cur_dst_y;

  Status m_status;

  void resample_x(Sample* Pdst, const Sample* Psrc);
  void scale_y_mov(Sample* Ptmp, const Sample* Psrc, Resample_Real weight, int dst_x);
  void scale_y_add(Sample* Ptmp, const Sample* Psrc, Resample_Real weight, int dst_x);
  void clamp(Sample* Pdst, int n);
  void resample_y(Sample* Pdst);

	int reflect(const int j, const int src_x, const Boundary_Op boundary_op);

  Contrib_List* make_clist(
    int src_x, int dst_x, Boundary_Op boundary_op,
    Resample_Real (*Pfilter)(Resample_Real),
    Resample_Real filter_support,
		Resample_Real filter_scale,
		Resample_Real src_ofs);

  int count_ops(Contrib_List* Pclist, int k)
  {
    int i, t = 0;
    for (i = 0; i < k; i++)
      t += Pclist[i].n;
    return (t);
  }

  Resample_Real m_lo;
  Resample_Real m_hi;

  Resample_Real clamp_sample(Resample_Real f)
  {
    if (f < m_lo)
      f = m_lo;
    else if (f > m_hi)
      f = m_hi;
    return f;
  }

public:
  Resampler(
    int src_x, int src_y,
    int dst_x, int dst_y,
    Boundary_Op boundary_op = BOUNDARY_CLAMP,
    Resample_Real sample_low = 0.0f, Resample_Real sample_high = 0.0f,		// output samples aren't clamped if sample_low >= sample_high
    const char* Pfilter_name = "lanczos4",
    Contrib_List* Pclist_x = NULL,
    Contrib_List* Pclist_y = NULL,
		Resample_Real filter_x_scale = 1.0f,
		Resample_Real filter_y_scale = 1.0f,
		Resample_Real src_x_ofs = 0.0f, 
		Resample_Real src_y_ofs = 0.0f);

  ~Resampler();
  
  // Reinits resampler so it can handle another frame.
  void restart(void);

	// true on failure
  bool put_line(const Sample* Psrc);

  // NULL if no lines are available
	const Sample* get_line(void);

  Status status(void) const { return m_status; }

  void get_clists(
    Contrib_List** ptr_clist_x,
    Contrib_List** ptr_clist_y);

	Contrib_List* get_clist_x(void) const {	return m_Pclist_x; }
	Contrib_List* get_clist_y(void) const {	return m_Pclist_y; }

  static int get_filter_num(void);
  static char* get_filter_name(int filter_num);
};

#endif

