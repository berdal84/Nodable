#include "ApplicationView.h"
#include "Core/Texture.h"
#include <imgui/backends/imgui_impl_sdl.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <iostream>
#include "System.h"
#include "Application.h"
#include "NodeView.h"
#include "File.h"
#include "Log.h"
#include "Config.h"

using namespace Nodable;

ApplicationView::ApplicationView(const char* _name, Application* _application):
        application(_application),
        backgroundColor(50, 50, 50),
        isStartupWindowVisible(true),
        isHistoryDragged(false)
{
    add("glWindowName");
    set("glWindowName", _name);

    // Add a member to know if we should display the properties panel or not
    add("showProperties");
    set("showProperties", false);

    add("showImGuiDemo");
    set("showImGuiDemo", false);

}

ApplicationView::~ApplicationView()
{
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext    ();
    SDL_GL_DeleteContext     (glcontext);
    SDL_DestroyWindow        (sdlWindow);
    SDL_Quit                 ();
}

bool ApplicationView::init()
{
    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) != 0)
    {
        LOG_ERROR( "ApplicationView", "SDL Error: %s\n", SDL_GetError());
        return false;
    }

    // Setup window
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_DisplayMode current;
    SDL_GetCurrentDisplayMode(0, &current);
    sdlWindow = SDL_CreateWindow( ((std::string)*get("glWindowName")).c_str(),
                                SDL_WINDOWPOS_CENTERED,
                                SDL_WINDOWPOS_CENTERED,
                                800,
                                600,
                                SDL_WINDOW_OPENGL |
                                SDL_WINDOW_RESIZABLE |
                                SDL_WINDOW_MAXIMIZED |
                                SDL_WINDOW_SHOWN
                                );

    this->glcontext = SDL_GL_CreateContext(sdlWindow);
    SDL_GL_SetSwapInterval(1); // Enable vsync


    // Setup Dear ImGui binding
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
    io.FontAllowUserScaling = true;
	//io.WantCaptureKeyboard  = true;
	//io.WantCaptureMouse     = true;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

    /** Add a paragraph font */
    {
        {
            ImFontConfig config;
            config.OversampleH = 3;
            config.OversampleV = 1;

            //io.Fonts->AddFontDefault();
            auto fontPath = application->getAssetPath("CenturyGothic.ttf").string();
            LOG_MESSAGE("ApplicationView", "Adding font from file: %s\n", fontPath.c_str());
            this->paragraphFont = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 20.0f, &config);
        }

        // Add Icons my merging to previous (paragraphFont) font.
        {
            // merge in icons from Font Awesome
            static const ImWchar icons_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
            ImFontConfig config;
            config.OversampleH = 3;
            config.OversampleV = 1;
            config.MergeMode = true;
            config.PixelSnapH = true;
            config.GlyphMinAdvanceX = 20.0f; // monospace to fix text alignment in drop down menus.
            auto fontPath = application->getAssetPath("fa-solid-900.ttf").string();
            LOG_MESSAGE("ApplicationView", "Adding font from file: %s\n", fontPath.c_str());
            io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 20.0f, &config, icons_ranges);
        }
    }

    /** Add a heading font */
    {
        ImFontConfig config;
        config.OversampleH    = 3;
        config.OversampleV    = 1;

        //io.Fonts->AddFontDefault();
        auto fontPath = application->getAssetPath("CenturyGothic.ttf").string();
        LOG_MESSAGE( "ApplicationView", "Adding font from file: %s\n", fontPath.c_str());
        this->headingFont = io.Fonts->AddFontFromFileTTF( fontPath.c_str(), 25.0f, &config);
    }

    // Configure ImGui Style
    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text]                   = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.21f, 0.21f, 0.21f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.69f, 0.69f, 0.69f, 1.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.66f, 0.66f, 0.66f, 1.00f);
    colors[ImGuiCol_Border]                 = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.30f, 0.30f, 0.30f, 0.50f);
    colors[ImGuiCol_FrameBg]                = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.90f, 0.80f, 0.80f, 1.00f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.90f, 0.65f, 0.65f, 1.00f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.49f, 0.63f, 0.69f, 1.00f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.60f, 0.60f, 0.60f, 0.98f);
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.61f, 0.61f, 0.62f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
    colors[ImGuiCol_CheckMark]              = ImVec4(0.31f, 0.23f, 0.14f, 1.00f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.71f, 0.46f, 0.22f, 0.63f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.71f, 0.46f, 0.22f, 1.00f);
    colors[ImGuiCol_Button]                 = ImVec4(0.50f, 0.50f, 0.50f, 0.63f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.70f, 0.70f, 0.70f, 0.95f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.98f, 0.73f, 0.29f, 0.95f);
    colors[ImGuiCol_Header]                 = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.89f, 0.65f, 0.11f, 0.96f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_Separator]              = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.71f, 0.71f, 0.71f, 0.78f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(1.00f, 0.62f, 0.00f, 1.00f);
    colors[ImGuiCol_ResizeGrip]             = ImVec4(1.00f, 1.00f, 1.00f, 0.30f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(1.00f, 1.00f, 1.00f, 0.60f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(1.00f, 1.00f, 1.00f, 0.90f);
    colors[ImGuiCol_Tab]                    = ImVec4(0.58f, 0.54f, 0.50f, 0.86f);
    colors[ImGuiCol_TabHovered]             = ImVec4(1.00f, 0.79f, 0.45f, 1.00f);
    colors[ImGuiCol_TabActive]              = ImVec4(1.00f, 0.73f, 0.25f, 1.00f);
    colors[ImGuiCol_TabUnfocused]           = ImVec4(0.53f, 0.53f, 0.53f, 0.97f);
    colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.79f, 0.79f, 0.79f, 1.00f);
    colors[ImGuiCol_DockingPreview]         = ImVec4(1.00f, 0.70f, 0.09f, 0.70f);
    colors[ImGuiCol_DockingEmptyBg]         = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_PlotLines]              = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]       = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.00f, 0.00f, 1.00f, 0.35f);
    colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.20f, 0.20f, 0.20f, 0.55f);

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowBorderSize   = 1.0f;
    style.FrameBorderSize    = 1.0f;
	style.FrameRounding      = 3.0f;
    style.ChildRounding      = 3.0f;
    style.WindowRounding     = 0.0f;
	style.AntiAliasedFill    = true;
	style.AntiAliasedLines   = true;
    style.WindowPadding      = ImVec2(10.0f,10.0f);


    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer bindings
    gl3wInit();
    ImGui_ImplSDL2_InitForOpenGL(sdlWindow, glcontext);
    const char* glsl_version = "#version 130";
    ImGui_ImplOpenGL3_Init(glsl_version);

	return true;
}

