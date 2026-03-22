#pragma once

/* 
* Copyright (c) 2023 SCANLAB GmbH
*
* RTC6 Remote Interface Wrapper
*
* This wrapper is supposed to make the remote interface functionality more easy to use.
* The wrapper provides an interface similar to the original RTC6 header files (RTC6impl.h/hpp).
*
* Building of the remote interface telegram packages as well as sending and receiving them is
* handled by the wrapper. For this to work, the user has to provide a platform specific
* implementation for some socket functions.
*
* One thing this wrapper does not do, is sending multiple consecutive list commands in one
* telegram. It will send one telegram per list command.
*
* How to use:
* 1. Create a derived class from NetworkAdapter
* 2. Create a datagram (UDP) socket with the IP address of desired RTC and port 63750
* 3. Implement platform specific socket functionality in the virtual functions
* - send
* - recv
* - select
* 4. Create an RTC object and pass the NetworkAdapter implementation
*/

#include "telegrams.h"
#include <vector>
#include <array>
#include <tuple>
#include <string>
#include <cstring>
#include <limits>
#include <memory>


using namespace remote_interface;


namespace rtc6_rif
{
static constexpr int32_t NETWORK_ERR_TIMEOUT = INT32_MIN;
static constexpr int32_t NETWORK_ERR_WRONG_ID = INT32_MIN + 1;

class NetworkAdapter
{
public:
    virtual ~NetworkAdapter() = default;

    // Send specified amount of bytes. Return 0 if successful, a negative error code otherwise.
    virtual int32_t send(const char* buf, size_t len) = 0;
    // Receive up to len bytes and write them into buf. Return 0 if successful, a negative error code otherwise.
    // If the received telegram should be discarded return a positive value.
    virtual int32_t recv(char* buf, size_t len) = 0;
    // Determine the read status. Return the number of read operations available.
    // Return 0 if no data is available after timeout_us microseconds, a negative error code otherwise.
    virtual int32_t select(long timeout_us) = 0;
};

class RTC
{
public:
    RTC(std::unique_ptr<NetworkAdapter> adapter);

    uint32_t get_last_error() const;
    uint32_t get_error() const;
    void reset_error(uint32_t Code);

    // Returns a network error specific to this wrapper.
    // Generally the last error code returned from user functions send/recv/select.
    // Special case: NETWORK_ERR_TIMEOUT if a timeout occured (exec_cmd/send_recv)
    // Special case: NETWORK_ERR_WRONG_ID if answer ID doesn't match original telegram ID (exec_cmd)
    int32_t get_network_error() const;

    void eth_set_com_timeouts_auto(const double InitialTimeout, const double MaxTimeout, const double Multiplier);

    // Returns a Remote Interface specific error (call if get_error Bit #31 is set)
    uint32_t get_remote_error();

    // Sends a special telegram to the RTC which disables remote_interface until next power cycle
    bool disable_remote_interface();

