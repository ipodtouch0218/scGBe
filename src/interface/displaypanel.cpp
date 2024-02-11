#include "displaypanel.h"
#include <algorithm>
#include <string>
#include <wx/dcbuffer.h>
#include <wx/rawbmp.h>
#include "emulatorthread.h"

#include <iostream>

extern EmulatorThread* emulator_thread;
extern uint8_t inputs;
extern bool open_emulator(std::string filename);

constexpr uint8_t SCREEN_RGB_COLORS[4][3] = {
    0x62, 0x68, 0x0D, // #62680D
    0x48, 0x61, 0x35, // #486135
    0x2E, 0x47, 0x3B, // #2E473B
    0x21, 0x34, 0x2E, // #21342E
};

DisplayPanel* display_panel_instance = nullptr;

void on_frame_complete() {
    display_panel_instance->Refresh();
}

DisplayPanel::DisplayPanel(wxFrame* parent)
    : wxPanel::wxPanel(parent, -1, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS)
{
    display_panel_instance = this;
    image = new wxBitmap(160, 144);
}

bool DisplayPanel::OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& files) {
    open_emulator(files.at(0).ToStdString());
    return true;
}

void DisplayPanel::render(wxDC& draw_context) {
    if (emulator_thread) {
        painting_mutex.Lock();
        wxAlphaPixelData bmdata(*image);
        wxAlphaPixelData::Iterator dst(bmdata);

        for (int y = 0; y < image->GetHeight(); y++) {
            dst.MoveTo(bmdata, 0, y);
            for (int x = 0; x < image->GetWidth(); x++) {
                uint8_t value = emulator_thread->gb().ppu().framebuffer[x + y * 160];
                dst.Red() = SCREEN_RGB_COLORS[value][0];
                dst.Green() = SCREEN_RGB_COLORS[value][1];
                dst.Blue() = SCREEN_RGB_COLORS[value][2];
                dst.Alpha() = 255;
                dst++;
            }
        }
        painting_mutex.Unlock();
    }

    int windowWidth, windowHeight;
    draw_context.GetSize(&windowWidth, &windowHeight);

    double scaling = std::min(windowWidth / image->GetWidth(), windowHeight / image->GetHeight());
    if (scaling <= 0) {
        scaling = std::max(image->GetWidth() / windowWidth, image->GetHeight() / windowHeight);
    }
    draw_context.SetUserScale(scaling, scaling);
    draw_context.DrawBitmap(
        *image,
        ((windowWidth / scaling) - image->GetWidth()) / 2,
        ((windowHeight / scaling) - image->GetHeight()) / 2
    );
}

void DisplayPanel::repaint() {
    wxClientDC draw_context(this);
    render(draw_context);
}

void DisplayPanel::on_paint(wxPaintEvent& event) {
    wxPaintDC draw_context(this);
    render(draw_context);
}

void DisplayPanel::on_key_up(wxKeyEvent& event) {
    uint8_t index;
    switch (event.GetKeyCode()) {
    case WXK_RETURN: index = 7; break;
    case WXK_BACK: index = 6; break;
    case 'Z': index = 5; break;
    case 'X': index = 4; break;
    case WXK_DOWN: index = 3; break;
    case WXK_UP: index = 2; break;
    case WXK_LEFT: index = 1; break;
    case WXK_RIGHT: index = 0; break;
    default: return;
    }
    inputs = utils::set_bit_value(inputs, index, 1);
}

void DisplayPanel::on_key_down(wxKeyEvent& event) {
    uint8_t index;
    switch (event.GetKeyCode()) {
    case WXK_RETURN: index = 7; break;
    case WXK_BACK: index = 6; break;
    case 'Z': index = 5; break;
    case 'X': index = 4; break;
    case WXK_DOWN: index = 3; break;
    case WXK_UP: index = 2; break;
    case WXK_LEFT: index = 1; break;
    case WXK_RIGHT: index = 0; break;
    default: return;
    }
    inputs = utils::set_bit_value(inputs, index, 0);
}

wxBEGIN_EVENT_TABLE(DisplayPanel, wxPanel)
    EVT_PAINT(DisplayPanel::on_paint)
    EVT_KEY_DOWN(DisplayPanel::on_key_down)
    EVT_KEY_UP(DisplayPanel::on_key_up)
wxEND_EVENT_TABLE()