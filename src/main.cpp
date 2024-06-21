#include <iostream>
#include <cstring>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <csignal>
#include <syslog.h>
#include <string>
#include <mqtt/async_client.h>
#include <thread>
#include <mutex>
#include <atomic>
#include <queue>
#include <spdlog/spdlog.h>

using namespace std;

queue<struct can_frame> frames;
volatile sig_atomic_t stop = 0;

void handle_signal(int signal)
{
    if (signal == SIGINT)
    {
        stop = 1;
    }
}

class PublisherCallback : public virtual mqtt::callback
{
  public:
    void connection_lost(const std::string &cause) override
    {
        spdlog::debug(
            "{:s}::{:d} Connection lost: {:s}", __FILE__, __LINE__, cause);
    }

    void delivery_complete(mqtt::delivery_token_ptr
                           __attribute__((unused)) token) override
    {
        spdlog::debug("{:s}::{:d} Message delivered", __FILE__, __LINE__);
    }
};

class SubscriberCallback : public virtual mqtt::callback
{
  public:
    void connection_lost(const std::string &cause) override
    {
        spdlog::debug(
            "{:s}::{:d} Connection lost: {:s}", __FILE__, __LINE__, cause);
    }

    void message_arrived(mqtt::const_message_ptr message) override
    {
        spdlog::debug(
            "{:s}::{:d} {:s}", __FILE__, __LINE__, message->get_payload_str());
    }

    void delivery_complete(mqtt::delivery_token_ptr
                           __attribute__((unused)) token) override
    {
        spdlog::debug("{:s}::{:d} Message delivered", __FILE__, __LINE__);
    }
};

void canbus_thread_process(std::atomic<bool> &stop_flag)
{
    // Create a socket
    int s = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (s < 0)
    {
        syslog(LOG_ERR, "Socket");
        stop_flag.store(true);
    }

    // Specify can0 device
    struct ifreq ifr;
    strcpy(ifr.ifr_name, "vcan0");
    ioctl(s, SIOCGIFINDEX, &ifr);

    // Configure the CAN interface
    struct sockaddr_can addr;
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    // Bind the socket to the CAN interface
    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        syslog(LOG_ERR, "Bind");
        stop_flag.store(true);
    }

    // Read CAN frames
    while (!stop_flag.load())
    {
        struct can_frame frame;
        int nbytes = read(s, &frame, sizeof(struct can_frame));
        if (nbytes < 0)
        {
            syslog(LOG_ERR, "Read");
            stop_flag.store(true);
        }

        if (nbytes < sizeof(struct can_frame))
        {
            syslog(LOG_ERR, "Incomplete CAN frame");
            stop_flag.store(true);
        }

        frames.emplace(frame);
    }

    // Close the socket
    close(s);
    syslog(LOG_INFO, "Socket closed");
}

int main()
{
    std::signal(SIGINT, handle_signal);
    spdlog::set_level(spdlog::level::info);
    openlog(NULL, 0, LOG_USER);
    syslog(LOG_USER, "Running simcan_mqtt");
    std::atomic<bool> stop_flag(false);

    // const string DFLT_SERVER_ADDRESS{"tcp://localhost:1883"};
    const string DFLT_SERVER_ADDRESS{"tcp://test.mosquitto.org:1883"};
    const string CLIENT_ID{"TheConfubulator"};
    const string PERSIST_DIR{"./persist"};
    const std::string PUB_TOPIC("testpub/topic");
    const std::string SUB_TOPIC("testsub/topic");
    const int QOS = 1;
    const int TIMEOUT = 10000;

    mqtt::async_client client(DFLT_SERVER_ADDRESS, CLIENT_ID);
    mqtt::connect_options connOpts;
    connOpts.set_keep_alive_interval(20);
    connOpts.set_clean_session(true);

    std::thread canbus_thread(canbus_thread_process, std::ref(stop_flag));

    try
    {
        PublisherCallback p_callback;
        client.set_callback(p_callback);

        SubscriberCallback s_callback;
        client.set_callback(s_callback);

        mqtt::token_ptr connectionToken = client.connect(connOpts);
        connectionToken->wait();

        mqtt::token_ptr subToken = client.subscribe(SUB_TOPIC, QOS);
        subToken->wait();
        do
        {
            while (!frames.empty())
            {
                auto frame = frames.front();
                frames.pop();
                std::string message = fmt::format(
                    "\"value1\": {:d},\"value2\": {:d},\"value3\": "
                    "{:d},\"value4\": {:d},\"value5\": {:d},\"value6\": "
                    "{:d},\"value7\": {:d} ",
                    frame.data[0],
                    frame.data[1],
                    frame.data[2],
                    frame.data[3],
                    frame.data[4],
                    frame.data[5],
                    frame.data[6],
                    frame.data[7]);
                mqtt::message_ptr pubMessage =
                    mqtt::make_message(PUB_TOPIC, message, QOS, false);
                client.publish(pubMessage)->wait();
                // Print the CAN ID and data
                // std::cout << "CAN ID: " << std::hex << frame.can_id << " DLC:
                // " << std::dec << int(frame.can_dlc) << " Data: "; for (int i
                // = 0; i < frame.can_dlc; ++i)
                // {
                //     std::cout << std::hex << int(frame.data[i]) << " ";
                // }
                // std::cout << std::dec << std::endl;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } while (!stop);
    }
    catch (const std::exception &e)
    {
        syslog(LOG_ERR, "{:s}::{:d} {:s}", __FILE__, __LINE__, e.what());
    }

    mqtt::token_ptr disconnectionToken = client.disconnect();
    disconnectionToken->wait();
    stop_flag.store(true);
    canbus_thread.join();

    return 0;
}
