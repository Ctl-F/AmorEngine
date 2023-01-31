#include "pch.h"
#include "Graphics.h"
#include "Input.h"

#include <glad/glad.h>
#include <glfw/glfw3.h>
#include "stb_image.h"

#include <fstream>
#include <filesystem>
#include <zlib.h>

namespace amor {
    namespace graphics {

#pragma region struct::Color
        Color Color::from_rgb_vec(const math::Vec3f& vec) {
            return { 
                     (byte)(vec.x * 255.0),
                     (byte)(vec.y * 255.0),
                     (byte)(vec.z * 255.0),
                     255
            };
        }
#pragma endregion
#pragma region struct::Resolution
        Resolution::Resolution(u32 w, u32 h) : width(w), height(h), pixelWidth(1), pixelHeight(1) {}
        Resolution::Resolution(u32 w, u32 h, u32 x, u32 y) : width(w), height(h), pixelWidth(x), pixelHeight(y) {}
#pragma endregion
#pragma region class::WindowBase
        WindowBase::WindowBase(RendererBase* renderer, const std::string& title, u32 width, u32 height) :
                        m_Title(title),
                        m_Size(width, height),
                        m_WindowHandle(nullptr),
                        m_RendererHandle(renderer),
                        m_Input(nullptr),
                        m_Fps(0.0),
                        m_Timer(new util::Timer()),
                        m_FpsTimer(new util::Timer()) {

            // this was originally was to be put into the InitializeGraphicsPipeline
            // however since our library is built around using glfw for window creation and 
            // input management it's been moved here. The specific window and context hints are still
            // left to the graphics pipeline however which should still theoretically allow for any
            // glfw compatible rendering api to be used. Some basic research suggests that most all
            // rendering apis are supported on glfw including (OpenGl, Vulkan, Metal, and DirectX)
            if (glfwInit() == GL_FALSE) {
                logging::GetInstance()->fail("Unable to initialize glfw", "GLFW");
                throw std::runtime_error("glfw init");
            }

            // NOTE: the renderer initialize graphics pipeline has been deferred  to the show() function now
            // but still before window creation
        }
        WindowBase::WindowBase(RendererBase* renderer, const std::string& title, const Resolution& size) :
                        m_Title(title),
                        m_Size(size.width * size.pixelWidth, size.height * size.pixelHeight),
                        m_WindowHandle(nullptr),
                        m_RendererHandle(renderer),
                        m_Input(nullptr),
                        m_Fps(0.0),
                        m_Timer(new util::Timer()),
                        m_FpsTimer(new util::Timer()) {

            if (glfwInit() == GL_FALSE) {
                logging::GetInstance()->fail("Unable to initialize glfw", "GLFW");
                throw std::runtime_error("glfw init");
            }

        }

        WindowBase::~WindowBase() {
            if (m_WindowHandle != nullptr) {
                glfwDestroyWindow(m_WindowHandle);
                m_WindowHandle = nullptr;
            }

            if (m_Input != nullptr) {
                delete m_Input;
                m_Input = nullptr;
            }

            if (m_Timer != nullptr) {
                delete m_Timer;
                m_Timer = nullptr;
            }
            
            if (m_FpsTimer != nullptr) {
                delete m_FpsTimer;
                m_FpsTimer = nullptr;
            }

            glfwTerminate();
        }

        double WindowBase::fps() const {
            return m_Fps;
        }

        void WindowBase::show() {
            // renderer initializes graphics here. For opengl the main thing to initialize are the window hints
            m_RendererHandle->InitializeGraphicsPipeline();

            m_WindowHandle = glfwCreateWindow(m_Size.width, m_Size.height, m_Title.c_str(), nullptr, nullptr);
            if (m_WindowHandle == nullptr) {
                logging::GetInstance()->fail("Error creating Window", "GLFW");
                throw std::runtime_error("Error creating glfw window");
            }

            m_Input = new input::Input(this);

            m_RendererHandle->InitializeWindowGraphicsPipeline(this);

            logging::GetInstance()->info("Window Created", "GLFW");
            logging::GetInstance()->info("Entering Main Loop", "MainWindow");
            main_loop();

            m_RendererHandle->DeinitializeGraphicsPipeline(this);
        }

