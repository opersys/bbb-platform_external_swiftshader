// Copyright 2016 The SwiftShader Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef sw_PixelProgram_hpp
#define sw_PixelProgram_hpp

#include "PixelRoutine.hpp"
#include "SamplerCore.hpp"

namespace sw
{
	class PixelProgram : public PixelRoutine
	{
	public:
		PixelProgram(const PixelProcessor::State &state, const PixelShader *shader) :
			PixelRoutine(state, shader), r(shader && shader->dynamicallyIndexedTemporaries),
			loopDepth(-1), ifDepth(0), loopRepDepth(0), breakDepth(0), currentLabel(-1), whileTest(false)
		{
			for(int i = 0; i < 2048; ++i)
			{
				labelBlock[i] = 0;
			}

			enableStack[0] = Int4(0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF);

			if(shader && shader->containsBreakInstruction())
			{
				enableBreak = Int4(0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF);
			}

			if(shader && shader->containsContinueInstruction())
			{
				enableContinue = Int4(0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF);
			}
		}

		virtual ~PixelProgram() {}

	protected:
		virtual void setBuiltins(Int &x, Int &y, Float4(&z)[4], Float4 &w);
		virtual void applyShader(Int cMask[4]);
		virtual Bool alphaTest(Int cMask[4]);
		virtual void rasterOperation(Float4 &fog, Pointer<Byte> cBuffer[4], Int &x, Int sMask[4], Int zMask[4], Int cMask[4]);

	private:
		// Temporary registers
		RegisterArray<4096> r;

		// Color outputs
		Vector4f c[RENDERTARGETS];
		RegisterArray<RENDERTARGETS, true> oC;

		// Shader variables
		Vector4f vPos;
		Vector4f vFace;

		// DX9 specific variables
		Vector4f p0;
		Array<Int, 4> aL;
		Array<Int, 4> increment;
		Array<Int, 4> iteration;

		Int loopDepth;    // FIXME: Add support for switch
		Int stackIndex;   // FIXME: Inc/decrement callStack
		Array<UInt, 16> callStack;

		// Per pixel based on conditions reached
		Int enableIndex;
		Array<Int4, 1 + 24> enableStack;
		Int4 enableBreak;
		Int4 enableContinue;
		Int4 enableLeave;

		void sampleTexture(Vector4f &c, const Src &sampler, Vector4f &uvwq, Vector4f &dsx, Vector4f &dsy, Vector4f &offset, SamplerMethod method, unsigned int options);
		void sampleTexture(Vector4f &c, int samplerIndex, Vector4f &uvwq, Vector4f &dsx, Vector4f &dsy, Vector4f &offset, SamplerMethod method, unsigned int options);

		// Raster operations
		void clampColor(Vector4f oC[RENDERTARGETS]);

		Int4 enableMask(const Shader::Instruction *instruction);

		Vector4f fetchRegister(const Src &src, unsigned int offset = 0);
		Vector4f readConstant(const Src &src, unsigned int offset = 0);
		RValue<Pointer<Byte>> uniformAddress(int bufferIndex, unsigned int index);
		RValue<Pointer<Byte>> uniformAddress(int bufferIndex, unsigned int index, Int& offset);
		Int relativeAddress(const Shader::Parameter &var, int bufferIndex = -1);

		Float4 linearToSRGB(const Float4 &x);

		// Instructions
		typedef Shader::Control Control;

		void M3X2(Vector4f &dst, Vector4f &src0, const Src &src1);
		void M3X3(Vector4f &dst, Vector4f &src0, const Src &src1);
		void M3X4(Vector4f &dst, Vector4f &src0, const Src &src1);
		void M4X3(Vector4f &dst, Vector4f &src0, const Src &src1);
		void M4X4(Vector4f &dst, Vector4f &src0, const Src &src1);
		void TEXLD(Vector4f &dst, Vector4f &src0, const Src &src1, bool project, bool bias);
		void TEXLDD(Vector4f &dst, Vector4f &src0, const Src &src1, Vector4f &src2, Vector4f &src3, bool project);
		void TEXLDL(Vector4f &dst, Vector4f &src0, const Src &src1, bool project);
		void TEXSIZE(Vector4f &dst, Float4 &lod, const Src &src1);
		void TEXKILL(Int cMask[4], Vector4f &src, unsigned char mask);
		void TEXOFFSET(Vector4f &dst, Vector4f &src0, const Src &src1, Vector4f &src2, bool project, bool bias);
		void TEXLDL(Vector4f &dst, Vector4f &src0, const Src &src1, Vector4f &src2, bool project, bool bias);
		void TEXELFETCH(Vector4f &dst, Vector4f &src, const Src&);
		void TEXELFETCH(Vector4f &dst, Vector4f &src, const Src&, Vector4f &src2);
		void TEXGRAD(Vector4f &dst, Vector4f &src, const Src&, Vector4f &src2, Vector4f &src3);
		void TEXGRAD(Vector4f &dst, Vector4f &src, const Src&, Vector4f &src2, Vector4f &src3, Vector4f &src4);
		void DISCARD(Int cMask[4], const Shader::Instruction *instruction);
		void DFDX(Vector4f &dst, Vector4f &src);
		void DFDY(Vector4f &dst, Vector4f &src);
		void FWIDTH(Vector4f &dst, Vector4f &src);
		void BREAK();
		void BREAKC(Vector4f &src0, Vector4f &src1, Control);
		void BREAKP(const Src &predicateRegister);
		void BREAK(Int4 &condition);
		void CONTINUE();
		void TEST();
		void CALL(int labelIndex, int callSiteIndex);
		void CALLNZ(int labelIndex, int callSiteIndex, const Src &src);
		void CALLNZb(int labelIndex, int callSiteIndex, const Src &boolRegister);
		void CALLNZp(int labelIndex, int callSiteIndex, const Src &predicateRegister);
		void ELSE();
		void ENDIF();
		void ENDLOOP();
		void ENDREP();
		void ENDWHILE();
		void ENDSWITCH();
		void IF(const Src &src);
		void IFb(const Src &boolRegister);
		void IFp(const Src &predicateRegister);
		void IFC(Vector4f &src0, Vector4f &src1, Control);
		void IF(Int4 &condition);
		void LABEL(int labelIndex);
		void LOOP(const Src &integerRegister);
		void REP(const Src &integerRegister);
		void WHILE(const Src &temporaryRegister);
		void SWITCH();
		void RET();
		void LEAVE();

		int ifDepth;
		int loopRepDepth;
		int breakDepth;
		int currentLabel;
		bool whileTest;

		// FIXME: Get rid of llvm::
		llvm::BasicBlock *ifFalseBlock[24 + 24];
		llvm::BasicBlock *loopRepTestBlock[4];
		llvm::BasicBlock *loopRepEndBlock[4];
		llvm::BasicBlock *labelBlock[2048];
		std::vector<llvm::BasicBlock*> callRetBlock[2048];
		llvm::BasicBlock *returnBlock;
		bool isConditionalIf[24 + 24];
	};
}

#endif
