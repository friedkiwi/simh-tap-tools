#include "MainFrame.h"

#include "TapeElementView.h"
#include "tap/Reader.h"

#include <string_view>

#include <wx/aboutdlg.h>
#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include <wx/event.h>
#include <wx/filedlg.h>
#include <wx/font.h>
#include <wx/listctrl.h>
#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/statusbr.h>
#include <wx/string.h>
#include <wx/textctrl.h>
#include <wx/textdlg.h>

namespace {

constexpr int OpenMenuId = wxID_OPEN;
constexpr int CloseFileMenuId = wxID_CLOSE;
constexpr int ExitMenuId = wxID_EXIT;
constexpr int CopyMenuId = wxID_COPY;
constexpr int FindMenuId = wxID_FIND;
constexpr int AboutMenuId = wxID_ABOUT;
constexpr int EncodingAsciiMenuId = wxID_HIGHEST + 1;
constexpr int EncodingEbcdicMenuId = wxID_HIGHEST + 2;

constexpr long NoSelection = -1;

wxString Utf8(std::string_view text)
{
    return wxString::FromUTF8(text.data(), text.size());
}

} // namespace

MainFrame::MainFrame(const wxString& title)
    : wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(1100, 720))
{
    BuildMenuBar();
    BuildContent();
    CreateStatusBar();
    UpdateWindowTitle();
    UpdateStatusText();

    Bind(wxEVT_MENU, &MainFrame::OnOpen, this, OpenMenuId);
    Bind(wxEVT_MENU, &MainFrame::OnCloseFile, this, CloseFileMenuId);
    Bind(wxEVT_MENU, [this](wxCommandEvent&) { Close(true); }, ExitMenuId);
    Bind(wxEVT_MENU, &MainFrame::OnCopy, this, CopyMenuId);
    Bind(wxEVT_MENU, &MainFrame::OnFind, this, FindMenuId);
    Bind(wxEVT_MENU, &MainFrame::OnEncodingAscii, this, EncodingAsciiMenuId);
    Bind(wxEVT_MENU, &MainFrame::OnEncodingEbcdic, this, EncodingEbcdicMenuId);
    Bind(wxEVT_MENU, &MainFrame::OnAbout, this, AboutMenuId);
    structure_list_->Bind(wxEVT_LIST_ITEM_SELECTED, &MainFrame::OnStructureSelected, this);
}

void MainFrame::BuildMenuBar()
{
    auto* file_menu = new wxMenu();
    file_menu->Append(OpenMenuId, wxString::FromUTF8("&Open...\tCtrl-O"));
    file_menu->Append(CloseFileMenuId, wxString::FromUTF8("&Close"));
    file_menu->AppendSeparator();
    file_menu->Append(ExitMenuId, wxString::FromUTF8("E&xit\tAlt-X"));

    auto* edit_menu = new wxMenu();
    edit_menu->Append(CopyMenuId, wxString::FromUTF8("&Copy\tCtrl-C"));
    edit_menu->AppendSeparator();
    edit_menu->Append(FindMenuId, wxString::FromUTF8("&Find...\tCtrl-F"));

    auto* encoding_menu = new wxMenu();
    ascii_encoding_item_ = encoding_menu->AppendRadioItem(EncodingAsciiMenuId, wxString::FromUTF8("&ASCII"));
    ebcdic_encoding_item_ = encoding_menu->AppendRadioItem(EncodingEbcdicMenuId, wxString::FromUTF8("&EBCDIC (CP 37)"));
    ascii_encoding_item_->Check(true);

    auto* view_menu = new wxMenu();
    view_menu->AppendSubMenu(encoding_menu, wxString::FromUTF8("&Encoding"));

    auto* help_menu = new wxMenu();
    help_menu->Append(AboutMenuId, wxString::FromUTF8("&About"));

    auto* menu_bar = new wxMenuBar();
    menu_bar->Append(file_menu, wxString::FromUTF8("&File"));
    menu_bar->Append(edit_menu, wxString::FromUTF8("&Edit"));
    menu_bar->Append(view_menu, wxString::FromUTF8("&View"));
    menu_bar->Append(help_menu, wxString::FromUTF8("&Help"));
    SetMenuBar(menu_bar);
}