        i32 WindowBase::get_display_count() const {
            i32 count;
            glfwGetMonitors(&count);
            return count;
        }

        void WindowBase::fullscreen_on_display(bool fullscreen, i32 displayNo) {
            if (!fullscreen) {
                glfwSetWindowMonitor(m_WindowHandle, nullptr, 0, 0, m_Size.width, m_Size.height, GLFW_DONT_CARE);
                return;
            }
            GLFWmonitor* monitor = select_monitor(displayNo);
            glfwSetWindowMonitor(m_WindowHandle, monitor, 0, 0, m_Size.width, m_Size.height, GLFW_DONT_CARE);
        }

        void WindowBase::center_on_display(i32 displayNo) {
            GLFWmonitor* monitor = select_monitor(displayNo);

            i32 displayX, displayY, displayW, displayH;
            glfwGetMonitorWorkarea(monitor, &displayX, &displayY, &displayW, &displayH);

            i32 centerX = displayX + displayW / 2;
            i32 centerY = displayY + displayH / 2;

            centerX -= m_Size.width / 2;
            centerY -= m_Size.height / 2;

            glfwSetWindowPos(m_WindowHandle, centerX, centerY);
        }

        GLFWmonitor* WindowBase::select_monitor(i32 displayNo) const {
            i32 count;
            GLFWmonitor** monitors = glfwGetMonitors(&count);
            if (displayNo == PRIMARY_MONITOR || displayNo >= count) {
                return glfwGetPrimaryMonitor();
            }
            else {
                return monitors[displayNo];
            }
        }

        input::Input* WindowBase::input() const {
            return m_Input;
        }

        void WindowBase::main_loop() {
            if (!OnUserInit()) {
                logging::GetInstance()->error("User Init exited with value of 'false'", "User");
                return;
            }

            m_Timer->start();
            m_FpsTimer->start();
            
            double avgFps = 0.0;
            while (!glfwWindowShouldClose(m_WindowHandle)) {
                avgFps = (avgFps + m_FpsTimer->delta_seconds()) / 2.0;
                m_Fps = 1.0 / avgFps;

                glfwPollEvents();
                m_Input->Update(this);

                if (!OnUserUpdate(m_Timer->delta_seconds())) {
                    break;
                }

                m_RendererHandle->PrepareFrame(this);
                OnUserRender(m_RendererHandle);
                m_RendererHandle->RenderFrame(this);
                m_RendererHandle->PostRenderFrame(this);

                glfwSwapBuffers(m_WindowHandle);
            }

            OnUserDeinit();
        }

        bool WindowBase::OnUserInit() { return true; }
        void WindowBase::OnUserDeinit() {}
        bool WindowBase::OnUserUpdate(double d) { return true; }
        void WindowBase::OnUserRender(RendererBase* r) {}

        void WindowBase::refresh_window_title() {
            glfwSetWindowTitle(m_WindowHandle, m_Title.c_str());
        }

        const amor::math::Rect& WindowBase::size() const {
            return m_Size;
        }
        GLFWwindow* WindowBase::internal_ptr() const {
            return m_WindowHandle;
        }

#pragma endregion
#pragma region class::RendererBase
        void RendererBase::PrepareFrame(WindowBase*) {
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT);
        };
        void RendererBase::PostRenderFrame(WindowBase* win) { };
#pragma endregion
#pragma region class::Texture
        Texture::Texture() : m_Width(0), m_Height(0), m_Pixels(nullptr), m_ImageLoaded(false) {
            
        }
        Texture::Texture(u32 width, u32 height) : m_Width(width), m_Height(height), m_Pixels(nullptr), m_ImageLoaded(false) {
            m_Pixels = new Color[width * height];
        }
        Texture::Texture(const char* filename) {
            i32 w, h, n;
            byte* data = stbi_load(filename, &w, &h, &n, 4);
            if (data == NULL) {
                logging::GetInstance()->error("Error loading image file: " + std::string(filename), "Texture");
                throw std::runtime_error("Cannot load file");
            }

            m_Width = w;
            m_Height = h;
            m_Pixels = reinterpret_cast<Color*>(data);
            m_ImageLoaded = true;
        }
        Texture::~Texture() {
            if (m_Pixels == nullptr) return;
            if (m_ImageLoaded) {
                stbi_image_free(m_Pixels);
            }
            else {
                delete[] m_Pixels;
            }
            m_Pixels = nullptr;
            m_ImageLoaded = false;
        }
    
