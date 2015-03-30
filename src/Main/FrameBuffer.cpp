// SwiftShader Software Renderer
//
// Copyright(c) 2005-2012 TransGaming Inc.
//
// All rights reserved. No part of this software may be copied, distributed, transmitted,
// transcribed, stored in a retrieval system, translated into any human or computer
// language by any means, or disclosed to third parties without the explicit written
// agreement of TransGaming Inc. Without such an agreement, no rights or licenses, express
// or implied, including but not limited to any patent rights, are granted to you.
//

#include "FrameBuffer.hpp"

#include "Timer.hpp"
#include "CPUID.hpp"
#include "serialvalid.h"
#include "Register.hpp"
#include "Renderer/Surface.hpp"
#include "Reactor/Reactor.hpp"
#include "Common/Debug.hpp"

#include <stdio.h>
#include <string.h>
#include <time.h>

#ifndef DISPLAY_LOGO
#define DISPLAY_LOGO (NDEBUG & 1)
#endif

#define ASYNCHRONOUS_BLIT 0   // FIXME: Currently leads to rare race conditions

extern const int logoWidth;
extern const int logoHeight;
extern const unsigned int logoData[];

namespace sw
{
	extern bool forceWindowed;

	Surface *FrameBuffer::logo;
	unsigned int *FrameBuffer::logoImage;
	void *FrameBuffer::cursor;
	int FrameBuffer::cursorWidth = 0;
	int FrameBuffer::cursorHeight = 0;
	int FrameBuffer::cursorHotspotX;
	int FrameBuffer::cursorHotspotY;
	int FrameBuffer::cursorPositionX;
	int FrameBuffer::cursorPositionY;
	int FrameBuffer::cursorX;
	int FrameBuffer::cursorY;
	bool FrameBuffer::topLeftOrigin = false;

	FrameBuffer::FrameBuffer(int width, int height, bool fullscreen, bool topLeftOrigin)
	{
		this->topLeftOrigin = topLeftOrigin;

		locked = 0;

		this->width = width;
		this->height = height;
		destFormat = FORMAT_X8R8G8B8;
		sourceFormat = FORMAT_X8R8G8B8;
		stride = 0;

		if(forceWindowed)
		{
			fullscreen = false;
		}

		windowed = !fullscreen;

		blitFunction = 0;
		blitRoutine = 0;

		blitState.width = 0;
		blitState.height = 0;
		blitState.destFormat = FORMAT_X8R8G8B8;
		blitState.sourceFormat = FORMAT_X8R8G8B8;
		blitState.cursorWidth = 0;
		blitState.cursorHeight = 0;

		logo = 0;

		if(ASYNCHRONOUS_BLIT)
		{
			terminate = false;
			FrameBuffer *parameters = this;
			blitThread = new Thread(threadFunction, &parameters);
		}
	}

	FrameBuffer::~FrameBuffer()
	{
		if(ASYNCHRONOUS_BLIT)
		{
			terminate = true;
			blitEvent.signal();
			blitThread->join();
			delete blitThread;
		}

		delete blitRoutine;
	}

	int FrameBuffer::getWidth() const
	{
		return width;
	}

	int FrameBuffer::getHeight() const
	{
		return height;
	}

	int FrameBuffer::getStride() const
	{
		return stride;
	}

	void FrameBuffer::setCursorImage(sw::Surface *cursorImage)
	{
		if(cursorImage)
		{
			cursor = cursorImage->lockExternal(0, 0, 0, sw::LOCK_READONLY, sw::PUBLIC);
			cursorImage->unlockExternal();

			cursorWidth = cursorImage->getExternalWidth();
			cursorHeight = cursorImage->getExternalHeight();
		}
		else
		{
			cursorWidth = 0;
			cursorHeight = 0;
		}
	}

	void FrameBuffer::setCursorOrigin(int x0, int y0)
	{
		cursorHotspotX = x0;
		cursorHotspotY = y0;
	}

	void FrameBuffer::setCursorPosition(int x, int y)
	{
		cursorPositionX = x;
		cursorPositionY = y;
	}

