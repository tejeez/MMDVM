// Monitor program to show received and transmitted FM waveforms.
//
// Compile with:
// g++ -o ../bin_linux/monitor monitor.cpp -Wall -Wextra -lzmq -lSDL2
//
// Useful references:
// https://github.com/aminosbh/basic-cpp-sdl-project/blob/master/src/main.cc
// https://brettviren.github.io/cppzmq-tour/index.html

#include <stdint.h>

#include <zmq.hpp>
#include <SDL2/SDL.h>

struct monitorFmMsg {
  char id[2]; // 'F', 'M'
  uint16_t rxSample, rxControl, rxRssi;
  uint16_t txSample, txControl, txBufData;
};

zmq::context_t zmq_ctx;

class Plotter {
public:
    Plotter(SDL_Rect area, int fullscale):
    area(area)
    {
        scaling = 1.0f / (float)fullscale;
    }

    void plot(SDL_Renderer *rend, int x, int flag, int sample)
    {
        if (flag == 0) {
            SDL_SetRenderDrawColor(rend, 0x00, 0x00, 0x00, 0xFF);
        } else if (flag == 4) {
            SDL_SetRenderDrawColor(rend, 0xFF, 0x00, 0x00, 0xFF);
        } else {
            SDL_SetRenderDrawColor(rend, 0x00, 0xFF, 0x00, 0xFF);
        }
        if (flag != 0 || x != prev_x) {
            SDL_Rect erase_rect = {
                .x = area.x + x,
                .y = area.y,
                .w = 1,
                .h = area.h,
            };
            SDL_RenderFillRect(rend, &erase_rect);
        }

        // Scaled to 0-1
        float scaled = std::max(std::min((float)sample * scaling, 1.0f), 0.0f);
        SDL_Rect plot_rect = {
            .x = area.x + x,
            .y = area.y + (int)std::round((1.0f - scaled) * (float)(area.h-1)),
            .w = 1,
            .h = 1,
        };
        SDL_SetRenderDrawColor(rend, 0xFF, 0xFF, 0xFF, 0xC0);
        SDL_RenderFillRect(rend, &plot_rect);

        prev_x = x;
    }
private:
    SDL_Rect area;
    int prev_x;
    float scaling;
};

int main(void)
{
    const int win_width = 720, win_height = 600;
    zmq::socket_t sock(zmq_ctx, zmq::socket_type::sub);
    sock.connect("ipc:///tmp/MMDVM_Monitor");
    sock.set(zmq::sockopt::subscribe, "FM");
    sock.set(zmq::sockopt::rcvtimeo, 50);

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
        return 1;

    SDL_Window *sdl_window = SDL_CreateWindow(
        "MMDVM Monitor",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        win_width, win_height,
        SDL_WINDOW_SHOWN
    );
    if (sdl_window == NULL)
        return 2;

    SDL_Renderer *rend = SDL_CreateRenderer(sdl_window, -1, SDL_RENDERER_ACCELERATED);
    if (rend == NULL)
        return 3;

    Plotter plot_rx_rssi(SDL_Rect { .x = 0, .y = 0,   .w = win_width, .h = 140 }, 4096);
    Plotter plot_rx_fm  (SDL_Rect { .x = 0, .y = 150, .w = win_width, .h = 140 }, 4096);
    Plotter plot_tx_fm  (SDL_Rect { .x = 0, .y = 300, .w = win_width, .h = 140 }, 4096);
    Plotter plot_tx_buf (SDL_Rect { .x = 0, .y = 450, .w = win_width, .h = 140 }, 500);

    bool running = true;
    int x_counter = 0;
    const int samples_per_pixel = 2;
    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (
                (ev.type == SDL_QUIT) ||
                (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_ESCAPE)
            ) {
                running = false;
            }
        }
        if (!running)
            break;

        zmq::message_t msg;
        if (sock.recv(msg)) {
            // Number of samples in received message
            size_t n = msg.size() / sizeof(monitorFmMsg);
            struct monitorFmMsg *samples = (struct monitorFmMsg*)msg.data();
            for (size_t i = 0; i < n; i++) {
                if (x_counter == 0) {
                    SDL_SetRenderDrawColor(rend, 0x30, 0x40, 0x20, 0xFF);
                    SDL_RenderClear(rend);
                }

                auto *sample = &samples[i];
                int plot_x = x_counter / samples_per_pixel;

                plot_rx_rssi.plot(rend, plot_x, sample->rxControl, sample->rxRssi);
                plot_rx_fm  .plot(rend, plot_x, sample->rxControl, sample->rxSample);
                plot_tx_fm  .plot(rend, plot_x, sample->txControl, sample->txSample);
                plot_tx_buf .plot(rend, plot_x, sample->txControl, sample->txBufData);

                if (++x_counter >= samples_per_pixel * win_width) {
                    x_counter = 0;
                    SDL_RenderPresent(rend);
                }
            }
        }
    }

    return 0;
}
