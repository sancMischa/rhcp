#ifndef DISPLAY_H_
#define DISPLAY_H_

#include <stdint.h>
#include <string>
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
        const char* Label;
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
    
    void displayTablePlot(int idx, float timeseries[], int len_timeseries, bool checkbox_status[], float posic_distances[], float ymax);
    bool displayLoadTextureFromFile(std::string& filename, GLuint* out_texture, int* out_width, int* out_height);

    const char* posicLUT(int index);


} // namespace rhcp


#endif // DISPLAY_H_