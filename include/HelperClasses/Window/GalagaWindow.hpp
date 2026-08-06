#pragma once

#include <algorithm>
#include <vector>
#include <Arduino.h> // millis()
#include "Window.hpp"
#include "WindowState.hpp"
#include "DisplayUtilities.hpp"
#include "HelperClasses/DrawCommands/FnDrawCommand.hpp"

namespace DisplayModule
{
    //out of bounds detection
    const int32_t oob_x_min_100 = -500;
    const int32_t oob_y_min_100 = -500;
    const int32_t oob_x_max_100 = 12800+500;
    const int32_t oob_y_max_100 = 12800+500;

    const int8_t sprite_atlas_w = 72;
    const int8_t sprite_atlas_h = 117;

    const int8_t sprite_w = 9;
    const int8_t sprite_h = 9;

    // 'Sprite Atlas', 72x45px
    static const byte galaga_sprite_atlas[] PROGMEM  = {
    0x3e, 0x20, 0xb5, 0x7a, 0xb0, 0x00, 0x00, 0x00, 0x00, 
    0x22, 0x2e, 0xb5, 0x69, 0x20, 0x00, 0x00, 0x00, 0x00, 
    0x3e, 0x28, 0xb2, 0x7a, 0xb0, 0x00, 0x00, 0x00, 0x00, 
    0x3e, 0x2e, 0x95, 0x48, 0x20, 0x00, 0x00, 0x00, 0x00, 
    0x22, 0x22, 0x95, 0x5a, 0xb0, 0x00, 0x00, 0x00, 0x00, 
    0x22, 0x2c, 0x90, 0x49, 0x20, 0x00, 0x00, 0x00, 0x00, 
    0x22, 0x11, 0x08, 0x9a, 0xb0, 0x00, 0x00, 0x00, 0x00, 
    0x14, 0x0a, 0x05, 0x04, 0x40, 0x00, 0x00, 0x00, 0x00, 
    0x08, 0x04, 0x02, 0x03, 0x80, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 
    0x40, 0x40, 0x30, 0x04, 0x04, 0x02, 0x01, 0x80, 0xc9, 
    0xc0, 0x20, 0x20, 0x0d, 0x22, 0x91, 0x49, 0x24, 0x4a, 
    0x48, 0x64, 0x32, 0x05, 0x46, 0xa3, 0x51, 0xa8, 0x4c, 
    0x54, 0x4a, 0x15, 0x05, 0x84, 0xc1, 0x60, 0xb0, 0x0a, 
    0x49, 0x64, 0xa2, 0x45, 0x46, 0xa2, 0x51, 0x28, 0x49, 
    0x02, 0x81, 0x40, 0xa1, 0x20, 0x90, 0x48, 0x24, 0xa0, 
    0x01, 0x00, 0x80, 0x40, 0x00, 0x00, 0x00, 0x00, 0x40, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x08, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x01, 0x80, 
    0x08, 0x03, 0xd8, 0x01, 0x86, 0x30, 0xc0, 0x0d, 0xe0, 
    0x1c, 0x0e, 0x9e, 0x01, 0x87, 0xf0, 0xc0, 0x3c, 0xb8, 
    0x14, 0x39, 0x8b, 0x82, 0xc2, 0x21, 0xa0, 0xe8, 0xce, 
    0x36, 0x31, 0x08, 0xec, 0x43, 0x61, 0x1b, 0x88, 0x46, 
    0x22, 0x0b, 0x0b, 0x8e, 0x61, 0x43, 0x38, 0xe8, 0x68, 
    0x7f, 0x06, 0x1e, 0x03, 0xa1, 0xc2, 0xe0, 0x3c, 0x30, 
    0x63, 0x06, 0x18, 0x00, 0xf0, 0x87, 0x80, 0x0c, 0x30, 
    0x00, 0x00, 0x00, 0x00, 0x30, 0x86, 0x00, 0x00, 0x00, 
    0x1c, 0x03, 0x00, 0x00, 0x00, 0x80, 0xe0, 0x00, 0x60, 
    0x22, 0x04, 0x84, 0x0e, 0x03, 0xc1, 0xf8, 0x18, 0x94, 
    0x14, 0x06, 0x4f, 0x4f, 0x06, 0xe2, 0x39, 0x4d, 0x32, 
    0x22, 0x09, 0x58, 0xbc, 0x84, 0xb0, 0xda, 0xa5, 0x4b, 
    0x24, 0x11, 0xbc, 0x3a, 0x61, 0x23, 0x12, 0x1e, 0xcb, 
    0x69, 0x36, 0x12, 0xba, 0x52, 0x25, 0x22, 0x8c, 0x27, 
    0x3b, 0x38, 0x99, 0x49, 0x91, 0x44, 0xc1, 0x78, 0x1e, 
    0x1e, 0x3f, 0x0c, 0x05, 0x22, 0x22, 0x40, 0x10, 0x0e, 
    0x08, 0x0e, 0x00, 0x00, 0xc1, 0xc1, 0x80, 0x00, 0x00, 
    0x00, 0x00, 0x01, 0x10, 0x00, 0x80, 0x04, 0x40, 0x00, 
    0x08, 0x24, 0x01, 0x08, 0x20, 0x82, 0x08, 0x40, 0x12, 
    0x36, 0x1b, 0x02, 0x84, 0xc1, 0x41, 0x90, 0xa0, 0x6c, 
    0xc1, 0x91, 0x0c, 0x83, 0x41, 0x41, 0x60, 0x98, 0x44, 
    0x22, 0x08, 0xb0, 0x42, 0x22, 0x22, 0x21, 0x06, 0x88, 
    0x14, 0x0d, 0x0c, 0x84, 0x4c, 0x19, 0x10, 0x98, 0x58, 
    0x14, 0x13, 0x02, 0x86, 0xc3, 0x61, 0xb0, 0xa0, 0x64, 
    0x08, 0x20, 0x81, 0x09, 0x00, 0x80, 0x48, 0x40, 0x82, 
    0x08, 0x40, 0x01, 0x00, 0x00, 0x00, 0x00, 0x40, 0x01, 
    0x08, 0x04, 0x02, 0x00, 0x04, 0x42, 0x21, 0x98, 0x44, 
    0x08, 0x04, 0x0a, 0x04, 0x82, 0xc9, 0x64, 0x82, 0x00, 
    0x14, 0x0a, 0x05, 0x83, 0xa1, 0xd0, 0x8a, 0x07, 0x01, 
    0x14, 0x0a, 0x05, 0x4d, 0x4f, 0x66, 0x12, 0x00, 0x00, 
    0x14, 0x22, 0x10, 0x06, 0xc6, 0x32, 0x08, 0x00, 0x00, 
    0xb6, 0xcf, 0x60, 0xa5, 0x63, 0x79, 0x0c, 0x02, 0x00, 
    0xdd, 0x8e, 0xc5, 0x6b, 0x85, 0xc2, 0x23, 0x03, 0x01, 
    0x88, 0x84, 0x42, 0x22, 0x49, 0xa4, 0xd2, 0x08, 0x00, 
    0xaa, 0x95, 0x4a, 0xa0, 0x01, 0x10, 0x88, 0xcc, 0x44, 
    0x00, 0x00, 0x00, 0x00, 0x04, 0x42, 0x20, 0x10, 0x00, 
    0x00, 0x00, 0x00, 0x04, 0x82, 0x48, 0x04, 0x00, 0x00, 
    0x00, 0x00, 0x04, 0x02, 0x20, 0x10, 0x00, 0x00, 0x00, 
    0x14, 0x04, 0x02, 0x88, 0x4c, 0x04, 0x02, 0x00, 0x00, 
    0x08, 0x0e, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x14, 0x04, 0x0a, 0x04, 0x20, 0x18, 0x04, 0x02, 0x00, 
    0x00, 0x00, 0x01, 0x08, 0x84, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x02, 0x49, 0x24, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x01, 0x10, 0x88, 0x40, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x14, 0x03, 0x00, 0x04, 0x00, 0x80, 0x10, 0x00, 0x60, 
    0x14, 0x05, 0x01, 0x82, 0x00, 0x80, 0x20, 0xc0, 0x50, 
    0x08, 0x06, 0x0e, 0x41, 0x80, 0x80, 0xc1, 0x38, 0x30, 
    0x08, 0x08, 0x01, 0x81, 0x41, 0x41, 0x40, 0xc0, 0x08, 
    0x08, 0x10, 0x00, 0x00, 0xc1, 0x41, 0x80, 0x00, 0x04, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x1c, 0x08, 0x0c, 0x07, 0x82, 0x20, 0xf0, 0x18, 0x08, 
    0x63, 0x06, 0x12, 0x42, 0x45, 0x51, 0x21, 0x24, 0x30, 
    0x14, 0x3d, 0x21, 0x52, 0x49, 0x49, 0x25, 0x42, 0x5e, 
    0x22, 0x40, 0x98, 0xbc, 0x58, 0x8d, 0x1e, 0x8c, 0x81, 
    0x41, 0x41, 0x84, 0x30, 0x64, 0x13, 0x06, 0x10, 0xc1, 
    0x88, 0xf1, 0x58, 0xb0, 0x22, 0x22, 0x06, 0x8d, 0x47, 
    0x94, 0xc9, 0x21, 0x4f, 0x41, 0x41, 0x79, 0x42, 0x49, 
    0x55, 0x09, 0x12, 0x41, 0x86, 0x30, 0xc1, 0x24, 0x48, 
    0x22, 0x1e, 0x0c, 0x02, 0x01, 0xc0, 0x20, 0x18, 0x3c, 
    0x1c, 0x08, 0x0c, 0x07, 0x82, 0x20, 0xf0, 0x18, 0x08, 
    0x77, 0x06, 0x12, 0x42, 0x47, 0x71, 0x21, 0x24, 0x30, 
    0x14, 0x3d, 0x39, 0x53, 0x4b, 0x69, 0x65, 0x4e, 0x5e, 
    0x3e, 0x44, 0x9f, 0xff, 0x59, 0xcd, 0x7f, 0xfc, 0x91, 
    0x5d, 0x5f, 0x87, 0x37, 0xe5, 0xd3, 0xf6, 0x70, 0xfd, 
    0x9c, 0xfd, 0x5f, 0xf1, 0x23, 0xe2, 0x47, 0xfd, 0x5f, 
    0xb6, 0xcd, 0x39, 0x4f, 0x41, 0x41, 0x79, 0x4e, 0x59, 
    0x77, 0x09, 0x12, 0x41, 0x87, 0x70, 0xc1, 0x24, 0x48, 
    0x22, 0x1e, 0x0c, 0x02, 0x01, 0xc0, 0x20, 0x18, 0x3c, 
    0x22, 0x09, 0x3f, 0x86, 0x8c, 0x18, 0xb0, 0xfe, 0x48, 
    0x14, 0x19, 0x22, 0x84, 0x4a, 0x29, 0x10, 0xa2, 0x4c, 
    0xdd, 0xae, 0xd5, 0x3b, 0x29, 0xca, 0x6e, 0x55, 0xba, 
    0xb6, 0xc5, 0x0f, 0xd5, 0x7b, 0x6f, 0x55, 0xf8, 0x51, 
    0xd5, 0x9b, 0x08, 0x86, 0xcd, 0x59, 0xb0, 0x88, 0x6c, 
    0xb6, 0xd5, 0xcf, 0xd1, 0x4b, 0x69, 0x45, 0xf9, 0xd5, 
    0x9c, 0xec, 0x95, 0x2b, 0xbd, 0xde, 0xea, 0x54, 0x9b, 
    0xa2, 0x91, 0x22, 0x86, 0x41, 0x41, 0x30, 0xa2, 0x44, 
    0xc1, 0x9a, 0x3f, 0x82, 0x42, 0x21, 0x20, 0xfe, 0x2c, 
    0x1c, 0x03, 0x9c, 0x03, 0x86, 0x30, 0xe0, 0x1c, 0xe0, 
    0x22, 0x05, 0x6a, 0x02, 0xc9, 0x49, 0xa0, 0x2b, 0x50, 
    0x36, 0x38, 0xe5, 0xc7, 0x4c, 0x99, 0x71, 0xd3, 0x8e, 
    0x22, 0x60, 0x50, 0xbc, 0x4a, 0xa9, 0x1e, 0x85, 0x03, 
    0x41, 0x54, 0x8c, 0x35, 0x24, 0x12, 0x56, 0x18, 0x95, 
    0xaa, 0xf1, 0x10, 0xb8, 0x12, 0x24, 0x0e, 0x84, 0x47, 
    0xc9, 0x9d, 0x25, 0xce, 0x33, 0x66, 0x39, 0xd2, 0x5c, 
    0x94, 0x8b, 0x2a, 0x01, 0x52, 0x25, 0x40, 0x2a, 0x68, 
    0x63, 0x0e, 0x1c, 0x00, 0xe1, 0xc3, 0x80, 0x1c, 0x38, 
    0x08, 0x11, 0xfc, 0x00, 0xca, 0xa9, 0x80, 0x1f, 0xc4, 
    0x08, 0x33, 0x48, 0x01, 0x68, 0x8b, 0x40, 0x09, 0x66, 
    0x14, 0x6e, 0xe4, 0x04, 0x3d, 0xde, 0x10, 0x13, 0xbb, 
    0x14, 0x43, 0x8f, 0x82, 0x4b, 0x69, 0x20, 0xf8, 0xe1, 
    0x14, 0x25, 0x38, 0x69, 0x41, 0x41, 0x4b, 0x0e, 0x52, 
    0xb6, 0x89, 0x0f, 0x90, 0xe1, 0x43, 0x84, 0xf8, 0x48, 
    0xdd, 0x90, 0xe4, 0x1b, 0xb1, 0x46, 0xec, 0x13, 0x84, 
    0x88, 0x85, 0x88, 0x0c, 0xd0, 0x85, 0x98, 0x08, 0xd0, 
    0xaa, 0x83, 0x3c, 0x04, 0x70, 0x87, 0x10, 0x1e
    };