        u32 Texture::width() const {
            return m_Width;
        }
        u32 Texture::height() const {
            return m_Height;
        }
        Color* Texture::data() const {
            return m_Pixels;
        }


        // not truly noendian, but this will use math to force a specific endian
        // format for the u32 bytes, unsafe because there's no buffer checks
        // so be careful with buffer overflows
        void write_u32_noendian_unsafe(byte* buffer, u32 value, u64& index) {
            buffer[index++] = (byte)((value >> 24) & 0xFF);
            buffer[index++] = (byte)((value >> 16) & 0xFF);
            buffer[index++] = (byte)((value >>  8) & 0xFF);
            buffer[index++] = (byte)((value) & 0xFF);
        }

        void read_u32_noendian_unsafe(byte* buffer, u32& value, u64& index) {
            value = (static_cast<u32>(buffer[index]) << 24) |
                    (static_cast<u32>(buffer[index+1]) << 16) |
                    (static_cast<u32>(buffer[index+2]) << 8) |
                    (static_cast<u32>(buffer[index+3]));
            index += 4;
        }

        // buffer_size (for colors): width * height * 4 (rgba)
        // u32: [b0] [b1] [b2] [b3]
        // file format: 'P', 'X', 'I', 'M', [width b0], [width b1], [width b2], [width b3], [height b0], [height b1], [height b2], [height b3], <compressed color stream>
        void Texture::save(const char* filename) {
            byte* colorBuffer = new byte[m_Width * m_Height * 4];
            byte* compressionBuffer = new byte[m_Width * m_Height * 4];
            uLongf compressedLength = (m_Width * m_Height * 4);

            std::copy((byte*)m_Pixels, (byte*)(m_Pixels + m_Width * m_Height), colorBuffer);

            if (compress(compressionBuffer, &compressedLength, colorBuffer, m_Width * m_Height * 4) != Z_OK) {
                logging::GetInstance()->error("Unabled to compress color buffer", "Texture.Save");
                delete[] colorBuffer;
                delete[] compressionBuffer;
                return;
            }

            u64 index = 4;
            byte* headerBuffer = new byte[12];
            headerBuffer[0] = 'P';
            headerBuffer[1] = 'X';
            headerBuffer[2] = 'I';
            headerBuffer[3] = 'M';
            write_u32_noendian_unsafe(headerBuffer, m_Width, index);
            write_u32_noendian_unsafe(headerBuffer, m_Height, index);

            std::ofstream file(filename, std::ios::binary | std::ios::out);
            if (file.is_open()) {

                file.write((char*)headerBuffer, 12);
                file.write((char*)compressionBuffer, compressedLength);

                file.close();
            }
            else {
                logging::GetInstance()->error("Unabled to open file for saving", "Texture.Save");
            }

            delete[] colorBuffer;
            delete[] compressionBuffer;
            delete[] headerBuffer;
        }
        void Texture::load(const char* filename) {
            if (!std::filesystem::exists(filename)) {
                logging::GetInstance()->error("File not found", "Texture.Load");
                return;
            }
            u64 fileSize = 0;

            {
                std::ifstream fsize(filename, std::ios::binary | std::ios::ate);
                fileSize = fsize.tellg();
                fsize.close();
            }
            if (fileSize == 0) {
                logging::GetInstance()->error("Unexpected file size for texture loading", "Texture.Load");
                return;
            }
            
            byte* fileBuffer = new byte[fileSize];
            std::ifstream file(filename, std::ios::binary);
            if (file.is_open()) {
                file.read((char*)fileBuffer, fileSize);
                file.close();
            }
            else {
                logging::GetInstance()->error("Unabled to open file for reading", "Texture.Load");
                delete[] fileBuffer;
                return;
            }

            if ((char)fileBuffer[0] != 'P' || (char)fileBuffer[1] != 'X' || (char)fileBuffer[2] != 'I' || (char)fileBuffer[3] != 'M') {
                logging::GetInstance()->error("Unexpected file format", "Texture.Load");
                delete[] fileBuffer;
                return;
            }

            u64 offset = 4;
            u32 width, height;
            read_u32_noendian_unsafe(fileBuffer, width, offset);
            read_u32_noendian_unsafe(fileBuffer, height, offset);

            // offset pointer past the header and size info to the start of the compressed color buffer
            fileBuffer += offset;
            uLongf colorBufferSize = (fileSize - offset);

            uLongf uncompressedSize = (width * height * 4);
            byte* uncompressedStream = new byte[uncompressedSize];

            if (uncompress(uncompressedStream, &uncompressedSize, fileBuffer, colorBufferSize) != Z_OK) {
                logging::GetInstance()->error("Unabled to decompress color data", "Texture.Load");
                delete[] uncompressedStream;
                delete[] (fileBuffer - offset);
                return;
            }

            if (m_ImageLoaded) {
                stbi_image_free(m_Pixels);
                m_Pixels = nullptr;
            }
            else if (m_Pixels != nullptr) {
                delete[] m_Pixels;
            }

            m_Width = width;
            m_Height = height;
            m_Pixels = new Color[width * height];

            std::copy(uncompressedStream, uncompressedStream + uncompressedSize, (byte*)m_Pixels);

            delete[] uncompressedStream;
            delete[] (fileBuffer - offset);
        }

#pragma endregion
#pragma region class::PrimitiveContext2D

