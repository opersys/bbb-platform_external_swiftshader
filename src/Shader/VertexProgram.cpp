// SwiftShader Software Renderer
//
// Copyright(c) 2005-2013 TransGaming Inc.
//
// All rights reserved. No part of this software may be copied, distributed, transmitted,
// transcribed, stored in a retrieval system, translated into any human or computer
// language by any means, or disclosed to third parties without the explicit written
// agreement of TransGaming Inc. Without such an agreement, no rights or licenses, express
// or implied, including but not limited to any patent rights, are granted to you.
//

#include "VertexProgram.hpp"

#include "Renderer.hpp"
#include "VertexShader.hpp"
#include "Vertex.hpp"
#include "Half.hpp"
#include "SamplerCore.hpp"
#include "Debug.hpp"

namespace sw
{
	VertexProgram::VertexProgram(const VertexProcessor::State &state, const VertexShader *shader) : VertexRoutine(state, shader)
	{
		ifDepth = 0;
		loopRepDepth = 0;
		breakDepth = 0;
		currentLabel = -1;
		whileTest = false;

		for(int i = 0; i < 2048; i++)
		{
			labelBlock[i] = 0;
		}
	}

	VertexProgram::~VertexProgram()
	{
		for(int i = 0; i < VERTEX_TEXTURE_IMAGE_UNITS; i++)
		{
			delete sampler[i];
		}
	}

	void VertexProgram::pipeline(Registers &r)
	{
		for(int i = 0; i < VERTEX_TEXTURE_IMAGE_UNITS; i++)
		{
			sampler[i] = new SamplerCore(r.constants, state.samplerState[i]);
		}

		if(!state.preTransformed)
		{
			program(r);
		}
		else
		{
			passThrough(r);
		}
	}