	void FrameBuffer::copy(void *source, Format format)
	{
		if(!source)
		{
			return;
		}

		if(!lock())
		{
			return;
		}

		if(topLeftOrigin)
		{
			target = source;
		}
		else
		{
			const int width2 = (width + 1) & ~1;
			const int sBytes = Surface::bytes(sourceFormat);
			const int sStride = sBytes * width2;

			target = (byte*)source + (height - 1) * sStride;
		}

		sourceFormat = format;

		cursorX = cursorPositionX - cursorHotspotX;
		cursorY = cursorPositionY - cursorHotspotY;

		if(ASYNCHRONOUS_BLIT)
		{
			blitEvent.signal();
			syncEvent.wait();
		}
		else
		{
			copyLocked();
		}

		unlock();

		profiler.nextFrame();   // Assumes every copy() is a full frame
	}

	void FrameBuffer::copyLocked()
	{
		BlitState update = {0};

		update.width = width;
		update.height = height;
		update.destFormat = destFormat;
		update.sourceFormat = sourceFormat;
		update.stride = stride;
		update.cursorWidth = cursorWidth;
		update.cursorHeight = cursorHeight;

		if(memcmp(&blitState, &update, sizeof(BlitState)) != 0)
		{
			blitState = update;
			delete blitRoutine;

			blitRoutine = copyRoutine(blitState);
			blitFunction = (void(*)(void*, void*))blitRoutine->getEntry();
		}

		blitFunction(locked, target);
	}