bool ApplicationView::draw()
{

    // Declare variables that can be modified my mouse and keyboard
    auto userWantsToDeleteSelectedNode(false);
    auto userWantsToArrangeSelectedNodeHierarchy(false);

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        ImGui_ImplSDL2_ProcessEvent(&event);

		switch (event.type)
		{
		case SDL_QUIT:
			application->stopExecution();
			break;

		case SDL_KEYUP:
			auto key = event.key.keysym.sym;

			if ((event.key.keysym.mod & KMOD_LCTRL)) {

				// History
				if (auto file = application->getCurrentFile()) {
					History* currentFileHistory = file->getHistory();
					     if (key == SDLK_z) currentFileHistory->undo();
					else if (key == SDLK_y) currentFileHistory->redo();
				}

				// File
				     if( key == SDLK_s)  application->saveCurrentFile();
				else if( key == SDLK_w)  application->closeCurrentFile();
				else if( key == SDLK_o)  this->browseFile();
			}
			else if (key == SDLK_DELETE )
            {
                userWantsToDeleteSelectedNode = true;
            }
			else if (key == SDLK_a)
            {
                userWantsToArrangeSelectedNodeHierarchy = true;
            }
			else if (key == SDLK_F1 )
            {
                isStartupWindowVisible = true;
            }

			break;
		}

    }

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame(sdlWindow);
	ImGui::NewFrame();
    ImGui::SetCurrentFont(this->paragraphFont);

	// Reset default mouse cursor
	ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);

    // Startup Window
    drawStartupWindow();

    // Demo Window
    {
        auto b = (bool)*get("showImGuiDemo");
        if (b){
            ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver); // Normally user code doesn't need/want to call this because positions are saved in .ini file anyway. Here we just want to make the demo initial state a bit more friendly!
            ImGui::ShowDemoWindow(&b);
            set("showImGuiDemo", b);
        }
    }

    // Fullscreen sdlWindow
    {

		// Get current file's history
		History* currentFileHistory = nullptr;

		if ( auto file = application->getCurrentFile())
			currentFileHistory = file->getHistory();

        static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

        // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
        // because it would be confusing to have two docking targets within each others.
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->GetWorkPos());
        ImGui::SetNextWindowSize(viewport->GetWorkSize());
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;


        // Remove padding
        bool isMainWindowOpen = true;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("Nodable", &isMainWindowOpen, window_flags);
        {
            ImGui::PopStyleVar();

            ImGui::PopStyleVar(2);

            bool redock_all = false;

            drawMenuBar(currentFileHistory, userWantsToDeleteSelectedNode,
                        userWantsToArrangeSelectedNodeHierarchy, redock_all);



            /*
             * Main Layout
             */

            ImGuiID dockspace_id = ImGui::GetID("dockspace_main");
            ImGuiID dockspace_documents = ImGui::GetID("dockspace_documents");
            ImGuiID dockspace_properties = ImGui::GetID("dockspace_properties");


            if ( !this->isLayoutInitialized )
            {
               ImGui::DockBuilderRemoveNode(dockspace_id); // Clear out existing layout
               ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace );
               ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);
               ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.2f, &dockspace_properties, NULL);
               ImGui::DockBuilderDockWindow("Global Props", dockspace_properties);
               ImGui::DockBuilderDockWindow("Properties", dockspace_properties);
               ImGui::DockBuilderDockWindow("File Info", dockspace_properties);
               ImGui::DockBuilderFinish(dockspace_id);
                this->isLayoutInitialized = true;
            }

             /*
             * Fill the layout with content
             */
            ImGui::DockSpace(dockspace_id);

            // Global Props
            if (ImGui::Begin("Global Props"))
            {
                this->drawPropertiesWindow();
                ImGui::ShowStyleEditor();
            }
            ImGui::End();


            // File info
            ImGui::Begin("File Info");
            {
                ImGui::Text("File properties will be there");
                const File* currFile = application->getCurrentFile();

                if (currFile)
                {
                    ImGui::Text("Name: %s", currFile->getName().c_str());
                    ImGui::Text("Path: %s", currFile->getPath().c_str());
                }

                if (ImGui::TreeNode("Language"))
                {
                    this->drawLanguageBrowser(application->getCurrentFile());
                    ImGui::TreePop();
                }

            }
            ImGui::End();

            // Language browser
            if ( ImGui::Begin("Properties") )
            {
                NodeView* view = NodeView::GetSelected();
                if ( view )
                {
                    ImGui::Text("Selected Node Properties");
                    ImGui::NewLine();
                    ImGui::Indent(10.0f);
                    ImGui::Text("Type: %s", view->getOwner()->getLabel());
                    ImGui::NewLine();
                    NodeView::DrawNodeViewAsPropertiesPanel(view);
                }
            }
            ImGui::End();


            // Opened documents
            for (size_t fileIndex = 0; fileIndex < application->getFileCount(); fileIndex++)
            {
                this->drawFileEditor(dockspace_id, redock_all, fileIndex);
            }


        }
        ImGui::End(); // Main window


		/*
		   Perform actions on selected node
		*/

		auto selectedNodeView = NodeView::GetSelected();
		if (selectedNodeView)
		{
			if (userWantsToDeleteSelectedNode)
			{
			    auto node = selectedNodeView->getOwner();
                node->flagForDeletion();
            }
			else if (userWantsToArrangeSelectedNodeHierarchy)
            {
				selectedNodeView->arrangeRecursively();
            }
		}
    }

    drawFileBrowser();

    // Rendering
	ImGui::Render();
	SDL_GL_MakeCurrent(sdlWindow, this->glcontext);
	auto io = ImGui::GetIO();
	glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
	glClearColor(backgroundColor.Value.x, backgroundColor.Value.y, backgroundColor.Value.z, backgroundColor.Value.w);
	glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Update and Render additional Platform Windows
    // (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
    //  For this specific demo app we could also call SDL_GL_MakeCurrent(window, gl_context) directly)
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
        SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
    }

    SDL_GL_SwapWindow(sdlWindow);

    return false;
}

