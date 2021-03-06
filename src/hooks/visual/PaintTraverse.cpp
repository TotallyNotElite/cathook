/*
  Created by Jenny White on 29.04.18.
  Copyright (c) 2018 nullworks. All rights reserved.
*/

#include <settings/Registered.hpp>
#include <MiscTemporary.hpp>
#include "HookedMethods.hpp"
#include "CatBot.hpp"

static settings::Bool pure_bypass{ "visual.sv-pure-bypass", "false" };
static settings::Int software_cursor_mode{ "visual.software-cursor-mode", "0" };

static settings::Int waittime{ "debug.join-wait-time", "2500" };
static settings::Bool no_reportlimit{ "misc.no-report-limit", "false" };

int spamdur = 0;
Timer joinspam{};
CatCommand join_spam("join_spam", "Spam joins server for X seconds", [](const CCommand &args) {
    if (args.ArgC() < 1)
        return;
    int id = atoi(args.Arg(1));
    joinspam.update();
    spamdur = id;
});
CatCommand join("mm_join", "Join mm Match", []() {
    auto gc = re::CTFGCClientSystem::GTFGCClientSystem();
    if (gc)
        gc->JoinMMMatch();
});
void *pure_orig  = nullptr;
void **pure_addr = nullptr;

// static CatVar disable_ban_tf(CV_SWITCH, "disable_mm_ban", "0", "Disable MM
// ban", "Disable matchmaking ban");

bool replaced = false;
namespace hooked_methods
{
Timer checkmmban{};
DEFINE_HOOKED_METHOD(PaintTraverse, void, vgui::IPanel *this_, unsigned int panel, bool force, bool allow_force)
{
    static bool textures_loaded        = false;
    static unsigned long panel_scope   = 0;
    static unsigned long motd_panel    = 0;
    static unsigned long motd_panel_sd = 0;
    static bool call_default           = true;
    static bool cur;
    static ConVar *software_cursor = g_ICvar->FindVar("cl_software_cursor");
    static const char *name;

#if ENABLE_VISUALS
    if (!textures_loaded)
        textures_loaded = true;
#endif
    static bool switcherido = false;
    static int scndwait     = 0;
    if (scndwait > int(waittime))
    {
        scndwait = 0;
        if (switcherido && spamdur && !joinspam.check(spamdur * 1000))
        {
            auto gc = re::CTFGCClientSystem::GTFGCClientSystem();
            if (gc)
                gc->JoinMMMatch();
        }
        else if (!joinspam.check(spamdur * 1000) && spamdur)
        {
            INetChannel *ch = (INetChannel *) g_IEngine->GetNetChannelInfo();
            if (ch)
                ch->Shutdown("");
        }
    }
    scndwait++;
    switcherido = !switcherido;
#if not ENABLE_VISUALS
    if (checkmmban.test_and_set(1000))
    {
        if (tfmm::isMMBanned())
        {
            *(int *) nullptr = 0;
            exit(1);
        }
    }
#endif
    /*
    static bool replacedban = false;
    if (disable_ban_tf && !replacedban)
    {
        static unsigned char patch[] = { 0x31, 0xe0 };
        static unsigned char patch2[] = { 0xb0, 0x01, 0x90 };
        uintptr_t addr = gSignatures.GetClientSignature("31 C0 5B 5E 5F 5D C3 8D
    B6 00 00 00 00 BA");
        uintptr_t addr2 = gSignatures.GetClientSignature("0F 92 C0 83 C4 ? 5B 5E
    5F 5D C3 8D B4 26 00 00 00 00 83 C4");
        if (addr && addr2)
        {
            logging::Info("MM Banned: 0x%08x, 0x%08x", addr, addr2);
            Patch((void*) addr, (void *) patch, sizeof(patch));
            Patch((void*) addr2, (void *) patch2, sizeof(patch2));
            replacedban = true;
        }
        else
            logging::Info("No disable ban Signature");

    }*/
    if (no_reportlimit && !replaced)
    {
        static BytePatch no_report_limit(gSignatures.GetClientSignature, "55 89 E5 57 56 53 81 EC ? ? ? ? 8B 5D ? 8B 7D ? 89 D8", 0x75, { 0xB8, 0x01, 0x00, 0x00, 0x00 });
        no_report_limit.Patch();
        replaced = true;
    }
    if (pure_bypass)
    {
        if (!pure_addr)
        {
            pure_addr = *reinterpret_cast<void ***>(gSignatures.GetEngineSignature("A1 ? ? ? ? 85 C0 74 ? C7 44 24 ? ? ? ? ? 89 04 24") + 1);
        }
        if (*pure_addr)
            pure_orig = *pure_addr;
        *pure_addr = (void *) 0;
    }
    else if (pure_orig)
    {
        *pure_addr = pure_orig;
        pure_orig  = (void *) 0;
    }
    call_default = true;
    if (isHackActive() && (panel_scope || motd_panel || motd_panel_sd) && ((no_zoom && panel == panel_scope) || (hacks::shared::catbot::catbotmode && hacks::shared::catbot::anti_motd && (panel == motd_panel || panel == motd_panel_sd))))
        call_default = false;

    if (software_cursor_mode)
    {
        cur = software_cursor->GetBool();
        switch (*software_cursor_mode)
        {
        case 1:
            if (!software_cursor->GetBool())
                software_cursor->SetValue(1);
            break;
        case 2:
            if (software_cursor->GetBool())
                software_cursor->SetValue(0);
            break;
#if ENABLE_GUI
/*
        case 3:
            if (cur != g_pGUI->Visible()) {
                software_cursor->SetValue(g_pGUI->Visible());
            }
            break;
        case 4:
            if (cur == g_pGUI->Visible()) {
                software_cursor->SetValue(!g_pGUI->Visible());
            }
*/
#endif
        }
    }
    if (call_default)
        original::PaintTraverse(this_, panel, force, allow_force);
    // To avoid threading problems.

    // logging::Info("Panel name: %s", g_IPanel->GetName(panel));
    if (!panel_scope)
        if (!strcmp(g_IPanel->GetName(panel), "HudScope"))
            panel_scope = panel;
    if (!motd_panel)
        if (!strcmp(g_IPanel->GetName(panel), "info"))
            motd_panel = panel;
    if (!motd_panel_sd)
        if (!strcmp(g_IPanel->GetName(panel), "ok"))
            motd_panel_sd = panel;
    if (!g_IEngine->IsInGame())
    {
        g_Settings.bInvalid = true;
    }

    if (disable_visuals)
        return;

    if (clean_screenshots && g_IEngine->IsTakingScreenshot())
        return;
#if ENABLE_GUI
// FIXME
#endif
    draw::UpdateWTS();
}
} // namespace hooked_methods
