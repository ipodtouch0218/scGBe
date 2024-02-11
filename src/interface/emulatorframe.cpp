#include "emulatorframe.h"
#include <wx/filedlg.h>
#include "displaypanel.h"
#include "emulatorthread.h"

extern EmulatorThread* emulator_thread;
extern bool open_emulator(std::string filename);
extern void close_emulator();

EmulatorFrame::EmulatorFrame(const wxString& title, const wxSize& size)
    : wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, size)
{
    DisplayPanel* panel = new DisplayPanel(this);
    SetDropTarget(panel);

    wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(panel, 1, wxEXPAND);
    SetSizer(sizer);

    wxMenu* menuFile = new wxMenu();
    menuFile->Append(wxID_OPEN);
    menuFile->Append(wxID_CLOSE);
    menuFile->AppendSeparator();
    menuFile->Append(wxID_EXIT);
    wxMenu* menuEmulation = new wxMenu();
    menuEmulation->Append(CustomMenuIds::ID_PAUSE, "Pause", wxEmptyString, true);

    wxMenuBar* menuBar = new wxMenuBar();
    menuBar->Append(menuFile, "&File");
    menuBar->Append(menuEmulation, "&Emulation");
    SetMenuBar(menuBar);

    SetMinClientSize(wxSize(160, 144));
}

void EmulatorFrame::on_file_open(wxCommandEvent& event) {
    wxFileDialog* file_picker = new wxFileDialog(this, "Open ROM", "", "", "*.gb", wxFD_FILE_MUST_EXIST);

    if (file_picker->ShowModal() == wxID_OK) {
        open_emulator(file_picker->GetPath().ToStdString());
    }
    file_picker->Destroy();
}

void EmulatorFrame::on_file_close(wxCommandEvent& event) {
    close_emulator();
}

void EmulatorFrame::on_file_exit(wxCommandEvent& event) {
    exit(0);
}

void EmulatorFrame::on_emulation_pause(wxCommandEvent& event) {
    if (!emulator_thread) {
        return;
    }

    if (emulator_thread->paused()) {
        emulator_thread->unpause();
    } else {
        emulator_thread->pause();
    }
}

wxBEGIN_EVENT_TABLE(EmulatorFrame, wxFrame)
    EVT_MENU(wxID_OPEN, EmulatorFrame::on_file_open)
    EVT_MENU(wxID_CLOSE, EmulatorFrame::on_file_close)
    EVT_MENU(wxID_EXIT, EmulatorFrame::on_file_exit)
    EVT_MENU(CustomMenuIds::ID_PAUSE, EmulatorFrame::on_emulation_pause)
wxEND_EVENT_TABLE()