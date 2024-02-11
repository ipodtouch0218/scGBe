#pragma once
#include <wx/wxprec.h>
#include <wx/wx.h>

namespace CustomMenuIds {
    enum CustomMenuIds {
        ID_PAUSE = 1,
    };
}

class EmulatorFrame : public wxFrame {
    private:
    wxBitmap display;

    public:
    EmulatorFrame(const wxString& title, const wxSize& size);

    private:
    void on_file_open(wxCommandEvent& event);
    void on_file_close(wxCommandEvent& event);
    void on_file_exit(wxCommandEvent& event);

    void on_emulation_pause(wxCommandEvent& event);

    wxDECLARE_EVENT_TABLE();
};