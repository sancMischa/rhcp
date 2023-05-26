#CXX = g++
#CXX = clang++

SRC = src
BIN = bin
IMGUI_DIR = lib
INC = inc
OBJ = $(BIN)/obj

EXE_DEMO = demo
EXE_APP = app
EXE_TEST = test

# SOURCES = $(IMGUI_DIR)/imgui/imgui.cpp $(IMGUI_DIR)/imgui/imgui_demo.cpp $(IMGUI_DIR)/imgui/imgui_draw.cpp $(IMGUI_DIR)/imgui/imgui_tables.cpp $(IMGUI_DIR)/imgui/imgui_widgets.cpp
# SOURCES += $(IMGUI_DIR)/backends/imgui_impl_glfw.cpp $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp

# SOURCES_APP = $(SRC)/app.cpp $(SRC)/can.cpp $(SRC)/display.cpp
# SOURCES_APP += $(SOURCES)

SOURCES = imgui.cpp imgui_demo.cpp imgui_draw.cpp imgui_tables.cpp imgui_widgets.cpp
SOURCES += imgui_impl_glfw.cpp imgui_impl_opengl3.cpp
SOURCES += implot_items.cpp implot.cpp implot_demo.cpp

SOURCES_APP = app.cpp can.cpp display.cpp 
SOURCES_APP += $(SOURCES)

SOURCES_EX = main.cpp
SOURCES_EX += $(SOURCES)

SOURCES_TEST = test.cpp can.cpp display.cpp
SOURCES_TEST += $(SOURCES)

OBJS_EX = $(SOURCES_EX:%.cpp=$(OBJ)/%.o)
OBJS_APP = $(SOURCES_APP:%.cpp=$(OBJ)/%.o)
OBJS_TEST = $(SOURCES_TEST:%.cpp=$(OBJ)/%.o)

# OBJS_EX = $(addsuffix .o, $(basename $(notdir $(SOURCES_EX))))
# OBJS_APP = $(addsuffix .o, $(basename $(notdir $(SOURCES_APP))))

UNAME_S := $(shell uname -s)
LINUX_GL_LIBS = -lGL

CXXFLAGS = -std=c++11 -I$(IMGUI_DIR)/imgui -I$(IMGUI_DIR)/backends -I$(IMGUI_DIR)/implot -I$(INC)
CXXFLAGS += -g -Wall -Wformat
LIBS =

##---------------------------------------------------------------------
## BUILD FLAGS PER PLATFORM
##---------------------------------------------------------------------

ifeq ($(UNAME_S), Linux) #LINUX
	ECHO_MESSAGE = "Linux"
	LIBS += $(LINUX_GL_LIBS) `pkg-config --static --libs glfw3`

	CXXFLAGS += `pkg-config --cflags glfw3`
	CFLAGS = $(CXXFLAGS)
endif


##---------------------------------------------------------------------
## BUILD RULES
##---------------------------------------------------------------------

$(OBJ)/%.o:$(SRC)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJ)/%.o:$(IMGUI_DIR)/imgui/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJ)/%.o:$(IMGUI_DIR)/backends/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJ)/%.o:$(IMGUI_DIR)/implot/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

.PHONY: app demo clean test

test: $(BIN)/$(EXE_TEST)
	@echo Build complete for $(ECHO_MESSAGE)

app: $(BIN)/$(EXE_APP)
	@echo Build complete for $(ECHO_MESSAGE)

demo: $(BIN)/$(EXE_DEMO)
	@echo Build complete for $(ECHO_MESSAGE)

$(BIN)/$(EXE_APP): $(OBJS_APP)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)

$(BIN)/$(EXE_DEMO): $(OBJS_EX)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)

$(BIN)/$(EXE_TEST): $(OBJS_TEST)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)

# clean_bin:
# 	rm -f $(BIN)

clean:
	rm -f $(BIN)/$(EXE_APP) $(BIN)/$(EXE_DEMO) $(OBJS_APP) $(OBJS_EX) $(OBJS_TEST)
