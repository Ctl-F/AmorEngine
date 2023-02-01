#include "editor.h"

#include <fstream>
#include <sstream>
#include <functional>

constexpr int PixelWidth = 2;
constexpr int PixelHeight = 2;
constexpr int Width = 200;
constexpr int Height = 200;

constexpr i32 FontScale = 10;

Pencil::Pencil() {
	graphics::PrimitiveContext2D icon = m_Texture.GetContext();
	icon.Clear({ 0, 0, 0, 255 });
	icon.DrawLine(1, 0, 16, 15, { 255, 255, 255, 255 });
	icon.DrawLine(0, 1, 15, 16, { 255, 255, 255, 255 });
	icon.Draw(1, 1, { 255, 255, 255, 255 });
	icon.Draw(15, 15, { 255, 255, 255, 255 });
}

void Pencil::ToolUpdate(input::Input& m_Input, graphics::PrimitiveContext2D& gfx, const math::Vec3f& m_HighlightedCell) {
	if (m_Input.mouse_check_pressed(input::MouseButton::Left)) {
		gfx.Draw((i32)m_HighlightedCell.x, (i32)m_HighlightedCell.y, { 255, 255, 255, 255 });
	}
	else if (m_Input.mouse_check_pressed(input::MouseButton::Right)) {
		gfx.Draw((i32)m_HighlightedCell.x, (i32)m_HighlightedCell.y, { 0, 0, 0, 0 });
	}
}

Fill::Fill() {
	graphics::PrimitiveContext2D icon = m_Texture.GetContext();
	icon.Clear({ 255, 255, 255, 255 });
}
void Fill::ToolUpdate(input::Input& input, graphics::PrimitiveContext2D& ctx, const math::Vec3f& cell) {
	if (input.mouse_check_pressed(input::MouseButton::Left) && l_released) {
		l_released = false;
		_fill((i32)cell.x, (i32)cell.y, { 255, 255, 255, 255 }, ctx);
	}
	else if (input.mouse_check_pressed(input::MouseButton::Right) && r_released) {
		r_released = false;
		_fill((i32)cell.x, (i32)cell.y, { 0, 0, 0, 255 }, ctx);
	}

	if (input.mouse_check_released(input::MouseButton::Left)) l_released = true;
	if (input.mouse_check_released(input::MouseButton::Right)) r_released = true;
	
}

Move::Move() {
	graphics::PrimitiveContext2D icon = m_Texture.GetContext();
	icon.Clear({ 0, 0, 0, 255 });
	
	graphics::Color white = { 255, 255, 255, 255 };

	icon.DrawLine(8, 0, 8, 16, white);
	icon.DrawLine(7, 0, 7, 16, white);
	icon.DrawLine(0, 8, 16, 8, white);
	icon.DrawLine(0, 7, 16, 7, white);
}

void Move::ToolUpdate(input::Input& input, graphics::PrimitiveContext2D& ctx, const math::Vec3f& cell) {
	if (input.key_check_released(input::Key::Right)) r_released = true;
	if (input.key_check_released(input::Key::Left))  l_released = true;
	if (input.key_check_released(input::Key::Up))    u_released = true;
	if (input.key_check_released(input::Key::Down))  d_released = true;

	i32 xoff = 0, yoff = 0;
	if (input.key_check_pressed(input::Key::Left) && l_released) {
		l_released = false;
		xoff = -1;
	}
	else if (input.key_check_pressed(input::Key::Right) && r_released) {
		r_released = false;
		xoff = 1;
	}
	if (input.key_check_pressed(input::Key::Up) && u_released) {
		u_released = false;
		yoff = -1;
	}
	else if (input.key_check_pressed(input::Key::Down) && d_released) {
		d_released = false;
		yoff = 1;
	}

	if (xoff == 0 && yoff == 0) return;

	graphics::Color* currentFrame = ctx.Data();
	graphics::Color* buffer = new graphics::Color[ctx.width() * ctx.height()];
	if (buffer == nullptr) {
		logging::GetInstance()->error("Unable to allocate color buffer for pixel movement");
		return;
	}

	for (i32 x = 0; x < ctx.width(); ++x) {
		for (i32 y = 0; y < ctx.height(); ++y) {
			i32 dx = x + xoff;
			i32 dy = y + yoff;

			if (dx < 0) dx = ctx.width() - 1;
			if (dx >= ctx.width()) dx = 0;
			if (dy < 0) dy = ctx.height() - 1;
			if (dy >= ctx.height()) dy = 0;

			graphics::Color col = ctx.Get(x, y);
			i32 tx = dx + dy * ctx.width();
			buffer[tx] = col;
		}
	}
	std::copy(buffer, buffer + (ctx.width() * ctx.height()), currentFrame);
	delete[] buffer;
}

