#include "display.h"
#include "imgui.h"
#include "implot.h"
#include <stdio.h>
#include <math.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

void rhcp::Demo_DragAndDrop() {
    ImGui::BulletText("Drag/drop items from the left column.");
    ImGui::BulletText("Drag/drop items between plots.");
    ImGui::Indent();
    ImGui::BulletText("Plot 1 Targets: Plot, Y-Axes, Legend");
    ImGui::BulletText("Plot 1 Sources: Legend Item Labels");
    ImGui::BulletText("Plot 2 Targets: Plot, X-Axis, Y-Axis");
    ImGui::BulletText("Plot 2 Sources: Plot, X-Axis, Y-Axis (hold Ctrl)");
    ImGui::Unindent();

    // convenience struct to manage DND items; do this however you like
    struct MyDndItem {
        int              Idx;
        int              Plt;
        ImAxis           Yax;
        char             Label[16];
        ImVector<ImVec2> Data;
        ImVec4           Color;
        MyDndItem()        {
            static int i = 0;
            Idx = i++;
            Plt = 0;
            Yax = ImAxis_Y1;
            snprintf(Label, sizeof(Label), "%02d Hz", Idx+1);
            Color = RandomColor();
            Data.reserve(1001);
            for (int k = 0; k < 1001; ++k) {
                float t = k * 1.0f / 999;
                Data.push_back(ImVec2(t, 0.5f + 0.5f * sinf(2*3.14f*t*(Idx+1))));
            }
        }
        void Reset() { Plt = 0; Yax = ImAxis_Y1; }
    };

    const int         k_dnd = 20;
    static MyDndItem  dnd[k_dnd];

    // child window to serve as initial source for our DND items
    ImGui::BeginChild("DND_LEFT",ImVec2(100,400));
    if (ImGui::Button("Reset Data")) {
        for (int k = 0; k < k_dnd; ++k)
            dnd[k].Reset();
    }
    for (int k = 0; k < k_dnd; ++k) {
        if (dnd[k].Plt > 0)
            continue;
        ImPlot::ItemIcon(dnd[k].Color); ImGui::SameLine();
        ImGui::Selectable(dnd[k].Label, false, 0, ImVec2(100, 0));
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
            ImGui::SetDragDropPayload("MY_DND", &k, sizeof(int));
            ImPlot::ItemIcon(dnd[k].Color); ImGui::SameLine();
            ImGui::TextUnformatted(dnd[k].Label);
            ImGui::EndDragDropSource();
        }
    }
    ImGui::EndChild();
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MY_DND")) {
            int i = *(int*)payload->Data; dnd[i].Reset();
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::SameLine();
    ImGui::BeginChild("DND_RIGHT",ImVec2(-1,400));
    // timeseries plot
    ImPlotAxisFlags flags = ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoGridLines | ImPlotAxisFlags_NoHighlight;
    if (ImPlot::BeginPlot("##DND1", ImVec2(-1,195))) {
        ImPlot::SetupAxis(ImAxis_X1, nullptr, flags|ImPlotAxisFlags_Lock);
        ImPlot::SetupAxis(ImAxis_Y1, "[drop here]", flags);
        ImPlot::SetupAxis(ImAxis_Y2, "[drop here]", flags|ImPlotAxisFlags_Opposite);
        ImPlot::SetupAxis(ImAxis_Y3, "[drop here]", flags|ImPlotAxisFlags_Opposite);

        for (int k = 0; k < k_dnd; ++k) {
            if (dnd[k].Plt == 1 && dnd[k].Data.size() > 0) {
                ImPlot::SetAxis(dnd[k].Yax);
                ImPlot::SetNextLineStyle(dnd[k].Color);
                ImPlot::PlotLine(dnd[k].Label, &dnd[k].Data[0].x, &dnd[k].Data[0].y, dnd[k].Data.size(), 0, 0, 2 * sizeof(float));
                // allow legend item labels to be DND sources
                if (ImPlot::BeginDragDropSourceItem(dnd[k].Label)) {
                    ImGui::SetDragDropPayload("MY_DND", &k, sizeof(int));
                    ImPlot::ItemIcon(dnd[k].Color); ImGui::SameLine();
                    ImGui::TextUnformatted(dnd[k].Label);
                    ImPlot::EndDragDropSource();
                }
            }
        }
        // allow the main plot area to be a DND target
        if (ImPlot::BeginDragDropTargetPlot()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MY_DND")) {
                int i = *(int*)payload->Data; dnd[i].Plt = 1; dnd[i].Yax = ImAxis_Y1;
            }
            ImPlot::EndDragDropTarget();
        }
        // allow each y-axis to be a DND target
        for (int y = ImAxis_Y1; y <= ImAxis_Y3; ++y) {
            if (ImPlot::BeginDragDropTargetAxis(y)) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MY_DND")) {
                    int i = *(int*)payload->Data; dnd[i].Plt = 1; dnd[i].Yax = y;
                }
                ImPlot::EndDragDropTarget();
            }
        }
        // allow the legend to be a DND target
        if (ImPlot::BeginDragDropTargetLegend()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MY_DND")) {
                int i = *(int*)payload->Data; dnd[i].Plt = 1; dnd[i].Yax = ImAxis_Y1;
            }
            ImPlot::EndDragDropTarget();
        }
        ImPlot::EndPlot();
    }
    
    ImGui::EndChild();
}

// to be called in loop, doesn't include begin table or end table
void rhcp::displayTablePlot(int idx, float timeseries[], int len_timeseries){
    // this is SLOW
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("POSIC %d", idx);
    ImGui::TableSetColumnIndex(1);
    ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(0,0)); // no graph padding
    if (ImPlot::BeginPlot("",ImVec2(-1, 30),ImPlotFlags_CanvasOnly|ImPlotFlags_NoChild)) {
        ImPlot::SetupAxes(nullptr,nullptr,ImPlotAxisFlags_NoDecorations,ImPlotAxisFlags_NoDecorations);
        ImPlot::SetupAxesLimits(0, len_timeseries - 1, 0, 260.0f, ImGuiCond_Always);
        ImPlot::SetNextLineStyle(ImPlot::GetColormapColor(idx));
        ImPlot::PlotLine("", timeseries, len_timeseries, 1, 0, 0, 0);
        ImPlot::EndPlot();
    }
}

bool rhcp::displayLoadTextureFromFile(const char* filename, GLuint* out_texture, int* out_width, int* out_height){
    
    // Load from file
    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
    if (image_data == NULL)
        return false;

    // Create a OpenGL texture identifier
    GLuint image_texture;

    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same

    // Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    stbi_image_free(image_data);

    *out_texture = image_texture;
    *out_width = image_width;
    *out_height = image_height;

    return true;
}


template <typename T>
inline T rhcp::RandomRange(T min, T max) {
    T scale = rand() / (T) RAND_MAX;
    return min + scale * ( max - min );
}

ImVec4 rhcp::RandomColor() {
    ImVec4 col;
    col.x = RandomRange(0.0f,1.0f);
    col.y = RandomRange(0.0f,1.0f);
    col.z = RandomRange(0.0f,1.0f);
    col.w = 1.0f;
    return col;
}

