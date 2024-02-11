#include "emulatorframe.h"
#include <wx/filedlg.h>
#include "displaypanel.h"

extern bool open_emulator(std::string filename);
extern void close_emulator();

EmulatorFrame::EmulatorFrame(const wxString& title, const wxSize& size)
    : wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, size)
{
    DisplayPanel* panel = new DisplayPanel(this);

    wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(panel, 1, wxEXPAND);
    SetSizer(sizer);

    wxMenu* menuFile = new wxMenu();
    menuFile->Append(wxID_OPEN);
    menuFile->Append(wxID_CLOSE);
    menuFile->AppendSeparator();
    menuFile->Append(wxID_EXIT);
    wxMenuBar* menuBar = new wxMenuBar();
    menuBar->Append(menuFile, "&File");
    SetMenuBar(menuBar);

    SetMinClientSize(wxSize(160, 144));
}

void EmulatorFrame::on_open(wxCommandEvent& event) {
    wxFileDialog* file_picker = new wxFileDialog(this, "Open ROM", "", "", "*.gb", wxFD_FILE_MUST_EXIST);

    if (file_picker->ShowModal() == wxID_OK) {
        open_emulator(file_picker->GetPath().ToStdString());
    }
    file_picker->Destroy();
}

void EmulatorFrame::on_close(wxCommandEvent& event) {
    close_emulator();
}

void EmulatorFrame::on_exit(wxCommandEvent& event) {
    exit(0);
}

wxBEGIN_EVENT_TABLE(EmulatorFrame, wxFrame)
    EVT_MENU(wxID_OPEN, EmulatorFrame::on_open)
    EVT_MENU(wxID_CLOSE, EmulatorFrame::on_close)
    EVT_MENU(wxID_EXIT, EmulatorFrame::on_exit)
wxEND_EVENT_TABLE()