	Routine *FrameBuffer::copyRoutine(const BlitState &state)
	{
		initializeLogo();

		const int width = state.width;
		const int height = state.height;
		const int width2 = (state.width + 1) & ~1;
		const int dBytes = Surface::bytes(state.destFormat);
		const int dStride = state.stride;
		const int sBytes = Surface::bytes(state.sourceFormat);
		const int sStride = topLeftOrigin ? (sBytes * width2) : -(sBytes * width2);

	//	char compareApp[32] = SCRAMBLE31(validationApp, APPNAME_SCRAMBLE);
	//	bool validApp = strcmp(compareApp, registeredApp) == 0;
		bool validKey = ValidateSerialNumber(validationKey, CHECKSUM_KEY, SERIAL_PREFIX);

		// Date of the end of the logo-free license
		const int endYear = 2099;
		const int endMonth = 12;
		const int endDay = 31;

		const int endDate = (endYear << 16) + (endMonth << 8) + endDay;

		time_t rawtime = time(0);
		tm *timeinfo = localtime(&rawtime);
		int year = timeinfo->tm_year + 1900;
		int month = timeinfo->tm_mon + 1;
		int day = timeinfo->tm_mday;

		int date = (year << 16) + (month << 8) + day;

		if(date > endDate)
		{
			validKey = false;
		}

		Function<Void, Pointer<Byte>, Pointer<Byte> > function;
		{
			Pointer<Byte> dst(function.arg(0));
			Pointer<Byte> src(function.arg(1));

			For(Int y = 0, y < height, y++)
			{
				Pointer<Byte> d = dst + y * dStride;
				Pointer<Byte> s = src + y * sStride;

				Int x0 = 0;

				#if DISPLAY_LOGO
					If(!Bool(validKey)/* || !Bool(validApp)*/)
					{
						If(y > height - logoHeight)
						{
							x0 = logoWidth;
							s += logoWidth * sBytes;
							d += logoWidth * dBytes;
						}
					}
				#endif

				if(state.destFormat == FORMAT_X8R8G8B8 || state.destFormat == FORMAT_A8R8G8B8)
				{
					Int x = x0;

					if(state.sourceFormat == FORMAT_X8R8G8B8 || state.sourceFormat == FORMAT_A8R8G8B8)
					{
						For(, x < width - 3, x += 4)
						{
							*Pointer<Int4>(d, 1) = *Pointer<Int4>(s, width % 4 ? 1 : 16);

							s += 4 * sBytes;
							d += 4 * dBytes;
						}
					}
					else if(state.sourceFormat == FORMAT_X8B8G8R8 || state.sourceFormat == FORMAT_A8B8G8R8)
					{
						For(, x < width - 3, x += 4)
						{
							Int4 bgra = *Pointer<Int4>(s, width % 4 ? 1 : 16);

							*Pointer<Int4>(d, 1) = ((bgra & Int4(0x00FF0000)) >> 16) |
							                       ((bgra & Int4(0x000000FF)) << 16) |
							                       (bgra & Int4(0xFF00FF00));

							s += 4 * sBytes;
							d += 4 * dBytes;
						}
					}
					else if(state.sourceFormat == FORMAT_A16B16G16R16)
					{
						For(, x < width - 1, x += 2)
						{
							UShort4 c0 = As<UShort4>(Swizzle(*Pointer<Short4>(s + 0), 0xC6)) >> 8;
							UShort4 c1 = As<UShort4>(Swizzle(*Pointer<Short4>(s + 8), 0xC6)) >> 8;

							*Pointer<Int2>(d) = As<Int2>(Pack(c0, c1));

							s += 2 * sBytes;
							d += 2 * dBytes;
						}
					}
					else ASSERT(false);

					For(, x < width, x++)
					{
						if(state.sourceFormat == FORMAT_X8R8G8B8 || state.sourceFormat == FORMAT_A8R8G8B8)
						{
							*Pointer<Int>(d) = *Pointer<Int>(s);
						}
						else if(state.sourceFormat == FORMAT_X8B8G8R8 || state.sourceFormat == FORMAT_A8B8G8R8)
						{
							Int rgba = *Pointer<Int>(s);

							*Pointer<Int>(d) = ((rgba & Int(0x00FF0000)) >> 16) |
							                   ((rgba & Int(0x000000FF)) << 16) |
							                   (rgba & Int(0xFF00FF00));
						}
						else if(state.sourceFormat == FORMAT_A16B16G16R16)
						{
							UShort4 c = As<UShort4>(Swizzle(*Pointer<Short4>(s), 0xC6)) >> 8;

							*Pointer<Int>(d) = Int(As<Int2>(Pack(c, c)));
						}
						else ASSERT(false);

						s += sBytes;
						d += dBytes;
					}
				}
				else if(state.destFormat == FORMAT_X8B8G8R8 || state.destFormat == FORMAT_A8B8G8R8)
				{
					Int x = x0;

					if(state.sourceFormat == FORMAT_X8B8G8R8 || state.sourceFormat == FORMAT_A8B8G8R8)
					{
						For(, x < width - 3, x += 4)
						{
							*Pointer<Int4>(d, 1) = *Pointer<Int4>(s, width % 4 ? 1 : 16);

							s += 4 * sBytes;
							d += 4 * dBytes;
						}
					}
					else if(state.sourceFormat == FORMAT_X8R8G8B8 || state.sourceFormat == FORMAT_A8R8G8B8)
					{
						For(, x < width - 3, x += 4)
						{
							Int4 bgra = *Pointer<Int4>(s, width % 4 ? 1 : 16);

							*Pointer<Int4>(d, 1) = ((bgra & Int4(0x00FF0000)) >> 16) |
							                       ((bgra & Int4(0x000000FF)) << 16) |
							                       (bgra & Int4(0xFF00FF00));

							s += 4 * sBytes;
							d += 4 * dBytes;
						}
					}
					else if(state.sourceFormat == FORMAT_A16B16G16R16)
					{
						For(, x < width - 1, x += 2)
						{
							UShort4 c0 = *Pointer<UShort4>(s + 0) >> 8;
							UShort4 c1 = *Pointer<UShort4>(s + 8) >> 8;

							*Pointer<Int2>(d) = As<Int2>(Pack(c0, c1));

							s += 2 * sBytes;
							d += 2 * dBytes;
						}
					}
					else ASSERT(false);

					For(, x < width, x++)
					{
						if(state.sourceFormat == FORMAT_X8B8G8R8 || state.sourceFormat == FORMAT_A8B8G8R8)
						{
							*Pointer<Int>(d) = *Pointer<Int>(s);
						}
						else if(state.sourceFormat == FORMAT_X8R8G8B8 || state.sourceFormat == FORMAT_A8R8G8B8)
						{
							Int bgra = *Pointer<Int>(s);
							*Pointer<Int>(d) = ((bgra & Int(0x00FF0000)) >> 16) |
							                   ((bgra & Int(0x000000FF)) << 16) |
							                   (bgra & Int(0xFF00FF00));
						}
						else if(state.sourceFormat == FORMAT_A16B16G16R16)
						{
							UShort4 c = *Pointer<UShort4>(s) >> 8;

							*Pointer<Int>(d) = Int(As<Int2>(Pack(c, c)));
						}
						else ASSERT(false);

						s += sBytes;
						d += dBytes;
					}
				}
				else if(state.destFormat == FORMAT_R8G8B8)
				{
					For(Int x = x0, x < width, x++)
					{
						if(state.sourceFormat == FORMAT_X8R8G8B8 || state.sourceFormat == FORMAT_A8R8G8B8)
						{
							*Pointer<Byte>(d + 0) = *Pointer<Byte>(s + 0);
							*Pointer<Byte>(d + 1) = *Pointer<Byte>(s + 1);
							*Pointer<Byte>(d + 2) = *Pointer<Byte>(s + 2);
						}
						else if(state.sourceFormat == FORMAT_X8B8G8R8 || state.sourceFormat == FORMAT_A8B8G8R8)
						{
							*Pointer<Byte>(d + 0) = *Pointer<Byte>(s + 2);
							*Pointer<Byte>(d + 1) = *Pointer<Byte>(s + 1);
							*Pointer<Byte>(d + 2) = *Pointer<Byte>(s + 0);
						}
						else if(state.sourceFormat == FORMAT_A16B16G16R16)
						{
							*Pointer<Byte>(d + 0) = *Pointer<Byte>(s + 5);
							*Pointer<Byte>(d + 1) = *Pointer<Byte>(s + 3);
							*Pointer<Byte>(d + 2) = *Pointer<Byte>(s + 1);
						}
						else ASSERT(false);

						s += sBytes;
						d += dBytes;
					}
				}
				else if(state.destFormat == FORMAT_R5G6B5)
				{
					For(Int x = x0, x < width, x++)
					{
						if(state.sourceFormat == FORMAT_X8R8G8B8 || state.sourceFormat == FORMAT_A8R8G8B8)
						{
							Int c = *Pointer<Int>(s);

							*Pointer<Short>(d) = Short((c & 0x00F80000) >> 8 |
						                               (c & 0x0000FC00) >> 5 |
						                               (c & 0x000000F8) >> 3);
						}
						else if(state.sourceFormat == FORMAT_X8B8G8R8 || state.sourceFormat == FORMAT_A8B8G8R8)
						{
							Int c = *Pointer<Int>(s);

							*Pointer<Short>(d) = Short((c & 0x00F80000) >> 19 |
						                               (c & 0x0000FC00) >> 5 |
						                               (c & 0x000000F8) << 8);
						}
						else if(state.sourceFormat == FORMAT_A16B16G16R16)
						{
							UShort4 cc = *Pointer<UShort4>(s) >> 8;
							Int c = Int(As<Int2>(Pack(cc, cc)));

							*Pointer<Short>(d) = Short((c & 0x00F80000) >> 19 |
						                               (c & 0x0000FC00) >> 5 |
						                               (c & 0x000000F8) << 8);
						}
						else ASSERT(false);

						s += sBytes;
						d += dBytes;
					}
				}
				else ASSERT(false);
			}

			#if DISPLAY_LOGO
				If(!Bool(validKey)/* || !Bool(validApp)*/)
				{
					UInt hash = UInt(0x0B020C04) + UInt(0xC0F090E0);   // Initial value
					UInt imageHash = S3TC_SUPPORT ? UInt(0x0F0D0700) + UInt(0xA0C0A090) : UInt(0x0207040B) + UInt(0xD0406010);

					While(hash != imageHash)
					{
						For(y = (height - 1), height - 1 - y < logoHeight, y--)
						{
							Pointer<Byte> logo = *Pointer<Pointer<Byte> >(&logoImage) + 4 * (logoHeight - height + y) * logoWidth;
							Pointer<Byte> s = src + y * sStride;
							Pointer<Byte> d = dst + y * dStride;

							For(Int x = 0, x < logoWidth, x++)
							{
								hash *= 16777619;
								hash ^= *Pointer<UInt>(logo);

								If(y >= 0 && x < width)
								{
									blend(state, d, s, logo);
								}

								logo += 4;
								s += sBytes;
								d += dBytes;
							}
						}
					}
				}
			#endif

			Int x0 = *Pointer<Int>(&cursorX);
			Int y0 = *Pointer<Int>(&cursorY);

			For(Int y1 = 0, y1 < cursorHeight, y1++)
			{
				Int y = y0 + y1;

				If(y >= 0 && y < height)
				{
					Pointer<Byte> d = dst + y * dStride + x0 * dBytes;
					Pointer<Byte> s = src + y * sStride + x0 * sBytes;
					Pointer<Byte> c = *Pointer<Pointer<Byte> >(&cursor) + y1 * cursorWidth * 4;

					For(Int x1 = 0, x1 < cursorWidth, x1++)
					{
						Int x = x0 + x1;

						If(x >= 0 && x < width)
						{
							blend(state, d, s, c);
						}

						c += 4;
						s += sBytes;
						d += dBytes;
					}
				}
			}
		}

		return function(L"FrameBuffer");
	}