void Fill::_fill(i32 x, i32 y, const graphics::Color& color, graphics::PrimitiveContext2D& ctx) {
	if (ctx.Get(x, y) == color) {
		return;
	}
	ctx.Draw(x, y, color);

	if (x - 1 >= 0)           _fill(x - 1, y, color, ctx);
	if (x + 1 < ctx.width())  _fill(x + 1, y, color, ctx);
	if (y - 1 >= 0)           _fill(x, y - 1, color, ctx);
	if (y + 1 < ctx.height()) _fill(x, y + 1, color, ctx);
}

struct Button {
public:
	Button(const math::Rect& l, std::function<void(Editor*)> act, u32 id) {
		Location = l;
		Action = act;
		Id = id;
	}


	math::Rect Location;
	bool clicked = false;
	std::function<void(Editor*)> Action;
	bool hovered = false;
	u32 Id;
	Editor* edit;

	void update(Editor* self, input::Input& input, int scale) {
		math::Vec3f pos = input.mouse_position() / (float)scale;
		edit = self;

		hovered = Location.contains(pos);

		if (hovered) {
			if (input.mouse_check_pressed(input::MouseButton::Left)) {
				clicked = true;
			}
		}

		if (input.mouse_check_released(input::MouseButton::Left)) {
			if (hovered && clicked) {
				Action(self);
			}

			clicked = false;
		}
	}

	void draw(graphics::PrimitiveContext2D& ctx, Tool* tool) {
		ctx.Blit(Location.x + clicked + 5 * (Id == edit->m_currentTool), Location.y + clicked, tool->GetIcon());

		if (hovered) {
			ctx.DrawRect(Location.x, Location.y, Location.width, Location.height, { 255, 0, 0, 255 });
		}
	}
};

Editor::Editor() :
	graphics::WindowBase(&m_Renderer, "Pixel Font Editor: 'A'", graphics::Resolution{ Width, Height, PixelWidth, PixelHeight }),
	m_Renderer(Width, Height, PixelWidth, PixelHeight),
	m_CurrentCanvas(graphics::BASE_FONT_SIZE, graphics::BASE_FONT_SIZE) {

	for (int i = 0; i < 256; i++) {
		m_FontTextures[i] = new graphics::Texture(graphics::BASE_FONT_SIZE, graphics::BASE_FONT_SIZE);
		m_FontTextures[i]->GetContext().Clear({ 0, 0, 0, 0 });
	}

	constexpr int true_size = graphics::BASE_FONT_SIZE * FontScale;

	int cx, cy;
	cx = Width / 2;
	cy = Height / 2;

	cx -= true_size / 2;
	cy -= true_size / 2;

	m_CanvasLocation = { cx, cy, true_size, true_size };

	m_Tools.push_back(new Pencil());
	m_Tools.push_back(new Move());
	m_Tools.push_back(new Fill());
	m_currentTool = 0;


	m_Buttons.push_back(new Button{
		math::Rect(2, 2, 16, 16),
		[](Editor* e) { e->m_currentTool = 0; },
		0});
	m_Buttons.push_back(new Button{
		math::Rect(2, 20, 16, 16),
		[](Editor* e) { e->m_currentTool = 1; },
		1});
	m_Buttons.push_back(new Button{
		math::Rect(2, 38, 16, 16),
		[](Editor* e) { e->m_currentTool = 2; },
		2});
}

Editor::~Editor() {
	for (int i = 0; i < 256; i++) {
		delete m_FontTextures[i];
	}

	for (u32 i = 0; i < m_Tools.size(); i++) {
		delete m_Tools[i];
	}
	for (u32 i = 0; i < m_Buttons.size(); i++) {
		delete m_Buttons[i];
	}
}