    // Control commands
    void config_list(const uint32_t Mem1, const uint32_t Mem2);
    void get_config_list();
    void set_start_list_pos(const uint32_t ListNo, const uint32_t Pos);
    void set_start_list(const uint32_t ListNo);
    void set_start_list_1();
    void set_start_list_2();
    uint32_t get_input_pointer();
    uint32_t get_list_space();
    uint32_t load_list(const uint32_t ListNo, const uint32_t Pos);
    void load_sub(const uint32_t Index);
    void load_char(const uint32_t Char);
    void load_text_table(const uint32_t Index);
    void set_extstartpos(const uint32_t Pos);
    void set_max_counts(const uint32_t Counts);
    uint32_t get_counts();
    void store_timestamp_counter();
    void simulate_ext_start_ctrl();
    void execute_list_pos(const uint32_t ListNo, const uint32_t Pos);
    void execute_list(const uint32_t ListNo);
    void execute_list_1();
    void execute_list_2();
    void stop_execution();
    void simulate_ext_stop();
    void set_extern_start_delay(const int32_t Delay, const uint32_t EncoderNo);
    void auto_change_pos(const uint32_t Pos);
    void auto_change();
    void start_loop();
    void quit_loop();
    void pause_list();
    void restart_list();
    void release_wait();
    uint32_t get_wait_status();
    void set_pause_list_cond(const uint32_t Mask1, const uint32_t Mask0);
    void set_pause_list_not_cond(const uint32_t Mask1, const uint32_t Mask0);
    double get_time();
    double get_lap_time();
    void set_control_mode(const uint32_t Mode);
    uint32_t read_status();
    void get_status(uint32_t& Status, uint32_t& Pos);
    uint32_t get_startstop_info();
    uint32_t get_overrun();
    void control_command(const uint32_t Head, const uint32_t Axis, const uint32_t Data);
    int32_t get_value(const uint32_t Signal);
    void get_values(const uintptr_t SignalPtr, const uintptr_t ResultPtr);
    uint32_t get_head_status(const uint32_t HeadNo);
    double get_head_para(const uint32_t HeadNo, const uint32_t ParaNo);
    double get_table_para(const uint32_t TableNo, const uint32_t ParaNo);
    int32_t get_z_distance(const int32_t X, const int32_t Y, const int32_t Z);
    void get_galvo_controls(const uintptr_t InPtr, const uintptr_t OutPtr);
    void set_laser_control(const uint32_t Ctrl);
    void set_laser_mode(const uint32_t Mode);
    void enable_laser();
    void disable_laser();
    void laser_signal_on();
    void laser_signal_off();
    void set_qswitch_delay(const uint32_t Delay);
    void set_laser_pulses_ctrl(const uint32_t HalfPeriod, const uint32_t PulseLength);
    void set_firstpulse_killer(const uint32_t Length);
    void set_laser_pin_out(const uint32_t Pins);
    uint32_t get_laser_pin_in();
    void set_standby(const uint32_t HalfPeriod, const uint32_t PulseLength);
    void get_standby(uint32_t& HalfPeriod, uint32_t& PulseLength);
    void set_pulse_picking(const uint32_t No);
    void set_pulse_picking_length(const uint32_t Length);
    void config_laser_signals(const uint32_t Config);
    uint32_t set_auto_laser_control(const uint32_t Ctrl, const uint32_t Value, const uint32_t Mode, const uint32_t MinValue, const uint32_t MaxValue);
    uint32_t set_auto_laser_params(const uint32_t Ctrl, const uint32_t Value, const uint32_t MinValue, const uint32_t MaxValue);
    void spot_distance_ctrl(const double Dist);
    void set_encoder_speed_ctrl(const uint32_t EncoderNo, const double Speed, const double Smooth);
    void set_default_pixel(const uint32_t PulseLength);
    void set_port_default(const uint32_t Port, const uint32_t Value);
    void write_8bit_port(const uint32_t Value);
    void write_io_port_mask(const uint32_t Value, const uint32_t Mask);
    void write_io_port(const uint32_t Value);
    uint32_t get_io_status();
    uint32_t read_io_port();
    uint32_t read_io_port_buffer(const uint32_t Index, uint32_t& Value, int32_t& XPos, int32_t& YPos, uint32_t& Time);
    void write_da_x(const uint32_t x, const uint32_t Value);
    void write_da_1(const uint32_t Value);
    void write_da_2(const uint32_t Value);
    uint32_t read_analog_in();
    void set_jump_speed_ctrl(const double Speed);
    void set_mark_speed_ctrl(const double Speed);
    void set_defocus(const int32_t Shift);
    void set_defocus_offset(const int32_t Shift);
    void set_delay_mode(const uint32_t VarPoy, const uint32_t DirectMode3D, const uint32_t EdgeLevel, const uint32_t MinJumpDelay, const uint32_t JumpLengthLimit);
    void goto_xyz(const int32_t X, const int32_t Y, const int32_t Z);
    void goto_xy(const int32_t X, const int32_t Y);
    void set_offset_xyz(const uint32_t HeadNo, const int32_t X, const int32_t Y, const int32_t Z, const uint32_t at_once);
    void set_offset(const uint32_t HeadNo, const int32_t X, const int32_t Y, const uint32_t at_once);
    void home_position_xyz(const int32_t XHome, const int32_t YHome, const int32_t ZHome);
    void home_position(const int32_t XHome, const int32_t YHome);
    void time_update();
    void time_control_eth(const double PPM);
    void set_serial_step(const uint32_t No, const uint32_t Step);
    void select_serial_set(const uint32_t No);
    double get_serial();
    double get_list_serial(uint32_t& Set);
    void set_zoom(const uint32_t Zoom);
    void set_matrix(const uint32_t HeadNo, const double M11, const double M12, const double M21, const double M22, const uint32_t at_once);
    void set_angle(const uint32_t HeadNo, const double Angle, const uint32_t at_once);
    void set_scale(const uint32_t HeadNo, const double Scale, const uint32_t at_once);
    void simulate_encoder(const uint32_t EncoderNo);
    void read_encoder(int32_t& Encoder0_0, int32_t& Encoder1_0, int32_t& Encoder0_1, int32_t& Encoder1_1);
    void get_encoder(int32_t& Encoder0, int32_t& Encoder1);
    void init_fly_2d(const int32_t OffsetX, const int32_t OffsetY, const uint32_t No);
    void get_fly_2d_offset(int32_t& OffsetX, int32_t& OffsetY);
    uint32_t get_marking_info();
    void set_rot_center(const int32_t X, const int32_t Y);
    uint32_t set_mcbsp_freq(const uint32_t Freq);
    void mcbsp_init(const uint32_t XDelay, const uint32_t RDelay);
    void set_mcbsp_x(const double Scale);
    void set_mcbsp_y(const double Scale);
    void set_mcbsp_rot(const double Resolution);
    void set_mcbsp_matrix();
    void set_mcbsp_global_x(const double Scale);
    void set_mcbsp_global_y(const double Scale);
    void set_mcbsp_global_rot(const double Resolution);
    void set_mcbsp_global_matrix();
    void set_mcbsp_in(const uint32_t Mode, const double Scale);
    void set_multi_mcbsp_in(const uint32_t Ctrl, const uint32_t P, const uint32_t Mode);
    uint32_t read_mcbsp(const uint32_t No);
    uint32_t read_multi_mcbsp(const uint32_t No);
    void apply_mcbsp(const uint32_t HeadNo, const uint32_t at_once);
    uint32_t get_scanahead_params(const uint32_t HeadNo, uint32_t& Preview, uint32_t& Vmax, double& Amax);
    uint32_t activate_scanahead_autodelays(const int32_t Mode);
    void set_scanahead_laser_shifts(const int32_t dLasOn, const int32_t dLasOff);
    void set_scanahead_line_params_ex(const uint32_t CornerScale, const uint32_t EndScale, const uint32_t AccScale, const uint32_t JumpScale);
    void set_scanahead_line_params(const uint32_t CornerScale, const uint32_t EndScale, const uint32_t AccScale);
    uint32_t set_scanahead_params(const uint32_t Mode, const uint32_t HeadNo, const uint32_t TableNo, const uint32_t Preview, const uint32_t Vmax, const double Amax);
    void set_scanahead_speed_control(const uint32_t Mode);
    void stepper_init(const uint32_t No, const uint32_t Period, const int32_t Dir, const int32_t Pos, const uint32_t Tol, const uint32_t Enable, const uint32_t WaitTime);
    void stepper_enable(const int32_t Enable1, const int32_t Enable2);
    void stepper_disable_switch(const int32_t Disable1, const int32_t Disable2);
    void stepper_control(const int32_t Period1, const int32_t Period2);
    void stepper_abs_no(const uint32_t No, const int32_t Pos, const uint32_t WaitTime);
    void stepper_rel_no(const uint32_t No, const int32_t dPos, const uint32_t WaitTime);
    void stepper_abs(const int32_t Pos1, const int32_t Pos2, const uint32_t WaitTime);
    void stepper_rel(const int32_t dPos1, const int32_t dPos2, const uint32_t WaitTime);
    void get_stepper_status(uint32_t& Status1, int32_t& Pos1, uint32_t& Status2, int32_t& Pos2);
    void set_sky_writing_para(const double Timelag, const int32_t LaserOnShift, const uint32_t Nprev, const uint32_t Npost);
    void set_sky_writing(const double Timelag, const int32_t LaserOnShift);
    void set_sky_writing_limit(const double Limit);
    void set_sky_writing_mode(const uint32_t Mode);
    uint32_t uart_config(const uint32_t BaudRate);
    void rs232_write_data(const uint32_t Data);
    void rs232_write_text(const char* Text);
    uint32_t rs232_read_data();
    void bounce_supp(const uint32_t Length);
    void set_hi(const uint32_t HeadNo, const double GalvoGainX, const double GalvoGainY, const int32_t GalvoOffsetX, const int32_t GalvoOffsetY);
    void set_jump_mode(const int32_t Flag, const uint32_t Length, const int32_t VA1, const int32_t VA2, const int32_t VB1, const int32_t VB2,
                       const int32_t JA1, const int32_t JA2, const int32_t JB1, const int32_t JB2);
    void select_cor_table(const uint32_t HeadA, const uint32_t HeadB);
    void set_free_variable(const uint32_t VarNo, const uint32_t Value);
    uint32_t get_free_variable(const uint32_t VarNo);
    void set_mcbsp_out_ptr(const uint32_t Num, const uintptr_t SignalPtr);
    void periodic_toggle(const uint32_t Port, const uint32_t Mask, const uint32_t P1, const uint32_t P2, const uint32_t Count, const uint32_t Start);
    void set_enduring_wobbel_1(const uint32_t CenterX, const uint32_t CenterY, const uint32_t CenterZ, const uint32_t LimitHi, const uint32_t LimitLo,
                               const double ScaleX, const double ScaleY, const double ScaleZ);
    void set_enduring_wobbel_2(const uint32_t CenterX, const uint32_t CenterY, const uint32_t CenterZ, const uint32_t LimitHi, const uint32_t LimitLo,
                               const double ScaleX, const double ScaleY, const double ScaleZ);
    void stop_trigger();
    void measurement_status(uint32_t& Busy, uint32_t& Pos);
    void get_waveform_offset(const uint32_t Channel, const uint32_t Offset, const uint32_t Number, const uintptr_t Ptr);
    void get_waveform(const uint32_t Channel, const uint32_t Number, const uintptr_t Ptr);
    uint32_t set_verify(const uint32_t Verify);
    uint32_t get_hex_version();
    uint32_t get_rtc_version();
    uint32_t get_bios_version();
    uint32_t get_serial_number();
    void set_input_pointer(const uint32_t Pos);
    void get_list_pointer(uint32_t& ListNo, uint32_t& Pos);
    void execute_at_pointer(const uint32_t Pos);
    void set_laser_pulse_sync(const uint32_t Mode, const uint32_t Delay);
    uint64_t get_timestamp_long();
    void clear_fly_overflow_ctrl(const uint32_t Mode);
    double get_temperature();
    void set_mcbsp_out_oie_ctrl(const uint32_t Signal1, const uint32_t Signal2);
    uint32_t load_correction_file(const char* Name, const uint32_t No, const uint32_t Dim);
    void number_of_correction_tables(const uint32_t Number);
    uint32_t load_z_table_no(const double A, const double B, const double C, const uint32_t No);
    uint32_t load_z_table(const double A, const double B, const double C);
    uint32_t load_z_table_no_20b(const double A, const double B, const double C, const uint32_t No);
    uint32_t load_z_table_20b(const double A, const double B, const double C);
    uint32_t load_oct_table_no(const double A, const double B, const uint32_t No);
    void set_timelag_compensation(const uint32_t HeadNo, const uint32_t TimelagXY, const uint32_t TimelagZ);
    void set_encoder_filter_ctrl(const uint32_t Length, const uint32_t Mode);
    uint32_t get_master_slave();
    uint32_t get_sync_status();
    void master_slave_config(const uint32_t Config);
    void eth_get_static_ip(uint32_t& Ip, uint32_t& Netmask, uint32_t& Gateway);
    uint32_t eth_set_static_ip(const uint32_t Ip, const uint32_t Netmask, const uint32_t Gateway);
    void eth_get_port_numbers(uint32_t& UDPsearch, uint32_t& UDPexcl, uint32_t& TCP);
    uint32_t eth_set_port_numbers(const uint32_t UDPsearch, const uint32_t UDPexcl, const uint32_t TCP);
    void eth_configure_link_loss(const uint32_t Mode);
    void set_sky_writing_min_speed_ctrl(const double Speed);
    void set_controlpreview_compensation_ctrl(const uint32_t ControlPreview, const uint32_t Mode);
    void set_fly_tracking_error(const uint32_t TrackingErrorX, const uint32_t TrackingErrorY);
    void set_laser_delays_ctrl(const int32_t LaserOnDelay, const uint32_t LaserOffDelay);
    void set_scanner_delays_ctrl(const uint32_t Jump, const uint32_t Mark, const uint32_t Polygon);
    void set_sky_writing_scale_ctrl(const double Scale);
    uint32_t get_extended_head_status_value_ctrl(const uint32_t Head, const uint32_t Axis, const uint32_t Value);
    void get_extended_head_status_block_ctrl(const uint32_t Head, const uint32_t Axis, const uintptr_t Ptr);
    uint32_t get_rcm_value_ctrl(const uint32_t Head, const uint32_t Axis, const uint32_t Value);
    uint32_t demux_rcm_ctrl(const uintptr_t Input, const uint32_t Input_length, const uint32_t N_signals, const uint32_t Index, const uint32_t Flags, uintptr_t Output);
    void master_slave_set_offset_ctrl(const uint32_t Mode, const uint32_t Ctrl);
    void marking_info_ack_ctrl(const uint32_t AckMask);
    void set_sl2_100_error_tolerance_ctrl(const uint32_t Tol_us);
    uint32_t get_sl2_100_error_tolerance_ctrl();
    void set_sl2_100_error_tolerance_enable_ctrl(const bool Enable);
    bool get_sl2_100_error_tolerance_enable_ctrl();
    void set_explicit_ack_mode_ctrl(const uint32_t Mode);
    void startstop_info_ack_ctrl(const uint32_t AckMask);

