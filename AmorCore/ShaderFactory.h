#pragma once
#include "Common.h"
#include <memory>
#include "OpenGl.h"
#include "Util.h"

namespace amor {
	namespace graphics {
		namespace utils {
			namespace opengl {
				
				class VertexClass;

				void shader_deallocator(u32& program_id);

				constexpr i32 AUTO_LAYOUT = -1;
				constexpr i32 NO_LAYOUT = -2;

				class Shader {
					friend class ShaderFactory;
				public:
					Shader();
					Shader(const Shader& other);
					Shader(Shader&& other) noexcept;
					~Shader();

					Shader& operator=(const Shader& other);
					Shader& operator=(Shader&& other) noexcept;

					void bind() const;
					void unbind() const;
					
					bool good() const;
					u32& internal_id();

				protected:
					Shader(u32 program_id);

				private:
					util::CountedRef<u32, shader_deallocator> m_glProgram;
				};

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

					void BindVertexClass(VertexClass* _class, bool explicitLayout = true);
					void InsertFromSource(const char* filename);

					std::string GetVertex() const;
					std::string GetFragment() const;

					Shader CompileGlProgram() const;

				protected:
					const std::string& PrimitiveString(GlPrimitive primitive) const;

				private:
					std::stringstream m_Shaderbuffer;
					std::string m_VertexShader, m_FragmentShader;
					i32 m_LocCounter;
					bool m_MainIsOpen, m_MainIsClosed;
					bool m_VersionIsSpecified;
					VertexClass* m_boundClass = nullptr;
					bool m_UseExplicitLayout = true;
				};
			}
		}
	}
}

