#pragma once
#include <wx/thread.h>
#include <wx/wxprec.h>
#include <wx/wx.h>

class DisplayPanel : public wxPanel {
    wxBitmap* image;
    wxMutex painting_mutex;

    public:
    DisplayPanel(wxFrame* parent);

    void repaint();
    void render(wxDC& draw_context);

    private:
    void on_paint(wxPaintEvent& event);
    void on_key_down(wxKeyEvent& event);
    void on_key_up(wxKeyEvent& event);

    wxDECLARE_EVENT_TABLE();
};