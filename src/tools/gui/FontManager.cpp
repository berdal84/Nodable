#include "FontManager.h"
#include "App.h"
#include "Config.h"

using namespace tools;

static FontManager* g_font_manager = nullptr;

FontManager* tools::init_font_manager()
{
    VERIFY(g_font_manager == nullptr, "init_ex called twice?");
    g_font_manager = new FontManager();
    Config* cfg = get_config();
    VERIFY(cfg != nullptr, "Unable to get the configuration. Did you init_ex the config?");
    g_font_manager->init(&cfg->font_manager);
    return g_font_manager;
}

FontManager* tools::get_font_manager()
{
    return g_font_manager;
}

void tools::shutdown_font_manager(FontManager* _manager)
{
    ASSERT(g_font_manager == _manager); // singleton
    delete g_font_manager;
    g_font_manager = nullptr;
}

void FontManager::init(const FontManagerConfig* config)
{
    VERIFY(m_config == nullptr, "init_ex() must be called ONCE");
    m_config = config;

    for (const FontConfig& text_font : config->text)
    {
        load_font(text_font);
    }

    // Assign text_fonts (user might want to change it later, but we need defaults)
    for( int each_slot = 0; each_slot < FontSlot_COUNT; ++each_slot )
    {
        if(auto font = config->defaults[each_slot] )
        {
            m_fonts[each_slot] = get_font(font);
        }
        else
        {
            LOG_WARNING("NodableView", "No default text_font declared for slot #%i, using ImGui's default text_font as fallback\n", each_slot);
            m_fonts[each_slot] = ImGui::GetDefaultFont();
        }
    }
}

ImFont* FontManager::load_font(const FontConfig& font_config)
{
    VERIFY(m_config != nullptr, "init_ex() must be called first");
    VERIFY(m_loaded_fonts.find(font_config.id) == m_loaded_fonts.end(), "use of same key for different fonts is not allowed");

    ImFont*   font     = nullptr;
    auto&     io       = ImGui::GetIO();

    // Create text_font
    {
        ImFontConfig imfont_cfg;
        imfont_cfg.RasterizerMultiply = 1.2f;
        imfont_cfg.OversampleH = 2;
        imfont_cfg.OversampleV = 3;
        tools::Path absolute_path = App::get_asset_path(font_config.path);
        LOG_VERBOSE("NodableView", "Adding text_font from file ... %s\n", absolute_path.c_str());
        font = io.Fonts->AddFontFromFileTTF(absolute_path.string().c_str(), font_config.size * m_config->subsamples, &imfont_cfg);
    }

    // Add Icons my merging to previous text_font.
    if (font_config.icons_enable )
    {
        if(strlen( m_config->icon.path) == 0)
        {
            LOG_WARNING("NodableView", "config().font_manager.icon.path is empty, icons will be \"?\"\n");
            return font;
        }

        // merge in icons text_font
        static const ImWchar icons_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
        ImFontConfig imfont_cfg;

        imfont_cfg.MergeMode   = true;
        imfont_cfg.RasterizerMultiply = 1.2f;
        imfont_cfg.OversampleH = 2;
        imfont_cfg.OversampleV = 3;
        //imfont_cfg.GlyphOffset.y = -(text_font.icons_size - text_font.size)/2.f;
        imfont_cfg.GlyphMinAdvanceX = font_config.icons_size * m_config->subsamples; // monospace to fix text alignment in drop down menus.
        tools::Path absolute_path = App::get_asset_path(m_config->icon.path);
        font = io.Fonts->AddFontFromFileTTF(absolute_path.string().c_str(), font_config.icons_size * m_config->subsamples, &imfont_cfg, icons_ranges);
        LOG_VERBOSE("NodableView", "Merging icons font ...\n");
    }

    font->Scale = 1.0f / m_config->subsamples;

    m_loaded_fonts.insert_or_assign(font_config.id, font);
    LOG_MESSAGE("NodableView", "Font %s added: \"%s\"\n", font_config.id, font_config.path );
    return font;
}

ImFont* FontManager::get_font(FontSlot slot) const
{
    VERIFY(m_config != nullptr, "init_ex() must be called first");
    return m_fonts[slot];
}

ImFont* FontManager::get_font(const char *id)const
{
    VERIFY(m_config != nullptr, "init_ex() must be called first");
    return m_loaded_fonts.at(id );
}