    //what sprite does it have?
    //also handles state info (setting any enemy to EXPLOSION_ENEMY starts the explosion anim)
    //this unfortunate fact results in there needing to be another bool to handle if a BULLET
    //is a PLAYER BULLET or ENEMY BULLET (different collision layers....)
    enum Type {
        LEVEL_TICK, //level tick   
        BONUS_POPUP, //score bonus numbers   
        DELTA, //triangle enemies, spawned in groups of 3
        SCORPION, //guy with the tail, spawned in groups of 3
        EXPLOSION_PLAYER,
        EXPLOSION_ENEMY,
        BULLET,
        GALAGA_DARK,  //if shot, die
        GALAGA_LIGHT, //if shot, turn into GALAGA_DARK
        BUTTERFLY, //sine wave butterfly
        BEE, //yellow bee
        PLAYER, //player ship
        NONE, //after the explosion anim plays, bullet hits
    };

    enum AiState {
        IDLE,                     //AI is off stage, not spawned, not simulated
        ENTER,                    //enter stage, fly to spot in formation (if no spot in formation is assigned, it'll ram and despawn!)
        CHALLENGING,              //fly in selected path, then despawn. Don't shoot at player
        FORMATION,                //hover with formation
        ATTACK_RUN,               //go on an attack run (takes into account Type for path info)
        ATTACK_RUN_SPLIT_SCORP,   //split into 3 following scorpions
        ATTACK_RUN_SPLIT_DELTA,   //split into 3 offset delta fliers
        ATTACK_RUN_SPLIT_RAM,     //split into 3 rammers, 2 go directly for player, 1 follows standard attack pattern
        ATTACK_RUN_ABDUCT         //only boss galaga can do this. Try and steal the player's ship!
    };

