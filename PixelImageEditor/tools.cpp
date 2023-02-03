#include "tools.h"

#include <imgui.h>

void Tool::render_tool_info(ImageEditor& editor, graphics::PixelRenderer& renderer) { }
void Tool::start(ImageEditor& editor) {}
void Tool::end(ImageEditor& editor) {}




void Brush::update(ImageEditor& editor, input::Input& input) {
	math::Vec3f cell = editor.get_pixel();
	graphics::PrimitiveContext2D ctx = editor.get_tex()->GetContext();
	graphics::Color color;

	if (cell.z == 0 && editor.in_mask((i32)cell.x, (i32)cell.y)) {
        bool do_draw = false;

		if (input.mouse_check_pressed(input::MouseButton::Left) && (action_detail == input::MouseButton::FIMKEY || action_detail == input::MouseButton::Left)) {
			if (input.mouse_check_just_pressed(input::MouseButton::Left)) {
				action_just_started = false;
				action_detail = input::MouseButton::Left;
			}

            do_draw = true;
			color = graphics::Color::from_rgb_vec({ editor.m_PrimaryRgbColor[0],  editor.m_PrimaryRgbColor[1],  editor.m_PrimaryRgbColor[2] });
		}
		else if (input.mouse_check_pressed(input::MouseButton::Right) && (action_detail == input::MouseButton::FIMKEY || action_detail == input::MouseButton::Right)) {
			if (input.mouse_check_just_pressed(input::MouseButton::Right)) {
				action_just_started = false;
				action_detail = input::MouseButton::Right;
			}

            do_draw = true;
			color = graphics::Color::from_rgb_vec({ editor.m_SecondaryRgbColor[0],  editor.m_SecondaryRgbColor[1],  editor.m_SecondaryRgbColor[2] });
		}

        if (do_draw) {
            if (m_brushSize == 1) {
                ctx.Draw((i32)cell.x, (i32)cell.y, color);
            }
            else if (m_brushSize == 2) {
                ctx.Draw((i32)cell.x, (i32)cell.y, color);

                if (editor.in_mask((i32)cell.x + 1, (i32)cell.y)) ctx.Draw((i32)cell.x + 1, (i32)cell.y, color);
                if (editor.in_mask((i32)cell.x, (i32)cell.y + 1)) ctx.Draw((i32)cell.x, (i32)cell.y + 1, color);
                if (editor.in_mask((i32)cell.x + 1, (i32)cell.y + 1)) ctx.Draw((i32)cell.x + 1, (i32)cell.y + 1, color);
            }
            else {
                i32 size = m_brushSize - 2;
                for (i32 ioff = -size; ioff <= size; ioff++) {
                    for (i32 joff = -size; joff <= size; joff++) {
                        if (m_isCircle && (ioff * ioff + joff * joff) > size * size) continue;
                        if (!editor.in_mask((i32)cell.x + ioff, (i32)cell.y + joff)) continue;

                        if (m_modulate && (((ioff * ioff) + (joff * joff)) & m_modulo) % m_modulo != 0) continue;

                        ctx.Draw((i32)cell.x + ioff, (i32)cell.y + joff, color);
                    }
                }
            }
        }
	}

	if (input.mouse_check_just_released(action_detail)) {
		action_just_started = true;
        action_detail = input::MouseButton::FIMKEY;
		editor.push_undo_step();
	}

    m_brushSize += input.key_check_just_pressed(input::Key::RightBracket) - input.key_check_just_pressed(input::Key::LeftBracket);
    m_brushSize = math::clamp(m_brushSize, 1, 10);
}

void Brush::render_tool_info(ImageEditor& editor, graphics::PixelRenderer& renderer) {
    math::Vec3f cell = editor.get_pixel();
    graphics::Color color{ 10, 10, 10, 10 };
    if (cell.z == 0.0f) {
        auto draw = [&](i32 x, i32 y) {
            auto zoom = editor.get_zoom();
            auto origin = editor.calculate_image_location().origin();
            x = origin.x + (x * zoom);
            y = origin.y + (y * zoom);

            if (editor.get_zoom() < 3) {
                renderer.Draw(x, y, color);
            }
            else {
                renderer.FillRect(x, y, editor.get_zoom(), editor.get_zoom(), color);
            }
        };

        renderer.SetBlending(graphics::BlendMode::Normal);

        if (m_brushSize == 1) {
            draw((i32)cell.x, (i32)cell.y);
        }
        else if (m_brushSize == 2) {
            draw((i32)cell.x, (i32)cell.y);
            draw((i32)cell.x + 1, (i32)cell.y);
            draw((i32)cell.x, (i32)cell.y + 1);
            draw((i32)cell.x + 1, (i32)cell.y + 1);
        }
        else {
            i32 size = m_brushSize - 2;
            for (i32 ioff = -size; ioff <= size; ioff++) {
                for (i32 joff = -size; joff <= size; joff++) {
                    if (m_isCircle && (ioff * ioff + joff * joff) > size * size) continue;
                    if (m_modulate && (((ioff * ioff) + (joff * joff)) & m_modulo) % m_modulo != 0) continue;

                    draw((i32)cell.x + ioff, (i32)cell.y + joff);
                }
            }
        }

        renderer.SetBlending(graphics::BlendMode::None);
    }
}

