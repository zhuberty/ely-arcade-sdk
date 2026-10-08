#pragma once

#include "raylib.h"
#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <cstring>
#include <cstdint>
#include <cstdio>

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

public:
    VideoRecorder() = default;

    // Destructor automatically finalizes video if still recording when game exits
    ~VideoRecorder()
    {
        Stop();
    }

    // Start recording a new video file
    bool Start(const std::string &filename, int windowWidth, int windowHeight, int fps)
    {
        if (recording)
            return false;

        width = windowWidth;
        height = windowHeight;
        pixelBuffer.resize(width * height);

        // Build the FFmpeg streaming command string
        std::string command = "ffmpeg -y -f rawvideo -pix_fmt rgba -s " +
                              std::to_string(width) + "x" + std::to_string(height) +
                              " -r " + std::to_string(fps) +
                              " -i - -c:v libx264 -pix_fmt yuv420p -b:v 5000k " + filename;

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

        // 1. Grab screen data from GPU
        Image screenImg = LoadImageFromScreen();

        // 2. LoadImageFromScreen already returns an upright image (raylib flips it internally),
        //    so no additional vertical flip is needed.

        // 3. Copy image into our safe vector array buffer
        std::memcpy(pixelBuffer.data(), screenImg.data, width * height * sizeof(uint32_t));

        // 4. Instantly free GPU memory to prevent memory leaks
        UnloadImage(screenImg);

        // 5. Pipe the right-side-up frame array to FFmpeg
        std::fwrite(pixelBuffer.data(), sizeof(uint32_t), pixelBuffer.size(), ffmpegPipe.get());
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