void MainFrame::BuildContent()
{
    auto* panel = new wxPanel(this);
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    auto* splitter = new wxSplitterWindow(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE);

    structure_list_ = new wxListCtrl(splitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
    structure_list_->AppendColumn(wxString::FromUTF8("#"), wxLIST_FORMAT_RIGHT, 56);
    structure_list_->AppendColumn(wxString::FromUTF8("Type"), wxLIST_FORMAT_LEFT, 140);
    structure_list_->AppendColumn(wxString::FromUTF8("Offset"), wxLIST_FORMAT_LEFT, 110);
    structure_list_->AppendColumn(wxString::FromUTF8("Size"), wxLIST_FORMAT_LEFT, 110);
    structure_list_->AppendColumn(wxString::FromUTF8("Details"), wxLIST_FORMAT_LEFT, 220);

    hex_view_ = new wxTextCtrl(
        splitter,
        wxID_ANY,
        wxString(),
        wxDefaultPosition,
        wxDefaultSize,
        wxTE_MULTILINE | wxTE_READONLY | wxTE_DONTWRAP | wxTE_RICH2);
    hex_view_->SetFont(wxFont(wxFontInfo(10).Family(wxFONTFAMILY_TELETYPE)));

    splitter->SplitVertically(structure_list_, hex_view_, 420);
    splitter->SetMinimumPaneSize(240);

    sizer->Add(splitter, 1, wxEXPAND);
    panel->SetSizer(sizer);
}

void MainFrame::LoadTapeFile(const std::filesystem::path& path)
{
    tap::Reader reader;
    const auto read_result = reader.read(path);
    if (!read_result) {
        const auto& error = read_result.error();
        wxMessageBox(
            wxString::Format(
                wxString::FromUTF8("Could not open tape file.\n\nOffset: %llu\nError: %s"),
                static_cast<unsigned long long>(error.offset),
                wxString::FromUTF8(error.message.c_str())),
            wxString::FromUTF8("Open Tape"),
            wxOK | wxICON_ERROR,
            this);
        return;
    }

    tape_image_ = read_result.value();
    loaded_path_ = path;
    selected_bytes_.clear();
    selected_element_index_ = std::numeric_limits<std::size_t>::max();
    next_search_offset_ = 0;

    PopulateStructureList();
    if (!tape_image_.empty()) {
        structure_list_->SetItemState(0, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
        ShowSelectedElement(0);
    } else {
        RefreshHexView();
    }

    UpdateWindowTitle();
    UpdateStatusText();
}

void MainFrame::ClearTape()
{
    tape_image_ = tap::TapeImage();
    loaded_path_.clear();
    selected_bytes_.clear();
    selected_element_index_ = std::numeric_limits<std::size_t>::max();
    next_search_offset_ = 0;
    PopulateStructureList();
    RefreshHexView();
    UpdateWindowTitle();
    UpdateStatusText();
}

void MainFrame::PopulateStructureList()
{
    structure_list_->DeleteAllItems();

    const auto& elements = tape_image_.elements();
    for (std::size_t index = 0; index < elements.size(); ++index) {
        const auto view = describeTapeElement(elements[index]);
        const auto row = structure_list_->InsertItem(static_cast<long>(index), wxString::Format("%zu", index));
        structure_list_->SetItem(row, 1, Utf8(view.type));
        structure_list_->SetItem(row, 2, Utf8(view.offset));
        structure_list_->SetItem(row, 3, Utf8(view.size));
        structure_list_->SetItem(row, 4, Utf8(view.details));
        structure_list_->SetItemData(row, static_cast<long>(index));
    }
}

void MainFrame::ShowSelectedElement(std::size_t index)
{
    const auto& elements = tape_image_.elements();
    if (index >= elements.size()) {
        selected_bytes_.clear();
        selected_element_index_ = std::numeric_limits<std::size_t>::max();
    } else {
        selected_bytes_ = describeTapeElement(elements[index]).bytes;
        selected_element_index_ = index;
    }

    next_search_offset_ = 0;
    RefreshHexView();
    UpdateStatusText();
}

void MainFrame::RefreshHexView()
{
    if (!hex_view_) {
        return;
    }

    hex_view_->SetValue(Utf8(formatHexView(selected_bytes_, encoding_)));
}

void MainFrame::SetEncoding(TextEncoding encoding)
{
    encoding_ = encoding;
    if (ascii_encoding_item_) {
        ascii_encoding_item_->Check(encoding_ == TextEncoding::Ascii);
    }
    if (ebcdic_encoding_item_) {
        ebcdic_encoding_item_->Check(encoding_ == TextEncoding::EbcdicCp37);
    }
    RefreshHexView();
    next_search_offset_ = 0;
    UpdateStatusText();
}

void MainFrame::UpdateWindowTitle()
{
    if (loaded_path_.empty()) {
        SetTitle(wxString::FromUTF8("SIMH Tape Tools"));
        return;
    }

    SetTitle(wxString::Format(
        wxString::FromUTF8("SIMH Tape Tools - %s"),
        wxString::FromUTF8(loaded_path_.string().c_str())));
}

void MainFrame::UpdateStatusText()
{
    if (loaded_path_.empty()) {
        SetStatusText(wxString::FromUTF8("No tape loaded"));
        return;
    }

    const auto encoding_name = encoding_ == TextEncoding::Ascii ? "ASCII" : "EBCDIC CP 37";
    if (selected_element_index_ == std::numeric_limits<std::size_t>::max()) {
        SetStatusText(wxString::Format(
            wxString::FromUTF8("%zu elements, %zu records, %zu tape marks | %s"),
            tape_image_.elementCount(),
            tape_image_.recordCount(),
            tape_image_.tapeMarkCount(),
            wxString::FromUTF8(encoding_name)));
        return;
    }

    SetStatusText(wxString::Format(
        wxString::FromUTF8("Element %zu | %zu bytes | %s"),
        selected_element_index_,
        selected_bytes_.size(),
        wxString::FromUTF8(encoding_name)));
}

void MainFrame::OnOpen(wxCommandEvent&)
{
    wxFileDialog dialog(
        this,
        wxString::FromUTF8("Open SIMH Tape File"),
        wxString(),
        wxString(),
        wxString::FromUTF8("Tape files (*.tap;*.TAP)|*.tap;*.TAP|All files (*.*)|*.*"),
        wxFD_OPEN | wxFD_FILE_MUST_EXIST);

    if (dialog.ShowModal() != wxID_OK) {
        return;
    }

    LoadTapeFile(std::filesystem::path(dialog.GetPath().ToStdString()));
}

void MainFrame::OnCloseFile(wxCommandEvent&)
{
    ClearTape();
}

void MainFrame::OnCopy(wxCommandEvent&)
{
    if (!hex_view_) {
        return;
    }

    wxString text;
    if (hex_view_->GetStringSelection().empty()) {
        text = hex_view_->GetValue();
    } else {
        text = hex_view_->GetStringSelection();
    }

    if (text.empty()) {
        return;
    }

    if (wxTheClipboard->Open()) {
        wxTheClipboard->SetData(new wxTextDataObject(text));
        wxTheClipboard->Close();
        SetStatusText(wxString::FromUTF8("Copied hex view text"));
    }
}

void MainFrame::OnFind(wxCommandEvent&)
{
    if (selected_bytes_.empty()) {
        wxMessageBox(wxString::FromUTF8("Select a tape element before searching."), wxString::FromUTF8("Find"), wxOK | wxICON_INFORMATION, this);
        return;
    }

    wxTextEntryDialog dialog(
        this,
        wxString::FromUTF8("Search the selected element using the active text encoding."),
        wxString::FromUTF8("Find"),
        wxString::FromUTF8(last_search_text_.c_str()));

    if (dialog.ShowModal() != wxID_OK) {
        return;
    }

    last_search_text_ = dialog.GetValue().ToStdString();
    const auto needle = encodeSearchText(last_search_text_, encoding_);
    const auto result = findBytesInHexView(selected_bytes_, needle, encoding_, next_search_offset_);
    if (!result.found) {
        wxMessageBox(wxString::FromUTF8("Search text was not found in the selected element."), wxString::FromUTF8("Find"), wxOK | wxICON_INFORMATION, this);
        next_search_offset_ = 0;
        return;
    }

    hex_view_->SetFocus();
    hex_view_->SetSelection(static_cast<long>(result.text_offset), static_cast<long>(result.text_offset + result.text_length));
    next_search_offset_ = result.byte_offset + 1;
    SetStatusText(wxString::Format(wxString::FromUTF8("Found at byte offset 0x%zX"), result.byte_offset));
}

void MainFrame::OnEncodingAscii(wxCommandEvent&)
{
    SetEncoding(TextEncoding::Ascii);
}

void MainFrame::OnEncodingEbcdic(wxCommandEvent&)
{
    SetEncoding(TextEncoding::EbcdicCp37);
}

void MainFrame::OnStructureSelected(wxListEvent& event)
{
    const auto item = event.GetIndex();
    if (item == NoSelection) {
        return;
    }

    ShowSelectedElement(static_cast<std::size_t>(structure_list_->GetItemData(item)));
}

void MainFrame::OnAbout(wxCommandEvent&)
{
    wxAboutDialogInfo info;
    info.SetName(wxString::FromUTF8("SIMH Tape Tools"));
    info.SetVersion(wxString::FromUTF8("0.1.0"));
    info.SetDescription(wxString::FromUTF8("SIMH tape file browser and editing shell."));
    wxAboutBox(info, this);
}
