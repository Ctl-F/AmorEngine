#include "pch.h"
#include "ShaderFactory.h"
#include "Vertex.h"

#include <glad/glad.h>

namespace amor {
	namespace graphics {
		namespace utils {
			namespace opengl {
				void shader_deallocator(u32& program_id) {
					glDeleteProgram(program_id);
					program_id = 0u;
				}

				Shader::Shader() : m_glProgram{ 0 } {}
				Shader::Shader(u32 programID) : m_glProgram{ programID } {}
				Shader::Shader(const Shader& other) : m_glProgram{ other.m_glProgram } { }
				Shader::Shader(Shader&& other) noexcept : m_glProgram{ std::move(other.m_glProgram) } { }
				Shader::~Shader() {

				}

				Shader& Shader::operator=(const Shader& other) {
					m_glProgram = other.m_glProgram;
					return *this;
				}
				Shader& Shader::operator=(Shader&& other) noexcept {
					m_glProgram = std::move(other.m_glProgram);
					return *this;
				}

				void Shader::bind() const {
					glUseProgram(*m_glProgram);
				}
				void Shader::unbind() const {
					glUseProgram(0u);
				}

				bool Shader::good() const {
					return *m_glProgram != 0;
				}

				u32& Shader::internal_id() {
					return *m_glProgram;
				}


				ShaderFactory::ShaderFactory() {
					m_VertexShader = "";
					m_FragmentShader = "";
					m_LocCounter = 0;

					m_MainIsOpen = false;
					m_MainIsClosed = false;
					m_VersionIsSpecified = false;
				}

				ShaderFactory::~ShaderFactory() {

				}


				const std::string& ShaderFactory::PrimitiveString(GlPrimitive primitive) const {
					static std::string s_Types[]{
						"float", "int", "vec2", "vec3", "vec4", "sampler2D",
							"bool", "bvec2", "bvec3","bvec4",
							"ivec2", "ivec3", "ivec4",
							"mat2", "mat2x3", "mat2x4",
							"mat3x2", "mat3", "mat3x4",
							"mat4x2", "mat4x3", "mat4",
					};

					return s_Types[(int)primitive];
				}


				void ShaderFactory::Version(const char* glslVersion) {
					m_Shaderbuffer << "#version " << glslVersion << "\n";
					m_VersionIsSpecified = true;
				}

				void ShaderFactory::InParam(const char* name, GlPrimitive type, int location) {
					if (location != NO_LAYOUT) {
						m_Shaderbuffer << "layout (location = ";

						int loc = location;
						if (location == AUTO_LAYOUT) {
							loc = m_LocCounter;
							m_LocCounter++;
						}

						m_Shaderbuffer << loc << ")";
					}
					m_Shaderbuffer << "in " << PrimitiveString(type) << " " << name << ";\n";
				}

				void ShaderFactory::OutParam(const char* name, GlPrimitive type) {
					m_Shaderbuffer << "out " << PrimitiveString(type) << " " << name << ";\n";
				}

				void ShaderFactory::UniformParam(const char* name, GlPrimitive type) {
					m_Shaderbuffer << "uniform " << PrimitiveString(type) << " " << name << ";\n";
				}

				void ShaderFactory::Main() {
					m_MainIsOpen = true;
					m_Shaderbuffer << "void main(){\n";
				}
				void ShaderFactory::EndMain() {
					m_MainIsClosed = true;
					m_Shaderbuffer << "}\n";
				}

				void ShaderFactory::RawStatement(const char* string) {
					m_Shaderbuffer << string;
				}

				void ShaderFactory::InsertFromSource(const char* filename) {
					if (!std::filesystem::exists(filename)) {
						logging::GetInstance()->error("File '" + std::string(filename) + "' does not exist");
						return;
					}

					std::ifstream file(filename);
					std::string line;
					if (file.is_open()) {
						while (file.good()) {
							std::getline(file, line);
							m_Shaderbuffer << line << "\n";
						}
						file.close();
					}
				}