void Brush::render_tool_options(ImageEditor& editor) {
	ImGui::Begin("Brush");

	ImGui::ColorPicker3("Primary Color", editor.m_PrimaryRgbColor);
	ImGui::ColorPicker3("Secondary Color", editor.m_SecondaryRgbColor);

    ImGui::SliderInt("Brush Size", &m_brushSize, 1, 10);
    ImGui::Checkbox("Circle Brush", &m_isCircle);

    ImGui::Checkbox("Holes", &m_modulate);
    if (m_modulate) {
        ImGui::InputInt("Factor", &m_modulo);
    }

	ImGui::End();
}

const char* Brush::get_name() {
	return "Brush";
}

void FloodFill::update(ImageEditor& editor, input::Input& input) {

	math::Vec3f cell = editor.get_pixel();
	graphics::PrimitiveContext2D ctx = editor.get_tex()->GetContext();

	bool doAction = false;
	graphics::Color color;
	graphics::Color baseColor = ctx.Get((i32)cell.x, (i32)cell.y);
	if (cell.z == 0 && editor.in_mask((i32)cell.x, (i32)cell.y)) {
		if (input.mouse_check_just_released(input::MouseButton::Left)) {
			color = graphics::Color::from_rgb_vec({ editor.m_PrimaryRgbColor[0],  editor.m_PrimaryRgbColor[1],  editor.m_PrimaryRgbColor[2] });
			doAction = true;
		}
		else if (input.mouse_check_just_released(input::MouseButton::Right)) {
			color = graphics::Color::from_rgb_vec({ editor.m_SecondaryRgbColor[0],  editor.m_SecondaryRgbColor[1],  editor.m_SecondaryRgbColor[2] });
			doAction = true;
		}
	}

	if (doAction) {
		editor.push_undo_step();
		flood_fill(editor, ctx, (i32)cell.x, (i32)cell.y, color, baseColor);
	}
}

void FloodFill::render_tool_options(ImageEditor& editor) {
	ImGui::Begin("Bucket Fill");

	ImGui::ColorPicker3("Primary Color", editor.m_PrimaryRgbColor);
	ImGui::ColorPicker3("Secondary Color", editor.m_SecondaryRgbColor);
    ImGui::Checkbox("Fun Mode", &m_FunMode);

    if (m_FunMode) {
        ImGui::SliderInt("Range", &m_FunModeRange, 1, 50);
    }
	ImGui::End();
}

const char* FloodFill::get_name() {
	return "Bucket Fill";
}

void FloodFill::flood_fill(ImageEditor& editor, graphics::PrimitiveContext2D& ctx, i32 x, i32 y, const graphics::Color& color, const graphics::Color& baseColor)
{
	if (color == baseColor) return;
	if (ctx.Get(x, y) != baseColor) return;
    if (!editor.in_mask(x, y)) return;

    if (m_FunMode) {
        signed char offset = (rand() % (m_FunModeRange * 2)) - m_FunModeRange;
        int r = (color.r + offset);
        int g = (color.g + offset);
        int b = (color.b + offset);
        r = math::clamp(r, 0, 255);
        g = math::clamp(g, 0, 255);
        b = math::clamp(b, 0, 255);

        ctx.Draw(x, y, { (byte)r, (byte)g, (byte)b, 255 });
    }
    else {
        ctx.Draw(x, y, color);
    }

	if (x - 1 >= 0) flood_fill(editor, ctx, x - 1, y, color, baseColor);
	if (x + 1 < ctx.width()) flood_fill(editor, ctx, x + 1, y, color, baseColor);
	if (y - 1 >= 0) flood_fill(editor, ctx, x, y - 1, color, baseColor);
	if (y + 1 < ctx.height()) flood_fill(editor, ctx, x, y + 1, color, baseColor);
}


void EyeDropper::update(ImageEditor& editor, amor::input::Input& input)
{
    graphics::PrimitiveContext2D ctx = editor.get_tex()->GetContext();
    math::Vec3f cell = editor.get_pixel();

    if (cell.z == 0) {
        float* target = nullptr;
        
        if (input.mouse_check_pressed(input::MouseButton::Left)) {
            target = editor.m_PrimaryRgbColor;
            
        }
        else if (input.mouse_check_pressed(input::MouseButton::Right)) {
            target = editor.m_SecondaryRgbColor;
        }

        if (target != nullptr) {
            graphics::Color color = ctx.Get((i32)cell.x, (i32)cell.y);

            target[0] = (float)(color.r) / 255.0;
            target[1] = (float)(color.g) / 255.0;
            target[2] = (float)(color.b) / 255.0;
        }
    }
}

