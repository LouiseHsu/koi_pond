#pragma once
#include <LovyanGFX.hpp>

class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789 _panel;
  lgfx::Bus_SPI      _bus;

public:
  LGFX() {
    {
      auto cfg = _bus.config();
      cfg.spi_host   = SPI2_HOST;
      cfg.spi_mode   = 0;
      cfg.freq_write = 40000000;
      cfg.pin_sclk   = 2;
      cfg.pin_mosi   = 1;
      cfg.pin_miso   = -1;
      cfg.pin_dc     = 4;
      _bus.config(cfg);
      _panel.setBus(&_bus);
    }
    {
      auto cfg = _panel.config();
      cfg.pin_cs       = 3;
      cfg.pin_rst      = 5;
      cfg.panel_width  = 240;
      cfg.panel_height = 320;
      cfg.invert       = true;
      _panel.config(cfg);
    }
    setPanel(&_panel);
  }
};