	void VertexProgram::program(Registers &r)
	{
	//	shader->print("VertexShader-%0.8X.txt", state.shaderID);

		unsigned short version = shader->getVersion();

		r.enableIndex = 0;
		r.stackIndex = 0;

		if(shader->containsLeaveInstruction())
		{
			r.enableLeave = Int4(0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF);
		}

		// Create all call site return blocks up front
		for(size_t i = 0; i < shader->getLength(); i++)
		{
			const Shader::Instruction *instruction = shader->getInstruction(i);
			Shader::Opcode opcode = instruction->opcode;

			if(opcode == Shader::OPCODE_CALL || opcode == Shader::OPCODE_CALLNZ)
			{
				const Dst &dst = instruction->dst;

				ASSERT(callRetBlock[dst.label].size() == dst.callSite);
				callRetBlock[dst.label].push_back(Nucleus::createBasicBlock());
			}
		}
	
		for(size_t i = 0; i < shader->getLength(); i++)
		{
			const Shader::Instruction *instruction = shader->getInstruction(i);
			Shader::Opcode opcode = instruction->opcode;

			if(opcode == Shader::OPCODE_DCL || opcode == Shader::OPCODE_DEF || opcode == Shader::OPCODE_DEFI || opcode == Shader::OPCODE_DEFB)
			{
				continue;
			}

			Dst dst = instruction->dst;
			Src src0 = instruction->src[0];
			Src src1 = instruction->src[1];
			Src src2 = instruction->src[2];
			Src src3 = instruction->src[3];
			Src src4 = instruction->src[4];

			bool predicate = instruction->predicate;
			Control control = instruction->control;
			bool integer = dst.type == Shader::PARAMETER_ADDR;
			bool pp = dst.partialPrecision;

			Vector4f d;
			Vector4f s0;
			Vector4f s1;
			Vector4f s2;
			Vector4f s3;
			Vector4f s4;

			if(src0.type != Shader::PARAMETER_VOID) s0 = fetchRegisterF(r, src0);
			if(src1.type != Shader::PARAMETER_VOID) s1 = fetchRegisterF(r, src1);
			if(src2.type != Shader::PARAMETER_VOID) s2 = fetchRegisterF(r, src2);
			if(src3.type != Shader::PARAMETER_VOID) s3 = fetchRegisterF(r, src3);
			if(src4.type != Shader::PARAMETER_VOID) s4 = fetchRegisterF(r, src4);

			switch(opcode)
			{
			case Shader::OPCODE_VS_1_0:										break;
			case Shader::OPCODE_VS_1_1:										break;
			case Shader::OPCODE_VS_2_0:										break;
			case Shader::OPCODE_VS_2_x:										break;
			case Shader::OPCODE_VS_2_sw:									break;
			case Shader::OPCODE_VS_3_0:										break;
			case Shader::OPCODE_VS_3_sw:									break;
			case Shader::OPCODE_DCL:										break;
			case Shader::OPCODE_DEF:										break;
			case Shader::OPCODE_DEFI:										break;
			case Shader::OPCODE_DEFB:										break;
			case Shader::OPCODE_NOP:										break;
			case Shader::OPCODE_ABS:		abs(d, s0);						break;
			case Shader::OPCODE_ADD:		add(d, s0, s1);					break;
			case Shader::OPCODE_IADD:       iadd(d, s0, s1);                break;
			case Shader::OPCODE_CRS:		crs(d, s0, s1);					break;
			case Shader::OPCODE_FORWARD1:	forward1(d, s0, s1, s2);		break;
			case Shader::OPCODE_FORWARD2:	forward2(d, s0, s1, s2);		break;
			case Shader::OPCODE_FORWARD3:	forward3(d, s0, s1, s2);		break;
			case Shader::OPCODE_FORWARD4:	forward4(d, s0, s1, s2);		break;
			case Shader::OPCODE_REFLECT1:	reflect1(d, s0, s1);			break;
			case Shader::OPCODE_REFLECT2:	reflect2(d, s0, s1);			break;
			case Shader::OPCODE_REFLECT3:	reflect3(d, s0, s1);			break;
			case Shader::OPCODE_REFLECT4:	reflect4(d, s0, s1);			break;
			case Shader::OPCODE_REFRACT1:	refract1(d, s0, s1, s2.x);		break;
			case Shader::OPCODE_REFRACT2:	refract2(d, s0, s1, s2.x);		break;
			case Shader::OPCODE_REFRACT3:	refract3(d, s0, s1, s2.x);		break;
			case Shader::OPCODE_REFRACT4:	refract4(d, s0, s1, s2.x);		break;
			case Shader::OPCODE_DP1:		dp1(d, s0, s1);					break;
			case Shader::OPCODE_DP2:		dp2(d, s0, s1);					break;
			case Shader::OPCODE_DP3:		dp3(d, s0, s1);					break;
			case Shader::OPCODE_DP4:		dp4(d, s0, s1);					break;
			case Shader::OPCODE_DET2:       det2(d, s0, s1);                break;
			case Shader::OPCODE_DET3:       det3(d, s0, s1, s2);            break;
			case Shader::OPCODE_DET4:       det4(d, s0, s1, s2, s3);        break;
			case Shader::OPCODE_ATT:		att(d, s0, s1);					break;
			case Shader::OPCODE_EXP2X:		exp2x(d, s0, pp);				break;
			case Shader::OPCODE_EXP2:		exp2(d, s0, pp);				break;
			case Shader::OPCODE_EXPP:		expp(d, s0, version);			break;
			case Shader::OPCODE_EXP:		exp(d, s0, pp);					break;
			case Shader::OPCODE_FRC:		frc(d, s0);						break;
			case Shader::OPCODE_TRUNC:      trunc(d, s0);                   break;
			case Shader::OPCODE_FLOOR:      floor(d, s0);                   break;
			case Shader::OPCODE_ROUND:      round(d, s0);                   break;
			case Shader::OPCODE_ROUNDEVEN:	roundEven(d, s0);               break;
			case Shader::OPCODE_CEIL:       ceil(d, s0);                    break;
			case Shader::OPCODE_LIT:		lit(d, s0);						break;
			case Shader::OPCODE_LOG2X:		log2x(d, s0, pp);				break;
			case Shader::OPCODE_LOG2:		log2(d, s0, pp);				break;
			case Shader::OPCODE_LOGP:		logp(d, s0, version);			break;
			case Shader::OPCODE_LOG:		log(d, s0, pp);					break;
			case Shader::OPCODE_LRP:		lrp(d, s0, s1, s2);				break;
			case Shader::OPCODE_STEP:		step(d, s0, s1);				break;
			case Shader::OPCODE_SMOOTH:		smooth(d, s0, s1, s2);			break;
			case Shader::OPCODE_FLOATBITSTOINT:
			case Shader::OPCODE_FLOATBITSTOUINT:
			case Shader::OPCODE_INTBITSTOFLOAT:
			case Shader::OPCODE_UINTBITSTOFLOAT: d = s0;                    break;
			case Shader::OPCODE_M3X2:		M3X2(r, d, s0, src1);			break;
			case Shader::OPCODE_M3X3:		M3X3(r, d, s0, src1);			break;
			case Shader::OPCODE_M3X4:		M3X4(r, d, s0, src1);			break;
			case Shader::OPCODE_M4X3:		M4X3(r, d, s0, src1);			break;
			case Shader::OPCODE_M4X4:		M4X4(r, d, s0, src1);			break;
			case Shader::OPCODE_MAD:		mad(d, s0, s1, s2);				break;
			case Shader::OPCODE_IMAD:		imad(d, s0, s1, s2);			break;
			case Shader::OPCODE_MAX:		max(d, s0, s1);					break;
			case Shader::OPCODE_IMAX:       imax(d, s0, s1);                break;
			case Shader::OPCODE_UMAX:       umax(d, s0, s1);                break;
			case Shader::OPCODE_MIN:		min(d, s0, s1);					break;
			case Shader::OPCODE_IMIN:       imin(d, s0, s1);                break;
			case Shader::OPCODE_UMIN:       umin(d, s0, s1);                break;
			case Shader::OPCODE_MOV:		mov(d, s0, integer);			break;
			case Shader::OPCODE_MOVA:       mov(d, s0, true);               break;
			case Shader::OPCODE_NEG:        neg(d, s0);                     break;
			case Shader::OPCODE_INEG:       ineg(d, s0);                    break;
			case Shader::OPCODE_F2B:		f2b(d, s0);						break;
			case Shader::OPCODE_B2F:		b2f(d, s0);						break;
			case Shader::OPCODE_F2I:        f2i(d, s0);                     break;
			case Shader::OPCODE_I2F:        i2f(d, s0);                     break;
			case Shader::OPCODE_F2U:        f2u(d, s0);                     break;
			case Shader::OPCODE_U2F:        u2f(d, s0);                     break;
			case Shader::OPCODE_I2B:        i2b(d, s0);                     break;
			case Shader::OPCODE_B2I:        b2i(d, s0);                     break;
			case Shader::OPCODE_MUL:		mul(d, s0, s1);					break;
			case Shader::OPCODE_IMUL:       imul(d, s0, s1);                break;
			case Shader::OPCODE_NRM2:		nrm2(d, s0, pp);				break;
			case Shader::OPCODE_NRM3:		nrm3(d, s0, pp);				break;
			case Shader::OPCODE_NRM4:		nrm4(d, s0, pp);				break;
			case Shader::OPCODE_POWX:		powx(d, s0, s1, pp);			break;
			case Shader::OPCODE_POW:		pow(d, s0, s1, pp);				break;
			case Shader::OPCODE_RCPX:		rcpx(d, s0, pp);				break;
			case Shader::OPCODE_DIV:		div(d, s0, s1);					break;
			case Shader::OPCODE_IDIV:       idiv(d, s0, s1);                break;
			case Shader::OPCODE_UDIV:       udiv(d, s0, s1);                break;
			case Shader::OPCODE_MOD:		mod(d, s0, s1);					break;
			case Shader::OPCODE_IMOD:       imod(d, s0, s1);                break;
			case Shader::OPCODE_UMOD:       umod(d, s0, s1);                break;
			case Shader::OPCODE_SHL:        shl(d, s0, s1);                 break;
			case Shader::OPCODE_ISHR:       ishr(d, s0, s1);                 break;
			case Shader::OPCODE_USHR:       ushr(d, s0, s1);                 break;
			case Shader::OPCODE_RSQX:		rsqx(d, s0, pp);				break;
			case Shader::OPCODE_SQRT:		sqrt(d, s0, pp);				break;
			case Shader::OPCODE_RSQ:		rsq(d, s0, pp);					break;
			case Shader::OPCODE_LEN2:		len2(d.x, s0, pp);				break;
			case Shader::OPCODE_LEN3:		len3(d.x, s0, pp);				break;
			case Shader::OPCODE_LEN4:		len4(d.x, s0, pp);				break;
			case Shader::OPCODE_DIST1:		dist1(d.x, s0, s1, pp);			break;
			case Shader::OPCODE_DIST2:		dist2(d.x, s0, s1, pp);			break;
			case Shader::OPCODE_DIST3:		dist3(d.x, s0, s1, pp);			break;
			case Shader::OPCODE_DIST4:		dist4(d.x, s0, s1, pp);			break;
			case Shader::OPCODE_SGE:		step(d, s1, s0);				break;
			case Shader::OPCODE_SGN:		sgn(d, s0);						break;
			case Shader::OPCODE_SINCOS:		sincos(d, s0, pp);				break;
			case Shader::OPCODE_COS:		cos(d, s0, pp);					break;
			case Shader::OPCODE_SIN:		sin(d, s0, pp);					break;
			case Shader::OPCODE_TAN:		tan(d, s0);						break;
			case Shader::OPCODE_ACOS:		acos(d, s0);					break;
			case Shader::OPCODE_ASIN:		asin(d, s0);					break;
			case Shader::OPCODE_ATAN:		atan(d, s0);					break;
			case Shader::OPCODE_ATAN2:		atan2(d, s0, s1);				break;
			case Shader::OPCODE_COSH:		cosh(d, s0, pp);				break;
			case Shader::OPCODE_SINH:		sinh(d, s0, pp);				break;
			case Shader::OPCODE_TANH:		tanh(d, s0, pp);				break;
			case Shader::OPCODE_ACOSH:		acosh(d, s0, pp);				break;
			case Shader::OPCODE_ASINH:		asinh(d, s0, pp);				break;
			case Shader::OPCODE_ATANH:		atanh(d, s0, pp);				break;
			case Shader::OPCODE_SLT:		slt(d, s0, s1);					break;
			case Shader::OPCODE_SUB:		sub(d, s0, s1);					break;
			case Shader::OPCODE_ISUB:       isub(d, s0, s1);                break;
			case Shader::OPCODE_BREAK:		BREAK(r);						break;
			case Shader::OPCODE_BREAKC:		BREAKC(r, s0, s1, control);		break;
			case Shader::OPCODE_BREAKP:		BREAKP(r, src0);				break;
			case Shader::OPCODE_CONTINUE:	CONTINUE(r);					break;
			case Shader::OPCODE_TEST:		TEST();							break;
			case Shader::OPCODE_CALL:		CALL(r, dst.label, dst.callSite);         break;
			case Shader::OPCODE_CALLNZ:		CALLNZ(r, dst.label, dst.callSite, src0); break;
			case Shader::OPCODE_ELSE:		ELSE(r);						break;
			case Shader::OPCODE_ENDIF:		ENDIF(r);						break;
			case Shader::OPCODE_ENDLOOP:	ENDLOOP(r);						break;
			case Shader::OPCODE_ENDREP:		ENDREP(r);						break;
			case Shader::OPCODE_ENDWHILE:	ENDWHILE(r);					break;
			case Shader::OPCODE_IF:			IF(r, src0);					break;
			case Shader::OPCODE_IFC:		IFC(r, s0, s1, control);		break;
			case Shader::OPCODE_LABEL:		LABEL(dst.index);				break;
			case Shader::OPCODE_LOOP:		LOOP(r, src1);					break;
			case Shader::OPCODE_REP:		REP(r, src0);					break;
			case Shader::OPCODE_WHILE:		WHILE(r, src0);					break;
			case Shader::OPCODE_RET:		RET(r);							break;
			case Shader::OPCODE_LEAVE:		LEAVE(r);						break;
			case Shader::OPCODE_CMP:		cmp(d, s0, s1, control);		break;
			case Shader::OPCODE_ICMP:		icmp(d, s0, s1, control);		break;
			case Shader::OPCODE_UCMP:       ucmp(d, s0, s1, control);       break;
			case Shader::OPCODE_SELECT:		select(d, s0, s1, s2);			break;
			case Shader::OPCODE_EXTRACT:	extract(d.x, s0, s1.x);			break;
			case Shader::OPCODE_INSERT:		insert(d, s0, s1.x, s2.x);		break;
			case Shader::OPCODE_ALL:		all(d.x, s0);					break;
			case Shader::OPCODE_ANY:		any(d.x, s0);					break;
			case Shader::OPCODE_NOT:		not(d, s0);						break;
			case Shader::OPCODE_OR:         or(d, s0, s1);                  break;
			case Shader::OPCODE_XOR:        xor(d, s0, s1);                 break;
			case Shader::OPCODE_AND:        and(d, s0, s1);                 break;
			case Shader::OPCODE_EQ:         equal(d, s0, s1);               break;
			case Shader::OPCODE_NE:         notEqual(d, s0, s1);            break;
			case Shader::OPCODE_TEXLDL:		TEXLDL(r, d, s0, src1);			break;
			case Shader::OPCODE_TEX:		TEX(r, d, s0, src1);			break;
			case Shader::OPCODE_TEXOFFSET:  TEXOFFSET(r, d, s0, src1, s2, s3);        break;
			case Shader::OPCODE_TEXLDLOFFSET: TEXLDL(r, d, s0, src1, s2);	break;
			case Shader::OPCODE_TEXELFETCH: TEXELFETCH(r, d, s0, src1, s2);           break;
			case Shader::OPCODE_TEXELFETCHOFFSET: TEXELFETCH(r, d, s0, src1, s2, s3); break;
			case Shader::OPCODE_TEXGRAD:    TEXGRAD(r, d, s0, src1, s2, s3);          break;
			case Shader::OPCODE_TEXGRADOFFSET: TEXGRAD(r, d, s0, src1, s2, s3, s4);   break;
			case Shader::OPCODE_TEXSIZE:	TEXSIZE(r, d, s0.x, src1);		break;
			case Shader::OPCODE_END:										break;
			default:
				ASSERT(false);
			}

			if(dst.type != Shader::PARAMETER_VOID && dst.type != Shader::PARAMETER_LABEL && opcode != Shader::OPCODE_NOP)
			{
				if(dst.integer)
				{
					switch(opcode)
					{
					case Shader::OPCODE_DIV:
						if(dst.x) d.x = Trunc(d.x);
						if(dst.y) d.y = Trunc(d.y);
						if(dst.z) d.z = Trunc(d.z);
						if(dst.w) d.w = Trunc(d.w);
						break;
					default:
						break;   // No truncation to integer required when arguments are integer
					}
				}

				if(dst.saturate)
				{
					if(dst.x) d.x = Max(d.x, Float4(0.0f));
					if(dst.y) d.y = Max(d.y, Float4(0.0f));
					if(dst.z) d.z = Max(d.z, Float4(0.0f));
					if(dst.w) d.w = Max(d.w, Float4(0.0f));

					if(dst.x) d.x = Min(d.x, Float4(1.0f));
					if(dst.y) d.y = Min(d.y, Float4(1.0f));
					if(dst.z) d.z = Min(d.z, Float4(1.0f));
					if(dst.w) d.w = Min(d.w, Float4(1.0f));
				}

				if(instruction->isPredicated())
				{
					Vector4f pDst;   // FIXME: Rename

					switch(dst.type)
					{
					case Shader::PARAMETER_VOID:																		break;
					case Shader::PARAMETER_TEMP:
						if(dst.rel.type == Shader::PARAMETER_VOID)
						{
							if(dst.x) pDst.x = r.r[dst.index].x;
							if(dst.y) pDst.y = r.r[dst.index].y;
							if(dst.z) pDst.z = r.r[dst.index].z;
							if(dst.w) pDst.w = r.r[dst.index].w;
						}
						else
						{
							Int a = relativeAddress(r, dst);

							if(dst.x) pDst.x = r.r[dst.index + a].x;
							if(dst.y) pDst.y = r.r[dst.index + a].y;
							if(dst.z) pDst.z = r.r[dst.index + a].z;
							if(dst.w) pDst.w = r.r[dst.index + a].w;
						}
						break;
					case Shader::PARAMETER_ADDR:		pDst = r.a0;													break;
					case Shader::PARAMETER_RASTOUT:
						switch(dst.index)
						{
						case 0:
							if(dst.x) pDst.x = r.o[Pos].x;
							if(dst.y) pDst.y = r.o[Pos].y;
							if(dst.z) pDst.z = r.o[Pos].z;
							if(dst.w) pDst.w = r.o[Pos].w;
							break;
						case 1:
							pDst.x = r.o[Fog].x;
							break;
						case 2:
							pDst.x = r.o[Pts].y;
							break;
						default:
							ASSERT(false);
						}
						break;
					case Shader::PARAMETER_ATTROUT:
						if(dst.x) pDst.x = r.o[D0 + dst.index].x;
						if(dst.y) pDst.y = r.o[D0 + dst.index].y;
						if(dst.z) pDst.z = r.o[D0 + dst.index].z;
						if(dst.w) pDst.w = r.o[D0 + dst.index].w;
						break;
					case Shader::PARAMETER_TEXCRDOUT:
				//	case Shader::PARAMETER_OUTPUT:
						if(version < 0x0300)
						{
							if(dst.x) pDst.x = r.o[T0 + dst.index].x;
							if(dst.y) pDst.y = r.o[T0 + dst.index].y;
							if(dst.z) pDst.z = r.o[T0 + dst.index].z;
							if(dst.w) pDst.w = r.o[T0 + dst.index].w;
						}
						else
						{
							if(dst.rel.type == Shader::PARAMETER_VOID)   // Not relative
							{
								if(dst.x) pDst.x = r.o[dst.index].x;
								if(dst.y) pDst.y = r.o[dst.index].y;
								if(dst.z) pDst.z = r.o[dst.index].z;
								if(dst.w) pDst.w = r.o[dst.index].w;
							}
							else if(dst.rel.type == Shader::PARAMETER_LOOP)
							{
								Int aL = r.aL[r.loopDepth];

								if(dst.x) pDst.x = r.o[dst.index + aL].x;
								if(dst.y) pDst.y = r.o[dst.index + aL].y;
								if(dst.z) pDst.z = r.o[dst.index + aL].z;
								if(dst.w) pDst.w = r.o[dst.index + aL].w;
							}
							else
							{
								Int a = relativeAddress(r, dst);

								if(dst.x) pDst.x = r.o[dst.index + a].x;
								if(dst.y) pDst.y = r.o[dst.index + a].y;
								if(dst.z) pDst.z = r.o[dst.index + a].z;
								if(dst.w) pDst.w = r.o[dst.index + a].w;
							}
						}
						break;
					case Shader::PARAMETER_LABEL:																		break;
					case Shader::PARAMETER_PREDICATE:	pDst = r.p0;													break;
					case Shader::PARAMETER_INPUT:																		break;
					default:
						ASSERT(false);
					}

					Int4 enable = enableMask(r, instruction);

					Int4 xEnable = enable;
					Int4 yEnable = enable;
					Int4 zEnable = enable;
					Int4 wEnable = enable;

					if(predicate)
					{
						unsigned char pSwizzle = instruction->predicateSwizzle;

						Float4 xPredicate = r.p0[(pSwizzle >> 0) & 0x03];
						Float4 yPredicate = r.p0[(pSwizzle >> 2) & 0x03];
						Float4 zPredicate = r.p0[(pSwizzle >> 4) & 0x03];
						Float4 wPredicate = r.p0[(pSwizzle >> 6) & 0x03];

						if(!instruction->predicateNot)
						{
							if(dst.x) xEnable = xEnable & As<Int4>(xPredicate);
							if(dst.y) yEnable = yEnable & As<Int4>(yPredicate);
							if(dst.z) zEnable = zEnable & As<Int4>(zPredicate);
							if(dst.w) wEnable = wEnable & As<Int4>(wPredicate);
						}
						else
						{
							if(dst.x) xEnable = xEnable & ~As<Int4>(xPredicate);
							if(dst.y) yEnable = yEnable & ~As<Int4>(yPredicate);
							if(dst.z) zEnable = zEnable & ~As<Int4>(zPredicate);
							if(dst.w) wEnable = wEnable & ~As<Int4>(wPredicate);
						}
					}

					if(dst.x) d.x = As<Float4>(As<Int4>(d.x) & xEnable);
					if(dst.y) d.y = As<Float4>(As<Int4>(d.y) & yEnable);
					if(dst.z) d.z = As<Float4>(As<Int4>(d.z) & zEnable);
					if(dst.w) d.w = As<Float4>(As<Int4>(d.w) & wEnable);

					if(dst.x) d.x = As<Float4>(As<Int4>(d.x) | (As<Int4>(pDst.x) & ~xEnable));
					if(dst.y) d.y = As<Float4>(As<Int4>(d.y) | (As<Int4>(pDst.y) & ~yEnable));
					if(dst.z) d.z = As<Float4>(As<Int4>(d.z) | (As<Int4>(pDst.z) & ~zEnable));
					if(dst.w) d.w = As<Float4>(As<Int4>(d.w) | (As<Int4>(pDst.w) & ~wEnable));
				}

				switch(dst.type)
				{
				case Shader::PARAMETER_VOID:
					break;
				case Shader::PARAMETER_TEMP:
					if(dst.rel.type == Shader::PARAMETER_VOID)
					{
						if(dst.x) r.r[dst.index].x = d.x;
						if(dst.y) r.r[dst.index].y = d.y;
						if(dst.z) r.r[dst.index].z = d.z;
						if(dst.w) r.r[dst.index].w = d.w;
					}
					else
					{
						Int a = relativeAddress(r, dst);

						if(dst.x) r.r[dst.index + a].x = d.x;
						if(dst.y) r.r[dst.index + a].y = d.y;
						if(dst.z) r.r[dst.index + a].z = d.z;
						if(dst.w) r.r[dst.index + a].w = d.w;
					}
					break;
				case Shader::PARAMETER_ADDR:
					if(dst.x) r.a0.x = d.x;
					if(dst.y) r.a0.y = d.y;
					if(dst.z) r.a0.z = d.z;
					if(dst.w) r.a0.w = d.w;
					break;
				case Shader::PARAMETER_RASTOUT:
					switch(dst.index)
					{
					case 0:
						if(dst.x) r.o[Pos].x = d.x;
						if(dst.y) r.o[Pos].y = d.y;
						if(dst.z) r.o[Pos].z = d.z;
						if(dst.w) r.o[Pos].w = d.w;
						break;
					case 1:
						r.o[Fog].x = d.x;
						break;
					case 2:		
						r.o[Pts].y = d.x;
						break;
					default:	ASSERT(false);
					}
					break;
				case Shader::PARAMETER_ATTROUT:	
					if(dst.x) r.o[D0 + dst.index].x = d.x;
					if(dst.y) r.o[D0 + dst.index].y = d.y;
					if(dst.z) r.o[D0 + dst.index].z = d.z;
					if(dst.w) r.o[D0 + dst.index].w = d.w;
					break;
				case Shader::PARAMETER_TEXCRDOUT:
			//	case Shader::PARAMETER_OUTPUT:
					if(version < 0x0300)
					{
						if(dst.x) r.o[T0 + dst.index].x = d.x;
						if(dst.y) r.o[T0 + dst.index].y = d.y;
						if(dst.z) r.o[T0 + dst.index].z = d.z;
						if(dst.w) r.o[T0 + dst.index].w = d.w;
					}
					else
					{
						if(dst.rel.type == Shader::PARAMETER_VOID)   // Not relative
						{
							if(dst.x) r.o[dst.index].x = d.x;
							if(dst.y) r.o[dst.index].y = d.y;
							if(dst.z) r.o[dst.index].z = d.z;
							if(dst.w) r.o[dst.index].w = d.w;
						}
						else if(dst.rel.type == Shader::PARAMETER_LOOP)
						{
							Int aL = r.aL[r.loopDepth];

							if(dst.x) r.o[dst.index + aL].x = d.x;
							if(dst.y) r.o[dst.index + aL].y = d.y;
							if(dst.z) r.o[dst.index + aL].z = d.z;
							if(dst.w) r.o[dst.index + aL].w = d.w;
						}
						else
						{
							Int a = relativeAddress(r, dst);

							if(dst.x) r.o[dst.index + a].x = d.x;
							if(dst.y) r.o[dst.index + a].y = d.y;
							if(dst.z) r.o[dst.index + a].z = d.z;
							if(dst.w) r.o[dst.index + a].w = d.w;
						}
					}
					break;
				case Shader::PARAMETER_LABEL:																		break;
				case Shader::PARAMETER_PREDICATE:	r.p0 = d;														break;
				case Shader::PARAMETER_INPUT:																		break;
				default:
					ASSERT(false);
				}
			}
		}

		if(currentLabel != -1)
		{
			Nucleus::setInsertBlock(returnBlock);
		}
	}

