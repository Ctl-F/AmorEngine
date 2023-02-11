#include "pch.h"
#include "Vertex.h"

#include <glad/glad.h>
#include <tuple>




namespace amor {
	namespace graphics {
		namespace utils {
			namespace opengl {

				template<typename t> u32 stride_t(t i) { 0u; }
				template<> u32 stride_t(vertex_info& info) {
					u32 gl_type = primitive_gl_type(std::get<1>(info));
					u32 cpp_type = 0;
					switch (gl_type) {
					case 0x1406: cpp_type = sizeof(float); break;
					case 0x1404: cpp_type = sizeof(i32); break;
					case 0x1400: cpp_type = sizeof(byte); break;
					}
					
					u32 count = primitive_size(std::get<1>(info));
					return cpp_type * count;
				}
				template<> u32 stride_t(std::vector<vertex_info>& vertex_class) {
					u32 stride = 0;

					for (size_t i = 0; i < vertex_class.size(); i++) {
						stride += stride_t(vertex_class[i]);
					}

					return stride;
				}


				VertexClass::VertexClass() : m_glVertexArrayObject{ 0 }, m_Shader{} {

				}
				VertexClass::~VertexClass() {
					// TODO: make safe
					
					unbind();
					glDeleteVertexArrays(1, &m_glVertexArrayObject);
				}

				void VertexClass::bind() {
					glBindVertexArray(m_glVertexArrayObject);

					if (m_Shader.good()) {
						m_Shader.bind();
					}
				}
				void VertexClass::unbind() {
					if (m_Shader.good()) {
						m_Shader.unbind();
					}

					glBindVertexArray(0);
				}

				void VertexClass::generate_gl_objects() {
					glGenVertexArrays(1, &m_glVertexArrayObject);
					
					i32 strideSize = stride_t(m_vertexInfo);

					for (size_t i = 0; i < m_vertexInfo.size(); i++) {
						glVertexAttribPointer(i,
							primitive_size(std::get<1>(m_vertexInfo[i])), // count
							primitive_gl_type(std::get<1>(m_vertexInfo[i])), // type
							std::get<3>(m_vertexInfo[i]), // normalized
							strideSize, // stride
							std::get<2>(m_vertexInfo[i])); // void* offset

						glEnableVertexAttribArray(i);
					}
				}

				void VertexClass::attach_shader(const Shader& shader) {
					m_Shader = shader;

					for (size_t i = 0; i < m_vertexInfo.size(); i++) {
						std::string uniform_name = std::get<0>(m_vertexInfo[i]);

						m_UniformRegistery[uniform_name] = 0u;
					}

				}

				VertexClassFactory::VertexClassFactory() : m_vertClass{nullptr}{}
				VertexClassFactory::~VertexClassFactory() {}
				
				void VertexClassFactory::define_attr(const std::string& name, GlPrimitive type, void* offset, bool normalized) {
					if (m_vertClass == nullptr) {
						logging::GetInstance()->error("Attempt to define a vertex attribute without a target", "debug.VertexClassFactory");
						return;
					}

					m_vertClass->m_vertexInfo.push_back({ name, type, offset, normalized });
				}

				void VertexClassFactory::target(VertexClass& vertClass) {
					if (m_vertClass != nullptr) {
						finish();
					}
					m_vertClass = &vertClass;
				}
				void VertexClassFactory::finish() {
					if (m_vertClass == nullptr) return;

					m_vertClass->generate_gl_objects();
				}
			}
		}
	}
}