bool Editor::OnUserInit() {
	center_on_display();
	m_CurrentCanvas.GetContext().Clear({ 0, 0, 0, 0 });

	return true;
}
bool Editor::OnUserUpdate(double delta) {
	math::Vec3f mouse = m_Input->mouse_position() * 0.5f;
	graphics::PrimitiveContext2D gfx = m_CurrentCanvas.GetContext();
	
	if (m_CanvasLocation.contains(mouse)) {
		m_HighlightedCell = ((mouse - m_CanvasLocation.origin()) / FontScale).floor();

		m_Tools[m_currentTool]->ToolUpdate(*m_Input, gfx, m_HighlightedCell);
	}
	else {
		m_HighlightedCell = { 0, 0, -1 };
	}

	i32 wheel = m_Input->mouse_wheel();

	if (wheel != 0) {
		if (m_Input->key_check_pressed(input::Key::LeftControl)) {
			m_currentTool += wheel;
			if (m_currentTool < 0) m_currentTool = m_Tools.size() - 1;
			if (m_currentTool >= m_Tools.size()) m_currentTool = 0;
		}
		else {

			graphics::PrimitiveContext2D tfx = m_FontTextures[m_currentCharacter]->GetContext();
			tfx.Blit(0, 0, m_CurrentCanvas);

			m_currentCharacter += wheel;
			gfx.Blit(0, 0, *m_FontTextures[m_currentCharacter]);

			m_Title[m_Title.length() - 2] = m_currentCharacter;
			refresh_window_title();
		}
	}

	if (m_Input->key_check_released(input::Key::S)) s_released = true;
	if (m_Input->key_check_released(input::Key::O)) o_released = true;
	if (m_Input->key_check_pressed(input::Key::F2)) f2_released = true;

	if(m_Input->key_check_pressed(input::Key::S) && s_released){
		s_released = false;
		save();
	}

	if (m_Input->key_check_pressed(input::Key::F2) && f2_released) {
		f2_released = false;
		serialize();
	}

	if (m_Input->key_check_pressed(input::Key::O) && o_released) {
		o_released = false;
		load();
	}

	for (u32 i = 0; i < m_Buttons.size(); i++) {
		m_Buttons[i]->update(this, *m_Input, 2);
	}

	return true;
}
void Editor::OnUserRender(graphics::RendererBase* _) {
	m_Renderer.Clear({ 0, 0, 0, 0 });
	
	std::string current = std::to_string((int)m_currentCharacter);
	current += "(";
	current += m_currentCharacter;
	current += ")";

	m_Renderer.DrawText(m_CanvasLocation.x + 35, m_CanvasLocation.y2() + 10, current, m_defaultFont);

	m_Renderer.BlitUpscaled(m_CanvasLocation.x, m_CanvasLocation.y, m_CurrentCanvas, FontScale, FontScale);
	m_Renderer.Blit(this->size().width / PixelWidth - 15, 3, m_CurrentCanvas);

	//
	//m_Renderer.Blit(1, 1, m_Tools[m_currentTool]->GetIcon());

	for (u32 i = 0; i < m_Buttons.size(); i++) {
		m_Buttons[i]->draw(m_Renderer, m_Tools[i]);
	}


	m_Renderer.SetBlending(graphics::BlendMode::Normal);

	if (m_HighlightedCell.z == 0) {
		m_Renderer.FillRect(m_CanvasLocation.x + (m_HighlightedCell.x * FontScale),
						m_CanvasLocation.y + (m_HighlightedCell.y * FontScale),
						FontScale, FontScale,
						{ 255, 255, 255, 64 });
	}

	draw_grid();

	m_Renderer.SetBlending(graphics::BlendMode::None);
}

void Editor::draw_grid() {
	const graphics::Color color{ 0, 255, 255, 255 };

	m_Renderer.DrawRect(m_CanvasLocation.x, m_CanvasLocation.y, m_CanvasLocation.width, m_CanvasLocation.height, color);

	for (int i = m_CanvasLocation.x; i < m_CanvasLocation.x2(); i += FontScale) {
		m_Renderer.DrawLine(i, m_CanvasLocation.y, i, m_CanvasLocation.y2(), color);
	}
	for (int j = m_CanvasLocation.y; j < m_CanvasLocation.y2(); j += FontScale) {
		m_Renderer.DrawLine(m_CanvasLocation.x, j, m_CanvasLocation.x2(), j, color);
	}

}

