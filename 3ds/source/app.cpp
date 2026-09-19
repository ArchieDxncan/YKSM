/*
 * YKSM is based on PKSM's GPLv3 application framework.
 * Copyright (C) 2016-2025 PKSM contributors
 * Copyright (C) 2026 YKSM contributors
 */
#include "app.hpp"
#include "Archive.hpp"
#include "gui.hpp"
#include "ScreenStack.hpp"
#include "Subsystems.hpp"
#include "thread.hpp"
#include "YokaiTitleSelectScreen.hpp"
#include "utils/logging.hpp"
#include <3ds.h>
#include <memory>

namespace
{
    u32 oldTimeLimit = UINT32_MAX;
    pksm::Subsystems subsystems;
}

Result App::init(const std::string& execPath)
{
    const bool ready =
        subsystems.acquire("hid", hidInit, hidExit) &&
        subsystems.acquire("gfx", gfxInitDefault, gfxExit) &&
        subsystems.acquire("logging", Logging::init, Logging::exit) &&
        subsystems.acquire(
            "cpu",
            []
            {
                APT_GetAppCpuTimeLimit(&oldTimeLimit);
                APT_SetAppCpuTimeLimit(30);
            },
            []
            {
                if (oldTimeLimit != UINT32_MAX) APT_SetAppCpuTimeLimit(oldTimeLimit);
            }) &&
        subsystems.acquire("cfgu", cfguInit, cfguExit) &&
        subsystems.acquire("romfs", romfsInit, romfsExit) &&
        subsystems.acquire("archive", [&execPath] { return Archive::init(execPath); }, Archive::exit) &&
        subsystems.acquire("threads", [] { return Threads::init(0, 2); }, Threads::exit) &&
        subsystems.acquire("am", amInit, amExit) &&
        subsystems.acquire("gui", Gui::init, Gui::exit);

    if (!ready)
    {
        const auto failure = subsystems.failure();
        return failure ? failure->status : -1;
    }

    Logging::detachConsole();
    gfxSetScreenFormat(GFX_TOP, GSP_BGR8_OES);
    gfxSetDoubleBuffering(GFX_TOP, true);
    gfxSetScreenFormat(GFX_BOTTOM, GSP_BGR8_OES);
    gfxSetDoubleBuffering(GFX_BOTTOM, true);
    gfxSwapBuffersGpu();
    gspWaitForVBlank();
    ScreenStack::push(std::make_unique<YokaiTitleSelectScreen>());
    return 0;
}

Result App::exit()
{
    Logging::info("Exiting YKSM");
    subsystems.releaseAll();
    return 0;
}

void App::end() {}