	void VertexProgram::passThrough(Registers &r)
	{
		if(shader)
		{
			for(int i = 0; i < 12; i++)
			{
				unsigned char usage = shader->output[i][0].usage;

				switch(usage)
				{
				case 0xFF:
					continue;
				case Shader::USAGE_PSIZE:
					r.o[i].y = r.v[i].x;
					break;
				case Shader::USAGE_TEXCOORD:
					r.o[i].x = r.v[i].x;
					r.o[i].y = r.v[i].y;
					r.o[i].z = r.v[i].z;
					r.o[i].w = r.v[i].w;
					break;
				case Shader::USAGE_POSITION:
					r.o[i].x = r.v[i].x;
					r.o[i].y = r.v[i].y;
					r.o[i].z = r.v[i].z;
					r.o[i].w = r.v[i].w;
					break;
				case Shader::USAGE_COLOR:
					r.o[i].x = r.v[i].x;
					r.o[i].y = r.v[i].y;
					r.o[i].z = r.v[i].z;
					r.o[i].w = r.v[i].w;
					break;
				case Shader::USAGE_FOG:
					r.o[i].x = r.v[i].x;
					break;
				default:
					ASSERT(false);
				}
			}
		}
		else
		{
			r.o[Pos].x = r.v[PositionT].x;
			r.o[Pos].y = r.v[PositionT].y;
			r.o[Pos].z = r.v[PositionT].z;
			r.o[Pos].w = r.v[PositionT].w;

			for(int i = 0; i < 2; i++)
			{
				r.o[D0 + i].x = r.v[Color0 + i].x;
				r.o[D0 + i].y = r.v[Color0 + i].y;
				r.o[D0 + i].z = r.v[Color0 + i].z;
				r.o[D0 + i].w = r.v[Color0 + i].w;
			}

			for(int i = 0; i < 8; i++)
			{
				r.o[T0 + i].x = r.v[TexCoord0 + i].x;
				r.o[T0 + i].y = r.v[TexCoord0 + i].y;
				r.o[T0 + i].z = r.v[TexCoord0 + i].z;
				r.o[T0 + i].w = r.v[TexCoord0 + i].w;
			}

			r.o[Pts].y = r.v[PointSize].x;
		}
	}