        PrimitiveContext2D Texture::GetContext() {
            return { m_Width, m_Height, m_Pixels };
        }


        PrimitiveContext2D::PrimitiveContext2D(u32 width, u32 height, Color* buffer) :
                m_Width(width), m_Height(height), m_Pixels(buffer), m_BufferLength(width * height) {

        }
        PrimitiveContext2D::~PrimitiveContext2D() {
            // we don't own the buffer pointer so we don't free the buffer pointer
            m_Pixels = nullptr;
        }

        void PrimitiveContext2D::SetBlending(BlendMode mode) {
            m_BlendMode = mode;

            if ((int)m_BlendMode) {
                DrawI = &PrimitiveContext2D::DrawB;
            }
            else {
                DrawI = &PrimitiveContext2D::Draw;
            }

            switch (m_BlendMode) {
            case BlendMode::Normal:
                m_BlendFunc = &PrimitiveContext2D::Blend;
                break;
            }
        }

        Color PrimitiveContext2D::Blend(const Color& src, const Color& dest) {
            math::Vec3f a = src.to_rgb_vec();
            math::Vec3f b = dest.to_rgb_vec();
            double alpha = src.a * 255.0;

            return Color::from_rgb_vec( (a * alpha) + (b * (1.0 - alpha)) );
        }

        u32 PrimitiveContext2D::Index(i32 x, i32 y) {
            if (x < 0 || y < 0 || x >= m_Width || y >= m_Height) return -1;
            return (y * m_Width + x);
        }

        void PrimitiveContext2D::Coordinate(u32 index, i32& x, i32& y) {
            y = index / m_Width;
            x = index & m_Width;
        }

        void PrimitiveContext2D::Clear(const Color& col) {
            std::fill(m_Pixels, m_Pixels + m_BufferLength, col);
        }
        void PrimitiveContext2D::FillRect(i32 x, i32 y, i32 width, i32 height, const Color& color) {
            if ((int)m_BlendMode) {
                for (u32 i = x; i < x+width; i++) {
                    for (u32 j = y; j < y+height; j++) {
                        DrawB(i, j, color);
                    }
                }
            }
            else {
                for (u32 j = 0; j < height; ++j) {
                    u32 startIndex = Index(x, y + j);
                    u32 endIndex = Index(x + width, y + j);

                    std::fill(m_Pixels + startIndex, m_Pixels + endIndex, color);
                }
            }
        }

