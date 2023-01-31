#pragma once

#include "Core.h"
#include "Common.h"
#include "Graphics.h"
#include "PixelRenderer.h"
#include "Input.h"

using namespace amor;

struct Button;

class Tool {
public:
	virtual void ToolUpdate(input::Input& input, graphics::PrimitiveContext2D& ctx, const math::Vec3f& currentCell) = 0;
	inline graphics::Texture& GetIcon() { return m_Texture; }
protected:
	graphics::Texture m_Texture{ 16, 16 };
};

class Pencil : public Tool {
public:
	Pencil();

	void ToolUpdate(input::Input& input, graphics::PrimitiveContext2D& ctx, const math::Vec3f& currentCell) override;
};

class Fill : public Tool {
public:
	Fill();

	void ToolUpdate(input::Input&, graphics::PrimitiveContext2D& ctx, const math::Vec3f&) override;

	void _fill(i32 x, i32 y, const graphics::Color& color, graphics::PrimitiveContext2D& ctx);

	bool l_released = true;
	bool r_released = true;
};

class Move : public Tool {
public:
	Move();

	void ToolUpdate(input::Input&, graphics::PrimitiveContext2D& ctx, const math::Vec3f&) override;

	bool u_released = true;
	bool d_released = true;
	bool l_released = true;
	bool r_released = true;
};

class Editor : public graphics::WindowBase {
public:
	Editor();
	~Editor();

	u32 m_currentTool;


protected:
	bool OnUserInit() override;
	bool OnUserUpdate(double delta) override;
	void OnUserRender(graphics::RendererBase* _) override;

	void draw_grid();

protected:
	void save();
	void load();
	void convert();
	void serialize();

private:
	graphics::PixelFont m_defaultFont;
	graphics::PixelRenderer m_Renderer;
	byte m_currentCharacter = 'A';
	graphics::Texture m_CurrentCanvas;

	graphics::Texture* m_FontTextures[256];

	math::Rect m_CanvasLocation;

	math::Vec3f m_HighlightedCell;
	
	bool s_released = true;
	bool o_released = true;
	bool f2_released = true;

	std::vector<Tool*> m_Tools;
	std::vector<Button*> m_Buttons;
};