	Vector4f VertexProgram::fetchRegisterF(Registers &r, const Src &src, unsigned int offset)
	{
		Vector4f reg;
		unsigned int i = src.index + offset;

		switch(src.type)
		{
		case Shader::PARAMETER_TEMP:
			if(src.rel.type == Shader::PARAMETER_VOID)
			{
				reg = r.r[i];
			}
			else
			{
				reg = r.r[i + relativeAddress(r, src)];
			}
			break;
		case Shader::PARAMETER_CONST:
			reg = readConstant(r, src, offset);
			break;
		case Shader::PARAMETER_INPUT:
            if(src.rel.type == Shader::PARAMETER_VOID)
			{
				reg = r.v[i];
			}
			else
			{
				reg = r.v[i + relativeAddress(r, src)];
			}
            break;
		case Shader::PARAMETER_VOID:			return r.r[0];   // Dummy
		case Shader::PARAMETER_FLOAT4LITERAL:
			reg.x = Float4(src.value[0]);
			reg.y = Float4(src.value[1]);
			reg.z = Float4(src.value[2]);
			reg.w = Float4(src.value[3]);
			break;
		case Shader::PARAMETER_ADDR:			reg = r.a0;		break;
		case Shader::PARAMETER_CONSTBOOL:		return r.r[0];   // Dummy
		case Shader::PARAMETER_CONSTINT:		return r.r[0];   // Dummy
		case Shader::PARAMETER_LOOP:			return r.r[0];   // Dummy
		case Shader::PARAMETER_PREDICATE:		return r.r[0];   // Dummy
		case Shader::PARAMETER_SAMPLER:
			if(src.rel.type == Shader::PARAMETER_VOID)
			{
				reg.x = As<Float4>(Int4(i));
			}
			else if(src.rel.type == Shader::PARAMETER_TEMP)
			{
				reg.x = As<Float4>(Int4(i) + As<Int4>(r.r[src.rel.index].x));
			}
			return reg;
		case Shader::PARAMETER_OUTPUT:
            if(src.rel.type == Shader::PARAMETER_VOID)
			{
				reg = r.o[i];
			}
			else
			{
				reg = r.o[i + relativeAddress(r, src)];
			}
			break;
		case Shader::PARAMETER_MISCTYPE:
			reg.x = As<Float>(Int(r.instanceID));
			return reg;
		default:
			ASSERT(false);
		}

		const Float4 &x = reg[(src.swizzle >> 0) & 0x3];
		const Float4 &y = reg[(src.swizzle >> 2) & 0x3];
		const Float4 &z = reg[(src.swizzle >> 4) & 0x3];
		const Float4 &w = reg[(src.swizzle >> 6) & 0x3];

		Vector4f mod;

		switch(src.modifier)
		{
		case Shader::MODIFIER_NONE:
			mod.x = x;
			mod.y = y;
			mod.z = z;
			mod.w = w;
			break;
		case Shader::MODIFIER_NEGATE:
			mod.x = -x;
			mod.y = -y;
			mod.z = -z;
			mod.w = -w;
			break;
		case Shader::MODIFIER_ABS:
			mod.x = Abs(x);
			mod.y = Abs(y);
			mod.z = Abs(z);
			mod.w = Abs(w);
			break;
		case Shader::MODIFIER_ABS_NEGATE:
			mod.x = -Abs(x);
			mod.y = -Abs(y);
			mod.z = -Abs(z);
			mod.w = -Abs(w);
			break;
		case Shader::MODIFIER_NOT:
			mod.x = As<Float4>(As<Int4>(x) ^ Int4(0xFFFFFFFF));
			mod.y = As<Float4>(As<Int4>(y) ^ Int4(0xFFFFFFFF));
			mod.z = As<Float4>(As<Int4>(z) ^ Int4(0xFFFFFFFF));
			mod.w = As<Float4>(As<Int4>(w) ^ Int4(0xFFFFFFFF));
			break;
		default:
			ASSERT(false);
		}

		return mod;
	}