	void FrameBuffer::blend(const BlitState &state, const Pointer<Byte> &d, const Pointer<Byte> &s, const Pointer<Byte> &c)
	{
		Short4 c1;
		Short4 c2;

		c1 = UnpackLow(As<Byte8>(c1), *Pointer<Byte8>(c));

		if(state.sourceFormat == FORMAT_X8R8G8B8 || state.sourceFormat == FORMAT_A8R8G8B8)
		{
			c2 = UnpackLow(As<Byte8>(c2), *Pointer<Byte8>(s));
		}
		else if(state.sourceFormat == FORMAT_X8B8G8R8 || state.sourceFormat == FORMAT_A8B8G8R8)
		{
			c2 = Swizzle(UnpackLow(As<Byte8>(c2), *Pointer<Byte8>(s)), 0xC6);
		}
		else if(state.sourceFormat == FORMAT_A16B16G16R16)
		{
			c2 = Swizzle(*Pointer<Short4>(s + 0), 0xC6);
		}
		else ASSERT(false);

		c1 = As<Short4>(As<UShort4>(c1) >> 9);
		c2 = As<Short4>(As<UShort4>(c2) >> 9);

		Short4 alpha = Swizzle(c1, 0xFF) & Short4(0xFFFFu, 0xFFFFu, 0xFFFFu, 0x0000);

		c1 = (c1 - c2) * alpha;
		c1 = c1 >> 7;
		c1 = c1 + c2;
		c1 = c1 + c1;

		if(state.destFormat == FORMAT_X8R8G8B8 || state.destFormat == FORMAT_A8R8G8B8)
		{
			*Pointer<UInt>(d) = UInt(As<Long>(Pack(As<UShort4>(c1), As<UShort4>(c1))));
		}
		else if(state.destFormat == FORMAT_X8B8G8R8 || state.destFormat == FORMAT_A8B8G8R8)
		{
			c1 = Swizzle(c1, 0xC6);

			*Pointer<UInt>(d) = UInt(As<Long>(Pack(As<UShort4>(c1), As<UShort4>(c1))));
		}
		else if(state.destFormat == FORMAT_R8G8B8)
		{
			Int c = Int(As<Int2>(Pack(As<UShort4>(c1), As<UShort4>(c1))));

			*Pointer<Byte>(d + 0) = Byte(c >> 0);
			*Pointer<Byte>(d + 1) = Byte(c >> 8);
			*Pointer<Byte>(d + 2) = Byte(c >> 16);
		}
		else if(state.destFormat == FORMAT_R5G6B5)
		{
			Int c = Int(As<Int2>(Pack(As<UShort4>(c1), As<UShort4>(c1))));

			*Pointer<Short>(d) = Short((c & 0x00F80000) >> 8 |
			                           (c & 0x0000FC00) >> 5 |
			                           (c & 0x000000F8) >> 3);
		}
		else ASSERT(false);
	}

