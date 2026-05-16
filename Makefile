CXX    = clang++
CC     = clang

# ── Where ImGui lives ─────────────────────────────────────────────────────────
# Assumes you cloned / copied the imgui repo into vendor/imgui next to src/.
# Adjust IMGUI_DIR if you put it somewhere else.
IMGUI_DIR = imgui

CXXFLAGS = -std=c++20 \
           -Iinclude \
           -Iinclude/glad \
           -I/opt/homebrew/include \
           -I$(IMGUI_DIR) \
           -I$(IMGUI_DIR)/backends

CFLAGS   = -Iinclude -Iinclude/glad

LDFLAGS  = -L/opt/homebrew/lib \
           -lglfw \
           -framework OpenGL \
           -framework Cocoa \
           -framework IOKit \
           -framework CoreVideo \
           -lassimp

BUILD = build

# ── Object list ──────────────────────────────────────────────────────────────
OBJS = \
  $(BUILD)/main.o \
  $(BUILD)/glad.o \
  $(BUILD)/imgui.o \
  $(BUILD)/imgui_draw.o \
  $(BUILD)/imgui_tables.o \
  $(BUILD)/imgui_widgets.o \
  $(BUILD)/imgui_demo.o \
  $(BUILD)/imgui_impl_glfw.o \
  $(BUILD)/imgui_impl_opengl3.o

# ── Final binary ─────────────────────────────────────────────────────────────
app: $(OBJS)
	$(CXX) $(OBJS) -o app $(LDFLAGS)

# ── Your code ────────────────────────────────────────────────────────────────
$(BUILD)/main.o: src/main.cpp
	mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -c src/main.cpp -o $(BUILD)/main.o

$(BUILD)/glad.o: src/glad.c
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -c src/glad.c -o $(BUILD)/glad.o

# ── ImGui core ───────────────────────────────────────────────────────────────
$(BUILD)/imgui.o: $(IMGUI_DIR)/imgui.cpp
	mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD)/imgui_draw.o: $(IMGUI_DIR)/imgui_draw.cpp
	mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD)/imgui_tables.o: $(IMGUI_DIR)/imgui_tables.cpp
	mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD)/imgui_widgets.o: $(IMGUI_DIR)/imgui_widgets.cpp
	mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD)/imgui_demo.o: $(IMGUI_DIR)/imgui_demo.cpp
	mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ── ImGui backends (GLFW + OpenGL3) ──────────────────────────────────────────
$(BUILD)/imgui_impl_glfw.o: $(IMGUI_DIR)/backends/imgui_impl_glfw.cpp
	mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD)/imgui_impl_opengl3.o: $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp
	mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ── Utility targets ───────────────────────────────────────────────────────────
clean:
	rm -rf app $(BUILD)

run: app
	./app