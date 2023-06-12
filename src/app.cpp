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
#include <string>
#include <unistd.h>

#include <math.h>
#include <time.h>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <cstdio>
#include <sstream>

#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

//---------------------------------------------------------------------
// FUNCTION PROTOTYPES
//---------------------------------------------------------------------

static void glfw_error_callback(int error, const char* description);

// will eventually be deleted once confirm combining bytes works
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
std::string constructPath(const std::string& path, const std::string& imageName);
void processArgs(int argc, char* argv[]);
void resetUptime(std::chrono::steady_clock::time_point& startTime);
std::string getUptime(const std::chrono::steady_clock::time_point& startTime);

//---------------------------------------------------------------------
// GLOBAL VARIABLES
//---------------------------------------------------------------------

const char* rhcp::can_interface_name = "can0";

//---------------------------------------------------------------------
// MAIN
//---------------------------------------------------------------------

int main(int argc, char* argv[]){

    int sock = 0;
    int *s = &sock; // can interface's socket
    struct canfd_frame frame = {.can_id = 0, .len = 0, .flags = 0, .__res0 = 0, .__res1 = 0, .data = {0}}; // without definition, won't work
    struct canfd_frame *read_frame = &frame;

    unsigned int POSIC_MSG_ID = 0x050; //
    const int POSIC_PAST_DPTS = 500; // number of messages to graph, max size of all time you can view
    const int NUM_POSICS = 17;
    const int BYTES_PER_POSIC = 2;
    int posic_timeseries_len = POSIC_PAST_DPTS * NUM_POSICS * BYTES_PER_POSIC;
    int single_posic_timeseries_len = POSIC_PAST_DPTS;
    float POSIC_MAX_VAL = 65535; // each posic is 16 bits

    std::deque<int> posic_deque;
    float single_posic_timeseries[single_posic_timeseries_len] = {0};
    int posic_timeseries[posic_timeseries_len] = {0};

    int can_connected = 0; // can not connected
    int nbytes = -1; // no new data read from can interface
    int can_sock_deinit = 1; // socket deinitialized
    
    std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();

    int hand_img_width, hand_img_height = 0;
    GLuint hand_img_texture = 0;

    std::string executable_path = argv[0];
    std::string image_name = "hand_img_v3_1.png";
    std::string hand_img_path = constructPath(executable_path, image_name);

    ImVec2 window_size;
    float aspect_ratio = 1;

    static ImGuiTableFlags flags = ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV |
                ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable;
    bool test_mode_en[NUM_POSICS] = {0};
    
    float posic_distances[NUM_POSICS] = {0};
    float posic_last_vals[NUM_POSICS] = {0};
    float posic_current_val = 0;
    int posic_dpts_idx = -1;

    rhcp::MyDndItem dnd_posics[NUM_POSICS];
    int count = 0;
    int prescaler = 50; // CAN rx rate division (only read in every "prescaler" number of CAN messages)
    
    // process command line arguments
    processArgs(argc, argv);

    // set drag and drop labels
    for (int i=0; i<NUM_POSICS; i++){
        dnd_posics[i].Label = rhcp::posicLUT(i);
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
    glfwSwapInterval(0); // enabling vsync (1) is SLOW

    // Create window with graphics context
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    rhcp::displayLoadTextureFromFile(hand_img_path, &hand_img_texture, &hand_img_width, &hand_img_height);

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
        ImGui::Text("Welcome to the Robotic Hand Control Panel (RHCP)\n");

        // CAN initialization
        can_connected = rhcp::canIsConnected();
        if(!can_connected){
            ImGui::Text("Device '%s' not connected or correctly initialized, retrying connection to '%s'...\n", rhcp::can_interface_name, rhcp::can_interface_name);
            ImGui::End();
            can_sock_deinit = 1;
        }
        else{
            if(can_sock_deinit){
                can_sock_deinit = rhcp::canInitSocket(s);
                if(can_sock_deinit)
                    return 1; // didn't sucessfully initialize socket
                else{
                    resetUptime(startTime);
                }
            }

            ImGui::Text("Connected on network interface '%s'\n", rhcp::can_interface_name);
            ImGui::Text("Uptime: %s\n", getUptime(startTime).c_str());

            // rendering hand image  
            window_size = ImGui::GetWindowSize(); 
            aspect_ratio = static_cast<float>(hand_img_width) / static_cast<float>(hand_img_height);     
            ImGui::Image((void*)(intptr_t)hand_img_texture, ImVec2(window_size.x - 30, (window_size.x / aspect_ratio)));

            ImGui::End();      

            nbytes = rhcp::canReadFrame(s, read_frame);

            if (nbytes != -1){ // recieved new data
                count++;

                // handle posic frame
                if (read_frame->can_id == POSIC_MSG_ID){
                    // updateTimeseries() accounts for ~0.3s
                    if (count >= prescaler){
                        updateTimeseries(read_frame->data, NUM_POSICS*BYTES_PER_POSIC, posic_timeseries_len, posic_timeseries, &posic_deque);
                        
                        if(posic_dpts_idx != single_posic_timeseries_len-1){
                            posic_dpts_idx++;
                        }
                        
                        
                        count = 0;
                    }
                }
                else{ // should be an else if with id check like above (future proof))
                    // handle tactile sensor frame as above
                }
            }

            ImGui::Begin("Sensor Graphs Window");
            ImGui::SeparatorText("Sensor Graphs");

            // plot posic table
            if (ImGui::BeginTable("##table", 5, flags, ImVec2(-1,0))){
                ImGui::TableSetupColumn("Sensor", ImGuiTableColumnFlags_WidthFixed, 75.0f);
                ImGui::TableSetupColumn("50kHz Test", ImGuiTableColumnFlags_WidthFixed, 75.0f);
                ImGui::TableSetupColumn("Latest Value", ImGuiTableColumnFlags_WidthFixed, 75.0f);
                ImGui::TableSetupColumn("Total Distance", ImGuiTableColumnFlags_WidthFixed, 75.0f);
                ImGui::TableHeadersRow();
                ImPlot::PushColormap(ImPlotColormap_Cool);

                for (int i=0; i<NUM_POSICS; i++){ // get data for each posic

                    getSensorTimeseries(posic_timeseries, POSIC_PAST_DPTS, NUM_POSICS, BYTES_PER_POSIC, i, single_posic_timeseries);
                    
                    // if (i==0){ // debugging first posic timeseries
                    //     printf("POSIC %d 16bit Timeseries: ", i);
                    //     for (int j=0; j<single_posic_timeseries_len; j++){
                    //         printf("%.0f ", single_posic_timeseries[j]);
                    //     }
                    //     printf("\n");
                    // }

                    posic_current_val = single_posic_timeseries[posic_dpts_idx];
                    rhcp::displayUpdatePosicParams(posic_distances, posic_last_vals, posic_current_val, i);
                    

                    rhcp::displayPosicTablePlot(i, 
                                            single_posic_timeseries, 
                                            single_posic_timeseries_len, 
                                            test_mode_en,
                                            posic_distances,
                                            posic_current_val,
                                            POSIC_MAX_VAL);
                    dnd_posics[i].data.assign(single_posic_timeseries, single_posic_timeseries+single_posic_timeseries_len);
                    // assign() is iteration based, could make this faster

                }
                
                ImPlot::PopColormap();
                ImGui::EndTable();
            }
            // count num

            // plot posic drag-n-drop
            rhcp::displayDragAndDrop(dnd_posics, NUM_POSICS, single_posic_timeseries_len, POSIC_MAX_VAL);

            // TODO: CAN communication with dorsal based on test_mode_en status

            // plot tactile sensor table
            // for (int i=0; i<NUM_XELAS, i++){
            //     getSensorTimeseries(xela stuff);

            //     if (ImGui::TreeNode("XELA Data")){
            //         // display xela data
            //         ImGui::TreePop();
            //     }
            // }

            // plot tactile sensor drag-n-drop

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

void getSensorTimeseries(int timeseries_flattened[],
                            int dim1_elements,
                            int dim2_elements,
                            int dim3_elements,
                            int dim2_idx,
                            float out[])
{
    for (int i = 0; i < dim1_elements; i++) {
        int byte1 = timeseries_flattened[i * dim2_elements * dim3_elements + dim2_idx * dim3_elements];
        int byte2 = timeseries_flattened[i * dim2_elements * dim3_elements + dim2_idx * dim3_elements + 1];
        int combinedBytes = (byte1 << 8) | byte2;
        out[i] = (float)combinedBytes;
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
    
    // could modify fn to take in a vector, then assign that vector to the timeseries deque, then 
    // plot that vector directly
    if(count_dpts<timeseries_len){ 
        temp_vec = std::vector<int>(timeseries_len, 0);  // Init all values to zero
        temp_vec.assign(timeseries->begin(), timeseries->end());  // Copy elements from deque
        count_dpts+=data_len;
    }
    else{
        temp_vec = std::vector<int>(timeseries->begin(), timeseries->end());
    }

    int* temp_ptr = &temp_vec[0];
    std::copy(temp_ptr, temp_ptr + timeseries_len, timeseries_flattened); // linear time operation

    // printf("Flattened Timeseries: \n");
    // for (int i = 0; i < timeseries_len; i++)
    //     printf("%02X ",timeseries_flattened[i]);
    // printf("\r\n");

}

// pushes an item to a queue of fixed length
void fixedPushBack(std::deque<int> *q, int max_len, int val){
    if (q->size() == (unsigned long int)max_len){
        q->pop_front();
    }
    q->push_back(val);
}

std::string constructPath(const std::string& path, const std::string& imageName) {
    std::string executablePath = path;
    std::string executableDirectory = "";
    std::string imagePath = "";

    // Get the directory of the executable
    size_t lastSlashPos = executablePath.find_last_of('/');
    if (lastSlashPos != std::string::npos) {
        executableDirectory = executablePath.substr(0, lastSlashPos);
    }

    // Construct the image path
    if (!executableDirectory.empty()) {
        imagePath = executableDirectory + "/../lib/img/" + imageName;
    }
    
    return imagePath;

}

void processArgs(int argc, char* argv[]){

    if (argc > 1){
        printf("argc = %d\n", argc);
        if (strcmp(argv[1], "-d") == 0){
            rhcp::can_interface_name = argv[2];
        }
    }

}

void resetUptime(std::chrono::steady_clock::time_point& startTime) {
    startTime = std::chrono::steady_clock::now();
}

std::string getUptime(const std::chrono::steady_clock::time_point& startTime) {
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    std::chrono::steady_clock::duration duration = now - startTime;
    long long totalSeconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();

    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int seconds = totalSeconds % 60;

    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(2) << hours << ":"
        << std::setfill('0') << std::setw(2) << minutes << ":"
        << std::setfill('0') << std::setw(2) << seconds;

    return oss.str();
}
