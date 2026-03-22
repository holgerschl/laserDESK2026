/* Copyright (c) 2023 SCANLAB GmbH */

#include "rtc6_rif_wrapper.h"
#include <ctime>
#include <cmath>
#include <algorithm>
#include <set>
#include <fstream>


namespace rtc6_rif
{
RTC::RTC(std::unique_ptr<NetworkAdapter> adapter)
    : net{ std::move(adapter) }
{
    // Query current seqnum
    const std::vector<uint32_t> payload{ 0x12345678 };
    const TGM_ANSW_RAW answ = send_recv(payload);
    seqnum = answ.payload.buffer[0] + 1;
}

uint32_t RTC::get_last_error() const
{
    return last_error;
}

uint32_t RTC::get_error() const
{
    return acc_error;
}

void RTC::reset_error(uint32_t Code)
{
    acc_error &= ~Code;
}

int32_t RTC::get_network_error() const
{
    return network_error;
}

void RTC::eth_set_com_timeouts_auto(const double InitialTimeoutMs, const double MaxTimeoutMs, const double Multiplier)
{
    timeout_initial_us = InitialTimeoutMs * 1'000;
    timeout_max_sum_us = MaxTimeoutMs * 1'000;
    timeout_multiplier = Multiplier;
}

bool RTC::disable_remote_interface()
{
    const std::vector<uint32_t> payload{ 0x0D15AB1E };
    const TGM_ANSW_RAW answ = send_recv(payload);
    return 0xD15AB1ED == answ.payload.buffer[0];
}


// Helpers
static std::tuple<uint32_t, uint32_t> as_uints(double d)
{
    uint32_t arr[sizeof(d) / sizeof(uint32_t)];
    std::memcpy(arr, &d, sizeof(d));
    return std::make_tuple(arr[0], arr[1]);
}

static constexpr std::tuple<uint32_t, uint32_t> as_uints(uint64_t u64)
{
    return std::make_tuple(static_cast<uint32_t>(u64), static_cast<uint32_t>(u64 >> 32));
}

static double as_double(const std::tuple<uint32_t, uint32_t> tup)
{
    const uint32_t arr[2] = { std::get<0>(tup), std::get<1>(tup) };
    double d;
    std::memcpy(&d, arr, sizeof(d));
    return d;
}

static constexpr uint64_t as_uint64_t(const std::tuple<uint32_t, uint32_t> tup)
{
    return (static_cast<uint64_t>(std::get<1>(tup)) << 32) | std::get<0>(tup);
}

static void append_string(std::vector<uint32_t>& buffer, const std::string& str)
{
    const size_t nullTerminatedLen = str.size() + 1;
    const size_t bufferLen = static_cast<size_t>(std::ceil(static_cast<double>(nullTerminatedLen) / sizeof(uint32_t)));

    std::vector<uint32_t> textBuffer(bufferLen);
    std::memcpy(textBuffer.data(), str.c_str(), nullTerminatedLen);
    buffer.insert(buffer.end(), textBuffer.begin(), textBuffer.end());
}

static constexpr uint32_t NO_Z_MOVE(0x7FFFFFFF);
#define DOUBLE_PARA(val) std::get<0>(as_uints(val)), std::get<1>(as_uints(val))
#define INT_PARA(val)    static_cast<uint32_t>(val)
#define UINT64_PARA(val) std::get<0>(as_uints(val)), std::get<1>(as_uints(val))


// Control commands
void RTC::config_list(const uint32_t Mem1, const uint32_t Mem2)
{
    exec_cmd({ R_DC_CONFIG_LIST, Mem1, Mem2 });
}

void RTC::get_config_list()
{
    exec_cmd({ R_DC_GET_CONFIG_LIST });
}

void RTC::set_start_list_pos(const uint32_t ListNo, const uint32_t Pos)
{
    exec_cmd({ R_DC_SET_START_LIST_POS, ListNo, Pos });
}

void RTC::set_start_list(const uint32_t ListNo)
{
    set_start_list_pos(ListNo, 0);
}

void RTC::set_start_list_1()
{
    set_start_list(1);
}

void RTC::set_start_list_2()
{
    set_start_list(2);
}

uint32_t RTC::get_input_pointer()
{
    return std::get<0>(exec_cmd<1>({ R_DC_GET_INPUT_POINTER }));
}
uint32_t RTC::get_list_space()
{
    return std::get<0>(exec_cmd<1>({ R_DC_GET_LIST_SPACE }));
}

uint32_t RTC::load_list(const uint32_t ListNo, const uint32_t Pos)
{
    return std::get<0>(exec_cmd<1>({ R_DC_LOAD_LIST_POS, ListNo, Pos }));
}

void RTC::load_sub(const uint32_t Index)
{
    exec_cmd({ R_DC_LOAD_SUB, Index });
}

void RTC::load_char(const uint32_t Char)
{
    exec_cmd({ R_DC_LOAD_CHAR, Char });
}

void RTC::load_text_table(const uint32_t Index)
{
    exec_cmd({ R_DC_LOAD_TEXT_TABLE, Index });
}

void RTC::set_extstartpos(const uint32_t Pos)
{
    exec_cmd({ R_DC_SET_EXTSTART_POS, Pos });
}

void RTC::set_max_counts(const uint32_t Counts)
{
    exec_cmd({ R_DC_SET_MAX_COUNT, Counts });
}

uint32_t RTC::get_counts()
{
    return std::get<0>(exec_cmd<1>({ R_DC_GET_COUNTS }));
}

void RTC::store_timestamp_counter()
{
    exec_cmd({ R_DC_STORE_TIMESTAMP });
}

void RTC::simulate_ext_start_ctrl()
{
    exec_cmd({ R_DC_SIMULATE_EXT_START });
}

void RTC::execute_list_pos(const uint32_t ListNo, const uint32_t Pos)
{
    exec_cmd({ R_DC_EXECUTE_LIST_POS, ListNo, Pos });
}

void RTC::execute_list(const uint32_t ListNo)
{
    execute_list_pos(ListNo, 0);
}

void RTC::execute_list_1()
{
    execute_list(1);
}

void RTC::execute_list_2()
{
    execute_list(2);
}

void RTC::stop_execution()
{
    exec_cmd({ R_DC_STOP_EXECUTION });
}

void RTC::simulate_ext_stop()
{
    exec_cmd({ R_DC_SIMULATE_EXT_STOP });
}

void RTC::set_extern_start_delay(const int32_t Delay, const uint32_t EncoderNo)
{
    exec_cmd({ R_DC_EXTERN_START_DELAY, INT_PARA(Delay), EncoderNo });
}

void RTC::auto_change_pos(const uint32_t Pos)
{
    exec_cmd({ R_DC_AUTO_CHANGE_POS, Pos });
}

void RTC::auto_change()
{
    auto_change_pos(0);
}

void RTC::start_loop()
{
    exec_cmd({ R_DC_START_LOOP });
}

void RTC::quit_loop()
{
    exec_cmd({ R_DC_QUIT_LOOP });
}

void RTC::pause_list()
{
    exec_cmd({ R_DC_PAUSE_LIST });
}

void RTC::restart_list()
{
    exec_cmd({ R_DC_RESTART_LIST });
}

void RTC::release_wait()
{
    exec_cmd({ R_DC_RELEASE_WAIT });
}

uint32_t RTC::get_wait_status()
{
    return std::get<0>(exec_cmd<1>({ R_DC_GET_WAIT_STATUS }));
}

void RTC::set_pause_list_cond(const uint32_t Mask1, const uint32_t Mask0)
{
    exec_cmd({ R_DC_SET_PAUSE_LIST_COND, Mask1, Mask0, 0 });
}

void RTC::set_pause_list_not_cond(const uint32_t Mask1, const uint32_t Mask0)
{
    exec_cmd({ R_DC_SET_PAUSE_LIST_COND, Mask1, Mask0, 1 });
}

double RTC::get_time()
{
    return as_double(exec_cmd<2>({ R_DC_GET_TIME }));
}

double RTC::get_lap_time()
{
    return as_double(exec_cmd<2>({ R_DC_GET_LAP_TIME }));
}

void RTC::set_control_mode(const uint32_t Mode)
{
    exec_cmd({ R_DC_SET_CONTROL_MODE, Mode });
}

uint32_t RTC::read_status()
{
    return std::get<0>(exec_cmd<1>({ R_DC_READ_STATUS }));
}


void RTC::get_status(uint32_t& Status, uint32_t& Pos)
{
    std::tie(Status, Pos) = exec_cmd<2>({ R_DC_GET_STATUS });
}

uint32_t RTC::get_startstop_info()
{
    return std::get<0>(exec_cmd<1>({ R_DC_GET_STARTSTOP_INFO }));
}

uint32_t RTC::get_overrun()
{
    return std::get<0>(exec_cmd<1>({ R_DC_GET_OVERRUN }));
}

void RTC::control_command(const uint32_t Head, const uint32_t Axis, const uint32_t Data)
{
    exec_cmd({ R_DC_CONTROL_COMMAND, Head, Axis, Data });
}

int32_t RTC::get_value(const uint32_t Signal)
{
    return std::get<0>(exec_cmd<1>({ R_DC_GET_VALUE, Signal }));
}

void RTC::get_values(const uintptr_t SignalPtr, const uintptr_t ResultPtr)
{
    if (SignalPtr && ResultPtr)
    {
        const uint32_t* const pSig(reinterpret_cast<uint32_t*>(SignalPtr));
        int32_t* const pRes(reinterpret_cast<int32_t*>(ResultPtr));
        std::tie(pRes[0], pRes[1], pRes[2], pRes[3]) = exec_cmd<4>({ R_DC_GET_VALUES, pSig[0], pSig[1], pSig[2], pSig[3] });
    }
}

uint32_t RTC::get_head_status(const uint32_t HeadNo)
{
    return std::get<0>(exec_cmd<1>({ R_DC_GET_HEAD_STATUS, HeadNo }));
}

double RTC::get_head_para(const uint32_t HeadNo, const uint32_t ParaNo)
{
    return as_double(exec_cmd<2>({ R_DC_GET_HEAD_PARA, HeadNo, ParaNo }));
}

double RTC::get_table_para(const uint32_t TableNo, const uint32_t ParaNo)
{
    return as_double(exec_cmd<2>({ R_DC_GET_TABLE_PARA, TableNo, ParaNo }));
}

int32_t RTC::get_z_distance(const int32_t X, const int32_t Y, const int32_t Z)
{
    return std::get<0>(exec_cmd<1>({ R_DC_GET_Z_DISTANCE, INT_PARA(X), INT_PARA(Y), INT_PARA(Z) }));
}

void RTC::get_galvo_controls(const uintptr_t InPtr, const uintptr_t OutPtr)
{
    if (InPtr && OutPtr)
    {
        const uint32_t* const pIn(reinterpret_cast<uint32_t*>(InPtr));
        int32_t* const pOut(reinterpret_cast<int32_t*>(OutPtr));
        std::tie(pOut[0], pOut[1], pOut[2], pOut[3]) = exec_cmd<4>({ R_DC_GET_GALVO_CONTROLS, pIn[0], pIn[1], pIn[2], pIn[3], pIn[4] });
    }
}

void RTC::set_laser_control(const uint32_t Ctrl)
{
    exec_cmd({ R_DC_SET_LASER_CONTROL, Ctrl });
}

void RTC::set_laser_mode(const uint32_t Mode)
{
    exec_cmd({ R_DC_SET_LASER_MODE, Mode });
}

void RTC::enable_laser()
{
    exec_cmd({ R_DC_ENABLE_LASER });
}

void RTC::disable_laser()
{
    exec_cmd({ R_DC_DISABLE_LASER });
}

void RTC::laser_signal_on()
{
    exec_cmd({ R_DC_LASER_SIGNAL_ON });
}

void RTC::laser_signal_off()
{
    exec_cmd({ R_DC_LASER_SIGNAL_OFF });
}

void RTC::set_qswitch_delay(const uint32_t Delay)
{
    exec_cmd({ R_DC_SET_QSWITCH_DELAY, Delay });
}

void RTC::set_laser_pulses_ctrl(const uint32_t HalfPeriod, const uint32_t PulseLength)
{
    exec_cmd({ R_DC_SET_LASER_TIMING, HalfPeriod, PulseLength });
}

void RTC::set_firstpulse_killer(const uint32_t Length)
{
    exec_cmd({ R_DC_SET_FIRST_PULSE_KILLER, Length });
}

void RTC::set_laser_pin_out(const uint32_t Pins)
{
    exec_cmd({ R_DC_SET_LASER_PIN_OUT, Pins });
}

uint32_t RTC::get_laser_pin_in()
{
    return std::get<0>(exec_cmd<1>({ R_DC_GET_LASER_PIN_IN }));
}

void RTC::set_standby(const uint32_t HalfPeriod, const uint32_t PulseLength)
{
    exec_cmd({ R_DC_SET_STANDBY_TIMING, HalfPeriod, PulseLength });
}

void RTC::get_standby(uint32_t& HalfPeriod, uint32_t& PulseLength)
{
    std::tie(HalfPeriod, PulseLength) = exec_cmd<2>({ R_DC_GET_STANDBY });
}

void RTC::set_pulse_picking(const uint32_t No)
{
    exec_cmd({ R_DC_PULSE_PICKING, No });
}

void RTC::set_pulse_picking_length(const uint32_t Length)
{
    exec_cmd({ R_DC_PULSE_PICKING_LENGTH, Length });
}

void RTC::config_laser_signals(const uint32_t Config)
{
    exec_cmd({ R_DC_CONFIG_LASER_SIGNALS, Config });
}

uint32_t RTC::set_auto_laser_control(const uint32_t Ctrl, const uint32_t Value, const uint32_t Mode, const uint32_t MinValue, const uint32_t MaxValue)
{
    return std::get<0>(exec_cmd<1>({ R_DC_SET_AUTO_LASER_CONTROL, Ctrl, Value, Mode, MinValue, MaxValue }));
}

uint32_t RTC::set_auto_laser_params(const uint32_t Ctrl, const uint32_t Value, const uint32_t MinValue, const uint32_t MaxValue)
{
    return std::get<0>(exec_cmd<1>({ R_DC_SET_AUTO_LASER_PARAMS, Ctrl, Value, MinValue, MaxValue }));
}

void RTC::spot_distance_ctrl(const double Dist)
{
    exec_cmd({ R_DC_SPOT_DISTANCE, DOUBLE_PARA(Dist) });
}

void RTC::set_encoder_speed_ctrl(const uint32_t EncoderNo, const double Speed, const double Smooth)
{
    exec_cmd({ R_DC_SET_ENCODER_SPEED, EncoderNo, DOUBLE_PARA(Speed), DOUBLE_PARA(Smooth) });
}

void RTC::set_default_pixel(const uint32_t PulseLength)
{
    exec_cmd({ R_DC_SET_DEFAULT_PIXEL, PulseLength });
}

void RTC::set_port_default(const uint32_t Port, const uint32_t Value)
{
    exec_cmd({ R_DC_SET_PORT_DEFAULT, Port, Value });
}

void RTC::write_8bit_port(const uint32_t Value)
{
    exec_cmd({ R_DC_WRITE_8BIT_PORT, Value });
}

void RTC::write_io_port_mask(const uint32_t Value, const uint32_t Mask)
{
    exec_cmd({ R_DC_WRITE_IO_PORT_MASK, Value, Mask });
}

void RTC::write_io_port(const uint32_t Value)
{
    write_io_port_mask(Value, 0xFFFF);
}

uint32_t RTC::get_io_status()
{
    return std::get<0>(exec_cmd<1>({ R_DC_GET_IO_STATUS }));
}

uint32_t RTC::read_io_port()
{
    return std::get<0>(exec_cmd<1>({ R_DC_READ_IO_PORT }));
}

uint32_t RTC::read_io_port_buffer(const uint32_t Index, uint32_t& Value, int32_t& XPos, int32_t& YPos, uint32_t& Time)
{
    uint32_t CurrentIndex;
    std::tie(CurrentIndex, Value, XPos, YPos, Time) = exec_cmd<5>({ R_DC_READ_IO_PORT_BUF, Index });
    return CurrentIndex;
}

void RTC::write_da_x(const uint32_t x, const uint32_t Value)
{
    exec_cmd({ R_DC_WRITE_DA_X, x, Value });
}

void RTC::write_da_1(const uint32_t Value)
{
    write_da_x(1, Value);
}

void RTC::write_da_2(const uint32_t Value)
{
    write_da_x(2, Value);
}

uint32_t RTC::read_analog_in()
{
    return std::get<0>(exec_cmd<1>({ R_DC_READ_ANALOG_IN }));
}

void RTC::set_jump_speed_ctrl(const double Speed)
{
    exec_cmd({ R_DC_SET_JUMP_SPEED, DOUBLE_PARA(Speed) });
}

void RTC::set_mark_speed_ctrl(const double Speed)
{
    exec_cmd({ R_DC_SET_MARK_SPEED, DOUBLE_PARA(Speed) });
}

void RTC::set_defocus(const int32_t Shift)
{
    exec_cmd({ R_DC_SET_DEFOCUS, INT_PARA(Shift) });
}

void RTC::set_defocus_offset(const int32_t Shift)
{
    exec_cmd({ R_DC_SET_DEFOCUS_OFFSET, INT_PARA(Shift) });
}

void RTC::set_delay_mode(const uint32_t VarPoly, const uint32_t DirectMove3D, const uint32_t EdgeLevel, const uint32_t MinJumpDelay, const uint32_t JumpLengthLimit)
{
    exec_cmd({ R_DC_SET_DELAY_MODE, VarPoly, DirectMove3D, EdgeLevel, MinJumpDelay, JumpLengthLimit });
}

void RTC::goto_xyz(const int32_t X, const int32_t Y, const int32_t Z)
{
    exec_cmd({ R_DC_GOTO_XYZ, INT_PARA(X), INT_PARA(Y), INT_PARA(Z) });
}

void RTC::goto_xy(const int32_t X, const int32_t Y)
{
    goto_xyz(X, Y, NO_Z_MOVE);
}

void RTC::set_offset_xyz(const uint32_t HeadNo, const int32_t X, const int32_t Y, const int32_t Z, const uint32_t at_once)
{
    exec_cmd({ R_DC_SET_OFFSET_XYZ, HeadNo, INT_PARA(X), INT_PARA(Y), INT_PARA(Z), at_once });
}

void RTC::set_offset(const uint32_t HeadNo, const int32_t X, const int32_t Y, const uint32_t at_once)
{
    set_offset_xyz(HeadNo, X, Y, NO_Z_MOVE, at_once);
}

void RTC::home_position_xyz(const int32_t XHome, const int32_t YHome, const int32_t ZHome)
{
    exec_cmd({ R_DC_HOME_POSITION_XYZ, INT_PARA(XHome), INT_PARA(YHome), INT_PARA(ZHome) });
}

void RTC::home_position(const int32_t XHome, const int32_t YHome)
{
    home_position_xyz(XHome, YHome, NO_Z_MOVE);
}

void RTC::time_update()
{
    std::time_t time(std::time(nullptr));
    std::tm* now = std::localtime(&time);

    exec_cmd({ R_DC_TIME_UPDATE,
               INT_PARA(now->tm_year + 1900),
               INT_PARA(now->tm_mon + 1),
               INT_PARA(now->tm_mday),
               INT_PARA(now->tm_yday),
               INT_PARA(now->tm_wday + 1),
               INT_PARA(now->tm_hour),
               INT_PARA(now->tm_min),
               INT_PARA(now->tm_sec) });
}

void RTC::time_control_eth(const double PPM)
{
    exec_cmd({ R_DC_TIME_CONTROL_ETH, DOUBLE_PARA(PPM) });
}

void RTC::set_serial_step(const uint32_t No, const uint32_t Step)
{
    exec_cmd({ R_DC_SET_SERIAL, No, Step });
}

void RTC::select_serial_set(const uint32_t No)
{
    exec_cmd({ R_DC_SELECT_SERIAL_SET, No });
}

double RTC::get_serial()
{
    return as_double(exec_cmd<2>({ R_DC_GET_SERIAL }));
}

double RTC::get_list_serial(uint32_t& Set)
{
    uint32_t LastMarkedLo, LastMarkedHi;
    std::tie(Set, LastMarkedLo, LastMarkedHi) = exec_cmd<3>({ R_DC_GET_LIST_SERIAL });
    return as_double(std::make_tuple(LastMarkedLo, LastMarkedHi));
}

void RTC::set_zoom(const uint32_t Zoom)
{
    exec_cmd({ R_DC_SET_ZOOM, Zoom });
}

void RTC::set_matrix(const uint32_t HeadNo, const double M11, const double M12, const double M21, const double M22, const uint32_t at_once)
{
    exec_cmd({ R_DC_SET_MATRIX, HeadNo, DOUBLE_PARA(M11), DOUBLE_PARA(M12), DOUBLE_PARA(M21), DOUBLE_PARA(M22), at_once });
}

void RTC::set_angle(const uint32_t HeadNo, const double Angle, const uint32_t at_once)
{
    exec_cmd({ R_DC_SET_ANGLE, HeadNo, DOUBLE_PARA(Angle), at_once });
}

void RTC::set_scale(const uint32_t HeadNo, const double Scale, const uint32_t at_once)
{
    exec_cmd({ R_DC_SET_SCALE, HeadNo, DOUBLE_PARA(Scale), at_once });
}

void RTC::simulate_encoder(const uint32_t EncoderNo)
{
    exec_cmd({ R_DC_SIMULATE_ENCODER, EncoderNo });
}

void RTC::read_encoder(int32_t& Encoder0_0, int32_t& Encoder1_0, int32_t& Encoder0_1, int32_t& Encoder1_1)
{
    std::tie(Encoder0_0, Encoder1_0, Encoder0_1, Encoder1_1) = exec_cmd<4>({ R_DC_READ_ENCODER });
}

void RTC::get_encoder(int32_t& Encoder0, int32_t& Encoder1)
{
    std::tie(Encoder0, Encoder1) = exec_cmd<2>({ R_DC_GET_ENCODER });
}

void RTC::init_fly_2d(const int32_t OffsetX, const int32_t OffsetY, const uint32_t No)
{
    exec_cmd({ R_DC_INIT_FLY_2D, INT_PARA(OffsetX), INT_PARA(OffsetY), No });
}

void RTC::get_fly_2d_offset(int32_t& OffsetX, int32_t& OffsetY)
{
    std::tie(OffsetX, OffsetY) = exec_cmd<2>({ R_DC_GET_FLY_2D_OFFSET });
}

uint32_t RTC::get_marking_info()
{
    return std::get<0>(exec_cmd<1>({ R_DC_GET_MARKING_INFO }));
}

void RTC::set_rot_center(const int32_t X, const int32_t Y)
{
    exec_cmd({ R_DC_ROT_CENTER, INT_PARA(X), INT_PARA(Y) });
}

uint32_t RTC::set_mcbsp_freq(const uint32_t Freq)
{
    return std::get<0>(exec_cmd<1>({ R_DC_SET_MCBSP_FREQ, Freq }));
}

void RTC::mcbsp_init(const uint32_t XDelay, const uint32_t RDelay)
{
    exec_cmd({ R_DC_MCBSP_INIT, XDelay, RDelay });
}

void RTC::set_mcbsp_x(const double Scale)
{
    exec_cmd({ R_DC_SET_MCBSP_MODE, 1, DOUBLE_PARA(Scale) });
}

void RTC::set_mcbsp_y(const double Scale)
{
    exec_cmd({ R_DC_SET_MCBSP_MODE, 2, DOUBLE_PARA(Scale) });
}

void RTC::set_mcbsp_rot(const double Resolution)
{
    exec_cmd({ R_DC_SET_MCBSP_MODE, 4, DOUBLE_PARA(Resolution) });
}

void RTC::set_mcbsp_matrix()
{
    exec_cmd({ R_DC_SET_MCBSP_MODE, 0, DOUBLE_PARA(0.0) });
}

void RTC::set_mcbsp_global_x(const double Scale)
{
    exec_cmd({ R_DC_SET_MCBSP_MODE_GLOBAL, 1, DOUBLE_PARA(Scale) });
}

void RTC::set_mcbsp_global_y(const double Scale)
{
    exec_cmd({ R_DC_SET_MCBSP_MODE_GLOBAL, 2, DOUBLE_PARA(Scale) });
}

void RTC::set_mcbsp_global_rot(const double Resolution)
{
    exec_cmd({ R_DC_SET_MCBSP_MODE_GLOBAL, 4, DOUBLE_PARA(Resolution) });
}

void RTC::set_mcbsp_global_matrix()
{
    exec_cmd({ R_DC_SET_MCBSP_MODE_GLOBAL, 0, DOUBLE_PARA(0.0) });
}

void RTC::set_mcbsp_in(const uint32_t Mode, const double Scale)
{
    exec_cmd({ R_DC_SET_MCBSP_IN, Mode, DOUBLE_PARA(Scale) });
}

void RTC::set_multi_mcbsp_in(const uint32_t Ctrl, const uint32_t P, const uint32_t Mode)
{
    exec_cmd({ R_DC_SET_MULTI_MCBSP_IN, Ctrl, P, Mode });
}

uint32_t RTC::read_mcbsp(const uint32_t No)
{
    return std::get<0>(exec_cmd<1>({ R_DC_READ_MCBSP, No }));
}

uint32_t RTC::read_multi_mcbsp(const uint32_t No)
{
    return std::get<0>(exec_cmd<1>({ R_DC_READ_MULTI_MCBSP, No }));
}

void RTC::apply_mcbsp(const uint32_t HeadNo, const uint32_t at_once)
{
    exec_cmd({ R_DC_APPLY_MCBSP, HeadNo, at_once });
}

uint32_t RTC::get_scanahead_params(const uint32_t HeadNo, uint32_t& Preview, uint32_t& Vmax, double& Amax)
{
    uint32_t Error, Al, Ah;
    std::tie(Error, Preview, Vmax, Al, Ah) = exec_cmd<5>({ R_DC_GET_SCANAHEAD_PARAMS, HeadNo });
    Amax = as_double(std::make_tuple(Al, Ah));
    return Error;
}

uint32_t RTC::activate_scanahead_autodelays(const int32_t Mode)
{
    return std::get<0>(exec_cmd<1>({ R_DC_SCANAHEAD_ACTIVATE_AUTO, INT_PARA(Mode) }));
}

void RTC::set_scanahead_laser_shifts(const int32_t dLasOn, const int32_t dLasOff)
{
    exec_cmd({ R_DC_SET_SCANAHEAD_LASER_SHIFT, INT_PARA(dLasOn), INT_PARA(dLasOff) });
}

void RTC::set_scanahead_line_params_ex(const uint32_t CornerScale, const uint32_t EndScale, const uint32_t AccScale, const uint32_t JumpScale)
{
    exec_cmd({ R_DC_SET_SCANAHEAD_LINE_PARAMS, CornerScale, EndScale, AccScale, JumpScale });
}

void RTC::set_scanahead_line_params(const uint32_t CornerScale, const uint32_t EndScale, const uint32_t AccScale)
{
    set_scanahead_line_params_ex(CornerScale, EndScale, AccScale, CornerScale);
}

uint32_t RTC::set_scanahead_params(const uint32_t Mode, const uint32_t HeadNo, const uint32_t TableNo, const uint32_t Preview, const uint32_t Vmax, const double Amax)
{
    return std::get<0>(exec_cmd<1>({ R_DC_SET_SCANAHEAD_PARAMS, Mode, HeadNo, TableNo, Preview, Vmax, DOUBLE_PARA(Amax) }));
}

void RTC::set_scanahead_speed_control(const uint32_t Mode)
{
    exec_cmd({ R_DC_SET_SCANAHEAD_SPEED_CTRL, Mode });
}

void RTC::stepper_init(const uint32_t No, const uint32_t Period, const int32_t Dir, const int32_t Pos, const uint32_t Tol, const uint32_t Enable, const uint32_t WaitTime)
{
    exec_cmd({ R_DC_STEPPER_INIT, No, Period, INT_PARA(Dir), INT_PARA(Pos), Tol, Enable, WaitTime });
}

void RTC::stepper_enable(const int32_t Enable1, const int32_t Enable2)
{
    exec_cmd({ R_DC_STEPPER_ENABLE, INT_PARA(Enable1), INT_PARA(Enable2) });
}

void RTC::stepper_disable_switch(const int32_t Disable1, const int32_t Disable2)
{
    exec_cmd({ R_DC_STEPPER_DISABLE_SWITCH, INT_PARA(Disable1), INT_PARA(Disable2) });
}

void RTC::stepper_control(const int32_t Period1, const int32_t Period2)
{
    exec_cmd({ R_DC_STEPPER_CTRL, INT_PARA(Period1), INT_PARA(Period2) });
}

void RTC::stepper_abs_no(const uint32_t No, const int32_t Pos, const uint32_t WaitTime)
{
    exec_cmd({ R_DC_STEPPER_NO_ABS, No, INT_PARA(Pos), WaitTime });
}

void RTC::stepper_rel_no(const uint32_t No, const int32_t dPos, const uint32_t WaitTime)
{
    exec_cmd({ R_DC_STEPPER_NO_REL, No, INT_PARA(dPos), WaitTime });
}

void RTC::stepper_abs(const int32_t Pos1, const int32_t Pos2, const uint32_t WaitTime)
{
    exec_cmd({ R_DC_STEPPER_ABS, INT_PARA(Pos1), INT_PARA(Pos2), WaitTime });
}

void RTC::stepper_rel(const int32_t dPos1, const int32_t dPos2, const uint32_t WaitTime)
{
    exec_cmd({ R_DC_STEPPER_REL, INT_PARA(dPos1), INT_PARA(dPos2), WaitTime });
}

void RTC::get_stepper_status(uint32_t& Status1, int32_t& Pos1, uint32_t& Status2, int32_t& Pos2)
{
    std::tie(Status1, Pos1, Status2, Pos2) = exec_cmd<4>({ R_DC_GET_STEPPER_STATUS });
}

void RTC::set_sky_writing_para(const double Timelag, const int32_t LaserOnShift, const uint32_t Nprev, const uint32_t Npost)
{
    exec_cmd({ R_DC_SET_SKY_WRITING_PARA, DOUBLE_PARA(Timelag), INT_PARA(LaserOnShift), Nprev, Npost });
}

void RTC::set_sky_writing(const double Timelag, const int32_t LaserOnShift)
{
    set_sky_writing_para(Timelag, LaserOnShift, 0xFFFF, 0xFFFF);
}

void RTC::set_sky_writing_limit(const double Limit)
{
    exec_cmd({ R_DC_SET_SKY_WRITING_ANGLE, DOUBLE_PARA(Limit) });
}

void RTC::set_sky_writing_mode(const uint32_t Mode)
{
    exec_cmd({ R_DC_SET_SKY_WRITING_MODE, Mode });
}

uint32_t RTC::uart_config(const uint32_t BaudRate)
{
    return std::get<0>(exec_cmd<1>({ R_DC_UART_CONFIG, BaudRate }));
}

void RTC::rs232_write_data(const uint32_t Data)
{
    exec_cmd({ R_DC_RS232_WRITE_DATA, Data });
}

void RTC::rs232_write_text(const char* Text)
{
    constexpr size_t maxLen = TGM_MAX_SIZE - sizeof(TGM_HEADER) - sizeof(uint32_t) - 1;
    const std::string strText(Text);

    for (size_t i = 0; i < strText.length(); i += maxLen)
    {
        std::vector<uint32_t> buffer{ R_DC_RS232_WRITE_TEXT };
        append_string(buffer, strText.substr(i, std::min(maxLen, strText.length() - i)).c_str());
        exec_cmd(buffer);
    }
}

uint32_t RTC::rs232_read_data()
{
    return std::get<0>(exec_cmd<1>({ R_DC_RS232_READ_DATA }));
}

void RTC::bounce_supp(const uint32_t Length)
{
    exec_cmd({ R_DC_BOUNCE_SUPP, Length });
}

void RTC::set_hi(const uint32_t HeadNo, const double GalvoGainX, const double GalvoGainY, const int32_t GalvoOffsetX, const int32_t GalvoOffsetY)
{
    exec_cmd({ R_DC_SET_HI, HeadNo, DOUBLE_PARA(GalvoGainX), DOUBLE_PARA(GalvoGainY), INT_PARA(GalvoOffsetX), INT_PARA(GalvoOffsetY) });
}

void RTC::set_jump_mode(const int32_t Flag, const uint32_t Length, const int32_t VA1, const int32_t VA2, const int32_t VB1, const int32_t VB2,
                        const int32_t JA1, const int32_t JA2, const int32_t JB1, const int32_t JB2)
{
    exec_cmd({ R_DC_SET_JUMP_MODE, INT_PARA(Flag), Length, INT_PARA(VA1), INT_PARA(VA2), INT_PARA(VB1), INT_PARA(VB2),
               INT_PARA(JA1), INT_PARA(JA2), INT_PARA(JB1), INT_PARA(JB2) });
}

void RTC::select_cor_table(const uint32_t HeadA, const uint32_t HeadB)
{
    exec_cmd({ R_DC_SELECT_COR_TABLE, HeadA, HeadB });
}

void RTC::set_free_variable(const uint32_t VarNo, const uint32_t Value)
{
    exec_cmd({ R_DC_SET_FREE_VAR, VarNo, Value });
}

uint32_t RTC::get_free_variable(const uint32_t VarNo)
{
    return std::get<0>(exec_cmd<1>({ R_DC_GET_FREE_VAR, VarNo }));
}

void RTC::set_mcbsp_out_ptr(const uint32_t Num, const uintptr_t SignalPtr)
{
    if (!SignalPtr)
    {
        return;
    }

    const uint32_t* const pSig = reinterpret_cast<uint32_t*>(SignalPtr);
    std::vector<uint32_t> buf{ R_DC_SET_MCBSP_OUT_PTR, Num };
    buf.insert(buf.cend(), pSig, pSig + std::min(Num, 8u));
    exec_cmd(buf);
}

void RTC::periodic_toggle(const uint32_t Port, const uint32_t Mask, const uint32_t P1, const uint32_t P2, const uint32_t Count, const uint32_t Start)
{
    exec_cmd({ R_DC_PERIODIC_TOGGLE, Port, Mask, P1, P2, Count, Start });
}

void RTC::set_enduring_wobbel_1(const uint32_t CenterX, const uint32_t CenterY, const uint32_t CenterZ, const uint32_t LimitHi, const uint32_t LimitLo,
                                const double ScaleX, const double ScaleY, const double ScaleZ)
{
    exec_cmd({ R_DC_ENDURING_WOBBEL_1, CenterX, CenterY, CenterZ, LimitHi, LimitLo, DOUBLE_PARA(ScaleX), DOUBLE_PARA(ScaleY), DOUBLE_PARA(ScaleZ) });
}

void RTC::set_enduring_wobbel_2(const uint32_t CenterX, const uint32_t CenterY, const uint32_t CenterZ, const uint32_t LimitHi, const uint32_t LimitLo,
                                const double ScaleX, const double ScaleY, const double ScaleZ)
{
    exec_cmd({ R_DC_ENDURING_WOBBEL_2, CenterX, CenterY, CenterZ, LimitHi, LimitLo, DOUBLE_PARA(ScaleX), DOUBLE_PARA(ScaleY), DOUBLE_PARA(ScaleZ) });
}

void RTC::stop_trigger()
{
    exec_cmd({ R_DC_STOP_TRIGGER });
}

void RTC::measurement_status(uint32_t& Busy, uint32_t& Pos)
{
    std::tie(Busy, Pos) = exec_cmd<2>({ R_DC_MEASUREMENT_STATUS });
}

void RTC::get_waveform_offset(const uint32_t Channel, const uint32_t Offset, const uint32_t Number, const uintptr_t Ptr)
{
    if (!Ptr || Number == 0)
    {
        return;
    }

    uint32_t off = 0;
    uint32_t remaining = Number;
    constexpr uint32_t maxNumber(sizeof(TGM_PL_ANSW_RAW) / sizeof(uint32_t) - 2);

    while (remaining > 0)
    {
        const auto answer = send_recv({ R_DC_GET_WAVEFORM, Channel, Offset + off, std::min(maxNumber, remaining) });

        uint32_t err, id;
        std::tie(err, id) = get_answer<0, 2>(answer);
        last_error = err;
        acc_error |= last_error;

        // ID mismatch
        if (answer.payload.buffer[1] != R_DC_GET_WAVEFORM && network_error != NETWORK_ERR_TIMEOUT)
        {
            network_error = NETWORK_ERR_WRONG_ID;
        }

        if (last_error || network_error)
        {
            return;
        }

        const uint32_t received = (answer.header.length / sizeof(int32_t)) - 2;
        std::memcpy(reinterpret_cast<int32_t*>(Ptr) + off, &answer.payload.buffer[2], received * sizeof(int32_t));
        off += received;
        remaining -= received;
    }
}

void RTC::get_waveform(const uint32_t Channel, const uint32_t Number, const uintptr_t Ptr)
{
    get_waveform_offset(Channel, 0, Number, Ptr);
}

uint32_t RTC::set_verify(const uint32_t Verify)
{
    return std::get<0>(exec_cmd<1>({ R_DC_SET_VERIFY, Verify }));
}

uint32_t RTC::get_hex_version()
{
    return std::get<0>(exec_cmd<1>({ R_DC_GET_HEX_VERSION }));
}

uint32_t RTC::get_rtc_version()
{
    return std::get<0>(exec_cmd<1>({ R_DC_GET_RTC_VERSION }));
}

uint32_t RTC::get_bios_version()
{
    return std::get<0>(exec_cmd<1>({ R_DC_GET_BIOS_VERSION }));
}

uint32_t RTC::get_serial_number()
{
    return std::get<0>(exec_cmd<1>({ R_DC_GET_SERIAL_NUMBER }));
}

void RTC::set_input_pointer(const uint32_t Pos)
{
    exec_cmd({ R_DC_SET_INPUT_POINTER, Pos });
}

void RTC::get_list_pointer(uint32_t& ListNo, uint32_t& Pos)
{
    std::tie(ListNo, Pos) = exec_cmd<2>({ R_DC_GET_LIST_POINTER });
}

void RTC::execute_at_pointer(const uint32_t Pos)
{
    exec_cmd({ R_DC_EXECUTE_AT_POINTER, Pos });
}

uint32_t RTC::get_remote_error()
{
    return std::get<0>(exec_cmd<1>({ R_DC_GET_REMOTE_ERROR }));
}

void RTC::set_laser_pulse_sync(const uint32_t Mode, const uint32_t Delay)
{
    exec_cmd({ R_DC_SET_LASER_PULSE_SYNC, Mode, Delay });
}

uint64_t RTC::get_timestamp_long()
{
    return as_uint64_t(exec_cmd<2>({ R_DC_GET_TIMESTAMP_LONG }));
}

void RTC::clear_fly_overflow_ctrl(const uint32_t Mode)
{
    exec_cmd({ R_DC_CLEAR_FLY_OVERFLOW, Mode });
}

double RTC::get_temperature()
{
    return as_double(exec_cmd<2>({ R_DC_GET_TEMPERATURE }));
}

void RTC::set_mcbsp_out_oie_ctrl(const uint32_t Signal1, const uint32_t Signal2)
{
    exec_cmd({ R_DC_SET_MCBSP_OUT_OIE, Signal1, Signal2 });
}

uint32_t RTC::load_correction_file(const char* Name, const uint32_t No, const uint32_t Dim)
{
    if (!Name)
    {
        return 3;
    }

    std::ifstream f(Name, std::ios_base::binary);
    if (!f.is_open())
    {
        return 3;
    }

    const std::vector<uint8_t> data((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    f.close();

    constexpr uint32_t maxSize(sizeof(TGM_PL_ANSW_RAW) - 3 * sizeof(uint32_t));
    uint32_t offset = 0;

    // Transfer correction file contents to RTC memory
    while (offset < data.size())
    {
        const uint32_t size(std::min(static_cast<uint32_t>(data.size()) - offset, maxSize));
        const uint32_t elements(size / sizeof(uint32_t));

        std::vector<uint32_t> payload(3 + elements);
        payload[0] = R_DC_LOAD_CORRECTION_FILE;
        payload[1] = offset;
        payload[2] = size;
        std::memcpy(payload.data() + 3, data.data() + offset, size);

        const auto answer = send_recv(payload);

        uint32_t err, id;
        std::tie(err, id) = get_answer<0, 2>(answer);
        last_error = err;
        acc_error |= last_error;

        // ID mismatch
        if (answer.payload.buffer[1] != R_DC_LOAD_CORRECTION_FILE && network_error != NETWORK_ERR_TIMEOUT)
        {
            network_error = NETWORK_ERR_WRONG_ID;
        }

        if (last_error || network_error)
        {
            return 5;
        }

        offset += size;
    }

    // Perform load_correction_file on the RTC
    const uint32_t answer = std::get<0>(exec_cmd<1>({ R_DC_LOAD_CORRECTION_FILE, UINT_MAX, (No & 0xFFFF) | ((Dim & 0xFFFF) << 16) }));
    if (network_error)
    {
        return 5;
    }

    return answer;
}

void RTC::number_of_correction_tables(const uint32_t Number)
{
    exec_cmd({ R_DC_NUMBER_OF_COR_TABLES, Number });
}

uint32_t RTC::load_z_table_no(const double A, const double B, const double C, const uint32_t No)
{
    return std::get<0>(exec_cmd<1>({ R_DC_LOAD_Z_TABLE, DOUBLE_PARA(A), DOUBLE_PARA(B), DOUBLE_PARA(C), No }));
}

uint32_t RTC::load_z_table(const double A, const double B, const double C)
{
    return load_z_table_no(A, B, C, 0);
}

uint32_t RTC::load_z_table_no_20b(const double A, const double B, const double C, const uint32_t No)
{
    return load_z_table_no(A / 16.0, B, C * 16.0, 0);
}

uint32_t RTC::load_z_table_20b(const double A, const double B, const double C)
{
    return load_z_table(A / 16.0, B, C * 16.0);
}

uint32_t RTC::load_oct_table_no(const double A, const double B, const uint32_t No)
{
    return std::get<0>(exec_cmd<1>({ R_DC_LOAD_OCT_TABLE, DOUBLE_PARA(A), DOUBLE_PARA(B), No }));
}

void RTC::set_timelag_compensation(const uint32_t HeadNo, const uint32_t TimelagXY, const uint32_t TimelagZ)
{
    exec_cmd({ R_DC_SET_TIMELAG_COMPENSATION, HeadNo, TimelagXY, TimelagZ });
}

void RTC::set_encoder_filter_ctrl(const uint32_t Length, const uint32_t Mode)
{
    exec_cmd({ R_DC_SET_ENCODER_FILTER, Length, Mode });
}

uint32_t RTC::get_master_slave()
{
    return std::get<0>(exec_cmd<1>({ R_DC_GET_MASTER_SLAVE }));
}

uint32_t RTC::get_sync_status()
{
    return std::get<0>(exec_cmd<1>({ R_DC_GET_SYNC_STATUS }));
}

void RTC::master_slave_config(const uint32_t Config)
{
    exec_cmd({ R_DC_MASTER_SLAVE_CONFIG, Config });
}

void RTC::eth_get_static_ip(uint32_t& Ip, uint32_t& Netmask, uint32_t& Gateway)
{
    std::tie(Ip, Netmask, Gateway) = exec_cmd<3>({ R_DC_GET_STATIC_IP });
}

uint32_t RTC::eth_set_static_ip(const uint32_t Ip, const uint32_t Netmask, const uint32_t Gateway)
{
    return std::get<0>(exec_cmd<1>({ R_DC_SET_STATIC_IP, Ip, Netmask, Gateway }));
}

void RTC::eth_get_port_numbers(uint32_t& UDPsearch, uint32_t& UDPexcl, uint32_t& TCP)
{
    std::tie(UDPsearch, UDPexcl, TCP) = exec_cmd<3>({ R_DC_GET_PORT_NUMBERS });
}

uint32_t RTC::eth_set_port_numbers(const uint32_t UDPsearch, const uint32_t UDPexcl, const uint32_t TCP)
{
    return std::get<0>(exec_cmd<1>({ R_DC_SET_PORT_NUMBERS, UDPsearch, UDPexcl, TCP }));
}

void RTC::eth_configure_link_loss(const uint32_t Mode)
{
    exec_cmd({ R_DC_CONFIGURE_LINK_LOSS, Mode });
}

void RTC::set_sky_writing_min_speed_ctrl(const double Speed)
{
    exec_cmd({ R_DC_SET_SKY_WRITING_MIN_SPEED, DOUBLE_PARA(Speed) });
}

void RTC::set_controlpreview_compensation_ctrl(const uint32_t ControlPreview, const uint32_t Mode)
{
    exec_cmd({ R_DC_SET_CTRL_PREVIEW_COMP, ControlPreview, Mode });
}

void RTC::set_fly_tracking_error(const uint32_t TrackingErrorX, const uint32_t TrackingErrorY)
{
    exec_cmd({ R_DC_SET_FLY_TRACKING_ERROR, TrackingErrorX, TrackingErrorY });
}

void RTC::set_laser_delays_ctrl(const int32_t LaserOnDelay, const uint32_t LaserOffDelay)
{
    exec_cmd({ R_DC_SET_LASER_DELAYS, INT_PARA(LaserOnDelay), LaserOffDelay });
}

void RTC::set_scanner_delays_ctrl(const uint32_t Jump, const uint32_t Mark, const uint32_t Polygon)
{
    exec_cmd({ R_DC_SET_SCANNER_DELAYS, Jump, Mark, Polygon });
}

void RTC::set_sky_writing_scale_ctrl(const double Scale)
{
    exec_cmd({ R_DC_SET_SKY_WRITING_SCALE, DOUBLE_PARA(Scale) });
}

uint32_t RTC::get_extended_head_status_value_ctrl(const uint32_t Head, const uint32_t Axis, const uint32_t Value)
{
    return std::get<0>(exec_cmd<1>({ R_DC_GET_EXTENDED_HEAD_STATUS_VALUE, Head, Axis, Value }));
}

void RTC::get_extended_head_status_block_ctrl(const uint32_t Head, const uint32_t Axis, const uintptr_t Ptr)
{
    if (!Ptr)
    {
        return;
    }

    const auto answer = send_recv(std::vector<uint32_t>{ R_DC_GET_EXTENDED_HEAD_STATUS_BLOCK, Head, Axis });

    uint32_t err, id;
    std::tie(err, id) = get_answer<0, 2>(answer);
    last_error = err;
    acc_error |= last_error;

    // ID mismatch
    if (answer.payload.buffer[1] != R_DC_GET_EXTENDED_HEAD_STATUS_BLOCK && network_error != NETWORK_ERR_TIMEOUT)
    {
        network_error = NETWORK_ERR_WRONG_ID;
    }

    if (last_error || network_error)
    {
        return;
    }

    std::memcpy(reinterpret_cast<int32_t*>(Ptr), &answer.payload.buffer[2], 256 * sizeof(uint32_t));
}

uint32_t RTC::get_rcm_value_ctrl(const uint32_t Head, const uint32_t Axis, const uint32_t Value)
{
    return std::get<0>(exec_cmd<1>({ R_DC_GET_RCM_VALUE, Head, Axis, Value }));
}

uint32_t RTC::demux_rcm_ctrl(const uintptr_t Input, const uint32_t InputLength, const uint32_t NSignals, const uint32_t Index, const uint32_t Flags, uintptr_t Output)
{
    const uint32_t* const in = reinterpret_cast<uint32_t*>(Input);
    uint32_t* const out = reinterpret_cast<uint32_t*>(Output);

    const bool incompleteFirstBlock = (Flags & (1 << 1)) != 0;
    const bool incompleteLastBlock = (Flags & (1 << 0)) != 0;

    constexpr uint32_t blockSize = 192;

    if (blockSize % NSignals != 0)
    {
        // SL2-100 block length not divisible by RCM block length
        return 0;
    }

    constexpr uint32_t noSyncFoundDummy = 255;
    uint32_t firstSync = noSyncFoundDummy;
    for (uint32_t i = 0; i < blockSize; ++i)
    {
        if (in[i] & (1 << 20))
        {
            firstSync = i;
            break;
        }
    }
    if (firstSync == noSyncFoundDummy)
    {
        // Failed to find sync in first 192 entries
        return 0;
    }

    const uint32_t firstFullBlock = firstSync - firstSync / NSignals * NSignals;

    const uint32_t* src = &in[firstFullBlock + Index];
    uint32_t* dest = &out[0];
    if (incompleteFirstBlock)
    {
        if (firstFullBlock + Index >= NSignals)
        {
            src -= NSignals;
        }
        else
        {
            dest += 1;
        }
    }

    const uint32_t nFullBlocks = (InputLength - firstFullBlock) / NSignals;

    const uint32_t* lastSource = &in[firstFullBlock + (nFullBlocks - 1) * NSignals + Index];

    while (src <= lastSource)
    {
        *dest = *src & 0xfffff;
        src += NSignals;
        dest += 1;
    }

    if (incompleteLastBlock && src < &in[InputLength])
    {
        *dest = *src & 0xfffff;
    }

    return static_cast<uint32_t>(dest - out);
}

void RTC::master_slave_set_offset_ctrl(const uint32_t Mode, const uint32_t Ctrl)
{
    exec_cmd({ R_DC_MASTER_SLAVE_SET_OFFSET, Mode, Ctrl });
}

void RTC::marking_info_ack_ctrl(const uint32_t AckMask)
{
    exec_cmd({ R_DC_MARKING_INFO_ACK, AckMask });
}

void RTC::set_sl2_100_error_tolerance_ctrl(const uint32_t Tol_us)
{
    exec_cmd({ R_DC_SET_SL2_100_ERROR_TOLERANCE, Tol_us });
}

uint32_t RTC::get_sl2_100_error_tolerance_ctrl()
{
    return std::get<0>(exec_cmd<1>({ R_DC_GET_SL2_100_ERROR_TOLERANCE }));
}

void RTC::set_sl2_100_error_tolerance_enable_ctrl(const bool Enable)
{
    exec_cmd({ R_DC_SET_SL2_100_ERROR_TOLERANCE_ENABLE, Enable ? 1u : 0u });
}

bool RTC::get_sl2_100_error_tolerance_enable_ctrl()
{
    return std::get<0>(exec_cmd<1>({ R_DC_GET_SL2_100_ERROR_TOLERANCE_ENABLE }));
}

void RTC::set_explicit_ack_mode_ctrl(const uint32_t Mode)
{
    exec_cmd({ R_DC_SET_EXPLICIT_ACK_MODE, Mode });
}

void RTC::startstop_info_ack_ctrl(const uint32_t AckMask)
{
    exec_cmd({ R_DC_STARTSTOP_INFO_ACK, AckMask });
}



// List commands
void RTC::list_nop()
{
    exec_cmd({ R_LC_NO_COMMAND });
}

void RTC::list_continue()
{
    exec_cmd({ R_LC_CONTINUE });
}

void RTC::list_next()
{
    exec_cmd({ R_LC_NEXT });
}

void RTC::set_end_of_list()
{
    exec_cmd({ R_LC_END_OF_LIST });
}

void RTC::list_return()
{
    exec_cmd({ R_LC_LIST_RETURN });
}

void RTC::list_repeat()
{
    exec_cmd({ R_LC_REPEAT });
}

void RTC::list_until(const uint32_t Number)
{
    exec_cmd({ R_LC_UNTIL, Number });
}

void RTC::list_jump_pos(const uint32_t Pos)
{
    exec_cmd({ R_LC_LIST_JUMP_POS, Pos });
}

void RTC::list_jump_rel(const int32_t Pos)
{
    exec_cmd({ R_LC_LIST_JUMP_REL, INT_PARA(Pos) });
}

void RTC::set_wait(const uint32_t WaitWord)
{
    exec_cmd({ R_LC_SET_WAIT, WaitWord });
}

void RTC::long_delay(const uint32_t Delay)
{
    exec_cmd({ R_LC_LONG_DELAY, Delay });
}

void RTC::set_extstartpos_list(const uint32_t Pos)
{
    exec_cmd({ R_LC_SET_EXTSTART_POS, Pos });
}

void RTC::simulate_ext_start(const int32_t Delay, const uint32_t EncoderNo)
{
    exec_cmd({ R_LC_SIMULATE_EXTERN_START, INT_PARA(Delay), EncoderNo });
}

void RTC::list_call_abs_repeat(const uint32_t Pos, const uint32_t Number)
{
    exec_cmd({ R_LC_LIST_CALL, Pos, Number, 1 });
}

void RTC::list_call_abs(const uint32_t Pos)
{
    list_call_abs_repeat(Pos, 0);
}

void RTC::list_call_repeat(const uint32_t Pos, const uint32_t Number)
{
    exec_cmd({ R_LC_LIST_CALL, Pos, Number, 0 });
}

void RTC::list_call(const uint32_t Pos)
{
    list_call_repeat(Pos, 0);
}

void RTC::sub_call_abs_repeat(const uint32_t Index, const uint32_t Number)
{
    exec_cmd({ R_LC_SUB_CALL, Index, Number, 1 });
}

void RTC::sub_call_abs(const uint32_t Index)
{
    sub_call_abs_repeat(Index, 0);
}

void RTC::sub_call_repeat(const uint32_t Index, const uint32_t Number)
{
    exec_cmd({ R_LC_SUB_CALL, Index, Number, 0 });
}

void RTC::sub_call(const uint32_t Index)
{
    sub_call_repeat(Index, 0);
}

void RTC::if_cond(const uint32_t Mask1, const uint32_t Mask0)
{
    exec_cmd({ R_LC_IF_COND, Mask1, Mask0, 0 });
}

void RTC::if_not_cond(const uint32_t Mask1, const uint32_t Mask0)
{
    exec_cmd({ R_LC_IF_COND, Mask1, Mask0, 1 });
}

void RTC::if_pin_cond(const uint32_t Mask1, const uint32_t Mask0)
{
    exec_cmd({ R_LC_IF_PIN_COND, Mask1, Mask0, 0 });
}

void RTC::if_not_pin_cond(const uint32_t Mask1, const uint32_t Mask0)
{
    exec_cmd({ R_LC_IF_PIN_COND, Mask1, Mask0, 1 });
}

void RTC::clear_io_cond_list(const uint32_t Mask1, const uint32_t Mask0, const uint32_t MaskClear)
{
    exec_cmd({ R_LC_CLEAR_IO_COND, Mask1, Mask0, MaskClear });
}

void RTC::set_io_cond_list(const uint32_t Mask1, const uint32_t Mask0, const uint32_t MaskSet)
{
    exec_cmd({ R_LC_SET_IO_COND, Mask1, Mask0, MaskSet });
}

void RTC::switch_io_port(const uint32_t MaskBits, const uint32_t ShiftBits)
{
    exec_cmd({ R_LC_SWITCH_IOPORT, MaskBits, ShiftBits });
}

void RTC::list_call_abs_cond(const uint32_t Mask1, const uint32_t Mask0, const uint32_t Pos)
{
    exec_cmd({ R_LC_LIST_CALL_COND, Mask1, Mask0, Pos, 1 });
}

void RTC::list_call_cond(const uint32_t Mask1, const uint32_t Mask0, const uint32_t Pos)
{
    exec_cmd({ R_LC_LIST_CALL_COND, Mask1, Mask0, Pos, 0 });
}

void RTC::sub_call_abs_cond(const uint32_t Mask1, const uint32_t Mask0, const uint32_t Index)
{
    exec_cmd({ R_LC_SUB_CALL_COND, Mask1, Mask0, Index, 1 });
}

void RTC::sub_call_cond(const uint32_t Mask1, const uint32_t Mask0, const uint32_t Index)
{
    exec_cmd({ R_LC_SUB_CALL_COND, Mask1, Mask0, Index, 0 });
}

void RTC::list_jump_pos_cond(const uint32_t Mask1, const uint32_t Mask0, const uint32_t Pos)
{
    exec_cmd({ R_LC_LIST_JUMP_POS_COND, Mask1, Mask0, Pos });
}

void RTC::list_jump_rel_cond(const uint32_t Mask1, const uint32_t Mask0, const int32_t Pos)
{
    exec_cmd({ R_LC_LIST_JUMP_REL_COND, Mask1, Mask0, INT_PARA(Pos) });
}

void RTC::config_laser_signals_list(const uint32_t Config)
{
    exec_cmd({ R_LC_CONFIG_LASER_SIGNALS, Config });
}

void RTC::laser_signal_on_list()
{
    exec_cmd({ R_LC_LASER_SIGNAL_ON });
}

void RTC::laser_signal_off_list()
{
    exec_cmd({ R_LC_LASER_SIGNAL_OFF });
}

void RTC::set_laser_pulses(const uint32_t HalfPeriod, const uint32_t PulseLength)
{
    exec_cmd({ R_LC_SET_LASER_TIMING, HalfPeriod, PulseLength });
}

void RTC::set_firstpulse_killer_list(const uint32_t Length)
{
    exec_cmd({ R_LC_SET_FIRST_PULSE_KILLER, Length });
}

void RTC::set_qswitch_delay_list(const uint32_t Delay)
{
    exec_cmd({ R_LC_SET_QSWITCH_DELAY, Delay });
}

void RTC::set_laser_pin_out_list(const uint32_t Pins)
{
    exec_cmd({ R_LC_SET_LASER_PIN_OUT, Pins });
}

void RTC::set_pulse_picking_list(const uint32_t No)
{
    exec_cmd({ R_LC_PULSE_PICKING, No });
}

void RTC::set_standby_list(const uint32_t HalfPeriod, const uint32_t PulseLength)
{
    exec_cmd({ R_LC_SET_STANDBY_TIMING, HalfPeriod, PulseLength });
}

void RTC::set_auto_laser_params_list(const uint32_t Ctrl, const uint32_t Value, const uint32_t MinValue, const uint32_t MaxValue)
{
    exec_cmd({ R_LC_SET_AUTO_LASER_PARAMS, Ctrl, Value, MinValue, MaxValue });
}

void RTC::set_laser_power(const uint32_t Port, const uint32_t Power)
{
    exec_cmd({ R_LC_LASER_POWER, Port, Power });
}

void RTC::spot_distance(const double Dist)
{
    exec_cmd({ R_LC_SPOT_DISTANCE, DOUBLE_PARA(Dist) });
}

void RTC::set_encoder_speed(const uint32_t Encoder, const double Speed, const double Smooth)
{
    exec_cmd({ R_LC_SET_ENCODER_SPEED, Encoder, DOUBLE_PARA(Speed), DOUBLE_PARA(Smooth) });
}

void RTC::set_default_pixel_list(const uint32_t PulseLength)
{
    exec_cmd({ R_LC_SET_DEFAULT_PIXEL, PulseLength });
}

void RTC::set_port_default_list(const uint32_t Port, const uint32_t Value)
{
    exec_cmd({ R_LC_SET_PORT_DEFAULT, Port, Value });
}

void RTC::set_pixel_line_3d(const uint32_t Channel, const uint32_t HalfPeriod, const double dX, const double dY, const double dZ)
{
    exec_cmd({ R_LC_SET_PIXEL_LINE, Channel, HalfPeriod, DOUBLE_PARA(dX), DOUBLE_PARA(dY), DOUBLE_PARA(dZ) });
}

void RTC::set_pixel_line(const uint32_t Channel, const uint32_t HalfPeriod, const double dX, const double dY)
{
    set_pixel_line_3d(Channel, HalfPeriod, dX, dY, 0);
}

void RTC::set_n_pixel(const uint32_t PortOutValue1, const uint32_t PortOutValue2, const uint32_t Number)
{
    exec_cmd({ R_LC_SET_N_PIXEL_DATA, PortOutValue1, PortOutValue2, Number });
}

void RTC::set_pixel(const uint32_t PortOutValue1, const uint32_t PortOutValue2)
{
    set_n_pixel(PortOutValue1, PortOutValue2, 1);
}

void RTC::write_8bit_port_list(const uint32_t Value)
{
    exec_cmd({ R_LC_WRITE_8BIT_PORT, Value });
}

void RTC::write_io_port_mask_list(const uint32_t Value, const uint32_t Mask)
{
    exec_cmd({ R_LC_WRITE_IO_PORT_MASK, Value, Mask });
}

void RTC::write_io_port_list(const uint32_t Value)
{
    write_io_port_mask_list(Value, 0xFFFF);
}

void RTC::read_io_port_list()
{
    exec_cmd({ R_LC_READ_IO_PORT });
}

void RTC::write_da_x_list(const uint32_t x, const uint32_t Value)
{
    exec_cmd({ R_LC_WRITE_DA_X, x, Value });
}

void RTC::write_da_1_list(const uint32_t Value)
{
    write_da_x_list(1, Value);
}

void RTC::write_da_2_list(const uint32_t Value)
{
    write_da_x_list(2, Value);
}

void RTC::set_mark_speed(const double Speed)
{
    exec_cmd({ R_LC_SET_MARK_SPEED, DOUBLE_PARA(Speed) });
}

void RTC::set_jump_speed(const double Speed)
{
    exec_cmd({ R_LC_SET_JUMP_SPEED, DOUBLE_PARA(Speed) });
}

void RTC::set_scanner_delays(const uint32_t Jump, const uint32_t Mark, const uint32_t Polygon)
{
    exec_cmd({ R_LC_SET_SCANNER_DELAYS, Jump, Mark, Polygon });
}

void RTC::set_laser_delays(const int32_t LaserOnDelay, const uint32_t LaserOffDelay)
{
    exec_cmd({ R_LC_SET_LASER_DELAYS, INT_PARA(LaserOnDelay), LaserOffDelay });
}

void RTC::set_defocus_list(const int32_t Shift)
{
    exec_cmd({ R_LC_SET_DEFOCUS, INT_PARA(Shift) });
}

void RTC::set_defocus_offset_list(const int32_t Shift)
{
    exec_cmd({ R_LC_SET_DEFOCUS_OFFSET, INT_PARA(Shift) });
}

void RTC::set_zoom_list(const uint32_t Zoom)
{
    exec_cmd({ R_LC_SET_ZOOM, Zoom });
}

void RTC::set_offset_xyz_list(const uint32_t HeadNo, const int32_t XOffset, const int32_t YOffset, const int32_t ZOffset, const uint32_t at_once)
{
    exec_cmd({ R_LC_SET_OFFSET_XYZ, HeadNo, INT_PARA(XOffset), INT_PARA(YOffset), INT_PARA(ZOffset), at_once });
}

void RTC::set_offset_list(const uint32_t HeadNo, const int32_t XOffset, const int32_t YOffset, const uint32_t at_once)
{
    set_offset_xyz_list(HeadNo, XOffset, YOffset, NO_Z_MOVE, at_once);
}

void RTC::set_matrix_list(const uint32_t HeadNo, const uint32_t Ind1, const uint32_t Ind2, const double Mij, const uint32_t at_once)
{
    exec_cmd({ R_LC_SET_MATRIX, HeadNo, Ind1, Ind2, DOUBLE_PARA(Mij), at_once });
}

void RTC::set_angle_list(const uint32_t HeadNo, const double Angle, const uint32_t at_once)
{
    exec_cmd({ R_LC_SET_ANGLE, HeadNo, DOUBLE_PARA(Angle), at_once });
}

void RTC::set_scale_list(const uint32_t HeadNo, const double Scale, const uint32_t at_once)
{
    exec_cmd({ R_LC_SET_SCALE, HeadNo, DOUBLE_PARA(Scale), at_once });
}

void RTC::timed_mark_abs_3d(const int32_t X, const int32_t Y, const int32_t Z, const double T)
{
    exec_cmd({ R_LC_MARK_XYZT_ABS, INT_PARA(X), INT_PARA(Y), INT_PARA(Z), DOUBLE_PARA(T) });
}

void RTC::timed_mark_abs(const int32_t X, const int32_t Y, const double T)
{
    timed_mark_abs_3d(X, Y, NO_Z_MOVE, T);
}

void RTC::mark_abs_3d(const int32_t X, const int32_t Y, const int32_t Z)
{
    timed_mark_abs_3d(X, Y, Z, 0.0);
}

void RTC::mark_abs(const int32_t X, const int32_t Y)
{
    timed_mark_abs(X, Y, 0.0);
}

void RTC::timed_mark_rel_3d(const int32_t dX, const int32_t dY, const int32_t dZ, const double T)
{
    exec_cmd({ R_LC_MARK_XYZT_REL, INT_PARA(dX), INT_PARA(dY), INT_PARA(dZ), DOUBLE_PARA(T) });
}

void RTC::timed_mark_rel(const int32_t dX, const int32_t dY, const double T)
{
    timed_mark_rel_3d(dX, dY, 0, T);
}

void RTC::mark_rel_3d(const int32_t dX, const int32_t dY, const int32_t dZ)
{
    timed_mark_rel_3d(dX, dY, dZ, 0.0);
}

void RTC::mark_rel(const int32_t dX, const int32_t dY)
{
    timed_mark_rel(dX, dY, 0.0);
}

void RTC::timed_arc_abs(const int32_t X, const int32_t Y, const double Angle, const double T)
{
    exec_cmd({ R_LC_ARC_XYAT_ABS, INT_PARA(X), INT_PARA(Y), DOUBLE_PARA(Angle), DOUBLE_PARA(T) });
}

void RTC::arc_abs(const int32_t X, const int32_t Y, const double Angle)
{
    timed_arc_abs(X, Y, Angle, 0.0);
}

void RTC::timed_arc_rel(const int32_t dX, const int32_t dY, const double Angle, const double T)
{
    exec_cmd({ R_LC_ARC_XYAT_REL, INT_PARA(dX), INT_PARA(dY), DOUBLE_PARA(Angle), DOUBLE_PARA(T) });
}

void RTC::arc_rel(const int32_t dX, const int32_t dY, const double Angle)
{
    timed_arc_rel(dX, dY, Angle, 0.0);
}

void RTC::arc_abs_3d(const int32_t X, const int32_t Y, const int32_t Z, const double Angle)
{
    exec_cmd({ R_LC_ARC_XYZA_ABS, INT_PARA(X), INT_PARA(Y), INT_PARA(Z), DOUBLE_PARA(Angle) });
}

void RTC::arc_rel_3d(const int32_t dX, const int32_t dY, const int32_t dZ, const double Angle)
{
    exec_cmd({ R_LC_ARC_XYZA_REL, INT_PARA(dX), INT_PARA(dY), INT_PARA(dZ), DOUBLE_PARA(Angle) });
}

void RTC::set_ellipse(const uint32_t A, const uint32_t B, const double Phi0, const double Phi)
{
    exec_cmd({ R_LC_SET_ELLIPSE, A, B, DOUBLE_PARA(Phi0), DOUBLE_PARA(Phi) });
}

void RTC::mark_ellipse_abs(const int32_t X, const int32_t Y, const double Alpha)
{
    exec_cmd({ R_LC_ELLIPSE_ABS, INT_PARA(X), INT_PARA(Y), DOUBLE_PARA(Alpha) });
}

void RTC::mark_ellipse_rel(const int32_t dX, const int32_t dY, const double Alpha)
{
    exec_cmd({ R_LC_ELLIPSE_REL, INT_PARA(dX), INT_PARA(dY), DOUBLE_PARA(Alpha) });
}

void RTC::jump_abs(const int32_t X, const int32_t Y)
{
    exec_cmd({ R_LC_JUMP_XY_ABS, INT_PARA(X), INT_PARA(Y) });
}

void RTC::jump_rel(const int32_t dX, const int32_t dY)
{
    exec_cmd({ R_LC_JUMP_XY_REL, INT_PARA(dX), INT_PARA(dY) });
}

void RTC::timed_jump_abs_3d(const int32_t X, const int32_t Y, const int32_t Z, const double T)
{
    exec_cmd({ R_LC_JUMP_XYZT_ABS, INT_PARA(X), INT_PARA(Y), INT_PARA(Z), DOUBLE_PARA(T) });
}

void RTC::timed_jump_abs(const int32_t X, const int32_t Y, const double T)
{
    timed_jump_abs_3d(X, Y, NO_Z_MOVE, T);
}

void RTC::jump_abs_3d(const int32_t X, const int32_t Y, const int32_t Z)
{
    timed_jump_abs_3d(X, Y, Z, 0.0);
}

void RTC::timed_jump_rel_3d(const int32_t dX, const int32_t dY, const int32_t dZ, const double T)
{
    exec_cmd({ R_LC_JUMP_XYZT_REL, INT_PARA(dX), INT_PARA(dY), INT_PARA(dZ), DOUBLE_PARA(T) });
}

void RTC::timed_jump_rel(const int32_t dX, const int32_t dY, const double T)
{
    timed_jump_rel_3d(dX, dY, 0, T);
}

void RTC::jump_rel_3d(const int32_t dX, const int32_t dY, const int32_t dZ)
{
    timed_jump_rel_3d(dX, dY, dZ, 0.0);
}

void RTC::jump_abs_drill(const int32_t X, const int32_t Y, const uint32_t DrillTime)
{
    jump_abs_drill_2(X, Y, DrillTime, 0, 0);
}

void RTC::jump_abs_drill_2(const int32_t X, const int32_t Y, const uint32_t DrillTime, const int32_t XOff, const int32_t YOff)
{
    exec_cmd({ R_LC_JUMP_XY_DRILL_ABS, INT_PARA(X), INT_PARA(Y), DrillTime, INT_PARA(XOff), INT_PARA(YOff) });
}

void RTC::jump_rel_drill(const int32_t dX, const int32_t dY, const uint32_t DrillTime)
{
    jump_rel_drill_2(dX, dY, DrillTime, 0, 0);
}

void RTC::jump_rel_drill_2(const int32_t dX, const int32_t dY, const uint32_t DrillTime, const int32_t XOff, const int32_t YOff)
{
    exec_cmd({ R_LC_JUMP_XY_DRILL_REL, INT_PARA(dX), INT_PARA(dY), DrillTime, INT_PARA(XOff), INT_PARA(YOff) });
}

void RTC::set_vector_control(const uint32_t Ctrl, const uint32_t Value)
{
    exec_cmd({ R_LC_SET_VECTOR_CONTROL, Ctrl, Value });
}

void RTC::para_laser_on_pulses_list(const uint32_t Period, const uint32_t Pulses, const uint32_t P)
{
    exec_cmd({ R_LC_LASER_ON_PULSES_PARA, Period, Pulses, P });
}

void RTC::laser_on_pulses_list(const uint32_t Period, const uint32_t Pulses)
{
    para_laser_on_pulses_list(Period, Pulses, 0xFFFF'FFFF);
}

void RTC::laser_on_list(const uint32_t Period)
{
    laser_on_pulses_list(Period, 0xFFFF'FFFF);
}

void RTC::para_mark_abs_3d(const int32_t X, const int32_t Y, const int32_t Z, const uint32_t P)
{
    exec_cmd({ R_LC_MARK_XYZP_ABS, INT_PARA(X), INT_PARA(Y), INT_PARA(Z), P });
}

void RTC::para_mark_abs(const int32_t X, const int32_t Y, const uint32_t P)
{
    para_mark_abs_3d(X, Y, NO_Z_MOVE, P);
}

void RTC::para_mark_rel_3d(const int32_t dX, const int32_t dY, const int32_t dZ, const uint32_t P)
{
    exec_cmd({ R_LC_MARK_XYZP_REL, INT_PARA(dX), INT_PARA(dY), INT_PARA(dZ), P });
}

void RTC::para_mark_rel(const int32_t dX, const int32_t dY, const uint32_t P)
{
    para_mark_rel_3d(dX, dY, 0, P);
}

void RTC::timed_para_mark_abs_3d(const int32_t X, const int32_t Y, const int32_t Z, const uint32_t P, const double T)
{
    exec_cmd({ R_LC_MARK_XYZPT_ABS, INT_PARA(X), INT_PARA(Y), INT_PARA(Z), P, DOUBLE_PARA(T) });
}

void RTC::timed_para_mark_abs(const int32_t X, const int32_t Y, const uint32_t P, const double T)
{
    timed_para_mark_abs_3d(X, Y, NO_Z_MOVE, P, T);
}

void RTC::timed_para_mark_rel_3d(const int32_t dX, const int32_t dY, const int32_t dZ, const uint32_t P, const double T)
{
    exec_cmd({ R_LC_MARK_XYZPT_REL, INT_PARA(dX), INT_PARA(dY), INT_PARA(dZ), P, DOUBLE_PARA(T) });
}

void RTC::timed_para_mark_rel(const int32_t dX, const int32_t dY, const uint32_t P, const double T)
{
    timed_para_mark_rel_3d(dX, dY, 0, P, T);
}

void RTC::para_jump_abs_3d(const int32_t X, const int32_t Y, const int32_t Z, const uint32_t P)
{
    exec_cmd({ R_LC_JUMP_XYZP_ABS, INT_PARA(X), INT_PARA(Y), INT_PARA(Z), P });
}

void RTC::para_jump_abs(const int32_t X, const int32_t Y, const uint32_t P)
{
    para_jump_abs_3d(X, Y, NO_Z_MOVE, P);
}

void RTC::para_jump_rel_3d(const int32_t dX, const int32_t dY, const int32_t dZ, const uint32_t P)
{
    exec_cmd({ R_LC_JUMP_XYZP_REL, INT_PARA(dX), INT_PARA(dY), INT_PARA(dZ), P });
}

void RTC::para_jump_rel(const int32_t dX, const int32_t dY, const uint32_t P)
{
    para_jump_rel_3d(dX, dY, 0, P);
}

void RTC::timed_para_jump_abs_3d(const int32_t X, const int32_t Y, const int32_t Z, const uint32_t P, const double T)
{
    exec_cmd({ R_LC_JUMP_XYZPT_ABS, INT_PARA(X), INT_PARA(Y), INT_PARA(Z), P, DOUBLE_PARA(T) });
}

void RTC::timed_para_jump_abs(const int32_t X, const int32_t Y, const uint32_t P, const double T)
{
    timed_para_jump_abs_3d(X, Y, NO_Z_MOVE, P, T);
}

void RTC::timed_para_jump_rel_3d(const int32_t dX, const int32_t dY, const int32_t dZ, const uint32_t P, const double T)
{
    exec_cmd({ R_LC_JUMP_XYZPT_REL, INT_PARA(dX), INT_PARA(dY), INT_PARA(dZ), P, DOUBLE_PARA(T) });
}

void RTC::timed_para_jump_rel(const int32_t dX, const int32_t dY, const uint32_t P, const double T)
{
    timed_para_jump_rel_3d(dX, dY, 0, P, T);
}

void RTC::micro_vector_abs_3d(const int32_t X, const int32_t Y, const int32_t Z, const int32_t LasOn, const int32_t LasOff)
{
    exec_cmd({ R_LC_MICRO_VECTOR_ABS_3D, INT_PARA(X), INT_PARA(Y), INT_PARA(Z), INT_PARA(LasOn), INT_PARA(LasOff) });
}

void RTC::micro_vector_abs(const int32_t X, const int32_t Y, const int32_t LasOn, const int32_t LasOff)
{
    micro_vector_abs_3d(X, Y, NO_Z_MOVE, LasOn, LasOff);
}

void RTC::micro_vector_rel_3d(const int32_t dX, const int32_t dY, const int32_t dZ, const int32_t LasOn, const int32_t LasOff)
{
    exec_cmd({ R_LC_MICRO_VECTOR_REL_3D, INT_PARA(dX), INT_PARA(dY), INT_PARA(dZ), INT_PARA(LasOn), INT_PARA(LasOff) });
}

void RTC::micro_vector_rel(const int32_t dX, const int32_t dY, const int32_t LasOn, const int32_t LasOff)
{
    micro_vector_rel_3d(dX, dY, 0, LasOn, LasOff);
}

void RTC::mark_char_abs(const uint32_t Char)
{
    exec_cmd({ R_LC_MARK_CHAR, Char, 1 });
}

void RTC::mark_char(const uint32_t Char)
{
    exec_cmd({ R_LC_MARK_CHAR, Char, 0 });
}

void RTC::select_char_set(const uint32_t No)
{
    exec_cmd({ R_LC_SELECT_CHAR_SET, No });
}

void RTC::mark_text_abs(const char* Text)
{
    constexpr size_t maxLen = TGM_MAX_SIZE - sizeof(TGM_HEADER) - (2 * sizeof(uint32_t)) - 1;
    const std::string strText(Text);

    for (size_t i = 0; i < strText.length(); i += maxLen)
    {
        std::vector<uint32_t> buffer{ R_LC_MARK_TEXT };
        append_string(buffer, strText.substr(i, std::min(maxLen, strText.length() - i)).c_str());
        buffer.push_back(1); // abs call
        exec_cmd(buffer);
    }
}

void RTC::mark_text(const char* Text)
{
    constexpr size_t maxLen = TGM_MAX_SIZE - sizeof(TGM_HEADER) - (2 * sizeof(uint32_t)) - 1;
    const std::string strText(Text);

    for (size_t i = 0; i < strText.length(); i += maxLen)
    {
        std::vector<uint32_t> buffer{ R_LC_MARK_TEXT };
        append_string(buffer, strText.substr(i, std::min(maxLen, strText.length() - i)).c_str());
        buffer.push_back(0); // non abs call
        exec_cmd(buffer);
    }
}

void RTC::mark_serial_abs(const uint32_t Mode, const uint32_t Digits)
{
    exec_cmd({ R_LC_MARK_SERIAL, Mode, Digits, 1 });
}

void RTC::mark_serial(const uint32_t Mode, const uint32_t Digits)
{
    exec_cmd({ R_LC_MARK_SERIAL, Mode, Digits, 0 });
}

void RTC::set_serial_step_list(const uint32_t No, const uint32_t Step)
{
    exec_cmd({ R_LC_SET_SERIAL, No, Step });
}

void RTC::select_serial_set_list(const uint32_t No)
{
    exec_cmd({ R_LC_SELECT_SERIAL_SET, No });
}

void RTC::time_fix_f_off(const uint32_t FirstDay, const uint32_t Offset)
{
    exec_cmd({ R_LC_TIME_FIX_F_OFF, FirstDay, Offset });
}

void RTC::time_fix_f(const uint32_t FirstDay)
{
    time_fix_f_off(FirstDay, 0);
}

void RTC::time_fix()
{
    time_fix_f(0);
}

void RTC::mark_date_abs(const uint32_t Part, const uint32_t Mode)
{
    exec_cmd({ R_LC_MARK_DATE, Part, Mode, 1 });
}

void RTC::mark_date(const uint32_t Part, const uint32_t Mode)
{
    exec_cmd({ R_LC_MARK_DATE, Part, Mode, 0 });
}

void RTC::mark_time_abs(const uint32_t Part, const uint32_t Mode)
{
    exec_cmd({ R_LC_MARK_TIME, Part, Mode, 1 });
}

void RTC::mark_time(const uint32_t Part, const uint32_t Mode)
{
    exec_cmd({ R_LC_MARK_TIME, Part, Mode, 0 });
}

void RTC::set_wobbel_mode_phase(const uint32_t Transversal, const uint32_t Longitudinal, const double Freq, const int32_t Mode, const double Phase)
{
    exec_cmd({ R_LC_SET_WOBBEL_MODE_PHASE, Transversal, Longitudinal, DOUBLE_PARA(Freq), INT_PARA(Mode), DOUBLE_PARA(Phase) });
}

void RTC::set_wobbel_mode(const uint32_t Transversal, const uint32_t Longitudinal, const double Freq, const int32_t Mode)
{
    exec_cmd({ R_LC_SET_WOBBEL_MODE, Transversal, Longitudinal, DOUBLE_PARA(Freq), INT_PARA(Mode) });
}

void RTC::set_wobbel(const uint32_t Transversal, const uint32_t Longitudinal, const double Freq)
{
    set_wobbel_mode(Transversal, Longitudinal, Freq, 0);
}

void RTC::set_wobbel_direction(const int32_t dX, const int32_t dY)
{
    exec_cmd({ R_LC_SET_WOBBEL_DIR, INT_PARA(dX), INT_PARA(dY) });
}

void RTC::set_wobbel_control(const uint32_t Ctrl, const uint32_t Value, const uint32_t MinValue, const uint32_t MaxValue)
{
    exec_cmd({ R_LC_SET_WOBBEL_CONTROL, Ctrl, Value, MinValue, MaxValue });
}

void RTC::set_wobbel_vector(const double dTrans, const double dLong, const uint32_t Period, const double dPower)
{
    exec_cmd({ R_LC_SET_WOBBEL_VECTOR, DOUBLE_PARA(dTrans), DOUBLE_PARA(dLong), Period, DOUBLE_PARA(dPower) });
}

void RTC::set_wobbel_vector2(const double dTrans, const double dLong, const uint32_t Period, const double dPower, const double dPower2, const uint32_t Ctrl)
{
    exec_cmd({ R_LC_SET_WOBBEL_VECTOR_2, DOUBLE_PARA(dTrans), DOUBLE_PARA(dLong), Period, DOUBLE_PARA(dPower), DOUBLE_PARA(dPower2), Ctrl });
}

void RTC::set_wobbel_offset(const int32_t OffsetTrans, const int32_t OffsetLong)
{
    exec_cmd({ R_LC_SET_WOBBEL_OFFSET, INT_PARA(OffsetTrans), INT_PARA(OffsetLong) });
}

void RTC::set_sky_writing_para_list(const double Timelag, const int32_t LaserOnShift, const uint32_t Nprev, const uint32_t Npost)
{
    exec_cmd({ R_LC_SET_SKY_WRITING_PARA, DOUBLE_PARA(Timelag), INT_PARA(LaserOnShift), Nprev, Npost });
}

void RTC::set_sky_writing_list(const double Timelag, const int32_t LaserOnShift)
{
    set_sky_writing_para_list(Timelag, LaserOnShift, 0xFFFF, 0xFFFF);
}

void RTC::set_sky_writing_limit_list(const double Limit)
{
    exec_cmd({ R_LC_SET_SKY_WRITING_ANGLE, DOUBLE_PARA(Limit) });
}

void RTC::set_sky_writing_mode_list(const uint32_t Mode)
{
    exec_cmd({ R_LC_SET_SKY_WRITING_MODE, Mode });
}

void RTC::set_control_mode_list(const uint32_t Mode)
{
    exec_cmd({ R_LC_SET_CONTROL_MODE, Mode });
}

void RTC::store_encoder(const uint32_t Pos)
{
    exec_cmd({ R_LC_STORE_ENCODER, Pos });
}

void RTC::save_and_restart_timer()
{
    exec_cmd({ R_LC_START_TIMER });
}

void RTC::store_timestamp_counter_list()
{
    exec_cmd({ R_LC_STORE_TIMESTAMP });
}

void RTC::wait_for_timestamp_counter_mode(const uint32_t Counter, const uint32_t Mode)
{
    exec_cmd({ R_LC_WAIT_FOR_TIMESTAMP, Counter, Mode });
}

void RTC::wait_for_timestamp_conter(const uint32_t Counter)
{
    wait_for_timestamp_counter_mode(Counter, 3);
}

void RTC::set_trigger(const uint32_t Period, const uint32_t Signal1, const uint32_t Signal2)
{
    set_trigger4(Period, Signal1, Signal2, -1, -1);
}

void RTC::set_trigger4(const uint32_t Period, const uint32_t Signal1, const uint32_t Signal2, const uint32_t Signal3, const uint32_t Signal4)
{
    exec_cmd({ R_LC_SET_TRIGGER, Period, Signal1, Signal2, Signal3, Signal4 });
}

void RTC::set_trigger8(const uint32_t Period, const uint32_t Signal1, const uint32_t Signal2, const uint32_t Signal3, const uint32_t Signal4,
                       const uint32_t Signal5, const uint32_t Signal6, const uint32_t Signal7, const uint32_t Signal8)
{
    exec_cmd({ R_LC_SET_TRIGGER_8, Period, Signal1, Signal2, Signal3, Signal4, Signal5, Signal6, Signal7, Signal8 });
}

void RTC::set_ext_start_delay_list(const int32_t Delay, const uint32_t EncoderNo)
{
    exec_cmd({ R_LC_EXTERN_START_DELAY, INT_PARA(Delay), EncoderNo });
}

void RTC::set_fly_x(const double Scale)
{
    exec_cmd({ R_LC_SET_FLY, DOUBLE_PARA(Scale), 1, 0 });
}

void RTC::set_fly_y(const double Scale)
{
    exec_cmd({ R_LC_SET_FLY, DOUBLE_PARA(Scale), 2, 0 });
}

void RTC::set_fly_z(const double Scale, const uint32_t No)
{
    exec_cmd({ R_LC_SET_FLY, DOUBLE_PARA(Scale), 3, No });
}

void RTC::set_fly_rot(const double Resolution)
{
    exec_cmd({ R_LC_SET_FLY, DOUBLE_PARA(Resolution), 4, 0 });
}

void RTC::set_rot_center_list(const int32_t X, const int32_t Y)
{
    exec_cmd({ R_LC_ROT_CENTER, INT_PARA(X), INT_PARA(Y) });
}

void RTC::set_fly_2d(const double ScaleX, const double ScaleY)
{
    exec_cmd({ R_LC_SET_FLY_2D, DOUBLE_PARA(ScaleX), DOUBLE_PARA(ScaleY) });
}

void RTC::fly_return_z(const int32_t X, const int32_t Y, const int32_t Z)
{
    exec_cmd({ R_LC_FLY_RETURN_Z, INT_PARA(X), INT_PARA(Y), INT_PARA(Z) });
}

void RTC::fly_return(const int32_t X, const int32_t Y)
{
    fly_return_z(X, Y, NO_Z_MOVE);
}

void RTC::set_fly_limits(const int32_t Xmin, const int32_t Xmax, const int32_t Ymin, const int32_t Ymax)
{
    exec_cmd({ R_LC_SET_FLY_LIMITS, INT_PARA(Xmin), INT_PARA(Xmax), INT_PARA(Ymin), INT_PARA(Ymax) });
}

void RTC::set_fly_limits_z(const int32_t Zmin, const int32_t Zmax)
{
    exec_cmd({ R_LC_SET_FLY_LIMITS_Z, INT_PARA(Zmin), INT_PARA(Zmax) });
}

void RTC::if_fly_x_overflow(const int32_t Mode)
{
    exec_cmd({ R_LC_IF_FLY_OVERFLOW, INT_PARA(Mode), 1 });
}

void RTC::if_fly_y_overflow(const int32_t Mode)
{
    exec_cmd({ R_LC_IF_FLY_OVERFLOW, INT_PARA(Mode), 2 });
}

void RTC::if_fly_z_overflow(const int32_t Mode)
{
    exec_cmd({ R_LC_IF_FLY_OVERFLOW, INT_PARA(Mode), 3 });
}

void RTC::if_not_fly_x_overflow(const int32_t Mode)
{
    exec_cmd({ R_LC_IF_NOT_FLY_OVERFLOW, INT_PARA(Mode), 1 });
}

void RTC::if_not_fly_y_overflow(const int32_t Mode)
{
    exec_cmd({ R_LC_IF_NOT_FLY_OVERFLOW, INT_PARA(Mode), 2 });
}

void RTC::if_not_fly_z_overflow(const int32_t Mode)
{
    exec_cmd({ R_LC_IF_NOT_FLY_OVERFLOW, INT_PARA(Mode), 3 });
}

void RTC::clear_fly_overflow(const uint32_t Mode)
{
    exec_cmd({ R_LC_CLEAR_FLY_OVERFLOW, Mode });
}

void RTC::park_position(const uint32_t Mode, const int32_t X, const int32_t Y)
{
    exec_cmd({ R_LC_PARK_POSITION, Mode, INT_PARA(X), INT_PARA(Y) });
}

void RTC::park_return(const uint32_t Mode, const int32_t X, const int32_t Y)
{
    exec_cmd({ R_LC_PARK_RETURN, Mode, INT_PARA(X), INT_PARA(Y) });
}

void RTC::activate_fly_xy_encoder(const double ScaleX, const double ScaleY, const int32_t EncX, const int32_t EncY)
{
    exec_cmd({ R_LC_ACTIVATE_FLY_ENC_XY, DOUBLE_PARA(ScaleX), DOUBLE_PARA(ScaleY), INT_PARA(EncX), INT_PARA(EncY) });
}

void RTC::activate_fly_xy(const double ScaleX, const double ScaleY)
{
    activate_fly_xy_encoder(ScaleX, ScaleY, 0, 0);
}

void RTC::activate_fly_2d_encoder(const double ScaleX, const double ScaleY, const int32_t EncX, const int32_t EncY)
{
    exec_cmd({ R_LC_ACTIVATE_FLY_ENC_2D, DOUBLE_PARA(ScaleX), DOUBLE_PARA(ScaleY), INT_PARA(EncX), INT_PARA(EncY) });
}

void RTC::activate_fly_2d(const double ScaleX, const double ScaleY)
{
    activate_fly_2d_encoder(ScaleX, ScaleY, 0, 0);
}

void RTC::wait_for_encoder_mode(const int32_t Value, const uint32_t Encoder, const uint32_t Mode)
{
    exec_cmd({ R_LC_WAIT_FOR_ENCODER_MODE, INT_PARA(Value), Encoder, Mode });
}

void RTC::wait_for_encoder(const int32_t Value, const uint32_t Encoder)
{
    wait_for_encoder_mode(Value, Encoder, 0);
}

void RTC::wait_for_encoder_in_range_mode(const int32_t EncXmin, const int32_t EncXmax, const int32_t EncYmin, const int32_t EncYmax, const uint32_t Mode)
{
    exec_cmd({ R_LC_WAIT_FOR_ENCODER_RANGE, INT_PARA(EncXmin), INT_PARA(EncXmax), INT_PARA(EncYmin), INT_PARA(EncYmax), Mode });
}

void RTC::wait_for_encoder_in_range(const int32_t EncXmin, const int32_t EncXmax, const int32_t EncYmin, const int32_t EncYmax)
{
    wait_for_encoder_in_range_mode(EncXmin, EncXmax, EncYmin, EncYmax, 0);
}

void RTC::if_not_activated()
{
    exec_cmd({ R_LC_IF_NOT_ACTIVATED });
}

void RTC::range_checking(const uint32_t Head, const uint32_t Mode, const uint32_t Data)
{
    exec_cmd({ R_LC_RANGE_CHECKING, Head, Mode, Data });
}

void RTC::set_fly_x_pos(const double Scale)
{
    exec_cmd({ R_LC_SET_FLY_POS, DOUBLE_PARA(Scale), 1 });
}

void RTC::set_fly_y_pos(const double Scale)
{
    exec_cmd({ R_LC_SET_FLY_POS, DOUBLE_PARA(Scale), 2 });
}

void RTC::set_fly_rot_pos(const double Resolution)
{
    exec_cmd({ R_LC_SET_FLY_POS, DOUBLE_PARA(Resolution), 4 });
}

void RTC::wait_for_mcbsp(const uint32_t Axis, const int32_t Value, const int32_t Mode)
{
    exec_cmd({ R_LC_WAIT_FOR_MCBSP, Axis, INT_PARA(Value), INT_PARA(Mode) });
}

void RTC::set_mcbsp_in_list(const uint32_t Mode, const double Scale)
{
    exec_cmd({ R_LC_SET_MCBSP_IN, Mode, DOUBLE_PARA(Scale) });
}

void RTC::set_multi_mcbsp_in_list(const uint32_t Ctrl, const uint32_t P, const uint32_t Mode)
{
    exec_cmd({ R_LC_SET_MULTI_MCBSP_IN, Ctrl, P, Mode });
}

void RTC::set_mcbsp_x_list(const double Scale)
{
    exec_cmd({ R_LC_SET_MCBSP_MODE, 1, DOUBLE_PARA(Scale) });
}

void RTC::set_mcbsp_y_list(const double Scale)
{
    exec_cmd({ R_LC_SET_MCBSP_MODE, 2, DOUBLE_PARA(Scale) });
}

void RTC::set_mcbsp_rot_list(const double Resolution)
{
    exec_cmd({ R_LC_SET_MCBSP_MODE, 4, DOUBLE_PARA(Resolution) });
}

void RTC::set_mcbsp_matrix_list()
{
    exec_cmd({ R_LC_SET_MCBSP_MODE, 0, DOUBLE_PARA(0.0) });
}

void RTC::apply_mcbsp_list(const uint32_t HeadNo, const uint32_t at_once)
{
    exec_cmd({ R_LC_APPLY_MCBSP, HeadNo, at_once });
}

void RTC::set_mcbsp_global_x_list(const double Scale)
{
    exec_cmd({ R_LC_SET_MCBSP_MODE_GLOBAL, 1, DOUBLE_PARA(Scale) });
}

void RTC::set_mcbsp_global_y_list(const double Scale)
{
    exec_cmd({ R_LC_SET_MCBSP_MODE_GLOBAL, 2, DOUBLE_PARA(Scale) });
}

void RTC::set_mcbsp_global_rot_list(const double Resolution)
{
    exec_cmd({ R_LC_SET_MCBSP_MODE_GLOBAL, 4, DOUBLE_PARA(Resolution) });
}

void RTC::set_mcbsp_global_matrix_list()
{
    exec_cmd({ R_LC_SET_MCBSP_MODE_GLOBAL, 0, DOUBLE_PARA(0.0) });
}

void RTC::set_fly_1_axis(const uint32_t Axis, const uint32_t Mode, const double Scale)
{
    exec_cmd({ R_LC_SET_FLY_1, Axis, Mode, DOUBLE_PARA(Scale) });
}

void RTC::fly_return_1_axis(const uint32_t Axis, const int32_t RetPos)
{
    exec_cmd({ R_LC_FLY_RETURN_1, Axis, INT_PARA(RetPos) });
}

void RTC::wait_for_1_axis(const int32_t Value, const uint32_t Mode, const int32_t WaitMode, const uint32_t LaserMode)
{
    exec_cmd({ R_LC_WAIT_FOR_1, INT_PARA(Value), Mode, INT_PARA(WaitMode), LaserMode });
}

void RTC::activate_fly_1_axis(const uint32_t Axis, const uint32_t Mode, const double Scale, const int32_t Offset)
{
    exec_cmd({ R_LC_ACTIVATE_FLY_1, Axis, Mode, DOUBLE_PARA(Scale), INT_PARA(Offset) });
}

void RTC::park_position_1_axis(const uint32_t Mode, const uint32_t Axis, const int32_t ParkPos)
{
    exec_cmd({ R_LC_PARK_POSITION_1, Mode, Axis, INT_PARA(ParkPos) });
}

void RTC::park_return_1_axis(const uint32_t Mode, const uint32_t Axis, const int32_t RetPos)
{
    exec_cmd({ R_LC_PARK_RETURN_1, Mode, Axis, INT_PARA(RetPos) });
}

void RTC::set_fly_2_axes(const uint32_t Axis1, const uint32_t Mode1, const double Scale1, const uint32_t Axis2, const uint32_t Mode2, const double Scale2)
{
    exec_cmd({ R_LC_SET_FLY_2, Axis1, Mode1, DOUBLE_PARA(Scale1), Axis2, Mode2, DOUBLE_PARA(Scale2) });
}

void RTC::fly_return_2_axes(const uint32_t Axis1, const int32_t RetPos1, const uint32_t Axis2, const int32_t RetPos2)
{
    exec_cmd({ R_LC_FLY_RETURN_2, Axis1, INT_PARA(RetPos1), Axis2, INT_PARA(RetPos2) });
}

void RTC::wait_for_2_axes(const uint32_t ModeX, const int32_t MinValueX, const int32_t MaxValueX,
                          const uint32_t ModeY, const int32_t MinValueY, const int32_t MaxValueY, const int32_t WaitMode, const uint32_t LaserMode)
{
    exec_cmd({ R_LC_WAIT_FOR_2, ModeX, INT_PARA(MinValueX), INT_PARA(MaxValueX), ModeY, INT_PARA(MinValueY), INT_PARA(MaxValueY), INT_PARA(WaitMode), LaserMode });
}

void RTC::activate_fly_2_axes(const uint32_t ModeX, const double ScaleX, const int32_t OffsetX,
                              const uint32_t ModeY, const double ScaleY, const int32_t OffsetY)
{
    exec_cmd({ R_LC_ACTIVATE_FLY_2, ModeX, DOUBLE_PARA(ScaleX), INT_PARA(OffsetX), ModeY, DOUBLE_PARA(ScaleY), INT_PARA(OffsetY) });
}

void RTC::park_position_2_axes(const uint32_t Mode, const int32_t ParkPosX, const int32_t ParkPosY)
{
    exec_cmd({ R_LC_PARK_POSITION_2, Mode, INT_PARA(ParkPosX), INT_PARA(ParkPosY) });
}

void RTC::park_return_2_axes(const uint32_t Mode, const int32_t RetPosX, const int32_t RetPosY)
{
    exec_cmd({ R_LC_PARK_RETURN_2, Mode, INT_PARA(RetPosX), INT_PARA(RetPosY) });
}

void RTC::set_fly_3_axes(const uint32_t ModeX, const double ScaleX, const uint32_t ModeY, const double ScaleY, const uint32_t ModeZ, const double ScaleZ)
{
    exec_cmd({ R_LC_SET_FLY_3, ModeX, DOUBLE_PARA(ScaleX), ModeY, DOUBLE_PARA(ScaleY), ModeZ, DOUBLE_PARA(ScaleZ) });
}

void RTC::fly_return_3_axes(const int32_t RetPosX, const int32_t RetPosY, const int32_t RetPosZ)
{
    exec_cmd({ R_LC_FLY_RETURN_3, INT_PARA(RetPosX), INT_PARA(RetPosY), INT_PARA(RetPosZ) });
}

void RTC::rs232_write_text_list(const char* Text)
{
    constexpr size_t maxLen = TGM_MAX_SIZE - sizeof(TGM_HEADER) - sizeof(uint32_t) - 1;
    const std::string strText(Text);

    for (size_t i = 0; i < strText.length(); i += maxLen)
    {
        std::vector<uint32_t> buffer{ R_LC_RS232_WRITE_TEXT };
        append_string(buffer, strText.substr(i, std::min(maxLen, strText.length() - i)).c_str());
        exec_cmd(buffer);
    }
}

void RTC::set_mcbsp_out(const uint32_t Signal1, const uint32_t Signal2)
{
    exec_cmd({ R_LC_SET_MCBSP_OUT, Signal1, Signal2 });
}

void RTC::periodic_toggle_list(const uint32_t Port, const uint32_t Mask, const uint32_t P1, const uint32_t P2, const uint32_t Count, const uint32_t Stop)
{
    exec_cmd({ R_LC_PERIODIC_TOGGLE, Port, Mask, P1, P2, Count, Stop });
}

void RTC::set_free_variable_list(const uint32_t VarNo, const uint32_t Value)
{
    exec_cmd({ R_LC_SET_FREE_VAR, VarNo, Value });
}

void RTC::select_cor_table_list(const uint32_t HeadA, const uint32_t HeadB)
{
    exec_cmd({ R_LC_SELECT_COR_TABLE, HeadA, HeadB });
}

void RTC::set_delay_mode_list(const uint32_t VarPoly, const uint32_t DirectMove3D, const uint32_t EdgeLevel, const uint32_t MinJumpDelay, const uint32_t JumpLengthLimit)
{
    exec_cmd({ R_LC_SET_DELAY_MODE, VarPoly, DirectMove3D, EdgeLevel, MinJumpDelay, JumpLengthLimit });
}

void RTC::activate_scanahead_autodelays_list(const int32_t Mode)
{
    exec_cmd({ R_LC_SCANAHEAD_ACTIVATE_AUTO, INT_PARA(Mode) });
}

void RTC::set_scanahead_laser_shifts_list(const int32_t dLasOn, const int32_t dLasOff)
{
    exec_cmd({ R_LC_SET_SCANAHEAD_LASER_SHIFT, INT_PARA(dLasOn), INT_PARA(dLasOff) });
}

void RTC::set_scanahead_line_params_ex_list(const uint32_t CornerScale, const uint32_t EndScale, const uint32_t AccScale, const uint32_t JumpScale)
{
    exec_cmd({ R_LC_SET_SCANAHEAD_LINE_PARAMS, CornerScale, EndScale, AccScale, JumpScale });
}

void RTC::set_scanahead_line_params_list(const uint32_t CornerScale, const uint32_t EndScale, const uint32_t AccScale)
{
    set_scanahead_line_params_ex_list(CornerScale, EndScale, AccScale, CornerScale);
}

void RTC::stepper_enable_list(const int32_t Enable1, const int32_t Enable2)
{
    exec_cmd({ R_LC_STEPPER_ENABLE, INT_PARA(Enable1), INT_PARA(Enable2) });
}

void RTC::stepper_control_list(const int32_t Period1, const int32_t Period2)
{
    exec_cmd({ R_LC_STEPPER_CTRL, INT_PARA(Period1), INT_PARA(Period2) });
}

void RTC::stepper_no_abs_list(const uint32_t No, const int32_t Pos)
{
    exec_cmd({ R_LC_STEPPER_NO_ABS, No, INT_PARA(Pos) });
}

void RTC::stepper_no_rel_list(const uint32_t No, const int32_t dPos)
{
    exec_cmd({ R_LC_STEPPER_NO_REL, No, INT_PARA(dPos) });
}

void RTC::stepper_abs_list(const int32_t Pos1, const int32_t Pos2)
{
    exec_cmd({ R_LC_STEPPER_ABS, INT_PARA(Pos1), INT_PARA(Pos2) });
}

void RTC::stepper_rel_list(const int32_t dPos1, const int32_t dPos2)
{
    exec_cmd({ R_LC_STEPPER_REL, INT_PARA(dPos1), INT_PARA(dPos2) });
}

void RTC::stepper_wait(const uint32_t No)
{
    exec_cmd({ R_LC_STEPPER_WAIT, No });
}

void RTC::camming(const uint32_t FirstPos, const uint32_t Npos, const uint32_t EncoderNo, const uint32_t Ctrl, const double Scale)
{
    exec_cmd({ R_LC_CAMMING, FirstPos, Npos, EncoderNo, Ctrl, DOUBLE_PARA(Scale) });
}

void RTC::set_jump_mode_list(const int32_t Flag)
{
    exec_cmd({ R_LC_SET_JUMP_MODE, INT_PARA(Flag) });
}

void RTC::enduring_wobbel()
{
    exec_cmd({ R_LC_ENDURING_WOBBEL });
}

void RTC::fly_prediction(const uint32_t PredictionX, const uint32_t PredictionY)
{
    exec_cmd({ R_LC_FLY_PREDICTION, PredictionX, PredictionY });
}

void RTC::wait_for_timestamp_counter_long(const uint64_t WaitCounter, const uint32_t MaxWaitTime, const uint32_t Mode)
{
    exec_cmd({ R_LC_WAIT_FOR_TIMESTAMP_LONG, UINT64_PARA(WaitCounter), MaxWaitTime, Mode });
}

void RTC::set_mcbsp_out_ptr_list(const uint32_t Num, const uintptr_t SignalPtr)
{
    if (!SignalPtr)
    {
        return;
    }

    const uint32_t* const pSig = reinterpret_cast<uint32_t*>(SignalPtr);
    std::vector<uint32_t> buf{ R_LC_SET_MCBSP_OUT_PTR, Num };
    buf.insert(buf.cend(), pSig, pSig + std::min(Num, 8u));
    exec_cmd(buf);
}

void RTC::set_mcbsp_out_oie_list(const uint32_t Signal1, const uint32_t Signal2)
{
    exec_cmd({ R_LC_SET_MCBSP_OUT_OIE, Signal1, Signal2 });
}

void RTC::write_port_list(const uint32_t Port, const uint32_t Value, const uint32_t NoDelay)
{
    exec_cmd({ R_LC_WRITE_PORT, Port, Value, NoDelay });
}

void RTC::fly_disable_list()
{
    exec_cmd({ R_LC_FLY_DISABLE });
}

void RTC::init_fly_2d_list(const int32_t OffsetX, const int32_t OffsetY, const uint32_t No)
{
    exec_cmd({ R_LC_INIT_FLY_2D, INT_PARA(OffsetX), INT_PARA(OffsetY), No });
}

void RTC::set_defocus_2_list(const int32_t Shift, const uint32_t Mode)
{
    exec_cmd({ R_LC_SET_DEFOCUS_2, INT_PARA(Shift), Mode });
}

void RTC::set_sky_writing_min_speed_list(const double Speed)
{
    exec_cmd({ R_LC_SET_SKY_WRITING_MIN_SPEED, DOUBLE_PARA(Speed) });
}

void RTC::set_sky_writing_scale_list(const double Scale)
{
    exec_cmd({ R_LC_SET_SKY_WRITING_SCALE, DOUBLE_PARA(Scale) });
}

void RTC::write_io_port_mask_ex_list(const uint32_t Value, const uint32_t Mask, const uint32_t NoDelay)
{
    exec_cmd({ R_LC_WRITE_IO_PORT_MASK_EX, Value, Mask, NoDelay });
}


// Internal

TGM_ANSW_RAW RTC::send_recv(const std::vector<uint32_t>& buffer)
{
    TGM_CMD_RAW tgm;
    tgm.header.version = TGM_VERSION;
    tgm.header.seqnum = seqnum++;
    tgm.header.type = TGM_TYPE::COMMAND;
    tgm.header.format = TGM_FORMAT::RAW;
    tgm.header.length = buffer.size() * sizeof(tgm.payload.buffer[0]);

    std::copy(buffer.begin(), buffer.end(), tgm.payload.buffer);

    return send_recv(tgm);
}

TGM_ANSW_RAW RTC::send_recv(const TGM_CMD_RAW& tgm)
{
    TGM_ANSW_RAW answer{};

    double timeout = timeout_initial_us;
    int32_t sel;
    do
    {
        // Original telegram or retry, will be executed only once by RTC if seqnum doesn't change
        network_error = net->send(reinterpret_cast<const char*>(&tgm), sizeof(TGM_HEADER) + tgm.header.length);
        if (network_error)
        {
            return answer;
        }

        sel = net->select((long)timeout);
        if (sel < 0)
        {
            network_error = sel;
            return answer;
        }
        else if (sel > 0)
        {
            const int32_t res = net->recv(reinterpret_cast<char*>(&answer), sizeof(TGM_ANSW_RAW));

            // Discard and try again (e.g. telegram was from the wrong card)
            if (res > 0)
            {
                continue;
            }

            if (res < 0)
            {
                network_error = res;
                return answer;
            }

            // Skip delayed retries from previous cmd
            if (answer.header.seqnum != tgm.header.seqnum)
            {
                continue;
            }

            return answer;
        }

        timeout *= timeout_multiplier;
    }
    while (timeout <= timeout_max_sum_us || sel > 0);

    // Timeout occured
    network_error = NETWORK_ERR_TIMEOUT;
    return answer;
}
}
