#pragma once

namespace sc2 {

namespace renderer {
    void Initialize(const char* title, int x, int y, int w, int h, unsigned int flags = 0);
    void Shutdown();
    void Matrix1BPP(const char* bytes, int w_mat, int h_mat, int off_x, int off_y, int px_w, int px_h);
    void Matrix8BPPHeightMap(const char* bytes, int w_mat, int h_mat, int off_x, int off_y, int px_w, int px_h);
    //? Liu: "px" in px_w and px_h means "pixel", so here we define the minimum unit to display
    //? Liu: display units by rectangles in different colors based on their alliances
    void Matrix8BPPPlayers(const char* bytes, int w_mat, int h_mat, int off_x, int off_y, int px_w, int px_h);
    void ImageRGB(const char* bytes, int width, int height, int off_x, int off_y);
    void Render();
}

}
