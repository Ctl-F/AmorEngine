#include "tools.h"

#include <imgui.h>

void Brush::update(ImageEditor& editor, input::Input& input) {
	math::Vec3f cell = editor.get_pixel();
	graphics::PrimitiveContext2D ctx = editor.get_tex()->GetContext();
	graphics::Color color;

	if (cell.z == 0 && editor.in_mask((i32)cell.x, (i32)cell.y)) {
		if (input.mouse_check_pressed(input::MouseButton::Left) && (action_detail == input::MouseButton::FIMKEY || action_detail == input::MouseButton::Left)) {
			if (input.mouse_check_just_pressed(input::MouseButton::Left)) {
				action_just_started = false;
				action_detail = input::MouseButton::Left;
			}

			color = graphics::Color::from_rgb_vec({ editor.m_PrimaryRgbColor[0],  editor.m_PrimaryRgbColor[1],  editor.m_PrimaryRgbColor[2] });

            if (m_brushSize == 1) {
                ctx.Draw((i32)cell.x, (i32)cell.y, color);
            }
            else if (m_brushSize == 2) {
                ctx.Draw((i32)cell.x, (i32)cell.y, color);

                if(editor.in_mask((i32)cell.x + 1, (i32) cell.y)) ctx.Draw((i32)cell.x+1, (i32)cell.y, color);
                if (editor.in_mask((i32)cell.x, (i32)cell.y + 1)) ctx.Draw((i32)cell.x, (i32)cell.y+1, color);
                if (editor.in_mask((i32)cell.x + 1, (i32)cell.y + 1)) ctx.Draw((i32)cell.x+1, (i32)cell.y+1, color);
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
		else if (input.mouse_check_pressed(input::MouseButton::Right) && (action_detail == input::MouseButton::FIMKEY || action_detail == input::MouseButton::Right)) {
			if (input.mouse_check_just_pressed(input::MouseButton::Right)) {
				action_just_started = false;
				action_detail = input::MouseButton::Right;
			}

			color = graphics::Color::from_rgb_vec({ editor.m_SecondaryRgbColor[0],  editor.m_SecondaryRgbColor[1],  editor.m_SecondaryRgbColor[2] });

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
            m_Matrix.apply(editor, (i32)cell.x, (i32)cell.y, ctx);
            m_pMouse = cell;
        }

    }

}

void Blend::render_tool_options(ImageEditor& editor) {
    ImGui::Begin("Smudge Settings");

    ImGui::InputInt("Radius", &m_blendRadius);
    ImGui::InputFloat("Strength", &m_blendStrength);

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

void BlendMatrix::apply(ImageEditor& editor, i32 x, i32 y, graphics::PrimitiveContext2D& ctx) {
    u32 index = 0;

    math::Vec3f color{ 0 };
    math::Vec3f core_color = ctx.Get(x, y).to_rgb_vec();
    for (i32 yoff = -1; yoff <= 1; yoff++) {
        for (i32 xoff = -1; xoff <= 1; xoff++) {
            if ((x + xoff < 0) || (x + xoff >= ctx.width()) || (y + yoff < 0) || (y + yoff >= ctx.height()) || !editor.in_mask(x + xoff, y + yoff)) {
                color += core_color * strength[index];
            }
            else {
                color += ctx.Get(x + xoff, y + yoff).to_rgb_vec() * strength[index];
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



void MaskBrush::update(ImageEditor& editor, input::Input& input) {
	math::Vec3f cell = editor.get_pixel();
	graphics::PrimitiveContext2D ctx = editor.get_mask()->GetContext();

	if (cell.z == 0) {
		if (input.mouse_check_pressed(input::MouseButton::Left)) {
            ctx.Draw((i32)cell.x, (i32)cell.y, { 255, 255, 255, 64 });
		}
		else if (input.mouse_check_pressed(input::MouseButton::Right)) {
			ctx.Draw((i32)cell.x, (i32)cell.y, { 0, 0, 0, 64 });
		}
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

	ImGui::End();
}

const char* MaskBrush::get_name() {
	return "Mask Brush";
}