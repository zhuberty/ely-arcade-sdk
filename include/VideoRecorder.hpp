#pragma once

#include "raylib.h"
#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <cmath>
#include <functional>
#include <utility>
#include "rlgl.h"

#ifdef _WIN32
#define popen _popen
#define pclose _pclose
#endif

class VideoRecorder
{
private:
    struct PipeDeleter
    {
        void operator()(FILE *pipe) const
        {
            if (pipe)
            {
                pclose(pipe);
                std::cout << "[VideoRecorder] Pipe closed safely. Video finalized.\n";
            }
        }
    };

    std::unique_ptr<FILE, PipeDeleter> ffmpegPipe;
    std::vector<uint32_t> pixelBuffer;
    int width = 0;
    int height = 0;
    bool recording = false;
    int fps = 60;
    double maxDurationSeconds = 120.0; // <= 0 means unlimited
    long long framesCaptured = 0;
    std::function<void(const VideoRecorder &)> overlayDrawer;

public:
    VideoRecorder()
    {
        overlayDrawer = [](const VideoRecorder &r)
        { r.DrawDefaultOverlay(); };
    }

    // Override the maximum recording length (seconds). Use <= 0 for no limit.
    void SetMaxDuration(double seconds) { maxDurationSeconds = seconds; }
    double GetMaxDuration() const { return maxDurationSeconds; }

    // Replace the on-screen recording indicator drawing logic.
    void SetOverlayDrawer(std::function<void(const VideoRecorder &)> drawer) { overlayDrawer = std::move(drawer); }

    // Seconds of video recorded so far
    double GetElapsedSeconds() const { return fps > 0 ? (double)framesCaptured / fps : 0.0; }

    // Draws the recording indicator. Call after CaptureFrame() and before EndDrawing()
    // so it appears on screen but is not part of the recorded video.
    void DrawOverlay() const
    {
        if (recording && overlayDrawer)
            overlayDrawer(*this);
    }

    // Default indicator: a red dot that blinks on and off (top-right corner)
    void DrawDefaultOverlay() const
    {
        const double blinkPeriod = 1.0; // seconds per on/off cycle
        bool on = std::fmod(GetElapsedSeconds(), blinkPeriod) < blinkPeriod * 0.5;
        if (on)
            DrawCircle(GetScreenWidth() - 24, 24, 8, RED);
    }

    // Destructor automatically finalizes video if still recording when game exits
    ~VideoRecorder()
    {
        Stop();
    }

    // Start recording a new video file
    bool Start(const std::string &filename, int windowWidth, int windowHeight, int targetFps)
    {
        if (recording)
            return false;

        width = windowWidth;
        height = windowHeight;
        fps = targetFps;
        framesCaptured = 0;
        pixelBuffer.resize(width * height);

        // Build the FFmpeg streaming command string
        std::string command = "ffmpeg -y -f rawvideo -pix_fmt rgba -s " +
                              std::to_string(width) + "x" + std::to_string(height) +
                              " -r " + std::to_string(fps) +
                              " -i - -vf scale=out_color_matrix=bt709:out_range=tv"
                               " -c:v libx264 -pix_fmt yuv420p -b:v 5000k"
                               " -colorspace bt709 -color_primaries bt709 -color_trc bt709 -color_range tv " +
                               filename;

        // Open pipe to FFmpeg process
        ffmpegPipe.reset(popen(command.c_str(), "w"));
        if (!ffmpegPipe)
        {
            std::cerr << "[VideoRecorder] Error: Failed to open FFmpeg pipe!\n";
            return false;
        }

        recording = true;
        std::cout << "[VideoRecorder] Recording started: " << filename << "\n";
        return true;
    }

    // Capture the current window frame and send it to FFmpeg
    void CaptureFrame()
    {
        if (!recording || !ffmpegPipe)
            return;

        // Flush any pending draw calls so the framebuffer holds the full scene
        rlDrawRenderBatchActive();

        // 1. Grab screen data from GPU
        Image screenImg = LoadImageFromScreen();

        // Skip frames if the window was resized (size must match the ffmpeg stream)
        if (screenImg.width != width || screenImg.height != height)
        {
            UnloadImage(screenImg);
            return;
        }

        // 2. LoadImageFromScreen already returns an upright image (raylib flips it internally),
        //    so no additional vertical flip is needed.

        // 3. Copy image into our safe vector array buffer
        std::memcpy(pixelBuffer.data(), screenImg.data, width * height * sizeof(uint32_t));

        // // 3b. Force opaque alpha (blending can leave alpha < 255 in the framebuffer)
        // for (uint32_t &px : pixelBuffer)
        //     px |= 0xFF000000u;

        // 4. Instantly free GPU memory to prevent memory leaks
        UnloadImage(screenImg);

        // 5. Pipe the right-side-up frame array to FFmpeg
        std::fwrite(pixelBuffer.data(), sizeof(uint32_t), pixelBuffer.size(), ffmpegPipe.get());
        framesCaptured++;

        // Auto-stop once the maximum duration is reached
        if (maxDurationSeconds > 0.0 && GetElapsedSeconds() >= maxDurationSeconds)
        {
            std::cout << "[VideoRecorder] Max duration reached, stopping.\n";
            Stop();
        }
    }

    // Force finalize the file and shut down the FFmpeg process
    void Stop()
    {
        if (!recording)
            return;

        ffmpegPipe.reset(); // Invokes PipeDeleter via unique_ptr
        recording = false;
    }

    bool IsRecording() const { return recording; }
};