//---------------------------------------------------------------------
// INCLUDES
//---------------------------------------------------------------------

#include "display.h"
#include "can.h"
#include "imgui.h"
#include "implot.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <stdio.h>
#include <stdlib.h>
#include <deque>
#include <vector>

#include <math.h>
#include <time.h>
#include <iostream>

#include <sys/stat.h>

#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

//---------------------------------------------------------------------
// FUNCTION PROTOTYPES
//---------------------------------------------------------------------

static void glfw_error_callback(int error, const char* description);

void getSensorTimeseries(int timeseries_flattened[],
                            int dim1_elements,
                            int dim2_elements, 
                            int dim3_elements,
                            int dim2_idx,  
                            float out[]);

void updateTimeseries(uint8_t data[], 
                        int data_len, 
                        int timeseries_len,
                        int timeseries_flattened[], 
                        std::deque<int> *timeseries);

void fixedPushBack(std::deque<int> *q, int max_len, int val);

inline bool fileExists (const std::string& name);

//---------------------------------------------------------------------
// MAIN
//---------------------------------------------------------------------

int main(){

    int sock = 0;
    int *s = &sock; // can interface's socket
    struct canfd_frame frame = {.can_id = 0, .len = 0, .flags = 0, .__res0 = 0, .__res1 = 0, .data = {0}}; // without definition, won't work
    struct canfd_frame *read_frame = &frame;

    unsigned int POSIC_MSG_ID = 0x44; //
    const int POSIC_PAST_DPTS = 7; // number of messages to graph
    const int NUM_POSICS = 17;
    const int BYTES_PER_POSIC = 2;
    int posic_timeseries_len = POSIC_PAST_DPTS * NUM_POSICS * BYTES_PER_POSIC;
    int single_posic_timeseries_len = POSIC_PAST_DPTS*BYTES_PER_POSIC;
    float POSIC_MAX_VAL = 250;

    std::deque<int> posic_deque;
    float single_posic_timeseries[single_posic_timeseries_len] = {0};
    int posic_timeseries[posic_timeseries_len] = {0};

    int can_disconnected = 1; // not connected
    int socket_deinit = 1; // not initialized
    int nbytes = -1; // no new data read from can interface

    int hand_img_width, hand_img_height = 0;
    GLuint hand_img_texture = 0;
    const char* hand_img_path = "./lib/img/hand_img.jpg";

    static ImGuiTableFlags flags = ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV |
                ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable;
    int test_mode_en[NUM_POSICS] = {0};

    rhcp::MyDndItem dnd_posics[NUM_POSICS];
    
    // set drag and drop labels
    for (int i=0; i<NUM_POSICS; i++){
        snprintf(dnd_posics[i].Label, sizeof(dnd_posics[i].Label), "POSIC %d", i+1); // set label
    }

    // GLFW Initilization
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;
    
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(1280, 720, "App", nullptr, nullptr);
    if (window == nullptr)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Create window with graphics context
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        // Poll and handle events (inputs, window resize, etc.)
        glfwPollEvents();

        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // app code
        ImGui::Begin("Hand Picture Window");
        ImGui::SeparatorText("RHCP Details");
        ImGui::Text("Description of GUI here");
        
        // rendering hand image
        rhcp::displayLoadTextureFromFile(hand_img_path, &hand_img_texture, &hand_img_width, &hand_img_height);
        // bool ret = rhcp::displayLoadTextureFromFile(hand_img_path, &hand_img_texture, &hand_img_width, &hand_img_height);
        // IM_ASSERT(ret); // interferes with debugging
        
        ImGui::Image((void*)(intptr_t)hand_img_texture, ImVec2(hand_img_width, hand_img_height));

        // can connection checking
        can_disconnected = rhcp::canIsConnected();

        if(can_disconnected){
            socket_deinit = 1;
            // printf("CAN Status (0 conn, 1 disconn, -1 err /proc/net/dev)): %d\n", can_disconnected);
            ImGui::Text("No CAN device found.\n");
            ImGui::End();
        }
        else{ // CAN connected
            ImGui::End();
            if(socket_deinit){ // initialize socket
                if(rhcp::canInitSocket(s)!=0)
                    return 1;
                socket_deinit = 0;
                printf("initialized socket\n");
            }      

            nbytes = rhcp::canReadFrame(s, read_frame);
            
            if (nbytes != -1){ // recieved new data

                // handle posic frame
                if (read_frame->can_id == POSIC_MSG_ID){
                    updateTimeseries(read_frame->data, read_frame->len, posic_timeseries_len, posic_timeseries, &posic_deque);
                }
                else{
                    // handle xela frame as above
                }
            }

            ImGui::Begin("Sensor Graphs Window");
            ImGui::SeparatorText("Sensor Graphs");

            // plot posic table
            if (ImGui::BeginTable("##table", 3, flags, ImVec2(-1,0))){
                ImGui::TableSetupColumn("Sensor", ImGuiTableColumnFlags_WidthFixed, 75.0f);
                ImGui::TableSetupColumn("50kHz Test", ImGuiTableColumnFlags_WidthFixed, 75.0f);
                ImGui::TableHeadersRow();
                ImPlot::PushColormap(ImPlotColormap_Cool);
                
                for (int i=0; i<NUM_POSICS; i++){ // get data for each posic
                    getSensorTimeseries(posic_timeseries, POSIC_PAST_DPTS, NUM_POSICS, BYTES_PER_POSIC, i, single_posic_timeseries);
                
                    rhcp::displayTablePlot(i, single_posic_timeseries, single_posic_timeseries_len, test_mode_en, POSIC_MAX_VAL);
                    dnd_posics[i].data.assign(single_posic_timeseries, single_posic_timeseries+single_posic_timeseries_len);
                    // assign() is iteration based, could make this faster

                }
                
                ImPlot::PopColormap();
                ImGui::EndTable();
            }

            // plot posic drag-n-drop
            rhcp::displayDragAndDrop(dnd_posics, NUM_POSICS, single_posic_timeseries_len, POSIC_MAX_VAL);

            // plot xela table
            // for (int i=0; i<NUM_XELAS, i++){
            //     getSensorTimeseries(xela stuff);

            //     if (ImGui::TreeNode("XELA Data")){
            //         // display xela data
            //         ImGui::TreePop();
            //     }
            // }

            // plot xela drag-n-drop

            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);

    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    if(rhcp::canCloseSocket(s)!=0){
        return 1;
    }

    return 0;
}