    //after you kill a special enemy or 100% a challenging stage attack run, a bonus may trigger after the enemy's explode  anim
    enum BonusType {
        _NONE,
        _100,
        _200,
        _500,
        _1000,
        _2000,
        _3000,
        _5000,
        _10000
    };

    enum LevelTickType {
        _1X,
        _5X,
        _10X,
        _20X
    };


    //Pathfinding System and control 
    //A Path consists of a set of PathSegments
    //All game-controlled entities follow a Path
    //Bullets use a FLY_THRU path set to target in front of the ship
    //Rammers use a FLY_THRU set to your current player ship position

    //Enemies mostly use complex paths consisting of straight flight interleaved with Turns
    //FLY_THRU is used for when we want the object to leave the stage. Fired bullet, rammer, or last step on an Enemy's flight plan
    //FLY_TO is used for straight segments where we are not yet ready to leave the stage
    //FLY_STRAIGHT tells the unit to travel in its facing direction by specified amount of pixels.

    //TURN_LEFT/TURN_RIGHT_AUTO are autogenerated turns. The turn is executed once the entity gets to the end of the last segment.
    //a virtual point turn_radius_100/100 away but perpendicular to the unit's path is created. The unit rotates around that point in a sweeping turn.
    //Auto Turns are terminated when we have turned more than turn_min_degrees_100 and when the entity's orientation aligns with the straight-line orientation to the next nav point.
    //Fixed turns (TURN_LEFT_FIXED/TURN_RIGHT_FIXED) require the unit to turn the specified amount of degrees.

