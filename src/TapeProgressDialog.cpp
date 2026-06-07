#include "TapeProgressDialog.h"

#include <algorithm>
#include <string>

#include <wx/gauge.h>
#include <wx/event.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

namespace {

wxString Utf8(std::string_view text)
{
    return wxString::FromUTF8(text.data(), text.size());
}

} // namespace

TapeProgressDialog::TapeProgressDialog(wxWindow* parent, std::string_view title)
    : wxDialog(parent, wxID_ANY, Utf8(title), wxDefaultPosition, wxSize(540, 180), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    auto* root = new wxBoxSizer(wxVERTICAL);
    activity_label_ = new wxStaticText(this, wxID_ANY, wxString::FromUTF8("Starting..."));
    count_label_ = new wxStaticText(this, wxID_ANY, wxString::FromUTF8(" "));
    gauge_ = new wxGauge(this, wxID_ANY, 100, wxDefaultPosition, wxDefaultSize, wxGA_HORIZONTAL | wxGA_SMOOTH);

    root->Add(activity_label_, 0, wxALL | wxEXPAND, 12);
    root->Add(gauge_, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 12);
    root->Add(count_label_, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 12);
    SetSizerAndFit(root);
    CentreOnParent();
    Show();
    Raise();
    wxSafeYield(this, true);

    Bind(wxEVT_CLOSE_WINDOW, &TapeProgressDialog::OnClose, this);
}

void TapeProgressDialog::Pulse(std::string_view activity, std::size_t current)
{
    RefreshText(activity, current, 0, true);
    gauge_->Pulse();
    PumpEvents();
}

void TapeProgressDialog::SetProgress(std::string_view activity, std::size_t current, std::size_t total)
{
    total = std::max<std::size_t>(total, 1);
    RefreshText(activity, current, total, false);
    gauge_->SetRange(static_cast<int>(total));
    gauge_->SetValue(static_cast<int>(std::min(current, total)));
    PumpEvents();
}

void TapeProgressDialog::RefreshText(std::string_view activity, std::size_t current, std::size_t total, bool indeterminate)
{
    activity_label_->SetLabel(Utf8(activity));
    if (indeterminate) {
        count_label_->SetLabel(wxString::Format(wxString::FromUTF8("%zu records processed"), current));
    } else {
        count_label_->SetLabel(wxString::Format(wxString::FromUTF8("%zu / %zu records processed"), current, total));
    }
    Layout();
}

void TapeProgressDialog::PumpEvents()
{
    wxSafeYield(this, true);
}

void TapeProgressDialog::OnClose(wxCloseEvent& event)
{
    event.Veto();
}