// [char][size_x][size_y][data...]
void Editor::save() {

	graphics::PrimitiveContext2D ttx = m_FontTextures[m_currentCharacter]->GetContext();
	ttx.Blit(0, 0, m_CurrentCanvas);

	std::ofstream file("out.pixfont", std::ios::binary | std::ios::out);

	for (i32 c = 0; c < 256; c++) {
		byte size = graphics::BASE_FONT_SIZE;
		byte ch = (byte)c;
		file.write((char*)&ch, 1);
		file.write((char*)&size, 1);
		file.write((char*)&size, 1);

		graphics::Color* data = m_FontTextures[c]->data();
		for (i32 j = 0; j < size; j++) {
			for (i32 i = 0; i < size; i++) {
				graphics::Color& col = data[i + j * size];
				file.write((char*)&col.r, 1);
			}
		}

	}

	file.close();

	logging::GetInstance()->info("Saved");
}

void Editor::load() {
	std::ifstream file("out.pixfont", std::ios::in | std::ios::binary);

	if (file.is_open()) {
		file.seekg(0, std::ios::beg);

		for (i32 c = 0; c < 256; c++) {
			byte ch;
			byte foo;
			file.read((char*)&ch, 1);
				
			file.read((char*)&foo, 1);
			file.read((char*)&foo, 1);

			graphics::Texture* tex = m_FontTextures[ch];
			graphics::PrimitiveContext2D ctx = tex->GetContext();

			for (i32 j = 0; j < foo; j++) {
				for (i32 i = 0; i < foo; i++) {
					byte r;
					file.read((char*)&r, 1);
					ctx.Draw(i, j, {r, r, r, 255});
				}
			}
		}
		file.close();
	}
	logging::GetInstance()->info("Loaded");

	m_CurrentCanvas.GetContext().Blit(0, 0, *m_FontTextures[m_currentCharacter]);
}

void Editor::convert() {
	// load old
	std::ifstream file("out.pixfont", std::ios::in | std::ios::binary);

	if (file.is_open()) {
		file.seekg(0, std::ios::beg);

		for (i32 c = 0; c < 256; c++) {
			byte ch;
			i32 foo;
			file.read((char*)&ch, 1);

			file.read((char*)&foo, sizeof(u32));
			file.read((char*)&foo, sizeof(u32));

			graphics::Texture* tex = m_FontTextures[ch];
			graphics::PrimitiveContext2D ctx = tex->GetContext();

			for (i32 j = 0; j < foo; j++) {
				for (i32 i = 0; i < foo; i++) {
					graphics::Color col;

					file.read((char*)&col, sizeof(graphics::Color));

					ctx.Draw(i, j, col);
				}
			}
		}
		file.close();
	}
	logging::GetInstance()->info("Loaded");

	m_CurrentCanvas.GetContext().Blit(0, 0, *m_FontTextures[m_currentCharacter]);

	save();
}

void writeByte(std::stringstream& stream, byte b) {
	
	static char hexValues[] = "0123456789ABCDEF";
	byte nibble = (b & 0xF0) >> 4;
	stream << hexValues[nibble];
	nibble = b & 0xF;
	stream << hexValues[nibble];
}

void Editor::serialize() {
	graphics::PrimitiveContext2D ttx = m_FontTextures[m_currentCharacter]->GetContext();
	ttx.Blit(0, 0, m_CurrentCanvas);

	std::stringstream out;
	for (i32 c = 0; c < 256; c++) {
		writeByte(out, c);
		writeByte(out, (byte)graphics::BASE_FONT_SIZE);
		writeByte(out, (byte)graphics::BASE_FONT_SIZE);

		graphics::Color* data = m_FontTextures[c]->data();
		for (i32 j = 0; j < graphics::BASE_FONT_SIZE; j++) {
			for (i32 i = 0; i < graphics::BASE_FONT_SIZE; i++) {
				graphics::Color& col = data[i + j * graphics::BASE_FONT_SIZE];
				writeByte(out, col.r);
			}
		}
	}

	logging::GetInstance()->info(out.str(), "Serializer");
}