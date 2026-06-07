#pragma once

#include "HexFormatter.h"
#include "tap/TapeImage.h"

#include <filesystem>
#include <limits>
#include <string>
#include <vector>

#include <wx/frame.h>

class wxCommandEvent;
class wxListEvent;
class wxListCtrl;
class wxString;
class wxTextCtrl;

class MainFrame final : public wxFrame
{
public:
    explicit MainFrame(const wxString& title);
    void LoadTapeFile(const std::filesystem::path& path);

private:
    void BuildMenuBar();
    void BuildContent();

    void ClearTape();
    void PopulateStructureList();
    void ShowSelectedElement(std::size_t index);
    void RefreshHexView();
    void SetEncoding(TextEncoding encoding);
    void UpdateWindowTitle();
    void UpdateStatusText();

    void OnOpen(wxCommandEvent& event);
    void OnCloseFile(wxCommandEvent& event);
    void OnCopy(wxCommandEvent& event);
    void OnFind(wxCommandEvent& event);
    void OnEncodingAscii(wxCommandEvent& event);
    void OnEncodingEbcdic(wxCommandEvent& event);
    void OnStructureSelected(wxListEvent& event);
    void OnAbout(wxCommandEvent& event);

    wxListCtrl* structure_list_ = nullptr;
    wxTextCtrl* hex_view_ = nullptr;
    wxMenuItem* ascii_encoding_item_ = nullptr;
    wxMenuItem* ebcdic_encoding_item_ = nullptr;

    tap::TapeImage tape_image_;
    std::filesystem::path loaded_path_;
    std::vector<std::uint8_t> selected_bytes_;
    std::size_t selected_element_index_ = std::numeric_limits<std::size_t>::max();
    TextEncoding encoding_ = TextEncoding::Ascii;
    std::string last_search_text_;
    std::size_t next_search_offset_ = 0;
};
