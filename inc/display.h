#ifndef DISPLAY_H_
#define DISPLAY_H_

#include <stdint.h>
#include "imgui.h"

#include <GL/gl.h>

namespace rhcp {

    // Function prototypes
    void Demo_DragAndDrop();
    ImVec4 RandomColor();
    
    template <typename T>
    inline T RandomRange(T min, T max);
    
    void displayTablePlot(int idx, float timeseries[], int len_timeseries, int checkbox_status[]);

    bool displayLoadTextureFromFile(const char* filename, GLuint* out_texture, int* out_width, int* out_height);

} // namespace rhcp


#endif // DISPLAY_H_