    // List commands
    void list_nop();
    void list_continue();
    void list_next();
    void set_end_of_list();
    void list_return();
    void list_repeat();
    void list_until(const uint32_t Number);
    void list_jump_pos(const uint32_t Pos);
    void list_jump_rel(const int32_t Pos);
    void set_wait(const uint32_t WaitWord);
    void long_delay(const uint32_t Delay);
    void set_extstartpos_list(const uint32_t Pos);
    void simulate_ext_start(const int32_t Delay, const uint32_t EncoderNo);
    void list_call_abs_repeat(const uint32_t Pos, const uint32_t Number);
    void list_call_abs(const uint32_t Pos);
    void list_call_repeat(const uint32_t Pos, const uint32_t Number);
    void list_call(const uint32_t Pos);
    void sub_call_abs_repeat(const uint32_t Index, const uint32_t Number);
    void sub_call_abs(const uint32_t Index);
    void sub_call_repeat(const uint32_t Index, const uint32_t Number);
    void sub_call(const uint32_t Index);
    void if_cond(const uint32_t Mask1, const uint32_t Mask0);
    void if_not_cond(const uint32_t Mask1, const uint32_t Mask0);
    void if_pin_cond(const uint32_t Mask1, const uint32_t Mask0);
    void if_not_pin_cond(const uint32_t Mask1, const uint32_t Mask0);
    void clear_io_cond_list(const uint32_t Mask1, const uint32_t Mask0, const uint32_t MaskClear);
    void set_io_cond_list(const uint32_t Mask1, const uint32_t Mask0, const uint32_t MaskSet);
    void switch_io_port(const uint32_t MaskBits, const uint32_t ShiftBits);
    void list_call_abs_cond(const uint32_t Mask1, const uint32_t Mask0, const uint32_t Pos);
    void list_call_cond(const uint32_t Mask1, const uint32_t Mask0, const uint32_t Pos);
    void sub_call_abs_cond(const uint32_t Mask1, const uint32_t Mask0, const uint32_t Index);
    void sub_call_cond(const uint32_t Mask1, const uint32_t Mask0, const uint32_t Index);
    void list_jump_pos_cond(const uint32_t Mask1, const uint32_t Mask0, const uint32_t Pos);
    void list_jump_rel_cond(const uint32_t Mask1, const uint32_t Mask0, const int32_t Pos);
    void config_laser_signals_list(const uint32_t Config);
    void laser_signal_on_list();
    void laser_signal_off_list();
    void set_laser_pulses(const uint32_t HalfPeriod, const uint32_t PulseLength);
    void set_firstpulse_killer_list(const uint32_t Length);
    void set_qswitch_delay_list(const uint32_t Delay);
    void set_laser_pin_out_list(const uint32_t Pins);
    void set_pulse_picking_list(const uint32_t No);
    void set_standby_list(const uint32_t HalfPeriod, const uint32_t PulseLength);
    void set_auto_laser_params_list(const uint32_t Ctrl, const uint32_t Value, const uint32_t MinValue, const uint32_t MaxValue);
    void set_laser_power(const uint32_t Port, const uint32_t Power);
    void spot_distance(const double Dist);
    void set_encoder_speed(const uint32_t Encoder, const double Speed, const double Smooth);
    void set_default_pixel_list(const uint32_t PulseLength);
    void set_port_default_list(const uint32_t Port, const uint32_t Value);
    void set_pixel_line_3d(const uint32_t Channel, const uint32_t HalfPeriod, const double dX, const double dY, const double dZ);
    void set_pixel_line(const uint32_t Channel, const uint32_t HalfPeriod, const double dX, const double dY);
    void set_n_pixel(const uint32_t PortOutValue1, const uint32_t PortOutValue2, const uint32_t Number);
    void set_pixel(const uint32_t PortOutValue1, const uint32_t PortOutValue2);
    void write_8bit_port_list(const uint32_t Value);
    void write_io_port_mask_list(const uint32_t Value, const uint32_t Mask);
    void write_io_port_list(const uint32_t Value);
    void read_io_port_list();
    void write_da_x_list(const uint32_t x, const uint32_t Value);
    void write_da_1_list(const uint32_t Value);
    void write_da_2_list(const uint32_t Value);
    void set_mark_speed(const double Speed);
    void set_jump_speed(const double Speed);
    void set_scanner_delays(const uint32_t Jump, const uint32_t Mark, const uint32_t Polygon);
    void set_laser_delays(const int32_t LaserOnDelay, const uint32_t LaserOffDelay);
    void set_defocus_list(const int32_t Shift);
    void set_defocus_offset_list(const int32_t Shift);
    void set_zoom_list(const uint32_t Zoom);
    void set_offset_xyz_list(const uint32_t HeadNo, const int32_t XOffset, const int32_t YOffset, const int32_t ZOffset, const uint32_t at_once);
    void set_offset_list(const uint32_t HeadNo, const int32_t XOffset, const int32_t YOffset, const uint32_t at_once);
    void set_matrix_list(const uint32_t HeadNo, const uint32_t Ind1, const uint32_t Ind2, const double Mij, const uint32_t at_once);
    void set_angle_list(const uint32_t HeadNo, const double Angle, const uint32_t at_once);
    void set_scale_list(const uint32_t HeadNo, const double Scale, const uint32_t at_once);
    void timed_mark_abs_3d(const int32_t X, const int32_t Y, const int32_t Z, const double T);
    void timed_mark_abs(const int32_t X, const int32_t Y, const double T);
    void mark_abs_3d(const int32_t X, const int32_t Y, const int32_t Z);
    void mark_abs(const int32_t X, const int32_t Y);
    void timed_mark_rel_3d(const int32_t dX, const int32_t dY, const int32_t dZ, const double T);
    void timed_mark_rel(const int32_t dX, const int32_t dY, const double T);
    void mark_rel_3d(const int32_t dX, const int32_t dY, const int32_t dZ);
    void mark_rel(const int32_t dX, const int32_t dY);
    void timed_arc_abs(const int32_t X, const int32_t Y, const double Angle, const double T);
    void arc_abs(const int32_t X, const int32_t Y, const double Angle);
    void timed_arc_rel(const int32_t dX, const int32_t dY, const double Angle, const double T);
    void arc_rel(const int32_t dX, const int32_t dY, const double Angle);
    void arc_abs_3d(const int32_t X, const int32_t Y, const int32_t Z, const double Angle);
    void arc_rel_3d(const int32_t dX, const int32_t dY, const int32_t dZ, const double Angle);
    void set_ellipse(const uint32_t A, const uint32_t B, const double Phi0, const double Phi);
    void mark_ellipse_abs(const int32_t X, const int32_t Y, const double Alpha);
    void mark_ellipse_rel(const int32_t dX, const int32_t dY, const double Alpha);
    void jump_abs(const int32_t X, const int32_t Y);
    void jump_rel(const int32_t dX, const int32_t dY);
    void timed_jump_abs_3d(const int32_t X, const int32_t Y, const int32_t Z, const double T);
    void timed_jump_abs(const int32_t X, const int32_t Y, const double T);
    void jump_abs_3d(const int32_t X, const int32_t Y, const int32_t Z);
    void timed_jump_rel_3d(const int32_t dX, const int32_t dY, const int32_t dZ, const double T);
    void timed_jump_rel(const int32_t dX, const int32_t dY, const double T);
    void jump_rel_3d(const int32_t dX, const int32_t dY, const int32_t dZ);
    void jump_abs_drill(const int32_t X, const int32_t Y, const uint32_t DrillTime);
    void jump_abs_drill_2(const int32_t X, const int32_t Y, const uint32_t DrillTime, const int32_t XOff, const int32_t YOff);
    void jump_rel_drill(const int32_t dX, const int32_t dY, const uint32_t DrillTime);
    void jump_rel_drill_2(const int32_t dX, const int32_t dY, const uint32_t DrillTime, const int32_t XOff, const int32_t YOff);
    void set_vector_control(const uint32_t Ctrl, const uint32_t Value);
    void para_laser_on_pulses_list(const uint32_t Period, const uint32_t Pulses, const uint32_t P);
    void laser_on_pulses_list(const uint32_t Period, const uint32_t Pulses);
    void laser_on_list(const uint32_t Period);
    void para_mark_abs_3d(const int32_t X, const int32_t Y, const int32_t Z, const uint32_t P);
    void para_mark_abs(const int32_t X, const int32_t Y, const uint32_t P);
    void para_mark_rel_3d(const int32_t dX, const int32_t dY, const int32_t dZ, const uint32_t P);
    void para_mark_rel(const int32_t dX, const int32_t dY, const uint32_t P);
    void timed_para_mark_abs_3d(const int32_t X, const int32_t Y, const int32_t Z, const uint32_t P, const double T);
    void timed_para_mark_abs(const int32_t X, const int32_t Y, const uint32_t P, const double T);
    void timed_para_mark_rel_3d(const int32_t dX, const int32_t dY, const int32_t dZ, const uint32_t P, const double T);
    void timed_para_mark_rel(const int32_t dX, const int32_t dY, const uint32_t P, const double T);
    void para_jump_abs_3d(const int32_t X, const int32_t Y, const int32_t Z, const uint32_t P);
    void para_jump_abs(const int32_t X, const int32_t Y, const uint32_t P);
    void para_jump_rel_3d(const int32_t dX, const int32_t dY, const int32_t dZ, const uint32_t P);
    void para_jump_rel(const int32_t dX, const int32_t dY, const uint32_t P);
    void timed_para_jump_abs_3d(const int32_t X, const int32_t Y, const int32_t Z, const uint32_t P, const double T);
    void timed_para_jump_abs(const int32_t X, const int32_t Y, const uint32_t P, const double T);
    void timed_para_jump_rel_3d(const int32_t dX, const int32_t dY, const int32_t dZ, const uint32_t P, const double T);
    void timed_para_jump_rel(const int32_t dX, const int32_t dY, const uint32_t P, const double T);
    void micro_vector_abs_3d(const int32_t X, const int32_t Y, const int32_t Z, const int32_t LasOn, const int32_t LasOff);
    void micro_vector_abs(const int32_t X, const int32_t Y, const int32_t LasOn, const int32_t LasOff);
    void micro_vector_rel_3d(const int32_t dX, const int32_t dY, const int32_t dZ, const int32_t LasOn, const int32_t LasOff);
    void micro_vector_rel(const int32_t dX, const int32_t dY, const int32_t LasOn, const int32_t LasOff);
    void mark_char_abs(const uint32_t Char);
    void mark_char(const uint32_t Char);
    void select_char_set(const uint32_t No);
    void mark_text_abs(const char* Text);
    void mark_text(const char* Text);
    void mark_serial_abs(const uint32_t Mode, const uint32_t Digits);
    void mark_serial(const uint32_t Mode, const uint32_t Digits);
    void set_serial_step_list(const uint32_t No, const uint32_t Step);
    void select_serial_set_list(const uint32_t No);
    void time_fix_f_off(const uint32_t FirstDay, const uint32_t Offset);
    void time_fix_f(const uint32_t FirstDay);
    void time_fix();
    void mark_date_abs(const uint32_t Part, const uint32_t Mode);
    void mark_date(const uint32_t Part, const uint32_t Mode);
    void mark_time_abs(const uint32_t Part, const uint32_t Mode);
    void mark_time(const uint32_t Part, const uint32_t Mode);
    void set_wobbel_mode_phase(const uint32_t Transversal, const uint32_t Longitudinal, const double Freq, const int32_t Mode, const double Phase);
    void set_wobbel_mode(const uint32_t Transversal, const uint32_t Longitudinal, const double Freq, const int32_t Mode);
    void set_wobbel(const uint32_t Transversal, const uint32_t Longitudinal, const double Freq);
    void set_wobbel_direction(const int32_t dX, const int32_t dY);
    void set_wobbel_control(const uint32_t Ctrl, const uint32_t Value, const uint32_t MinValue, const uint32_t MaxValue);
    void set_wobbel_vector(const double dTrans, const double dLong, const uint32_t Period, const double dPower);
    void set_wobbel_vector2(const double dTrans, const double dLong, const uint32_t Period, const double dPower, const double dPower2, const uint32_t Ctrl);
    void set_wobbel_offset(const int32_t OffsetTrans, const int32_t OffsetLong);
    void set_sky_writing_para_list(const double Timelag, const int32_t LaserOnShift, const uint32_t Nprev, const uint32_t Npost);
    void set_sky_writing_list(const double Timelag, const int32_t LaserOnShift);
    void set_sky_writing_limit_list(const double Limit);
    void set_sky_writing_mode_list(const uint32_t Mode);
    void set_control_mode_list(const uint32_t Mode);
    void store_encoder(const uint32_t Pos);
    void save_and_restart_timer();
    void store_timestamp_counter_list();
    void wait_for_timestamp_counter_mode(const uint32_t Counter, const uint32_t Mode);
    void wait_for_timestamp_conter(const uint32_t Counter);
    void set_trigger(const uint32_t Period, const uint32_t Signal1, const uint32_t Signal2);
    void set_trigger4(const uint32_t Period, const uint32_t Signal1, const uint32_t Signal2, const uint32_t Signal3, const uint32_t Signal4);
    void set_trigger8(const uint32_t Period, const uint32_t Signal1, const uint32_t Signal2, const uint32_t Signal3, const uint32_t Signal4,
                      const uint32_t Signal5, const uint32_t Signal6, const uint32_t Signal7, const uint32_t Signal8);
    void set_ext_start_delay_list(const int32_t Delay, const uint32_t EncoderNo);
    void set_fly_x(const double Scale);
    void set_fly_y(const double Scale);
    void set_fly_z(const double Scale, const uint32_t No);
    void set_fly_rot(const double Resolution);
    void set_rot_center_list(const int32_t X, const int32_t Y);
    void set_fly_2d(const double ScaleX, const double ScaleY);
    void fly_return_z(const int32_t X, const int32_t Y, const int32_t Z);
    void fly_return(const int32_t X, const int32_t Y);
    void set_fly_limits(const int32_t Xmin, const int32_t Xmax, const int32_t Ymin, const int32_t Ymax);
    void set_fly_limits_z(const int32_t Zmin, const int32_t Zmax);
    void if_fly_x_overflow(const int32_t Mode);
    void if_fly_y_overflow(const int32_t Mode);
    void if_fly_z_overflow(const int32_t Mode);
    void if_not_fly_x_overflow(const int32_t Mode);
    void if_not_fly_y_overflow(const int32_t Mode);
    void if_not_fly_z_overflow(const int32_t Mode);
    void clear_fly_overflow(const uint32_t Mode);
    void park_position(const uint32_t Mode, const int32_t X, const int32_t Y);
    void park_return(const uint32_t Mode, const int32_t X, const int32_t Y);
    void activate_fly_xy_encoder(const double ScaleX, const double ScaleY, const int32_t EncX, const int32_t EncY);
    void activate_fly_xy(const double ScaleX, const double ScaleY);
    void activate_fly_2d_encoder(const double ScaleX, const double ScaleY, const int32_t EncX, const int32_t EncY);
    void activate_fly_2d(const double ScaleX, const double ScaleY);
    void wait_for_encoder_mode(const int32_t Value, const uint32_t Encoder, const uint32_t Mode);
    void wait_for_encoder(const int32_t Value, const uint32_t Encoder);
    void wait_for_encoder_in_range_mode(const int32_t EncXmin, const int32_t EncXmax, const int32_t EncYmin, const int32_t EncYmax, const uint32_t Mode);
    void wait_for_encoder_in_range(const int32_t EncXmin, const int32_t EncXmax, const int32_t EncYmin, const int32_t EncYmax);
    void if_not_activated();
    void range_checking(const uint32_t Head, const uint32_t Mode, const uint32_t Data);
    void set_fly_x_pos(const double Scale);
    void set_fly_y_pos(const double Scale);
    void set_fly_rot_pos(const double Resolution);
    void wait_for_mcbsp(const uint32_t Axis, const int32_t Value, const int32_t Mode);
    void set_mcbsp_in_list(const uint32_t Mode, const double Scale);
    void set_multi_mcbsp_in_list(const uint32_t Ctrl, const uint32_t P, const uint32_t Mode);
    void set_mcbsp_x_list(const double Scale);
    void set_mcbsp_y_list(const double Scale);
    void set_mcbsp_rot_list(const double Resolution);
    void set_mcbsp_matrix_list();
    void apply_mcbsp_list(const uint32_t HeadNo, const uint32_t at_once);
    void set_mcbsp_global_x_list(const double Scale);
    void set_mcbsp_global_y_list(const double Scale);
    void set_mcbsp_global_rot_list(const double Resolution);
    void set_mcbsp_global_matrix_list();
    void set_fly_1_axis(const uint32_t Axis, const uint32_t Mode, const double Scale);
    void fly_return_1_axis(const uint32_t Axis, const int32_t RetPos);
    void wait_for_1_axis(const int32_t Value, const uint32_t Mode, const int32_t WaitMode, const uint32_t LaserMode);
    void activate_fly_1_axis(const uint32_t Axis, const uint32_t Mode, const double Scale, const int32_t Offset);
    void park_position_1_axis(const uint32_t Mode, const uint32_t Axis, const int32_t ParkPos);
    void park_return_1_axis(const uint32_t Mode, const uint32_t Axis, const int32_t RetPos);
    void set_fly_2_axes(const uint32_t Axis1, const uint32_t Mode1, const double Scale1, const uint32_t Axis2, const uint32_t Mode2, const double Scale2);
    void fly_return_2_axes(const uint32_t Axis1, const int32_t RetPos1, const uint32_t Axis2, const int32_t RetPos2);
    void wait_for_2_axes(const uint32_t ModeX, const int32_t MinValueX, const int32_t MaxValueX,
                         const uint32_t ModeY, const int32_t MinValueY, const int32_t MaxValueY, const int32_t WaitMode, const uint32_t LaserMode);
    void activate_fly_2_axes(const uint32_t ModeX, const double ScaleX, const int32_t OffsetX,
                             const uint32_t ModeY, const double ScaleY, const int32_t OffsetY);
    void park_position_2_axes(const uint32_t Mode, const int32_t ParkPosX, const int32_t ParkPosY);
    void park_return_2_axes(const uint32_t Mode, const int32_t RetPosX, const int32_t RetPosY);
    void set_fly_3_axes(const uint32_t ModeX, const double ScaleX, const uint32_t ModeY, const double ScaleY, const uint32_t ModeZ, const double ScaleZ);
    void fly_return_3_axes(const int32_t RetPosX, const int32_t RetPosY, const int32_t RetPosZ);
    void rs232_write_text_list(const char* Text);
    void set_mcbsp_out(const uint32_t Signal1, const uint32_t Signal2);
    void periodic_toggle_list(const uint32_t Port, const uint32_t Mask, const uint32_t P1, const uint32_t P2, const uint32_t Count, const uint32_t Stop);
    void set_free_variable_list(const uint32_t VarNo, const uint32_t Value);
    void select_cor_table_list(const uint32_t HeadA, const uint32_t HeadB);
    void set_delay_mode_list(const uint32_t VarPoly, const uint32_t DirectMove3D, const uint32_t EdgeLevel, const uint32_t MinJumpDelay, const uint32_t JumpLengthLimit);
    void activate_scanahead_autodelays_list(const int32_t Mode);
    void set_scanahead_laser_shifts_list(const int32_t dLasOn, const int32_t dLasOff);
    void set_scanahead_line_params_ex_list(const uint32_t CornerScale, const uint32_t EndScale, const uint32_t AccScale, const uint32_t JumpScale);
    void set_scanahead_line_params_list(const uint32_t CornerScale, const uint32_t EndScale, const uint32_t AccScale);
    void stepper_enable_list(const int32_t Enable1, const int32_t Enable2);
    void stepper_control_list(const int32_t Period1, const int32_t Period2);
    void stepper_no_abs_list(const uint32_t No, const int32_t Pos);
    void stepper_no_rel_list(const uint32_t No, const int32_t dPos);
    void stepper_abs_list(const int32_t Pos1, const int32_t Pos2);
    void stepper_rel_list(const int32_t dPos1, const int32_t dPos2);
    void stepper_wait(const uint32_t No);
    void camming(const uint32_t FirstPos, const uint32_t Npos, const uint32_t EncoderNo, const uint32_t Ctrl, const double Scale);
    void set_jump_mode_list(const int32_t Flag);
    void enduring_wobbel();
    void fly_prediction(const uint32_t PredictionX, const uint32_t PredictionY);
    void wait_for_timestamp_counter_long(const uint64_t WaitCounter, const uint32_t MaxWaitTime, const uint32_t Mode);
    void set_mcbsp_out_ptr_list(const uint32_t Num, const uintptr_t SignalPtr);
    void set_mcbsp_out_oie_list(const uint32_t Signal1, const uint32_t Signal2);
    void write_port_list(const uint32_t Port, const uint32_t Value, const uint32_t NoDelay);
    void fly_disable_list();
    void init_fly_2d_list(const int32_t OffsetX, const int32_t OffsetY, const uint32_t No);
    void set_defocus_2_list(const int32_t Shift, const uint32_t Mode);
    void set_sky_writing_min_speed_list(const double Speed);
    void set_sky_writing_scale_list(const double Scale);
    void write_io_port_mask_ex_list(const uint32_t Value, const uint32_t Mask, const uint32_t NoDelay);