    //ROTATE_TO has the unit rotate towards an orientation in place.

    //Complex paths 
    enum PathSegmentType {
        FLY_THRU, 
        FLY_TO, 
        FLY_TO_MOVING_TARGET, //fly towards something that is itself moving
        FLY_STRAIGHT,         //fly in the current direction the unit is facing
        TURN_LEFT_AUTO,       //turns solve for the tangent of the path to the next node
                              //the unit will turn around turn_radius to try and line up with the straight line path to the next node
        TURN_RIGHT_AUTO,

        TURN_LEFT_FIXED,
        TURN_RIGHT_FIXED,

        ROTATE_TO, //rotates in place to desired orientation. Used when a unit gets back to its formation spot but needs to rotate to orientation
    };

    struct PathSegment {

        int32_t speed_100;       //100x the speed in pixels per second that this path segment can be taken at

        int32_t turn_radius_100; //100X the turn radius in pixels of this node. Does nothing if this isn't a turn!
        int32_t turn_min_degrees_100 = 2250;//default to 22.5 degrees of turn minimum. This is to ensure some turning happens if we schedule a turn while facing almost the direction of the next straightline move segment
        int32_t turn_max_degrees_100 = 38250; //both of these are used for auto turns

        int32_t turn_degrees_100 = 500; //only used for fixed turns
        int32_t initial_orientation_degrees_100=-1; //for turns, what was our initial orientation? n
                                                    //needed to keep track of how far we've turned

        int32_t orientation_target_degrees_100; //target in degrees to rotate to x100
        int32_t orientation_rotate_speed_100 = -1; //amount of degreesx100 to rotate per tick 

        int32_t target_distance_100; //used for FLY_STRAIGHT
        int32_t distance_travelled_100=0; 

        int32_t initial_position_x_100=-1; //sets on entering FLY_THRU state. Used to calculate fly thru 
        int32_t initial_position_y_100=-1;

        int32_t target_pos_x_100; //100x the X coord of the FLY_THRU or FLY_TO value. Where it'll try to go. 
        int32_t target_pos_y_100; //         Y                           

        //construct path segment of type
        //goal is to set a sane default for all types
        PathSegment(PathSegmentType default_type=FLY_THRU, int32_t target_x_100=6400, int32_t target_y_100=12800) {
            this->type=default_type; //set the type
            this->speed_100 = 100;//hundredths of pixels per update frame. Assume 64 pixels/2s, 32 pixels/s, @30fps that's about 1 pix/frame
            this->turn_radius_100=900; //9 px default turn radius

            switch(default_type) {
                case FLY_THRU:
                {
                    this->target_pos_x_100=target_x_100;
                    this->target_pos_y_100=target_y_100;
                }
                break;
                case FLY_TO:
                {
                    this->target_pos_x_100=target_x_100;
                    this->target_pos_y_100=target_y_100;
                }
                break;
                case FLY_STRAIGHT:
                {
                    this->target_distance_100=3200; //32 pixels
                }
                break;
                case TURN_LEFT_AUTO:
                {
                    //in theory this should autoconfig to align to next straight seg
                }
                break;
                case TURN_RIGHT_AUTO:
                {
                    //in theory this should autoconfig to align to next straight seg
                }
                break;
                case TURN_LEFT_FIXED:
                {
                    this->turn_degrees_100=9000; //90 degrees, why the fuck not?
                }
                break;
                case TURN_RIGHT_FIXED:
                {
                    this->turn_degrees_100=-9000; //apparently right turns are neg
                }
                break;
                case ROTATE_TO:
                {
                    this->orientation_target_degrees_100=27000; //default rotate_to is facing down (towards player)
                    //this would mainly be used for orientation after a ship rejoins formation
                }
                break;
                default:
                    //oopsi, did not implement
                break;
            }
        }

        PathSegmentType type=PathSegmentType::FLY_THRU;
            
    };


    class Path {
        std::vector<PathSegment> path_segments;
        int32_t speed_multiplier_100 = 100; //This number / 100 is the speed multiplier for the entire path

        int32_t euclidian_distance(int32_t dx, int32_t dy) {
            int32_t abs_x = abs(dx);
            int32_t abs_y = abs(dy);
            int32_t max_val = (abs_x > abs_y) ? abs_x : abs_y;
            int32_t min_val = (abs_x > abs_y) ? abs_y : abs_x;
            
            // 0.414 is approximately 424 / 1024
            return max_val + ((424 * min_val) >> 10); 
        }

        //travel straight in the direction of our current orientation
        //shoot_thru: do we want to fly through our target_pos_x/y?
        //returns: false if we shouldn't keep going (reached our target pos and shoot_thru is false)
        bool fly_straight(int32_t * position_x_100, int32_t * position_y_100, int32_t * orientation_degrees_100, bool shoot_thru = true ) {
            uint32_t speed_100 = (((float)this->path_segments.back().speed_100))*(((float)speed_multiplier_100)/100);
            bool return_value = true;
            if(shoot_thru==false) {
                int32_t distance_to_target_100 = euclidian_distance(this->path_segments.back().target_pos_x_100-*position_x_100, this->path_segments.back().target_pos_y_100-*position_y_100);
                if(distance_to_target_100<speed_100) {
                    speed_100 = distance_to_target_100;
                    return_value = false;
                }
            }
            int32_t x_delta = cosf((float)(*orientation_degrees_100/100)*(M_PI)/(float)180.0)*speed_100;
            int32_t y_delta = sin((float)(*orientation_degrees_100/100)*(M_PI)/(float)180.0)*speed_100;

            *position_x_100 = *position_x_100 + x_delta;
            *position_y_100 = *position_y_100 + y_delta;

            return return_value;
        }

