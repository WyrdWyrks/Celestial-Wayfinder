#pragma once

#include <algorithm>
#include <Arduino.h> // millis()
#include "Window.hpp"
#include "WindowState.hpp"
#include "DisplayUtilities.hpp"
#include "HelperClasses/DrawCommands/FnDrawCommand.hpp"

namespace DisplayModule
{

    const int8_t sprite_atlas_w = 72;
    const int8_t sprite_atlas_h = 45;

    // 'Sprite Atlas', 72x45px
    const unsigned char galaga_sprite_atlas [] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x14, 0x03, 0x00, 0x04, 0x00, 0x80, 0x10, 0x00, 0x60, 0x14, 0x05, 0x01, 0x82, 0x00, 
    0x80, 0x20, 0xc0, 0x50, 0x08, 0x06, 0x0e, 0x41, 0x80, 0x80, 0xc1, 0x38, 0x30, 0x08, 0x08, 0x01, 
    0x81, 0x41, 0x41, 0x40, 0xc0, 0x08, 0x08, 0x10, 0x00, 0x00, 0xc1, 0x41, 0x80, 0x00, 0x04, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x94, 0x91, 0x0c, 0x62, 0xc2, 0x21, 0xa3, 0x18, 0x44, 0xdd, 0xbe, 0x16, 0xc3, 0x65, 0xd3, 
    0x61, 0xb4, 0x3e, 0x63, 0x65, 0x63, 0x86, 0x39, 0x4e, 0x30, 0xe3, 0x53, 0x22, 0x48, 0x9e, 0x7d, 
    0xad, 0x5a, 0xdf, 0x3c, 0x89, 0x77, 0x29, 0x90, 0x4a, 0x67, 0x73, 0x29, 0x04, 0xca, 0xd5, 0xf6, 
    0x9e, 0x72, 0x22, 0x22, 0x27, 0x3c, 0xb7, 0x94, 0x98, 0xe3, 0x99, 0x56, 0x35, 0x4c, 0xe3, 0x8c, 
    0x5d, 0x0d, 0x96, 0xcf, 0x8d, 0xd8, 0xf9, 0xb4, 0xd8, 0x22, 0x0b, 0x0c, 0x64, 0x49, 0x49, 0x13, 
    0x18, 0x68, 0x22, 0x09, 0x3f, 0x86, 0x8c, 0x18, 0xb0, 0xfe, 0x48, 0x14, 0x19, 0x22, 0x84, 0x4a, 
    0x29, 0x10, 0xa2, 0x4c, 0xdd, 0xae, 0xd5, 0x3b, 0x29, 0xca, 0x6e, 0x55, 0xba, 0xb6, 0xc5, 0x0f, 
    0xd5, 0x7b, 0x6f, 0x55, 0xf8, 0x51, 0xd5, 0x9b, 0x08, 0x86, 0xcd, 0x59, 0xb0, 0x88, 0x6c, 0xb6, 
    0xd5, 0xcf, 0xd1, 0x4b, 0x69, 0x45, 0xf9, 0xd5, 0x9c, 0xec, 0x95, 0x2b, 0xbd, 0xde, 0xea, 0x54, 
    0x9b, 0xa2, 0x91, 0x22, 0x86, 0x41, 0x41, 0x30, 0xa2, 0x44, 0xc1, 0x9a, 0x3f, 0x82, 0x42, 0x21, 
    0x20, 0xfe, 0x2c, 0x1c, 0x03, 0x9c, 0x03, 0x86, 0x30, 0xe0, 0x1c, 0xe0, 0x22, 0x05, 0x6a, 0x02, 
    0xc9, 0x49, 0xa0, 0x2b, 0x50, 0x36, 0x38, 0xe5, 0xc7, 0x4c, 0x99, 0x71, 0xd3, 0x8e, 0x22, 0x60, 
    0x50, 0xbc, 0x4a, 0xa9, 0x1e, 0x85, 0x03, 0x41, 0x54, 0x8c, 0x35, 0x24, 0x12, 0x56, 0x18, 0x95, 
    0xaa, 0xf1, 0x10, 0xb8, 0x12, 0x24, 0x0e, 0x84, 0x47, 0xc9, 0x9d, 0x25, 0xce, 0x33, 0x66, 0x39, 
    0xd2, 0x5c, 0x94, 0x8b, 0x2a, 0x01, 0x52, 0x25, 0x40, 0x2a, 0x68, 0x63, 0x0e, 0x1c, 0x00, 0xe1, 
    0xc3, 0x80, 0x1c, 0x38, 0x08, 0x11, 0xfc, 0x00, 0xca, 0xa9, 0x80, 0x1f, 0xc4, 0x08, 0x33, 0x48, 
    0x01, 0x68, 0x8b, 0x40, 0x09, 0x66, 0x14, 0x6e, 0xe4, 0x04, 0x3d, 0xde, 0x10, 0x13, 0xbb, 0x14, 
    0x43, 0x8f, 0x82, 0x4b, 0x69, 0x20, 0xf8, 0xe1, 0x14, 0x25, 0x38, 0x69, 0x41, 0x41, 0x4b, 0x0e, 
    0x52, 0xb6, 0x89, 0x0f, 0x90, 0xe1, 0x43, 0x84, 0xf8, 0x48, 0xdd, 0x90, 0xe4, 0x1b, 0xb1, 0x46, 
    0xec, 0x13, 0x84, 0x88, 0x85, 0x88, 0x0c, 0xd0, 0x85, 0x98, 0x08, 0xd0, 0xaa, 0x83, 0x3c, 0x04, 
    0x70, 0x87, 0x10, 0x1e, 0x60 };


    enum UnitType {
        BULLET,   
        ABDUCTOR, //abducts player ship   
        SINE,     //butterfly enemies that approach in sine wave pattern
        BEE,      //the yellow bees that loop around behind you
        PLAYER,
    };

    class Unit {
        int8_t x_center_;
        int8_t y_center_;
        int8_t collision_radius;

        int8_t orientation_code=0; 

        UnitType type = PLAYER;


        
        // Fast 2D distance using integer math (scaled by 1024 to avoid floats)
        uint32_t fastDist2D2(int32_t dx, int32_t dy) {
            uint32_t abs_x = abs(dx);
            uint32_t abs_y = abs(dy);
            uint32_t max_val = (abs_x > abs_y) ? abs_x : abs_y;
            uint32_t min_val = (abs_x > abs_y) ? abs_y : abs_x;
            
            // 0.414 is approximately 424 / 1024
            return max_val + ((424 * min_val) >> 10); 
        }



        public:
            Unit(int x_c=64, int y_c=64,int coll_radius=4) {
                this->x_center_=x_c;
                this->y_center_=y_c;
                this->collision_radius=coll_radius;
            }

            void setOrientation(uint8_t orientation) {
                this->orientation_code = orientation % 8;
            }

            uint8_t getOrientation() {
                return this->orientation_code;
            }

            bool testCollision(int x_other, int y_other, int coll_radius_other) {
                
                uint32_t dx = abs(this->x_center_-x_other);
                uint32_t dy = abs(this->y_center_-y_other); 
                uint32_t distance = fastDist2D2(dx,dy);
                if (distance<=(coll_radius_other+this->collision_radius)) {
                    return true;
                }
                
                return false;
            }

            void _draw(DrawContext &ctx) {
                auto *d = ctx.display;
                //d->fillScreen(BLACK);
                int8_t i, j, byteWidth = (sprite_atlas_w + 7) / 8;

                /*for (j = 0; j < 9; j++) {
                    for (i = 0; i < 9; i++) {
                        if (pgm_read_byte(&galaga_sprite_atlas + (j* byteWidth ) + ((i+orientation_code*9) / 8)) & (B10000000 >> (i % 8))) {
                            int8_t drawX = x_center_ + i - 5;

                            int8_t drawY = y_center_ + j - 5;
                            
                            if(drawX >= 0 && drawX < 128 && drawY >= 0 && drawY < 128) {
                                d->drawPixel(drawX,drawY,1);
                            }
                        }
                    }
                }*/
               int x=0;
               int y=0;
               int b=0;
               d->startWrite();
                for (int16_t j = 0; j < 45; j++, y++) {
                    for (int16_t i = 0; i < 72; i++) {
                    if (i & 7)
                        b <<= 1;
                    else
                        b = galaga_sprite_atlas[j * byteWidth + i / 8];
                        d->writePixel(x + i, y, (b & 0x80) ? 1 : 0);
                    }
                }
                d->endWrite();

            }
            
    }; 







    // -------------------------------------------------------------------------
    // GalagaGame
    // -------------------------------------------------------------------------
    // Full breakout game in a single state.
    //
    // Controls:
    //   ENC_UP   → move paddle right
    //   ENC_DOWN → move paddle left
    //   BUTTON_3 → exit (handled by BreakoutWindow)
    //   BUTTON_4 → restart
    //
    // Layout (128 × 128):
    //   Row 0–9    : status bar (score left, lives right)
    //   Row 10–127 : game field (bricks, ball, paddle)

    class GalagaGameState : public WindowState
    {
    public:
        static constexpr int W           = 128;
        static constexpr int H           = 128;
        static constexpr int STATUS_H    = 10;

        static constexpr int BRICK_COLS  = 6;
        static constexpr int BRICK_ROWS  = 5;
        static constexpr int BRICK_W     = 18;
        static constexpr int BRICK_H     = 5;
        static constexpr int BRICK_GAP_X = 2;
        static constexpr int BRICK_GAP_Y = 2;
        static constexpr int BRICK_LEFT  = (W - (BRICK_COLS * BRICK_W + (BRICK_COLS - 1) * BRICK_GAP_X)) / 2;
        static constexpr int BRICK_TOP   = STATUS_H + 3;

        static constexpr int PADDLE_W    = 24;
        static constexpr int PADDLE_H    = 3;
        static constexpr int PADDLE_Y    = H - 10;
        static constexpr int PADDLE_STEP = 8;

        static constexpr int BALL_SZ     = 3;

        static constexpr int TOTAL_BRICKS = BRICK_ROWS * BRICK_COLS;

        // Render cadence: how often the frame is redrawn (40 FPS).
        static constexpr uint32_t FRAME_MS = 25;
        // Physics cadence: how often the simulation advances one step. The ball
        // velocities are tuned per-step, so this fixes the ball's real speed
        // independent of FRAME_MS.
        static constexpr uint32_t STEP_MS  = 25;
        // Cap on catch-up steps in a single update so a long stall can't make
        // the simulation spiral trying to replay a huge backlog.
        static constexpr int MAX_STEPS = 4;

        GalagaGameState()
        {
            refreshIntervalMs = FRAME_MS;

            // Bindings live in the constructor so they are registered exactly once.
            // Each paddle move also advances the simulation: a stream of encoder
            // inputs keeps the queue busy so the autonomous refresh timeout (which
            // drives onTick) never fires — without this the ball would freeze for
            // as long as the paddle is moving.
            bindInput(InputID::ENC_UP, [this](const InputContext &)
            {
               // _paddleX = std::min(_paddleX + PADDLE_STEP, W - PADDLE_W);
                _stepPhysics();
            });

            bindInput(InputID::ENC_DOWN, [this](const InputContext &)
            {
                //_paddleX = std::max(_paddleX - PADDLE_STEP, 0);
                _stepPhysics();
            });

            bindInput(InputID::BUTTON_4, "Restart", [this](const InputContext &)
            {
                _resetGame();
            });
        }

        void onEnter(const StateTransferData &) override
        {
            _resetGame();
            addDrawCommand(std::make_shared<FnDrawCommand>([this](DrawContext &ctx)
            {
                _draw(ctx);
            }));
        }

        void onTick() override
        {
            _stepPhysics();
            player.setOrientation(player.getOrientation()+1);
        }

    private:
        uint8_t _score=69;
        uint8_t _lives=3;

        Unit player;

        void _resetGame()
        {
            
        }

        static float _fabs(float v) { return v < 0 ? -v : v; }

        // Advance the simulation on a fixed STEP_MS timestep using real elapsed
        // time. Called both from onTick() (autonomous refresh) and from the
        // paddle input handlers, so the ball keeps moving at a constant speed
        // regardless of the render rate or how fast inputs are arriving.
        void _stepPhysics()
        {
            
        }

        void _update()
        {
           
        }

        void _draw(DrawContext &ctx)
        {
            auto *d = ctx.display;
            d->fillScreen(BLACK);

            // --- Status bar ---
            d->setTextSize(1);
            d->setTextColor(DrawCommand::DrawColorPrimary());
            d->setCursor(2, 1);
            d->print("SC:");
            d->print(_score);
            player._draw(ctx);

            // Lives as small filled squares on the right
            for (int i = 0; i < _lives && i < 5; ++i)
                d->fillRect(W - 4 - i * 5, 2, 3, 3, DrawCommand::DrawColorPrimary());

            d->drawFastHLine(0, STATUS_H - 1, W, DrawCommand::DrawColorPrimary());

            // --- Game Over / Win overlay ---
            // if (_over)
            // {
            //     d->setCursor(26, 52);
            //     d->print("GAME  OVER");
            //     d->setCursor(26, 64);
            //     d->print("Score: ");
            //     d->print(_score);
            //     d->setCursor(14, 76);
            //     d->print("[4] Restart");
            //     return;
            // }
            // if (_won)
            // {
            //     d->setCursor(32, 52);
            //     d->print("YOU  WIN!");
            //     d->setCursor(26, 64);
            //     d->print("Score: ");
            //     d->print(_score);
            //     d->setCursor(14, 76);
            //     d->print("[4] Again");
            //     return;
            // }

        }
    };

    // -------------------------------------------------------------------------
    // GalagaWindow
    // -------------------------------------------------------------------------
    // Thin window wrapper around BreakoutGameState.
    //
    // Usage:
    //   Utilities::pushWindow(std::make_shared<BreakoutWindow>());

    class GalagaWindow : public Window
    {
    public:
        GalagaWindow()
        {
            _game = std::make_shared<GalagaGameState>();

            registerInput(InputID::BUTTON_3, "Back");
            addInputCommand(InputID::BUTTON_3, [](const InputContext &)
            {
                Utilities::popWindow();
            });

            setInitialState(_game);
        }

    private:
        std::shared_ptr<GalagaGameState> _game;
    };

} // namespace DisplayModule