	Vector4f VertexProgram::readConstant(Registers &r, const Src &src, unsigned int offset)
	{
		Vector4f c;
		unsigned int i = src.index + offset;

		if(src.rel.type == Shader::PARAMETER_VOID)   // Not relative
		{
			c.x = c.y = c.z = c.w = *Pointer<Float4>(r.data + OFFSET(DrawData,vs.c[i]));

			c.x = c.x.xxxx;
			c.y = c.y.yyyy;
			c.z = c.z.zzzz;
			c.w = c.w.wwww;

			if(shader->containsDefineInstruction())   // Constant may be known at compile time
			{
				for(size_t j = 0; j < shader->getLength(); j++)
				{
					const Shader::Instruction &instruction = *shader->getInstruction(j);

					if(instruction.opcode == Shader::OPCODE_DEF)
					{
						if(instruction.dst.index == i)
						{
							c.x = Float4(instruction.src[0].value[0]);
							c.y = Float4(instruction.src[0].value[1]);
							c.z = Float4(instruction.src[0].value[2]);
							c.w = Float4(instruction.src[0].value[3]);

							break;
						}
					}
				}
			}
		}
		else if(src.rel.type == Shader::PARAMETER_LOOP)
		{
			Int loopCounter = r.aL[r.loopDepth];

			c.x = c.y = c.z = c.w = *Pointer<Float4>(r.data + OFFSET(DrawData,vs.c[i]) + loopCounter * 16);

			c.x = c.x.xxxx;
			c.y = c.y.yyyy;
			c.z = c.z.zzzz;
			c.w = c.w.wwww;
		}
		else
		{
			if(src.rel.deterministic)
			{
				Int a = relativeAddress(r, src);
			
				c.x = c.y = c.z = c.w = *Pointer<Float4>(r.data + OFFSET(DrawData,vs.c[i]) + a * 16);

				c.x = c.x.xxxx;
				c.y = c.y.yyyy;
				c.z = c.z.zzzz;
				c.w = c.w.wwww;
			}
			else
			{
				int component = src.rel.swizzle & 0x03;
				Float4 a;

				switch(src.rel.type)
				{
				case Shader::PARAMETER_ADDR:   a = r.a0[component]; break;
				case Shader::PARAMETER_TEMP:   a = r.r[src.rel.index][component]; break;
				case Shader::PARAMETER_INPUT:  a = r.v[src.rel.index][component]; break;
				case Shader::PARAMETER_OUTPUT: a = r.o[src.rel.index][component]; break;
				case Shader::PARAMETER_CONST:  a = *Pointer<Float>(r.data + OFFSET(DrawData,vs.c[src.rel.index][component])); break;
				default: ASSERT(false);
				}

				Int4 index = Int4(i) + As<Int4>(a) * Int4(src.rel.scale);

				index = Min(As<UInt4>(index), UInt4(VERTEX_UNIFORM_VECTORS));   // Clamp to constant register range, c[VERTEX_UNIFORM_VECTORS] = {0, 0, 0, 0}
				
				Int index0 = Extract(index, 0);
				Int index1 = Extract(index, 1);
				Int index2 = Extract(index, 2);
				Int index3 = Extract(index, 3);

				c.x = *Pointer<Float4>(r.data + OFFSET(DrawData,vs.c) + index0 * 16, 16);
				c.y = *Pointer<Float4>(r.data + OFFSET(DrawData,vs.c) + index1 * 16, 16);
				c.z = *Pointer<Float4>(r.data + OFFSET(DrawData,vs.c) + index2 * 16, 16);
				c.w = *Pointer<Float4>(r.data + OFFSET(DrawData,vs.c) + index3 * 16, 16);

				transpose4x4(c.x, c.y, c.z, c.w);
			}
		}

		return c;
	}

	Int VertexProgram::relativeAddress(Registers &r, const Shader::Parameter &var)
	{
		ASSERT(var.rel.deterministic);

		if(var.rel.type == Shader::PARAMETER_TEMP)
		{
			return As<Int>(Extract(r.r[var.rel.index].x, 0)) * var.rel.scale;
		}
		else if(var.rel.type == Shader::PARAMETER_INPUT)
		{
			return As<Int>(Extract(r.v[var.rel.index].x, 0)) * var.rel.scale;
		}
		else if(var.rel.type == Shader::PARAMETER_OUTPUT)
		{
			return As<Int>(Extract(r.o[var.rel.index].x, 0)) * var.rel.scale;
		}
		else if(var.rel.type == Shader::PARAMETER_CONST)
		{
			RValue<Int4> c = *Pointer<Int4>(r.data + OFFSET(DrawData, vs.c[var.rel.index]));

			return Extract(c, 0) * var.rel.scale;
		}
		else ASSERT(false);

		return 0;
	}

	Int4 VertexProgram::enableMask(Registers &r, const Shader::Instruction *instruction)
	{
		Int4 enable = instruction->analysisBranch ? Int4(r.enableStack[r.enableIndex]) : Int4(0xFFFFFFFF);
		
		if(!whileTest)
		{
			if(shader->containsBreakInstruction() && instruction->analysisBreak)
			{
				enable &= r.enableBreak;
			}

			if(shader->containsContinueInstruction() && instruction->analysisContinue)
			{
				enable &= r.enableContinue;
			}

			if(shader->containsLeaveInstruction() && instruction->analysisLeave)
			{
				enable &= r.enableLeave;
			}
		}

		return enable;
	}