	void FrameBuffer::threadFunction(void *parameters)
	{
		FrameBuffer *frameBuffer = *static_cast<FrameBuffer**>(parameters);

		while(!frameBuffer->terminate)
		{
			frameBuffer->blitEvent.wait();

			if(!frameBuffer->terminate)
			{
				frameBuffer->copyLocked();

				frameBuffer->syncEvent.signal();
			}
		}
	}

	void FrameBuffer::initializeLogo()
	{
		#if DISPLAY_LOGO
			if(!logo)
			{
				#if S3TC_SUPPORT
					logo = new Surface(0, logoWidth, logoHeight, 1, FORMAT_DXT5, true, false);
					void *data = logo->lockExternal(0, 0, 0, LOCK_WRITEONLY, sw::PUBLIC);
					memcpy(data, logoData, logoWidth * logoHeight);
					logo->unlockExternal();
				#else
					logo = new Surface(0, logoWidth, logoHeight, 1, FORMAT_A8R8G8B8, true, false);
					void *data = logo->lockExternal(0, 0, 0, LOCK_WRITEONLY, sw::PUBLIC);
					memcpy(data, logoData, logoWidth * logoHeight * 4);
					logo->unlockExternal();
				#endif

				logoImage = (unsigned int*)logo->lockInternal(0, 0, 0, LOCK_READONLY, sw::PUBLIC);
				logo->unlockInternal();
			}
		#endif
	}
}
