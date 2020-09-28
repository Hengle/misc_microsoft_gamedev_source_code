//-----------------------------------------------------------------------------
// File: stream.h
// Binary stream I/O classes, with minimal support for text I/O.
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef STREAM_H
#define STREAM_H

#include "common/utils/utils.h"
#include "common/math/math.h"

namespace gr
{
	class Stream
	{
	public:
		Stream(const std::string& name) : 
			mName(name) 
		{ 
		}

		virtual ~Stream() 
		{ 
		}

		const std::string& getName(void) const 
		{ 
			return mName; 
		}

		void setName(const std::string& name)
		{
			mName = name;
		}

		virtual int size(void) const = 0;

		virtual int readBytes(void* p, int n) = 0;
		virtual int writeBytes(const void* p, int n) = 0;
		
		virtual int curOfs(void) const = 0;
		virtual int bytesLeft(void) const = 0;
				
		virtual void seek(int ofs, bool absolute = true) = 0;
		virtual bool seekable(void) const = 0;
		
		virtual const void* ptr(int len) { len; return NULL; } 

		virtual bool readable(void) const = 0;
		virtual bool writable(void) const = 0;
    
		// true on error
		virtual bool errorStatus(void) const { return false; }
		
		// true if object is in a "signaled" state:
		// Either the object has a non-zero error status, or a readable object is out of data.
		virtual operator bool() const 
		{ 
			if (errorStatus())
				return true;

			if ((readable()) && (!bytesLeft()))
				return true;

			return false;
		}
				
		virtual void printf(const char* pMsg, ...)
		{
			va_list args;
			va_start(args, pMsg);
			char buf[1024];
			_vsnprintf(buf, sizeof(buf), pMsg, args);
			va_end(args);

			writeBytes(buf, static_cast<int>(strlen(buf)));
		}

		// -1 at EOF
		virtual int getc(void) 
		{
			if (bytesLeft() == 0)
				return -1;
			uint8 c;
			readObj(c);
			return c;
		}

		virtual Stream& putc(uint8 c)
		{
			return writeObj(c);
		}

		template<class Type> 
		Stream& readObj(Type& obj) 
		{ 
			readBytes(&obj, sizeof(Type)); 
			return *this; 
		}

		template<class Type> 
		Stream& writeObj(const Type& obj) 
		{ 
			writeBytes(&obj, sizeof(Type)); 
			return *this; 
		}

		template<class Type>
		Type readValue(void) 
		{ 
			Type ret = 0; 
			readObj(ret); 
			return ret; 
		}

		template<class Type>
		Stream& readVecRaw(Type& v, int numExpected = -1)
		{
			v.clear();

			int numElements = readValue<int>();
			if ((-1 != numExpected) && (numElements != numExpected))
			{
				Trace("Stream::readVec: Unexpected number of elements in stream!\n");
				return *this;
			}

			v.resize(numElements);
//			for (int i = 0; i < numElements; i++)
//				readObj(v[i]);
			readBytes(&v[0], numElements * sizeof(Type::value_type));
						
			return *this;
		}

		template<class Type>
		Stream& readVec(Type& v, int numExpected = -1)
		{
			v.clear();

			int numElements = readValue<int>();
			if ((-1 != numExpected) && (numElements != numExpected))
			{
				Trace("Stream::readVec: Unexpected number of elements in stream!\n");
				return *this;
			}

			v.resize(numElements);
			for (int i = 0; i < numElements; i++)
				*this >> v[i];

			return *this;
		}

		template<class Type>
		Stream& writeVecRaw(const Type& v)
		{
			writeObj(static_cast<int>(v.size()));
			
			//for (int i = 0; i < v.size(); i++)
			//	writeObj(v[i]);
			
			writeBytes(&v[0], static_cast<int>(v.size()) * sizeof(Type::value_type));

			return *this;
		}

		template<class Type>
		Stream& writeVec(const Type& v)
		{
			writeObj(static_cast<int>(v.size()));
			
			for (int i = 0; i < v.size(); i++)
				*this << v[i];

			return *this;
		}
											