    template<size_t N = 0>
    auto exec_cmd(const std::vector<uint32_t>& buffer)
    {
        const auto answer = send_recv(buffer);

        uint32_t err, id;
        std::tie(err, id) = get_answer<0, 2>(answer);
        last_error = err;
        acc_error |= last_error;

        // ID mismatch
        if (answer.payload.buffer[1] != buffer[0] && network_error != NETWORK_ERR_TIMEOUT)
        {
            network_error = NETWORK_ERR_WRONG_ID;
        }

        return get_answer<2, N>(answer);
    }

    TGM_ANSW_RAW send_recv(const std::vector<uint32_t>& buffer);
    TGM_ANSW_RAW send_recv(const TGM_CMD_RAW& tgm);

private:
    std::unique_ptr<NetworkAdapter> net;

    uint32_t last_error = 0;
    uint32_t acc_error = 0;
    uint32_t seqnum = 0;

    int32_t network_error = 0;

    double timeout_initial_us = 1000.0;
    double timeout_max_sum_us = 1000000.0;
    double timeout_multiplier = 1.3;

    template<typename Array, std::size_t... I>
    auto a2t_impl(const Array& a, std::index_sequence<I...>)
    {
        return std::make_tuple(a[I]...);
    }

    template<std::size_t M, typename T, std::size_t N, typename Indices = std::make_index_sequence<M>>
    auto a2t(const std::array<T, N>& a)
    {
        return a2t_impl(a, Indices{});
    }

    template<typename T, std::size_t N, typename Indices = std::make_index_sequence<N>>
    auto a2t(const std::array<T, N>& a)
    {
        return a2t_impl(a, Indices{});
    }

    template<std::size_t S, std::size_t N>
    auto get_answer(const TGM_ANSW_RAW& tgm)
    {
        std::array<uint32_t, N> arr;
        std::copy(std::begin(tgm.payload.buffer) + S, std::begin(tgm.payload.buffer) + S + N, std::begin(arr));
        return a2t<N>(arr);
    }
};
}