	void VertexProgram::M3X2(Registers &r, Vector4f &dst, Vector4f &src0, Src &src1)
	{
		Vector4f row0 = fetchRegisterF(r, src1, 0);
		Vector4f row1 = fetchRegisterF(r, src1, 1);

		dst.x = dot3(src0, row0);
		dst.y = dot3(src0, row1);
	}

	void VertexProgram::M3X3(Registers &r, Vector4f &dst, Vector4f &src0, Src &src1)
	{
		Vector4f row0 = fetchRegisterF(r, src1, 0);
		Vector4f row1 = fetchRegisterF(r, src1, 1);
		Vector4f row2 = fetchRegisterF(r, src1, 2);

		dst.x = dot3(src0, row0);
		dst.y = dot3(src0, row1);
		dst.z = dot3(src0, row2);
	}

	void VertexProgram::M3X4(Registers &r, Vector4f &dst, Vector4f &src0, Src &src1)
	{
		Vector4f row0 = fetchRegisterF(r, src1, 0);
		Vector4f row1 = fetchRegisterF(r, src1, 1);
		Vector4f row2 = fetchRegisterF(r, src1, 2);
		Vector4f row3 = fetchRegisterF(r, src1, 3);

		dst.x = dot3(src0, row0);
		dst.y = dot3(src0, row1);
		dst.z = dot3(src0, row2);
		dst.w = dot3(src0, row3);
	}

	void VertexProgram::M4X3(Registers &r, Vector4f &dst, Vector4f &src0, Src &src1)
	{
		Vector4f row0 = fetchRegisterF(r, src1, 0);
		Vector4f row1 = fetchRegisterF(r, src1, 1);
		Vector4f row2 = fetchRegisterF(r, src1, 2);

		dst.x = dot4(src0, row0);
		dst.y = dot4(src0, row1);
		dst.z = dot4(src0, row2);
	}

	void VertexProgram::M4X4(Registers &r, Vector4f &dst, Vector4f &src0, Src &src1)
	{
		Vector4f row0 = fetchRegisterF(r, src1, 0);
		Vector4f row1 = fetchRegisterF(r, src1, 1);
		Vector4f row2 = fetchRegisterF(r, src1, 2);
		Vector4f row3 = fetchRegisterF(r, src1, 3);

		dst.x = dot4(src0, row0);
		dst.y = dot4(src0, row1);
		dst.z = dot4(src0, row2);
		dst.w = dot4(src0, row3);
	}

	void VertexProgram::BREAK(Registers &r)
	{
		llvm::BasicBlock *deadBlock = Nucleus::createBasicBlock();
		llvm::BasicBlock *endBlock = loopRepEndBlock[loopRepDepth - 1];

		if(breakDepth == 0)
		{
			r.enableIndex = r.enableIndex - breakDepth;
			Nucleus::createBr(endBlock);
		}
		else
		{
			r.enableBreak = r.enableBreak & ~r.enableStack[r.enableIndex];
			Bool allBreak = SignMask(r.enableBreak) == 0x0;

			r.enableIndex = r.enableIndex - breakDepth;
			branch(allBreak, endBlock, deadBlock);
		}

		Nucleus::setInsertBlock(deadBlock);
		r.enableIndex = r.enableIndex + breakDepth;
	}

	void VertexProgram::BREAKC(Registers &r, Vector4f &src0, Vector4f &src1, Control control)
	{
		Int4 condition;

		switch(control)
		{
		case Shader::CONTROL_GT: condition = CmpNLE(src0.x,  src1.x);	break;
		case Shader::CONTROL_EQ: condition = CmpEQ(src0.x, src1.x);		break;
		case Shader::CONTROL_GE: condition = CmpNLT(src0.x, src1.x);	break;
		case Shader::CONTROL_LT: condition = CmpLT(src0.x,  src1.x);	break;
		case Shader::CONTROL_NE: condition = CmpNEQ(src0.x, src1.x);	break;
		case Shader::CONTROL_LE: condition = CmpLE(src0.x, src1.x);		break;
		default:
			ASSERT(false);
		}

		BREAK(r, condition);
	}

	void VertexProgram::BREAKP(Registers &r, const Src &predicateRegister)   // FIXME: Factor out parts common with BREAKC
	{
		Int4 condition = As<Int4>(r.p0[predicateRegister.swizzle & 0x3]);

		if(predicateRegister.modifier == Shader::MODIFIER_NOT)
		{
			condition = ~condition;
		}

		BREAK(r, condition);
	}

	void VertexProgram::BREAK(Registers &r, Int4 &condition)
	{
		condition &= r.enableStack[r.enableIndex];

		llvm::BasicBlock *continueBlock = Nucleus::createBasicBlock();
		llvm::BasicBlock *endBlock = loopRepEndBlock[loopRepDepth - 1];

		r.enableBreak = r.enableBreak & ~condition;
		Bool allBreak = SignMask(r.enableBreak) == 0x0;

		r.enableIndex = r.enableIndex - breakDepth;
		branch(allBreak, endBlock, continueBlock);

		Nucleus::setInsertBlock(continueBlock);
		r.enableIndex = r.enableIndex + breakDepth;
	}

	void VertexProgram::CONTINUE(Registers &r)
	{
		r.enableContinue = r.enableContinue & ~r.enableStack[r.enableIndex];
	}

	void VertexProgram::TEST()
	{
		whileTest = true;
	}

	void VertexProgram::CALL(Registers &r, int labelIndex, int callSiteIndex)
	{
		if(!labelBlock[labelIndex])
		{
			labelBlock[labelIndex] = Nucleus::createBasicBlock();
		}

		if(callRetBlock[labelIndex].size() > 1)
		{
			r.callStack[r.stackIndex++] = UInt(callSiteIndex);
		}

		Int4 restoreLeave = r.enableLeave;

		Nucleus::createBr(labelBlock[labelIndex]);
		Nucleus::setInsertBlock(callRetBlock[labelIndex][callSiteIndex]);

		r.enableLeave = restoreLeave;
	}

	void VertexProgram::CALLNZ(Registers &r, int labelIndex, int callSiteIndex, const Src &src)
	{
		if(src.type == Shader::PARAMETER_CONSTBOOL)
		{
			CALLNZb(r, labelIndex, callSiteIndex, src);
		}
		else if(src.type == Shader::PARAMETER_PREDICATE)
		{
			CALLNZp(r, labelIndex, callSiteIndex, src);
		}
		else ASSERT(false);
	}

	void VertexProgram::CALLNZb(Registers &r, int labelIndex, int callSiteIndex, const Src &boolRegister)
	{
		Bool condition = (*Pointer<Byte>(r.data + OFFSET(DrawData,vs.b[boolRegister.index])) != Byte(0));   // FIXME
		
		if(boolRegister.modifier == Shader::MODIFIER_NOT)
		{
			condition = !condition;	
		}

		if(!labelBlock[labelIndex])
		{
			labelBlock[labelIndex] = Nucleus::createBasicBlock();
		}

		if(callRetBlock[labelIndex].size() > 1)
		{
			r.callStack[r.stackIndex++] = UInt(callSiteIndex);
		}

		Int4 restoreLeave = r.enableLeave;

		branch(condition, labelBlock[labelIndex], callRetBlock[labelIndex][callSiteIndex]);
		Nucleus::setInsertBlock(callRetBlock[labelIndex][callSiteIndex]);

		r.enableLeave = restoreLeave;
	}

	void VertexProgram::CALLNZp(Registers &r, int labelIndex, int callSiteIndex, const Src &predicateRegister)
	{
		Int4 condition = As<Int4>(r.p0[predicateRegister.swizzle & 0x3]);

		if(predicateRegister.modifier == Shader::MODIFIER_NOT)
		{
			condition = ~condition;
		}

		condition &= r.enableStack[r.enableIndex];

		if(!labelBlock[labelIndex])
		{
			labelBlock[labelIndex] = Nucleus::createBasicBlock();
		}

		if(callRetBlock[labelIndex].size() > 1)
		{
			r.callStack[r.stackIndex++] = UInt(callSiteIndex);
		}

		r.enableIndex++;
		r.enableStack[r.enableIndex] = condition;
		Int4 restoreLeave = r.enableLeave;

		Bool notAllFalse = SignMask(condition) != 0;
		branch(notAllFalse, labelBlock[labelIndex], callRetBlock[labelIndex][callSiteIndex]);
		Nucleus::setInsertBlock(callRetBlock[labelIndex][callSiteIndex]);

		r.enableIndex--;
		r.enableLeave = restoreLeave;
	}

