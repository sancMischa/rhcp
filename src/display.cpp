#include "display.h"
#include <stdio.h>
#include <math.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace rhcp{

    MyDndItem::MyDndItem(){
        Idx = 0;
        Plt = 0;
        Yax = ImAxis_Y1;
        Color = rhcp::RandomColor();
    }

    void MyDndItem::Reset(){ 
        Plt = 0; Yax = ImAxis_Y1; 
    }

}

void rhcp::displayDragAndDrop(MyDndItem dnd_items[], int num_dnd_items, int dataseries_len, float ymax) {

    bool reset_axes = false;

    ImGui::Begin("DragnDrop Window");

    // child window to serve as initial source for our DND items
    ImGui::BeginChild("DND_LEFT",ImVec2(100,400));
    if (ImGui::Button("Clear Graph")) {
        for (int k = 0; k < num_dnd_items; ++k)
            dnd_items[k].Reset();
    }
    for (int k = 0; k < num_dnd_items; ++k) {
        if (dnd_items[k].Plt > 0)
            continue;
        ImPlot::ItemIcon(dnd_items[k].Color); ImGui::SameLine();
        ImGui::Selectable(dnd_items[k].Label, false, 0, ImVec2(100, 0));
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
            ImGui::SetDragDropPayload("MY_DND", &k, sizeof(int));
            ImPlot::ItemIcon(dnd_items[k].Color); ImGui::SameLine();
            ImGui::TextUnformatted(dnd_items[k].Label);
            ImGui::EndDragDropSource();
        }
    }
    ImGui::EndChild(); // dnd source window

    // dnd accepting window (graphs)
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MY_DND")) {
            int i = *(int*)payload->Data; dnd_items[i].Reset();
        }
        ImGui::EndDragDropTarget();
    }
    ImGui::SameLine();
    ImGui::BeginChild("DND_RIGHT",ImVec2(-1,400));
    ImPlotAxisFlags flags = ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoGridLines | ImPlotAxisFlags_NoHighlight;

    if (ImGui::Button("Reset Scale")){
        reset_axes = true;
    }

    if (reset_axes){
        ImPlot::SetNextAxesLimits(0, dataseries_len-1, 0, ymax, ImGuiCond_Always);
        reset_axes = false;
    }

    if (ImPlot::BeginPlot("##DND1", ImVec2(-1,195))) {

        ImPlot::SetupAxesLimits(0, dataseries_len-1, 0, ymax); // ImGuiCond_Always prevents zooming (although the zoom action is still available)

        ImPlot::SetupAxis(ImAxis_X1, nullptr, flags);
        ImPlot::SetupAxis(ImAxis_Y1, "[drop here]", flags);
        ImPlot::SetupAxis(ImAxis_Y2, "[drop here]", flags|ImPlotAxisFlags_Opposite);
        ImPlot::SetupAxis(ImAxis_Y3, "[drop here]", flags|ImPlotAxisFlags_Opposite);

        for (int k = 0; k < num_dnd_items; ++k) {
            if (dnd_items[k].Plt == 1 && dnd_items[k].data.size() > 0) {
                ImPlot::SetAxis(dnd_items[k].Yax);
                ImPlot::SetNextLineStyle(dnd_items[k].Color);
                // unsure about parameters in this next function
                ImPlot::PlotLine(dnd_items[k].Label, &dnd_items[k].data[0], dataseries_len, 1, 0, 0, 0);
                // allow legend item labels to be DND sources
                if (ImPlot::BeginDragDropSourceItem(dnd_items[k].Label)) {
                    ImGui::SetDragDropPayload("MY_DND", &k, sizeof(int));
                    ImPlot::ItemIcon(dnd_items[k].Color); ImGui::SameLine();
                    ImGui::TextUnformatted(dnd_items[k].Label);
                    ImPlot::EndDragDropSource();
                }
            }
        }
        // allow the main plot area to be a DND target
        if (ImPlot::BeginDragDropTargetPlot()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MY_DND")) {
                int i = *(int*)payload->Data; dnd_items[i].Plt = 1; dnd_items[i].Yax = ImAxis_Y1;
            }
            ImPlot::EndDragDropTarget();
        }
        // allow each y-axis to be a DND target
        for (int y = ImAxis_Y1; y <= ImAxis_Y3; ++y) {
            if (ImPlot::BeginDragDropTargetAxis(y)) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MY_DND")) {
                    int i = *(int*)payload->Data; dnd_items[i].Plt = 1; dnd_items[i].Yax = y;
                }
                ImPlot::EndDragDropTarget();
            }
        }
        // allow the legend to be a DND target
        if (ImPlot::BeginDragDropTargetLegend()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MY_DND")) {
                int i = *(int*)payload->Data; dnd_items[i].Plt = 1; dnd_items[i].Yax = ImAxis_Y1;
            }
            ImPlot::EndDragDropTarget();
        }
        ImPlot::EndPlot();
    }
    
    ImGui::EndChild();

    ImGui::End();
}

// to be called in loop, doesn't include begin table or end table
void rhcp::displayTablePlot(int idx, float timeseries[], int len_timeseries, bool checkbox_status[], float ymax){


    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("POSIC %d", idx);
    ImGui::TableSetColumnIndex(1);
    ImGui::PushID(idx); // https://github.com/ocornut/imgui/blob/master/docs/FAQ.md#q-about-the-id-stack-system
    ImGui::Checkbox("##0n", &checkbox_status[idx]); // Label = "", ID = hash of ("Window name", i, "#00n") - unique
    ImGui::PopID();
    ImGui::TableSetColumnIndex(2);
    ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(0,0)); // no graph padding
    if (ImPlot::BeginPlot("",ImVec2(-1, 30),ImPlotFlags_CanvasOnly|ImPlotFlags_NoChild)) {
        ImPlot::SetupAxes(nullptr,nullptr,ImPlotAxisFlags_NoDecorations,ImPlotAxisFlags_NoDecorations);
        ImPlot::SetupAxesLimits(0, len_timeseries - 1, 0, ymax, ImGuiCond_Always);
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

