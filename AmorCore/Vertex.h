#pragma once
#include "Common.h"
#include "OpenGl.h"

#include "ShaderFactory.h"

/**
* In open gl we need a VertexArrayObject
*/

// todo: add explicit layout options, to do this a complete vertex class will need to be bound
// to the Shader Factory because the attribLocation must be linked before the program is.

namespace amor {
	namespace graphics {
		namespace utils {
			namespace opengl {

				using vertex_info = std::tuple<std::string, GlPrimitive, void*, bool>;

				/// <summary>
				///  a class to represent a gl vertex type, this internally controls 
				/// the vertex attributes
				/// </summary>
				class VertexClass {
					friend class VertexClassFactory;
					friend class ShaderFactory;
				public:
					VertexClass();
					~VertexClass();

					void bind();
					void unbind();

					void attach_shader(const Shader& shader);

				private:
					void generate_gl_objects();

				private:
					u32 m_glVertexArrayObject;
					std::vector<vertex_info> m_vertexInfo;
					Shader m_Shader;
					std::unordered_map<std::string, u32> m_UniformRegistery;
				};

				/// <summary>
				/// A class to build a gl vertex type. Example:
				/// 
				/// struct vertex{
				///		float x, y, z;
				///		float u, v;
				///		float normalx, normaly, normalz;
				/// }
				/// 
				/// 
				/// void build(){
				///		VertexFactory vf{};
				/// 	VertexClass vClass{};
				/// 
				///		vf.target(vClass);
				///		vf.define_attr("position", GlPrimitive::Vec3, offsetof(vertex, x));
				///		vf.define_attr("uv", GlPrimitive::Vec2, offsetof(vertex, u));
				///		vf.define_attr("normal", GlPrimitive::Vec3, offsetof(vertex, normalx));
				///		
				///		vf.finish();
				/// 
				///		vClass.bind();
				/// }
				/// 
				/// </summary>
				class VertexClassFactory {
				public:
					VertexClassFactory();
					~VertexClassFactory();

					void define_attr(const std::string& name, GlPrimitive type, void* offset, bool normalized = false);
					void target(VertexClass& vertClass);
					void finish();
				private:
					VertexClass* m_vertClass = nullptr;
				};

			}
		}
	}
}