void ApplicationView::drawFileBrowser()
{
    fileBrowser.Display();
    if (fileBrowser.HasSelected())
    {
        auto selectedFiles = fileBrowser.GetMultiSelected();
        for (const auto & selectedFile : selectedFiles)
        {
            application->openFile(selectedFile.string().c_str());
        }
        fileBrowser.ClearSelected();
        fileBrowser.Close();
    }
}

void ApplicationView::drawFileEditor(ImGuiID dockspace_id, bool redock_all, size_t fileIndex)
{
    File *file = application->getFileAtIndex(fileIndex);

    ImGui::SetNextWindowDockID(dockspace_id, redock_all ? ImGuiCond_Always : ImGuiCond_Appearing);
    ImGuiWindowFlags window_flags = (file->isModified() ? ImGuiWindowFlags_UnsavedDocument : 0);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

    auto child_bg = ImGui::GetStyle().Colors[ImGuiCol_ChildBg];
    child_bg.w = 0;

    ImGui::PushStyleColor(ImGuiCol_ChildBg, child_bg);

    bool open = true;
    bool visible = ImGui::Begin(file->getName().c_str(), &open, window_flags);
    {
        ImGui::PopStyleColor(1);
        ImGui::PopStyleVar();

        if (visible)
        {
            const bool isCurrentFile = fileIndex == application->getCurrentFileIndex();

            if ( !isCurrentFile && ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows))
            {
                application->setCurrentFileWithIndex(fileIndex);
            }

            // History bar on top
            drawHistoryBar(file->getHistory());

            // File View in the middle
            View* eachFileView = file->getComponent<View>();
            ImVec2 availSize = ImGui::GetContentRegionAvail();

            if ( isCurrentFile )
            {
                availSize.y -= ImGui::GetTextLineHeightWithSpacing();
            }

            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0,0,0,0.35f) );
            eachFileView->drawAsChild("FileView", availSize);
            ImGui::PopStyleColor();

            // Status bar
            if ( isCurrentFile )
            {
                drawStatusBar();
            }

        }
    }
    ImGui::End(); // File Window

    if (!open)
    {
        application->closeFile(fileIndex);
    }
}

