#ifndef DISPLAY_H_
#define DISPLAY_H_

#include <stdint.h>
#include "imgui.h"
#include "implot.h"
#include <vector>

#include <GL/gl.h>

namespace rhcp {

    // Structs and variables
    struct MyDndItem {
        int Idx;
        int Plt;
        ImAxis Yax;
        char Label[16];
        std::vector<float> data; // length needs to be NUM_POSICS, but don't want to constrain to that
        ImVec4 Color;

        MyDndItem();
        void Reset();
    };

    // Function prototypes
    void displayDragAndDrop(MyDndItem dnd_items[], int num_dnd_items, int dataseries_len, float ymax);
    ImVec4 RandomColor();
    
    template <typename T>
    inline T RandomRange(T min, T max);
    
    void displayTablePlot(int idx, float timeseries[], int len_timeseries, int checkbox_status[], float ymax);
    bool displayLoadTextureFromFile(const char* filename, GLuint* out_texture, int* out_width, int* out_height);

} // namespace rhcp


#endif // DISPLAY_H_