        // stupid filled circle algorithm,
        // find a better one
        void PrimitiveContext2D::FillCircle(i32 x, i32 y, i32 radius, const Color& color) {
            i32 x1, y1, x2, y2;
            i32 radius_squared;
            i32 dx, dy;

            x1 = x - radius;
            y1 = y - radius;
            x2 = x + radius;
            y2 = y + radius;
            radius_squared = radius * radius;
            for (i32 i = x1; i < x2; i++) {
                dx = (i - x);
                for (i32 j = y1; j < y2; j++) {
                    dy = (j - y);
                    if (dx*dx + dy*dy < radius_squared - radius) (*this.*DrawI)(i, j, color);
                }
            }
        }

        inline void Plot8(PrimitiveContext2D* ctx, i32 xc, i32 yc, i32 x, i32 y, const Color& color) {
            /*ctx->Draw*/ (*ctx.*(ctx->DrawI))(xc+x, yc+y, color);
            /*ctx->Draw*/ (*ctx.*(ctx->DrawI))(xc-x, yc+y, color);
            /*ctx->Draw*/ (*ctx.*(ctx->DrawI))(xc+x, yc-y, color);
            /*ctx->Draw*/ (*ctx.*(ctx->DrawI))(xc-x, yc-y, color);
            /*ctx->Draw*/ (*ctx.*(ctx->DrawI))(xc+y, yc+x, color);
            /*ctx->Draw*/ (*ctx.*(ctx->DrawI))(xc-y, yc+x, color);
            /*ctx->Draw*/ (*ctx.*(ctx->DrawI))(xc+y, yc-x, color);
            /*ctx->Draw*/ (*ctx.*(ctx->DrawI))(xc-y, yc-x, color);
        }

        // Draw Rect
        void PrimitiveContext2D::DrawRect(i32 x, i32 y, i32 width, i32 height, const Color& color) {
            // basic approach
            if ((int)m_BlendMode) {
                DrawLine(x, y, x + width, y, color);
                DrawLine(x, y + height, x + width, y + height, color);
                DrawLine(x, y + 1, x, y + height - 1, color);
                DrawLine(x + width, y + 1, x + width, y + height - 1, color);
            }
            else {
                std::fill(m_Pixels + Index(x, y), m_Pixels + Index(x + width + 1, y), color);
                std::fill(m_Pixels + Index(x, y + height), m_Pixels + Index(x + width + 1, y + height), color);
                for (u32 j = 1; j < height; j++) {
                    m_Pixels[Index(x, y + j)] = color;
                    m_Pixels[Index(x + width, y + j)] = color; // maybe -1 needed here? not sure
                }
            }
        }

        // bresenham's circle algorithm
        void PrimitiveContext2D::DrawCircle(i32 x, i32 y, i32 radius, const Color& color) {
            int xx = 0;
            int yy = radius;
            int d = 3 - (2 * radius);
            Plot8(this, x, y, xx, yy, color);

            while (yy >= xx) {
                xx++;

                if (d > 0) {
                    --yy;
                    d = d + 4 * (xx - yy) + 10;
                }
                else {
                    d = d + 4 * xx + 6;
                }
                
                Plot8(this, x, y, xx, yy, color);
            }
        }

