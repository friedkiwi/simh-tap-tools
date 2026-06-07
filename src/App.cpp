#include "App.h"

#include "MainFrame.h"

#include <filesystem>
#include <optional>

#include <wx/string.h>

wxIMPLEMENT_APP(TapeToolsApp);

namespace {

std::filesystem::path PathFromWxString(const wxString& value)
{
#if defined(_WIN32)
    return std::filesystem::path(value.ToStdWstring());
#else
    return std::filesystem::path(value.ToStdString());
#endif
}

std::optional<std::filesystem::path> StartupTapePath(int argc, wxChar** argv)
{
    bool treat_next_as_path = false;
    for (int index = 1; index < argc; ++index) {
        const wxString argument(argv[index]);
        if (!treat_next_as_path && argument == wxString::FromUTF8("--")) {
            treat_next_as_path = true;
            continue;
        }
        if (!treat_next_as_path && argument.StartsWith(wxString::FromUTF8("-"))) {
            continue;
        }

        return PathFromWxString(argument);
    }

    return std::nullopt;
}

} // namespace

bool TapeToolsApp::OnInit()
{
    if (!wxApp::OnInit()) {
        return false;
    }

    auto* frame = new MainFrame(wxString::FromUTF8("SIMH Tape Tools"));
    frame->Show(true);
    if (const auto startup_path = StartupTapePath(argc, argv)) {
        frame->LoadTapeFile(*startup_path);
    }
    return true;
}