	protected:
		std::string mName;

		Stream(const Stream& b);
		Stream& operator= (const Stream& b);
	};

	inline Stream& operator<< (Stream& dest, const bool& i)		{	return dest.writeObj(i); }
	inline Stream& operator<< (Stream& dest, const int8& i)		{	return dest.writeObj(i); }
	inline Stream& operator<< (Stream& dest, const uint8& i)	{ return dest.writeObj(i); }
	inline Stream& operator<< (Stream& dest, const int16& i)	{ return dest.writeObj(i); }
	inline Stream& operator<< (Stream& dest, const uint16& i) { return dest.writeObj(i); }
	inline Stream& operator<< (Stream& dest, const int32& i)	{ return dest.writeObj(i); }
	inline Stream& operator<< (Stream& dest, const uint32& i) { return dest.writeObj(i); }
	inline Stream& operator<< (Stream& dest, const int64& i)	{ return dest.writeObj(i); }
	inline Stream& operator<< (Stream& dest, const uint64& i) { return dest.writeObj(i); }
	inline Stream& operator<< (Stream& dest, const float& i)	{ return dest.writeObj(i); }
	inline Stream& operator<< (Stream& dest, const double& i) { return dest.writeObj(i); }
	inline Stream& operator<< (Stream& dest, const char* pMsg) { dest.writeBytes(pMsg, static_cast<int>(strlen(pMsg))); return dest; }
	
	template<class T> inline Stream& operator<< (Stream& dest, const std::vector<T>& vec)
	{
		return dest.writeVec(vec);
	}
	
	inline Stream& operator>> (Stream& src, bool& i)		{ return src.readObj(i); }
	inline Stream& operator>> (Stream& src, int8& i)		{ return src.readObj(i); }
	inline Stream& operator>> (Stream& src, uint8& i)		{ return src.readObj(i); }
	inline Stream& operator>> (Stream& src, int16& i)		{ return src.readObj(i); }
	inline Stream& operator>> (Stream& src, uint16& i)	{ return src.readObj(i); }
	inline Stream& operator>> (Stream& src, int32& i)		{ return src.readObj(i); }
	inline Stream& operator>> (Stream& src, uint32& i)	{ return src.readObj(i); }
	inline Stream& operator>> (Stream& src, int64& i)		{ return src.readObj(i); }
	inline Stream& operator>> (Stream& src, uint64& i)	{ return src.readObj(i); }
	inline Stream& operator>> (Stream& src, float& i)		{ return src.readObj(i); }
	inline Stream& operator>> (Stream& src, double& i)	{ return src.readObj(i); }
			
	template<class T> inline Stream& operator>> (Stream& src, std::vector<T>& vec)
	{
		return src.readVec(vec);
	}
		
	class WritableDataStream : public Stream
	{
		WritableDataStream(const WritableDataStream& b);
		WritableDataStream& operator= (const WritableDataStream& b);

		std::vector<uint8> mBuf;
	public:
		WritableDataStream(int reserveSize = -1, const std::string& name = "?") :
			Stream(name)
		{ 
			if (-1 != reserveSize)
				mBuf.reserve(reserveSize);
		}

		virtual int size(void) const
		{
			return static_cast<int>(mBuf.size());
		}

		virtual int readBytes(void* p, int n)
		{
			p;
			n;
			StaticAssert(false);
			return 0;
		}
		
		virtual int writeBytes(const void* p, int n)
		{
			Assert(p);
			if (n <= 0)
				return 0;
			
			mBuf.insert(mBuf.end(), reinterpret_cast<const uint8*>(p), reinterpret_cast<const uint8*>(p) + n);
			
			return n;
		}
		
		virtual int curOfs(void) const
		{
			return size();
		}

		virtual int bytesLeft(void) const
		{
			return 0;
		}
				
		virtual void seek(int ofs, bool absolute = true)
		{
			ofs;
			absolute;
			StaticAssert(false);
		}