	void VertexProgram::ELSE(Registers &r)
	{
		ifDepth--;

		llvm::BasicBlock *falseBlock = ifFalseBlock[ifDepth];
		llvm::BasicBlock *endBlock = Nucleus::createBasicBlock();

		if(isConditionalIf[ifDepth])
		{
			Int4 condition = ~r.enableStack[r.enableIndex] & r.enableStack[r.enableIndex - 1];
			Bool notAllFalse = SignMask(condition) != 0;

			branch(notAllFalse, falseBlock, endBlock);

			r.enableStack[r.enableIndex] = ~r.enableStack[r.enableIndex] & r.enableStack[r.enableIndex - 1];
		}
		else
		{
			Nucleus::createBr(endBlock);
			Nucleus::setInsertBlock(falseBlock);
		}

		ifFalseBlock[ifDepth] = endBlock;

		ifDepth++;
	}

	void VertexProgram::ENDIF(Registers &r)
	{
		ifDepth--;

		llvm::BasicBlock *endBlock = ifFalseBlock[ifDepth];

		Nucleus::createBr(endBlock);
		Nucleus::setInsertBlock(endBlock);

		if(isConditionalIf[ifDepth])
		{
			breakDepth--;
			r.enableIndex--;
		}
	}

	void VertexProgram::ENDLOOP(Registers &r)
	{
		loopRepDepth--;

		r.aL[r.loopDepth] = r.aL[r.loopDepth] + r.increment[r.loopDepth];   // FIXME: +=

		llvm::BasicBlock *testBlock = loopRepTestBlock[loopRepDepth];
		llvm::BasicBlock *endBlock = loopRepEndBlock[loopRepDepth];

		Nucleus::createBr(testBlock);
		Nucleus::setInsertBlock(endBlock);

		r.loopDepth--;
		r.enableBreak = Int4(0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF);
	}

	void VertexProgram::ENDREP(Registers &r)
	{
		loopRepDepth--;

		llvm::BasicBlock *testBlock = loopRepTestBlock[loopRepDepth];
		llvm::BasicBlock *endBlock = loopRepEndBlock[loopRepDepth];

		Nucleus::createBr(testBlock);
		Nucleus::setInsertBlock(endBlock);

		r.loopDepth--;
		r.enableBreak = Int4(0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF);
	}

	void VertexProgram::ENDWHILE(Registers &r)
	{
		loopRepDepth--;

		llvm::BasicBlock *testBlock = loopRepTestBlock[loopRepDepth];
		llvm::BasicBlock *endBlock = loopRepEndBlock[loopRepDepth];

		Nucleus::createBr(testBlock);
		Nucleus::setInsertBlock(endBlock);

		r.enableIndex--;
		r.enableBreak = Int4(0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF);
		whileTest = false;
	}

	void VertexProgram::IF(Registers &r, const Src &src)
	{
		if(src.type == Shader::PARAMETER_CONSTBOOL)
		{
			IFb(r, src);
		}
		else if(src.type == Shader::PARAMETER_PREDICATE)
		{
			IFp(r, src);
		}
		else
		{
			Int4 condition = As<Int4>(fetchRegisterF(r, src).x);
			IF(r, condition);
		}
	}

	void VertexProgram::IFb(Registers &r, const Src &boolRegister)
	{
		ASSERT(ifDepth < 24 + 4);

		Bool condition = (*Pointer<Byte>(r.data + OFFSET(DrawData,vs.b[boolRegister.index])) != Byte(0));   // FIXME

		if(boolRegister.modifier == Shader::MODIFIER_NOT)
		{
			condition = !condition;
		}

		llvm::BasicBlock *trueBlock = Nucleus::createBasicBlock();
		llvm::BasicBlock *falseBlock = Nucleus::createBasicBlock();

		branch(condition, trueBlock, falseBlock);

		isConditionalIf[ifDepth] = false;
		ifFalseBlock[ifDepth] = falseBlock;

		ifDepth++;
	}

	void VertexProgram::IFp(Registers &r, const Src &predicateRegister)
	{
		Int4 condition = As<Int4>(r.p0[predicateRegister.swizzle & 0x3]);

		if(predicateRegister.modifier == Shader::MODIFIER_NOT)
		{
			condition = ~condition;
		}

		IF(r, condition);
	}

	void VertexProgram::IFC(Registers &r, Vector4f &src0, Vector4f &src1, Control control)
	{
		Int4 condition;

		switch(control)
		{
		case Shader::CONTROL_GT: condition = CmpNLE(src0.x,  src1.x);	break;
		case Shader::CONTROL_EQ: condition = CmpEQ(src0.x, src1.x);		break;
		case Shader::CONTROL_GE: condition = CmpNLT(src0.x, src1.x);	break;
		case Shader::CONTROL_LT: condition = CmpLT(src0.x,  src1.x);	break;
		case Shader::CONTROL_NE: condition = CmpNEQ(src0.x, src1.x);	break;
		case Shader::CONTROL_LE: condition = CmpLE(src0.x, src1.x);		break;
		default:
			ASSERT(false);
		}

		IF(r, condition);
	}

	void VertexProgram::IF(Registers &r, Int4 &condition)
	{
		condition &= r.enableStack[r.enableIndex];

		r.enableIndex++;
		r.enableStack[r.enableIndex] = condition;

		llvm::BasicBlock *trueBlock = Nucleus::createBasicBlock();
		llvm::BasicBlock *falseBlock = Nucleus::createBasicBlock();

		Bool notAllFalse = SignMask(condition) != 0;

		branch(notAllFalse, trueBlock, falseBlock);

		isConditionalIf[ifDepth] = true;
		ifFalseBlock[ifDepth] = falseBlock;

		ifDepth++;
		breakDepth++;
	}

	void VertexProgram::LABEL(int labelIndex)
	{
		if(!labelBlock[labelIndex])
		{
			labelBlock[labelIndex] = Nucleus::createBasicBlock();
		}

		Nucleus::setInsertBlock(labelBlock[labelIndex]);
		currentLabel = labelIndex;
	}

	void VertexProgram::LOOP(Registers &r, const Src &integerRegister)
	{
		r.loopDepth++;

		r.iteration[r.loopDepth] = *Pointer<Int>(r.data + OFFSET(DrawData,vs.i[integerRegister.index][0]));
		r.aL[r.loopDepth] = *Pointer<Int>(r.data + OFFSET(DrawData,vs.i[integerRegister.index][1]));
		r.increment[r.loopDepth] = *Pointer<Int>(r.data + OFFSET(DrawData,vs.i[integerRegister.index][2]));

		// FIXME: Compiles to two instructions?
		If(r.increment[r.loopDepth] == 0)
		{
			r.increment[r.loopDepth] = 1;
		}

		llvm::BasicBlock *loopBlock = Nucleus::createBasicBlock();
		llvm::BasicBlock *testBlock = Nucleus::createBasicBlock();
		llvm::BasicBlock *endBlock = Nucleus::createBasicBlock();

		loopRepTestBlock[loopRepDepth] = testBlock;
		loopRepEndBlock[loopRepDepth] = endBlock;

		// FIXME: jump(testBlock)
		Nucleus::createBr(testBlock);
		Nucleus::setInsertBlock(testBlock);

		branch(r.iteration[r.loopDepth] > 0, loopBlock, endBlock);
		Nucleus::setInsertBlock(loopBlock);

		r.iteration[r.loopDepth] = r.iteration[r.loopDepth] - 1;   // FIXME: --
		
		loopRepDepth++;
		breakDepth = 0;
	}