        uint32_t get_degrees_100_between_points(int32_t x1, int32_t y1, int32_t x2, int32_t y2) {
            int32_t delta_x_100 = x1 - x2;
            int32_t delta_y_100 = y1 - y2;

            float theta = atan2f(delta_x_100, delta_y_100);//angle, in radians, of the path from initial pos to position we're flying thru
            float theta_deg = theta * (180/M_PI);
            return (int32_t)(theta_deg*100.0); //get orientation in x100 degrees
        }

        //used for FLY_THRU and FLY_TO. Sets the unit's orientation based on the correct orientation to fly_straight() to the 
        //FLY_THRU/TO CMD's target_pos_x_100 and target_pos_y_100 locations
        void test_set_orientation_to_point(int32_t * position_x_100, int32_t * position_y_100, int32_t * orientation_degrees_100) {
            //we just entered this state, set our inital pos and orientation
            if(this->path_segments.back().initial_position_x_100==-1||this->path_segments.back().initial_position_x_100==-1){
                this->path_segments.back().initial_position_x_100=*position_x_100; //set initial pos so this if shouldn't run more than once per FLY_THRU command
                this->path_segments.back().initial_position_y_100=*position_y_100;                            

                *orientation_degrees_100 = get_degrees_100_between_points(*position_x_100, *position_y_100, this->path_segments.back().target_pos_x_100,this->path_segments.back().target_pos_y_100);
            }
        }

                //helper function to detect if unit is OOB
        bool is_out_of_bounds(int32_t position_x_100, int32_t position_y_100) {
            if(position_y_100<oob_y_min_100||position_y_100>oob_y_max_100||position_x_100<oob_x_min_100||position_x_100>oob_x_max_100) { return true;}
            return false;
        }

        //this sets up turns to see if they've set their initial orientation (x100) and angular rotation speed (x100)
        //angular rotation speed is calculated from a turn's speed_100 value
        //deg_mag turn controls whether it is a left or right turn
        int32_t set_theta_deg_100_from_speed_100(int32_t * initial_orientation_100, int32_t deg_mag_turn) {
            if(this->path_segments.back().orientation_rotate_speed_100==-1||this->path_segments.back().initial_orientation_degrees_100==-1) {
                int32_t r_turn_100 = this->path_segments.back().turn_radius_100;
                int32_t speed_100 = this->path_segments.back().speed_100;
                this->path_segments.back().orientation_rotate_speed_100 = (deg_mag_turn*(speed_100)*36000)/(2*M_PI*r_turn_100);
                this->path_segments.back().initial_orientation_degrees_100 = *initial_orientation_100;
            }
        }

        //checks if the amount we're about to turn results in overshooting the target orientation.
        //returns how much we should turn.
        //returns rot_speed_100 x100 degrees if we won't hit our target
        //returns less if needed to perfectly hit deg_100_needed orientation
        int32_t check_for_turn_overshoot(int32_t rot_speed_100, int32_t orientation_100, int32_t deg_100_needed) {
            if(rot_speed_100<0) { //right turn, decreasing angle (we want our orientation to decrease towrads 100_needed)
                //we could be at 1 degrees, and wrap around to 359
                //we could also be at 359 degrees and decrease to 358
                //it is possible for deg_100_needed to be greater than orientation
                if(deg_100_needed>orientation_100) { 
                    //if deg_100_needed is 359 and orientation is 1, orientation is less than deg100. We don't want that
                    //so this shift should push deg_100 to 179 and orientation to 181
                    deg_100_needed = (deg_100_needed-18000)%36000;
                    orientation_100 = (orientation_100-18000)%36000;
                }

                //this should be a negative number, like rot_speed_100
                int32_t delta_needed = deg_100_needed-orientation_100;

                if (delta_needed>=rot_speed_100) {
                    return delta_needed;
                } else {
                    return rot_speed_100;
                }


            } else { //left turn, increasing angle (we want our orientation to increase towards deg_100_needed)
                //we could be at 359 degrees, and wrap around to 1 degrees
                //we could also be at 1 degree and increase to 2 degrees
                //it is possible for deg_100_needed to be less than orientation
                if(deg_100_needed<orientation_100)
                {
                    deg_100_needed = (deg_100_needed+18000)%36000;
                    orientation_100 = (orientation_100+18000)%36000;
                    //if deg_100_needed is too high, this adjusts it away from the 0-36000 discontinuity
                    //orientation_100 is adjusted too: but gets a modulo to ensure it is not negative after
                    //this next part only works if 
                }

                //this should be a positive number, like rot_speed_100
                int32_t delta_needed = deg_100_needed-orientation_100;
                if(delta_needed<=rot_speed_100) {
                    return delta_needed; //we don't need the full rotation to hit target
                } else {
                    return rot_speed_100; //we can't hit the target, just keep rotating at speed
                }
            }
        }