		virtual bool readable(void) const { return false; }
		virtual bool writable(void) const { return true; }
		virtual bool seekable(void) const {	return false;	}
		
		virtual const void* ptr(int len) 
		{ 
			if (len > size())
				return NULL;
			return &mBuf[0];
		} 
	};

	class ReadableDataStream : public Stream
	{
		ReadableDataStream();
		ReadableDataStream(const ReadableDataStream& b);
		ReadableDataStream& operator= (const ReadableDataStream& b);

		const uint8* mpBuf;
		const uint8* mpCur;
		int mSize, mLeft;
		
	public:
		ReadableDataStream(const void* p, int n, const std::string& name = "?") :
			Stream(name),
			mpBuf(reinterpret_cast<const uint8*>(p)),
			mpCur(reinterpret_cast<const uint8*>(p)),
			mSize(n),
			mLeft(n)
		{ 
		}

		virtual int size(void) const
		{
			return mSize;
		}

		virtual int readBytes(void* p, int n)
		{
			Assert(p);
			if (n <= 0)
				return 0;

			n = Math::Min(mLeft, n);
			memcpy(p, mpCur, n);
			mpCur += n;
			mLeft -= n;
			return n;
		}
		
		virtual int writeBytes(const void* p, int n)
		{
			p;
			n;
			StaticAssert(false);
			return 0;
		}
		
		virtual int curOfs(void) const
		{
			return mpCur - mpBuf;
		}

		virtual int bytesLeft(void) const
		{
			return mLeft;
		}
				
		virtual void seek(int ofs, bool absolute = true)
		{
			int aOfs = ofs;
			if (!absolute)
				aOfs += curOfs();

			Verify(ofs >= 0);
			Verify(ofs < mSize);

			mpCur = mpBuf + aOfs;
			mLeft = mSize - aOfs;
		}

		virtual bool readable(void) const { return true; }
		virtual bool writable(void) const { return false; }
		virtual bool seekable(void) const	{	return true; }
		
		virtual const void* ptr(int len) 
		{ 
			if (len >= size())
				return NULL;
			return mpBuf;
		} 
	};

	class FILEStream : public Stream
	{
		FILEStream(const FILEStream& b);
		FILEStream& operator= (const FILEStream& b);

		bool mAutoClose;
		FILE* mpFile;
		int mSize, mStartOfs, mLeft, mOfs;
		bool mReadable, mWritable;
				
	public:
		FILEStream() :
			Stream(""),
			mAutoClose(false),
			mpFile(NULL),
			mSize(0),
			mStartOfs(0),
			mLeft(0),
			mReadable(0),
			mWritable(0),
			mOfs(0)
		{
		}

		FILEStream(FILE* pFile, bool readable = true, bool writable = false, bool autoClose = false, int size = -1, const std::string& name = "?") :
			Stream(name),
			mpFile(DebugNull(pFile)),
			mAutoClose(autoClose),
			mSize(size),
			mLeft(size),
			mReadable(readable),
			mWritable(writable)
		{
			mStartOfs = mOfs = ftell(mpFile);
									
			if (size == -1)
			{
				fseek(mpFile, 0, SEEK_END);
				mSize = mLeft = ftell(mpFile) - mStartOfs;
				fseek(mpFile, mStartOfs, SEEK_SET);
			}
		}

		FILEStream(const char* pFilename, bool forReading = true, bool errorOnFailure = true) :
			Stream(pFilename),
			mpFile(fopen(pFilename, forReading ? "rb" : "wb")),
			mAutoClose(true),
			mReadable(forReading),
			mWritable(!forReading)	
		{
			if ((errorOnFailure) && (!mpFile))
				Error("Unable to open file \"%s\" for %s!\n", pFilename, forReading ? "reading" : "writing");

			mStartOfs = mSize = mLeft = mOfs = 0;

			if (mpFile)
			{
				fseek(mpFile, 0, SEEK_END);
				mSize = mLeft = ftell(mpFile) - mStartOfs;
				fseek(mpFile, mStartOfs, SEEK_SET);
			}
		}

