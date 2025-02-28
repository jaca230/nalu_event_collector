#ifndef NALU_UDP_RECEIVER_H
#define NALU_UDP_RECEIVER_H

#include <vector>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <netinet/in.h>
#include <functional>
#include <chrono>
#include "nalu_udp_data_buffer.h"
#include "nalu_event_collector_params.h"


class NaluUdpReceiver {
public:
    NaluUdpReceiver(const std::string& address, uint16_t port, size_t buffer_size = 1024*1024*100, size_t max_packet_size = 1040, int timeout_sec = 10);
    NaluUdpReceiver(const NaluUdpReceiverParams& params);
    ~NaluUdpReceiver();

    void start();
    void stop();

    // Getter and Setter for the NaluUdpDataBuffer
    NaluUdpDataBuffer& getDataBuffer();

private:
    void receiveLoop();
    void initSocket();

    std::string address_;
    uint16_t port_;
    int socket_fd_;
    std::thread receiver_thread_;
    std::atomic<bool> running_;

    NaluUdpDataBuffer nalu_udp_data_buffer_;  // Data buffer

    size_t max_packet_size_;
    int timeout_sec_;
};

#endif // NALU_UDP_RECEIVER_H
