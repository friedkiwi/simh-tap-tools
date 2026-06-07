#include "As400FileListDialog.h"

#include <algorithm>
#include <string>

#include <wx/listctrl.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/stdpaths.h>

namespace {

wxString Utf8(const std::string& text)
{
    return wxString::FromUTF8(text.c_str());
}

} // namespace

As400FileListDialog::As400FileListDialog(wxWindow* parent, const std::vector<as400::FileListEntry>& entries)
    : wxDialog(parent, wxID_ANY, wxString::FromUTF8("IBM AS/400 File List"), wxDefaultPosition, wxSize(1040, 560), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    auto* root = new wxBoxSizer(wxVERTICAL);
    root->Add(new wxStaticText(this, wxID_ANY, wxString::FromUTF8("Double-click a file to jump to its HDR1 record.")), 0, wxALL | wxLEFT | wxRIGHT | wxTOP, 10);

    file_list_ = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL | wxBORDER_SUNKEN);
    file_list_->AppendColumn(wxString::FromUTF8("#"), wxLIST_FORMAT_RIGHT, 54);
    file_list_->AppendColumn(wxString::FromUTF8("File"), wxLIST_FORMAT_LEFT, 180);
    file_list_->AppendColumn(wxString::FromUTF8("Set"), wxLIST_FORMAT_LEFT, 96);
    file_list_->AppendColumn(wxString::FromUTF8("Section"), wxLIST_FORMAT_LEFT, 80);
    file_list_->AppendColumn(wxString::FromUTF8("Sequence"), wxLIST_FORMAT_LEFT, 84);
    file_list_->AppendColumn(wxString::FromUTF8("Generation"), wxLIST_FORMAT_LEFT, 92);
    file_list_->AppendColumn(wxString::FromUTF8("Created"), wxLIST_FORMAT_LEFT, 112);
    file_list_->AppendColumn(wxString::FromUTF8("Expires"), wxLIST_FORMAT_LEFT, 112);
    file_list_->AppendColumn(wxString::FromUTF8("System"), wxLIST_FORMAT_LEFT, 140);
    Populate(entries);
    file_list_->Bind(wxEVT_LIST_ITEM_ACTIVATED, &As400FileListDialog::OnItemActivated, this);

    root->Add(file_list_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    SetSizerAndFit(root);
    SetMinSize(wxSize(860, 420));
    CentreOnParent();
}

bool As400FileListDialog::HasSelection() const
{
    return selected_element_index_ != static_cast<std::size_t>(-1);
}

std::size_t As400FileListDialog::SelectedElementIndex() const
{
    return selected_element_index_;
}

void As400FileListDialog::Populate(const std::vector<as400::FileListEntry>& entries)
{
    file_list_->DeleteAllItems();
    for (std::size_t index = 0; index < entries.size(); ++index) {
        const auto& entry = entries[index];
        const auto row = file_list_->InsertItem(static_cast<long>(index), wxString::Format("%zu", index + 1));
        file_list_->SetItem(row, 1, Utf8(entry.file_name));
        file_list_->SetItem(row, 2, Utf8(entry.set));
        file_list_->SetItem(row, 3, Utf8(entry.section));
        file_list_->SetItem(row, 4, Utf8(entry.sequence));
        file_list_->SetItem(row, 5, Utf8(entry.generation));
        file_list_->SetItem(row, 6, Utf8(entry.created));
        file_list_->SetItem(row, 7, Utf8(entry.expires));
        file_list_->SetItem(row, 8, Utf8(entry.system));
        file_list_->SetItemData(row, static_cast<long>(entry.element_index));
    }

    for (int column = 0; column < file_list_->GetColumnCount(); ++column) {
        file_list_->SetColumnWidth(column, wxLIST_AUTOSIZE_USEHEADER);
    }
}

void As400FileListDialog::OnItemActivated(wxListEvent& event)
{
    const auto item = event.GetIndex();
    if (item == wxNOT_FOUND) {
        return;
    }

    selected_element_index_ = static_cast<std::size_t>(file_list_->GetItemData(item));
    EndModal(wxID_OK);
}