        //If ship turns theta_deg_100_to_turn in x100 degrees, will the ship be lined up to fly 
        //to next segment's position?
        //Returns rot_speed_100 if it is not possible to align within theta_deg_100_to_turn amount of x100 degrees. (keep turning)        
        //returns the amount of x100 degrees needed to perfectly line up with the next target pos, if
        //that required amount is less than theta_deg_100_to_turn.
        //
        int32_t check_if_alignable(int32_t * orientation_100, int32_t * position_x_100, int32_t * position_y_100) {
            if (this->path_segments.size()<2) {
                return -1;
            } else { //a segment is ahead
                PathSegment seg = this->path_segments[this->path_segments.size()-2];
                //if type of the next segment is straight
                if (seg.type==PathSegmentType::FLY_THRU||
                    seg.type==PathSegmentType::FLY_TO||
                    seg.type==PathSegmentType::FLY_TO_MOVING_TARGET){

                    //figure out angle needed to point our ship at the target
                    int32_t deg_100_needed = get_degrees_100_between_points(*position_x_100,*position_y_100,
                        seg.target_pos_x_100,seg.target_pos_y_100);

                    //determine if this rotation 
                    int32_t rot_speed_100 = this->path_segments.back().orientation_rotate_speed_100;
                    
                    return check_for_turn_overshoot(rot_speed_100, *orientation_100, deg_100_needed);

                } else {
                    //if(*orientation_100)
                    //NOT SUPPORTED TO NOT HAVE STRAIGHT LINE SEG AFTER TURN
                    //DON'T DO THIS
                    return this->path_segments.back().orientation_rotate_speed_100/2;
                }
            }

        }

        int32_t mirror_x_over_centerline(int32_t centerline_x_100, int32_t x_position_100) {
            //if x_position_100 is greater than centerline, is positive. Else, is negative
            int32_t x_centerline_delta_100 = x_position_100 - centerline_x_100;
            x_centerline_delta_100*=-1; //flip x_centerline_delta
            return centerline_x_100+x_centerline_delta_100;
        }

        public: 

            //mirror entire path over centerline
            void mirror() {
                //find centerline from oob
                //x centerline is the average of the out-of-bounds x min and max positions 
                int32_t centerline_x_100 = (oob_x_max_100-oob_x_min_100)/2;

                for(int i=0;i<this->path_segments.size();i++) {
                    switch(path_segments[i].type) {
                        case (PathSegmentType::FLY_THRU):
                        case (PathSegmentType::FLY_TO):
                        case (PathSegmentType::FLY_TO_MOVING_TARGET):
                        {
                            this->path_segments[i].target_pos_x_100=
                            mirror_x_over_centerline(centerline_x_100, 
                                this->path_segments[i].target_pos_x_100);
                        }
                        break;
                        case (PathSegmentType::TURN_LEFT_AUTO):
                        {
                            this->path_segments[i].type=PathSegmentType::TURN_RIGHT_AUTO;
                        }
                        break;
                        case (PathSegmentType::TURN_RIGHT_AUTO):
                        {
                            this->path_segments[i].type=PathSegmentType::TURN_LEFT_AUTO;
                        }
                        break;
                        case (PathSegmentType::TURN_LEFT_FIXED):
                        {
                            this->path_segments[i].type=PathSegmentType::TURN_RIGHT_FIXED;
                            this->path_segments[i].turn_degrees_100*=-1;
                        }
                        break;
                        case (PathSegmentType::TURN_RIGHT_FIXED):
                        {
                            this->path_segments[i].type=PathSegmentType::TURN_LEFT_FIXED;
                            this->path_segments[i].turn_degrees_100*=-1;
                        }
                        break;

                        default:
                        {
                            //nop (ROTATE_TO, )
                        }
                        break;
                    }
                }
            }

            //construct path taking into account the unit that wants to gen the path
            Path(Type type=Type::BEE, int32_t target_position_x_100=6400, int32_t target_position_y_100=12800, int32_t unit_position_x_100=6400, int32_t unit_position_y_100=0)
            {
                switch(type) {
                    case Type::BEE:
                    {
                        this->path_segments.push_back(PathSegment()); //here's a default
                    }
                    break;
                    case Type::BUTTERFLY:
                    {

                    }
                    break;
                    case Type::GALAGA_LIGHT: //apparently the pathfinding doesn't change based on Galaga bosu damage state
                    case Type::GALAGA_DARK:
                    {

                    }
                }
            }
            bool follow(int32_t * position_x_100, int32_t * position_y_100, int32_t * orientation_degrees_100) {
                //*position_x_100 = 
                if(path_segments.size()<1) {return false; }//can't follow an empty path

                int32_t deg_mag_turn = -1; //default to turns subtracting degrees (Right turn)
                switch (this->path_segments.back().type) {

                    case PathSegmentType::FLY_STRAIGHT:
                    {
                        fly_straight(position_x_100,position_y_100,orientation_degrees_100);
                        this->path_segments.back().distance_travelled_100 += (((float)this->path_segments.back().speed_100))*(((float)speed_multiplier_100)/100);
                        if(this->path_segments.back().distance_travelled_100>this->path_segments.back().target_distance_100) { path_segments.pop_back(); } //we've hit our target, drop the segment 
                    }
                    break;
                    case PathSegmentType::FLY_THRU:
                    {
                        //point unit at target position if needed
                        test_set_orientation_to_point(position_x_100, position_y_100, orientation_degrees_100);

                        //fly towards/thru target position
                        fly_straight(position_x_100,position_y_100,orientation_degrees_100); //our orientation is set, just keep flyin'

                        if(is_out_of_bounds(*position_x_100,*position_y_100)) {
                            this->path_segments.pop_back(); //drop segment, we don't need to keep flyin oob
                        }
                    }
                    break;
                    case PathSegmentType::FLY_TO:
                    {
                        //point unit at target position if needed
                        test_set_orientation_to_point(position_x_100, position_y_100, orientation_degrees_100);

                        //fly towards/thru target position
                        bool finished_flying = fly_straight(position_x_100,position_y_100,orientation_degrees_100,false); //set to false so we stop

                        if (finished_flying) {this->path_segments.pop_back();} //drop segment, we've reached our FLY_TO target pos
                    }
                    break;
                    case PathSegmentType::ROTATE_TO:
                    {
                        //difference between where we are and where we need to rotate to. Takes shortest path to target orientation.
                        int32_t rot_delta = (this->path_segments.back().orientation_target_degrees_100-*orientation_degrees_100+54000)%36000 - 18000;
                        int32_t sign = 1;
                        int32_t mag = abs(rot_delta);
                        if(rot_delta<0) { sign=-1; }

                        if(mag>this->path_segments.back().orientation_rotate_speed_100){ //we have more than 1 tick of rotation to go
                            mag = this->path_segments.back().orientation_rotate_speed_100; //limit our rotation to the max
                        } else { //this our last tick of rotation
                            this->path_segments.pop_back(); // drop segment, we're done after this
                        }

                        rot_delta = sign*mag;
                        
                        *orientation_degrees_100+=rot_delta;

                    }
                    break;
                    case PathSegmentType::TURN_LEFT_AUTO:  //left turns add degrees
                        deg_mag_turn=1; //override Right-turn default 
                    case PathSegmentType::TURN_RIGHT_AUTO: //right turns subtract degrees
                    {
                        //holy shit! a real use of switch case fallthrough!
                        set_theta_deg_100_from_speed_100(orientation_degrees_100,deg_mag_turn);
                        //int32_t turn_theta_100 = deg_mag_turn*theta_deg_100_from_speed_100(this->path_segments.back().speed_100,this->path_segments.back().turn_radius_100);
                        int32_t actual_turn_value_100 = check_if_alignable(orientation_degrees_100,position_x_100,position_y_100);

                        *orientation_degrees_100+=actual_turn_value_100; //actually turn ship

                        //if we didn't rotate orientation_rotate_speed_100, we have finished turning!
                        if(actual_turn_value_100!=this->path_segments.back().orientation_rotate_speed_100) {
                            this->path_segments.pop_back();
                        }
                    }
                    break;  
                    case PathSegmentType::TURN_LEFT_FIXED:
                        deg_mag_turn=1; //override Right-turn default 

                    case PathSegmentType::TURN_RIGHT_FIXED:
                    {
                        set_theta_deg_100_from_speed_100(orientation_degrees_100,deg_mag_turn);

                        int32_t actual_turn_value_100 = check_for_turn_overshoot(this->path_segments.back().orientation_rotate_speed_100, *orientation_degrees_100, this->path_segments.back().initial_orientation_degrees_100+this->path_segments.back().orientation_target_degrees_100);
                        *orientation_degrees_100+=actual_turn_value_100; //actually turn ship

                        //check if done turnings
                        if(actual_turn_value_100!=this->path_segments.back().orientation_rotate_speed_100) {
                            this->path_segments.pop_back();
                        }
                    }
                    break;
                    default:

                    break;
                }
                return true; //followed path
            }
    };


