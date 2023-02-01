#pragma once
#include "Input.h"
#include "Graphics.h"
#include "editor.h"

using namespace amor;

class ImageEditor;

enum class BlendSize {
	x33,
};

class BlendMatrix {
public:
	BlendMatrix(BlendSize size);
	~BlendMatrix();

	void apply(ImageEditor& editor, i32 x, i32 y, graphics::PrimitiveContext2D& ctx);

private:
	BlendSize size;
	float* strength;
};

class Tool {
public:
	virtual void update(ImageEditor& editor, amor::input::Input& input) = 0;
	virtual void render_tool_options(ImageEditor& editor) = 0;
	virtual const char* get_name() = 0;
};

class Brush : public Tool {
public:
	void update(ImageEditor& editor, amor::input::Input& input) override;

	void render_tool_options(ImageEditor& editor) override;

	const char* get_name() override;

private:
	bool action_just_started = true;
	input::MouseButton action_detail = amor::input::MouseButton::FIMKEY;
	i32 m_brushSize = 1;
	bool m_isCircle = false;
	bool m_modulate = false;
	i32 m_modulo = 2;
};


class MaskBrush : public Tool {
public:

	void update(ImageEditor& editor, amor::input::Input& input) override;

	void render_tool_options(ImageEditor& editor) override;

	const char* get_name() override;
};

class FloodFill : public Tool {
public:
	void update(ImageEditor& editor, amor::input::Input& input) override;

	void render_tool_options(ImageEditor& editor) override;

	const char* get_name() override;

private:
	void flood_fill(ImageEditor& editor, amor::graphics::PrimitiveContext2D& ctx, i32 x, i32 y, const amor::graphics::Color& color, const amor::graphics::Color& baseColor);

	bool m_FunMode = false;
	i32 m_FunModeRange = 10;
};

class EyeDropper : public Tool {
	void update(ImageEditor& editor, amor::input::Input& input) override;

	void render_tool_options(ImageEditor& editor) override;

	const char* get_name() override;
};

class Blend : public Tool {
public:
	void update(ImageEditor& editor, amor::input::Input& input) override;
	void render_tool_options(ImageEditor& editor) override;
	const char* get_name() override;

private:
	i32 m_blendRadius = 1;
	float m_blendStrength = 0.5f;
	math::Vec3f m_pMouse{ 0, 0 };
	BlendMatrix m_Matrix{ BlendSize::x33 };
};