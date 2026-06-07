#include "MainFrame.h"

#include "As400FileListDialog.h"
#include "TapeProgressDialog.h"
#include "TapeElementView.h"
#include "as400/FileList.h"
#include "tap/Reader.h"

#include <algorithm>
#include <string>
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
constexpr int DecoderGenericMenuId = wxID_HIGHEST + 3;
constexpr int DecoderIbmAs400MenuId = wxID_HIGHEST + 4;
constexpr int As400FileListMenuId = wxID_HIGHEST + 5;

constexpr long NoSelection = -1;

wxString Utf8(std::string_view text)
{
    return wxString::FromUTF8(text.data(), text.size());
}

DecoderProperty ToDecoderProperty(const as400::RecordField& field)
{
    DecoderProperty property;
    property.name = field.name;
    property.value = field.value;
    property.children.reserve(field.children.size());
    for (const auto& child : field.children) {
        property.children.push_back(ToDecoderProperty(child));
    }
    return property;
}

template <typename T>
T progressStep(T total)
{
    return std::max<T>(1, total / 100);
}

template <typename T>
bool shouldUpdateProgress(T current, T total, T& last_bucket, T step)
{
    if (total == 0) {
        return true;
    }

    const auto bucket = current / step;
    const auto last = last_bucket / step;
    if (bucket != last || current == total) {
        last_bucket = current;
        return true;
    }

    return false;
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
    Bind(wxEVT_MENU, &MainFrame::OnDecoderGeneric, this, DecoderGenericMenuId);
    Bind(wxEVT_MENU, &MainFrame::OnDecoderIbmAs400, this, DecoderIbmAs400MenuId);
    Bind(wxEVT_MENU, &MainFrame::OnAs400FileList, this, As400FileListMenuId);
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

    auto* decoder_menu = new wxMenu();
    generic_decoder_item_ = decoder_menu->AppendRadioItem(DecoderGenericMenuId, wxString::FromUTF8("&Generic"));
    as400_decoder_item_ = decoder_menu->AppendRadioItem(DecoderIbmAs400MenuId, wxString::FromUTF8("&IBM AS/400"));
    generic_decoder_item_->Check(true);

    auto* as400_menu = new wxMenu();
    as400_menu->Append(As400FileListMenuId, wxString::FromUTF8("&File list...\tCtrl-L"));

    auto* view_menu = new wxMenu();
    view_menu->AppendSubMenu(encoding_menu, wxString::FromUTF8("&Encoding"));
    view_menu->AppendSubMenu(decoder_menu, wxString::FromUTF8("&Decoders"));
    view_menu->AppendSubMenu(as400_menu, wxString::FromUTF8("&IBM AS/400"));

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
    main_splitter_ = new wxSplitterWindow(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE);

    structure_list_ = new wxListCtrl(main_splitter_, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
    structure_list_->AppendColumn(wxString::FromUTF8("#"), wxLIST_FORMAT_RIGHT, 56);
    structure_list_->AppendColumn(wxString::FromUTF8("Type"), wxLIST_FORMAT_LEFT, 140);
    structure_list_->AppendColumn(wxString::FromUTF8("Offset"), wxLIST_FORMAT_LEFT, 110);
    structure_list_->AppendColumn(wxString::FromUTF8("Size"), wxLIST_FORMAT_LEFT, 110);
    structure_list_->AppendColumn(wxString::FromUTF8("Details"), wxLIST_FORMAT_LEFT, 220);

    hex_decoder_splitter_ = new wxSplitterWindow(main_splitter_, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE);

    hex_view_ = new wxTextCtrl(
        hex_decoder_splitter_,
        wxID_ANY,
        wxString(),
        wxDefaultPosition,
        wxDefaultSize,
        wxTE_MULTILINE | wxTE_READONLY | wxTE_DONTWRAP | wxTE_RICH2);
    hex_view_->SetFont(wxFont(wxFontInfo(10).Family(wxFONTFAMILY_TELETYPE)));

    decoder_panel_ = new DecoderPropertyPane(hex_decoder_splitter_);
    decoder_panel_->SetMessage("AS/400 record", "Select a data record to inspect its AS/400 label type.");
    hex_decoder_splitter_->Initialize(hex_view_);
    hex_decoder_splitter_->SetMinimumPaneSize(120);
    hex_decoder_splitter_->SetSashGravity(1.0);

    main_splitter_->SplitVertically(structure_list_, hex_decoder_splitter_, 420);
    main_splitter_->SetMinimumPaneSize(240);
    main_splitter_->SetSashGravity(0.0);

    sizer->Add(main_splitter_, 1, wxEXPAND);
    panel->SetSizer(sizer);
}

void MainFrame::LoadTapeFile(const std::filesystem::path& path)
{
    tap::Reader reader;
    const auto read_result = [&]() {
        TapeProgressDialog progress(this, "Loading tape file", "bytes");
        std::uint64_t last_progress_bytes = 0;
        std::uint64_t step = 1;
        std::uint64_t seen_total = 0;
        return reader.read(path, [&progress, &last_progress_bytes, &step, &seen_total](const tap::ProgressInfo& info) {
            if (seen_total != info.bytes_total) {
                seen_total = info.bytes_total;
                step = progressStep<std::uint64_t>(std::max<std::uint64_t>(1, info.bytes_total));
            }
            if (shouldUpdateProgress(info.bytes_read, info.bytes_total, last_progress_bytes, step)) {
                progress.SetProgress("Reading tape file...", info.bytes_read, info.bytes_total);
            }
        });
    }();
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
    DetectDecoderMode();

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
    SetDecoderMode(DecoderMode::Generic);
    PopulateStructureList();
    RefreshHexView();
    UpdateWindowTitle();
    UpdateStatusText();
}

void MainFrame::PopulateStructureList()
{
    structure_list_->DeleteAllItems();

    const auto& elements = tape_image_.elements();
    if (elements.empty()) {
        return;
    }

    TapeProgressDialog progress(this, "Rendering tape structure", "records");
    std::size_t last_progress_records = 0;
    const auto step = progressStep(elements.size());
    for (std::size_t index = 0; index < elements.size(); ++index) {
        const auto view = describeTapeElement(elements[index]);
        const auto row = structure_list_->InsertItem(static_cast<long>(index), wxString::Format("%zu", index));
        structure_list_->SetItem(row, 1, Utf8(view.type));
        structure_list_->SetItem(row, 2, Utf8(view.offset));
        structure_list_->SetItem(row, 3, Utf8(view.size));
        structure_list_->SetItem(row, 4, Utf8(view.details));
        structure_list_->SetItemData(row, static_cast<long>(index));
        const auto current = index + 1;
        if (shouldUpdateProgress(current, elements.size(), last_progress_records, step)) {
            progress.SetProgress("Rendering tape structure...", current, elements.size());
        }
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
    UpdateDecoderPanel();
    UpdateStatusText();
}

void MainFrame::JumpToElement(std::size_t index)
{
    const auto& elements = tape_image_.elements();
    if (index >= elements.size()) {
        return;
    }

    structure_list_->SetItemState(static_cast<long>(index), wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
    structure_list_->EnsureVisible(static_cast<long>(index));
    ShowSelectedElement(index);
    structure_list_->SetFocus();
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

void MainFrame::SetDecoderMode(DecoderMode decoder_mode)
{
    decoder_mode_ = decoder_mode;
    if (generic_decoder_item_) {
        generic_decoder_item_->Check(decoder_mode_ == DecoderMode::Generic);
    }
    if (as400_decoder_item_) {
        as400_decoder_item_->Check(decoder_mode_ == DecoderMode::IbmAs400);
    }

    if (decoder_mode_ == DecoderMode::IbmAs400) {
        SetEncoding(TextEncoding::EbcdicCp37);
        if (hex_decoder_splitter_ && !hex_decoder_splitter_->IsSplit()) {
            const auto height = std::max(120, hex_decoder_splitter_->GetClientSize().GetHeight() - 180);
            hex_decoder_splitter_->SplitHorizontally(hex_view_, decoder_panel_, height);
        }
    } else {
        SetEncoding(TextEncoding::Ascii);
        if (hex_decoder_splitter_ && hex_decoder_splitter_->IsSplit()) {
            hex_decoder_splitter_->Unsplit(decoder_panel_);
        }
    }

    if (hex_decoder_splitter_) {
        hex_decoder_splitter_->Layout();
    }
    UpdateDecoderPanel();
}

void MainFrame::DetectDecoderMode()
{
    SetDecoderMode(as400_parser_.isAs400Tape(tape_image_) ? DecoderMode::IbmAs400 : DecoderMode::Generic);
}

void MainFrame::UpdateDecoderPanel()
{
    if (!decoder_panel_ || decoder_mode_ != DecoderMode::IbmAs400) {
        return;
    }

    const auto& elements = tape_image_.elements();
    if (selected_element_index_ >= elements.size() || !elements[selected_element_index_].isRecord()) {
        decoder_panel_->SetMessage("AS/400 record", "Select a data record to inspect its AS/400 label type.");
        hex_decoder_splitter_->Layout();
        return;
    }

    const auto info = as400_parser_.parseRecord(elements[selected_element_index_].record().data);
    if (info.recognized) {
        DecoderProperty record;
        record.name = "Record";
        record.value = info.name;
        record.children.push_back(DecoderProperty{"Label code", info.code, {}});
        record.children.push_back(DecoderProperty{"Decoder", "IBM AS/400", {}});
        for (const auto& field : info.fields) {
            record.children.push_back(ToDecoderProperty(field));
        }
        if (info.fields.empty()) {
            record.children.push_back(DecoderProperty{"Status", "No additional label fields decoded.", {}});
        }
        decoder_panel_->SetTitle("AS/400 record: " + info.name + " (" + info.code + ")");
        decoder_panel_->SetProperties({record});
    } else {
        decoder_panel_->SetTitle("AS/400 record: Unknown");
        decoder_panel_->SetProperties({
            DecoderProperty{
                "Record",
                "Unknown",
                {
                    DecoderProperty{"Status", "No AS/400 tape label recognized.", {}},
                    DecoderProperty{"Leading CP37 text", info.code, {}},
                },
            },
        });
    }

    hex_decoder_splitter_->Layout();
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
    const auto decoder_name = decoder_mode_ == DecoderMode::IbmAs400 ? "IBM AS/400" : "Generic";
    if (selected_element_index_ == std::numeric_limits<std::size_t>::max()) {
        SetStatusText(wxString::Format(
            wxString::FromUTF8("%zu elements, %zu records, %zu tape marks | %s | %s"),
            tape_image_.elementCount(),
            tape_image_.recordCount(),
            tape_image_.tapeMarkCount(),
            wxString::FromUTF8(encoding_name),
            wxString::FromUTF8(decoder_name)));
        return;
    }

    SetStatusText(wxString::Format(
        wxString::FromUTF8("Element %zu | %zu bytes | %s | %s"),
        selected_element_index_,
        selected_bytes_.size(),
        wxString::FromUTF8(encoding_name),
        wxString::FromUTF8(decoder_name)));
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
    if (decoder_panel_ && decoder_panel_->HasFocusedChild()) {
        if (decoder_panel_->CopySelectionToClipboard()) {
            SetStatusText(wxString::FromUTF8("Copied decoder property"));
        }
        return;
    }

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
    if (decoder_mode_ == DecoderMode::IbmAs400) {
        SetDecoderMode(DecoderMode::Generic);
        return;
    }
    SetEncoding(TextEncoding::Ascii);
}

void MainFrame::OnEncodingEbcdic(wxCommandEvent&)
{
    SetEncoding(TextEncoding::EbcdicCp37);
}

void MainFrame::OnDecoderGeneric(wxCommandEvent&)
{
    SetDecoderMode(DecoderMode::Generic);
}

void MainFrame::OnDecoderIbmAs400(wxCommandEvent&)
{
    SetDecoderMode(DecoderMode::IbmAs400);
}

void MainFrame::OnAs400FileList(wxCommandEvent&)
{
    if (tape_image_.empty()) {
        wxMessageBox(
            wxString::FromUTF8("Load a tape before opening the IBM AS/400 file list."),
            wxString::FromUTF8("IBM AS/400 File List"),
            wxOK | wxICON_ERROR,
            this);
        return;
    }

    if (!as400_parser_.isAs400Tape(tape_image_)) {
        wxMessageBox(
            wxString::FromUTF8("The current tape is not recognized as an IBM AS/400 tape."),
            wxString::FromUTF8("IBM AS/400 File List"),
            wxOK | wxICON_ERROR,
            this);
        return;
    }

    const auto file_entries = as400::collectAs400FileList(tape_image_, as400_parser_);
    if (file_entries.empty()) {
        wxMessageBox(
            wxString::FromUTF8("No HDR1 file labels were found on this tape."),
            wxString::FromUTF8("IBM AS/400 File List"),
            wxOK | wxICON_INFORMATION,
            this);
        return;
    }

    As400FileListDialog dialog(this, file_entries);
    if (dialog.ShowModal() == wxID_OK && dialog.HasSelection()) {
        JumpToElement(dialog.SelectedElementIndex());
    }
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