void EyeDropper::render_tool_options(ImageEditor& editor)
{
    ImGui::Begin("Eyedropper");

    ImGui::ColorEdit3("Primary Color", editor.m_PrimaryRgbColor);
    ImGui::ColorEdit3("Secondary Color", editor.m_SecondaryRgbColor);

    ImGui::End();
}

const char* EyeDropper::get_name()
{
    return "Eyedropper";
}



void Blend::update(ImageEditor& editor, input::Input& input) {
    if (m_blendRadius <= 0) m_blendRadius = 1;
    if (m_blendRadius > 10) m_blendRadius = 10;
    if (m_blendStrength <= 0.0f) m_blendRadius = 0.01f;
    if (m_blendStrength >= 1.0f) m_blendRadius = 1.0f;

    graphics::PrimitiveContext2D ctx = editor.get_tex()->GetContext();
    math::Vec3f cell = editor.get_pixel();

    if (cell.z == 0 && editor.in_mask((i32)cell.x, (i32)cell.y)) {
        if (input.mouse_check_just_pressed(input::MouseButton::Left)){
            editor.push_undo_step();
        }
        if (input.mouse_check_just_released(input::MouseButton::Left)) {
            editor.push_undo_step();
        }

        if (input.mouse_check_pressed(input::MouseButton::Left) && cell != m_pMouse) {
            m_Matrix.apply(editor, (i32)cell.x, (i32)cell.y, ctx, m_wrap);
            m_pMouse = cell;
        }

    }

}

void Blend::render_tool_options(ImageEditor& editor) {
    ImGui::Begin("Smudge Settings");

 /*   ImGui::InputInt("Radius", &m_blendRadius);
    ImGui::InputFloat("Strength", &m_blendStrength);*/

    ImGui::Text("Edge Policy"); ImGui::SameLine();
    ImGui::RadioButton("Clamp", &m_wrap, 0); ImGui::SameLine();
    ImGui::RadioButton("Wrap", &m_wrap, 1);

    ImGui::End();
}

const char* Blend::get_name() {
    return "Blend Tool";
}


BlendMatrix::BlendMatrix(BlendSize size) : strength(nullptr) {
    this->size = size;
    
    switch (size) {
    case BlendSize::x33: {
        strength = new float[9];

        strength[4] = 0.5f;
        strength[3] = 0.1f;
        strength[5] = 0.1f;
        strength[1] = 0.1f;
        strength[7] = 0.1f;
        strength[0] = 0.025f;
        strength[2] = 0.025f;
        strength[6] = 0.025f;
        strength[8] = 0.025f;
        break;
    }
    }

}

void BlendMatrix::apply(ImageEditor& editor, i32 x, i32 y, graphics::PrimitiveContext2D& ctx, bool wrap) {
    u32 index = 0;

    math::Vec3f color{ 0 };
    math::Vec3f core_color = ctx.Get(x, y).to_rgb_vec();
    for (i32 yoff = -1; yoff <= 1; yoff++) {
        for (i32 xoff = -1; xoff <= 1; xoff++) {
            if (wrap) {
                i32 xx, yy;
                xx = x + xoff;
                yy = y + yoff;

                if (xx < 0) { xx = ctx.width() - 1; }
                if (xx >= ctx.width()) { xx = 0; }
                if (yy < 0) { yy = ctx.height() - 1; }
                if (yy > ctx.height()) { yy = 0; }

                if (!editor.in_mask(xx, yy)) {
                    color += core_color * strength[index];
                }
                else {
                    color += ctx.Get(xx, yy).to_rgb_vec() * strength[index];
                }
            }
            else {
                if ((x + xoff < 0) || (x + xoff >= ctx.width()) || (y + yoff < 0) || (y + yoff >= ctx.height()) || !editor.in_mask(x + xoff, y + yoff)) {
                    color += core_color * strength[index];
                }
                else {
                    color += ctx.Get(x + xoff, y + yoff).to_rgb_vec() * strength[index];
                }
            }
            index++;
        }
    }
    ctx.Draw(x, y, graphics::Color::from_rgb_vec(color));
}

BlendMatrix::~BlendMatrix() {
    if (strength != nullptr) {
        delete[] strength;
    }
}


void MaskBrush::start(ImageEditor& editor) {
    bool& show = editor.show_mask_setting();
    m_wasShowMask = show;
    show = true;
}
void MaskBrush::end(ImageEditor& editor) {
    editor.show_mask_setting() = m_wasShowMask;
}

