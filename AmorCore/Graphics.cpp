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
        static void glfw_error_callback(int error, const char* description) {
            logging::GetInstance()->error(std::string("errno ") + std::to_string(error) + " - " + description, "GLFW");
        }

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

            glfwSetErrorCallback(glfw_error_callback);

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

            glfwSetErrorCallback(glfw_error_callback);
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
                m_IsFullscreen = false;
                return;
            }
            GLFWmonitor* monitor = select_monitor(displayNo);
            glfwSetWindowMonitor(m_WindowHandle, monitor, 0, 0, m_Size.width, m_Size.height, GLFW_DONT_CARE);
            m_IsFullscreen = true;
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

        void WindowBase::refresh_context() {
            glfwMakeContextCurrent(m_WindowHandle);
        }

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
            uLongf colorBufferSize = (uLongf)(fileSize - offset);

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
            double alpha = src.a / 255.0;
            a = (a * alpha) + (b * (1.0 - alpha));

            if (a.x > 1.0f) a.x = 1.0f;
            if (a.y > 1.0f) a.y = 1.0f;
            if (a.z > 1.0f) a.z = 1.0f;
            if (a.x < 0.0f) a.x = 0.0f;
            if (a.y < 0.0f) a.y = 0.0f;
            if (a.z < 0.0f) a.z = 0.0f;
            
            return Color::from_rgb_vec( a );
        }

        u32 PrimitiveContext2D::Index(i32 x, i32 y) {
            if (x < 0 || y < 0 || x >= (i32)m_Width || y >= (i32)m_Height) return -1;
            return (y * (i32)m_Width + x);
        }

        void PrimitiveContext2D::Coordinate(u32 index, i32& x, i32& y) {
            y = index / m_Width;
            x = index & m_Width;
        }

        void PrimitiveContext2D::Clear(const Color& col) {
            std::fill(m_Pixels, m_Pixels + m_BufferLength, col);
        }
        void PrimitiveContext2D::FillRect(i32 x, i32 y, i32 width, i32 height, const Color& color) {
            if (x < 0) {
                width += x; // x is negative
                x = 0;
            }
            if (y < 0) {
                height += y;
                y = 0;
            }
            if (x >= (i32)m_Width) {
                return;
            }
            if (y >= (i32)m_Height) {
                return;
            }
            if (x + width > (i32)m_Width) {
                width -= (x + width) - (i32)m_Width;
            }
            if (y + height > (i32)m_Height) {
                height -= (y + height) - (i32)m_Height;
            }


            if ((int)m_BlendMode) {
                for (i32 i = x; i < x+width; i++) {
                    for (i32 j = y; j < y+height; j++) {
                        DrawB(i, j, color);
                    }
                }
            }
            else {
                for (i32 j = 0; j < height; ++j) {
                    u32 startIndex = Index(x, y + j);
                    u32 endIndex = Index(x + width, y + j);

                    if (startIndex == -1 && endIndex == -1) continue;
                    if (y + j >= (i32)m_Height) return;

                    if (startIndex == -1) {
                        startIndex = Index(0, y + j);
                    }
                    else if (endIndex == -1) {
                        endIndex = Index(m_Width - 1, y + j);
                    }

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
                for (i32 j = 1; j < height; j++) {
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
            if (x >= (i32)m_Width || y >= (i32)m_Height) return;

            i32 x2 = x + (tex.width() * scaleX);
            i32 y2 = y + (tex.height() * scaleY);

            // todo: Implove performance by calculating bounds like regular blit
            Color* data = tex.data();

            i32 drawX = x;
            i32 drawY = y;
            for (i32 pi = 0; pi < (i32)tex.width(); ++pi) {
                for (i32 pj = 0; pj < (i32)tex.height(); ++pj) {
                    FillRect(drawX, drawY, scaleX, scaleY, data[pi + pj * (i32)tex.width()]);
                    drawY += scaleY;
                }
                drawX += scaleX;
                drawY = y;
            }
        }

        void PrimitiveContext2D::Blit(i32 x, i32 y, const Texture& tex) {
            if (x >= (i32)m_Width || y >= (i32)m_Height) return;

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

            if (x + bounds.width >= (i32)m_Width) {
                bounds.width -= (x + bounds.width) - (i32)m_Width;
            }
            if (y + bounds.height >= (i32)m_Height) {
                bounds.height -= (y + bounds.height) - (i32)m_Height;
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

        void PrimitiveContext2D::BlitCutout(i32 x, i32 y, const Texture& tex, const Color& color) {
            if (x >= (i32)m_Width || y >= (i32)m_Height) return;
            // internal texture bounds
            amor::math::Rect bounds{ (i32)tex.width(), (i32)tex.height() };

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

            if (x + bounds.width >= (i32)m_Width) {
                bounds.width -= (x + bounds.width) - (i32)m_Width;
            }
            if (y + bounds.height >= (i32)m_Height) {
                bounds.height -= (y + bounds.height) - (i32)m_Height;
            }

            Color* data = tex.data();

            for (i32 i = bounds.x; i < bounds.x2(); i++) {
                for (i32 j = bounds.y; j < bounds.y2(); j++) {
                    if (data[i + j * tex.width()] == color) continue;
                    CLASS_INVOKE(*this, DrawI, x + i, y + j, data[i + j * tex.width()]);
                }
            }
        }

        void PrimitiveContext2D::DrawText(i32 x, i32 y, const std::string& message, Font& font) {
            int cursorX, cursorY;
            cursorX = x;
            cursorY = y;

            for (u32 i = 0; i < message.length(); i++) {
                if (message[i] == '\r') {
                    cursorX = x;
                    continue;
                }
                if (message[i] == '\n') {
                    cursorX = x;
                    cursorY += font.get_size('|').height;
                    continue;
                }
                if (message[i] == '\t') {
                    cursorX += font.get_size(' ').x * 4;
                    continue;
                }

                Texture& tex = font.get_char(message[i]);
                this->BlitCutout(cursorX, cursorY, tex, {0, 0, 0, 255});
                cursorX += font.get_size(message[i]).width;
            }
        }

#pragma endregion
#pragma region PixelFont
        Font::~Font() {}
        math::Rect Font::get_size(const char* string) {
            math::Rect size{ 0, 0 };
            i32 targetX = 0;
            i32 currentX = 0;
            while (*string != '\0') {
                if (*string == '\n') {
                    size.y += get_size(*string).height;
                    currentX = 0;
                }
                else if (*string == '\t') {
                    size.x += get_size(' ').x * 4;
                    currentX += 4;
                    if (currentX >= targetX) {
                        targetX = currentX;
                    }
                }
                else if (*string == '\r') {
                    currentX = 0;
                }
                else {
                    size.x += get_size(*string).x;
                    if (++currentX >= targetX) {
                        targetX = currentX;
                    }
                }
                string++;
            }

            return size;
        }


        PixelFont::PixelFont() : m_NullTexture{ new graphics::Texture{ BASE_FONT_SIZE, BASE_FONT_SIZE } } {
            gen_null_texture();

            for (i32 c = 0; c < 256; c++) {
                m_TextureAtlas[c] = nullptr;
            }

            load_default();
        }

        PixelFont::PixelFont(const std::string& filename) : m_NullTexture{ new graphics::Texture{ BASE_FONT_SIZE, BASE_FONT_SIZE } } {
            gen_null_texture();
            for (i32 c = 0; c < 256; c++) {
                m_TextureAtlas[c] = nullptr;
            }
            load(filename);
        }
        PixelFont::~PixelFont() {
            for (i32 c = 0; c < 256; c++) {
                if (m_TextureAtlas[c] != nullptr) {
                    delete m_TextureAtlas[c];
                }
            }

            if (m_NullTexture != nullptr) {
                delete m_NullTexture;
            }
        }

        Texture& PixelFont::get_char(char ch) {
            byte c = (byte)ch;
            if (m_TextureAtlas[c] == nullptr) {
                return *m_NullTexture;
            }
            if (m_TextureAtlas[c]->data() == nullptr) {
                return *m_NullTexture;
            }
            return *m_TextureAtlas[c];
        }

        math::Rect PixelFont::get_size(char ch) {
            byte c = (byte)ch;
            Texture* tex = m_NullTexture;
            if (m_TextureAtlas[c]->data() != nullptr) {
                tex = m_TextureAtlas[c];
            }

            return { (i32)tex->width(), (i32)tex->height() };
        }

        void PixelFont::load(const std::string& filename) {
            // todo load
        }

        void PixelFont::gen_null_texture() {
            auto context = m_NullTexture->GetContext();

            context.Clear({ 0, 0, 0, 0 });

            /*context.DrawRect(0, 0, BASE_FONT_SIZE, BASE_FONT_SIZE, { 255, 255, 255, 255 });
            context.DrawRect(1, 1, BASE_FONT_SIZE - 1, BASE_FONT_SIZE - 1, { 255, 255, 255, 255 });

            context.DrawLine(0, 0, BASE_FONT_SIZE, BASE_FONT_SIZE, { 255, 255, 255, 255 });
            context.DrawLine(BASE_FONT_SIZE, 0, 0, BASE_FONT_SIZE, { 255, 255, 255, 255 });*/
        }


        PixelFont& PixelFont::font_default() {
            static PixelFont s_font{};
            return s_font;
        }
#pragma region DEFAULT FONT DATA
        // define a blank frame so as to reduce the disk filesize slightly, the preprocessor can expand this out as needed
#define BLANK "0C0C000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
        static const char* DEFAULT_FONT_DATA = "00" BLANK "01" BLANK "02" BLANK "03" BLANK "04" BLANK "05" BLANK "06" BLANK "07" BLANK "08" BLANK "09" BLANK "0A" BLANK "0B" BLANK "0C" BLANK "0D" BLANK "0E" BLANK "0F" BLANK
            "10" BLANK "11" BLANK "12" BLANK "13" BLANK "14" BLANK "15" BLANK "16" BLANK "17" BLANK "18" BLANK "19" BLANK "1A" BLANK "1B" BLANK "1C" BLANK "1D" BLANK "1E" BLANK "1F" BLANK
            "20" BLANK ""
            "210C0C0000000000FFFF000000000000000000FFFFFFFF0000000000000000FFFFFFFF0000000000000000FFFFFFFF0000000000000000FFFFFFFF0000000000000000FFFFFFFF000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000000000000000000000000000FFFF00000000000000000000FFFF0000000000"
            "220C0C000000FFFF00FFFF00000000000000FFFF00FFFF0000000000000000FF0000FF00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
            "230C0C000000FFFF000000FFFF0000000000FFFF000000FFFF0000000000FFFF000000FFFF0000000000FFFF000000FFFF0000FFFFFFFFFFFFFFFFFFFFFFFF0000FFFF000000FFFF0000000000FFFF000000FFFF0000000000FFFF000000FFFF000000FFFFFFFFFFFFFFFFFFFFFFFF00FFFF000000FFFF0000000000FFFF000000FFFF0000000000FFFF000000FFFF00000000"
            "240C0C0000000000FF000000000000000000FFFFFFFFFF000000000000FFFF00FF00FFFF0000000000FFFF00FF0000000000000000FFFF00FF000000000000000000FFFFFFFF00000000000000000000FF00FF000000000000000000FF00FFFF0000000000000000FF00FFFF0000000000FFFF00FF00FFFF000000000000FFFFFFFFFF000000000000000000FF000000000000"
            "250C0C0000FFFF000000000000000000FF0000FF0000000000FF0000FF0000FF00000000FFFF000000FFFF00000000FFFF000000000000000000FFFF000000000000000000FFFF000000000000000000FFFF000000000000000000FFFF000000000000000000FFFF000000FFFF00000000FFFF000000FF0000FF0000FFFF00000000FF0000FF000000000000000000FFFF0000"
            "260C0C00FFFFFF0000000000000000FFFFFFFFFF00000000000000FFFF0000FFFF000000000000FFFF0000FFFF00000000000000FFFFFFFF000000000000000000FFFF00000000FFFF00000000FFFFFFFF0000FFFF000000FFFFFFFFFFFFFFFF000000FFFF00000000FFFFFF000000FFFF000000FFFF00FFFF0000FFFFFFFFFFFF000000FFFF0000FFFFFFFF0000000000FF00"
            "270C0C0000000000FFFF00000000000000000000FFFF0000000000000000000000FF0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
            "280C0C000000000000FFFF000000000000000000FFFF00000000000000000000FFFF000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF0000000000000000000000FFFF00000000000000000000FFFF0000000000000000000000FFFF00000000"
            "290C0C00000000FFFF0000000000000000000000FFFF00000000000000000000FFFF0000000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF000000000000000000FFFF00000000000000000000FFFF000000000000000000FFFF000000000000"
            "2A0C0C0000000000FF000000000000000000FFFFFFFFFF0000000000000000FFFFFF0000000000000000FFFFFFFFFF0000000000000000FF00FF0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
            "2B0C0C0000000000000000000000000000000000000000000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000FFFFFFFFFFFFFFFF00000000FFFFFFFFFFFFFFFF00000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF0000000000000000000000000000000000000000000000000000000000"
            "2C0C0C0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000FFFF00000000000000000000FFFF000000000000000000FFFF000000000000"
            "2D0C0C0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000FFFFFFFFFFFFFFFF00000000FFFFFFFFFFFFFFFF0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
            "2E0C0C000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000FFFF00000000000000000000FFFF00000000000000"
            "2F0C0C00000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF000000000000000000FFFF00000000000000000000FFFF000000000000"
            "300C0C0000000000FFFF000000000000000000FFFFFFFF00000000000000FFFF0000FFFF000000000000FFFF0000FFFF000000000000FFFF0000FFFF000000000000FFFF0000FFFF000000000000FFFF0000FFFF000000000000FFFF0000FFFF000000000000FFFF0000FFFF000000000000FFFF0000FFFF00000000000000FFFFFFFF000000000000000000FFFF0000000000"
            "310C0C0000000000FFFF000000000000000000FFFFFF0000000000000000FFFFFFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF0000000000"
            "320C0C00000000FFFFFFFF00000000000000FFFFFFFFFFFF000000000000FFFF0000FFFF00000000000000000000FFFF00000000000000000000FFFF000000000000000000FFFF000000000000000000FFFFFF0000000000000000FFFFFF0000000000000000FFFFFF000000000000000000FFFF00000000000000000000FFFFFFFFFFFF000000000000FFFFFFFFFFFF000000"
            "330C0C00000000FFFFFF0000000000000000FFFFFFFFFF00000000000000FFFF0000FFFF00000000000000000000FFFF00000000000000000000FFFF0000000000000000FFFFFF000000000000000000FFFFFF0000000000000000000000FFFF00000000000000000000FFFF000000000000FFFF0000FFFF000000000000FFFFFFFFFF0000000000000000FFFFFF0000000000"
            "340C0C0000000000000000FF00000000000000000000FFFF000000000000000000FFFFFF0000000000000000FFFFFFFF00000000000000FFFF00FFFF000000000000FFFF0000FFFF000000000000FFFFFFFFFFFFFFFF00000000FFFFFFFFFFFFFFFF0000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF000000"
            "350C0C000000FFFFFFFFFFFF000000000000FFFFFFFFFFFF000000000000FFFF00000000000000000000FFFF00000000000000000000FFFFFFFFFF0000000000000000FFFFFFFFFF00000000000000000000FFFFFF00000000000000000000FFFF00000000000000000000FFFF0000000000FFFF0000FFFFFF0000000000FFFFFFFFFFFF0000000000000000FFFF0000000000"
            "360C0C0000000000FFFFFFFF000000000000FFFFFFFFFFFF0000000000FFFFFF000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00FFFFFF000000000000FFFFFFFFFFFFFF0000000000FFFFFF0000FFFFFF00000000FFFF00000000FFFF00000000FFFFFF0000FFFFFF0000000000FFFFFFFFFFFF00000000000000FFFFFFFF00000000"
            "370C0C000000FFFFFFFFFFFFFF0000000000FFFFFFFFFFFFFF00000000000000000000FFFF000000000000000000FFFF00000000000000000000FFFF000000000000000000FFFF00000000000000000000FFFF000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF0000000000"
            "380C0C00000000FFFFFFFF00000000000000FFFFFFFFFFFF000000000000FFFF0000FFFF000000000000FFFF0000FFFF000000000000FFFF0000FFFF00000000000000FFFFFFFF0000000000000000FFFFFFFF00000000000000FFFF0000FFFF000000000000FFFF0000FFFF000000000000FFFF0000FFFF000000000000FFFFFFFFFFFF00000000000000FFFFFFFF00000000"
            "390C0C00000000FFFFFFFFFF000000000000FFFFFFFFFFFFFF00000000FFFFFF000000FFFFFF000000FFFF0000000000FFFF000000FFFFFF000000FFFFFF00000000FFFFFFFFFFFFFFFF0000000000FFFFFFFFFFFFFF000000000000000000FFFFFF000000000000000000FFFF000000000000000000FFFFFF0000000000FFFFFFFFFFFF000000000000FFFFFFFFFF00000000"
            "3A0C0C0000000000000000000000000000000000000000000000000000000000FFFF00000000000000000000FFFF00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000FFFF00000000000000000000FFFF0000000000000000000000000000000000000000000000000000000000"
            "3B0C0C0000000000000000000000000000000000000000000000000000000000FFFF00000000000000000000FFFF00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF000000000000000000FFFF000000000000000000000000000000000000"
            "3C0C0C000000000000000000000000000000000000000000000000000000000000FFFFFF0000000000000000FFFFFF0000000000000000FFFFFF0000000000000000FFFFFF0000000000000000FFFFFF00000000000000000000FFFFFF00000000000000000000FFFFFF00000000000000000000FFFFFF00000000000000000000FFFFFF000000000000000000000000000000"
            "3D0C0C0000000000000000000000000000000000000000000000000000000000000000000000000000FFFFFFFFFFFFFFFF00000000FFFFFFFFFFFFFFFF00000000000000000000000000000000000000000000000000000000FFFFFFFFFFFFFFFF00000000FFFFFFFFFFFFFFFF0000000000000000000000000000000000000000000000000000000000000000000000000000"
            "3E0C0C000000000000000000000000000000000000000000000000000000FFFFFF00000000000000000000FFFFFF00000000000000000000FFFFFF00000000000000000000FFFFFF00000000000000000000FFFFFF0000000000000000FFFFFF0000000000000000FFFFFF0000000000000000FFFFFF0000000000000000FFFFFF000000000000000000000000000000000000"
            "3F0C0C0000000000FFFFFF0000000000000000FFFFFFFFFF000000000000FFFFFF0000FFFF0000000000FFFF000000FFFF0000000000FFFF000000FFFF000000000000000000FFFFFF0000000000000000FFFFFF0000000000000000FFFFFF000000000000000000FFFF00000000000000000000000000000000000000000000FFFF00000000000000000000FFFF0000000000"
            "400C0C000000FFFFFFFFFFFFFF00000000FF00000000000000FF0000FF00000000FFFFFF0000FFFF000000FFFFFFFFFFFF00FFFF000000FFFF0000FFFF00FFFF0000FFFF000000FFFF00FFFF0000FFFF0000FFFF0000FFFF000000FFFFFFFFFF0000FF00FF000000FFFF00FF0000FF00FF00000000000000FFFF000000FFFF000000000000000000000000FFFFFFFFFF000000"
            "410C0C00000000FFFFFF00000000000000FFFFFF00FFFFFF0000000000FFFF000000FFFF00000000FFFF0000000000FFFF000000FFFF0000000000FFFF000000FFFF0000000000FFFF000000FFFFFFFFFFFFFFFFFF000000FFFF0000000000FFFF000000FFFF0000000000FFFF000000FFFF0000000000FFFF000000FFFF0000000000FFFF000000FFFF0000000000FFFF0000"
            "420C0C0000FFFFFFFFFFFFFF0000000000FFFFFFFFFFFFFFFF00000000FFFF00000000FFFF00000000FFFF00000000FFFF00000000FFFF00000000FFFF00000000FFFFFFFFFFFFFFFF00000000FFFFFFFFFFFFFF0000000000FFFF00000000FFFF00000000FFFF00000000FFFF00000000FFFF00000000FFFF00000000FFFFFFFFFFFFFFFF00000000FFFFFFFFFFFFFF000000"
            "430C0C00000000FFFFFFFF000000000000FFFFFFFFFFFFFFFF00000000FFFF00000000FFFF000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF0000000000000000000000FFFF00000000FFFF00000000FFFFFFFFFFFFFFFF000000000000FFFFFFFF00000000"
            "440C0C0000FFFFFFFFFF00000000000000FFFFFFFFFFFFFF0000000000FFFF000000FFFF0000000000FFFF00000000FFFF00000000FFFF00000000FFFF00000000FFFF00000000FFFF00000000FFFF00000000FFFF00000000FFFF00000000FFFF00000000FFFF00000000FFFF00000000FFFF000000FFFF0000000000FFFFFFFFFFFFFF0000000000FFFFFFFFFF0000000000"
            "450C0C0000FFFFFFFFFFFFFFFF00000000FFFFFFFFFFFFFFFF00000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFFFFFFFFFF000000000000FFFFFFFFFFFF000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFFFFFFFFFFFFFF00000000FFFFFFFFFFFFFFFF0000"
            "460C0C0000FFFFFFFFFFFFFFFF00000000FFFFFFFFFFFFFFFF00000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFFFFFFFFFF000000000000FFFFFFFFFFFF000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF0000000000000000"
            "470C0C000000FFFFFFFFFFFF0000000000FFFFFFFFFFFFFFFF000000FFFFFF00000000FFFF000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF000000FFFFFFFF000000FFFF000000FFFFFFFF000000FFFF0000000000FFFF00000000FFFF00000000FFFF00000000FFFFFFFFFFFFFFFF0000000000FFFFFFFFFFFF000000"
            "480C0C00FFFF000000000000FFFF0000FFFF000000000000FFFF0000FFFF000000000000FFFF0000FFFF000000000000FFFF0000FFFF000000000000FFFF0000FFFFFFFFFFFFFFFFFFFF0000FFFFFFFFFFFFFFFFFFFF0000FFFF000000000000FFFF0000FFFF000000000000FFFF0000FFFF000000000000FFFF0000FFFF000000000000FFFF0000FFFF000000000000FFFF00"
            "490C0C0000FFFFFFFFFFFFFFFF00000000FFFFFFFFFFFFFFFF00000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000FFFFFFFFFFFFFFFF00000000FFFFFFFFFFFFFFFF0000"
            "4A0C0C000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000FFFF00FFFF00000000000000FFFFFFFFFF0000000000000000FFFFFF0000000000"
            "4B0C0C0000FFFF000000FFFF0000000000FFFF000000FFFF0000000000FFFF0000FFFFFF0000000000FFFF00FFFFFF000000000000FFFFFFFFFF00000000000000FFFFFFFF0000000000000000FFFF00FFFF00000000000000FFFF00FFFFFF000000000000FFFF0000FFFFFF0000000000FFFF000000FFFF0000000000FFFF000000FFFF0000000000FFFF000000FFFF000000"
            "4C0C0C0000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFFFFFFFFFFFF0000000000FFFFFFFFFFFFFF000000"
            "4D0C0C00FFFF000000000000FFFF0000FFFFFF00000000FFFFFF0000FFFFFFFF0000FFFFFFFF0000FFFFFFFFFFFFFFFFFFFF0000FFFF00FFFFFFFF00FFFF0000FFFF0000FFFF0000FFFF0000FFFF0000FFFF0000FFFF0000FFFF000000000000FFFF0000FFFF000000000000FFFF0000FFFF000000000000FFFF0000FFFF000000000000FFFF0000FFFF000000000000FFFF00"
            "4E0C0C00FFFF000000000000FFFF0000FFFF000000000000FFFF0000FFFFFF0000000000FFFF0000FFFFFFFF00000000FFFF0000FFFF00FFFF000000FFFF0000FFFF0000FFFF0000FFFF0000FFFF0000FFFF0000FFFF0000FFFF000000FFFF00FFFF0000FFFF00000000FFFFFFFF0000FFFF0000000000FFFFFF0000FFFF000000000000FFFF0000FFFF000000000000FFFF00"
            "4F0C0C000000FFFFFFFFFFFF0000000000FFFFFFFFFFFFFFFF000000FFFFFF00000000FFFFFF0000FFFF000000000000FFFF0000FFFF000000000000FFFF0000FFFF000000000000FFFF0000FFFF000000000000FFFF0000FFFF000000000000FFFF0000FFFF000000000000FFFF0000FFFFFF00000000FFFFFF000000FFFFFFFFFFFFFFFF0000000000FFFFFFFFFFFF000000"
            "500C0C00FFFFFFFFFFFF000000000000FFFFFFFFFFFFFF0000000000FFFF000000FFFFFF00000000FFFF00000000FFFF00000000FFFF00000000FFFF00000000FFFF000000FFFFFF00000000FFFFFFFFFFFFFF0000000000FFFFFFFFFFFF000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF000000000000000000"
            "510C0C0000FFFFFFFFFFFFFF00000000FFFFFFFFFFFFFFFFFF000000FFFF0000000000FFFF000000FFFF0000000000FFFF000000FFFF0000000000FFFF000000FFFF0000000000FFFF000000FFFF0000000000FFFF000000FFFF000000FFFFFFFF000000FFFF000000FFFFFFFF000000FFFF00000000FFFFFFFF0000FFFFFFFFFFFFFFFFFFFFFF000000FFFFFFFFFFFF00FFFF"
            "520C0C0000FFFFFFFFFFFF000000000000FFFFFFFFFFFFFF0000000000FFFF000000FFFFFF00000000FFFF00000000FFFF00000000FFFF00000000FFFF00000000FFFF000000FFFFFF00000000FFFFFFFFFFFFFF0000000000FFFFFFFFFF00000000000000FFFF000000FFFF0000000000FFFF000000FFFFFF00000000FFFF00000000FFFF00000000FFFF00000000FFFF0000"
            "530C0C00000000FFFFFFFF00000000000000FFFFFFFFFFFF0000000000FFFFFF0000FFFF0000000000FFFF00000000000000000000FFFFFF00000000000000000000FFFFFF00000000000000000000FFFFFFFF00000000000000000000FFFFFF00000000000000000000FFFF0000000000FFFF000000FFFF0000000000FFFFFFFFFFFFFF000000000000FFFFFFFFFF00000000"
            "540C0C00FFFFFFFFFFFFFFFFFFFF0000FFFFFFFFFFFFFFFFFFFF000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF0000000000"
            "550C0C00FFFF000000000000FFFF0000FFFF000000000000FFFF0000FFFF000000000000FFFF0000FFFF000000000000FFFF0000FFFF000000000000FFFF0000FFFF000000000000FFFF0000FFFF000000000000FFFF0000FFFF000000000000FFFF0000FFFF000000000000FFFF000000FFFF00000000FFFF00000000FFFFFFFFFFFFFFFF000000000000FFFFFFFF00000000"
            "560C0C00FFFF000000000000FFFF0000FFFF000000000000FFFF0000FFFF000000000000FFFF0000FFFF000000000000FFFF0000FFFF000000000000FFFF000000FFFF00000000FFFF00000000FFFF00000000FFFF0000000000FFFF0000FFFF000000000000FFFFFFFFFFFF00000000000000FFFFFFFF000000000000000000FFFF00000000000000000000FFFF0000000000"
            "570C0CFFFF0000000000000000FFFFFFFF0000000000000000FFFFFFFF0000000000000000FFFFFFFF0000000000000000FFFF00FFFF000000000000FFFF0000FFFF000000000000FFFF0000FFFF0000FFFF0000FFFF0000FFFF0000FFFF0000FFFF0000FFFF00FFFFFFFF00FFFF000000FFFFFFFFFFFFFFFF00000000FFFFFF0000FFFFFF0000000000FF00000000FF000000"
            "580C0CFFFFFF000000000000FFFFFF00FFFFFF00000000FFFFFF000000FFFFFF0000FFFFFF0000000000FFFFFFFFFFFF00000000000000FFFFFFFF000000000000000000FFFF00000000000000000000FFFF000000000000000000FFFFFFFF00000000000000FFFFFFFFFFFF0000000000FFFFFF0000FFFFFF000000FFFFFF00000000FFFFFF00FFFFFF000000000000FFFFFF"
            "590C0C00FFFF000000000000FFFF0000FFFF000000000000FFFF0000FFFF000000000000FFFF0000FFFFFF00000000FFFFFF000000FFFFFF0000FFFFFF0000000000FFFFFFFFFFFF00000000000000FFFFFFFF000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF0000000000"
            "5A0C0CFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF000000000000000000FFFF000000000000000000FFFFFF0000000000000000FFFFFF00000000000000FFFFFFFF000000000000FFFFFFFF00000000000000FFFFFF0000000000000000FFFFFF000000000000000000FFFF000000000000000000FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
            "5B0C0C00000000FFFFFFFF0000000000000000FFFFFFFF0000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFFFFFF0000000000000000FFFFFFFF00000000"
            "5C0C0CFFFF00000000000000000000FFFFFF00000000000000000000FFFFFF00000000000000000000FFFFFF00000000000000000000FFFFFF00000000000000000000FFFFFF00000000000000000000FFFFFF00000000000000000000FFFFFF00000000000000000000FFFFFF00000000000000000000FFFFFF00000000000000000000FFFFFF00000000000000000000FFFF"
            "5D0C0C00000000FFFFFFFF0000000000000000FFFFFFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF0000000000000000FFFFFFFF0000000000000000FFFFFFFF00000000"
            "5E0C0C0000000000FFFF000000000000000000FFFFFFFF00000000000000FFFFFFFFFFFF0000000000FFFFFF0000FFFFFF00000000FFFF00000000FFFF0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
            "5F0C0C00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000FFFFFFFFFFFFFFFFFFFF0000FFFFFFFFFFFFFFFFFFFF00"
            "600C0C00000000FFFF00000000000000000000FFFFFF00000000000000000000FFFFFF00000000000000000000FFFF00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
            "610C0C00000000000000000000000000000000000000000000000000000000FFFFFFFF00000000000000FFFFFFFFFFFF0000000000FFFFFF0000FFFF0000000000FFFF000000FFFF0000000000000000FFFFFFFF000000000000FFFFFFFFFFFF0000000000FFFFFF0000FFFF0000000000FFFFFF0000FFFF000000000000FFFFFFFFFFFFFF000000000000FFFFFF00FFFF0000"
            "620C0C000000000000000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00FFFFFF000000000000FFFFFFFFFFFFFF0000000000FFFFFF0000FFFF0000000000FFFF000000FFFF0000000000FFFF000000FFFF0000000000FFFFFFFFFFFFFF0000000000FFFFFFFFFFFF000000"
            "630C0C0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000FFFFFFFF00000000000000FFFFFFFFFF000000000000FFFFFF000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFFFF00000000000000000000FFFFFFFFFF0000000000000000FFFFFFFF000000"
            "640C0C000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF000000000000FFFFFFFFFFFF0000000000FFFFFFFFFFFFFF0000000000FFFF000000FFFF0000000000FFFF000000FFFF0000000000FFFFFFFFFFFFFF000000000000FFFFFFFFFFFF00000000"
            "650C0C00000000000000000000000000000000000000000000000000000000FFFFFFFFFF000000000000FFFFFFFFFFFFFF0000000000FFFF000000FFFF0000000000FFFF000000FFFF0000000000FFFFFFFFFFFFFF0000000000FFFFFFFFFFFF000000000000FFFF00000000000000000000FFFFFF00000000000000000000FFFFFFFFFF0000000000000000FFFFFFFF000000"
            "660C0C0000000000FFFFFF0000000000000000FFFFFFFF0000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF0000000000000000FFFFFFFFFFFF000000000000FFFFFFFFFFFF0000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF000000000000"
            "670C0C0000000000000000000000000000000000000000FFFF000000000000FFFFFFFFFFFF0000000000FFFFFFFFFFFFFF0000000000FFFF000000FFFF0000000000FFFF000000FFFF0000000000FFFFFFFFFFFFFF000000000000FFFFFFFFFFFF00000000000000000000FFFF000000000000000000FFFFFF0000000000FFFFFFFFFFFF000000000000FFFFFFFFFF00000000"
            "680C0C0000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00FFFFFFFF0000000000FFFFFFFFFFFFFFFF00000000FFFFFF000000FFFF00000000FFFF00000000FFFF00000000FFFF00000000FFFF00000000FFFF00000000FFFF0000"
            "690C0C0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000FFFF00000000000000000000FFFF00000000000000000000000000000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF0000000000"
            "6A0C0C0000000000000000000000000000000000000000000000000000000000FFFF00000000000000000000FFFF00000000000000000000000000000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF000000000000000000FFFFFF000000000000000000FFFF000000000000"
            "6B0C0C0000000000000000000000000000000000000000000000000000000000000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF0000FFFF000000000000FFFF00FFFFFF000000000000FFFFFFFFFF00000000000000FFFFFFFF0000000000000000FFFF00FFFF00000000000000FFFF00FFFFFF000000000000FFFF0000FFFF00000000"
            "6C0C0C0000000000000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF0000000000000000"
            "6D0C0C0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000FFFFFF0000FFFFFF00000000FFFFFFFFFFFFFFFF000000FFFF0000FFFF0000FFFF0000FFFF0000FFFF0000FFFF0000FFFF000000000000FFFF0000FFFF000000000000FFFF0000FFFF000000000000FFFF0000FFFF000000000000FFFF00"
            "6E0C0C00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000FFFF00000000000000000000FFFFFFFFFFFF000000000000FFFFFFFFFFFFFF0000000000FFFF000000FFFF0000000000FFFF000000FFFF0000000000FFFF000000FFFF0000000000FFFF000000FFFF0000000000FFFF000000FFFF00000000"
            "6F0C0C00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000FFFFFFFF00000000000000FFFFFFFFFFFF000000000000FFFF0000FFFF000000000000FFFF0000FFFF000000000000FFFFFFFFFFFF00000000000000FFFFFFFF00000000"
            "700C0C0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000FFFFFFFFFF00000000000000FFFFFFFFFFFF000000000000FFFF0000FFFF000000000000FFFFFFFFFFFF000000000000FFFFFFFFFF00000000000000FFFF00000000000000000000FFFF0000000000000000"
            "710C0C00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000FFFFFFFFFF000000000000FFFFFFFFFFFF000000000000FFFF0000FFFF000000000000FFFF0000FFFF000000000000FFFFFFFFFFFF00000000000000FFFFFFFFFF00000000000000000000FFFF00000000000000000000FFFF000000"
            "720C0C0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000FFFF00FFFF00000000000000FFFFFFFFFFFF000000000000FFFF0000FFFF000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF0000000000000000"
            "730C0C00000000000000000000000000000000000000000000000000000000000000000000000000000000FFFFFFFF00000000000000FFFFFFFFFFFF000000000000FFFF0000FFFF00000000000000FFFFFF0000000000000000000000FFFF0000000000000000000000FFFF000000000000FFFF0000FFFF000000000000FFFFFFFFFFFF00000000000000FFFFFF0000000000"
            "740C0C0000000000000000000000000000000000000000000000000000000000FFFF00000000000000000000FFFF0000000000000000FFFFFFFFFFFF000000000000FFFFFFFFFFFF0000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFFFF00000000000000000000FFFF00000000"
            "750C0C0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000FFFF000000FFFF0000000000FFFF000000FFFF0000000000FFFF000000FFFF0000000000FFFF000000FFFF0000000000FFFF000000FFFF000000000000FFFFFFFFFF00FFFF00000000FFFFFFFFFF00FFFF00"
            "760C0C000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000FFFF0000FFFF000000000000FFFF0000FFFF000000000000FFFF0000FFFF000000000000FFFF0000FFFF000000000000FFFFFFFFFFFF00000000000000FFFFFFFF000000000000000000FFFF0000000000"
            "770C0C0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000FFFF00000000FFFF00000000FFFF00000000FFFF00000000FFFF00000000FFFF00000000FFFF00000000FFFF00000000FFFF00FFFF00FFFF00000000FFFFFFFFFFFFFFFF0000000000FFFF0000FFFF000000"
            "780C0C00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000FFFFFF00000000FFFFFF000000FFFF00000000FFFF0000000000FFFF0000FFFF00000000000000FFFFFFFF0000000000000000FFFFFFFF00000000000000FFFF0000FFFF0000000000FFFF00000000FFFF000000FFFFFF00000000FFFFFF00"
            "790C0C0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000FFFFFF00000000FFFF00000000FFFFFF0000FFFFFF0000000000FFFFFFFFFFFF00000000000000FFFFFFFF000000000000000000FFFF00000000000000000000FFFF000000000000000000FFFF0000000000000000FFFFFF000000000000"
            "7A0C0C000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000FFFFFFFFFFFF000000000000FFFFFFFFFFFF000000000000000000FFFF000000000000000000FFFF000000000000000000FFFF000000000000000000FFFFFFFFFFFF000000000000FFFFFFFFFFFF000000"
            "7B0C0C0000000000FFFFFFFF00000000000000FFFFFFFFFF000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF0000000000000000FFFF00000000000000000000FFFF000000000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF0000000000000000000000FFFFFFFFFF0000000000000000FFFFFFFF000000"
            "7C0C0C0000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF0000000000"
            "7D0C0C000000FFFFFFFF0000000000000000FFFFFFFFFF0000000000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF000000000000000000000000FFFF00000000000000000000FFFF0000000000000000FFFF00000000000000000000FFFF00000000000000000000FFFF000000000000FFFFFFFFFF00000000000000FFFFFFFF0000000000"
            "7E0C0C000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000FFFFFF00000000FFFF0000FFFFFFFFFF0000FFFFFF00FFFFFF00FFFFFFFFFFFF0000FFFF000000FFFFFFFF0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
            "7F0C0C00000000FFFFFFFFFF00000000000000FF00FF00FF00000000000000FF00FF00FF00000000000000FF00FF00FF00000000000000FF00FF00FF00000000000000FFFFFFFFFF00000000000000FFFFFFFFFF00000000000000FF00FF00FF00000000000000FF00FF00FF00000000000000FF00FF00FF00000000000000FF00FF00FF00000000000000FFFFFFFFFF000000"
            "800C0C0000000000FFFF000000000000000000FFFFFFFF00000000000000FFFF0000FFFF0000000000FFFF00FFFF00FFFF000000FFFFFFFFFFFF00FFFFFF00FFFFFFFFFFFFFF00FFFFFFFFFFFFFFFFFFFF00FFFFFFFFFF00FFFFFFFF00FFFFFFFFFF000000FFFFFFFFFFFFFFFF0000000000FFFF00FFFFFF00000000000000FFFFFFFF000000000000000000FFFF0000000000"
            "81" BLANK "82" BLANK "83" BLANK "84" BLANK "85" BLANK "86" BLANK "87" BLANK "88" BLANK "89" BLANK "8A" BLANK "8B" BLANK "8C" BLANK "8D" BLANK "8E" BLANK "8F" BLANK
            "90" BLANK "91" BLANK "92" BLANK "93" BLANK "94" BLANK "95" BLANK "96" BLANK "97" BLANK "98" BLANK "99" BLANK "9A" BLANK "9B" BLANK "9C" BLANK "9D" BLANK "9E" BLANK "9F" BLANK
            "A0" BLANK "A1" BLANK "A2" BLANK "A3" BLANK "A4" BLANK "A5" BLANK "A6" BLANK "A7" BLANK "A8" BLANK "A9" BLANK "AA" BLANK "AB" BLANK "AC" BLANK "AD" BLANK "AE" BLANK "AF" BLANK
            "B0" BLANK "B1" BLANK "B2" BLANK "B3" BLANK "B4" BLANK "B5" BLANK "B6" BLANK "B7" BLANK "B8" BLANK "B9" BLANK "BA" BLANK "BB" BLANK "BC" BLANK "BD" BLANK "BE" BLANK "BF" BLANK
            "C0" BLANK "C1" BLANK "C2" BLANK "C3" BLANK "C4" BLANK "C5" BLANK "C6" BLANK "C7" BLANK "C8" BLANK "C9" BLANK "CA" BLANK "CB" BLANK "CC" BLANK "CD" BLANK "CE" BLANK "CF" BLANK
            "D0" BLANK "D1" BLANK "D2" BLANK "D3" BLANK "D4" BLANK "D5" BLANK "D6" BLANK "D7" BLANK "D8" BLANK "D9" BLANK "DA" BLANK "DB" BLANK	"DC" BLANK "DD" BLANK "DE" BLANK "DF" BLANK
            "E0" BLANK "E1" BLANK "E2" BLANK "E3" BLANK "E4" BLANK "E5" BLANK "E6" BLANK "E7" BLANK "E8" BLANK "E9" BLANK "EA" BLANK "EB" BLANK "EC" BLANK "ED" BLANK "EE" BLANK "EF" BLANK
            "F0" BLANK "F1" BLANK "F2" BLANK "F3" BLANK "F4" BLANK "F5" BLANK "F6" BLANK "F7" BLANK "F8" BLANK "F9" BLANK "FA" BLANK "FB" BLANK "FC" BLANK "FD" BLANK "FE" BLANK "FF" BLANK;

#define FONT_DATA_LEN 75265

        byte _get_nibble(char c) {
            static const char* s_HexStr = "0123456789ABCDEF";
            for (i32 i = 0; i < 16; i++) {
                if (s_HexStr[i] == c) {
                    return i;
                }
            }
            return -1;
        }
        byte _get_at(u64& offset) {
            byte result = 0;
            byte nibble = _get_nibble(DEFAULT_FONT_DATA[offset]);
            result |= nibble << 4;

            nibble = _get_nibble(DEFAULT_FONT_DATA[offset + 1]);
            result |= nibble;
            offset += 2;
            return result;
        }

        void PixelFont::load_default() {
            u64 data_offset = 0;
            for (i32 i = 0; i < 256; i++) {
                byte index = _get_at(data_offset);
                byte width = _get_at(data_offset);
                byte height = _get_at(data_offset);


                if (m_TextureAtlas[index] != nullptr) {
                    delete m_TextureAtlas[index];
                }
                m_TextureAtlas[index] = new Texture{ width, height };
                PrimitiveContext2D ctx = m_TextureAtlas[index]->GetContext();

                for (i32 j = 0; j < height; j++) {
                    for (i32 i = 0; i < width; i++) {
                        byte v = _get_at(data_offset);
                        ctx.Draw(i, j, { v, v, v, 255 });
                    }
                }
            }
        }
#pragma endregion
    }
}