//---------------------------------------------------------------------
// HELPER FUNCTIONS
//---------------------------------------------------------------------

static void glfw_error_callback(int error, const char* description){
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

// retrieves timeseries data from one sensor
// TODO: each posic is 2 bytes of data --> need to put these two bytes into one number
// that will be added to the float out[] array 
void getSensorTimeseries(int timeseries_flattened[],
                            int dim1_elements,
                            int dim2_elements, 
                            int dim3_elements,
                            int dim2_idx,  
                            float out[])
{
    int timeseries_idx;

    for (int i=0; i<dim1_elements; i++){
       timeseries_idx = i * (dim2_elements * dim3_elements) + dim2_idx * dim3_elements;

        for (int j=0; j<dim3_elements; j++){
            out[i * dim3_elements + j] = timeseries_flattened[timeseries_idx + j];
        }
    }
}

// push items to queue and update the associated flattened array
void updateTimeseries(uint8_t data[], 
                        int data_len, 
                        int timeseries_len,
                        int timeseries_flattened[],
                        std::deque<int> *timeseries)
{
    static int count_dpts = 0;
    std::vector<int> temp_vec;

    for (int i=0; i<data_len; i++){
        fixedPushBack(timeseries, timeseries_len, data[i]);
    }
    
    // modify fn to take in a vector, then assign that vector to the timeseries deque, then 
    // plot that vector directly
    if(count_dpts<timeseries_len){ 
        temp_vec = std::vector<int>(timeseries_len, 0);  // Init all values to zero
        temp_vec.assign(timeseries->begin(), timeseries->end());  // Copy elements from deque
        count_dpts++;
    }
    else{
        temp_vec = std::vector<int>(timeseries->begin(), timeseries->end());
    }

    // printf("Address of timeseries_flattened: %d\n", posic_timeseries);
    // timeseries_flattened = &temp_vec[0]; // improper use of address reassignment
    int* temp_ptr = &temp_vec[0];
    std::copy(temp_ptr, temp_ptr + timeseries_len, timeseries_flattened); // linear time operation
    // printf("Address of timeseries_flattened after vector reassignment: %d\n", posic_timeseries);

    // printf("Address of timeseries_flattened: %d\n", timeseries_flattened);
    // timeseries_flattened = &temp_vec[0];
    // printf("Address of timeseries_flattened after vector reassignment: %d\n", timeseries_flattened);

}

// pushes an item to a queue of fixed length
void fixedPushBack(std::deque<int> *q, int max_len, int val){
    if (q->size() == (unsigned long int)max_len){ // ensure this comparison still works
        q->pop_front();
    }
    q->push_back(val);
}

inline bool fileExists (const std::string& name) {
  struct stat buffer;   
  return (stat (name.c_str(), &buffer) == 0); 
}