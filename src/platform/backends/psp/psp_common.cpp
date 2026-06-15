#include "psp_common.h"

#include <pspiofilemgr.h>
#include <pspiofilemgr_fcntl.h>

#include "core/logger.h"

#define LOG_FILE "runtime.log"

static bool s_logfile_ok = false;

static void log_callback([[maybe_unused]] LogSeverity severity, [[maybe_unused]] std::string_view message) {
    /* HACK: we are opening and then closing the log file per-message. That's bad. */
    if (!s_logfile_ok) {
        return;
    }

    SceUID fd = sceIoOpen(LOG_FILE, PSP_O_WRONLY | PSP_O_APPEND | PSP_O_CREAT, 0777);
    if (fd < 0) {
        s_logfile_ok = false;
        return;
    }

    sceIoWrite(fd, message.data(), (SceSize)message.size());
    sceIoWrite(fd, "\n", 1); /* adds newline */
    sceIoClose(fd);
}

void log_init() {
    if (SceUID fd = sceIoOpen(LOG_FILE, PSP_O_WRONLY, 0777); fd >= 0) {
        /* delete previous log file if it already exists */
        sceIoClose(fd);
        sceIoRemove(LOG_FILE);
    }

    s_logfile_ok = true;
    Logger::get().set_log_callback(log_callback);
}