void ApplicationView::drawPropertiesWindow()
{
    ImGui::Text("Wires");
    ImGui::SliderFloat("thickness", &bezierThickness, 0.5f, 10.0f);
    ImGui::SliderFloat("out roundness", &bezierCurveOutRoundness, 0.0f, 1.0f);
    ImGui::SliderFloat("in roundness", &bezierCurveInRoundness, 0.0f, 1.0f);
    ImGui::SliderFloat("connector radius", &connectorRadius, 1.0f, 10.0f);
    ImGui::SliderFloat("node padding", &nodePadding, 1.0f, 20.0f);
    ImGui::Checkbox("arrows", &displayArrows);
}

void ApplicationView::drawStartupWindow() {
    if (isStartupWindowVisible && !ImGui::IsPopupOpen(startupScreenTitle))
    {
        ImGui::OpenPopup(startupScreenTitle);
    }

    ImGui::SetNextWindowSizeConstraints(ImVec2(500,200), ImVec2(500,50000));
    ImGui::SetNextWindowPos( ImGui::GetMainViewport()->GetCenter(), NULL, ImVec2(0.5f,0.5f) );

    auto flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize;

    if ( ImGui::BeginPopupModal(startupScreenTitle, nullptr, flags) )
    {

        std::filesystem::path path(NODABLE_ASSETS_DIR"/nodable-logo-xs.png");
        auto logo = Texture::GetWithPath(path);
        ImGui::SameLine( (ImGui::GetContentRegionAvailWidth() - logo->width) * 0.5f); // center img
        ImGui::Image((void*)(intptr_t)logo->image, ImVec2((float)logo->width, (float)logo->height));

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(25.0f, 20.0f) );
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

        {
            ImGui::PushFont(headingFont);
            ImGui::NewLine();
            ImGui::Text("Nodable is node-able");
            ImGui::PopFont();
            ImGui::NewLine();
        }


        ImGui::TextWrapped("The goal of Nodable is to allow you to edit a computer program in a textual and nodal way at the same time." );

        {
            ImGui::PushFont(headingFont);
            ImGui::NewLine();
            ImGui::Text("Manifest");
            ImGui::PopFont();
            ImGui::NewLine();
        }

        ImGui::TextWrapped( "The nodal and textual points of view each have pros and cons. The user should not be forced to choose one of the two." );

        {
            ImGui::PushFont(headingFont);
            ImGui::NewLine();
            ImGui::Text("Disclaimer");
            ImGui::PopFont();
            ImGui::NewLine();
        }

        ImGui::TextWrapped( "This software is a prototype, use at your own risk." );

        ImGui::NewLine();ImGui::NewLine();

        const char* credit = "berenger@dalle-cort.fr";
        ImGui::SameLine( ImGui::GetContentRegionAvailWidth() - ImGui::CalcTextSize(credit).x);
        ImGui::TextWrapped( credit );

        if (ImGui::IsMouseClicked(0) || ImGui::IsMouseClicked(1) )
        {
            ImGui::CloseCurrentPopup();
            isStartupWindowVisible = false;
        }
        ImGui::PopStyleVar(); // ImGuiStyleVar_FramePadding
        ImGui::EndPopup();
    }

}

