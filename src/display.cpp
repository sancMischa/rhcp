#include "display.h"
#include "imgui.h"

const char* posic_names[] = {"POSIC 1", "POSIC 2", "POSIC 3"};

void rhcp::displayPosicTabs(int num_posics, uint16_t posic_data[]){
    
    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
    if (ImGui::BeginTabBar("POSICs", tab_bar_flags)){
        for (int i=0; i<num_posics; i++){
            if (ImGui::BeginTabItem(posic_names[i]))
                {
                    ImGui::Text("%s", posic_names[i]);
                    
                    static float arr[] = { 0.6f, 0.1f, 1.0f, 0.5f, 0.92f, 0.1f, 0.2f };
                    ImGui::PlotLines("Frame Times", arr, IM_ARRAYSIZE(arr));
                    ImGui::EndTabItem();
                }
        }
        ImGui::EndTabBar();
    }

}