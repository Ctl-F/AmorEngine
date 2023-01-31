#pragma once
#include "Common.h"

namespace amor {
	namespace graphics {
		namespace utils {
			namespace opengl {
				enum class GlPrimitive {
					Float,
					Int,
					Vec2,
					Vec3,
					Vec4,
					Sampler2D,
				};

				constexpr i32 AUTO_LAYOUT = -1;
				constexpr i32 NO_LAYOUT = -2;

				class ShaderFactory
				{
				public:
					ShaderFactory();
					~ShaderFactory();

					void Version(const char* glslVersion);
					void InParam(const char* name, GlPrimitive type, i32 location = AUTO_LAYOUT);
					void OutParam(const char* name, GlPrimitive type);
					void UniformParam(const char* name, GlPrimitive type);

					void Main();
					void EndMain();

					void RawStatement(const char* string);

					void WriteVertex();
					void WriteFragment();


					std::string GetVertex() const;
					std::string GetFragment() const;

					u32 CompileGlProgram() const;

				protected:
					const std::string& PrimitiveString(GlPrimitive primitive) const;

				private:
					std::stringstream m_Shaderbuffer;
					std::string m_VertexShader, m_FragmentShader;
					i32 m_LocCounter;
					bool m_MainIsOpen, m_MainIsClosed;
					bool m_VersionIsSpecified;
				};
			}
		}
	}
}

