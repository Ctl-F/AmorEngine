#include "tools.h"

#include <imgui.h>

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

    if (cell.z == 0) {
        if (input.mouse_check_just_pressed(input::MouseButton::Left)){
            editor.push_undo_step();
        }
        if (input.mouse_check_just_released(input::MouseButton::Left)) {
            editor.push_undo_step();
        }

        if (input.mouse_check_pressed(input::MouseButton::Left) && cell != m_pMouse) {
            graphics::Color color = ctx.Get((i32)cell.x, (i32)cell.y);
            math::Vec3f colorVec = color.to_rgb_vec() * (1.0f - m_blendStrength);
            
            i32 r = (m_blendRadius + m_blendRadius + 1) * (m_blendRadius + m_blendRadius + 1) - 1;
            float dStrength = m_blendStrength / (float)r;

            for (i32 i = -m_blendRadius; i <= m_blendRadius; i++) {
                for (i32 j = -m_blendRadius; j <= m_blendRadius; j++) {
                    if (i == 0 && j == 0) continue;
                    i32 x = (i32)cell.x + i;
                    i32 y = (i32)cell.y + j;

                    if (x < 0 || x >= ctx.width()) continue;
                    if (y < 0 || y >= ctx.height()) continue;

                    colorVec += (ctx.Get(x, y).to_rgb_vec()) * dStrength;
                }
            }

            color = graphics::Color::from_rgb_vec(colorVec.normalized());
            ctx.Draw((i32)cell.x, (i32)cell.y, color);

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
