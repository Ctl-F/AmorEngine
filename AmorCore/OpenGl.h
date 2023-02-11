#pragma once

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
					Bool,
					Bool2,
					Bool3,
					Bool4,
					Int2,
					Int3,
					Int4,
					Mat2,
					Mat23,
					Mat24,
					Mat32,
					Mat3,
					Mat34,
					Mat42,
					Mat43,
					Mat4,
				};


				inline i32 primitive_size(GlPrimitive primitive) {
					static i32 s_sizes[]{ 1, 1, 2, 3, 4, -1, 1, 2, 3, 4, 2, 3, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
					return s_sizes[(i32)primitive];
				}


				inline u32 primitive_gl_type(GlPrimitive primitive) {
					static u32 s_glTypes[]{ 0x1406, 0x1404, 0x1406, 0x1406, 0x1406, 0, 0x1400, 0x1400, 0x1400, 0x1400, 0x1404, 0x1404, 0x1404, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
					return s_glTypes[(i32)primitive];
				}
			}
		}
	}
}