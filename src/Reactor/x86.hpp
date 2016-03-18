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

#include "Nucleus.hpp"

namespace sw
{
	namespace x86
	{
		RValue<Int> cvtss2si(RValue<Float> val);
		RValue<Int2> cvtps2pi(RValue<Float4> val);
		RValue<Int2> cvttps2pi(RValue<Float4> val);
		RValue<Int4> cvtps2dq(RValue<Float4> val);
		
		RValue<Float> rcpss(RValue<Float> val);
		RValue<Float> sqrtss(RValue<Float> val);
		RValue<Float> rsqrtss(RValue<Float> val);

		RValue<Float4> rcpps(RValue<Float4> val);
		RValue<Float4> sqrtps(RValue<Float4> val);
		RValue<Float4> rsqrtps(RValue<Float4> val);
		RValue<Float4> maxps(RValue<Float4> x, RValue<Float4> y);
		RValue<Float4> minps(RValue<Float4> x, RValue<Float4> y);

		RValue<Float> roundss(RValue<Float> val, unsigned char imm);
		RValue<Float> floorss(RValue<Float> val);
		RValue<Float> ceilss(RValue<Float> val);

		RValue<Float4> roundps(RValue<Float4> val, unsigned char imm);
		RValue<Float4> floorps(RValue<Float4> val);
		RValue<Float4> ceilps(RValue<Float4> val);

		RValue<Float4> cmpps(RValue<Float4> x, RValue<Float4> y, unsigned char imm);
		RValue<Float4> cmpeqps(RValue<Float4> x, RValue<Float4> y);
		RValue<Float4> cmpltps(RValue<Float4> x, RValue<Float4> y);
		RValue<Float4> cmpleps(RValue<Float4> x, RValue<Float4> y);
		RValue<Float4> cmpunordps(RValue<Float4> x, RValue<Float4> y);
		RValue<Float4> cmpneqps(RValue<Float4> x, RValue<Float4> y);
		RValue<Float4> cmpnltps(RValue<Float4> x, RValue<Float4> y);
		RValue<Float4> cmpnleps(RValue<Float4> x, RValue<Float4> y);
		RValue<Float4> cmpordps(RValue<Float4> x, RValue<Float4> y);

		RValue<Float> cmpss(RValue<Float> x, RValue<Float> y, unsigned char imm);
		RValue<Float> cmpeqss(RValue<Float> x, RValue<Float> y);
		RValue<Float> cmpltss(RValue<Float> x, RValue<Float> y);
		RValue<Float> cmpless(RValue<Float> x, RValue<Float> y);
		RValue<Float> cmpunordss(RValue<Float> x, RValue<Float> y);
		RValue<Float> cmpneqss(RValue<Float> x, RValue<Float> y);
		RValue<Float> cmpnltss(RValue<Float> x, RValue<Float> y);
		RValue<Float> cmpnless(RValue<Float> x, RValue<Float> y);
		RValue<Float> cmpordss(RValue<Float> x, RValue<Float> y);

		RValue<Int4> pabsd(RValue<Int4> x);

		RValue<Short4> paddsw(RValue<Short4> x, RValue<Short4> y);
		RValue<Short4> psubsw(RValue<Short4> x, RValue<Short4> y);
		RValue<UShort4> paddusw(RValue<UShort4> x, RValue<UShort4> y);
		RValue<UShort4> psubusw(RValue<UShort4> x, RValue<UShort4> y);
		RValue<SByte8> paddsb(RValue<SByte8> x, RValue<SByte8> y);
		RValue<SByte8> psubsb(RValue<SByte8> x, RValue<SByte8> y);
		RValue<Byte8> paddusb(RValue<Byte8> x, RValue<Byte8> y);
		RValue<Byte8> psubusb(RValue<Byte8> x, RValue<Byte8> y);

		RValue<Short4> paddw(RValue<Short4> x, RValue<Short4> y);
		RValue<Short4> psubw(RValue<Short4> x, RValue<Short4> y);
		RValue<Short4> pmullw(RValue<Short4> x, RValue<Short4> y);
		RValue<Short4> pand(RValue<Short4> x, RValue<Short4> y);
		RValue<Short4> por(RValue<Short4> x, RValue<Short4> y);
		RValue<Short4> pxor(RValue<Short4> x, RValue<Short4> y);
		RValue<Short4> pshufw(RValue<Short4> x, unsigned char y);
		RValue<Int2> punpcklwd(RValue<Short4> x, RValue<Short4> y);
		RValue<Int2> punpckhwd(RValue<Short4> x, RValue<Short4> y);
		RValue<Short4> pinsrw(RValue<Short4> x, RValue<Int> y, unsigned int i);
		RValue<Int> pextrw(RValue<Short4> x, unsigned int i);
		RValue<Long1> punpckldq(RValue<Int2> x, RValue<Int2> y);
		RValue<Long1> punpckhdq(RValue<Int2> x, RValue<Int2> y);
		RValue<Short4> punpcklbw(RValue<Byte8> x, RValue<Byte8> y);
		RValue<Short4> punpckhbw(RValue<Byte8> x, RValue<Byte8> y);
		RValue<Byte8> paddb(RValue<Byte8> x, RValue<Byte8> y);
		RValue<Byte8> psubb(RValue<Byte8> x, RValue<Byte8> y);
		RValue<Int2> paddd(RValue<Int2> x, RValue<Int2> y);
		RValue<Int2> psubd(RValue<Int2> x, RValue<Int2> y);

