// readtga.h
// Quick and dirty TGA file format reader -- only supports
// uncompressed 24bpp files.

#ifndef READTGA_H
#define READTGA_H

#include "..\JPGmain.h" 

#pragma pack( push, enter_include1 )
#pragma pack(1)
typedef struct 
{
  uchar id_len;        //0
  uchar cmap;          //1
  uchar type;          //2
  ushort cmap_first;   //3
  ushort cmap_len;     //5
  uchar cmap_size;     //7
  ushort x_org;        //8
  ushort y_org;        //10
  ushort width;        //12
  ushort height;       //14
  uchar depth;         //16
  uchar descr;         //17
} TGA_Header;
#pragma pack( pop, enter_include1 )

class TGA_Reader
{
  FILE *m_Pfile;
  TGA_Header m_header;
  uchar *m_Pbuf;
  int m_cur_line;
  int m_start_pos;

public:

  TGA_Reader() : m_Pfile(NULL), m_Pbuf(NULL), m_cur_line(0), m_start_pos(0)
  {
    memset(&m_header, 0, sizeof(m_header));
  }

  void close(void)
  {
    if (m_Pfile)
      fclose(m_Pfile);

    delete m_Pbuf;
    m_Pbuf = NULL;
  }

  ~TGA_Reader()
  {
    close();
  }

  bool open(const char *Pfilename)
  {
    close();

    m_Pfile = fopen(Pfilename, "rb");
    if (!m_Pfile)
      return (true);

    if (fread(&m_header, sizeof(m_header), 1, m_Pfile) != 1)
    {
      close();
      return (true);
    }

#if 0
    for (int skip = m_header.id_len; skip > 0; skip--)
      if (fgetc(m_Pfile) == EOF)
      {
        close();
        return (true);
      }
#endif
    if (m_header.id_len)
      fseek(m_Pfile, m_header.id_len, SEEK_CUR);

    if ((m_header.width < 1) ||
        (m_header.height < 1) ||
        (m_header.descr >> 6) ||
        (m_header.type != 2) ||
        (m_header.depth != 24) ||
        (m_header.cmap_len))
    {
      close();
      return (true);
    }

    m_Pbuf = new uchar[3 * m_header.width];
    m_cur_line = 0;
    
    m_start_pos = ftell(m_Pfile);

    return (false);
  }

  int width(void) const
  {
    assert(m_Pfile);
    return (m_header.width);
  }

  int height(void) const
  {
    assert(m_Pfile);
    return (m_header.height);
  }

  bool flip_x(void) const
  {
    return (m_header.descr & 0x10) != 0;
  }

  bool flip_y(void) const
  {
    return (m_header.descr & 0x20) == 0;
  }

  const uchar *read_line(void)
  {
    assert(m_Pfile);

    if (m_cur_line >= m_header.height)
      return (NULL);

    if (fread(m_Pbuf, m_header.width * 3, 1, m_Pfile) != 1)
      return (NULL);

    m_cur_line++;

    return (m_Pbuf);
  }

  void seek_to_start(void)
  {
    m_cur_line = 0;
    fseek(m_Pfile, m_start_pos, SEEK_SET);
  }
};

#endif

