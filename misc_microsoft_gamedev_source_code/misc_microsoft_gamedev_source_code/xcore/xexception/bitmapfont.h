//============================================================================
// bitmapfont.h
// Copyright (c) 2002, Blue Shift, Inc.
// Copyright (c) 2005, Ensemble Studios
//============================================================================
#pragma once

struct Bitmap_Font
{
	uchar* Pbitmap;
	uint width;
	uint height;
	uint cell_width;
	uint cell_height;
	uint row_cells;
	uint char_width;
	uint char_low;
	uint char_high;

	Bitmap_Font(uchar* Pbitmap, uint width, uint height, uint cell_width, uint cell_height, uint row_cells, uint char_width, uint char_low, uint char_high)
	{
		this->Pbitmap = Pbitmap;
		this->width = width;
		this->height = height;
		this->cell_width = cell_width;
		this->cell_height = cell_height;
		this->row_cells = row_cells;
		this->char_width = char_width;
		this->char_low = char_low;
		this->char_high = char_high;
	}
	
	template <class PIXEL_FUNCTOR>
	void render(const char* Pstr, uint x, uint y, uint x_scale, uint y_scale, PIXEL_FUNCTOR& render_pixel)
	{
		while (*Pstr)
		{
			uint c = *Pstr++;
			if ((c < char_low) || (c >= char_high))
			{
				x += char_width * x_scale;
				continue;
			}

			c -= char_low;
			
			const uint src_x = (c % row_cells) * cell_width;
			const uint src_y = (c / row_cells) * cell_height;
			const uchar* Psrc = Pbitmap + src_x + src_y * width; 

			for (uint dst_y = 0; dst_y < cell_height; dst_y++)
			{
				for (uint dst_x = 0; dst_x < char_width; dst_x++)
				{
				   for (uint sy = 0; sy < y_scale; sy++)
					   for (uint sx = 0; sx < x_scale; sx++)
					      render_pixel(x + dst_x * x_scale + sx, y + dst_y * y_scale + sy, Psrc[dst_x]);
			   }
			   
				Psrc += width;
			}

			x += char_width * x_scale;
		}
	}
};