		virtual ~FILEStream()
		{
			if (mAutoClose)
				close();
		}
    
		virtual int size(void) const 
		{
			Assert(mpFile);
			return mSize; 
		}

		virtual int readBytes(void* p, int n)
		{
			Assert(n >= 0);
			Assert(mpFile);
			Assert(mReadable);

			if (!n)
				return 0;

			Verify(n <= mLeft);
			
			Assert(ftell(mpFile) == mOfs);

			const int ret = static_cast<int>(fread(p, 1, n, mpFile));
			
			mOfs += ret;
			mLeft -= ret;
			
			return ret;
		}

		virtual int writeBytes(const void* p, int n)
		{
			Assert(n >= 0);
			Assert(mpFile);
			Assert(mWritable);

			if (!n)
				return 0;

			Assert(ftell(mpFile) == mOfs);
			
			const int endOfs = mOfs + n - 1;
			
			mSize = Math::Max(mSize, endOfs + 1);
						
			const int ret = static_cast<int>(fwrite(p, 1, n, mpFile));
			
			mOfs += ret;
			mLeft = Math::Max(0, mLeft - ret);

			return ret;
		}
		
		virtual int curOfs(void) const
		{
			Assert(mpFile);
			Assert(ftell(mpFile) == mOfs);
			return mOfs - mStartOfs;
		}

		virtual int bytesLeft(void) const
		{
			Assert(mpFile);

			return mLeft;
		}
				
		virtual void seek(int ofs, bool absolute = true)
		{
			Assert(mpFile);

			int aOfs = ofs;

			if (!absolute)
				aOfs += curOfs();
			
			Assert(aOfs >= 0);
			Assert(aOfs < size());

			mOfs = mStartOfs + aOfs;
			fseek(mpFile, mOfs, SEEK_SET);
			
			mLeft = mSize - aOfs;
		}

		virtual bool readable(void) const { return mReadable; }
		virtual bool writable(void) const { return mWritable; }
		virtual bool seekable(void) const {	return true; }
				
		virtual bool errorStatus(void) const
		{
			return (!mpFile) || (ferror(mpFile) != 0);
		}

		FILE* getFILE(void) const
		{
			return mpFile;
		}

		void close(void)
		{
			if (mpFile)
			{
				fclose(mpFile);
				mpFile = NULL;
			}
		}

		void open(FILE* pFile, bool readable = true, bool writable = false, bool autoClose = false, int size = -1, const std::string& name = "?") 
		{
			close();

			setName(name);
			mpFile = DebugNull(pFile);
			mAutoClose = autoClose;
			mSize = size;
			mLeft = size;
			mReadable = readable;
			mWritable = writable;
			mStartOfs = mOfs = ftell(mpFile);
						
			if (size == -1)
			{
				fseek(mpFile, 0, SEEK_END);
				mSize = mLeft = ftell(mpFile) - mStartOfs;
				fseek(mpFile, mStartOfs, SEEK_SET);
			}
		}

		// true on failure
		bool open(const char* pFilename, bool forReading = true, bool errorOnFailure = true)
		{
			close();

			setName(pFilename);
			mpFile = fopen(pFilename, forReading ? "rb" : "wb");
			mAutoClose = true;
			mReadable = forReading;
			mWritable = !forReading;	

			if ((errorOnFailure) && (!mpFile))
				Error("Unable to open file \"%s\" for %s!\n", pFilename, forReading ? "reading" : "writing");

			mStartOfs = mSize = mLeft = mOfs = 0;

			if (mpFile)
			{
				fseek(mpFile, 0, SEEK_END);
				mSize = mLeft = ftell(mpFile) - mStartOfs;
				fseek(mpFile, mStartOfs, SEEK_SET);
			}

			return NULL == mpFile;
		}

		bool isOpened(void) const
		{
			return NULL != mpFile;
		}
	};

} // namespace gr

#endif // STREAM_H