void ApplicationView::drawMenuBar(
        History *currentFileHistory,
        bool &userWantsToDeleteSelectedNode,
        bool &userWantsToArrangeSelectedNodeHierarchy,
        bool &redock_all)
{
    if (ImGui::BeginMenuBar()) {

        if (ImGui::BeginMenu("File")) {

            //ImGui::MenuItem(ICON_FA_FILE   "  New", "Ctrl + N");
            if (ImGui::MenuItem(ICON_FA_FOLDER      "  Open", "Ctrl + O")) browseFile();
            if (ImGui::MenuItem(ICON_FA_SAVE        "  Save", "Ctrl + S")) application->saveCurrentFile();
            if (ImGui::MenuItem(ICON_FA_TIMES       "  Close", "Ctrl + W")) application->closeCurrentFile();
            if (ImGui::MenuItem(ICON_FA_SIGN_OUT_ALT"  Quit", "Alt + F4")) application->stopExecution();

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit")) {

            if (currentFileHistory) {
                if (ImGui::MenuItem("Undo", "Ctrl + Z")) currentFileHistory->undo();
                if (ImGui::MenuItem("Redo", "Ctrl + Y")) currentFileHistory->redo();
                ImGui::Separator();
            }

            auto isAtLeastANodeSelected = NodeView::GetSelected() != nullptr;
            userWantsToDeleteSelectedNode |= ImGui::MenuItem("Delete", "Del.", false, isAtLeastANodeSelected);
            userWantsToArrangeSelectedNodeHierarchy |= ImGui::MenuItem("ReArrange nodes", "A", false,
                                                                       isAtLeastANodeSelected);

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            //auto frame = ImGui::MenuItem("Frame All", "F");
            redock_all |= ImGui::MenuItem("Redock documents");

            ImGui::Separator();
            auto detailSimple = ImGui::MenuItem("Minimalist View", "", NodeView::s_viewDetail == ViewDetail_Minimalist);
            auto detailAdvanced = ImGui::MenuItem("Essential View", "", NodeView::s_viewDetail == ViewDetail_Essential);
            auto detailComplex = ImGui::MenuItem("Exhaustive View", "", NodeView::s_viewDetail == ViewDetail_Exhaustive);

            ImGui::Separator();
            auto showProperties = ImGui::MenuItem(ICON_FA_COGS "  Show Properties", "", (bool) *get("showProperties"));
            auto showImGuiDemo = ImGui::MenuItem("Show ImGui Demo", "", (bool) *get("showImGuiDemo"));

            ImGui::Separator();

            if (SDL_GetWindowFlags(sdlWindow) & SDL_WINDOW_FULLSCREEN_DESKTOP) {
                auto toggleFullscreen = ImGui::MenuItem("Fullscreen", "", true);
                if (toggleFullscreen)
                    SDL_SetWindowFullscreen(sdlWindow, 0);
            } else {
                auto toggleFullscreen = ImGui::MenuItem("Fullscreen", "", false);
                if (toggleFullscreen)
                    SDL_SetWindowFullscreen(sdlWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
            }

            ImGui::Separator();

            //if( frame)
            // TODO

            if (detailSimple)
                NodeView::s_viewDetail = ViewDetail_Minimalist;

            if (detailAdvanced)
                NodeView::s_viewDetail = ViewDetail_Essential;

            if (detailComplex)
                NodeView::s_viewDetail = ViewDetail_Exhaustive;

            if (showProperties)
                set("showProperties", !(bool) *get("showProperties"));

            if (showImGuiDemo)
                set("showImGuiDemo", !(bool) *get("showImGuiDemo"));


            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("An issue ?")) {
            if (ImGui::MenuItem("Report on Github.com")) {
                System::OpenURL("https://github.com/berdal84/Nodable/issues");
            }

            if (ImGui::MenuItem("Report by email")) {
                System::OpenURL("mail:berenger@dalle-cort.fr");
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("Show Startup Screen", "F1")) {
                isStartupWindowVisible = true;
            }

            if (ImGui::MenuItem("Browse source code")) {
                System::OpenURL("https://www.github.com/berdal84/nodable");
            }

            if (ImGui::MenuItem("Extern deps. credits")) {
                System::OpenURL("https://github.com/berdal84/nodable#dependencies--credits-");
            }

            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
}

void ApplicationView::drawStatusBar() const {/*
  Status bar
*/

    auto lastLog = Log::GetLastMessage();

    if( lastLog != nullptr )
    {
        ImVec4 statusLineColor;

        switch ( lastLog->verbosity )
        {
            case Log::Verbosity::Error:
                statusLineColor  = ImVec4(0.5f, 0.0f, 0.0f,1.0f);
                break;

            case Log::Verbosity::Warning:
                statusLineColor  = ImVec4(0.5f, 0.0f, 0.0f,1.0f);
                break;

            default:
                statusLineColor  = ImVec4(0.0f, 0.0f, 0.0f,0.5f);
        }

        ImGui::TextColored(statusLineColor, "%s", lastLog->text.c_str());
    }
}

void ApplicationView::drawHistoryBar(History *currentFileHistory) {
    if (currentFileHistory) {

        if (ImGui::IsMouseReleased(0)) {
            isHistoryDragged = false;
        }

//				ImGui::Text(ICON_FA_CLOCK " History: ");

        auto historyButtonSpacing = float(1);
        auto historyButtonHeight = float(10);
        auto historyButtonMaxWidth = float(40);

        auto historySize = currentFileHistory->getSize();
        auto historyCurrentCursorPosition = currentFileHistory->getCursorPosition();
        auto availableWidth = ImGui::GetContentRegionAvailWidth();
        auto historyButtonWidth = fmin(historyButtonMaxWidth,
                                       availableWidth / float(historySize + 1) - historyButtonSpacing);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(historyButtonSpacing, 0));

        for (size_t commandId = 0; commandId <= historySize; commandId++) {
            ImGui::SameLine();

            std::string label("##" + std::to_string(commandId));

            // Draw an highlighted button for the current history position
            if (commandId == historyCurrentCursorPosition) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
                ImGui::Button(label.c_str(), ImVec2(historyButtonWidth, historyButtonHeight));
                ImGui::PopStyleColor();

                // or a simple one for other history positions
            } else
                ImGui::Button(label.c_str(), ImVec2(historyButtonWidth, historyButtonHeight));

            // Hovered item
            if (ImGui::IsItemHovered()) {
                if (ImGui::IsMouseDown(0)) // hovered + mouse down
                {
                    isHistoryDragged = true;
                }

                // Draw command description
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, float(0.8));
                ImGui::BeginTooltip();
                ImGui::Text("%s", currentFileHistory->getCommandDescriptionAtPosition(commandId).c_str());
                ImGui::EndTooltip();
                ImGui::PopStyleVar();
            }

            // When dragging history
            const auto xMin = ImGui::GetItemRectMin().x;
            const auto xMax = ImGui::GetItemRectMax().x;
            if (isHistoryDragged &&
                ImGui::GetMousePos().x < xMax &&
                ImGui::GetMousePos().x > xMin) {
                currentFileHistory->setCursorPosition(commandId); // update history cursor position
            }


        }
        ImGui::PopStyleVar();
    }
}

void Nodable::ApplicationView::browseFile()
{
	fileBrowser.Open();
}

void ApplicationView::drawBackground()
{
    ImGui::BeginChild("background");
    std::filesystem::path path(NODABLE_ASSETS_DIR"/nodable-logo-xs.png");
    auto logo = Texture::GetWithPath(path);

    for( int x = 0; x < 5; x++ )
    {
        for( int y = 0; y < 5; y++ )
        {
            ImGui::SameLine( (ImGui::GetContentRegionAvailWidth() - logo->width) * 0.5f); // center img
            ImGui::Image((void*)(intptr_t)logo->image, ImVec2((float)logo->width, (float)logo->height));
        }
        ImGui::NewLine();
    }
    ImGui::EndChild();

}

void ApplicationView::drawLanguageBrowser(const File* file )const
{
    if (file != nullptr)
    {
        const Language* language = file->getLanguage();
        const auto functions = language->getAllFunctions();
        const Serializer* serializer = language->getSerializer();


        ImGui::Columns(1);
        for(const auto& each_fct : functions )
        {
            auto name = serializer->serialize(each_fct.signature);
            ImGui::Text("%s", name.c_str());
        }
    }
    else
    {
        ImGui::Text("Open a file to see its properties");
    }

}