        // Bresenham's Line algorithm
        void PrimitiveContext2D::DrawLine(i32 x1, i32 y1, i32 x2, i32 y2, const Color& color) {
            i32 x, y, dx, dy, dx1, dy1, px, py, xe, ye, i;
            dx = x2 - x1;
            dy = y2 - y1;
            dx1 = (i32)fabs(dx);
            dy1 = (i32)fabs(dy);
            px = 2 * dy1 - dx1;
            py = 2 * dx1 - dy1;

            if (dy1 <= dx1) {
                if (dx >= 0) {
                    x = x1;
                    y = y1;
                    xe = x2;
                }
                else {
                    x = x2;
                    y = y2;
                    xe = x1;
                }

                (*this.*DrawI)(x, y, color);
                for (i = 0; x < xe; ++i) {
                    ++x;
                    if (px < 0) {
                        px = px + 2 * dy1;
                    }
                    else {
                        if ((dx < 0 && dy < 0) || (dx > 0 && dy > 0)) {
                            ++y;
                        }
                        else {
                            --y;
                        }
                        px = px + 2 * (dy1 - dx1);
                    }
                    (*this.*DrawI)(x, y, color);
                }
            }
            else {
                if (dy >= 0) {
                    x = x1;
                    y = y1;
                    ye = y2;
                }
                else {
                    x = x2;
                    y = y2;
                    ye = y1;
                }
                (*this.*DrawI)(x, y, color);
                for (i = 0; y < ye; ++i) {
                    ++y;
                    if (py <= 0) {
                        py = py + 2 * dx1;
                    }
                    else {
                        if ((dx < 0 && dy < 0) || (dx > 0 && dy > 0)) {
                            ++x;
                        }
                        else {
                            --x;
                        }
                        py = py + 2 * (dx1 - dy1);
                    }
                    (*this.*DrawI)(x, y, color);
                }
            }
        }

        void PrimitiveContext2D::Draw(i32 x, i32 y, const Color& color) {
            u32 index = Index(x, y);
            if (index == -1) return;
            m_Pixels[index] = color;
        }

        void PrimitiveContext2D::DrawB(i32 x, i32 y, const Color& src) {
            u32 index = Index(x, y);
            if (index == -1) return;

            Color& dest = m_Pixels[index];
            m_Pixels[index] = (*this.*m_BlendFunc)(src, dest);
        }

        Color PrimitiveContext2D::Get(i32 x, i32 y) {
            u32 index = Index(x, y);
            if (index == -1) return { 0, 0, 0, 0 };
            return m_Pixels[index];
        }

        void PrimitiveContext2D::BlitUpscaled(i32 x, i32 y, const Texture& tex, i32 scaleX, i32 scaleY) {
            if (x >= m_Width || y >= m_Height) return;

            i32 x2 = x + (tex.width() * scaleX);
            i32 y2 = y + (tex.height() * scaleY);

            // todo: Implove performance by calculating bounds like regular blit
            Color* data = tex.data();

            i32 drawX = x;
            i32 drawY = y;
            for (i32 pi = 0; pi < tex.width(); ++pi) {
                for (i32 pj = 0; pj < tex.height(); ++pj) {
                    FillRect(drawX, drawY, scaleX, scaleY, data[pi + pj * tex.width()]);
                    drawY += scaleY;
                }
                drawX += scaleX;
                drawY = y;
            }
        }

        void PrimitiveContext2D::Blit(i32 x, i32 y, const Texture& tex) {
            if (x >= m_Width || y >= m_Height) return;

            // internal texture bounds
            amor::math::Rect bounds{ (i32)tex.width(), (i32)tex.height()};
            
            if (x < 0) {
                bounds.width += x;
                bounds.x = -x;
                x = 0;
            }
            if (y < 0) {
                bounds.height += y;
                bounds.y = -y;
                y = 0;
            }

            if (x + bounds.width >= m_Width) {
                bounds.width -= (x + bounds.width) - m_Width;
            }
            if (y + bounds.height >= m_Height) {
                bounds.height -= (y + bounds.height) - m_Height;
            }

            Color* data = tex.data();

            if ((int)m_BlendMode) {
                for (i32 i = bounds.x; i < bounds.x2(); i++) {
                    for (i32 j = bounds.y; j < bounds.y2(); j++) {
                        DrawB(i, j, data[i + j * tex.width()]);
                    }
                }
            }
            else {
                // j is the internal row
                for (i32 j = bounds.y; j < bounds.y2(); ++j) {
                    u32 textureRowOffset = j * tex.width();
                    u32 myRowOffset = (y + j) * m_Width;

                    std::copy(data + textureRowOffset + bounds.x, data + textureRowOffset + bounds.x2(), m_Pixels + myRowOffset + x);
                }
            }
        }

#pragma endregion
    }
}