				void ShaderFactory::BindVertexClass(VertexClass* _class, bool explicitLayout) {
					m_boundClass = _class;
					m_UseExplicitLayout = explicitLayout;
				}

				void ShaderFactory::WriteVertex() {
					if (!m_VersionIsSpecified) {
						logging::GetInstance()->warn("Shader version does not appear to be specified for vertex shader.", "GLSL");
					}
					if (!m_MainIsOpen) {
						logging::GetInstance()->warn("Main function does not appear to be specified for vertex shader.", "GLSL");
					}
					else if (!m_MainIsClosed) {
						logging::GetInstance()->warn("Main function does not appear to be closed for vertex shader.", "GLSL");
					}

					m_VertexShader = m_Shaderbuffer.str();
					m_Shaderbuffer.str("");

					m_MainIsOpen = false;
					m_MainIsClosed = false;
					m_VersionIsSpecified = false;
					m_LocCounter = 0;
				}

				void ShaderFactory::WriteFragment() {

					if (!m_VersionIsSpecified) {
						logging::GetInstance()->warn("Shader version does not appear to be specified for fragment shader.", "GLSL");
					}
					if (!m_MainIsOpen) {
						logging::GetInstance()->warn("Main function does not appear to be specified for fragment shader.", "GLSL");
					}
					else if (!m_MainIsClosed) {
						logging::GetInstance()->warn("Main function does not appear to be closed for fragment shader.", "GLSL");
					}

					m_FragmentShader = m_Shaderbuffer.str();
					m_Shaderbuffer.str("");

					m_MainIsOpen = false;
					m_MainIsClosed = false;
					m_VersionIsSpecified = false;
					m_LocCounter = 0;
				}


				Shader ShaderFactory::CompileGlProgram() const {
					constexpr i32 BUFFER_SIZE = 1024;
					u32 vID, fID, pID;
					const i8* sourcePtr = m_VertexShader.c_str();
					i32 success;
					i8 errorBuffer[BUFFER_SIZE] = { 0 };

					vID = glCreateShader(GL_VERTEX_SHADER);
					glShaderSource(vID, 1, &sourcePtr, NULL);
					glCompileShader(vID);

					glGetShaderiv(vID, GL_COMPILE_STATUS, &success);
					if (!success) {
						glGetShaderInfoLog(vID, BUFFER_SIZE, NULL, errorBuffer);
						logging::GetInstance()->error(errorBuffer, "GLSL.Vertex");
						return 0;
					}

					sourcePtr = m_FragmentShader.c_str();
					fID = glCreateShader(GL_FRAGMENT_SHADER);
					glShaderSource(fID, 1, &sourcePtr, NULL);
					glCompileShader(fID);

					glGetShaderiv(fID, GL_COMPILE_STATUS, &success);
					if (!success) {
						glGetShaderInfoLog(fID, BUFFER_SIZE, NULL, errorBuffer);
						logging::GetInstance()->error(errorBuffer, "GLSL.Fragment");
						glDeleteShader(vID);
						return 0;
					}

					pID = glCreateProgram();
					glAttachShader(pID, vID);
					glAttachShader(pID, fID);

					if (!m_UseExplicitLayout && m_boundClass != nullptr) {
						for (size_t i = 0; i < m_boundClass->m_vertexInfo.size(); i++) {
							glBindAttribLocation(pID, i,
								std::get<0>(m_boundClass->m_vertexInfo[i]).c_str());
						}
					}

					
					glLinkProgram(pID);

					glDeleteShader(vID);
					glDeleteShader(fID);

					glGetProgramiv(pID, GL_LINK_STATUS, &success);
					if (!success) {
						glGetProgramInfoLog(pID, BUFFER_SIZE, NULL, errorBuffer);
						logging::GetInstance()->error(errorBuffer, "GLSL.Linker");
						return 0;
					}

					glUseProgram(pID);
					return pID;
				}
			}
		}
	}
}