void MaskBrush::update(ImageEditor& editor, input::Input& input) {
	math::Vec3f cell = editor.get_pixel();
	graphics::PrimitiveContext2D ctx = editor.get_mask()->GetContext();
    graphics::Color color;
	if (cell.z == 0) {
        bool do_action = false;

		if (input.mouse_check_pressed(input::MouseButton::Left)) {
         //   ctx.Draw((i32)cell.x, (i32)cell.y, { 255, 255, 255, 64 });
            color = { 255, 255, 255, 64 };
            do_action = true;
        }
		else if (input.mouse_check_pressed(input::MouseButton::Right)) {
		//	ctx.Draw((i32)cell.x, (i32)cell.y, { 0, 0, 0, 64 });
            color = { 0, 0, 0, 64 };
            do_action = true;
        }

        if (do_action) {
            if (m_brushSize == 1) {
                ctx.Draw((i32)cell.x, (i32)cell.y, color);
            }
            else if (m_brushSize == 2) {
                ctx.Draw((i32)cell.x, (i32)cell.y, color);
                ctx.Draw((i32)cell.x + 1, (i32)cell.y, color);
                ctx.Draw((i32)cell.x, (i32)cell.y + 1, color);
                ctx.Draw((i32)cell.x + 1, (i32)cell.y + 1, color);
            }
            else {
                i32 size = m_brushSize - 2;
                for (i32 ioff = -size; ioff <= size; ioff++) {
                    for (i32 joff = -size; joff <= size; joff++) {
                        if (m_isCircle && (ioff * ioff + joff * joff) > size * size) continue;
                        ctx.Draw((i32)cell.x + ioff, (i32)cell.y + joff, color);
                    }
                }
            }
        }
	}

    m_brushSize += input.key_check_just_pressed(input::Key::RightBracket) - input.key_check_just_pressed(input::Key::LeftBracket);
    m_brushSize = math::clamp(m_brushSize, 1, 10);
}

void MaskBrush::render_tool_info(ImageEditor& editor, graphics::PixelRenderer& renderer) {
    math::Vec3f cell = editor.get_pixel();
    graphics::Color color{ 10, 100, 100, 10 };
    if (cell.z == 0.0f) {
        auto draw = [&](i32 x, i32 y) {
            auto zoom = editor.get_zoom();
            auto origin = editor.calculate_image_location().origin();
            x = origin.x + (x * zoom);
            y = origin.y + (y * zoom);

            if (editor.get_zoom() < 3) {
                renderer.Draw(x, y, color);
            }
            else {
                renderer.FillRect(x, y, editor.get_zoom(), editor.get_zoom(), color);
            }
        };

        renderer.SetBlending(graphics::BlendMode::Normal);

        if (m_brushSize == 1) {
            draw((i32)cell.x, (i32)cell.y);
        }
        else if (m_brushSize == 2) {
            draw((i32)cell.x, (i32)cell.y);
            draw((i32)cell.x + 1, (i32)cell.y);
            draw((i32)cell.x, (i32)cell.y + 1);
            draw((i32)cell.x + 1, (i32)cell.y + 1);
        }
        else {
            i32 size = m_brushSize - 2;
            for (i32 ioff = -size; ioff <= size; ioff++) {
                for (i32 joff = -size; joff <= size; joff++) {
                    if (m_isCircle && (ioff * ioff + joff * joff) > size * size) continue;
                    draw((i32)cell.x + ioff, (i32)cell.y + joff);
                }
            }
        }

        renderer.SetBlending(graphics::BlendMode::None);
    }
}

void MaskBrush::render_tool_options(ImageEditor& editor) {
	ImGui::Begin("Mask Brush");

    if (ImGui::Button("Clear Mask (Include)")) {
        editor.get_mask()->GetContext().Clear({ 255, 255, 255, 64 });
    }

    if (ImGui::Button("Clear Mask (Exclude)")) {
        editor.get_mask()->GetContext().Clear({ 0, 0, 0, 64 });
    }

    if (ImGui::Button("Invert Mask")) {
        invert(*editor.get_mask());
    }

    ImGui::SliderInt("Brush Size", &m_brushSize, 1, 10);
    ImGui::Checkbox("Circle Brush", &m_isCircle);

	ImGui::End();
}

void MaskBrush::invert(graphics::Texture& texture) {
    u64 size = texture.width() * texture.height();
    graphics::Color* data = texture.data();
    for (u64 i = 0; i < size; i++) {
        if (data[i].r == 255) {
            data[i] = { 0, 0, 0, 64 };
        }
        else {
            data[i] = { 255, 255, 255, 64 };
        }
    }
}

const char* MaskBrush::get_name() {
	return "Mask Brush";
}