    class Unit {
        int32_t x_position_100;
        int32_t y_position_100;
        int32_t orientation_degrees_100;

        int32_t collision_radius_100;

        int8_t orientation_code=0; 


        Path activePath;

        Type type = Type::PLAYER;
        BonusType bonus = BonusType::_NONE;

        int32_t euclidian_distance(int32_t dx, int32_t dy) {
            int32_t abs_x = abs(dx);
            int32_t abs_y = abs(dy);
            int32_t max_val = (abs_x > abs_y) ? abs_x : abs_y;
            int32_t min_val = (abs_x > abs_y) ? abs_y : abs_x;
            
            // 0.414 is approximately 424 / 1024
            return max_val + ((424 * min_val) >> 10); 
        }

                //converts between degrees x 100 and orientation value (for sprite mapping)
        // x100 degrees to orientation
        //     9000                 |       0
        // 13500|    4500           | 7            1
        //                          |
        //--18000  0 --             | 6            2
        //                          |
        // 22500|  31500            | 5     4      3
        //     27000                |
        // orientation value brackets:
        // 0 6750->11250      (>6750 AND <11250)
        // 1 2250->6750       (>2250 AND <6750)
        // 2 2250->33750      (>33750 OR <2250)
        // 3 29250->33750     (>29250 AND <33750)
        // 4 24750->29250     (>24750 AND < 29250)
        // 5 20250->24750     (>20250 AND < 24750)
        // 6 15750->20250     (>15750 AND < 20250)
        // 7 11250->15750     (>11250 AND < 15750)


        uint8_t orientation_code_from_orientation_degrees_100(int32_t orientation_degrees_100) {
            orientation_degrees_100 = orientation_degrees_100%36000; //clamp to 0-360 degrees (0-360000 x100 degrees value)

            if(orientation_degrees_100>6750&&orientation_degrees_100<=11250)       {return 4;}//Fine
            else if(orientation_degrees_100>2250&&orientation_degrees_100<=6750)   {return 3;}//5
            else if(orientation_degrees_100>33750||orientation_degrees_100<2250)   {return 2;}//fine
            else if(orientation_degrees_100>29250&&orientation_degrees_100<=33750) {return 5;}//7
            else if(orientation_degrees_100>24750&&orientation_degrees_100<=29250) {return 0;}//Fine
            else if(orientation_degrees_100>20250&&orientation_degrees_100<=24750) {return 7;}//1
            else if(orientation_degrees_100>15750&&orientation_degrees_100<=20250) {return 6;}//Fine
            else if(orientation_degrees_100>11250&&orientation_degrees_100<=15750) {return 1;}//3
            //if(orientation_degrees_100>6750&&orientation_degrees_100<=11250)       {return 0;}
            //else if(orientation_degrees_100>2250&&orientation_degrees_100<=6750)   {return 1;}
            //else if(orientation_degrees_100>33750||orientation_degrees_100<2250)   {return 2;}
            //else if(orientation_degrees_100>29250&&orientation_degrees_100<=33750) {return 3;}
            //else if(orientation_degrees_100>24750&&orientation_degrees_100<=29250) {return 4;}
            //else if(orientation_degrees_100>20250&&orientation_degrees_100<=24750) {return 5;}
            //else if(orientation_degrees_100>15750&&orientation_degrees_100<=20250) {return 6;}
            //else if(orientation_degrees_100>11250&&orientation_degrees_100<=15750) {return 7;}

            return 0;
        }



