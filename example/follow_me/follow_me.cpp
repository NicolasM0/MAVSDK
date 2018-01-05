/**
* @file follow_me.cpp
* @brief Example that demonstrates the usage of Follow Me plugin.
* @author Shakthi Prashanth <shakthi.prashanth.m@intel.com>
* @date 2018-01-03
*/

#include <dronecore/dronecore.h>
#include <iostream>
#include <thread>
#include <chrono>
#include "fake_location_provider.h"

using namespace dronecore;
using namespace std::placeholders; // for `_1`
using namespace std::chrono; // for seconds(), milliseconds(), etc
using namespace std::this_thread;  // for sleep_for()

// For coloring output
#define ERROR_CONSOLE_TEXT "\033[31m" //Turn text on console red
#define TELEMETRY_CONSOLE_TEXT "\033[34m" //Turn text on console blue
#define NORMAL_CONSOLE_TEXT "\033[0m"  //Restore normal console colour

inline void action_error_exit(Action::Result result, const std::string &message);
inline void follow_me_error_exit(FollowMe::Result result, const std::string &message);
inline void connection_error_exit(DroneCore::ConnectionResult result, const std::string &message);

int main(int, char **)
{
    DroneCore dc;

    DroneCore::ConnectionResult conn_result = dc.add_udp_connection();
    connection_error_exit(conn_result, "Connection failed");

    // Wait for the device to connect via heartbeat
    while (!dc.is_connected()) {
        std::cout << "Wait for device to connect via heartbeat" << std::endl;
        sleep_for(seconds(1));
    }

    // Device got discovered.
    Device &device = dc.device();
    while (!device.telemetry().health_all_ok()) {
        std::cout << "Waiting for device to be ready" << std::endl;
        sleep_for(seconds(1));
    }
    std::cout << "Device is ready" << std::endl;

    // Arm
    Action::Result arm_result = device.action().arm();
    action_error_exit(arm_result, "Arming failed");
    std::cout << "Armed" << std::endl;

    // Subscribe to receive updates on flight mode. You can find out whether FollowMe is active.
    device.telemetry().flight_mode_async(
    std::bind([&](Telemetry::FlightMode flight_mode) {
        const FollowMe::TargetLocation last_location = device.follow_me().get_last_location();
        std::cout << "[FlightMode: " << Telemetry::flight_mode_str(flight_mode)
                  << "] Vehicle is at: " << last_location.latitude_deg << ", "
                  << last_location.longitude_deg << " degrees." << std::endl;
    }, std::placeholders::_1));

    // Takeoff
    Action::Result takeoff_result = device.action().takeoff();
    action_error_exit(takeoff_result, "Takeoff failed");
    std::cout << "In Air..." << std::endl;
    sleep_for(seconds(5));

    // Start Follow Me
    FollowMe::Result follow_me_result = device.follow_me().start();
    follow_me_error_exit(follow_me_result, "Failed to start FollowMe mode");

    boost::asio::io_context io; // for event loop
    // Register for platform-specific Location provider. We're using FakeLocationProvider for the example.
    (new FakeLocationProvider(io))->request_location_updates([&device](double lat, double lon) {
        device.follow_me().set_target_location({lat, lon, 0.0, 0.f, 0.f, 0.f});
    });
    io.run(); // will run as long as location updates continue to happen.

    // Stop Follow Me
    follow_me_result = device.follow_me().stop();
    follow_me_error_exit(follow_me_result, "Failed to stop FollowMe mode");

    // Land
    const Action::Result land_result = device.action().land();
    action_error_exit(land_result, "Landing failed");

    // We are relying on auto-disarming but let's keep watching the telemetry for a bit longer.
    sleep_for(seconds(5));
    std::cout << "Finished..." << std::endl;
    return 0;
}

// Handles Action's result
inline void action_error_exit(Action::Result result, const std::string &message)
{
    if (result != Action::Result::SUCCESS) {
        std::cerr << ERROR_CONSOLE_TEXT << message << Action::result_str(
                      result) << NORMAL_CONSOLE_TEXT << std::endl;
        exit(EXIT_FAILURE);
    }
}
// Handles FollowMe's result
inline void follow_me_error_exit(FollowMe::Result result, const std::string &message)
{
    if (result != FollowMe::Result::SUCCESS) {
        std::cerr << ERROR_CONSOLE_TEXT << message << FollowMe::result_str(
                      result) << NORMAL_CONSOLE_TEXT << std::endl;
        exit(EXIT_FAILURE);
    }
}
// Handles connection result
inline void connection_error_exit(DroneCore::ConnectionResult result, const std::string &message)
{
    if (result != DroneCore::ConnectionResult::SUCCESS) {
        std::cerr << ERROR_CONSOLE_TEXT << message
                  << DroneCore::connection_result_str(result)
                  << NORMAL_CONSOLE_TEXT << std::endl;
        exit(EXIT_FAILURE);
    }
}