	void VertexProgram::REP(Registers &r, const Src &integerRegister)
	{
		r.loopDepth++;

		r.iteration[r.loopDepth] = *Pointer<Int>(r.data + OFFSET(DrawData,vs.i[integerRegister.index][0]));
		r.aL[r.loopDepth] = r.aL[r.loopDepth - 1];

		llvm::BasicBlock *loopBlock = Nucleus::createBasicBlock();
		llvm::BasicBlock *testBlock = Nucleus::createBasicBlock();
		llvm::BasicBlock *endBlock = Nucleus::createBasicBlock();

		loopRepTestBlock[loopRepDepth] = testBlock;
		loopRepEndBlock[loopRepDepth] = endBlock;

		// FIXME: jump(testBlock)
		Nucleus::createBr(testBlock);
		Nucleus::setInsertBlock(testBlock);

		branch(r.iteration[r.loopDepth] > 0, loopBlock, endBlock);
		Nucleus::setInsertBlock(loopBlock);

		r.iteration[r.loopDepth] = r.iteration[r.loopDepth] - 1;   // FIXME: --

		loopRepDepth++;
		breakDepth = 0;
	}

	void VertexProgram::WHILE(Registers &r, const Src &temporaryRegister)
	{
		r.enableIndex++;

		llvm::BasicBlock *loopBlock = Nucleus::createBasicBlock();
		llvm::BasicBlock *testBlock = Nucleus::createBasicBlock();
		llvm::BasicBlock *endBlock = Nucleus::createBasicBlock();
		
		loopRepTestBlock[loopRepDepth] = testBlock;
		loopRepEndBlock[loopRepDepth] = endBlock;

		Int4 restoreBreak = r.enableBreak;
		Int4 restoreContinue = r.enableContinue;

		// FIXME: jump(testBlock)
		Nucleus::createBr(testBlock);
		Nucleus::setInsertBlock(testBlock);
		r.enableContinue = restoreContinue;

		const Vector4f &src = fetchRegisterF(r, temporaryRegister);
		Int4 condition = As<Int4>(src.x);
		condition &= r.enableStack[r.enableIndex - 1];
		r.enableStack[r.enableIndex] = condition;

		Bool notAllFalse = SignMask(condition) != 0;
		branch(notAllFalse, loopBlock, endBlock);
		
		Nucleus::setInsertBlock(endBlock);
		r.enableBreak = restoreBreak;
		
		Nucleus::setInsertBlock(loopBlock);

		loopRepDepth++;
		breakDepth = 0;
	}

	void VertexProgram::RET(Registers &r)
	{
		if(currentLabel == -1)
		{
			returnBlock = Nucleus::createBasicBlock();
			Nucleus::createBr(returnBlock);
		}
		else
		{
			llvm::BasicBlock *unreachableBlock = Nucleus::createBasicBlock();

			if(callRetBlock[currentLabel].size() > 1)   // Pop the return destination from the call stack
			{
				// FIXME: Encapsulate
				UInt index = r.callStack[--r.stackIndex];
 
				llvm::Value *value = index.loadValue();
				llvm::Value *switchInst = Nucleus::createSwitch(value, unreachableBlock, (int)callRetBlock[currentLabel].size());

				for(unsigned int i = 0; i < callRetBlock[currentLabel].size(); i++)
				{
					Nucleus::addSwitchCase(switchInst, i, callRetBlock[currentLabel][i]);
				}
			}
			else if(callRetBlock[currentLabel].size() == 1)   // Jump directly to the unique return destination
			{
				Nucleus::createBr(callRetBlock[currentLabel][0]);
			}
			else   // Function isn't called
			{
				Nucleus::createBr(unreachableBlock);
			}

			Nucleus::setInsertBlock(unreachableBlock);
			Nucleus::createUnreachable();
		}
	}

	void VertexProgram::LEAVE(Registers &r)
	{
		r.enableLeave = r.enableLeave & ~r.enableStack[r.enableIndex];

		// FIXME: Return from function if all instances left
		// FIXME: Use enableLeave in other control-flow constructs
	}

	void VertexProgram::TEXLDL(Registers &r, Vector4f &dst, Vector4f &src0, const Src &src1)
	{
		Vector4f tmp;
		sampleTexture(r, tmp, src1, src0.x, src0.y, src0.z, src0.w);

		dst.x = tmp[(src1.swizzle >> 0) & 0x3];
		dst.y = tmp[(src1.swizzle >> 2) & 0x3];
		dst.z = tmp[(src1.swizzle >> 4) & 0x3];
		dst.w = tmp[(src1.swizzle >> 6) & 0x3];
	}

	void VertexProgram::TEX(Registers &r, Vector4f &dst, Vector4f &src0, const Src &src1)
	{
		Float4 lod = Float4(0.0f);
		Vector4f tmp;
		sampleTexture(r, tmp, src1, src0.x, src0.y, src0.z, lod);

		dst.x = tmp[(src1.swizzle >> 0) & 0x3];
		dst.y = tmp[(src1.swizzle >> 2) & 0x3];
		dst.z = tmp[(src1.swizzle >> 4) & 0x3];
		dst.w = tmp[(src1.swizzle >> 6) & 0x3];
	}

	void VertexProgram::TEXOFFSET(Registers &r, Vector4f &dst, Vector4f &src0, const Src& src1, Vector4f &src2, Vector4f &src3)
	{
		UNIMPLEMENTED();
	}

	void VertexProgram::TEXLDL(Registers &r, Vector4f &dst, Vector4f &src, const Src&, Vector4f &offset)
	{
		UNIMPLEMENTED();
	}

	void VertexProgram::TEXELFETCH(Registers &r, Vector4f &dst, Vector4f &src0, const Src& src1, Vector4f &src2)
	{
		UNIMPLEMENTED();
	}

	void VertexProgram::TEXELFETCH(Registers &r, Vector4f &dst, Vector4f &src0, const Src& src1, Vector4f &src2, Vector4f &offset)
	{
		UNIMPLEMENTED();
	}

	void VertexProgram::TEXGRAD(Registers &r, Vector4f &dst, Vector4f &src0, const Src& src1, Vector4f &src2, Vector4f &src3)
	{
		UNIMPLEMENTED();
	}

	void VertexProgram::TEXGRAD(Registers &r, Vector4f &dst, Vector4f &src0, const Src& src1, Vector4f &src2, Vector4f &src3, Vector4f &offset)
	{
		UNIMPLEMENTED();
	}

	void VertexProgram::TEXSIZE(Registers &r, Vector4f &dst, Float4 &lod, const Src &src1)
	{
		Pointer<Byte> textureMipmap = r.data + OFFSET(DrawData, mipmap[16]) + src1.index * sizeof(Texture) + OFFSET(Texture, mipmap);
		for(int i = 0; i < 4; ++i)
		{
			Pointer<Byte> mipmap = textureMipmap + (As<Int>(Extract(lod, i)) + Int(1)) * sizeof(Mipmap);
			dst.x = Insert(dst.x, As<Float>(Int(*Pointer<Short>(mipmap + OFFSET(Mipmap, width)))), i);
			dst.y = Insert(dst.y, As<Float>(Int(*Pointer<Short>(mipmap + OFFSET(Mipmap, height)))), i);
			dst.z = Insert(dst.z, As<Float>(Int(*Pointer<Short>(mipmap + OFFSET(Mipmap, depth)))), i);
		}
	}

	void VertexProgram::sampleTexture(Registers &r, Vector4f &c, const Src &s, Float4 &u, Float4 &v, Float4 &w, Float4 &q)
	{
		if(s.type == Shader::PARAMETER_SAMPLER && s.rel.type == Shader::PARAMETER_VOID)
		{
			Pointer<Byte> texture = r.data + OFFSET(DrawData,mipmap[16]) + s.index * sizeof(Texture);
			sampler[s.index]->sampleTexture(texture, c, u, v, w, q, r.a0, r.a0, false, false, true);	
		}
		else
		{
			Int index = As<Int>(Float(fetchRegisterF(r, s).x.x));

			for(int i = 0; i < 16; i++)
			{
				if(shader->usesSampler(i))
				{
					If(index == i)
					{
						Pointer<Byte> texture = r.data + OFFSET(DrawData,mipmap[16]) + i * sizeof(Texture);
						sampler[i]->sampleTexture(texture, c, u, v, w, q, r.a0, r.a0, false, false, true);
						// FIXME: When the sampler states are the same, we could use one sampler and just index the texture
					}
				}
			}
		}
	}
}