        public:
            Unit(int x_center_100=6400, int y_center_100=6400,int coll_radius_100=400) {
                this->x_position_100=x_center_100;
                this->y_position_100=y_center_100;
                this->collision_radius_100=coll_radius_100;
            }

            void setOrientation(uint8_t orientation) {
                this->orientation_code = orientation % 8;
            }

            uint8_t getOrientation() {
                return this->orientation_code;
            }

            bool testCollision(int x_other_100, int y_other_100, int coll_radius_other) {
                
                int32_t dx = abs(this->x_position_100-x_other_100);
                int32_t dy = abs(this->y_position_100-y_other_100); 
                int32_t distance = euclidian_distance(dx,dy);
                if (distance<=(coll_radius_other+this->collision_radius_100)) {
                    return true;
                }
                
                return false;
            }

            //WORKING: BE CAREFUL IT'S FUCKY
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
               int x=0-orientation_code_from_orientation_degrees_100(this->orientation_degrees_100)*9+(this->x_position_100-450)/100;
               int y=(this->y_position_100-450)/100;
               int b=0;

               //istart = 0, iend = 9 for first orientation

               //correct for orientationCode 0
               //int16_t istart=0;
               //int16_t iend=9;

                int16_t istart=0;
                int16_t iend=72;

                //working config for orient=1
                //int16_t imin = 8;
                //int16_t imax = 18;

                //working config for orient=2
                //int16_t imin = 17;
                //int16_t imax = 27;

                //working config for orient=3
                //int16_t imin = 26;
                //int16_t imax = 36;
                int16_t imin = orientation_code_from_orientation_degrees_100(this->orientation_degrees_100)*9-1;
                int16_t imax = (orientation_code_from_orientation_degrees_100(this->orientation_degrees_100)+1)*9;
                int16_t typecode = static_cast<int8_t>(type)+1;
               d->startWrite(); //render entire spritemap
                for (int16_t j = typecode*9; j < typecode*9+9; j++, y++) {
                    //for (int16_t i = orientation_code_from_orientation_degrees_100(this->orientation_degrees_100)*9-1; i < orientation_code_from_orientation_degrees_100(this->orientation_degrees_100)*9+9; i++) {
                    for (int16_t i = istart; i < iend; i++) {

                    if (i & 7)
                        b <<= 1;
                    else
                        b = galaga_sprite_atlas[j * byteWidth + i / 8];
                        if(i>imin&&i<imax)
                        {
                            //d->writePixel(x + 27 + i, y, (b & 0x80) ? 1 : 0);
                            d->writePixel(x + i, y, (b & 0x80) ? 1 : 0);
                        }

                    }
                }
                d->endWrite();
                this->orientation_degrees_100+=500;
                this->orientation_degrees_100=this->orientation_degrees_100%36000;

            }

            void _update() {
                if(this->activePath.follow(&this->x_position_100,&this->y_position_100,&this->orientation_degrees_100)) {

                } else {
                    //oops, can't follow
                }
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

        std::vector<LevelTickType> getTickListForStage(uint8_t stage) {
            uint8_t num_20_ticks = stage/20;
            uint8_t remainder_20 = stage%20;

            uint8_t num_10_ticks = remainder_20/10;
            uint8_t remainder_10 = remainder_20%10;

            uint8_t num_5_ticks = remainder_10/5;
            uint8_t remainder_5 = remainder_10%5;

            uint8_t num_1_ticks = remainder_5;

            std::vector<LevelTickType> return_tick_list = {};

            for(int i=0;i<num_1_ticks;i++) { return_tick_list.push_back(LevelTickType::_1X);}
            for(int i=0;i<num_5_ticks;i++) { return_tick_list.push_back(LevelTickType::_5X);}
            for(int i=0;i<num_10_ticks;i++) { return_tick_list.push_back(LevelTickType::_10X);}
            for(int i=0;i<num_20_ticks;i++) { return_tick_list.push_back(LevelTickType::_20X);}     

            return return_tick_list;                   
        }


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
            player._update();
        }

        void _update()
        {
           
        }

        //debug draw sprite atlas
        void _draw_sprite_atlas(DrawContext &ctx) {
            int8_t i, j, byteWidth = (sprite_atlas_w + 7) / 8;
            auto *d = ctx.display;
            d->fillScreen(BLACK);
            int x=0;
            int y=0;
            int b=0;
            d->startWrite(); //render entire spritemap
            for (int16_t j = 0; j < sprite_atlas_h; j++, y++) {
                for (int16_t i = 0; i < sprite_atlas_w; i++) {
                if (i & 7)
                    b <<= 1;
                else
                    b = galaga_sprite_atlas[j * byteWidth + i / 8];
                    d->writePixel(x + 27 + i, y, (b & 0x80) ? 1 : 0);
                }
            }
            d->endWrite();
        }

        void _draw(DrawContext &ctx)
        {
            auto *d = ctx.display;
            d->fillScreen(BLACK);

            // --- Status bar ---
            d->setTextSize(1);
            d->setTextColor(DrawCommand::DrawColorPrimary());
            //d->setCursor(2, 1);
            //d->print("SC:");
            //d->print(_score);
            player._draw(ctx);
            //_draw_sprite_atlas(ctx);
            //_draw_sprite_atlas(ctx);
            // Lives as small filled squares on the right
            //for (int i = 0; i < _lives && i < 5; ++i)
            //    d->fillRect(W - 4 - i * 5, 2, 3, 3, DrawCommand::DrawColorPrimary());

           // d->drawFastHLine(0, STATUS_H - 1, W, DrawCommand::DrawColorPrimary());

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
