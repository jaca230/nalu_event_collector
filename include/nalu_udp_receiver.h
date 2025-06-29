#ifndef NALU_UDP_RECEIVER_H
#define NALU_UDP_RECEIVER_H

#include <atomic>
#include <cstdint>
#include <string>
#include <thread>

#include "nalu_udp_data_buffer.h"
#include "nalu_event_collector_params.h"

/**
 * @class NaluUdpReceiver
 * @brief Receives and validates UDP packets, stores payloads with index.
 */
class NaluUdpReceiver {
   public:
    NaluUdpReceiver(const std::string& address, uint16_t port,
                    size_t buffer_size = 1024 * 1024,
                    size_t max_packet_size = 1040,
                    int timeout_sec = 10);

    explicit NaluUdpReceiver(const NaluUdpReceiverParams& params);
    ~NaluUdpReceiver();

    void start();
    void stop();

    NaluUdpDataBuffer& getDataBuffer();

   private:
    void initSocket();
    void receiveLoop();

    std::string address_;
    uint16_t port_;
    int socket_fd_;
    std::thread receiver_thread_;
    std::atomic<bool> running_;
    NaluUdpDataBuffer nalu_udp_data_buffer_;
    size_t max_packet_size_;
    int timeout_sec_;
};

#endif  // NALU_UDP_RECEIVER_H
