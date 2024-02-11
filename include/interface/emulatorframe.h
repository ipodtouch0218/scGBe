#pragma once
#include <wx/wxprec.h>
#include <wx/wx.h>

class EmulatorFrame : public wxFrame {
    private:
    wxBitmap display;

    public:
    EmulatorFrame(const wxString& title, const wxSize& size);

    private:
    void on_open(wxCommandEvent& event);
    void on_close(wxCommandEvent& event);
    void on_exit(wxCommandEvent& event);

    wxDECLARE_EVENT_TABLE();
};