		RValue<UShort4> pavgw(RValue<UShort4> x, RValue<UShort4> y);

		RValue<Short4> pmaxsw(RValue<Short4> x, RValue<Short4> y);
		RValue<Short4> pminsw(RValue<Short4> x, RValue<Short4> y);

		RValue<Short4> pcmpgtw(RValue<Short4> x, RValue<Short4> y);
		RValue<Short4> pcmpeqw(RValue<Short4> x, RValue<Short4> y);
		RValue<Byte8> pcmpgtb(RValue<SByte8> x, RValue<SByte8> y);
		RValue<Byte8> pcmpeqb(RValue<Byte8> x, RValue<Byte8> y);

		RValue<Short4> packssdw(RValue<Int2> x, RValue<Int2> y);
		RValue<Short8> packssdw(RValue<Int4> x, RValue<Int4> y);
		RValue<SByte8> packsswb(RValue<Short4> x, RValue<Short4> y);
		RValue<Byte8> packuswb(RValue<UShort4> x, RValue<UShort4> y);

		RValue<UShort8> packusdw(RValue<UInt4> x, RValue<UInt4> y);

		RValue<UShort4> psrlw(RValue<UShort4> x, unsigned char y);
		RValue<UShort8> psrlw(RValue<UShort8> x, unsigned char y);
		RValue<Short4> psraw(RValue<Short4> x, unsigned char y);
		RValue<Short8> psraw(RValue<Short8> x, unsigned char y);
		RValue<Short4> psllw(RValue<Short4> x, unsigned char y);
		RValue<Short8> psllw(RValue<Short8> x, unsigned char y);
		RValue<Int2> pslld(RValue<Int2> x, unsigned char y);
		RValue<Int4> pslld(RValue<Int4> x, unsigned char y);
		RValue<Int2> psrad(RValue<Int2> x, unsigned char y);
		RValue<Int4> psrad(RValue<Int4> x, unsigned char y);
		RValue<UInt2> psrld(RValue<UInt2> x, unsigned char y);
		RValue<UInt4> psrld(RValue<UInt4> x, unsigned char y);

		RValue<UShort4> psrlw(RValue<UShort4> x, RValue<Long1> y);
		RValue<Short4> psraw(RValue<Short4> x, RValue<Long1> y);
		RValue<Short4> psllw(RValue<Short4> x, RValue<Long1> y);
		RValue<Int2> pslld(RValue<Int2> x, RValue<Long1> y);
		RValue<UInt2> psrld(RValue<UInt2> x, RValue<Long1> y);
		RValue<Int2> psrad(RValue<Int2> x, RValue<Long1> y);

		RValue<Int4> pmaxsd(RValue<Int4> x, RValue<Int4> y);
		RValue<Int4> pminsd(RValue<Int4> x, RValue<Int4> y);
		RValue<UInt4> pmaxud(RValue<UInt4> x, RValue<UInt4> y);
		RValue<UInt4> pminud(RValue<UInt4> x, RValue<UInt4> y);

		RValue<Short4> pmulhw(RValue<Short4> x, RValue<Short4> y);
		RValue<UShort4> pmulhuw(RValue<UShort4> x, RValue<UShort4> y);
		RValue<Int2> pmaddwd(RValue<Short4> x, RValue<Short4> y);

		RValue<Short8> pmulhw(RValue<Short8> x, RValue<Short8> y);
		RValue<UShort8> pmulhuw(RValue<UShort8> x, RValue<UShort8> y);
		RValue<Int4> pmaddwd(RValue<Short8> x, RValue<Short8> y);

		RValue<Int> movmskps(RValue<Float4> x);
		RValue<Int> pmovmskb(RValue<Byte8> x);

		RValue<Int4> pmovzxbd(RValue<Int4> x);
		RValue<Int4> pmovsxbd(RValue<Int4> x);
		RValue<Int4> pmovzxwd(RValue<Int4> x);
		RValue<Int4> pmovsxwd(RValue<Int4> x);

		void emms();
	}
}
